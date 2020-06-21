/* (c) Copyright 2013, The Rockefeller University *11115* */
/* $Id: abexitmq.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                              ABEXITMQ                                *
*                                                                      *
*         Produce an abnormal termination exit with a message          *
*    but conditionally return to caller if this is a declared test     *
*                                                                      *
************************************************************************
*  V1A, 06/09/13, GNR - New routine                                    *
*  ==>, 06/09/13, GNR - Last date before committing to svn repository  *
***********************************************************************/

/* This routine is a copy of abexitm() except for the added feature of
returning to the caller if e64qtest() indicates this is a test run.
The reason for separating this function from traditional abexitm() is
that abexitm() is now declared to be non-returning and that attribute
is useful for optimization of codes that can never be tests.  Tests
that use printf() instead of cryout() need to make their own version
of abexitmq() as well as of abexitm().

Usage: void abexitmq(int code, char *emsg)

Arguments: 'code' is a unique abnormal termination code which should
      be documented in the ROCKS manual and on-line.
         'emsg' is the error message text to be printed.  "***" will
      be prepended to this message.  Maximum length is 128 characters.

Return:  This routine returns void if e64qtest() returns TRUE,
      otherwise it does not return.  After printing the message, it
      currently calls abexitq() to finish up.
*/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "sysdef.h"
#include "rocks.h"
#include "rkxtra.h"

extern int abexloop;

void abexitmq(int code, char *emsg) {

   if (!abexloop) {
      abexloop = 1;
      cryout(RK_P1, "0***", RK_LN2+4, emsg, RK_CCL+128, NULL);
      }
   abexitq(code);
   } /* End abexitmq() */

