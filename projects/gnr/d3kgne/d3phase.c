/* (c) Copyright 1991-2009, Neurosciences Research Foundation, Inc. */
/* $Id: d3phase.c 23 2009-05-14 19:04:57Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                               d3phase                                *
*           Calculate amplitude and phase of cell response             *
*                                                                      *
*  Note:  In code earlier than V8D, some calculations use bin value    *
*  and some use (bin - pht) for the case (bin > pht).  In V8D and      *
*  later, this usage is under control of PHASE card option D (PHOPD,   *
*  corresponding to cell response STEP option) except that RANDOM      *
*  phase always uses KNEE.  COMPAT=C forces use of the old options     *
*  in all cases.  Note however that the value of cell response         *
*  returned is subject to the STEP vs KNEE functions in d3go().        *
************************************************************************
*  Original version by Michael Cook, undated                           *
*  Rev, 11/15/91, GNR - Add OPT=M--divide ft by its max value          *
*  Rev, 08/08/92, GNR - Use SRA, PHASE_MASK where needed               *
*  Rev, 07/23/94, GNR - Pass results and input cell data in CELLDATA   *
*  V8A, 09/12/95, GNR - Return response amplitude as function value,   *
*                       enhance accuracy of trig table to S16, change  *
*                       scale of accumulation bins to S24, replace     *
*                       st with pht, add d3gm(), misc. optimizations   *
*  Rev, 03/17/97, GNR - Change centroid simeth to work as documented,  *
*                       set indeterminate centroid or first phases to  *
*                       random, not previous, phase as documented, do  *
*                       not spike if bin is below threshold, change    *
*                       all threshold tests to bin >=, not > thresh    *
*  V8C, 05/04/03, GNR - Change scale of accumulation bins to S20       *
*  V8D, 06/03/06, GNR - Delete gmphaff(), convolve()                   *
*  ==>, 08/30/07, GNR - Last mod before committing to svn repository   *
*  Rev, 12/31/08, GNR - Replace jm64sh with mssle                      *
*  V8E, 02/12/09, GNR - Phase params to PHASEDEF block                 *
***********************************************************************/

#define PI       3.14159265357989

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "sysdef.h"
#include "rkarith.h"
#include "d3global.h"
#include "celldata.h"

extern struct CELLDATA CDAT;

/* Table of sin(x), x=2*PI*I/32, I=0,39 (1-1/4 periods).  Thus,
*  sin(arg) = mysin[i], cos(arg) = mysin[i+8] for arg = 2*PI*i/32. */

const double binsin[40] = {
   0.000000000, 0.195090322, 0.382683432, 0.555570233,
   0.707106781, 0.831469612, 0.923879533, 0.980785280,
   1.000000000, 0.980785280, 0.923879533, 0.831469612,
   0.707106781, 0.555570233, 0.382683432, 0.195090322,
   0.000000000, -.195090322, -.382683432, -.555570233,
   -.707106781, -.831469612, -.923879533, -.980785280,
   -1.00000000, -.980785280, -.923879533, -.831469612,
   -.707106781, -.555570233, -.382683432, -.195090322,
   0.000000000, 0.195090322, 0.382683432, 0.555570233,
   0.707106781, 0.831469612, 0.923879533, 0.980785280 };

/*=====================================================================*
*                               d3phase                                *
*                                                                      *
*  Synopsis: long d3phase(struct CELLTYPE *)                           *
*                                                                      *
*  Calculate the amplitude and phase of cell response using methods    *
*  specified by ppd->simeth, ppd->phimeth, respectively.  A pointer    *
*  to the input distribution is in CDAT.pCellResp.  This is supposed   *
*  to be a probability distribution, hence always positive, but some   *
*  defensive coding is included to do something reasonable if there    *
*  are some negative probabilities.                                    *
*                                                                      *
*  Returns:                                                            *
*     Cell response (mV S20)                                           *
*                                                                      *
*  NOTE: cell_resp here is S20, tpd is S20, thresholds are S20.        *
*=====================================================================*/

