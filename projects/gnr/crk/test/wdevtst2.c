/* (c) Copyright 2010, George N. Reeke, Jr. */
/***********************************************************************
*                                                                      *
*                             WDEVTST2.C                               *
*                                                                      *
*  Test wdev() algorithm with the open-source "dieharder" package.     *
*                                                                      *
*  This program calculates large batches of random numbers using       *
*  nwdevs() and outputs them on stdout.  The user is expected to       *
*  pipe these to dieharder using the construct:                        *
*     wdevtst2 seed27 seed31 | dieharder - g 200 -a > results.txt      *
*                                                                      *
*----------------------------------------------------------------------*
*  V1A, 02/18/10, GNR - New routine                                    *
***********************************************************************/

#define MAIN
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "sysdef.h"
#include "rocks.h"
#include "rkxtra.h"
#include "rkwdev.h"

#define NUM    1024

extern FILE *stdout;
extern FILE *stderr;

int main(int argc, char *argv[]) {

   wseed wsd;
   si32 rand[NUM];

   static char emsg[] = "Synopsis: wdevtst2 seed27 seed31 | "
      "dieharder -g 200 -a > results.txt\n";

   if (argc != 3 || cntrl(argv[1]) || cntrl(argv[2])) {
      fwrite(emsg, sizeof(emsg), 1, stderr);
      exit(1);
      }

   wsd.seed27 = (si32)atoi(argv[1]);
   wsd.seed31 = (si32)atoi(argv[2]);

   while (1) {
      nwdevs(rand, &wsd, NUM);
      fwrite((char *)rand, sizeof(si32), NUM, stdout);
      }

   } /* End main */

