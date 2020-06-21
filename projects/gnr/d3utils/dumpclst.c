/* (c) Copyright 2010, The Rockefeller University *11113* */
/***********************************************************************
*                              dumpclst                                *
*                                                                      *
*  This is a CNS utility program to dump a CONNLIST (connection list)  *
*  file.                                                               *
*                                                                      *
*  Synopsis:                                                           *
*     dumpclst <connlist file>                                         *
*                                                                      *
*  This program is assigned error codes in the range 558-559.          *
************************************************************************
*  V1A, 06/02/10, GNR - New program, based on norbrand.c               *
*  Rev, 06/25/10, GNR - Add source cell decoder                        *
***********************************************************************/

#define MAIN

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "rocks.h"
#include "rfdef.h"
#include "connlist.h"

#define DECODE
#define NROTS 18
#define NXIMG 56

/*=====================================================================*
*                        dumpclst main program                         *
*=====================================================================*/

int main(int argc, char *argv[]) {

   struct RFdef *pclf;        /* Ptr to file being read */
   struct ECLREC ecr;         /* One CONNLIST record */
   long   nrec = 0;

/* Check that there is just one argument -- the CONNLIST file name */

   if (argc != 2)
      abexitm(558, "Usage: dumpclst <CONNLIST file name>");

/* Open CONNLIST file.  There is no header to check, so
*  program must assume this is a valid CONNLIST file.  */

   pclf = rfallo(argv[1], READ, BINARY, SEQUENTIAL, TOP,
      LOOKAHEAD, NO_REWIND, RELEASE_BUFF, 6000, IGNORE,
      IGNORE, ABORT);
   rfopen(pclf, NULL, READ, BINARY, SEQUENTIAL, TOP,
      LOOKAHEAD, NO_REWIND, RELEASE_BUFF, 6000, IGNORE,
      IGNORE, ABORT);

/* Now read the connlist file and print the information */

#ifdef DECODE
   cryout(RK_P2, "0 Source Cell   Target Cell     c(ij)  jct"
      "         x     y     r", RK_LN2|RK_SUBTTL, NULL);
#else
   cryout(RK_P2, "0 Source Cell   Target Cell     c(ij)  jct",
      RK_LN2|RK_SUBTTL, NULL);
#endif

   while (1) {
      ecr.xjcell = rfri4(pclf);
      if (pclf->lbsr == ATEOF) break;
      ecr.xicell = rfri4(pclf);
      ecr.ecij   = rfri2(pclf);
      ecr.jct    = rfri2(pclf);
#ifdef DECODE        /* JIFFY code to decode xjcell */
      {  div_t xjqr1,xjqr2;
         int xxj,yxj,rxj;
         xjqr1 = div(ecr.xjcell,NXIMG*NXIMG);
         rxj = xjqr1.quot;
         xjqr2 = div(xjqr1.rem, NXIMG);
         yxj = xjqr2.quot, xxj = xjqr2.rem;
         convrt("(P2,#4,IJ12,IJ14,B15IH10.5,IH5,5X,3*I6)",
            &ecr, &xxj, &yxj, &rxj, NULL);
         }
#else
      convrt("(P2,#4,IJ12,IJ14,B15IH10.5,IH5)", &ecr, NULL);
#endif
      nrec += 1;
      }

   convrt("(P1,'0Dump completed, ',J1IL12,'records read.',W)",
      &nrec, NULL);

   return 0;
   } /* End dumpclst() */