long d3phase(struct CELLTYPE *il) {

   struct PHASEDEF *ppp = il->pctpd;/* Ptr to phase parameters   */
   long *ppd = CDAT.pCellResp;      /* Ptr to phase distribution */
   double xcoord, ycoord;           /* Accumulators for centroid */
   long cell_resp;                  /* Holds new cell response   */
   long rand;                       /* A random number (uniform) */
   long tpd[PHASE_RES];             /* Thresholded phase dist    */
   long tth = ppp->pht;             /* Phasing threshold         */
   int new_phasei;                  /* Holds new phase of si     */
   register int p;                  /* Phase angle loop counter  */

   static double norm = 1.0/(double)PHASE_RES;

/* If OPT=M was specified and max(|pd|) > 128 mV, scale pd to
*  a new max of 128 mV (old 1.0) per suggestion of G.Tononi.
*  N.B.  In compatibility mode, do not rescale negatives.  */

   if (ppp->phops & PHOPM) {
      long tmx,mx = -LONG_MAX;
      if (ppp->phops & PHOPC) for (p=0; p<PHASE_RES; p++) {
         tmx = ppd[p];
         if (tmx > mx) mx = tmx; }
      else                    for (p=0; p<PHASE_RES; p++) {
         tmx = labs(ppd[p]);
         if (tmx > mx) mx = tmx; }
      if (mx > (128*S20)) {
         mx = ds64nq(64*S20, 0, 0, mx); /* 128S20/mx (S31) */
         for (p=0; p<PHASE_RES; p++)
            ppd[p] = mssle(ppd[p], mx, -31, OVF_PHAS);
         }
      } /* End PHOPM */

/* Store thresholded phase distribution according to STEP vs
*  KNEE mode to simplify some of the following arithmetic.
*  (Compatibility mode uses STEP except for RANDOM phase.)  */

   if (ppp->phops & (PHOPD|PHOPC)) for (p=0; p<PHASE_RES; p++)
      tpd[p] = (ppd[p] >= tth) ? ppd[p] : 0;
   else                            for (p=0; p<PHASE_RES; p++) {
      long ttt = ppd[p] - tth;
      tpd[p] = (ttt > 0) ? ttt : 0; }

/* If either phi or si is to be calculated by the centroid method,
*  get the sums of the tpd's that are above threshold, considered
*  as vectors in the complex plane.  */

   if (il->ctf & CTFCN) {
      xcoord = ycoord = 0.0;
      for (p=0; p<PHASE_RES; p++) if (tpd[p] > 0) {
         xcoord += (double)tpd[p] * binsin[p+8];
         ycoord += (double)tpd[p] * binsin[p];
         }
      }

/*---------------------------------------------------------------------*
*             Calculate phi according to requested method              *
*---------------------------------------------------------------------*/

/* Note:  All cases in this switch except constant phase generate
*  one random number whether or not it needs to be used.  This is
*  to keep results consistent when cells are divided differently
*  across computational nodes in a parallel machine.  */

   switch (ppp->phimeth) {

      case PHI_CENTROID:            /* CENTROID PHASE */

/* Get the centroid of the tpd's considered as vectors in the
*  plane.  Then pick the phase angle of the centroid (same
*  as phase angle of the sum).  Map the result to [0,31].  */

         rand = udev(&ppp->phiseed); /* Whether used or not */
         /* Use & for one test, not && for two... */
         if (xcoord == 0.0 & ycoord == 0.0)
            new_phasei = (int)(rand & PHASE_MASK);
         else {
            /* Map [-PI,PI] --> [0, 31] with rounding */
            double tphi = (16.0/PI) * atan2(ycoord, xcoord);
            new_phasei = (int)(tphi + ((double)PHASE_RES + 0.5)) &
               PHASE_MASK;
            }
         break;

      case PHI_RANDOM: {            /* RANDOM PHASE */

/* Let cum = sum over p of ppd[p] >= pht, then generate a uniformly
*  distributed random number rand and break at the point where the
*  cumulative distribution of ppd exceeds cum*rand.  This is "using
*  the dpd as a probability distribution," so KNEE rather than STEP
*  is always used, as in earlier versions.  Scale everything by
*  PHASE_EXP to prevent overflow.  */

         long cum = 0, sum = 0;
         rand = udev(&ppp->phiseed);
         /* Get the total of probabilities over threshold */
         for (p=0; p<PHASE_RES; p++) if (tpd[p] > 0)
            cum += (ppd[p] - tth) >> PHASE_EXP;
         if (cum <= 0.0)
            new_phasei = (int)(rand & PHASE_MASK);
         else {
            cum = mssle(cum, rand, -31, OVF_PHAS);
            for (p=0; p<PHASE_RES; p++) if (tpd[p] > 0) {
               sum += (ppd[p] - tth) >> PHASE_EXP;
               if (sum > cum) break;
               }
            new_phasei = p;
            }
         break;
         } /* End local scope */

      case PHI_FIRST:               /* FIRST PHASE */

/* Pick the first p such that ppd[p] >= pht */

         rand = udev(&ppp->phiseed);
         for (p=0; p<PHASE_RES; p++) if (tpd[p] > 0) {
            new_phasei = p; goto GOT_FIRST; }
         new_phasei = (int)(rand & PHASE_MASK);
GOT_FIRST:
         break;

      case PHI_MODE: {              /* MODE PHASE */

/* Pick phase with maximum amplitude */

         long mx = -LONG_MAX;
         long count = 0;
         rand = udev(&ppp->phiseed);
         /* Find the max amp and count how many attain it */
         for (p=0; p<PHASE_RES; p++)
            if (ppd[p] > mx) {
               new_phasei = p;
               mx = ppd[p];
               count = 1;
               }
            else if (ppd[p] == mx) {
               count++;
               }

/* If the mode is less than threshold, pick phase at random */

         if (mx < tth) {
            new_phasei = (int)(rand & PHASE_MASK);
            break;
            }

/* Otherwise, use phase stored above.  If more than one phase has
*  the same amplitude, pick one of them at random.  This action
*  will be very rare, so a speedier algorithm would be useless.  */

         if (count > 1) {
            rand %= count;
            for (p=0; p<PHASE_RES; p++) {
               if (ppd[p] == mx && rand-- == 0) {
                  new_phasei = p; break; }
               }
            }
         break;
         } /* End local scope */

      case PHI_CONST:               /* CONSTANT PHASE */

/* New phase = user supplied constant */

         new_phasei = ppp->fixpha;
         break;

      default:
         break;
      } /* End phimeth switch */

/*---------------------------------------------------------------------*
*             Calculate si according to requested method               *
*---------------------------------------------------------------------*/

   switch (ppp->simeth) {

      case SI_CENTROID:             /* CENTROID SI */
         cell_resp = (long)(norm*sqrt(xcoord*xcoord + ycoord*ycoord));
         break;

      case SI_HEIGHT:               /* SI AT SELECTED PHASE */
         cell_resp = ppd[new_phasei];     /* Already S20 */
         break;

      case SI_CONST:                /* CONSTANT SI */
         cell_resp = (tpd[new_phasei] > 0) ? ppp->fixsi : 0;
         break;

      case SI_SUMPOS:               /* SUM OF SUPERTHRESHOLD SI */
         cell_resp = 0;
         for (p=0; p<PHASE_RES; p++) if (tpd[p] > 0)
            addck(cell_resp, tpd[p], OVF_PHAS);
         break;

      case SI_MEAN:                 /* MEAN OF SUPERTHRESHOLD SI */
         cell_resp = 0;
         for (p=0; p<PHASE_RES; p++) if (tpd[p] > 0)
            cell_resp += tpd[p] >> PHASE_EXP;
         break;

      default:
         break;

      } /* End simeth switch */

   CDAT.new_phi = (byte)new_phasei;
   return cell_resp;

   } /* End d3phase() */
