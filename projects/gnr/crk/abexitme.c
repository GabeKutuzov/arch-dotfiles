/* (c) Copyright 1999-2008, The Rockefeller University *11115* */
/* $Id: abexitme.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                              ABEXITME                                *
*                                                                      *
*         Produce an abnormal termination exit with a message          *
*                   and also a system errno message                    *
*                                                                      *
************************************************************************
*  V1A, 09/19/99, GNR - New routine                                    *
*  V1B, 02/23/08, GNR - Use perror to get a system error message       *
*  ==>, 09/23/08, GNR - Last date before committing to svn repository  *
***********************************************************************/

/*
   This routine is used to produce an abnormal termination from a
program that uses the ROCKS user interface.  It prints a message
via cryout, then calls abexit() to perform final termination.
Users may provide their own version of this routine to avoid use
of cryout.  If final clean-up is all that is required, it is
sufficient to replace abexit() itself.

Usage: void abexitme(int code, char *emsg)

Arguments: 'code' is a unique abnormal termination code which should
      be documented in the ROCKS manual and in src/crk/errtab.
         'emsg' is the error message text to be printed.  "***" will
      be prepended to this message.  The value of the system "errno"
      will also be printed, with an appropriate message if this is a
      UNIX system.

Return:  This routine does not return.  It exits via abexit().
*/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "sysdef.h"
#include "rocks.h"
#include "rkxtra.h"

#define MXENCHAR  5        /* Max errno chars */

extern int abexloop;

void abexitme(int code, char *emsg) {

   if (!abexloop) {
#if defined(UNIX) || defined(DOS)
      int sverr = errno;   /* Save error across cryout call */
#endif
      char acode[MXENCHAR+1];

      abexloop = 1;
      cryout(RK_P1, "0***", RK_LN2+4, emsg, RK_CCL+128, NULL);
#if defined(UNIX) || defined(DOS)
      wbcdwt(&sverr, acode, RK_IORF|RK_LFTJ|MXENCHAR-1);
      cryout(RK_P1, " ***The system error code is ", RK_LN1,
         acode, RK_CCL+RK_FLUSH+MXENCHAR, NULL);
      errno = sverr;
      perror("***System error text");
#else
      cryout(RK_P1, " ***Error details not available on this OS",
         RK_LN1+RK_FLUSH, NULL);
#endif
      }
   abexit(code);
   } /* End abexitme() */
