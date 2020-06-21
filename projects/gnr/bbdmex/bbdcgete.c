/* (c) Copyright 2010-2015, The Rockefeller University *11116* */
/* $Id: bbdcgete.c 16 2017-01-13 20:36:40Z  $ */
/***********************************************************************
*               BBD (Client Side) mex-Function Package                 *
*                             bbdcgete.c                               *
*           Receive effector data from neural system to BBD            *
*                                                                      *
*  This is the matlab mex-function version of bbdcgete().              *
*                                                                      *
*  This routine may be called on a client BBD to receive simulated     *
*  motor output from the neural system simulator.                      *
*                                                                      *
*  MATLAB Synopsis:                                                    *
*     [effout retcode] = bbdcgete(BBDComm)                             *
*                                                                      *
*  Arguments:                                                          *
*     BBDComm  MATLAB array containing private data returned by a      *
*              previous call to bbdcinit.                              *
*                                                                      *
*  Return values:                                                      *
*     effout   If there is exactly one effector, a MATLAB array of     *
*              mnad single-precision values received from CNS.  If     *
*              there is more than one effector, then a MATLAB cell     *
*              array with one element for each registered effector.    *
*              Each element is an array of results as for a single     *
*              effector.  Array dimensions are set from the row and    *
*              column sizes received from CNS.  Array components may   *
*              be the activities of single cells or sums of above-     *
*              threshold activity in the cell types specified for      *
*              effectors on the CNS server.  N.B.  Unlike the non-     *
*              mex version, this routine rescales CNS output from the  *
*              internal S14 or S7 scale to an 0-1 or mV scale.         *
*     retcode  An int32 1x1 MATLAB array that will contain 0 if normal *
*              data were received or 1 if the server signalled that    *
*              the application should terminate.                       *
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
*  V1B, 11/26/10, GNR - Output cell array--one matrix per effector     *
*  Rev, 12/15/15, GNR - Remove scaling by CNSBigSi to d3go             *
*  Rev, 12/20/15, GNR - Set effout size to cols,rows sent from server  *
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
   float        *pmn,*pmne;   /* Ptrs to one data array */
   mxArray      *pec;         /* Ptr to one cell element */
   si32         *prc;         /* Ptr to MATLAB retcode array */
   mwSize       eodims[2];    /* Size of output array */
   int          jd;           /* Device index */
   ui16         ckint;        /* Control integer */

   static const mwSize rcdims[2] = { 1, 1 };

   /* Standard MATLAB check for proper number and type of
   *  arguments, then locate BBDComData */
   if (nlhs != 2 || nrhs != 1)
      abexitm(BBDcErrMexCall, "Bad arguments to bbdcgete");
   BBDcd = (struct BBDComData *)mxGetData(prhs[0]);
   if (!BBDcd)
      abexitm(BBDcErrMexCall, "Bad BBDcd arg to bbdcgete");

   /* Check whether server already terminated */
   if (BBDcd->Mlogrc > 0)
      abexitm(BBDcErrServer, ssprintf(NULL, "Server (CNS) terminated "
         "with abexit code %5d", BBDcd->Mlogrc));

   /* If multiple devices, create cell array to hold output vectors.
   *  Otherwise, create vector for the only device (done before loop
   *  below so result exists if early exit due to BBDLC_TERM) */
   pd = BBDcd->pfeff;
   if (BBDcd->neffs > 1) {
      eodims[0] = BBDcd->neffs, eodims[1] = 1;
      plhs[0] = mxCreateCellArray(2, eodims);
      }
   else {
      eodims[0] = pd->UD.Eff.effx, eodims[1] = pd->UD.Eff.effy;
      plhs[0] = pec =
         mxCreateNumericArray(2, eodims, mxSINGLE_CLASS, mxREAL);
      }

   /* Output for return code */
   plhs[1] = mxCreateNumericArray(2, rcdims, mxINT32_CLASS, mxREAL);
   prc = (si32 *)mxGetData(plhs[1]);
   *prc = 0;

   /* Loop over all the devices on the argument list */
   pf = BBDcd->ndrecv;
   for (jd=0; pd; ++jd,pd=pd->pndev) {

      /* Receive and check the control integer */
      ckint = rfri2(pf);
#ifdef DBG_BDID
      mexPrintf("bbdcgete read bdid = %d (saved %d) for effector %d\n",
         (int)ckint, (int)pd->bdid, jd);
#endif
      if (ckint == BBDLC_TERM) {
         *prc = rfri4(pf);
         break;
         }
      if (ckint != pd->bdid) abexitm(BBDcErrBadData, "Bad control "
         "integer received from neural server");

      /* If multiple devices, create matrix to hold results for
      *  this device and store address in cell array.  */
      if (BBDcd->neffs > 1) {
         eodims[0] = pd->UD.Eff.effx, eodims[1] = pd->UD.Eff.effy;
         pec = mxCreateNumericArray(2, eodims, mxSINGLE_CLASS, mxREAL);
         mxSetCell(plhs[0], jd, pec);
         }

      /* Receive the motor neuron data */
      pmn = (float *)mxGetData(pec);
      pmne = pmn + pd->ldat;
      while (pmn < pmne)
         *pmn++ = rfrr4(pf);

      } /* End device loop */

   return;

   } /* End bbdcgete() */

