/* (c) Copyright 1991-2012 Neurosciences Research Foundation, Inc. */
/* $Id: d3gcon.c 51 2012-05-24 20:53:36Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                               d3gcon                                 *
*                                                                      *
*   CNS geometric connection evaluation routine (parallel version):    *
*   All routines in this file are to be called on comp nodes ONLY      *
*                                                                      *
*     Start-of-layer call: int d3inhb()                                *
*        Call as each layer is entered, once per cycle, to form        *
*           sum over all groups if needed for OPT=X.                   *
*        Returns 0 for compatibility with serial version, which may    *
*           return nonzero if interrupt button is pressed in d3bxvp.   *
*                                                                      *
*     Start-of-group call: void d3inhg()                               *
*        Output:                                                       *
*           This routine does group-specific calculations of scaled,   *
*           uncapped individual band sums and stores them in the       *
*           INHIBBAND structures.  It also saves separately in each    *
*           INHIBBLK the the uncapped scaled inner ring (r0sum) totals *
*           (if IBOPT=V) and the capped outer ring excitatory and      *
*           inhibitory (rnsums) totals for later use by d3inhc.        *
*                                                                      *
*     Start-of-cell call: void d3inhc()                                *
*        Output:                                                       *
*           (1) The undecayed, uncapped bandsums are returned in the   *
*           dynamic INHIBBAND structures.  All but the innermost band  *
*           (IBOPT=V) will have been calculated earlier, by d3inhg().  *
*           (2) The capped but otherwise "raw" total values applicable *
*           to the current cell are saved in CDAT.pAffData.  (3)       *
*           Values undergoing decay are passed to the next time step   *
*           in iepers,iipers.  (4) Effective values following optional *
*           decay and phasing are in CDAT.pAffData.  (5) Excitatory    *
*           and H,Q,S inhibitory totals are in CDAT.pGeomSums.         *
*                                                                      *
************************************************************************
*                                                                      *
*     See d3inhb.c for the serial versions of these routines.  The     *
*        serial versions provide the same interface via CDAT, but      *
*        internally, the sums are done by an FFT-like algorithm at     *
*        the time d3inhb() is called.  Under parallel execution, the   *
*        fancy algorithm of d3inhb is not advantageous.  Accordingly,  *
*        this program has been rewritten to accomplish the same result *
*        using a crude brute-force algorithm designed to minimize      *
*        memory usage.  There are several differences required else-   *
*        where to accommodate the two variants:                        *
*     (1) Setup relevant INHIBBLK variables in d3tchk in each case.    *
*     (2) d3bxvp cannot be implemented in the parallel version because *
*        borders are computed on-the-fly and values are not stored.    *
*                                                                      *
************************************************************************
*  Written by George N. Reeke                                          *
*  V4A, 01/27/89, SCD - Translated to 'C' from version V3F             *
*  V4B, 01/08/90, GNR - Rewrite d3inhb as d3gcon to implement          *
*                       low memory, parallel calculation               *
*  V5C, 11/09/91, GNR - Revise phasing code added earlier by MC        *
*  V5E, 07/13/92, GNR - Remove gcnsums arg, use boxsums, l1xxx,        *
*                       use proper stride when phase is present        *
*  V6D, 01/10/94, GNR - Add self-avoidance (OPT=V), d3inhc routine     *
*  V7A, 05/07/94, GNR - Correct BC=NORM bug, add COMPAT=N for old way  *
*  V8A, 09/03/95, GNR - Move falloff calcs to d3foff.c, phase calcs    *
*                       to gmphaff, control mode of inhibition per     *
*                       INHIBBLK, add decay, delay, remove COMPAT=N    *
*  Rev, 09/27/96, GNR - Variable scaling for greater falloff accuracy, *
*                       correct bug--xtotal was not subject to falloff *
*  Rev, 01/12/98, GNR - Separate excit,inhib persistence for dprnt     *
*  V8C, 03/01/03, GNR - Cell responses in millivolts, add conductances *
*  ==>, 08/29/07, GNR - Last mod before committing to svn repository   *
*  Rev, 04/23/08, GNR - Add ssck (sum sign check) mechanism & IBOPKR   *
*  Rev, 12/31/08, GNR - Replace jm64sl,jm64sh with mssle or msrsle     *
*  V8E, 05/05/09, GNR - Add gjrev, modify thresh tests to elim 0's     *
*  V8G, 08/17/10, GNR - Add autoscale adjustment to betas              *
*  V8H, 04/01/11, GNR - Add gdefer                                     *
*  Rev, 04/09/12, GNR - Add IBOPT=F, calc decay even during defer      *
*  Rev, 05/16/12, GNR - Count scaling overflows, but do not kill run   *
***********************************************************************/

