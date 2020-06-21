/* MP program for testing interrupts on PP */
/* This version modified by GNR so lp is malloc rather
*  than stack, giving us room to flush it before calling
*  the PP.  The flag value that will be returned by the PP
*  was stuck into lp[4] so that it will be included in the
*  same flush.  */

/* Define size of TC move to be made (number of ints) */
#define TCSIZE 50

/* Include standard library functions */

#include <stdlib.h>        /* MVP Standard Definitions */
#include <stdarg.h>
#include <stdio.h>

/* Include MVP files */
#include "mvp.h"           /* MVP hardware functions */
#include "mvp_hw.h"        /* MVP hardware parameters */
#include "task.h"          /* MP multitasking kernel */

/* Local (GIC) include files */
#include "bios.h"          /* Basic I/O functions */
#include "host.h"          /* HOST Communication functions */
#include "timemgr.h"       /* Timer functions */
#include "dspAPI.h"        /* DSP Command interface */
#include "xfer.h"          /* Transfer control interface */
#include "param.h"         /* Cache row size, etc. */

/* malloc replacement for safer cache management:
*  (1) Always request alignment on a 64 byte boundary
*  (2) Always request space in a multiple of 64 bytes
*/

void *mallocv(unsigned int n) {
   n = (n+63) & (~63);
   return memalign(MVP_CACHE_SUBBLOCK, n);
   }


/*--------------------------------------------------------------------*/
/*                                                                    */
/*                         MAIN PROCEDURE                             */
/*                                                                    */
/*--------------------------------------------------------------------*/

/* #pragma SHARED(test_pp) */
shared void test_pp( DSPARG * ); /* My DSP test function */

userMain(int argc, char *argv[]) {

   int *dumBuff;
   int *outBuff;
   int i, flag=1;
   long *lp;

   if ((dumBuff = (int *)mallocv(sizeof(int)*TCSIZE))==NULL)
       printf("no mem for dumbuff \n");
   if ((outBuff = (int *)mallocv(sizeof(int)*TCSIZE))==NULL)
       printf("no mem for outbuff \n");
   if ((lp = (long *)mallocv(sizeof(long)*5))==NULL)
       printf("no mem for lp \n");

/* fill some memory */

   for (i=0; i<TCSIZE; i++) dumBuff[i] = i;

   lp[0] = (long) (lp+4);
   lp[1] = (long) dumBuff;
   lp[2] = (long) outBuff;
   lp[3] = TCSIZE;
   lp[4] = flag;      /*** THIS IS NOT A PARAMETER TO PP, ITS ADDRESS IS ***/

   dspAPIInit();
   i = dspInit(0x1);
   if (i != DSP_OK ) {
#ifndef SIM
      printf(" dspInit() failed\n");
#endif
      exit(8);
      }

#ifndef SIM
   printf("Starting PP function\n");
   printf(" flag is %d \n", flag);
#endif

   flush(dumBuff, sizeof(int)*TCSIZE);
   flush(lp, sizeof(long)*5);
   dspExec(0x1, test_pp, 4L, lp, DSP_WAIT, 0);
   flag = lp[4];   /* Should have been changed by PP */
#ifndef SIM
   printf("return from PP function\n");
#endif

/* did the interrupt work? */

#ifndef SIM
    printf(" flag is %d \n", flag);
#endif

   } /* End int program */
 


