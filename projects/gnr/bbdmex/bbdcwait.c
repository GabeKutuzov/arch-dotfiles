/* (c) Copyright 2010, The Rockefeller University *11116* */
/* $Id: bbdcwait.c 16 2017-01-13 20:36:40Z  $ */
/***********************************************************************
*               BBD (Client Side) mex-Function Package                 *
*                             bbdcwait.c                               *
*                       Wait for CNS to finish                         *
*                                                                      *
*  This is the matlab mex-function version of bbdcwait().              *
*  This routine may be called by the application when it wishes to     *
*  wait until CNS finishes, rather than terminate at once via a call   *
*  to exit, which runs bbdcquit to kill CNS from this end.             *
*                                                                      *
*  MATLAB Synopsis: bbdcwait(BBDComm)                                  *
*                                                                      *
*  Argument:                                                           *
*     BBDComm  MATLAB array containing private data returned by a      *
*              previous call to bbdcinit.                              *
*                                                                      *
*  Return Values:  None                                                *
************************************************************************
*  V1A, 10/22/10, GNR - New mex version                                *
*  ==>, 11/04/10, GNR - Last mod before committing to svn repository   *
***********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>

#define I_AM_MEX

#include "mex.h"
#include "sysdef.h"
#include "bbd.h"
#include "rksubs.h"

void mexFunction(int nlhs, mxArray *plhs[],
      int nrhs, const mxArray *prhs[]) {

   struct BBDComData *BBDcd;  /* Common BBD data struct */
   int bbdstat;

   /* Standard MATLAB check for proper number and type of
   *  arguments, then locate BBDComData */

   if (nlhs != 0 || nrhs != 1)
      abexitm(BBDcErrMexCall, "Bad arguments to bbdcwait");
   BBDcd = (struct BBDComData *)mxGetData(prhs[0]);
   if (!BBDcd)
      abexitm(BBDcErrMexCall, "Bad BBDcd arg to bbdcwait");

   waitpid(BBDcd->LogProc, &bbdstat, 0);

   } /* End bbdcwait() */
