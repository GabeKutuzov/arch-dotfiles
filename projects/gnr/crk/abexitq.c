/* (c) Copyright 2013-2016, The Rockefeller University *11115* */
/* $Id: abexitq.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                               ABEXITQ                                *
*                                                                      *
*                Produce an abnormal termination exit                  *
*    but conditionally return to caller if this is a declared test     *
*                                                                      *
************************************************************************
*  V1A, 06/09/13, GNR - Newly written                                  *
*  ==>, 06/09/13, GNR - Last date before committing to svn repository  *
*  Rev, 02/19/16, GNR - Add #include rkarith.h for e64qtest declarat'n *
***********************************************************************/

/* This routine is a copy of abexit() except for the added feature of
returning to the caller if e64qtest() indicates this is a test run.
The reason for separating this function from traditional abexit() is
that abexit() is now declared to be non-returning and that attribute
is useful for optimization of codes that can never be tests.  Tests
that use printf() instead of cryout() need to make their own version
of abexitq() as well as of abexit().

Usage: void abexitq(int code)

Argument: 'code' is a unique abnormal termination code which should
      be documented in the ROCKS manual and on-line.

Return:  This routine returns void if e64qtest() returns TRUE,
      otherwise it does not return.  It currently calls the
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
#include "rkarith.h"

int abexloop;

void abexitq(int code) {

   static char abmsg[] = "0***Program terminated with abend code ";
   char acode[4];             /* Code buffer */

   if (abexloop < 2) {
      abexloop = 2;
      wbcdwt(&code, acode, RK_IORF|RK_LFTJ|RK_NINT|3);
      cryout(RK_P1, abmsg, RK_LN2, acode, RK_CCL+RK_FLUSH+4, NULL);
      }
   if (e64qtest()) return;

#ifdef UNIX
   if (RKC.kparse & RKKP_EDMPRQ) abort();
   exit(code > 255 ? 100 : code);
#else
   exit(code);
#endif
   } /* End abexitq() */
