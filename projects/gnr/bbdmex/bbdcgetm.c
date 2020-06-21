/* (c) Copyright 2010, The Rockefeller University *11116* */
/* $Id: bbdcgetm.c 16 2017-01-13 20:36:40Z  $ */
/***********************************************************************
*               BBD (Client Side) mex-Function Package                 *
*                             bbdcgetm.c                               *
*       Receive an arbitrary message from neural system to BBD         *
*                                                                      *
*  This is the matlab mex-function version of bbdcgetm().              *
*                                                                      *
*  This routine may be called on a client BBD to receive a message     *
*  sent with the bbdsputm() call.  The client must call bbdcgetm()     *
*  in the same sequence relative to any sensor or effector data        *
*  transmission as the corresponding bbdsputm() call.                  *
*                                                                      *
*  Unfortunately, we have to allocate a temporary array to receive     *
*  the message as a C char string in order to call mxCreateString      *
*  to convert it into the wide chars that MATLAB uses.                 *
*                                                                      *
*  MATLAB Synopsis:                                                    *
*     text = bbdcgetm(BBDComm)                                         *
*                                                                      *
*  Arguments:                                                          *
*     BBDComm  MATLAB array containing private data returned by a      *
*              previous call to bbdcinit.                              *
*                                                                      *
*  Return value:                                                       *
*     text     Text returned from the CNS server.                      *
*                                                                      *
*  Prerequisites:                                                      *
*     bbdcinit() and bbdcchk() must have been called and completed     *
*     successfully.                                                    *
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
#include "rksubs.h"
#include "rfdef.h"

void mexFunction(int nlhs, mxArray *plhs[],
      int nrhs, const mxArray *prhs[]) {

   struct BBDComData *BBDcd;  /* Common BBD data struct */
   struct RFdef *pf;          /* File descriptor */
   char         *ptxt;        /* Ptr to actual text */
   si32         llmsg;        /* Message length */
   mwSize       txtsz[2];     /* Size control for output */
   ui16         ckint;        /* Control integer */

   /* Standard MATLAB check for proper number and type of
   *  arguments, then locate BBDComData */

   if (nlhs != 1 || nrhs != 1)
      abexitm(BBDcErrMexCall, "Bad arguments to bbdcgetm");
   BBDcd = (struct BBDComData *)mxGetData(prhs[0]);
   if (!BBDcd)
      abexitm(BBDcErrMexCall, "Bad BBDcd arg to bbdcgetm");

   /* Check whether server already terminated */
   if (BBDcd->Mlogrc > 0)
      abexitm(BBDcErrServer, ssprintf(NULL, "Server (CNS) terminated "
         "with abexit code %5d", BBDcd->Mlogrc));

   pf = BBDcd->ndrecv;

   /* Receive and check the control integer */
   ckint = rfri2(pf);
   if (ckint != BBDLC_MSG) abexitm(BBDcErrBadData, "Bad control "
      "integer received from neural server");

   /* Receive the message length, allocate temp array to
   *  receive it, and receive it (it may have been fragmented
   *  in transit)  */
   llmsg = rfri4(pf);
   if (llmsg <= 0) abexitm(BBDcErrBadData, "Zero-length message "
      "received from neural server");
   ptxt = mxMalloc((mwSize)llmsg+1);
   rfread(pf, ptxt, (size_t)llmsg, ABORT);
   ptxt[llmsg] = '\0';
   plhs[0] = mxCreateString(ptxt);
   mxFree(ptxt);

   return;

   } /* End bbdcgetm() */
