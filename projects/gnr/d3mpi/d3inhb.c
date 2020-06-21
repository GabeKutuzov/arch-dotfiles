/* (c) Copyright 1989-2016, The Rockefeller University *21116* */
/* $Id: d3inhb.c 70 2017-01-16 19:27:55Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                               d3inhb                                 *
*             CNS geometric connection evaluation routine              *
*                                                                      *
*     Start-of-layer call: d3inhb()                                    *
*        Output:  The scaled, uncapped individual band contributions   *
*           in nib blocks of mgrp si64 words beginning at bandsum.     *
*        Calls d3bxvp if requested to plot inhibitory surround boxes.  *
*           Returns nonzero if interrupt button pressed in d3bxvp      *
*           (signal to terminate execution), otherwise zero.           *
*                                                                      *
*     Start-of-group call: d3inhg()                                    *
*        Output:                                                       *
*           This routine looks up scaled, uncapped band sums for the   *
*           current group and stores the inner ring (r0sum) totals     *
*           (if IBOPT=V) and the capped outer ring excitatory and      *
*           inhibitory (rnsums) totals for later use by d3inhc.        *
*                                                                      *
*     Start-of-cell call: void d3inhc()                                *
*        Output:                                                       *
*           (1) The undecayed, uncapped bandsums are returned in the   *
*           dynamic INHIBBAND structures.  All but the innermost band  *
*           (IBOPT=V) will have been calculated earlier, by d3inhg().  *
*           (2) The scaled and capped but otherwise "raw" total values *
*           applicable to the current cell are saved in CDAT.pAffD32.  *
*           (3) Values undergoing decay are passed to the next time    *
*           step in GDCY.peipv.  (4) Effective excitatory and H,Q,S    *
*           inhibitory values following optional decay and phasing     *
*           are in CDAT.pGeomSums.                                     *
*                                                                      *
*     Formerly used on serial computers only, this code has been       *
*     revised in R65 to handle parallel also.  Now that enough memory  *
*     is likely to be available, the three calls have the same inputs  *
*     and outputs.  This algorithm can be modified later to handle     *
*     circular neighborhoods on both kinds of computers.  Because of   *
*     the need to do xtotal sum over all groups, the group sum code,   *
*     with all its thresholding and squaring, is now a separate func-  *
*     tion (as it was in d3gcon).                                      *
*                                                                      *
************************************************************************
*  Written by George N. Reeke                                          *
*  V4A, 01/27/89, SCD - Translated to 'C' from version V3F             *
*  Rev, 11/01/90, GNR - Pass d3bxvp return code through                *
*  V5E, 07/05/92, GNR - Add d3inhg to handle phase                     *
*  Rev, 08/08/92, GNR - Handle compilers that do logical right shifts  *
*  V6D, 01/10/94, GNR - Add self-avoidance (OPT=V), d3inhc routine,    *
*                       update calc of falloff to agree with par vers. *
*  V7A, 05/01/94, GNR - Add delay, correct bandsum bug with OPT=X,     *
*                       correct BC=NORM bug, add COMPAT=N for old way  *
*  V8A, 09/03/95, GNR - Move falloff calcs to d3foff.c, phase calcs    *
*                       to gmphaff, control mode of inhibition per     *
*                       INHIBBLK, add decay, remove COMPAT=N           *
*  Rev, 09/27/96, GNR - Variable scaling for greater falloff accuracy, *
*                       consolidate two loops over INHIBBLKs in d3inhb *
*  V8C, 03/01/03, GNR - Cell responses in millivolts, add conductances *
*  ==>, 09/29/06, GNR - Last mod before committing to svn repository   *
*  Rev, 04/23/08, GNR - Add ssck (sum sign check) mechanism & IBOPKR   *
*  Rev, 12/31/08, GNR - Replace jm64sl,jm64sh with mssle or msrsle     *
*  V8E, 05/05/09, GNR - Add gjrev, modify thresh tests to elim 0's     *
*  V8G, 08/17/10, GNR - Add autoscale adjustment to betas              *
*  V8H, 04/01/11, GNR - Add gdefer                                     *
*  Rev, 04/09/12, GNR - Add IBOPT=F, calc decay even during defer      *
*  Rev, 05/16/12, GNR - Count scaling overflows, but do not kill run   *
*  Rev, 01/09/13, GNR - Revised d3decay args, use fsdecay              *
*  Rev, 04/20/13, GNR - Go to 64-bit sums, eliminate ihsf              *
*  Rev, 05/01/13, GNR - Add IBOPCS, explicitly disallow IBOPGS+IBOPSA  *
*  Rev, 05/11/13, GNR - Remove gconn falloff                           *
*  R65, 01/11/16, GNR - Merge parallel into serial code, move boxsum   *
*                       and strip ptrs here from inhibblk              *
*  R66, 01/30/16, GNR - Inhib betas are now > 0 excit, < 0 inhib,      *
*                       implement IBOPMX option, remove IBOPGS         *
*  R67, 11/02/16, GNR - Correct IBOPRS code for case ngb > 1           *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "sysdef.h"
#include "rkarith.h"
#include "d3global.h"
#include "celldata.h"
#include "rocks.h"
#ifdef PARn
#include "rksubs.h"
#else
#include "rkxtra.h"
#endif

/* Define DBGBSUMS to activate bandsums debug output */
/*** #define DBGBSUMS ***/
/* Define DBGBSLN as quoted layer name for DBGBSUMS output */
#define DBGBSLN "F2"
#ifdef DBGBSUMS
#if LNSIZE < MAX_EMSG_LENGTH
#define LDBPSLN LNSIZE
#else
#define LDBPSLN MAX_EMSG_LENGTH
#endif
/* Function to print a rectangular block of B20/27 si64 sums */
static void dbpsums(si64 *sums, ui32 nitms, ui32 ncols, ui32 roff) {
   si64 *sumc = sums + ncols - roff, *sume = sums + nitms;
   ui32 bs = ((20+RP->bsdc)<<RK_BS)+(4<<RK_DS)+(RK_IORF|RK_NI64)+7;
   char dmsg[LDBPSLN];
   roff = 8*(roff%(LDBPSLN/8));   /* JIC */
   memset(dmsg, ' ', roff);
   while (sums < sume) {
      int newrow = sums >= sumc;
      if (newrow) sumc += ncols;
      if (roff+8 > LDBPSLN || newrow) {
         dmsg[roff] = '\0';
#ifdef PAR
         dbgprt(dmsg);
#else
         cryout(RK_P4, dmsg, RK_LN1, NULL);
#endif
         roff = 0;
         }
#ifdef PARn
      wbcdwtPn(sums++, dmsg+roff, bs);
#else
      wbcdwt(sums++, dmsg+roff, bs);
#endif
      roff += 8;
      }
   dmsg[roff] = '\0';
#ifdef PAR
   dbgprt(dmsg);
#else
   cryout(RK_P4, dmsg, RK_LN1, NULL);
#endif
   } /* End dbpsums() */
#endif

extern struct CELLDATA CDAT;

