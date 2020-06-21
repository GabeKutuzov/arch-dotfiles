/* (c) Copyright 2010, George N. Reeke, Jr. */
/***********************************************************************
*                                                                      *
*                             WDEVTEST.C                               *
*                                                                      *
*  Test wdev() and related "wide" random number generators.            *
*  This program calculates and prints one table of random numbers      *
*  with each routine under test and compares with the correct          *
*  results obtained for wdev() by exact integer calculation with       *
*  dc.  This is just a test of the math--statistics for randomness     *
*  etc. should be calculated with a suitable 3d party package.         *
*                                                                      *
*  Note:  For the derived (e.g. Gaussian) results, the "correct"       *
*  answers are obtained here by calculation from the wdev() results.   *
*  (Except wndev() is also tested again ndev() in compatible mode.)    *
*  Strictly, these calculations should have been done independently,   *
*  but they are so trivial we are at least testing that the wdevxx     *
*  routines are not broken in some stupid way.                         *
*----------------------------------------------------------------------*
*  V1A, 01/04/10, GNR - New routine, based on old randtest             *
***********************************************************************/

#define MAIN
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "sysdef.h"
#include "rocks.h"
#include "rkarith.h"
#include "rkwdev.h"
#include "normtab.h"

#define NUM    10
#define qNUM  "10"
#define b22 0x00400000
#define w31 0x7fffffff
#define Tmean  (1<<24)
#define Tsigma ((1<<28)/3)
#define Dmean  1.0
#define Dsigma (1.0/3.0)

