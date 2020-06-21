/* (c) Copyright 2010, The Rockefeller University *11116* */
/* $Id: bbdcregv.c 16 2017-01-13 20:36:40Z  $ */
/***********************************************************************
*               BBD (Client Side) mex-Function Package                 *
*                             bbdcregv.c                               *
*                        Register a BBD Value                          *
*                                                                      *
*  This is the matlab mex-function version of bbdcregv().              *
*                                                                      *
*  Register a value ("v" is for "value" in bbdcregv) for               *
*  transmission to the nervous system of a BBD.  N.B.  After all       *
*  senses, effectors, and values have been registered, bbdcchk()       *
*  must be called to check that the same registrations have been       *
*  made in CNS and to establish the communications link between        *
*  client and server.                                                  *
*                                                                      *
*  MATLAB Synopsis:                                                    *
*     bbdcregv(BBDComm, valnm)                                         *
*                                                                      *
*  Arguments:                                                          *
*     BBDComm  MATLAB array containing private data returned by a      *
*              previous call to bbdcinit.                              *
*     valnm    Character string giving a name to this value.           *
*              This name must not exceed 15 characters and must        *
*              match the corresponding name on the VALUE card in       *
*              the CNS control file.                                   *
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
   BBDDev *pvl;               /* Ptr to data for this value */

   /* Standard MATLAB check for proper number and type of
   *  arguments, then locate BBDComData */

   if (nlhs != 0 || nrhs != 2 || !mxIsChar(prhs[1]))
      abexitm(BBDcErrMexCall, "Bad arguments to bbdcregv");
   BBDcd = (struct BBDComData *)mxGetData(prhs[0]);
   if (!BBDcd)
      abexitm(BBDcErrMexCall, "Bad BBDcd arg to bbdcregv");

/* Allocate a new BBDDev struct to hold the data, clear it,
*  link it into the end of the chain, and mark it VALUE type.  */

   pvl = (BBDDev *)mxCalloc(1, sizeof(BBDDev));
   mexMakeMemoryPersistent(pvl);
   *BBDcd->ppfval = pvl;
   BBDcd->ppfval = &pvl->pndev;
   pvl->bdflgs = BBDTVal;

/* Store the data */

   pvl->ldat = 1;
   mxGetString(prhs[1], pvl->devnm, MaxDevNameLen+1);

   } /* End bbdcregv() */