/*=====================================================================*
*                               grpsum                                 *
*=====================================================================*/

static si64 grpsum(struct INHIBBLK *ib, s_type *pcell) {

   struct CELLTYPE *jl = ib->pisrc;
   s_type *plimit;                  /* Fence above cells in group */
   si64 wksum = jesl(0);
   size_t ligrp = (size_t)spsize(jl->nel, jl->phshft);
   si32 lit = (si32)ib->effitt;     /* Local inhibition threshs */
   si32 lnt = (si32)ib->effnitt;
   si32 lsit = (si32)ib->subitt;    /* Local subtracted threshs */
   si32 lsnt = (si32)ib->subnitt;
   si32 litr = (si32)ib->gjrev;     /* Local rev potl adjuster  */
   int  sjincr = jl->lsp;

   plimit = pcell + ligrp;

   /* If computing max instead of sum, do it here */
   if (ib->ibopt & IBOPMX) {
      si32 smax = -SI32_MAX;
      for ( ; pcell < plimit; pcell+=sjincr) {
         si32 work;
         d3gtjs2(work, pcell);
         work -= litr;
         if      (work > lit) work -= lsit;
         else if (work < lnt) work -= lsnt;
         else continue;
         if (work > smax) smax = work;
         }
      /* Scaling:  Shift S7/14 to S20/27 and also multiply by nel
      *  to bring on same scale as sums because beta has built-in
      *  division by nel (cannot overflow going to si64).  */
      wksum = jssw(jmsw(smax, jl->nel), FBwk-FBsi);
      }

   /* If squaring individual s(i), perform sum in a separate loop to
   *  avoid testing the IBOPCS flag inside the loop.  This new mode is
   *  compatible with self-avoidance later because s(i) squares do not
   *  interact.  Sum of squares cannot overflow:  16-bit work * 16-bit
   *  work * 32-bit nel */
   else if (ib->ibopt & IBOPCS) {
      for ( ; pcell < plimit; pcell+=sjincr) {
         si64 tssq;
         si32 work;
         d3gtjs2(work, pcell);
         work -= litr;
         if      (work > lit) work -= lsit;
         else if (work < lnt) work -= lsnt;
         else continue;
         tssq = jmsw(work, absj(work));
         wksum = jasw(wksum, tssq);
         }
      /* Scaling:  S14 sums are now S28, shift 1 to S27.
      *  mV S7 sums are now S14.  Left shift 6 to S20, but
      *  also divide by 128 per doc ==> shift 1 to S20.  */
      wksum = jsrrsw(wksum, 1);
      }

   /* Otherwise, just sum the cell activities.
   *  R65, 01/16/16, GNR - Improve scaling for accuracy.
   *  R66, 02/02/16, GNR - Remove old IBOPGS (sum then square)  */
   else {
      for ( ; pcell < plimit; pcell+=sjincr) {
         si32 work;
         d3gtjs2(work, pcell);
         work -= litr;
         if      (work > lit) wksum = jasl(wksum, work - lsit);
         else if (work < lnt) wksum = jasl(wksum, work - lsnt);
         }
      /* Scale wksum from S7/14 to S20/27 */
      wksum = jslsw(wksum, (FBwk-FBsi));
      }

   return wksum;
   } /* End grpsum() */


/*=====================================================================*
*                                                                      *
*                               d3inhb                                 *
*                                                                      *
*=====================================================================*/

