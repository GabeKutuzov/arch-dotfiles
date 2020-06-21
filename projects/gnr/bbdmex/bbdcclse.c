/* (c) Copyright 2013, The Rockefeller University *11116* */
/* $Id: bbdcclse.c 16 2017-01-13 20:36:40Z  $ */
/***********************************************************************
*               BBD (Client Side) mex-Function Package                 *
*                             bbdcclse.c                               *
*                      Close out BBD mex client                        *
*                                                                      *
*  This is a matlab mex-function to call bbdcquit().                   *
*                                                                      *
*  This function will assure that all socket connections to the CNS    *
*  server are closed so the interface can be reused without MATLAB     *
*  "clear mex" or exit.                                                *
*                                                                      *
*  MATLAB Synopsis:                                                    *
*     bbdcclse(BBDComm)                                                *
*                                                                      *
*  Arguments:                                                          *
*     BBDComm  MATLAB array containing private data returned by a      *
*              previous call to bbdcinit.                              *
*                                                                      *
*  Prerequisites:                                                      *
*  bbdcinit() must be called before this routine is called.            *
*                                                                      *
*  Error Handling:                                                     *
*  All errors are terminal and result in calls to abexitm() with       *
*  a suitable message.  There is nothing useful the caller can do      *
*  to recover.                                                         *
************************************************************************
*  V1A, 04/18/13, GNR - New mex program derived from bbdcregs.c        *
*  ==>, 04/18/13, GNR - Last mod before committing to svn repository   *
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

   /* Standard MATLAB check for proper number and type of
   *  arguments, then locate BBDComData */

   if (nlhs != 0 || nrhs != 1)
      abexitm(BBDcErrMexCall, "Bad arguments to bbdcclse");
   BBDcd = (struct BBDComData *)mxGetData(prhs[0]);
   if (!BBDcd)
      abexitm(BBDcErrMexCall, "Bad BBDcd arg to bbdcclse");

   /* Call the real bbdcquit() function */
   bbdcquit(BBDcd);

   } /* End bbdcclse() */
