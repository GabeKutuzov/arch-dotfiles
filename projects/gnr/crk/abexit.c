/* (c) Copyright 1989-2009, The Rockefeller University *11115* */
/* $Id: abexit.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                               ABEXIT                                 *
*                                                                      *
*                Produce an abnormal termination exit                  *
*                                                                      *
************************************************************************
*  V1A, 02/09/89, GNR - Newly written                                  *
*  Rev, 04/19/89, GNR - Incorporate rkxtra.h                           *
*  Rev, 11/05/97, GNR - Revise for addition of abexitm, ssprintf       *
*  Rev, 04/02/98, GNR - Add loop detection                             *
*  Rev, 02/06/00, GNR - Under UNIX, replaces codes > 255 with 100      *
*  ==>, 09/23/08, GNR - Last date before committing to svn repository  *
*  Rev, 08/22/09, GNR - Replace ibcdwt w/wbcdwt, fix for 64-bit        *
***********************************************************************/

/*
   This routine is used to produce an abnormal termination from a
program that uses the ROCKS user interface.  It prints a message
via cryout giving the termination code.  This message has the
incidental function of flushing the cryout buffer so no print is
lost.

   This routine also provides a common bottleneck through which
all (user) abnormal terminations flow.  Users may provide their
own version of this routine, either to avoid use of cryout, or
to provide any additional clean-up which may be required.

Usage: void abexit(int code)

Argument: 'code' is a unique abnormal termination code which should
      be documented in the ROCKS manual and on-line.

Return:  This routine does not return.  It currently calls the
      standard C library exit routine, but may be implemented on
      IBM systems to produce a genuine abend.
*/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "sysdef.h"
#include "rocks.h"
#include "rockv.h"
#include "rkxtra.h"

int abexloop;

void abexit(int code) {

   static char abmsg[] = "0***Program terminated with abend code ";
   char acode[4];             /* Code buffer */

   if (abexloop < 2) {
      abexloop = 2;
      wbcdwt(&code, acode, RK_IORF|RK_LFTJ|RK_NINT|3);
      cryout(RK_P1, abmsg, RK_LN2, acode, RK_CCL+RK_FLUSH+4, NULL);
      }
#ifdef UNIX
   if (RKC.kparse & RKKP_EDMPRQ) abort();
   exit(code > 255 ? 100 : code);
#else
   exit(code);
#endif
   } /* End abexit() */
