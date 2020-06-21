/* (c) Copyright 2011, The Rockefeller University *11113* */
/***********************************************************************
*                              CITINV.C                                *
*                                                                      *
*  "Quick-and-dirty" to process CIT101 image database inventory        *
***********************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>

#define MAIN

#include "sysdef.h"
#include "rocks.h"
#include "rkxtra.h"

#define MXCATNM   32

int main(int argc, char *argv[]) {

   int icat = 0;              /* Category number */
   int nfic,ifst;             /* Number files, first file */
   int tnf = 0;               /* Running total nfic */
   char catnm[MXCATNM];       /* Category name */

   while (cryin()) {
      inform("(SW1,A32,I)", catnm, &nfic, NULL);
      icat += 1;
      ifst = tnf + 1;
      tnf += nfic;
      convrt("(I4,2*I6,2H  ,A32)", &icat, &ifst, &nfic, catnm, NULL);
      }

   convrt("('0Total files: ',J0I6)", &tnf, NULL);
   cryout(RK_P1,"\0",RK_LN0+RK_FLUSH+1,NULL);

   } /* End citinv */