int d3inhb(void) {

   struct CELLTYPE *il = CDAT.cdlayr,*jl; /* Ptr to current cell type */
   struct INHIBBLK *ib;          /* Ptr to current INHIBBLK */
   si64 *boxsum;                 /* Ptr to allocated boxsum area */
   /* Next 6 vars point to start of boxsum rows, left of left border */
   si64 *boxsumlo;               /* Ptr to low in-region boxsum */
   si64 *boxsum00;               /* Ptr to low in-node boxsum */
   si64 *boxsumbb;               /* Ptr to in-node boxsum fence */
   si64 *boxsumhi;               /* Ptr to in-region boxsum fence */
   si64 *hstrip;                 /* Ptr to horizntl summation strip */
   si64 *vstrip;                 /* Ptr to vertical summation strip */
   si64 *bptr;                   /* Ptr into boxsum array */
   si64 *beptr;                  /* Ptr into end of boxsum array */
   s_type *pcell;                /* Ptr to first cell in a group */
#ifndef PAR0
   struct XYNORMDEF *xy;         /* Ptr to normalization table */
   si64 *pbndsum;                /* Pointer into bandsum array */
   si64 *xeptr;                  /* Limit for bptr,xptr loops */
   si64 xtotal;                  /* Total of boxsums for OPT=X */
#endif
   size_t nbytes;                /* Number of bytes to copy */
   ui32 logrpy = (ui32)il->logrpy;     /* Low group y on this node */
   ui32 irngx = (ui32)il->pback->ngx;  /* Groups along x */
   ui32 irngy = (ui32)il->pback->ngy;  /* Groups along y */
   ui32 mgrp  = il->higrp - il->logrp; /* Groups on this node */

/* Loop over all INHIBBLKs associated with this cell type */

   for (ib=il->pib1; ib; ib=ib->pib) {

      /* If on Node 0 this code is only needed for box plot */
#if defined(PAR0) && !defined(DBGBSUMS)
      if (!(RP->kpl & KPLEDG)) continue;
#endif

      /* If deferring (and no decay or decay-fast-start) this GCONN,
      *  zero bandsums and AffData for d3dprt and omit calculation. */
      if (CDAT.curr_cyc1 <= ib->gcdefer) {
         memset(ib->inhbnd[0].bandsum, 0, ib->nib*mgrp*sizeof(si64));
         if (ib->lafd > 0) memset(
            (char *)(CDAT.pAffD32+ib->ogafs), 0, (size_t)ib->lafd);
         }
      else {   /* Retain single indenting of following code ... */

      /* Make local copies of selected variables */
      size_t ligrp;                    /* Len s(j) data for 1 grp  */
      ui32 lorgrp,hirgrp;      /* Relative 1st group in boxsum row */
      ui32 nrm1     = (ui32)ib->l1n1;  /* Number of rings minus 1  */
      ui32 l2n1     = nrm1+nrm1;       /* 2*(nr-1)                 */
      ui32 l1xn1    = irngx+nrm1;      /* ngx+nr-1                 */
      ui32 l1xn2    = ib->l1xn2;       /* ngx+2*(nr-1)             */
      ui32 l1n1xn2  = ib->l1n1xn2;     /* (nr-1)*(ngx+2*(nr-1))    */
      ui32 l1xyn2   = ib->l1xyn2;      /* ngx*(mgy+2*(nr-1))       */
      ui32 l1yxn2   = ib->l1yxn2;      /* mgy*(ngx+2*(nr-1))       */
      ui32 l1xn2yn2 = ib->l1xn2yn2;    /* (ngx+l2n1)*(mgy+l2n1)    */
#ifndef PAR0
      ui32 nibm1    = ib->nib - 1;     /* Number of bands minus 1  */
      ui32 l1n1x    = ib->l1n1x;       /* (nr-1)*ngx               */
      int do_norm;                     /* TRUE if BC=NORM          */
      int iband;                       /* Band index               */
      int iring,lring;                 /* Ring index, last ring    */
      int opt_eq_X = ((ib->ibopt & IBOPTX) != 0); /* TRUE if OPT=X */
#endif
      int sjincr;                      /* Stride for Sj            */
      int torwrap = 0;                 /* Control for toroid wrap  */

      e64dec((int)ib->iovec);

      /* Compute starting coordinates of boxsums.  The setting of
      *  torwrap here is somewhat obscure (noting that the copies at
      *  BC_TOROIDAL fill faster if info already avail at other end):
      *  -Need calc at bottom if boxsumlo > boxsum && boxsumhi > hstrip
      *  -Need calc at top    if boxsumhi < hstrip && boxsumlo < boxsum
      *  It is not possible for both extra calcs to be needed in one
      *  situation, as that would be covered by BC_TOROIDAL fills.
      *  R67, 10/25/16, GNR - Revised to handle == cases correctly  */

      boxsum = RP->boxsum;
      hstrip = boxsum + l1xn2yn2;
      vstrip = hstrip + l1xyn2;
      boxsum00 = boxsum + l1n1xn2;
      boxsumlo = boxsum00 - l1xn2*logrpy;
      boxsumbb = boxsum00 + l1yxn2;
      boxsumhi = boxsum00 + l1xn2*(irngy - logrpy);
      if ((enum BoundaryCdx)ib->kbc == BC_TOROIDAL) {
         if      (boxsumlo > boxsum && boxsumhi > hstrip) torwrap = 2;
         else if (boxsumhi < hstrip && boxsumlo < boxsum) torwrap = -2;
         }
      if (boxsumlo < boxsum) boxsumlo = boxsum;
      if (boxsumhi > hstrip) boxsumhi = hstrip;
      lorgrp = il->logrp % irngx;
      hirgrp = il->higrp % irngx;
      if (hirgrp == 0) hirgrp = irngx; /* Handle full row case */

/*---------------------------------------------------------------------*
*                                                                      *
*     Stage I--Form the box sums (sums over cells in each group)       *
*                                                                      *
*  Rev, 01/13/16, GNR - For parallel, only sum groups on this node     *
*---------------------------------------------------------------------*/

/* Clear the boxsum array.
*     (Assume a new INHIBBLK will always draw from a new
*     source layer, otherwise why would it be there?--
*     therefore, cannot reuse boxsums from previous block.) */

      memset((char *)(boxsum), 0, l1xn2yn2*sizeof(si64));

/* Carry out box sums.  As of V8I, sums are 64-bit, eliminating need
*  for dynamic scaling.  */

      jl = ib->pisrc;
      ligrp = (size_t)spsize(jl->nel,jl->phshft);
      sjincr = jl->lsp;
      /* Ptr to cell data for first cell in row at boxsumlo
      *  (any ligrp*coff overflow would have been caught in d3nset) */
      {  ui32 coff = irngx*(logrpy > nrm1 ? logrpy - nrm1 : 0);
         pcell = jl->pps[ib->ihdelay] + ligrp*coff;
         } /* End coff local scope */
      bptr = boxsumlo + nrm1;
Toroid_Far_Edge_Restart:
      for ( ; bptr<boxsumhi; bptr+=l2n1) {
         beptr = bptr + irngx;
         while (bptr < beptr) {     /* Loop over x */
            *bptr++ = grpsum(ib, pcell);
            pcell += ligrp;
            }  /* End of X loop */
         }  /* End of Y loop */
      /* In case of toroidal boundary condition on a parallel computer,
      *  it may be necessary to evaluate a second block of groups from
      *  the opposite edge of the region.  At the end of this code,
      *  boxsumlo and boxsumhi indicate boundaries within which
      *  data are available for building border regions.  */
      switch (torwrap) {
      case -2:    /* Fill in top of region with cells from bottom */
         boxsumhi = hstrip;
         pcell = jl->pps[ib->ihdelay];
         torwrap = 0;      /* No cleanup needed */
         goto Toroid_Far_Edge_Restart;
      case 2:     /* Fill in bottom with cells from top */
         pcell = jl->pps[ib->ihdelay] + ligrp * irngx *
            (irngy + logrpy - nrm1);
         boxsumhi = boxsumlo;
         bptr = (boxsumlo = boxsum) + nrm1;
         torwrap = 1;      /* Do cleanup when loop done */
         goto Toroid_Far_Edge_Restart;
      case 1:     /* Cleanup after bottom fill-in sums */
         boxsumhi = hstrip;
      default:    /* Done */
         break;
         } /* End torwrap switch */

/*---------------------------------------------------------------------*
*                                                                      *
*     Stage II--Fill in borders of boxsum array                        *
*        according to selected boundary conditions                     *
*                                                                      *
*---------------------------------------------------------------------*/

      switch ((enum BoundaryCdx)ib->kbc) {

case BC_ZERO:     /* Zero boundary */
         break;   /* End of case 0 */

case BC_NORM:     /* Norm */
         break;   /* End of case 1 */

case BC_NOISE:    /* Noise boundary */

/* In first loop, noise value is stored in all boxes from start of
*     boxsum array up to boxsumlo.
*  A second loop fills in from boxsumhi to end of boxsum array.
*  If mgy > 1, a third loop fills in the sides in swatches of
*     2*(nr-1) boxes that cover one row at the right and the
*     next row at the left in one contiguous piece.  */

      {  si64 border64 = ib->border;   /* Noise sum of nel cells */
         boxsumlo += nrm1, boxsumhi -= nrm1;
         for (bptr=boxsum; bptr<boxsumlo; ++bptr) *bptr = border64;
         for (bptr=boxsumhi; bptr<hstrip; ++bptr) *bptr = border64;
         bptr = boxsumlo;
         while ((bptr+=irngx) < boxsumhi) {
            beptr = bptr + l2n1;       /* Limit for double swatch */
            while (bptr < beptr) *bptr++ = border64;
            }
         } /* End BC_NOISE local scope */
         break;  /* End of case 2 */

case BC_EDGE:   /* Nearest edge boundary */

/* First the left and right edges are filled in with
*     duplicates of the adjacent box inside the border.
*  Then the top and bottom are filled in by copying the
*     nearest adjacent full row (including left/right edges).  */

         for (bptr=boxsumlo+nrm1; bptr<boxsumhi; bptr+=l1xn2) {
            si64 *leftlim = bptr;
            si64 left_edge_value = *bptr;
            si64 right_edge_value = *(bptr + irngx - 1);

            /* Complete one pair of side swatches */
            bptr -= nrm1;  /* Back up to first storage location */
            while (bptr < leftlim) {
               *(bptr + l1xn1) = right_edge_value;
               *bptr++ = left_edge_value;
               }
            }

         /* Top/bottom moves--move whole rows with memcpy */
         nbytes = (size_t)l1xn2 * sizeof(si64);
         beptr = boxsumlo;
         for (bptr=boxsum; bptr<beptr; bptr+=l1xn2)
            memcpy(bptr, beptr, nbytes);
         beptr = boxsumhi - l1xn2;
         for (bptr=hstrip-l1xn2; bptr>beptr; bptr-=l1xn2)
            memcpy(bptr, beptr, nbytes);
         break;   /* End of case 3 */

case BC_MIRROR:   /* Mirrors at boundaries */

/* First the left and right edges are filled in with
*     mirror points from inside the border.
*  Then the top and bottom are filled in by copying full
*     rows through the mirror (including left/right edges).
*  Setup tests assure that nrings <= ngx,ngy so no overlap. */

         for (bptr=boxsumlo+nrm1; bptr<boxsumhi; bptr+=l1xn2) {
            si64 *leftdst = bptr - nrm1;
            si64 *leftsrc = bptr + nrm1 - 1;
            while (leftdst < bptr) {
               *(leftsrc + irngx) = *(leftdst + irngx);
               *leftdst++ = *leftsrc--;
               }
            }

         /* Top/bottom moves--move whole rows with memcpy */
         nbytes = (size_t)l1xn2 * sizeof(si64);
         beptr = bptr = boxsumlo;
         while ((bptr-=l1xn2) >= boxsum)
            memcpy(bptr, beptr, nbytes), beptr += l1xn2;
         beptr = bptr = boxsumhi - l1xn2;
         while ((bptr+=l1xn2) < hstrip)
            memcpy(bptr, beptr, nbytes), beptr -= l1xn2;
         break;   /* End of case 4 */

case BC_TOROIDAL:   /* Toroidal boundary conditions */

/* First, duplicate the values at the left and right edges.
*  Then, duplicate the top and bottom.  */

         nbytes = (size_t)nrm1 * sizeof(si64);
         for (bptr=boxsumlo+nrm1; bptr<boxsumhi; bptr+=l1xn2) {
            beptr = bptr - nrm1;
            memcpy(beptr, beptr+irngx, nbytes);
            memcpy(bptr+irngx, bptr, nbytes);
            }

         /* Top/bottom moves--move entire border areas with memcpy.
         *  At this point, borders that require summation from the
         *  opposite top or bottom edge of the region will have been
         *  computed already under control of the torwrap switch.
         *  Code here will do nothing in those cases, but is still
         *  needed when the entire region is contained between
         *  boxsum00 and boxsumbb (serial computer).  */
         if ((nbytes = hstrip - boxsumhi))   /* Assignment intended */
            memcpy(boxsumhi, boxsumlo, nbytes*sizeof(si64));
         if ((nbytes = boxsumlo - boxsum))   /* Assignment intended */
            memcpy(boxsum, boxsumhi - nbytes, nbytes*sizeof(si64));

         }  /* End of boundary-condition switch */

#if defined(DBGBSUMS)
      /* Print boxsums */
      if (!memcmp(il->lname,DBGBSLN,2)) {
         char *rlnm = fmtlrlnm(il);
#ifdef PAR
         dbgprt(ssprintf(NULL, " -->For Series %d, Trial %d, %s, "
            "Gconn %d", RP->CP.ser, RP->CP.trial, rlnm, (int)ib->igt));
         dbgprt(" ==>Boxsums for layer " DBGBSLN);
#else
         convrt("(P4,' -->For Series ',J0UJ6,', Trial ',J0UJ8,"
            "', ',J0A22,', Gconn ',J0IH6)", &RP->CP.ser,
            &RP->CP.trial, rlnm, &ib->igt, NULL);
         convrt("(P4,' ==>Boxsums for layer " DBGBSLN "')", NULL);
#endif
         dbpsums(boxsum, l1xn2yn2, l1xn2, 0);
         }
#endif
#ifndef PARn
      /* Perform (border) verification plot if requested */
      if (RP->kpl & KPLEDG) {
         int rc = d3bxvp(ib);
         if (rc > 0) return rc;
         }
#endif

/* PAR0 has no more to do after debug outputs */
#ifndef PAR0

/*---------------------------------------------------------------------*
*                                                                      *
*     Stage III--Apply positional modulation (falloff)                 *
*                                                                      *
*        This strange option was deleted in V8I, 05/11/13              *
*---------------------------------------------------------------------*/


/*---------------------------------------------------------------------*
*                                                                      *
*     Stage IV.  Perform band sums                                     *
*        Treat the central group as a band all by itself.              *
*        Then loop over the remaining bands (separate betas).          *
*           Within each band, loop over ngb rings.  Ring processing    *
*           proceeds outward.  With round bands, lists of which box-   *
*           sums to add are prestored in the table at prroff.  With    *
*           traditional square bands, redundant sums are minimized by  *
*           performing first a horizontal strip update, then a ring    *
*           summation, and finally a vertical strip update.  If OPT=X, *
*           final band branches to stage V to process the last ring.   *
*        At this stage, the unscaled band sums are stored for          *
*           later use with OPT=X.  Sums may be skipped when            *
*           beta=0 only when OPT=X is not requested for the            *
*           same INHIBBLK (see Stage V).                               *
*        Parallel computation introduces the following new wrinkles:   *
*           Horizontal and vertical strip sums need to be performed    *
*           for the entire width of the boxsum array (for inner row    *
*           groups), but results in the first and last rows can only   *
*           be stored for groups that are on the node and allocated.   *
*           Furthermore, xtotal still must be a sum over all groups.   *
*                                                                      *
*---------------------------------------------------------------------*/

/* Inner ring processing:
*  Copy boxsums into first bandsum segment and accumulate xtotal =
*  sum of all boxsums.  (In the parallel case, get xtotal as sum
*  of all cells, because all cells are not necessarily included in
*  the available boxsums).  */

      xtotal = jesl(0);
      /* Zero the bandsum array for inner band */
      memset(ib->inhbnd[0].bandsum, 0, mgrp*sizeof(si64));

#ifdef PAR
      if (opt_eq_X) {
         s_type *plimit;
         pcell = jl->pps[ib->ihdelay];
         plimit = pcell + jl->lspt;
         for ( ; pcell < plimit; pcell+=ligrp) {
            si64 wksum = grpsum(ib, pcell);
            xtotal = jasw(xtotal, wksum);
            }
         }
      if (opt_eq_X || ib->inhbnd[0].asbeta) {
         pbndsum = ib->inhbnd[0].bandsum;
         bptr = boxsum00 + nrm1 + lorgrp;
         beptr = boxsum00 + l1xn1;
         xeptr = boxsumbb - l1xn1 + hirgrp;
         for (;;) {
            if (bptr >= beptr) {
               if (bptr >= xeptr) break;
               bptr += l2n1;
               beptr += l1xn2;
               }
            *pbndsum++ = *bptr++;
            }
         }
#else
      /* xtotal is only needed for OPT=X, but in serial case it
      *  probably takes longer to test this than just always add. */
      if (opt_eq_X || ib->inhbnd[0].asbeta) {
         si64 *xptr;
         pbndsum = ib->inhbnd[0].bandsum;
         for (xptr=boxsum00+nrm1; xptr<boxsumbb; xptr+=l2n1) {
            xeptr = xptr + irngx;
            for ( ; xptr < xeptr; xptr++) {
               *pbndsum++ = *xptr;
               xtotal = jasw(xtotal, *xptr);
               }
            }
         }
#endif

#if defined(DBGBSUMS)
   /* Print xtotal and band 0 sums */
   if (!memcmp(il->lname,DBGBSLN,2)) {
#ifdef PAR
      if (opt_eq_X) {
         char xdec[10];
         wbcdwtPn(&xtotal, xdec, ((20+RP->bsdc)<<RK_BS) +
            (4<<RK_DS)+(RK_IORF|RK_NI64)+9);
         dbgprt(ssprintf(NULL, " --xtotal = %10s", xdec));
         }
      dbgprt(" --Bandsums for ring 0");
#else
      if (opt_eq_X)
         convrt("(P4,' --xtotal = ',B20/27IW10.4)", &xtotal, NULL);
      convrt("(P4,' --Bandsums for ring 0')", NULL);
#endif
      dbpsums(ib->inhbnd[0].bandsum, mgrp, irngx, lorgrp);
      }
#endif

/* Done inner (self) group.  See if there are any outer bands */

      if (!nibm1) goto BANDSFIN;    /* Skip to end of loop if
                                    *  done with all bands */

/*=====================================================================*
*     One level of indenting suppressed, here to end of Stage IV       *
*=====================================================================*/

   iring = iband = 0;
   if (ib->ibopt & IBOPRS) {

/*---------------------------------------------------------------------*
*                   Handle round-surround bandsums                     *
*---------------------------------------------------------------------*/

      size_t *prro = ib->prroff;
      ui32 ioff, ioffe, ioff1;      /* Offset loop controls */
      for (;;) {
         /* Note non-standard loop control */
         if (++iband > nibm1) goto BANDSFIN;    /* Done all bands */
         /* If on last band and OPT=X, skip to stage V */
         if (opt_eq_X && (iband == nibm1)) break;

         /* Zero the bandsum array for this band */
         pbndsum = ib->inhbnd[iband].bandsum;
         memset(pbndsum, 0, mgrp*sizeof(si64));

         /* Note that the prroff table has been set up to include
         *  groups in multiple rings, so an inner loop over rings
         *  is not needed here.  */
         if (ib->inhbnd[iband].asbeta || opt_eq_X) {
            ioff1 = prro[iband-1];
            ioffe = prro[iband];
            /* Loop over all the groups on this node */
            bptr = boxsum00 + nrm1 + lorgrp;
            xeptr = boxsum00 + l1xn1;  /* l1xn1 = ngx + nr -1 */
            beptr = boxsumbb - l1xn1 + hirgrp;
            while (bptr < beptr) {
               if (bptr >= xeptr) {    /* Navigate boxsum array */
                  bptr += l2n1;        /* Skip edges, go to next row */
                  xeptr += l1xn2;
                  }
               /* Note that we need to add two boxsums for each entry
               *  in the prroff table--at center location plus and
               *  minus the tabulated offset.  This avoids minus signs
               *  in the table.  */
               for (ioff=ioff1; ioff<ioffe; ++ioff) {
                  size_t obox = prro[ioff];
                  *pbndsum = jasw(*pbndsum, *(bptr+obox));
                  *pbndsum = jasw(*pbndsum, *(bptr-obox));
                  }
               ++pbndsum,++bptr;
               } /* End loop over groups */
#if defined(DBGBSUMS)
   /* Print bandsums for this band */
   if (!memcmp(il->lname,DBGBSLN,2)) {
#ifdef PAR
      dbgprt(ssprintf(NULL, " --Band sums for band %d\n", iband));
#else
      convrt("(P4,' --Band sums for band ',J0I4)", &iband, NULL);
#endif
      dbpsums(ib->inhbnd[iband].bandsum, mgrp, irngx, lorgrp);
      }
#endif
            } /* End band summation (beta != 0) */
         } /* End loop over bands */
      } /* End round surrounds */

   else {

/*---------------------------------------------------------------------*
*         Perform fast sums for traditional square surrounds           *
*---------------------------------------------------------------------*/

/* Initialize hstrip array (for horiz strip sums) */

      si64 *source, *dest = hstrip;
      ui32 rrbs = 0;                   /* Boxsum ring offset */
      ui32 rrhs = 0;                   /* Hstrip ring offset */
      nbytes = irngx * sizeof(si64);
      for (source = boxsum+nrm1; source < hstrip;
            source+=l1xn2, dest+=irngx) {
         memcpy((char *)dest, (char *)source, nbytes);
         }

/* Initialize vstrip array (for vert strip sums) */

      memcpy((char *)vstrip, (char *)(boxsum00),
         l1yxn2*sizeof(si64));

/* Start ring number at zero */

      for (;;) {

         /* Note non-standard loop control */
         if (++iband > nibm1) goto BANDSFIN;    /* Done all bands */
         /* If on last band and OPT=X, skip to stage V */
         if (opt_eq_X && (iband == nibm1)) break;

         /* Zero the bandsum array for this band */
         memset((char *)(ib->inhbnd[iband].bandsum), 0,
            mgrp*sizeof(si64));

         lring = iring + ib->ngb;
         while (iring < lring) {
            si64 *strptr;
            iring++;             /* Don't increment in the while! */
            rrbs += l1xn2;       /* boxsum offset */
            rrhs += irngx;       /* hstrip offset */

/* Horizontal strip update.
*  Loop over hstrip array, adding two groups to each
*  horizontal strip, from (x+ring,y) and (x-ring,y).  */

            strptr = hstrip;
            bptr = boxsum + nrm1;
            beptr = boxsum + l1xn2yn2;
            for ( ; bptr<beptr; bptr+=l2n1) {
               si64 *horiz_limit = bptr + irngx;
               for ( ; bptr<horiz_limit; bptr++,strptr++) {
                  /* hstrip(X,Y) = hstrip(X,Y) +
                  *  boxsum(X+ring,Y) + boxsum(X-ring,Y) */
                  *strptr = jasw(*strptr,
                     jasw(*(bptr+iring), *(bptr-iring)));
                  }  /* End loop over columns */
               }  /* End of loop over rows */

#if defined(DBGBSUMS)
   /* Print horizontal strips for this ring */
   if (!memcmp(il->lname,DBGBSLN,2)) {
#ifdef PAR
      dbgprt(ssprintf(NULL, " --Horizontal strip sums for "
         "ring %d\n", iring));
#else
      convrt("(P4,' --Horizontal strip sums for ring ',J0I4)",
         &iring, NULL);
#endif
      dbpsums(hstrip, l1xyn2, irngx, 0);
      }
#endif

/* Ring summation--add four tiles to make a full ring (skip loop if
*  beta = 0).  Note:  The hstrip array does not have the horizontal
*  margins, so it is the best place to control the loop indexing.  */

            if (ib->inhbnd[iband].asbeta || opt_eq_X) {
               si64 *hstrptr = hstrip + l1n1x + lorgrp;
               si64 *hlim = hstrptr + mgrp;
               si64 *vstrptr = vstrip + nrm1 + lorgrp;
               si64 *vlim = vstrip + l1xn1;
               si64 ttb,tlr;     /* Top+bottom, left+right sums */
               pbndsum = ib->inhbnd[iband].bandsum;
               for ( ; hstrptr < hlim; ++vstrptr,++hstrptr) {
                  if (vstrptr >= vlim)
                     vstrptr += l2n1, vlim += l1xn2;
                  /* Top + bottom strips */
                  ttb = jasw(*(hstrptr+rrhs), *(hstrptr-rrhs));
                  /* Left + right strips */
                  tlr = jasw(*(vstrptr-iring), *(vstrptr+iring));
                  *pbndsum = jasw(*pbndsum, jasw(ttb, tlr));
                  ++pbndsum;
                  }  /* End loop over group boxes */
               }  /* End of ring summation */

/* Vertical strip update.
*  Loop over vstrip array, adding two cells to each
*  vertical strip, from (X,Y+ring) and (X,Y-ring).  */

            strptr = vstrip;
            for (bptr=boxsum00; bptr<boxsumbb; bptr++,strptr++) {
               *strptr = jasw(*strptr,
                  jasw(*(bptr+rrbs), *(bptr-rrbs)));
               }

#if defined(DBGBSUMS)
   /* Print vertical strips for this ring */
   if (!memcmp(il->lname,DBGBSLN,2)) {
#ifdef PARn
      dbgprt(ssprintf(NULL, " --Vertical strip sums for "
         "ring %d\n", iring));
#else
      convrt("(P4,' --Vertical strip sums for ring ',J0I4)",
         &iring, NULL);
#endif
      dbpsums(vstrip, l1yxn2, l1xn2, 0);
      }
#endif

            }  /* End of loop over rings */

#if defined(DBGBSUMS)
   /* Print bandsums for this ring */
   if (!memcmp(il->lname,DBGBSLN,2)) {
#ifdef PAR
      dbgprt(ssprintf(NULL, " --Band sums for band %d\n", iband));
#else
      convrt("(P4,' --Band sums for band ',J0I4)", &iband, NULL);
#endif
      dbpsums(ib->inhbnd[iband].bandsum, mgrp, irngx, lorgrp);
      }
#endif

         }  /* End of loop over bandsums */
      } /* End square-surrounds method */

/*=====================================================================*
*                 End one level of indent suppression                  *
*=====================================================================*/

/*---------------------------------------------------------------------*
*                                                                      *
*     Stage V.  Special last-band processing for OPT=X                 *
*        Instead of a normal last-band sum, here take the              *
*           recorded xtotal and subtract from it the inhibition        *
*           for the inner bands which has already been stored in       *
*           the bandsum array.  It is more efficient to do this        *
*           in a separate pass, as here, whenever ngb > 1, as          *
*           the main summation loop is executed once per ring          *
*           but this is only done once per band.                       *
*        To minimize any cache reloads, groups are processed           *
*           sequentially with an outer loop over bands rather          *
*           than adding all bands for one group at a time, even        *
*           though that would sum in a register and save stores.       *
*        For consistency with handling of other bands, scaling         *
*           is postponed to stage VI.                                  *
*                                                                      *
*---------------------------------------------------------------------*/

      if (ib->inhbnd[nibm1].asbeta) {
         si64 *bndlim;
         si64 *ipbndsum;

/* Store xtotal in all groups of outer segment */

         pbndsum = ib->inhbnd[nibm1].bandsum;
         bndlim = pbndsum + mgrp;
         while (pbndsum < bndlim) *pbndsum++ = xtotal;

/* Subtract all inner bands from outer band for all groups.
*  R65, 01/22/16, GNR - Remove assumption that consecutive bandsums
*                       are adjacent in memory for safer code.  */

         for (iband=0; iband<nibm1; ++iband) {
            ipbndsum = ib->inhbnd[iband].bandsum;
            pbndsum = ib->inhbnd[nibm1].bandsum; /* Outermost band */
            for (; pbndsum < bndlim; ++pbndsum) {
               *pbndsum = jrsw(*pbndsum, *ipbndsum++);
               }
            }
#if defined(DBGBSUMS)
   /* Print bandsums for this band */
   if (!memcmp(il->lname,DBGBSLN,2)) {
#ifdef PAR
      dbgprt(ssprintf(NULL, " --OPT=X band sums for band %d\n",
         nibm1));
#else
      convrt("(P4,' --OPT=X band sums for band ',J0I4)", &nibm1, NULL);
#endif
      dbpsums(ib->inhbnd[nibm1].bandsum, mgrp, irngx, lorgrp);
      }
#endif

         }  /* End of last bandsum */

BANDSFIN:

/*---------------------------------------------------------------------*
*                                                                      *
*     Stage VI.  Normalize boundaries, scale, and store band sums.     *
*           Capping is now done in d3inhg or d3inhc.                   *
*        Stage VI must be done after the extended field sums, which    *
*           use unscaled values from the inner bands.  Stage VI only   *
*           needs to loop over bands, not rings.                       *
*        For OPT=X, scaling is performed without normalization as the  *
*           center hole should not be expanded outside the bounds      *
*           included in xtotal.                                        *
*        This algorithm for OPT=X is incorrect if the boundary         *
*           condition is other than zero, norm, or toroidal.           *
*           In this version, these cases are rejected by input         *
*           checking in g2gconn.  It is not exactly clear how the      *
*           other cases would be defined, but probably one would       *
*           set the border to zero, then go back and repeat all        *
*           the ring sums, subtracting from last band sum.             *
*        Another decision: to loop over all bands, one cell            *
*           (thus doing fewer evaluations of normalization)            *
*           or do all cells, one band (thus avoiding cache loads       *
*           and paging due to large stride).  This may be wrong,       *
*           but I pick the former as being less bad in a big run.      *
*                                                                      *
***********************************************************************/

/* Prepare for normalization */

      do_norm = (ib->kbc == BC_NORM);     /* TRUE if BC=NORM */
      xy = ib->xynorm;

/* Loop over bands, then y, then x (needed for norm table lookup).
*  If beta=0 and OPT=X, now need to clear band sums and then can
*  proceed directly to next band.  */

      for (iband=0; iband<=nibm1; iband++) {
         struct INHIBBAND *pb = ib->inhbnd + iband;
         ui32 igrp;
         si32 beta, wkbeta;
         pbndsum = pb->bandsum;

         if ((beta = pb->asbeta) == 0) {
            if (opt_eq_X) memset(pbndsum, 0, mgrp*sizeof(si64));
            continue;
            }

         /* If about to do last band and OPT=X, kill norm */
         if (opt_eq_X && (iband == nibm1)) do_norm = FALSE;

/* Scale (possibly using normalized beta) bandsums for all groups.
*  S20/27 (band sum) + 20 (beta) - 20 ==> S20/27.
*  R65, 01/22/16, GNR - Separate loops for norm vs no norm to remove
*    'if' from inner loop.  Norm y table origin is now il->logrp.  */

         if (do_norm) {
            ui32 igx = lorgrp, igy = 0;
            for (igrp=il->logrp; igrp<il->higrp; ++pbndsum,++igrp) {
               /* Normalized beta = beta*(total area)/(used area) */
               wkbeta = dm64nq(beta, ib->gcarea,
                  ((si32)xy[igx].x)*((si32)xy[igy].y));
               *pbndsum = dmrswjwd(*pbndsum, wkbeta, pb->barea, -FBwk);
               if (++igx >= irngx) igx = 0, ++igy;
               }
            } /* End do_norm */
         else {            /* No normalization */
            si64 *peband = pbndsum + mgrp;
            for ( ; pbndsum < peband; ++pbndsum) {
               *pbndsum = dmrswjwd(*pbndsum, beta, pb->barea, -FBwk);
               }
            } /* End !do_norm */
         }  /* End of loop over bands */

#endif /* !PAR0 */
      }}  /* End else and grand loop over INHIBBLKs */

   return D3_NORMAL;
   }  /* End d3inhb() */


