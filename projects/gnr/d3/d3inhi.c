/* (c) Copyright 1989-2016, The Rockefeller University *21115* */
/* $Id: d3inhi.c 71 2017-02-03 19:42:45Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                               d3inhi                                 *
*                                                                      *
*  This file contains d3inhi, a routine to initialize normalization    *
*  and self-avoidance for geometric connections (run at new trial      *
*  series time), and d3inht, a routine to apply autoscale and gbcm     *
*  factors (and any other initialization needed at new-cycle time).    *
*                                                                      *
************************************************************************
*  Rev, 11/14/89, JMS - Rounding added to mean noise border calc       *
*  V6D, 02/15/94, GNR - Broken out from d3gcon.c                       *
*  V8A, 09/02/95, GNR - Move falloff table setup to d3foff,            *
*                       fix border calc bug, add IBOEFV and d3inhs     *
*  Rev, 01/01/98, GNR - Add initialization of norm tables              *
*  Rev, 02/20/98, GNR - Add resting potential to border calculation    *
*  V8C, 03/01/03, GNR - Modify scaling for mV S7 cell activity         *
*  ==>, 12/29/07, GNR - Last mod before committing to svn repository   *
*  Rev, 05/03/08, GNR - IBOPT=K and COMPAT=K for knee response         *
*  V8E, 02/17/09, GNR - Allow for step vs knee thresholding            *
*  Rev, 05/06/09, GNR - Subtract gjrev adjustment from all inputs      *
*  V8G, 08/17/10, GNR - Add d3inht()                                   *
*  V8H, 05/09/12, GNR - Add AUTOSCALE OPT=T, add self-update check     *
*  Rev, 05/17/12, GNR - Add gbcm scaling                               *
*  Rev, 05/24/12, GNR - Merge d3inhs() into d3inht()                   *
*  V8I, 01/12/13, GNR - Add effitt, effnitt                            *
*  Rev, 05/11/13, GNR - Remove gconn falloff, store band area instead  *
*                       of scaling beta for better large-band accuracy.*
*  Rev, 06/19/13, GNR - Multiply betas by autoscale per kasop mode.    *
*  R65, 01/11/16, GNR - Merge parallel into serial code, even run on   *
*                       PAR0 nodes in case needed for bxvp.  Store     *
*                       border as si64 to avoid possible overflow.     *
*  R66, 01/30/16, GNR - Inhib betas are now > 0 excit, < 0 inhib,      *
*                       compute geometry tables for round surrounds    *
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
#include "rocks.h"

#define SQRTHLF 0.70710678119 /* Constant for adjusting erfc arg */
#define SQRT1PI 0.56418958355 /* sqrt(1/pi) */

extern struct CELLDATA CDAT;

/*=====================================================================*
*                               d3inhi                                 *
*                                                                      *
*     Initialization routine for geometric connections.                *
*     Set threshold values to be subtracted if IBOPKR is set.          *
*     Determine whether special calculation for IBOPSA is needed.      *
*     Calculate border noise for all layers with BC==NOISE.            *
*     Initialize normalization tables if needed.                       *
*     Initialize round-surround tables if needed.                      *
*     Calculate band areas (for beta adjustment) for all layers.       *
*     This routine is called from main program during setup for a      *
*        new trial series, as some gconn parameters may be changed     *
*        by a 'CHANGE' card.                                           *
*     Double precision is used rather extravagantly in this            *
*        routine to maximize compatibility with IBM version.           *
*=====================================================================*/

