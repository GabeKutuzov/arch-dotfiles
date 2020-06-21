/**********************************************************************
*                                                                     *
*                             UDEVSKTS.C                              *
*                                                                     *
**********************************************************************/

/*
   Test udevskip.c

   V1A, 07/24/89, GNR
   Rev, 05/04/16, GNR - Add udevwskp tests

   This program calculates and prints 2 tables of udevs with varying
      skips between them.  The skips are executed both with udev
      calls and with udevskip calls to be sure the latter are correct.
*/

#define MAIN
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "sysdef.h"
#include "rocks.h"
#include "rkarith.h"

#define  NUM       10
#define  SEED  314159
int main() {

   double time1,time2,time3;
   si32 ran1[NUM],ran2[NUM],ran3[NUM];
   si32 j,seed;
   static si32 skip[NUM] = {0,1,2,3,7,16,547,1023,1024,47563};
   int i;

/* Develop the first series */

   second();
   seed = SEED;
   for (i=0; i<NUM; i++) {
      for (j=0; j<skip[i]; j++) udev(&seed);
      ran1[i] = udev(&seed);
      }
   time1 = second();

/* Develop the second series */

   seed = SEED;
   for (i=0; i<NUM; i++) {
      udevskip(&seed,skip[i]);
      ran2[i] = udev(&seed);
      }
   time2 = second() - time1;

/* Develop the third series--use the same skips
*  except times (2**31-1)--should give same answers */

   seed = SEED;
   for (i=0; i<NUM; i++) {
      si64 bigskip = jmsw(skip[i], 0x7fffffff);
      udevwskp(&seed, bigskip);
      ran3[i] = udev(&seed);
      }
   time3 = second() - time2; 

/* Print results */

   cryout(RK_P1,"0       udev    udevskip    udevwskp",RK_LN2+36,NULL);
   for (i=0; i<NUM; i++) 
      convrt("(3*IJ12)",&ran1[i],&ran2[i],&ran3[i],NULL);
   convrt("(' time',Q7.4,2*Q12.4)",&time1,&time2,&time3,NULL);
   cryout(RK_P1,"\0",RK_LN0+RK_FLUSH+1,NULL);
   return 0;
   } /* End udevskts */


