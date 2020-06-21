/* (c) Copyright 2013, The Rockefeller University *11116* */
/* $Id: bbdcquit.c 16 2017-01-13 20:36:40Z  $ */
/***********************************************************************
*               BBD (Client Side) mex-Function Package                 *
*                             bbdcquit.c                               *
*          Terminate communication with a network simulation           *
*                                                                      *
*  This routine can be called by the client MATLAB program via         *
*  bbdcclse() or via the registered mexAtExit() routine bbdcqex()      *
*  to run when matlab exits.  It shuts down CNS if not already dead,   *
*  closes files, releases memory, and returns.  Note:  This is a C     *
*  program callable from a mex function, not itself a mex function.    *
*                                                                      *
*  Argument:                                                           *
*     BBDcd    Pointer to a BBDComData array containing information    *
*              used for communication among the various routines in    *
*              the bbdc package and created by a previous call to      *
*              bbdcinit().                                             *
*                                                                      *
************************************************************************
*  V1A, 04/18/13, GNR - Separate source file removed from bbdcinit.c   *
*                       so can be called explicitly or at exit time.   *
*  ==>, 04/18/13, GNR - Last mod before committing to svn repository   *
*  Rev, 04/26/13, GNR - Bug fix, avoid multiply freeing pdev space     *
*  Rev, 05/06/13, GNR - Wait for LogProc instead of killing it         *
***********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define I_AM_MEX

#include "mex.h"
#include "sysdef.h"
#include "bbd.h"
#include "rfdef.h"
#include "rksubs.h"

/*=====================================================================*
*                              bbdcquit                                *
*=====================================================================*/

/* Subroutine used to free a list of BBDDevs */

static void bbdcfmlist(BBDDev **ppdev) {
   BBDDev *pdev = *ppdev;
   while (pdev) {
      BBDDev *pnxd = pdev->pndev;
      mxFree(pdev);
      pdev = pnxd;
      }
   *ppdev = NULL;
   return;
   } /* End bbdcfmlist() */

void bbdcquit(struct BBDComData *BBDcd) {

/* If Mlogrc is < 0, it means CNS has not yet terminated and
*  this program must tell it to quit.  There are three subcases:
*  (1) CNS already entered the CYCLE loop.  It can be shut down
*      via a special code sent in place of the usual sensor data.
*  (2) InpProc finished sending the input file, but bbdcchk was
*      not entered yet or is waiting on the rfopen(ndrecv).  CNS
*      is not responding but didn't send a terminal error msg.
*      This is an unusual case, maybe CNS is in a debug wait loop,
*      and there is no obvious way to kill it from here.  Just
*      give the user a message to kill CNS manually.
*  (3) CNS is still processing the input file.  It can be shut
*      down by just sending it an END card.  */

   if (BBDcd->Mlogrc < 0) {

      if (BBDcd->Minprc >= 0) {
         if (!BBDcd->ndsend) {
            /* Case (2):  No escape now.  I am not calling
            *  mexErrMsgTxt here because we are already in
            *  the closeout routine.  */
            fputs("CNS is not responding.  Kill it manually!\n",
               stderr);
            fflush(stderr); }
         else {
            /* Case (1):  Send a control integer larger than
            *  BBDLC_MASK, viz. BBDLC_TERM  */
            rfwi2(BBDcd->ndsend, BBDLC_TERM);
            rfflush(BBDcd->ndsend, NOMSG_ABORT);
            }
         }
      }

   /* Case (3) above ends up here anyway */
   if (BBDcd->InpProc) {
      kill(BBDcd->InpProc, SIGTERM);
      rksleep(1, 0);
      BBDcd->InpProc = 0;
      }

   if (BBDcd->LogProc) {
      int LogStatus = 0;
      waitpid(BBDcd->LogProc, &LogStatus, 0); 
      if (LogStatus < 0) {
         fputs("Error in wait for LogProc\n", stderr);
         fflush(stderr); }
      BBDcd->LogProc = 0;
      }

/* Now CNS should have terminated.  Close remaining open files  */

   if (BBDcd->ndsend)
      rfclose(BBDcd->ndsend, NO_REWIND, RELEASE_BUFF, NOMSG_ABORT);
   if (BBDcd->ndrecv)
      rfclose(BBDcd->ndrecv, NO_REWIND, RELEASE_BUFF, NOMSG_ABORT);

/* Get rid of semaphores */

   sem_close(BBDcd->pMinpSem);
   sem_close(BBDcd->pMlogSem);
   sem_unlink(MinpSemNm);
   sem_unlink(MlogSemNm);

/* Now free memory allocated by the mxMalloc mechanism */

   bbdcfmlist(&BBDcd->pfeff);
   bbdcfmlist(&BBDcd->pfsns);
   bbdcfmlist(&BBDcd->pfttv);
   bbdcfmlist(&BBDcd->pfetv);
   bbdcfmlist(&BBDcd->pfstv);
   bbdcfmlist(&BBDcd->pfval);
   if (BBDcd->chnm) {
      mxFree(BBDcd->chnm); BBDcd->chnm = NULL; }
   if (BBDcd->exsi) {
      mxFree(BBDcd->exsi); BBDcd->exsi = NULL; }
   if (BBDcd->ppvdefs) {
      mxFree(BBDcd->ppvdefs); BBDcd->ppvdefs = NULL; }

   /* return, not exit--exit() has already been called somewhere */
   return;

   } /* End bbdcquit() */