void d3inhi(void) {

   struct CELLTYPE *il;
   struct INHIBBLK *ib;

#ifdef PAR0
   if (!(RP->kpl & KPLEDG)) return;
#endif

/* Loop over cell types and INHIBBLKs */

   for (il=RP->pfct; il; il=il->pnct) {
      il->ctf &= ~CTFGV;
      CDAT.cdlayr = il;                /* For d3lime() */
      for (ib=il->pib1; ib; ib=ib->pib) {
         struct CELLTYPE *jl = ib->pisrc;
         struct INHIBBAND *pihb,*pihb0,*pihbe;
         si64 rsi = 0;                 /* Ring area increment */
         si64 area = jesl(jl->nel);    /* No. cells in 1st ring */
         si64 xtotal = jesl(jl->nelt); /* Total cells for OPT=X */
         int jec = (int)ib->iovec;     /* Overflow count offset */
         ui32 jngb = (ui32)ib->ngb;    /* Groups/band */
         ui32 jngbm1 = jngb - 1;       /* For ceil(ring/ngb) */
         ui32 jnib = (ui32)ib->nib;

/* Set values for thresholds testing and subtraction */

         ib->effitt  = ib->itt   ? ib->itt - 1 : 0;
         ib->effnitt = ib->ittlo ? ib->ittlo + 1 : 0;
         if (ib->ibopt & IBOPKR)
            ib->subitt = ib->itt, ib->subnitt = ib->ittlo;
         else
            ib->subitt = ib->subnitt = 0;

/* Set IBOEFV bit if special calculation for self-avoidance (IBOPSA)
*  is actually required.  If inner beta is zero, or if there is only
*  one cell per group in the source, a simpler method can be used.  */

         ib->ibopt &= ~IBOEFV;
         if ((ib->ibopt & IBOPSA) && ib->jself) {
            if (jl->nel > 1) {
               if (ib->inhbnd->beta != 0) {
                  ib->ibopt |= IBOEFV;
                  il->ctf |= CTFGV; }
               }
            else
               ib->inhbnd->beta = 0;
            }

/* Set value for mean border noise.
*  Formulas are derived in design notes p. 27C for case of Gaussian
*  noise added to a fraction nfr of the cells.  In case COMPAT=C,
*  all scales are 7 bits greater than indicated.  ti,ts,tit0 are
*  dimensionless due to division by nsg.  Case nsg == 0, although
*  unlikely, is handled separately to avoid divide by 0 error.
*  V8E, 02/17/09, GNR - Remove addition of vrest to fnmn
*  Rev, 05/05/09, GNR - Subtract gjrev from fnmn
*  Rev, 08/27/13, GNR - Remove ihsf scaling, result to S20
*  R66, 02/15/16, GNR - Return positive, not negative, noise
*/

         if (ib->kbc == BC_NOISE) {
            double excess,fnbord,fnmn,fthi,ti,ts,tit0;
            /* Calc noise mean of source cells, nmn (S20)->fnmn
            *  adjusted for rev potl (S7)->fnmn (S20) */
            fnmn = (double)(((si32)ib->gjrev << (FBwk-FBsi)) -
               jl->No.nmn);
            fthi = (double)((si32)ib->itt<<(FBwk-FBsi)); /* S7->S20 */

            excess = fthi + fnmn;     /* (inhib thresh - noise mean) */
            /* If nsg == 0, use simple average noise */
            if (jl->No.nsg == 0) fnbord = (excess < 0.0) ?
               (ib->ibopt & IBOPKR ? excess : fnmn) : 0.0;
            /* Otherwise, have to do error integrals */
            else {
               double fnsg = (double)(jl->No.nsg>>(FBsc-FBwk));
               double fths = (RP->compat & OLDRANGE) ? dS27 : dS28;
               ti = (ts = SQRTHLF/fnsg) * excess;
               tit0 = ib->ibopt & IBOPKR ? ti : ts*fnmn;
               ts *= (fths + fnmn);
               fnbord = SQRTHLF*fnsg*(tit0*rerfc(ti) - ts*rerfc(ts) +
                  SQRT1PI*(exp(-ts*ts) - exp(-ti*ti)));
               }
            if (fnbord < 0.0) {
               fnbord *= (double)jl->nel*(double)jl->No.nfr/dS24;
               ib->border = dbl2swe(fabs(fnbord) - 0.5, jec);
               }
            else
               ib->border = jesl(0);
            } /* End noise border setup */

/* Create entries in norm table.
*  Each entry is length of largest ring edge that is
*  inside the layer for the given x or y coord--
*  = min(nr1+1+x,nr1+1+(ngx-1-x),2*nr1+1) for x coords
*  = min(nr1+1+y,nr1+1+(ngy-1-y),2*nr1+1) for y coords
*  N.B.  This code moved here from d3genr, 01/01/98, GNR.
*  These tables cannot change during a run, however, they are now
*  calculated redundantly here on new trial series because edge
*  plots can be turned on and off on the CYCLE card at Group III
*  time and d3nset() is now set up to place these tables in movable
*  ("lindv") storage.
*  R65, 01/18/16, GNR - Origin of y table is now il->logrp.
*/
         if (ib->kbc == BC_NORM) {
            struct XYNORMDEF *xy, *xye;
            ui32 fring, lftx, rgtx, boty, topy, txy;
            ui32 irngx = (ui32)il->pback->ngx;
            ui32 irngy = (ui32)il->pback->ngy;
            ui32 nr = (ui32)ib->l1n1;        /* Number of rings - 1 */

            fring = 2*nr + 1;                /* Full outer ring edge */
            lftx  = nr + 1;                  /* Left x when x = 0 */
            rgtx  = nr + irngx;              /* Right x when x = 0 */
            boty  = nr + 1 + il->logrpy;     /* Low y when y = logrpy */
            topy  = nr + irngy - il->logrpy; /* Top y when y = logrpy */
            xy = ib->xynorm;
            xye = xy + max(irngx, il->mgy);
            for ( ; xy<xye; xy++,lftx++,boty++,rgtx--,topy--) {
               txy    = min(lftx, fring);
               xy->x  = min(txy, rgtx);
               txy    = min(boty, fring);
               xy->y  = min(txy, topy);
               }
            }  /* End if BC=NORM */

/* Create round-surround table and store band areas.
*  It is important that each cell group within the circle of radius
*  nrings be used exactly once.  Accordingly, code loops over grid
*  first, recording (in borrowed boxsum space) the band that each
*  point is closest to and keeping a count of points in each band.
*  Then a pass is made over the grid space, computing and storing
*  the offsets into the prroff table.  The code is simplified by
*  knowing that the center is always on a grid point.
*  In the following, nr = number rings - 1 = number outer rings,
*     nrow = length of one scan row = (2 * nr) + 1
*  Format of table constructed in boxsum space (ptct, ptro):
*  Words 0 to (nib-1):  Count of cell groups in bands 1 to nib
*  Words nr+1, nr+i, ... nr+1+nr:  Band that group i positions
*     to right of center (ring 0) belongs to = ceil(i/ngb).
*  Remaining nr rows of length nrow:  In jth row, j = 1..nrings,
*     band that group in each position of that row belongs to,
*     with 0 coding a group that is outside the surround circle.
*  Format of prroff table:
*  Word 0:  Always nib (offset in table for band 1 entries).
*  Words 1 to (nib-1):  Offset of last point in band iband.
*  Then n groups of entries giving offsets to points in bands 1 to
*  (nib-1).  The number of entries for band iband is cumulative
*  count[iband] - cum count[iband-1].  There are no table entries
*  for ring 0, which is just the center point.
*
*  Rev, 11/01/16, GNR - Correct to handle ngb > 1.
*/
#define iband(iring) ((iring + jngbm1)/jngb)

         pihb0 = ib->inhbnd;
         pihbe = pihb0 + jnib - 1;
         if (ib->ibopt & IBOPRS) {
            size_t *prro = ib->prroff;
            ui32 *ptro, *ptct = (ui32 *)RP->boxsum;
            size_t boff = 0;                 /* Base (row) offset */
            size_t l1xn2 = (size_t)ib->l1xn2;/* Boxsum row increment */
            float rrsq, rsq;
            ui32 nr = (ui32)ib->l1n1;        /* Number of rings - 1 */
            ui32 iring, nr1 = nr + 1;        /* Ring counter, nrings */
            ui32 icr;                        /* Cols on current row */
            ui32 jqr;                        /* Quadrant row index */
            si32 jqc;                        /* Column index (must be
                                             *  signed as neg index) */
            ui32 nrow = nr + nr1;            /* Groups in a row */
            ptro = ptct + nr;
            rsq = (float)(nr * nr);
            memset(ptct, 0, nrow*nr1*sizeof(ui32));
            /* Points on row 0 and col 0 half diameters */
            for (iring=1; iring<nr1; ++iring) {
               ptct[iring-1] = 2;
               ptro[iring] = ptro[iring*nrow] = iband(iring);
               }
            /* Now loop over upper left and right quadrants */
            for (jqr=1; jqr<nr; ++jqr) {
               ptro += nrow;
               rrsq = (float)(jqr * jqr);
               jqc = (si32)roundf(sqrtf(rsq - rrsq));
               while (jqc > 0) {
                  icr = (ui32)roundf(sqrtf(rrsq + (float)(jqc * jqc)));
                  ptro[jqc] = ptro[-jqc] = iband(icr);
                  ptct[icr-1] += 2;    /* icr < 1 is impossible here */
                  jqc -= 1;
                  }
               }
            /* Put the cumulative offsets at the start of the table
            *  (needed for looping in d3inhb) and zero counts for
            *  reuse in next loop.  (icr is band index here.)  */
            prro[0] = (size_t)(jqr = jnib);
            for (icr=1; icr<jnib; ++icr) {
               prro[icr] = jqr += ptct[icr-1];
               ptct[icr-1] = 0; }
            /* Finally, build the table:  Now that we have the
            *  starting offsets for each band, we can distribute
            *  the grid points to the correct bands.  Here
            *  jqr loops over rows, icr over columns.  */
            ptro = ptct;
            for (jqr=0; jqr<=nr; ++jqr) {
               icr = (jqr == 0) ? nr1 : 0;
               for ( ; icr<nrow; ++icr) {
                  /* jqc = band at this row,col minus 1 */
                  jqc = ptro[icr] - 1;
                  if (jqc < 0) continue;
                  prro[prro[jqc]+ptct[jqc]++] = boff + icr - nr;
                  } /* End column loop */
               ptro += nrow, boff += l1xn2;
               } /* End row loop */

            /* Now compute band areas for circular bands.  Note:
            *  Setup code assures nib >= 2 w/IBOPTX, ngb*(nib-1) <=
            *  23169, but 'area' is si64 in case nel is large.
            *  'area' is initialized to nel above for inner band.  */
            for (icr=0,pihb=pihb0; pihb<=pihbe; ++icr,++pihb) {
               si32 rndiff;
               if ((pihb == pihbe) && (ib->ibopt & IBOPTX)) {
                  swloem(pihb->barea, xtotal, ib->iovec);
                  }
               else {
                  swloem(pihb->barea, area, ib->iovec);
                  /* Update area according to band area.  Area of
                  *  outer band with OPT=X is size of source minus
                  *  total of all inner rings.  */
                  xtotal = jrsw(xtotal, area);
                  if (qsw(xtotal) < 0) {
                     d3exit(fmturlnm(il), GCONNSZ_ERR, ib->igt);
#ifndef GCC
                     pihb->barea = SI32_MAX; /* JIC */
#endif
                     }
                  rndiff = 2*(prro[icr+1] - prro[icr]);
                  area = jmsw(rndiff, (si32)jl->nel);
                  } /* End else !IBOPTX */
               } /* End loop over INHIBBANDs */
            } /* End round-surround setup */

         /* Calculation of band areas for traditional square bands */
         else for (pihb=pihb0; pihb<=pihbe; ++pihb) {
            if ((pihb == pihbe) && (ib->ibopt & IBOPTX)) {
               /* Area of outer band with OPT=X is size of source
               *  minus size of next inner square band.  */
               si32 edge = 2*jngb*(jnib-2) + 1;
               area = jrsw(jesl(jl->nelt),
                  jmsw((si32)(edge*edge),(si32)jl->nel));
               }
            if (qsw(area) < 0 || jckslo(area)) {
               d3exit(fmturlnm(il), GCONNSZ_ERR, ib->igt);
#ifndef GCC
               pihb->barea = SI32_MAX; /* JIC */
#endif
               }
            else
               pihb->barea = swlo(area);
            /* Update area according to ring area */
            if (pihb == pihb0) {
               area = mrssw(area, jngb<<2, 0);
               rsi = mrssw(area, jngb, 0);
               area = jasw(area, rsi);
               rsi = jasw(rsi, rsi);
               }
            else
               area = jasw(area, rsi);
            } /* End loop over INHIBBANDs */

         }  /* End loop over INHIBBLKs */
      }  /* End loop over CELLTYPEs */
   }  /* End of d3inhi() */

