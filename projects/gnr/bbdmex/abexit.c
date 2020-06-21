/* (c) Copyright 2010, The Rockefeller University *11114* */
/* $Id: abexit.c 16 2017-01-13 20:36:40Z  $ */
/***********************************************************************
*          Replacement ROCKS Routines for MATLAB Environment           *
*                                                                      *
*  This file contains replacements for the standard ROCKS error exit   *
*  routines, abexit, abexitm, and abexitme, that are designed for use  *
*  in a MATLAB mex-file environment.  This assures that no ROCKS       *
*  interface routines will be linked incorrectly into the MATLAB       *
*  environment.  abexitme() is made equivalent to abexitm(), as errno  *
*  is irrelevant.  The routines exit via mexErrMsgTxt.  The mexAtExit  *
*  call can be used to add additional functionality if needed for a    *
*  particular application.                                             *
*                                                                      *
*  Because these routines are very short, it is probably easier to     *
*  just include them as a .o file with the mex code rather than to     *
*  build a library.                                                    *
************************************************************************
*  V1A, 10/29/10, GNR - Routines borrowed from getgd3.c                *
*  ==>, 11/04/10, GNR - Last mod before committing to svn repository   *
***********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "mex.h"
#include "sysdef.h"
#include "rksubs.h"

static int abexloop = 0;

void abexit(int code) {

   if (!abexloop) abexloop = TRUE;
   mexErrMsgTxt(ssprintf(NULL, "\n***Terminated with abexit "
      "code %4d", code));
   exit(102);     /* Avoid compile-time no-return warning */

   } /* End abexit() */

void abexitm(int code, char *msg) {

   mexPrintf("\n*** %.128s\n", msg);
   abexit(code);

   } /* End abexitm() */

void abexitme(int code, char *msg) {

   int sverrno = errno;
   mexPrintf("\n*** %.128s\n", msg);
   mexPrintf("*** System errno is %d\n", sverrno);
   abexit(code);

   } /* End abexitme() */