#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "rkarith.h"
#include "d3global.h"
#include "celldata.h"

extern struct CELLDATA CDAT;

static long gpsum(struct INHIBBLK *ib, long gx, long gy);
static FoffFn d3foff;         /* Ptr to falloff calculation routine */
static long gpfoff;           /* Ring 0 falloff factor */

/***********************************************************************
*                                                                      *
*                               d3inhb                                 *
*                                                                      *
*     Calculate total activity in source layer for OPT=X layers.       *
*                                                                      *
***********************************************************************/

int d3inhb() {

   struct CELLTYPE *il;          /* Ptr to current CELLTYPE */
   struct INHIBBLK *ib;          /* Ptr to an INHIBBLK */
   long gx,gy;                   /* Group x,y coords */
   long lngx,lngy;               /* Max values of x,y */
   long sum;                     /* Sum of all cells' activity */

/* Loop over all INHIBBLKs for this cell layer.  If OPT=X, calculate
*  xtotal = sum of cell activity over entire repertoire.  Call gpsum
*  to get each group sum, thereby assuring consistency with sums
*  calculated in d3inhg/d3inhc.
*  Changed in V8A to apply falloff curve. */

   il = CDAT.cdlayr;
   for (ib=il->pib1; ib; ib=ib->pib) {
      if (CDAT.curr_cyc1 <= ib->gcdefer) continue;
      sum = 0;                   /* Clear total */
      if (ib->ibopt & IBOPTX) {
         d3foff = qfofffnc(ib);  /* Locate falloff routine */
         lngx = ib->l1x, lngy = ib->l1y;
         for (gy=0; gy<lngy; gy++) {
            for (gx=0; gx<lngx; gx++) {
               sum += gpsum(ib, gx, gy);
               } /* End x loop */
            } /* End y loop */
         } /* End if OPT=X */
      ib->xtotal = sum;
      } /* End loop over INHIBBLK's */

   /* Serial version may return non-zero from d3bxvp */
   return D3_NORMAL;
   } /* End d3inhb() */


/***********************************************************************
*                                                                      *
*                               d3inhg                                 *
*                                                                      *
***********************************************************************/

