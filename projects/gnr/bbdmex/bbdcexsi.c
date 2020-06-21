/* (c) Copyright 2011, The Rockefeller University *11116* */
/* $Id: bbdcexsi.c 16 2017-01-13 20:36:40Z  $ */
/***********************************************************************
*               BBD (Client Side) mex-Function Package                 *
*                             bbdcexsi.c                               *
*                     Provide extra server input                       *
*                                                                      *
*  This is the matlab mex-function version of bbdcexsi().              *
*  The purpose of this routine is to provide a text string that will   *
*  be inserted in the input to a server (typically CNS) after the      *
*  BBDHOST card created by bbdcminp and before the contents of the     *
*  specified control file.  It is assumed that this string will        *
*  contain a ROCKS EXECUTE card that will provide values for variable  *
*  symbols contained in the control file.  For example, it can be      *
*  used to pass a run number to CNS so that the CNS output files       *
*  automatically carry the same run number as the client.  bbdcexsi()  *
*  must be called by the client before bbdcinit() is called.  (It is   *
*  a separate program in order to avoid changing the bbdcinit() call   *
*  for existing programs.)                                             *
*                                                                      *
*  MATLAB Synopsis:                                                    *
*  bbdcexsi(exstring)                                                  *
*                                                                      *
*  Argument:                                                           *
*     exstring A string containing valid control information for the   *
*              server.  It should end with a newline ('\n') character. *
*              The string is copied to a MATLAB string named BBDCEXSI  *
*              in the global workspace.  This string is deleted when   *
*              bbdcinit is called.                                     *
************************************************************************
*  V1A, 01/17/11, GNR - New program                                    *
***********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define I_AM_MEX

#include "mex.h"
#include "sysdef.h"
#include "bbd.h"

void mexFunction(int nlhs, mxArray *plhs[],
   int nrhs, const mxArray *prhs[]) {

   mwSize   lexsi;         /* Length of exsi string */

/* Standard MATLAB check for proper number and type of arguments */

   if (nlhs != 0 || nrhs != 1 || !mxIsChar(prhs[0]) ||
         (lexsi = mxGetNumberOfElements(prhs[0])) == 0)
      abexitm(BBDcErrMexCall, "bbdcexsi requires one string argument");

/* Save a copy for bbdcinit */

   if (mexPutVariable("global", "BBDCEXSI", prhs[0]) != 0)
      abexitm(BBDcErrControl, "bbdcexsi unable to copy arg string");

   } /* End bbdcexsi() */