/*=====================================================================*
*                                                                      *
*                               d3inhg                                 *
*                                                                      *
*=====================================================================*/

void d3inhg(void) {

   struct CELLTYPE *il;       /* Pointer to current CELLTYPE */
   struct INHIBBLK *ib;       /* Pointer to current INHIBBLK */
   struct INHIBBAND *pb;      /* Pointer to current INHIBBAND */
   si32 *pgx;                 /* Pointer to afference data */
   si32 *peff;                /* Ptr to group peipv */
   ui32 gn,gx,gy;             /* Current group coordinates */
   int  iband;                /* Number of current band */
   int  kfs;                  /* Fast start decay selector */
   int  laffs,p;              /* Sum distribution controls */
   ui16 ccyc0;                /* Current cycle (0-based) */

/* Setups before INHIBBLK loop */

   il = CDAT.cdlayr;             /* Locate CELLTYPE */
   if (!(il->ctf & CTFGV))       /* Clear afference accumulators */
      memset((char *)CDAT.pGeomSums, 0, CDAT.lGeomSums);
   laffs = 1 << il->pdshft;
   ccyc0 = CDAT.curr_cyc1 - 1;

/* Loop over all INHIBBLKs for this cell type */

   gn = CDAT.groupn - il->logrp;
   gx = (ui32)CDAT.groupx, gy = (ui32)(CDAT.groupy - il->logrpy);
   for (ib=il->pib1; ib; ib=ib->pib)
         if (CDAT.curr_cyc1 > ib->gcdefer) {

      e64dec((int)ib->iovec);

/* Loop over bands */

      ib->rnsums[EXCIT] = ib->rnsums[INHIB] = jesl(0);
      pb = ib->inhbnd;           /* Locate band data */
      for (iband=0; iband<ib->nib; iband++, pb++) {

/* For all bands (all but the innermost band if IBOEFV), cap the
*  (already scaled) bandsums and accumulate the totals into rnsums.
*  For the innermost band if IBOEFV, instead store the scaled total
*  in r0sum and the regenerated net scale factor in r0scale.  Leave
*  the scaled, uncapped totals in bandsum[groupn-logrp] for dprnt.
*  An awkwardness here is that the scale factor for each group
*  has already been calculated in d3inhb on the fly, but there
*  is no room to store all these scales, so the one we want now
*  must be reconstructed as wkbeta = asbeta * norm.
*  V8C, 10/12/03, GNR - Corrected norm to use gcarea, not l1xy. */

         if (pb->beta) {
            si64 wksum = pb->bandsum[gn];
            si32 wkbeta;
            if (iband > 0 || !(ib->ibopt & IBOEFV)) {
               /* Outer band or any band when not IBOEFV */
               si64 tmxib = jesl(pb->mxib);
               if (qsw(wksum) >= 0) {
                  if (qsw(jrsw(wksum, tmxib)) >= 0)
                     ib->rnsums[EXCIT] = jasw(ib->rnsums[EXCIT], tmxib);
                  else
                     ib->rnsums[EXCIT] = jasw(ib->rnsums[EXCIT], wksum);
                  }
               else {
                  if (qsw(jasw(wksum, tmxib)) >= 0)
                     ib->rnsums[INHIB] = jasw(ib->rnsums[INHIB], wksum);
                  else
                     ib->rnsums[INHIB] = jrsw(ib->rnsums[INHIB], tmxib);
                  }
               }
            else {
               /* Scaled inner band */
               ib->r0sum = wksum;
               /* Calculate overall inner band scale */
               wkbeta = pb->asbeta;
               /* Look up and apply normalization factor */
               if (ib->kbc == BC_NORM) {
                  struct XYNORMDEF *xy = ib->xynorm;
                  wkbeta = dm64nq(wkbeta, ib->gcarea,
                     ((si32)xy[gx].x) * ((si32)xy[gy].y));
                  }
               ib->r0scale = wkbeta;
               } /* End ring 0 */
            } /* End if beta */

         } /* End loop over bands */

/* All bands for this INHIBBLK are now done.  If CTFGV, items needed
*  for individual cell calculations have been stored, there is no more
*  to do here.  Otherwise, finish up for current group, handling excit
*  and inhib contributions separately, and using d3decay to apply
*  decay.  rnsums should be down to 32 bits now, but we check to make
*  sure.  If celltype has phase, distribute gcon uniformly, otherwise
*  there is just one term.  */

      if (il->ctf & CTFGV) continue;

      if (qdecay(&ib->GDCY)) {
         peff = ib->GDCY.peipv + (size_t)gn * (size_t)ib->GDCY.ndpsp;
         kfs = CDAT.curr_cyc1 == ib->gfscyc;
         }
      pgx = CDAT.pAffD32 + ib->ogafs;

      if (ib->gssck & PSP_POS) {                   /* EXCIT terms */

         si64 *pSums = CDAT.pAffD64 + ib->ogsei[EXCIT];
         si32 gaxp;
         swlodm(gaxp, ib->rnsums[EXCIT]);          /* Capped sum */
         *pgx++ = gaxp;
         if (qdecay(&ib->GDCY)) {                  /* Apply decay */
#if 0 /*** DEBUG -- Restore old OPT=F behavior ***/
            if (kfs)
               *pgx = *peff = gaxp;
            else
               gaxp = d3decay(&ib->GDCY, peff, pgx, gaxp);
#else
            gaxp = CDAT.pdecay[kfs](&ib->GDCY, peff, pgx, gaxp);
#endif
            peff += ib->GDCY.ndal, pgx += ib->GDCY.ndal;
            }
         *pgx++ = gaxp;
         if (ccyc0 >= ib->gdefer) {
            for (p=0; p<laffs; p++) pSums[p] = jasl(pSums[p], gaxp);
            }

         } /* End excitatory terms */

      if (ib->gssck & PSP_NEG) {                   /* INHIB terms */

         si64 *pSums = CDAT.pAffD64 + ib->ogsei[INHIB];
         si32 gaxn;
         swlodm(gaxn, ib->rnsums[INHIB]);          /* Capped sum */
         *pgx++ = gaxn;
         if (qdecay(&ib->GDCY)) {                  /* Apply decay */
#if 0 /*** DEBUG ***/
            if (kfs)
               *pgx = *peff = gaxn;
            else
               gaxn = d3decay(&ib->GDCY, peff, pgx, gaxn);
#else
            gaxn = CDAT.pdecay[kfs](&ib->GDCY, peff, pgx, gaxn);
#endif
            pgx += ib->GDCY.ndal;
            }
         *pgx++ = gaxn;
         if (ccyc0 >= ib->gdefer) {
            for (p=0; p<laffs; p++) pSums[p] = jasl(pSums[p], gaxn);
            }

         } /* End inhibitory terms */

      } /* End loop over INHIBBLKs */

   } /* End d3inhg() */