void d3inhg(void) {

   struct CELLTYPE *il;       /* Ptr to current CELLTYPE */
   struct INHIBBLK *ib;       /* Ptr to current INHIBBLK */
   struct INHIBBAND *pb;      /* Ptr to current INHIBBAND */
   long *pgx;                 /* Ptr to afference data */
   long *povct;               /* Ptr to overflow count */
   long gpsinside;            /* Num of groups within source bounds */
   long gpstotal;             /* Num of groups in complete rings */
   long gn,gx,gy;             /* Current group coordinates */
   long innersum;             /* Unscaled inner sum for OPT=X */
   long ovct;                 /* Overflow count if no stats */
   long wksum;                /* Working sum adjusted for min,max */
   long x,y;                  /* Working group coordinates */
   long xlo,xhi,ylo,yhi;      /* Loops limits for ring edges */
   si32 wkbeta;               /* Working value of beta (S20) */
   int  iband;                /* Number of current band */
   int  ilast;                /* Number of last band */
   int  igrp;                 /* Node-relative current group */
   int  iring,lring;          /* Number of current,last rings */
   int  laffs,p;              /* Sum distribution controls */
   int  pbsf;                 /* Post-beta shift factor */
   ui16 ccyc0;                /* Current cycle (0-based) */
   char do_norm;              /* True if BC=NORM */
   char do_expand;            /* True if OPT=X */

/* Setups before INHIBBLK loop */

   il = CDAT.cdlayr;             /* Locate CELLTYPE */
   povct = (il->Ct.kctp & CTPNS) ? &ovct : &il->CTST->gmover;
   ovct = 0;
   if (!(il->ctf & CTFGV))       /* Clear afference accumulators */
      memset((char *)CDAT.pGeomSums, 0, CDAT.lGeomSums);
   laffs = 1 << il->pdshft;
   ccyc0 = CDAT.curr_cyc1 - 1;

/* Loop over all INHIBBLKs for this cell type */

   gn = CDAT.groupn, gx = CDAT.groupx, gy = CDAT.groupy;
   igrp = CDAT.groupn - il->logrp;
   for (ib=il->pib1; ib; ib=ib->pib) {

/* If deferring this GCONN, zero bandsums and AffData for d3dprt
*  and omit calculation.  */

      if (CDAT.curr_cyc1 <= ib->gcdefer) {
         pb = ib->inhbnd;
         for (iband=0; iband<(int)ib->nib; iband++,pb++)
            pb->bandsum = 0;
         if (ib->lafd > 0) memset(
            (char *)(CDAT.pAffData+ib->ogafs), 0, (size_t)ib->lafd);
         continue;
         }

/* Locate appropriate routine for falloff calculations */

      d3foff = qfofffnc(ib);

/* Set normalization and expansion switches for this INHIBBLK.
*  If normalization is ON, compute the number of groups included
*  in the group sums for the current x,y coords (this same value
*  is used for all bands--see comment below). This number equals
*  the length of the largest ring edge that is inside the layer
*  for x, times the length of the largest ring edge that is
*  inside the layer for y, i.e.:
*     Min(NR1+1+X,NR1+1+(NGX-1-X),2*NR1+1) [for x coords] times
*     Min(NR1+1+Y,NR1+1+(NGY-1-Y),2*NR1+1) [for y coords].     */

      do_expand = ((ib->ibopt & IBOPTX) != 0);
      if (do_norm = (ib->kbc == BC_NORM)) {     /* Assignment
                                                *  intended */
         long gpsx,gpsy;                  /* Groups along x,y */
         long nrm1 = ib->l1n1;            /* Number rings - 1 */
         gpsx = gpsy = nrm1 + nrm1 + 1;   /* Full box size */
         gpstotal = ib->l1area;
         gpsx = min(gpsx, nrm1+1+gx);
         gpsx = min(gpsx, nrm1+ib->l1x-gx);
         gpsy = min(gpsy, nrm1+1+gy);
         gpsy = min(gpsy, nrm1+ib->l1y-gy);
         gpsinside = gpsx*gpsy;
         } /* End setup for normalization */

/* Loop over bands */

      ib->rnsums[EXCIT] = ib->rnsums[INHIB] = innersum = 0;
      pb = ib->inhbnd;              /* Locate band data */
      ilast = ib->nib - 1;
      iring = lring = 0;            /* First band has but one ring */
      pbsf  = (int)ib->ihsf - 23;   /* Post-beta scale factor */
      e64push(E64_COUNT, povct);
      for (iband=0; iband<=ilast; iband++, pb++) {

/* If OPT=X and about to do last band,
*  calculate bandsum by subtraction and kill normalization */

         if (do_expand && (iband == ilast)) {
            pb->bandsum = ib->xtotal - innersum;
            do_norm = OFF; }

/* All other cases, perform normal band sum */

         else {

            /* If beta is zero and no OPT=X, we can skip this one */
            pb->bandsum = 0;
            if (pb->asbeta || do_expand) {

/* Perform sum over rings */

               for (; iring<=lring; iring++) {

/* Perform sum for central ring, which degenerates to a single group */

                  if (!iring) pb->bandsum = gpsum(ib,gx,gy);

/* For each ring other than the first, scan over two rows of groups
*  along the top and bottom of the ring, then two shorter rows along
*  the sides */

                  else {

                     xlo = gx - iring; xhi = gx + iring;
                     ylo = gy - iring; yhi = gy + iring;

                     /* Rows at bottom and top of ring */
                     for (x=xlo; x<=xhi; x++) pb->bandsum +=
                        (gpsum(ib,x,ylo) + gpsum(ib,x,yhi));

                     /* Rows at left and right of ring */
                     for (y=ylo+1; y<yhi; y++) pb->bandsum +=
                        (gpsum(ib,xlo,y) + gpsum(ib,xhi,y));

                     } /* End scan around a ring */

                  } /* End sum over rings */

               innersum += pb->bandsum;
               } /* End if asbeta != 0 or expand */
            } /* End calculation of bandsum */

/* Normalize, scale, and cap the band sums.
*     For OPT=X, capping and scaling is performed without normalization
*        as the center hole should not be expanded outside the bounds
*        included in xtotal.
*     This algorithm is incorrect for the case where the boundary
*        condition is other than zero or toroidal.  In this version,
*        these cases are rejected by input checking in g2gconn. It is
*        not exactly clear how the other cases would be defined, but
*        probably one would set the border to zero, then go back and
*        repeat all the ring sums, subtracting from last band sum.  */

         if (pb->asbeta) {

/* Apply normalization to beta factor if required.  This code is a
*  corrected version of that used in the IBM Assembler Darwin III.
*  It still normalizes all bands by the same overall area ratio
*  that applies to the entire geometric connection box, but now
*  the ratio is to the size of the inhibitory rings, not the full
*  size of the input repertoire.  Further thought should be given
*  to normalizing each band by its own area ratio.  */

            /* Normalized beta = beta*(total area)/(used area) */
            wkbeta = (do_norm) ?
               dm64nq(pb->asbeta, gpstotal, gpsinside) : pb->asbeta;

/* Scale total bandsum for current group and save for statistics.  If
*  IBOEFV, save scaled inner band sum and its scale factor for d3inhc.
*  It is not necessary to zero r0sum when beta[0]=0, as IBOEFV will be
*  turned off in these cases.  Calculate scaled, capped total of
*  remaining bands and accumulate in rnsums.  If not IBOEFV, treat all
*  bands this way.  Dynamic scaling is used to improve accuracy.
*  If ib->ihsf = log2(max terms in sum - 1), sum is (S23-ihsf), times
*  beta (S20) yields S(43-ihsf), rescaled to (S20) for storage.  */

            pb->bandsum = wksum =
               msrsle(pb->bandsum, wkbeta, pbsf, OVF_GEOM);
            if (iband > 0 || !(ib->ibopt & IBOEFV)) {
               /* Outer band or any band when not IBOEFV */
               if (wksum >= 0)
                  ib->rnsums[EXCIT] += min(wksum, pb->mxib);
               else
                  ib->rnsums[INHIB] += max(wksum, -(pb->mxib));
               }
            else {
               /* Scaled inner band and combined scale factor */
               ib->r0sum = wksum;
               ib->r0scale = mssle(wkbeta, gpfoff, -FBfo, OVF_GEOM);
               }
            } /* End calculation of bandsum (asbeta != 0) */
         else
            pb->bandsum = 0;  /* For detailed print */

         iring = lring + 1;   /* (Needed in case band skipped) */
         lring += ib->ngb;    /* Set for next band of rings */
         } /* End loop over bands */
      e64pop();

/* All bands for this INHIBBLK are now done.  If CTFGV, items needed
*  for individual cell calculations have been stored, there is no more
*  to do here.  Otherwise, finish up for current group, handling excit
*  and inhib contributions separately, and using d3decay to apply
*  decay.  If celltype has phase, distribute gcon uniformly, otherwise
*  there is just one term.  */

      if (il->ctf & CTFGV) continue;

      pgx = CDAT.pAffData + ib->ogafs;

      if (ib->gssck & PSP_POS) {                   /* EXCIT terms */

         long *pSums = CDAT.pAffData + ib->ogsei[EXCIT];
         long gaxp = *pgx++ = ib->rnsums[EXCIT];
         /* N.B. 04/19/12, GNR - Here w/OPT=F we store current value
         *  as persistence for next time but do not add omega times
         *  it into itself--to give a more gentle startup.  */
         if (ib->GDCY.omega) {                     /* Apply decay */
            if (CDAT.curr_cyc1 == ib->gfscyc)
               *pgx++ = ib->ieveff[gn] = gaxp;
            else
               gaxp = d3decay(&ib->GDCY, ib->ieveff+gn, gaxp, pgx++);
            }
         *pgx++ = gaxp;
         if (ccyc0 >= ib->gdefer) {
            for (p=0; p<laffs; p++) pSums[p] += gaxp;
            }

         } /* End excitatory terms */

      if (ib->gssck & PSP_NEG) {                   /* INHIB terms */

         long *pSums = CDAT.pAffData + ib->ogsei[INHIB];
         long gaxn = *pgx++ = ib->rnsums[INHIB];
         if (ib->GDCY.omega) {                     /* Apply decay */
            if (CDAT.curr_cyc1 == ib->gfscyc)
               *pgx++ = ib->iiveff[gn] = gaxn;
            else
               gaxn = d3decay(&ib->GDCY, ib->iiveff+gn, gaxn, pgx++);
         *pgx++ = gaxn;
         if (ccyc0 >= ib->gdefer) {
            for (p=0; p<laffs; p++) pSums[p] += gaxn;
            }

         } /* End inhibitory terms */

      } /* End loop over INHIBBLKs */

   } /* End d3inhg() */


/***********************************************************************
*                                                                      *
*                               d3inhc                                 *
*                                                                      *
***********************************************************************/

void d3inhc(void) {

   struct CELLTYPE *il;       /* Pointer to current CELLTYPE */
   struct INHIBBLK *ib;       /* Pointer to current INHIBBLK */
   struct INHIBBAND *pb;      /* Pointer to current INHIBBAND */
   long *pgx;                 /* Pointer to afference distribution */
   long gaxp,gaxn;            /* Positive,negative sums */
   int  laffs,p;
   ui16 ccyc0;                /* Current cycle (0-based) */

/* If no INHIBBLK has effective self-avoidance (IBOEFV), return at
*  once, all work has been done in d3inhg().  (This is just a safety,
*  we should not be called from d3go() in this case.)  */

   il = CDAT.cdlayr;
   if (!(il->ctf & CTFGV)) return;

/* Otherwise, it is necessary to process all INHIBBLKs, not just
*  the ones with IBOEFV set, because we need to clear the GeomSums
*  and add all terms into them--there is no place to store partial
*  sums from the INHIBBLKs that do not have IBOEFV set, alas.  */

   memset((char *)CDAT.pGeomSums, 0, CDAT.lGeomSums);
   laffs = 1 << il->pdshft;
   ccyc0 = CDAT.curr_cyc1 - 1;
   for (ib=il->pib1; ib; ib=ib->pib)
         if (CDAT.curr_cyc1 > ib->gcdefer) {

/* Initialize totals from d3inhg() data for outer bands */

      pb = ib->inhbnd;
      gaxp = ib->rnsums[EXCIT];
      gaxn = ib->rnsums[INHIB];

/* If this is an INHIBBLK with effective self-avoidance, and if
*  the activity of the current cell at (t-1) exceeds itt or ittlo
*  thresholds, scale it and subtract from saved central ring sum
*  (algebraic add--neg sums stored).  Save this as the adjusted
*  first band sum.  */

      if (ib->ibopt & IBOEFV) {

         long litt, work, wksum = ib->r0sum;
         long effsi = CDAT.old_siS7 - (long)ib->gjrev;
         litt = (long)ib->itt; if (litt) litt -= 1;
         if (effsi > litt) {
            work = effsi - (long)ib->subitt;
            goto UseSiVal;
            }
         litt = (long)ib->ittlo; if (litt) litt += 1;
         if (effsi < litt) {
            work = effsi - (long)ib->subnitt;
            goto UseSiVal;
            }
         goto OmitSiVal;
UseSiVal:
         wksum += msrsle(work, ib->r0scale, -FBsi, OVF_GEOM);
         pb->bandsum = wksum;

         /* Apply cap to inner band sum and add into outer band sums */
         if (wksum >= 0)
            gaxp += min(wksum, pb->mxib);
         else
            gaxn += max(wksum, -(pb->mxib));
         } /* End adjusting for current cell */
OmitSiVal: ;

/* Apply decay, store detail info and accumulate sums according
*  to mode of excitation or inhibition, cf. d3inhg().  */

      pgx = CDAT.pAffData + ib->ogafs;

      if (ib->gssck & PSP_POS) {                   /* EXCIT terms */

         long *pSums = CDAT.pAffData + ib->ogsei[EXCIT];
         *pgx++ = gaxp;
         if (ib->GDCY.omega) {                     /* Apply decay */
            if (CDAT.curr_cyc1 == ib->gfscyc)
               *pgx++ = ib->ieveff[CDAT.cdcell] = gaxp;
            else gaxp = d3decay(&ib->GDCY, ib->ieveff+CDAT.cdcell,
               gaxp, pgx++);
            }
         *pgx++ = gaxp;
         if (ccyc0 >= ib->gdefer) {
            for (p=0; p<laffs; p++) pSums[p] += gaxp;
            }

         } /* End excitatory terms */

      if (ib->gssck & PSP_NEG) {                   /* INHIB terms */

         long *pSums = CDAT.pAffData + ib->ogsei[INHIB];
         *pgx++ = gaxn;
         if (ib->GDCY.omega) {                     /* Apply decay */
            if (CDAT.curr_cyc1 == ib->gfscyc)
               *pgx++ = ib->iiveff[CDAT.cdcell] = gaxn;
            else gaxn = d3decay(&ib->GDCY, ib->iiveff+CDAT.cdcell,
               gaxn, pgx++);
            }
         *pgx++ = gaxn;
         if (ccyc0 >= ib->gdefer) {
            for (p=0; p<laffs; p++) pSums[p] += gaxn;
            }

         } /* End inhibitory terms */

      } /* End INHIBBLK loop */

   } /* End d3inhc() */


/***********************************************************************
*                                                                      *
*                            gpsum(ib,x,y)                             *
*                                                                      *
*     Calculate group sum for given x,y position on scale (S23-SF).    *
*     Leave falloff factor in gpfoff for use by caller.                *
*     Caller must set d3foff = qfofffnc(ib) before calling gpsum().    *
***********************************************************************/

long gpsum(struct INHIBBLK *ib, long x, long y) {

/* Local declarations */

   struct CELLTYPE *jl = ib->pisrc; /* Ptr to parent CELLTYPE */
   s_type *pi;                      /* Ptr to s(i) value in group */
   s_type *pi1,*pi2;                /* Ptr to starting, ending cells
                                    *  in current group */
   long wksum,sum = 0;              /* Working sum, final sum */
   long ligrp;                      /* Len of s(j) data for 1 grp */
   long lit = (long)ib->itt;        /* Local inhibition threshs */
   long lnt = (long)ib->ittlo;
   long lsit = (long)ib->subitt;    /* Local subtracted threshs */
   long lsnt = (long)ib->subntt;
   long litr = (long)ib->gjrev;     /* Local rev potl adjuster */
   long work,simit;                 /* Contribution from one cell */
   int  sfs;                        /* Adjusted shift factor */

/* Determine falloff factor, if any, using coordinates before any
*  boundary condition is applied.  */

   gpfoff = d3foff(ib, x, y);

/* Analyze whether the coordinates are out-of-bounds, and, if so,
*  reduce to internal coordinates by applying boundary conditions.
*  Note: Setup code guarantees that no coordinate can be more than
*  one box diameter away from the nearest edge. */

   if (x < 0) {               /* x left of cell layer */

      switch ((enum BoundaryCdx)ib->kbc) {

      case BC_ZERO:              /* Zero */
      case BC_NORM:              /* Normalized */
         return 0;

      case BC_NOISE:             /* Noise */
         sum = ib->border;
         goto fall_off_corr;

      case BC_EDGE:              /* Repeat nearest edge */
         x = 0;
         break;

      case BC_MIRROR:            /* Mirror at nearest edge */
         x = -(1 + x);
         break;

      case BC_TOROIDAL:          /* Toroidal */
         x += ib->l1x;
         break;

         } /* End BC switch for x<0 */
      } /* End if (x < 0) */

   else if (x >= ib->l1x) {      /* x right of cell layer */

      switch ((enum BoundaryCdx)ib->kbc) {

      case BC_ZERO:              /* Zero */
      case BC_NORM:              /* Normalized */
         return 0;

      case BC_NOISE:             /* Noise */
         sum = ib->border;
         goto fall_off_corr;

      case BC_EDGE:              /* Repeat nearest edge */
         x = ib->l1x - 1;
         break;

      case BC_MIRROR:            /* Mirror at nearest edge */
         x = ib->l1x + ib->l1x - x - 1;
         break;

      case BC_TOROIDAL:          /* Toroidal */
         x -= ib->l1x;
         break;

         } /* End BC switch for x<0 */
      } /* End if (x >= ib->l1x) */

   if (y < 0) {               /* y above cell layer */

      switch ((enum BoundaryCdx)ib->kbc) {

      case BC_ZERO:              /* Zero */
      case BC_NORM:              /* Normalized */
         return 0;

      case BC_NOISE:             /* Noise */
         sum = ib->border;
         goto fall_off_corr;

      case BC_EDGE:              /* Repeat nearest edge */
         y = 0;
         break;

      case BC_MIRROR:            /* Mirror at nearest edge */
         y = -(1 + y);
         break;

      case BC_TOROIDAL:          /* Toroidal */
         y += ib->l1y;
         break;

         } /* End BC switch for y<0 */
      } /* End if (y < 0) */

   else if (y >= ib->l1y) {      /* y right of cell layer */

      switch ((enum BoundaryCdx)ib->kbc) {

      case BC_ZERO:              /* Zero */
      case BC_NORM:              /* Normalized */
         return 0;

      case BC_NOISE:             /* Noise */
         sum = ib->border;
         goto fall_off_corr;

      case BC_EDGE:              /* Repeat nearest edge */
         y = ib->l1y - 1;
         break;

      case BC_MIRROR:            /* Mirror at nearest edge */
         y = ib->l1y + ib->l1y - y - 1;
         break;

      case BC_TOROIDAL:          /* Toroidal */
         y -= ib->l1y;
         break;

         } /* End BC switch for y<0 */
      } /* End if (y >= ib->l1y) */

/* Add up the activity off all the cells in the group at the reduced
*  coordinates, applying the inhibition thresholds.  Adjust lit,lnt
*  to reject zero terms.  */

   ligrp = spsize(jl->nel,jl->phshft);
   if (lit) lit -= 1;
   if (lnt) lnt += 1;
   pi1 = jl->pps[ib->ihdelay] + (y*ib->l1x + x)*ligrp;
   pi2 = pi1 + ligrp;
   wksum = 0;
   for (pi=pi1; pi<pi2; pi+=jl->lsp) {
      d3gts2(work, pi);
      work -= litr;
      if      (work > lit) wksum -= (work - lsit);
      else if (work < lnt) wksum -= (work - lsnt);
      }
   /* If requested, square the boxsums.  Rescale by dS14 and group
   *  size to keep in range, NEL*(SUM/NEL)^2 = SUM^2/NEL.  With
   *  mV scales, sum can be +/-, so restore sign after squaring.  */
   if (ib->ibopt & IBOPGS) {
      double dwks = (double)wksum, dnel = dS14*(double)jl->nel;
      wksum = (long)(dwks * dwks / dnel);
      if (dwks < 0.0) wksum = -wksum;
      }
   /* Sum needs to be on scale S23 - ib->ihsf, but wksum is S7 */
   sfs = Ss2hi - (int)ib->ihsf;
   sum += (sfs >= 0) ? wksum << sfs : SRA(wksum, -sfs);

/* Correct the sum using the falloff factor calculated above */

   if (ib->ibopt & IBOPFO)
      sum = msrsle(sum, gpfoff, - FBfo, OVF_GEOM);

/* Return the result */

   return sum;
   } /* End gpsum() */
