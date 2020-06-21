/* (c) Copyright 1994-2012 Neurosciences Research Foundation, Inc. */
/* $Id: d3inhi.c 52 2012-06-01 19:54:05Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                               d3inhi                                 *
*                                                                      *
*  This file contains d3inhi, a routine to initialize normalization,   *
*  falloff, and self-avoidance for geometric connections, and d3inht,  *
*  a routine to adjust the beta scale factors according to the areas   *
*  of the rings affected and apply autoscale and gbcm factors (and     *
*  any other initialization needed at new-cycle time).                 *
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

#ifndef PAR0

#define SQRTHLF 0.70710678119 /* Constant for adjusting erfc arg */
#define SQRT1PI 0.56418958355 /* sqrt(1/pi) */

/*=====================================================================*
*                               d3inhi                                 *
*                                                                      *
*     Initialization routine for geometric connections.                *
*     Set threshold values to be subtracted if IBOPKR is set.          *
*     Calculate falloff tables and border noise for all layers.        *
*     Initialize normalization tables if serial.                       *
*     Determine whether special calculation for IBOPSA is needed.      *
*     This routine is kept in its own source file because it works     *
*        with both serial (d3inhb.c) and parallel (d3gcon.c) versions  *
*        of the geometric connection evaluation routines.              *
*     This routine is called from main program during setup for a      *
*        new trial series, as some gconn parameters may be changed     *
*        by a 'CHANGE' card.                                           *
*     Double precision is used rather extravagantly in this            *
*        routine to maximize compatibility with IBM version.           *
*=====================================================================*/

void d3inhi(void) {

   struct CELLTYPE *il,*jl;
   struct INHIBBLK *ib;

/* Loop over cell types and INHIBBLKs */

   for (il=RP->pfct; il; il=il->pnct) {
      il->ctf &= ~CTFGV;
      for (ib=il->pib1; ib; ib=ib->pib) {

         jl = ib->pisrc;         /* Ptr to source layer */

/* Set thresholds to be subtracted if IBOPKR, otherwise 0 */

         if (ib->ibopt & IBOPKR)
            ib->subitt = ib->itt, ib->subnitt = ib->ittlo;
         else
            ib->subitt = ib->subnitt = 0;

/* Set IBOEFV bit if special calculation for self-avoidance (IBOPSA)
*  is actually required.  If inner beta is zero, or if there is only
*  one cell per group in the source, a simpler method can be used.  */

         ib->ibopt &= ~IBOEFV;
         if ((ib->ibopt & IBOPSA) && (jl == il)) {
            if (jl->nel > 1) {
               if (ib->inhbnd->beta != 0) {
                  ib->ibopt |= IBOEFV;
                  il->ctf |= CTFGV; }
               }
            else
               ib->inhbnd->beta = 0;
            }

/* Set value for mean border noise.  Note that the negative of the
*  border noise is required, because gpsum returns negative sums.
*  Formulas are derived in design notes p. 27C for case of Gaussian
*  noise added to a fraction nfr of the cells.  In case COMPAT=C,
*  all scales are 7 bits greater than indicated.  ti,ts,tit0 are
*  dimensionless due to division by nsg.  Case nsg == 0, although
*  unlikely, is handled separately to avoid divide by 0 error.
*  V8E, 02/17/09, GNR - Remove addition of vrest to fnmn
*  Rev, 05/05/09, GNR - Subtract gjrev from fnmn
*/

         if (ib->kbc == BC_NOISE) {
            double excess,fnbord,fnmn,fthi,ti,ts,tit0;
            /* Calc noise mean of source cells, nmn (S20)->fnmn (S23)
   e         *  adjusted for rev potl (S7)->fnmn (S23) */
            fnmn = -(double)((jl->No.nmn<<(FBrs-FBwk)) -
               (ib->gjrev<<(FBrs-FBsi)));
            fthi = (double)((long)ib->itt<<(FBrs-FBsi)); /* S7->S23 */
            excess = fthi + fnmn;     /* (inhib thresh - noise mean) */
            /* If nsg == 0, use simple average noise */
            if (jl->No.nsg == 0) fnbord = (excess < 0.0) ?
               (ib->ibopt & IBOPKR ? excess : fnmn) : 0.0;
            /* Otherwise, have to do error integrals */
            else {
               double fnsg = (double)(jl->No.nsg>>1);    /* S24->S23 */
               double fths = (RP->compat & OLDRANGE) ? dS30 : (2*dS30);
               ti = (ts = SQRTHLF/fnsg) * excess;
               tit0 = ib->ibopt & IBOPKR ? ti : ts*fnmn;
               ts *= (fths + fnmn);
               fnbord = SQRTHLF*fnsg*(tit0*rerfc(ti) - ts*rerfc(ts) +
                  SQRT1PI*(exp(-ts*ts) - exp(-ti*ti)));
               }
            if (fnbord < 0.0) {
               fnbord *= (double)jl->nel*(double)jl->No.nfr/dS24;
               ts = (double)(1L << ib->ihsf);
               ib->border = (long)((fnbord - 0.5*ts)/ts);
               }
            else
               ib->border = 0;
            } /* End noise border setup */

/* Make falloff table.
*     The falloff table has one entry for each possible value
*     of the radius metric (d, r, or s) from 0 out to the edge
*     of the cell layer as determined diagonally. */

         if ((ib->ibopt & (IBOPFO|IBOPTT)) == (IBOPFO|IBOPTT))
            mfofftbl(ib);

#ifndef PAR
/* Create entries in norm table (serial only).
*  Each entry is length of largest ring edge that is
*  inside the layer for the given x or y coord--
*  = min(nr1+1+x,nr1+1+(ngx-1-x),2*nr1+1) for x coords
*  = min(nr1+1+y,nr1+1+(ngy-1-y),2*nr1+1) for y coords
*  N.B.  This code moved here from d3genr, 01/01/98, GNR.
*  These tables cannot change during a run, and d3nset is now
*  set up to assure that their location cannot change either.
*  However, it was deemed better to do a small redundant
*  calculation here than to leave this code in d3genr, thus
*  assuring safety if tables are later allowed to move around.
*/
         if (ib->kbc == BC_NORM) {
            struct XYNORMDEF *xy, *xye;
            long leflen, fring, rghtx, rghty;

            rghtx = ib->l1n1 + ib->l1x;   /* Right X */
            rghty = ib->l1n1 + ib->l1y;   /* Right Y */
            leflen= ib->l1n1 + 1;         /* Left length */
            fring = ib->l1n1 + leflen; /* Full outer ring edge */
            xy = ib->xynorm;
            xye = xy + max(ib->l1x, ib->l1y);
            for ( ; xy<xye; xy++,leflen++,rghtx--,rghty--) {
               leflen = min(leflen, fring);
               xy->x  = min(leflen, rghtx);
               xy->y  = min(leflen, rghty);
               }
            }  /* End if BC=NORM */
#endif

         }  /* End of loop over INHIBBLKs */
      }  /* End of loop over CELLTYPEs */
   }  /* End of d3inhi() */

