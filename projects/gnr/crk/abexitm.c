/* (c) Copyright 1997-2008, The Rockefeller University *11115* */
/* $Id: abexitm.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                               ABEXITM                                *
*                                                                      *
*         Produce an abnormal termination exit with a message          *
*                                                                      *
************************************************************************
*  V1A, 11/05/97, GNR - New routine                                    *
*  ==>, 09/23/08, GNR - Last date before committing to svn repository  *
***********************************************************************/

/*
   This routine is used to produce an abnormal termination from a
program that uses the ROCKS user interface.  It prints a message
via cryout, then calls abexit() to perform final termination.
Users may provide their own version of this routine to avoid use
of cryout.  If final clean-up is all that is required, it is
sufficient to replace abexit() itself.

Usage: void abexitm(int code, char *emsg)

Arguments: 'code' is a unique abnormal termination code which should
      be documented in the ROCKS manual and on-line.
         'emsg' is the error message text to be printed.  "***" will
      be prepended to this message.  Maximum length is 128 characters.

Return:  This routine does not return.  It exits via abexit().
*/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "sysdef.h"
#include "rocks.h"
#include "rkxtra.h"

extern int abexloop;

void abexitm(int code, char *emsg) {

   if (!abexloop) {
      abexloop = 1;
      cryout(RK_P1, "0***", RK_LN2+4, emsg, RK_CCL+128, NULL);
      }
   abexit(code);
   } /* End abexitm() */

