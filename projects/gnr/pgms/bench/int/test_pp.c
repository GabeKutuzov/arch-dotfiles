/***********************************************************************
*                         TEST PP INTERRUPTS                           *
***********************************************************************/
/* Include standard library functions */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mvp_hw.h"           /* MVP hardware defs */
#include "timemgr.h"	      /* Timer functions */
#include "dspAPI.h"	      /* DSP Command interface */
#include "param.h"	      /* System dependent, and compatibility */
#include "ptreq.h"            /* Programmable memory transfers */

   int intF;

/*---------------------------------------------------------------------*
*                       PP server main program                         *
*---------------------------------------------------------------------*/

/* #pragma SHARED (test_pp) */
shared void test_pp(DSPARG *argBuf) {

   PTREQ *PTR;
   int *flag, *dumBuff, *outB;
   int tcsize, outBuff[50];
   interrupt void int_handler();
   void (**ISR) ();
   
   flag = (int *)argBuf->argv[0];
   dumBuff = (int *)argBuf->argv[1];
   outB = (int *)argBuf->argv[2];
   tcsize = (int)argBuf->argv[3];
   intF = *flag;

   ISR =  (void *) 0x010001cc; 
   *ISR =  int_handler; 
   
   asm(" inten = 1\\\\19");         /* Enable tc interrupt only, also
                                 *  set bit 0 = 0 to clear intflg */
   asm(" intflg = intflg");       /* Clear interrupt flags */

/* enable interrupts */
                             
   asm(" nop"); 
   asm(" eint");      /* globally enable interrupts */

/* move data into onchip memory */

   PTR = (PTREQ *) 0x00000700;
   memset((char *)PTR, 0, PTREQ_SIZE);

/* make PTreq */

   PTR->options = PTOPT_S_LINKEND;
   PTR->srcAddr = dumBuff;
   PTR->dstAddr = outBuff;
   PTR->srcACnt = PTR->dstACnt = sizeof(int);
   PTR->srcBCnt = PTR->dstBCnt = tcsize - 1;
   PTR->w32.srcBPitch = PTR->dstBPitch = sizeof(int);

   DPtReqIssue(PTR);

/* dummy loop, waiting to be interrupted */

   while (intF<2);

   *flag = intF;

   } /* end of int_pp */

#if 0
/* Interrupt handler */
/* The exit code has been placed here in assembler to overcome a
*  compiler bug that causes sr to be loaded with what should be
*  contents of d1 and so forth, see email to Lisa C. from TI
*  hotline, 08/11/95.  This does not correct the entry code bug
*  that loads a value into d0 without saving it.  This cannot
*  be corrected without writing the entire routine in Assembler,
*  but is not critical for this test because the above mainline
*  code is not using d0 at this point.     -GNR    */

interrupt void int_handler() {

   asm(" inten = 0");       /* clear bit 0, w=0. Must be cleared to
                                      clear flag in intflg */
   asm(" intflg = intflg");        /* clear PTEND flag */
   intF = 2;

   asm(" d1 = *sp++");
   asm(" reti1");
   asm(" reti2");
   asm(" reti3");
   asm(" reti4");
   }
#endif

/* This routine is called from the assembler int_handler.
*  We are testing our ability to call C from assembler
*  inside an interrupt handler.  */

void int_inner(void) {
   intF = 2;
   }


