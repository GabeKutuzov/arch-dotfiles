/* (c) Copyright 2010, The Rockefeller University *11116* */
/* $Id: bbdcputv.c 16 2017-01-13 20:36:40Z  $ */
/***********************************************************************
*               BBD (Client Side) mex-Function Package                 *
*                             bbdcputv.c                               *
*              Send value data from BBD to neural system               *
*                                                                      *
*  This is the matlab mex-function version of bbdcputv().              *
*                                                                      *
*  This routine may be called on a client BBD to send simulated value  *
*  data to the neural system simulator.                                *
*                                                                      *
*  MATLAB Synopsis:                                                    *
*     bbdcputv(BBDComm, values)                                        *
*                                                                      *
*  Argument:                                                           *
*     BBDComm  MATLAB array containing private data returned by a      *
*              previous call to bbdcinit.                              *
*     values   MATLAB single-precision floating point vector contain-  *
*              ing a concatenation of all the values registered by     *
*              previous calls to bbdcregv().  Values should be in the  *
*              range 0.0 to 1.0.                                       *
*                                                                      *
*  Prerequisites:                                                      *
*     First call bbdcinit() to start everything up, then call          *
*     routines in the bbdcreg[stve] family to register the senses,     *
*     cameras, values, and effectors needed in this run, then call     *
*     bbdcchk() to confirm that configurations in server and client    *
*     agree.  Then this routine can be called as needed.               *
*                                                                      *
*  Error Handling:                                                     *
*     Error checking is minimal because we are now in the main BBD     *
*     action loop.  Everything should have already been checked.       *
*     All error are terminal and result in a call to abexit() with a   *
*     suitable message.  There is nothing useful the caller can do.    *
************************************************************************
*  V1A, 10/22/10, GNR - New mex version                                *
*  ==>, 11/04/10, GNR - Last mod before committing to svn repository   *
***********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

#define I_AM_MEX

#include "mex.h"
#include "sysdef.h"
#include "bbd.h"
#include "rfdef.h"

void mexFunction(int nlhs, mxArray *plhs[],
      int nrhs, const mxArray *prhs[]) {

   struct BBDComData *BBDcd;  /* Common BBD data struct */
   BBDDev       *pd;          /* Current device */
   struct RFdef *pf;          /* File descriptor */
   float        *pv,*pve;     /* Ptrs to client's data array */

   /* Standard MATLAB check for proper number and type of
   *  arguments, then locate BBDComData */

   if (nlhs != 0 || nrhs != 2 || !mxIsClass(prhs[1], "single"))
      abexitm(BBDcErrMexCall, "Bad arguments to bbdcputv");
   BBDcd = (struct BBDComData *)mxGetData(prhs[0]);
   if (!BBDcd)
      abexitm(BBDcErrMexCall, "Bad BBDcd arg to bbdcputv");
   pv = (float *)mxGetData(prhs[1]);

   /* Check whether server already terminated */
   if (BBDcd->Mlogrc > 0)
      abexitm(BBDcErrServer, ssprintf(NULL, "Server (CNS) terminated "
         "with abexit code %5d", BBDcd->Mlogrc));

   pf = BBDcd->ndsend;

   /* Loop over all the devices on the argument list */
   for (pd=BBDcd->pfval; pd; pd=pd->pndev) {

#ifdef DEBUG
      mexPrintf("Writing value %s to CNS\n", pd->devnm);
#endif

      /* Send the control integer */
      rfwi2(pf, pd->bdid);

      /* Send the value data */
      pve = pv + pd->ldat;
      while (pv < pve) {
#ifdef DEBUG
         mexPrintf("  %8.3f", *pv);
#endif
         rfwr4(pf, *pv++);
         }

#ifdef DEBUG
      mexPrintf("\n");
#endif

      } /* End device loop */

   /* Flush the stream */
   rfflush(pf, ABORT);

   } /* End bbdcputv() */