/*=====================================================================*
*                               d3inht                                 *
*                                                                      *
*  Call once per celltype at the beginning of each cycle to perform    *
*  any needed gconn initialization.  Currently:                        *
*  --Scale the beta factors according to the area of the rings         *
*     affected, apply autoscale and gbcm adjustments.                  *
*  Note, 05/24/12, GNR - Former d3inhs() routine was merged into       *
*     this routine, eliminating confusing updating of beta factors     *
*     in place, at cost of a small computation per cycle.  Tests       *
*     for gbcm*cycles underflow and for extended area too large        *
*     were moved to d3news() (Node 0 code).                            *
*=====================================================================*/

void d3inht(struct CELLTYPE *il) {

   extern struct CELLDATA CDAT;
   struct INHIBBLK *ib;

   for (ib=il->pib1; ib; ib=ib->pib) {
      struct INHIBBAND *pihb,*pihb0,*pihbe;
      struct CELLTYPE *jl = ib->pisrc;    /* Ptr to source layer */
      si32 sclmul;                        /* Scale multiplier */
      /*  d3news() checks for gbcm*cycles overflow, no need here.*/
      si32 gbcmul = S24 - (si32)(RP->ntc - CDAT.curr_cyc1)*ib->gbcm;
      /* d3go() turns off KAUT_G bit if KAUT_ET on first cycle or
      *  KAUT_IMM on any cycle, no need to check those bits here.  */
      int dobs = il->kautu & KAUT_G && il != jl;

      sclmul = (gbcmul <= 0) ? 0 : msrsle(gbcmul,
         dobs ? RP->psclmul[il->oautsc] : S24, -FBsc, OVF_GEOM);

      /* Loop over bands, scale beta for each */
      pihb0 = ib->inhbnd;
      pihbe = pihb0 + ib->nib - 1;
      if (sclmul == 0) for (pihb=pihb0; pihb<=pihbe; ++pihb)
         pihb->asbeta = 0;
      else {
         si32 rsi = 0;                    /* Ring scale increment */
         si32 scale = jl->nel;            /* No. cells in 1st ring */
         for (pihb=pihb0; pihb<=pihbe; ++pihb) {

            if ((pihb == pihbe) && (ib->ibopt & IBOPTX)) {
               /* Area of outer band with OPT=X is size of source
               *  minus size of inner square.  */
               long edge = ((ib->ngb*(ib->nib-2))<<1) + 1;
               scale = jl->nelt - edge*edge*jl->nel;
               }
            pihb->asbeta = ((sclmul == S24) ? pihb->beta :
               mssle(pihb->beta, sclmul, -FBsc, OVF_GEOM)) / scale;
            /* Update scale according to ring area */
            if (pihb == pihb0) {
               scale *= (ib->ngb<<2);
               rsi = scale*ib->ngb;
               scale += rsi;
               rsi <<= 1;
               }
            else
               scale += rsi;
            } /* End loop over INHIBBANDs */
         }  /* End if nonzero scale multiplier */
      } /* End INHIBBLK loop */

   } /* End d3inht() */
#endif   /* ifndef PAR0 */
