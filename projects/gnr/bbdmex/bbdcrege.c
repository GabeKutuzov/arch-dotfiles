/* (c) Copyright 2010-2015, The Rockefeller University *11116* */
/* $Id: bbdcrege.c 16 2017-01-13 20:36:40Z  $ */
/***********************************************************************
*               BBD (Client Side) mex-Function Package                 *
*                             bbdcrege.c                               *
*                       Register a BBD effector                        *
*                                                                      *
*  This is the matlab mex-function version of bbdcrege().              *
*                                                                      *
*  Register an effector array ("e" is for "effector" in bbdcrege)      *
*  for receipt of motor neuron command data from the nervous system    *
*  of a BBD.  N.B.  After all senses, effectors, and values have       *
*  been registered, bbdcchk() must be called to check that the same    *
*  registrations have been made in the server and to establish the     *
*  communications link between client and server.                      *
*                                                                      *
*  MATLAB Synopsis:                                                    *
*     bbdcrege(BBDComm, effnm, mnad)                                   *
*                                                                      *
*  Arguments:                                                          *
*     BBDComm  MATLAB array containing private data returned by a      *
*              previous call to bbdcinit.                              *
*     effnm    Character string giving a name to this effector.        *
*              This name must not exceed 15 characters and must        *
*              match the corresponding name assigned in the server.    *
*     mnad     MATLAB double or int32 integer giving the motor neuron  *
*              array size (number of elements).  Zero may be coded to  *
*              indicate that the size received later from CNS should   *
*              be accepted without validation.  Otherwise, depending   *
*              on the EFFECTOR card in CNS, this may be 1 if only ex-  *
*              citatory or only inhibitory sums are specified, 2 if    *
*              both, or a larger number if multiple change arbors or   *
*              single-cell activities are returned.                    *
*                                                                      *
*  Prerequisites:                                                      *
*  bbdcinit() must be called before this routine is called.            *
*                                                                      *
*  Error Handling:                                                     *
*  All errors are terminal and result in calls to abexitm() with       *
*  a suitable message.  There is nothing useful the caller can do      *
*  to recover.                                                         *
************************************************************************
*  V1A, 10/22/10, GNR - New mex version                                *
*  ==>, 11/04/10, GNR - Last mod before committing to svn repository   *
*  Rev, 12/20/15, GNR - Allow double mnad and mnad == 0                                *
***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define I_AM_MEX

#include "mex.h"
#include "sysdef.h"
#include "bbd.h"
#include "rksubs.h"

void mexFunction(int nlhs, mxArray *plhs[],
      int nrhs, const mxArray *prhs[]) {

   struct BBDComData *BBDcd;  /* Common BBD data struct */
   BBDDev *peff;              /* Ptr to data for this effector */
   si32   mnad;               /* Array dimension */

   /* Standard MATLAB check for proper number and type of
   *  arguments, then locate arguments */

   if (nlhs != 0 || nrhs != 3 || !mxIsChar(prhs[1]))
      abexitm(BBDcErrMexCall, "bbdcrege requires 3 RHS, 0 LHS args");
   BBDcd = (struct BBDComData *)mxGetData(prhs[0]);
   if (!BBDcd)
      abexitm(BBDcErrMexCall, "Bad BBDcd arg to bbdcrege");

/* A little basic error checking */

   if (mxGetNumberOfElements(prhs[1]) > 15)
      abexitm(BBDcErrMexCall, "Effector name > 15 chars");
   if (mxGetNumberOfElements(prhs[2]) != 1)
      abexitm(BBDcErrMexCall, "mnad arg to bbdcrege must be a scalar");

/* Get and check mnad argument */
   
   if (mxIsDouble(prhs[2])) {
      double dmnad = *(double *)mxGetData(prhs[2]);
      if (dmnad > (double)SI32_MAX)
         abexitm(BBDcErrParams, "Motor neuron array is too large");
      mnad = (si32)dmnad;
      }
   else if (mxIsInt32(prhs[2]))
      mnad = *(si32 *)mxGetData(prhs[2]);
   else
      abexitm(BBDcErrMexCall, "mnad arg to bbdcrege is wrong type");
   if (mnad < 0 || mnad > UI16_MAX) abexitm(BBDcErrParams,
      "Invalid motor neuron array size");

/* Allocate a new BBDDev struct to hold the data, clear it,
*  link it onto the end of the chain, mark it EFFECTOR type.  */

   peff = (BBDDev *)mxCalloc(1, sizeof(BBDDev));
   mexMakeMemoryPersistent(peff);
   *BBDcd->ppfeff = peff;
   BBDcd->ppfeff = &peff->pndev;
   peff->bdflgs = BBDTEff;

/* Store the data */

   peff->ldat = mnad;
   mxGetString(prhs[1], peff->devnm, MaxDevNameLen+1);
   BBDcd->neffs += 1;

   } /* End bbdcrege() */