/*=====================================================================*
*                               d3inht                                 *
*                                                                      *
*  Call once per celltype at the beginning of each cycle to perform    *
*  any needed gconn initialization.  Currently:                        *
*  --Apply autoscale and gbcm adjustments to the beta factors.         *
*  Note, 05/24/12, GNR - Former d3inhs() routine was merged into       *
*     this routine, eliminating confusing updating of beta factors     *
*     in place, at cost of a small computation per cycle.  Tests       *
*     for gbcm*cycles underflow and for extended area too large        *
*     were moved to d3news() (Node 0 code).                            *
*=====================================================================*/

void d3inht(struct CELLTYPE *il) {

   extern struct CELLDATA CDAT;
   struct INHIBBLK *ib;
#ifndef PARn
   int qdbg = RP->CP.dbgflags & DBG_GBETA && RP->CP.trial % 100 < 4;

   if (qdbg) {             /*** DEBUG ***/
      convrt("(P1,' For Series ',J0UJ10,', Trial ',J0UJ10,"
         "', Cycle ',J0IH10,2H, J0A" qLXN ")", &RP->CP.ser,
         &RP->CP.trial, &CDAT.curr_cyc1, fmtlrlnm(il), NULL);
      }
#endif

   for (ib=il->pib1; ib; ib=ib->pib) {
      struct INHIBBAND *pihb,*pihb0,*pihbe;
      /*  d3news() checks for gbcm*cycles overflow, no need here */
      si32 gbcmul = S24 - (si32)(RP->ntc - CDAT.curr_cyc1)*ib->gbcm;
      int jec = (int)ib->iovec;           /* Overflow count offset */

      if (gbcmul <= 0) gbcmul = 0;

#ifndef PARn
      if (qdbg) {          /*** DEBUG ***/
         convrt("(P1,'  GCONN ',J0IH10,': gbcmul = ',B24IJ8.5)",
            &ib->igt, &gbcmul, NULL);
         if (il->nauts > 0) {
            int naut = (int)il->nauts;
            convrt("(', asmul = ',RB24IJ8.<5)", &naut,
               il->pauts->asmul, NULL);
            }
         }
#endif

      /* Loop over bands, scale beta for each */
      pihb0 = ib->inhbnd;
      pihbe = pihb0 + ib->nib - 1;
      if (gbcmul == 0) for (pihb=pihb0; pihb<=pihbe; ++pihb)
         pihb->asbeta = 0;

      else for (pihb=pihb0; pihb<=pihbe; ++pihb) {
         si32 sclmul = gbcmul;            /* Scale multiplier */

         /* Apply gbcmul and autoscale multiplier to beta.  If there
         *  is autoscaling with OPT=H|W, beta is multiplied by asmul
         *  only if inhibitory (beta < 0), otherwise w/either sign.
         *  Also must take into account whether to use scale for
         *  self- or other input (not the old il != jl test).
         *  d3ascc() sets CDAT.qasov to test USG_QIR bit only if
         *  scale modification is to be done in this cycle.  */
         if (CDAT.qasov & ib->gpspov) {
            int iasop = ib->jself;
            if (pihb->beta < 0) iasop += NSnS;
            iasop = CDAT.kasop[iasop];
            if (iasop != AS_NOP) sclmul = msrsle(sclmul,
               il->pauts->asmul[iasop], -FBsc, jec);
            }
         pihb->asbeta = (sclmul == S24) ? pihb->beta :
            mssle(pihb->beta, sclmul, -FBsc, jec);

#ifndef PARn
         if (qdbg) {       /*** DEBUG ***/
            convrt("(P1,'  Beta = ',B20IJ10.5,', sclmul = ',"
               "B24IJ10.5,', asbeta = ',B20IJ10.5)", &pihb->beta,
               &sclmul, &pihb->asbeta, NULL);
            }
#endif
         } /* End loop over INHIBBANDs */
      } /* End INHIBBLK loop */

   } /* End d3inht() */
