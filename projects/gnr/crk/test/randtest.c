/**********************************************************************
*                                                                     *
*                             RANDTEST.C                              *
*                                                                     *
**********************************************************************/

/*
   Test udev.assemble, ndev.assemble, rannum.c, and rannor.c

   V1A, 03/14/89, GNR
   V1B, 03/24/89, GNR - Add test of udev, ndev
   V1C, 11/16/09, GNR - Small revisions for 64-bit compilation
   V1D, 11/14/11, GNR - Add a couple of tests for si32perm, si16perm

   This program calculates and prints 3 tables of rannum's with
      different shifts and one table of rannor's.  The results
      are to be compared with those calculated by the IBM assembler
      version, which are considered to be "correct" by definition.
*/

#define MAIN
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "sysdef.h"
#include "rocks.h"
#include "rkarith.h"
#include "normtab.h"

#define NUM    10
int main() {

   si32 ran1[NUM],ran2[NUM],ran3[NUM];
   float ran4[NUM];
   si32 udevl[NUM],ndevl[NUM],nds[NUM];
   si16 sp1[NUM],sp2[NUM];
   si32 seed;
   int i;

   seed = 1009;
   rannum(ran1,NUM,&seed,0);
   rannum(ran2,NUM,&seed,21);
   rannum(ran3,NUM,&seed,-1);
   rannor(ran4,NUM,&seed,1.0F,0.33333F);

   seed = 1009;
   for (i=0;i<NUM;i++) udevl[i] = udev(&seed);
   for (i=0;i<(2*NUM);i++) udev(&seed);
   for (i=0;i<NUM;i++) {
      ndevl[i] = ndev(&seed,1L<<24,(1L<<28)/3);
      nds[i] = seed; }

   cryout(RK_P2,"0UDEV + RANNUM results:",RK_SUBTTL+RK_LN2,
      "        udev   rannum(0)  rannum(21)  rannum(-1)",RK_LN1,NULL);
   convrt("(10<IJ12,3*IJ12/>)",udevl,ran1,ran2,ran3,NULL);

   cryout(RK_P2,"0NDEV + RANNOR results:",RK_NFSUBTTL+RK_LN2,
      "        ndev      rannor        seed",RK_LN1,NULL);
   convrt("(10<B24IJ12.7,F12.7,IJ12/>)",ndevl,ran4,nds,NULL);

   for (i=0; i<NUM; ++i) {
      ran1[i] = ran2[i] = i;
      sp1[i] = i; sp2[i] = 3*i + 1;
      }
   si32perm(ran1, &seed, 7);
   si32perm(ran2, &seed, 10);
   si16perm(sp1,  &seed, 8);
   si16perm(sp2,  &seed, 10);
   convrt("('0si32perm of 0-6: ',7IJ4)", ran1, NULL);
   convrt("(' si32perm of 0-9: ',10IJ4)", ran2, NULL);
   convrt("('0si16perm of 0-7:    ',8IH4)", sp1, NULL);
   convrt("(' si16perm of 1+3-28: ',10IH4)", sp2, NULL);
   cryout(RK_P2,"\0",1+RK_FLUSH,NULL);
   return 0;
   } /* End RANDTEST */


