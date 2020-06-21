/* (c) Copyright 2010, The Rockefeller University *11116* */
/* $Id: bbdcputt.c 16 2017-01-13 20:36:40Z  $ */
/***********************************************************************
*                BBD (Client Side) mex-FunctionPackage                 *
*                             bbdcputt.c                               *
*              Send image data from BBD to neural system               *
*                                                                      *
*  This is the matlab mex-function version of bbdcputt().              *
*                                                                      *
*  This routine may be called on a client BBD to send simulated image  *
*  (television or still camera) data to the neural system simulator.   *
*                                                                      *
*  MATLAB Synopsis:                                                    *
*     bbdcputt(BBDComm, type, image_data)                              *
*                                                                      *
*  Arguments:                                                          *
*     BBDComm  MATLAB array containing private data returned by a      *
*              previous call to bbdcinit.                              *
*     type     MATLAB int32 integer indicating which images are        *
*              being sent by this call:  1 indicates BBDCFr_TRIAL      *
*              images (one per trial), 2 indicates BBDCFr_EVENT        *
*              images (one per designated event), and 3 indicates      *
*              BBDCFr_SERIES images (sent at start of new series       *
*              only).                                                  *
*     image_data  MATLAB cell array where each element contains the    *
*              image data specified in one call to bbdcregt() of       *
*              the size specified there and the type indicated by      *
*              the 'type' argument.  If the image source is of         *
*              the type that also returns stimulus identification      *
*              (i.e. was registered with bbdcrtid), then it occupies   *
*              two elements of the image-data array, first one for     *
*              the actual image data, then one with two int16 values   *
*              for the stimulus id and group, in that order.           *
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
   BBDDev       *pdev,*pd;    /* Current device */
   struct RFdef *pf;          /* File descriptor */
   si32         *ptyp;        /* Ptr to type arg */
   mwIndex      itv;          /* Index into image cell array */

   /* Standard MATLAB check for proper number and type of
   *  arguments, then locate BBDComData */

   if (nlhs != 0 || nrhs != 3 || !mxIsInt32(prhs[1]) ||
         !mxIsCell(prhs[2]))
      abexitm(BBDcErrMexCall, "Bad arguments to bbdcputt");
   BBDcd = (struct BBDComData *)mxGetData(prhs[0]);
   if (!BBDcd)
      abexitm(BBDcErrMexCall, "Bad BBDcd arg to bbdcputt");

   /* Check whether server already terminated */
   if (BBDcd->Mlogrc > 0)
      abexitm(BBDcErrServer, ssprintf(NULL, "Server (CNS) terminated "
         "with abexit code %5d", BBDcd->Mlogrc));

   pf = BBDcd->ndsend;

   /* Determine which image frequency we are handling here */
   ptyp = (si32 *)mxGetData(prhs[1]);
   switch (*ptyp) {
   case BBDCFr_TRIAL:   pdev = BBDcd->pfttv; break;
   case BBDCFr_EVENT:   pdev = BBDcd->pfetv; break;
   case BBDCFr_SERIES:  pdev = BBDcd->pfstv; break;
   default:
      abexitm(BBDcErrMexCall, "Bad image frequency to bbdcputt");
      } /* End ptyp switch */

   /* Loop over all the devices on the argument list */
   itv = 0;
   for (pd=pdev; pd; pd=pd->pndev,++itv) {

      /* Send the control integer */
      rfwi2(pf, pd->bdid);

      /* Send the image data if device is used */
      if (pd->bdflgs & BBDUsed) {
         mxArray *ptv = mxGetCell(prhs[2], itv);
         byte *ptvd = (byte *)mxGetData(ptv);
         rfwrite(pf, ptvd, pd->ldat, ABORT);
         if (pd->bdflgs & BBDMdlt) {
            ui16 *ptid;
            ptv = mxGetCell(prhs[2], ++itv);
            if (!mxIsClass(ptv, "uint16"))
               abexitm(BBDcErrMexCall, "Image id data not uint16");
            ptid = (ui16 *)mxGetData(ptv);
            rfwi2(pf, ptid[0]);
            rfwi2(pf, ptid[1]);
            }
         } /* End if BBDUsed */

      } /* End device loop */

   /* Flush the stream */
   rfflush(pf, ABORT);

   } /* End bbdcputt() */
