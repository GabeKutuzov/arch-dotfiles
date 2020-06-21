/***********************************************************************
*                                                                      *
*                             WDEVSKTS.C                               *
*                                                                      *
*  Test wdevskip.c                                                     *
*                                                                      *
*  This program calculates and prints 4 tables of wdevs with varying   *
*     skips between them.  Two tables are for 64-bit mode, two for     *
*     32-bit compatibility mode.  The skips are executed both with     *
*     wdev and with wdevskip calls to be sure the latter are correct.  *
************************************************************************
*  V1A, 02/07/10, GNR, derived from udevskts                           *
***********************************************************************/

#define MAIN
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "sysdef.h"
#include "rocks.h"
#include "rkarith.h"
#include "rkwdev.h"

#define  NUM          15
#define qNUM         "15"
#define  USEED    314159
#define  WSEED27  597483
#define  WSEED31 9063117

int main() {

   double time1u,time1w,time1s,time2w,time2s;
   wseed wsd;
   si32 ran1u[NUM],ran1w[NUM],ran1s[NUM],ran2w[NUM],ran2s[NUM];
   si32 usd;
   static long skip[NUM] = {0,1,2,3,4,7,16,547,1023,1024,
      2047,15384,47563,69999,131072};
   int i,j,nerr1w,nerr1s,nerr2s;

/* Develop the compatibility series */

   second();
   usd = USEED;
   for (i=0; i<NUM; i++) {
      for (j=0; j<skip[i]; j++) udev(&usd);
      ran1u[i] = udev(&usd);
      }
   time1u = second();
   wsd.seed27 = -1, wsd.seed31 = USEED;
   for (i=0; i<NUM; i++) {
      for (j=0; j<skip[i]; j++) wdev(&wsd);
      ran1w[i] = wdev(&wsd);
      }
   time1w = second();
   wsd.seed27 = -1, wsd.seed31 = USEED;
   for (i=0; i<NUM; i++) {
      wdevskip(&wsd, skip[i]);
      ran1s[i] = wdev(&wsd);
      }
   time1s = second();

/* Develop the second (two-seed wdevs) series */

   wsd.seed27 = WSEED27, wsd.seed31 = WSEED31;
   for (i=0; i<NUM; i++) {
      for (j=0; j<skip[i]; j++) wdev(&wsd);
      ran2w[i] = wdev(&wsd);
      }
   time2w = second();
   wsd.seed27 = WSEED27, wsd.seed31 = WSEED31;
   for (i=0; i<NUM; i++) {
      wdevskip(&wsd, skip[i]);
      ran2s[i] = wdev(&wsd);
      }
   time2s = second();

/* Print results */

   nerr1w = nerr1s = nerr2s = 0;
   for (i=0; i<NUM; i++) {
      if (ran1w[i] != ran1u[i]) ++nerr1w;
      if (ran1s[i] != ran1u[i]) ++nerr1s;
      if (ran2s[i] != ran2w[i]) ++nerr2s;
      }

   cryout(RK_P1,"0        udev        wdev    wdevskip (compat mode)",
      RK_LN2, NULL);
   convrt("(" qNUM "<1X,3*IJ12/>)", ran1u, ran1w, ran1s, NULL);
   convrt("(' errors      ',2*I12)", &nerr1w, &nerr1s, NULL);
   convrt("(' times',Q7.4,2*Q12.4)", &time1u, &time1w, &time1s, NULL);

   cryout(RK_P1,"0        wdev    wdevskip (58-bit mode)",
      RK_LN2, NULL);
   convrt("(" qNUM "<1X,2*IJ12/>)", ran2w, ran2s, NULL);
   convrt("(' errors      ',I12)", &nerr2s, NULL);
   convrt("(' times',Q7.4,Q12.4)", &time2w, &time2s, NULL);

   cryout(RK_P1,"\0",RK_LN0+RK_FLUSH+1,NULL);
   return 0;
   } /* End udevskts */


