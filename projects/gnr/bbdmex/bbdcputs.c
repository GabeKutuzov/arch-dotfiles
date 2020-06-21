/* (c) Copyright 2010, The Rockefeller University *11116* */
/* $Id: bbdcputs.c 16 2017-01-13 20:36:40Z  $ */
/***********************************************************************
*               BBD (Client Side) mex-Function Package                 *
*                             bbdcputs.c                               *
*              Send sense data from BBD to neural system               *
*                                                                      *
*  This is the matlab mex-function version of bbdcputs().              *
*                                                                      *
*  This routine may be called by a MATLAB client to send simulated     *
*  sensory neuron data to the neural system simulator.                 *
*                                                                      *
*  MATLAB Synopsis:                                                    *
*     bbdcputs(BBDComm, sense_data)                                    *
*                                                                      *
*  Arguments:                                                          *
*     BBDComm  MATLAB array containing private data returned by a      *
*              previous call to bbdcinit.                              *
*     sense_data  MATLAB cell array, where each element contains the   *
*              sense data specified in one call to bbdcregs() of the   *
*              size and type specified there.  If the sense is of      *
*              the type that also returns stimulus identification      *
*              (i.e. was registered with bbdcrsid), then it occupies   *
*              two elements of the sense-data array, first one for     *
*              the actual sense data, then one with two int16 values   *
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
*     All errors are terminal and result in a call to abexit() with    *
*     a suitable message.  There is nothing useful the caller can do.  *
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
   mwIndex      isns;         /* Index into sense_data cell array */

   /* Standard MATLAB check for proper number and type of
   *  arguments, then locate BBDComData */

   if (nlhs != 0 || nrhs != 2 || !mxIsCell(prhs[1]))
      abexitm(BBDcErrMexCall, "Bad arguments to bbdcputs");
   BBDcd = (struct BBDComData *)mxGetData(prhs[0]);
   if (!BBDcd)
      abexitm(BBDcErrMexCall, "Bad BBDcd arg to bbdcputs");

   /* Check whether server already terminated */
   if (BBDcd->Mlogrc > 0)
      abexitm(BBDcErrServer, ssprintf(NULL, "Server (CNS) terminated "
         "with abexit code %5d", BBDcd->Mlogrc));

   pf = BBDcd->ndsend;

   /* Loop over all the devices on the argument list */
   isns = 0;
   for (pd=BBDcd->pfsns; pd; pd=pd->pndev,++isns) {

#ifdef DEBUG
      mexPrintf("Writing sense %s to CNS, bdid = %d, ldat = %d\n",
         pd->devnm, (int)pd->bdid, pd->ldat);
#endif

      /* Send the control integer */
      rfwi2(pf, pd->bdid);

      /* Send the sense data if device is used */
      if (pd->bdflgs & BBDUsed) {
         mxArray *psns = mxGetCell(prhs[1], isns);
         if (pd->bdflgs & BBDSns_R4) {
            /* Handle 4-byte real (float) sense data */
            float *psd,*psde;
#ifdef DEBUG
            int iout = 0;
#endif
            if (!mxIsClass(psns, "single"))
               abexitm(BBDcErrMexCall, "Sense data not single float");
            psd = (float *)mxGetData(psns);
            psde = psd + pd->ldat;
            while (psd < psde) {
#ifdef DEBUG
               mexPrintf("  %8.3f", *psd);
               if (++iout % 10 == 0) mexPrintf("\n");
#endif
               rfwr4(pf, *psd++);
               }
            }
         else {
            /* Handle single-byte sense data */
            byte *psd;
            if (!mxIsClass(psns, "uint8"))
               abexitm(BBDcErrMexCall, "Sense data not byte");
            psd = (byte *)mxGetData(psns);
            rfwrite(pf, psd, pd->ldat, ABORT);
            }
         if (pd->bdflgs & BBDMdlt) {
            si16 *psid;
            psns = mxGetCell(prhs[1], ++isns);
            if (!mxIsClass(psns, "int16"))
               abexitm(BBDcErrMexCall, "Sense id data not int16");
            psid = (si16 *)mxGetData(psns);
            rfwi2(pf, psid[0]);
            rfwi2(pf, psid[1]);
            }
         } /* End if BBDUsed */
#ifdef DEBUG
      mexPrintf("\n");
#endif

      } /* End device loop */

   /* Flush the stream */
   rfflush(pf, ABORT);

   } /* End bbdcputs() */