int main() {

   /* Names ending in 'r' = returned, in 'c' = correct */
   wseed wdsr[NUM], wdsc[NUM], seed, seed0;  /* Seeds */
   si32 useed, wdevr[NUM], wdevc[NUM];       /* Results */
   si32 wndevr[NUM], wndevc[NUM], ndevr[NUM], ndevc[NUM];
   si32 wdevsr[NUM], wdevsc[NUM];
   ui32 wdevir[NUM], wdevic[NUM];
   float wdevfr[NUM], wdevfc[NUM], wndevfr[NUM], wndevfc[NUM];
   double wdevdr[NUM], wdevdc[NUM], wndevdr[NUM], wndevdc[NUM];
   int i, nerr = 0;

   seed0.seed27 = 4093, seed0.seed31 = 65535;
   wdsc[0].seed27 = 107904068, wdsc[0].seed31 = 1101446745;
   wdsc[1].seed27 = 131296584, wdsc[1].seed31 =  706406075;
   wdsc[2].seed27 =  85094722, wdsc[2].seed31 = 1277301909;
   wdsc[3].seed27 =  28950806, wdsc[3].seed31 = 1366649151;
   wdsc[4].seed27 =  67256403, wdsc[4].seed31 = 1934676192;
   wdsc[5].seed27 =  80706865, wdsc[5].seed31 = 1052859717;
   wdsc[6].seed27 = 128954516, wdsc[6].seed31 =  148012339;
   wdsc[7].seed27 =  47955465, wdsc[7].seed31 =  857318347;
   wdsc[8].seed27 = 121058251, wdsc[8].seed31 = 1481670306;
   wdsc[9].seed27 =   5262011, wdsc[9].seed31 =  212462330;

/* Test wdev() */

   seed = seed0;
   for (i=0; i<NUM; i++) {
      si32 r = wdsc[i].seed27;
      r = (r >> 15) | (r << 17);    /* Emulate rotate */
      wdevc[i] = wdsc[i].seed31 ^ (r & 0x7fffffff);
      wdevr[i] = wdev(&seed); wdsr[i] = seed;
      if (wdevr[i] != wdevc[i] ||
         seed.seed27 != wdsc[i].seed27 ||
         seed.seed31 != wdsc[i].seed31) ++nerr;
      }
   cryout(RK_P2,"0WDEV results:",RK_SUBTTL+RK_LN3,
      "        wdev     correct                 newseed"
      "                 correct", RK_LN0, NULL);
   convrt("(" qNUM "<IJ12,IJ12,IS24,IS24/>)", wdevr, wdevc, wdsr,
      wdsc, NULL);

/* Test wdevs(), wdevi(), wdevf(), wdevd() */

   seed = seed0;
   for (i=0; i<NUM; i++) {
      wdevsc[i] = wdevc[i] | wdsc[i].seed27 << 17 & 0x80000000;
      wdevsr[i] = wdevs(&seed);
      if (wdevsr[i] != wdevsc[i]) ++nerr;
      }
   seed = seed0;
   for (i=0; i<NUM; i++) {
      ui64 rs = jmuw((ui32)wdevc[i] << 1, (ui32)i + 99);
      wdevic[i] = uwhi(rs);
      wdevir[i] = wdevi(&seed, (ui32)i + 99);
      if (wdevir[i] != wdevic[i]) ++nerr;
      }
   seed = seed0;
   for (i=0; i<NUM; i++) {
      wdevfc[i] = (float)wdevc[i]/2147483648.0;
      wdevfr[i] = wdevf(&seed);
      if (wdevfr[i] != wdevfc[i]) ++nerr;
      }
   seed = seed0;
   for (i=0; i<NUM; i++) {
      wdevdc[i] = (double)wdevc[i]/2147483648.0;
      wdevdr[i] = wdevd(&seed);
      if (wdevdr[i] != wdevdc[i]) ++nerr;
      }

   cryout(RK_P2,
      "0       wdevs     correct       wdevi     correct"
      "       wdevf     correct       wdevd     correct",
      RK_NFSUBTTL+RK_LN2, NULL);
   convrt("(" qNUM "<1X,4*IJ12,2*F12.9,2*Q12.9/>)",
      wdevsr, wdevsc, wdevir, wdevic,
      wdevfr, wdevfc, wdevdr, wdevdc, NULL);

/* Test wndev(), wndevf(), wndevd(), wndev(compat) */

   seed = seed0;
   for (i=0; i<NUM; i++) {
      si32 ruc = wdevsc[i];
      ndevr[i] = ((ruc & 0x003fffff) << 3) |
          ((si32)normtab[(ruc & w31) >> 23] << 25);
      ruc = jm64nh(Tsigma, ndevr[i]);
      if (wdevsc[i] & b22) ruc = -ruc;
      wndevc[i] = Tmean + ruc;
      wndevr[i] = wndev(&seed, Tmean, Tsigma);
      if (wndevr[i] != wndevc[i]) ++nerr;
      }
   seed = seed0;
   for (i=0; i<NUM; i++) {
      si32 ruc = ndevr[i];
      if (wdevsc[i] & b22) ruc = -ruc;
      wndevfc[i] = (float)ruc * (float)Dsigma / 268435456.0 + Dmean;
      wndevfr[i] = wndevf(&seed, Dmean, Dsigma);
      if (fabsf(wndevfr[i] - wndevfc[i]) > 0.000001) ++nerr;
      }
   seed = seed0;
   for (i=0; i<NUM; i++) {
      si32 ruc = ndevr[i];
      if (wdevsc[i] & b22) ruc = -ruc;
      wndevdc[i] = (double)ruc * Dsigma / 268435456.0 + Dmean;
      wndevdr[i] = wndevd(&seed, Dmean, Dsigma);
      if (fabs(wndevdr[i] - wndevdc[i]) > 1E-12) ++nerr;
      }
   seed.seed27 = -1;
   seed.seed31 = useed = 1009;
   for (i=0; i<NUM; i++) {
      ndevc[i] = ndev(&useed, Tmean, Tsigma);
      ndevr[i] = wndev(&seed, Tmean, Tsigma);
      if (ndevr[i] != ndevc[i]) ++nerr;
      }

   cryout(RK_P2,
      "0       wndev     correct      wndevf     correct"
      "      wndevd     correct     vs ndev     correct",
      RK_NFSUBTTL+RK_LN2, NULL);
   convrt("(" qNUM "<1X,2*B24IJ12.8,2*F12.8,2*Q12.8,2*B24IJ12.8/>)",
      wndevr, wndevc, wndevfr, wndevfc,
      wndevdr, wndevdc, ndevr, ndevc, NULL);

/* Test nwdev() */

   seed = seed0;
   nwdev(wdevr, &seed, NUM);
   for (i=0; i<NUM; i++) {
      if (wdevr[i] != wdevc[i]) ++nerr;
      }
   cryout(RK_P2,"0       nwdev     correct",
      RK_NFSUBTTL+RK_LN2, NULL);
   convrt("(" qNUM "<IJ12,IJ12/>)", wdevr, wdevc, NULL);

/* Test nwdevs(), nwdevi(), nwdevf(), nwdevd() */

   seed = seed0;
   nwdevs(wdevsr, &seed, NUM);
   for (i=0; i<NUM; i++) {
      if (wdevsr[i] != wdevsc[i]) ++nerr;
      }
   seed = seed0;
   nwdevi(wdevir, &seed, NUM, 99);
   for (i=0; i<NUM; i++) {
      ui64 rs = jmuw((ui32)wdevc[i] << 1, 99);
      wdevic[i] = uwhi(rs);
      if (wdevir[i] != wdevic[i]) ++nerr;
      }
   seed = seed0;
   nwdevf(wdevfr, &seed, NUM);
   for (i=0; i<NUM; i++) {
      if (wdevfr[i] != wdevfc[i]) ++nerr;
      }
   seed = seed0;
   nwdevd(wdevdr, &seed, NUM);
   for (i=0; i<NUM; i++) {
      if (wdevdr[i] != wdevdc[i]) ++nerr;
      }

   cryout(RK_P2,
      "0      nwdevs     correct      nwdevi     correct"
      "      nwdevf     correct      nwdevd     correct",
      RK_NFSUBTTL+RK_LN2, NULL);
   convrt("(" qNUM "<1X,4*IJ12,2*F12.9,2*Q12.9/>)",
      wdevsr, wdevsc, wdevir, wdevic,
      wdevfr, wdevfc, wdevdr, wdevdc, NULL);

/* Test nwndev(), nwndevf(), nwndevd() */

   seed = seed0;
   nwndev(wndevr, &seed, NUM, Tmean, Tsigma);
   for (i=0; i<NUM; i++) {
      if (wndevr[i] != wndevc[i]) ++nerr;
      }
   seed = seed0;
   nwndevf(wndevfr, &seed, NUM, Dmean, Dsigma);
   for (i=0; i<NUM; i++) {
      if (fabsf(wndevfr[i] - wndevfc[i]) > 0.000001) ++nerr;
      }
   seed = seed0;
   nwndevd(wndevdr, &seed, NUM, Dmean, Dsigma);
   for (i=0; i<NUM; i++) {
      if (fabs(wndevdr[i] - wndevdc[i]) > 1E-12) ++nerr;
      }

   cryout(RK_P2,
      "0       wndev     correct      wndevf     correct"
      "      wndevd     correct", RK_NFSUBTTL+RK_LN2, NULL);
   convrt("(" qNUM "<1X,2*B24IJ12.8,2*F12.8,2*Q12.8/>)",
      wndevr, wndevc, wndevfr, wndevfc, wndevdr, wndevdc, NULL);

/* All done */

   convrt("('0wdev tests completed, ',J1I6,'errors.')",
      &nerr, NULL);
   cryout(RK_P1,"\0",RK_LN0+RK_FLUSH+1,NULL);
   return 0;
   } /* End WDEVTEST */