/*=====================================================================*
*                                                                      *
*                               d3inhc                                 *
*                                                                      *
*=====================================================================*/

void d3inhc(void) {

   struct CELLTYPE *il;       /* Pointer to current CELLTYPE */
   struct INHIBBLK *ib;       /* Pointer to current INHIBBLK */
   struct INHIBBAND *pb;      /* Pointer to current INHIBBAND */
   si32 *pgx;                 /* Pointer to afference distribution */
   si32 *peff;                /* Ptr to cell ieveff */
   si64 hnaxp,hnaxn;          /* Holding copies of rnsums */
   ui32 gn;                   /* Current group number */
   int  kfs;                  /* Fast start decay selector */
   int  laffs,p;              /* Sum distribution controls */
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
   gn = CDAT.groupn - il->logrp;
   ccyc0 = CDAT.curr_cyc1 - 1;
   for (ib=il->pib1; ib; ib=ib->pib)
         if (CDAT.curr_cyc1 > ib->gcdefer) {

      e64dec((int)ib->iovec);

/* Initialize totals from d3inhg() data for outer bands -- we cannot
*  just write over rnsums, because may be needed for a later cell.  */

      pb = ib->inhbnd;
      hnaxp = ib->rnsums[EXCIT];
      hnaxn = ib->rnsums[INHIB];

/* If this is an INHIBBLK with effective self-avoidance, and if
*  the activity of the current cell at (t-1) exceeds itt or ittlo
*  thresholds, scale it and subtract from saved central ring sum
*  Save this as the adjusted first band sum.  */

      if (ib->ibopt & IBOEFV) {
         si64 wksum = ib->r0sum;    /* Sum of all cells in ring 0 */
         si64 tmxib = jesl(pb->mxib);
         si64 tsval;
         si32 work, effsi = CDAT.old_siS7 - (long)ib->gjrev;
         if (effsi > (si32)ib->effitt) {
            work = effsi - (si32)ib->subitt;
            goto UseSiVal;
            }
         if (effsi < (si32)ib->effnitt) {
            work = effsi - (si32)ib->subnitt;
            goto UseSiVal;
            }
         goto OmitSiVal;
UseSiVal:
         /* Subtract out square if square was added.  Apply the saved
         *  r0scale, which combines beta and normalization.  As with
         *  other rnsums, we want to end up with S20/27.  */
         if (ib->ibopt & IBOPCS) {
            tsval = jmsw(work, work);
            /* S7/14 + 14 (work*work) + 20 (r0scale) - 21 ==> S20/27 */
            tsval = dmrswjwd(tsval, ib->r0scale, pb->barea, -21);
            }
         else
            /* S7/14 (work) + S20 (r0scale) - 7 ==> S20/27 */
            tsval = dmrswjwd(jesl(work), ib->r0scale, pb->barea, -7);
         pb->bandsum[gn] = wksum = jrsw(wksum, tsval);

         /* Apply cap to inner band sum and add into outer band sums */
         if (qsw(wksum) >= 0) {
            if (qsw(jrsw(wksum, tmxib)) >= 0)
               hnaxp = jasw(hnaxp, tmxib);
            else
               hnaxp = jasw(hnaxp, wksum);
            }
         else {
            if (qsw(jasw(wksum, tmxib)) >= 0)
               hnaxn = jasw(hnaxn, wksum);
            else
               hnaxn = jrsw(hnaxn, tmxib);
            }
         } /* End adjusting for current cell */
OmitSiVal: ;

/* Apply decay, store detail info and accumulate sums according
*  to mode of excitation or inhibition, cf. d3inhg().  */

      if (qdecay(&ib->GDCY)) { peff = ib->GDCY.peipv +
            (size_t)(CDAT.cdcell - il->locell) * (size_t)ib->GDCY.ndpsp;
         kfs = CDAT.curr_cyc1 == ib->gfscyc;
         }
      pgx = CDAT.pAffD32 + ib->ogafs;

      if (ib->gssck & PSP_POS) {                   /* EXCIT terms */

         si64 *pSums = CDAT.pAffD64 + ib->ogsei[EXCIT];
         si32 gaxp;
         swlodm(gaxp, hnaxp);                      /* Capped sum */
         *pgx++ = gaxp;
         if (qdecay(&ib->GDCY)) {                  /* Apply decay */
#if 0 /*** DEBUG -- Restore old OPT=F behavior ***/
            if (kfs)
            else gaxp = d3decay(&ib->GDCY, peff, pgx, gaxp);
#else
            gaxp = CDAT.pdecay[kfs](&ib->GDCY, peff, pgx, gaxp);
#endif
            peff += ib->GDCY.ndal, pgx += ib->GDCY.ndal;
            }
         *pgx++ = gaxp;
         if (ccyc0 >= ib->gdefer) {
            for (p=0; p<laffs; p++) pSums[p] = jasl(pSums[p], gaxp);
            }

         } /* End excitatory terms */

      if (ib->gssck & PSP_NEG) {                   /* INHIB terms */

         si64 *pSums = CDAT.pAffD64 + ib->ogsei[INHIB];
         si32 gaxn;
         swlodm(gaxn, hnaxn);                      /* Capped sum */
         *pgx++ = gaxn;
         if (qdecay(&ib->GDCY)) {                  /* Apply decay */
#if 0 /*** DEBUG ***/
            if (kfs)
               *pgx = *peff = gaxn;
            else gaxn = d3decay(&ib->GDCY, peff, pgx, gaxn);
#else
            gaxn = CDAT.pdecay[kfs](&ib->GDCY, peff, pgx, gaxn);
#endif
            pgx += ib->GDCY.ndal;
            }
         *pgx++ = gaxn;
         if (ccyc0 >= ib->gdefer) {
            for (p=0; p<laffs; p++) pSums[p] = jasl(pSums[p], gaxn);
            }

         } /* End inhibitory terms */

      } /* End INHIBBLK loop */

   } /* End d3inhc() */

