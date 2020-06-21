/***********************************************************************
*               PP worker function for d3benchmark test                *
*             Version for TI MVP chip, 08/09/95, LC, GNR               *
*                                                                      *
*  IN THIS VERSION, THE MP BREAKS UP THE WORK INTO WORKUNITS, LOADS    *
*  THE DATA FOR EACH WORKUNIT ONTO AN AVAILABLE PP, AND DISPATCHES     *
*  THE PP TO CALCULATE THE BATCH OF CONNECTIONS USING THIS FUNCTION    *
*                                                                      *
*  Handling of DAS statistics:  Clear and accumulate separately for    *
*  each batch.  Code in MP that handles Cij write-back also sums DAS   *
*  stats into off-chip CONNBLKs.                                       *
***********************************************************************/

/* Include standard library functions */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bserv.h"            /* Application-specific definitions */

/* Note:  During debugging, it was found that globals declared here
*  with initializers were seen during execution as having value 0.  */

/*=====================================================================*
*                       PP server main program                         *
*=====================================================================*/

#pragma SHARED (bserv_pp)
void bserv_pp(WORKUNIT *pWU) {
   
   struct CONNDATA *ic = pWU->Myic;    /* Ptr to connection data */
   struct WORKOUT *pwo = pWU->Mywo;    /* Ptr to output data */

   long wksum = 0;
   int ncb = pWU->nbatch;              /* Num conns in batch */
   int jc;                             /* Connection counter */
   short simmti = pWU->si - pWU->mti;  /* s(i) - mti (S8) */

/* Clear DAS stats for this batch */

   memset(pwo->das, 0, NDAS*sizeof(long));
   
/* Calculate contributions of all connections in current batch.
*  Arrange to use fast 16-bit multiplication where possible.  */

#if 1

   for (jc=0; jc<ncb; jc++,ic++) {
      short wksj  = ic->sj;            /* (S8) */
      short sjmet = wksj - pWU->et;    /* (S8) */
      short wkcij =                    /* (S7) */
         (ic->cij >= 128) ? ic->cij - 256 : ic->cij;
      if (sjmet > 0) wksum += sjmet*wkcij;

   /* Perform amplification if delta != zero.  Only the "4-way" rule
      is implemented in this "benchmark".  To make life simpler,
      the 'phi' factor is ignored (i.e. taken to be 1.0), and scaling
      is fudged to make intermediate results fit in 32-bit registers.
      "DAS" statistics are always collected.                             */

      if (pWU->delta) {
         short sjmmtj = wksj - pWU->mtj;  /* (S8) */
         int ampcase = 0;
         if (simmti < 0) ampcase += 2;
         if (sjmmtj < 0) ampcase += 1;
         pwo->das[ampcase]++;
         sjmmtj *= simmti;
         wkcij += (long)(pWU->delta*sjmmtj)>>19;
         if (wkcij > 127) wkcij = 127;
         if (wkcij <-127) wkcij =-127;
         ic->cij = (byte)wkcij;
         }

      } /* End loop over individual connections */

#else

/* Alternative version of inner loop, doing just what is
*  needed to get das.  */

   for (jc=0; jc<ncb; jc++,ic++) {
      short sjmmtj = ic->sj - pWU->mtj;   /* (S8) */
      int ampcase = 0;
      if (simmti < 0) ampcase += 2;
      if (sjmmtj < 0) ampcase += 1;
      pwo->das[ampcase]++;
      } /* End loop over individual connections */

#endif

/* Store working sum and return to dispatcher */

   pwo->ax = wksum*pWU->scale;
   } /* End bserv_pp main program */


