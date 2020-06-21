/* (c) Copyright 2011, The Rockefeller University *11115* */
/* $Id: sprintf.c 19 2011-02-23 22:56:28Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                               SPRINTF                                *
*                                                                      *
*     Standard C library sprintf() emulation with added features       *
*                                                                      *
*  Synopsis: int sprintf(char *str, const char *format, ...)           *
*                                                                      *
*  See description in rkprintf.c for use of added format codes.        *
*  The idea is that this version of sprintf() can be linked with any   *
*  program that already has printf calls, or uses library functions    *
*  that have printf calls, but can now use the features of convrt(),   *
*  such as array output, noninteger fixed-point numbers, etc.          *
*                                                                      *
*  N.B.  sprintf() does not provide a mechanism to assure that the     *
*  output string can not be overfilled.  The fact that field widths    *
*  in rkprintf() are exact, not minima, helps, but still, if one is    *
*  not exactly sure that this cannot happen, it is better to use       *
*  snprintf().  Also, this version of sprintf() arbitrarily limits     *
*  the length of the output to MXLSPFO (currently 255) characters      *
*  to minimize the effects of any overflows that do occur.             *
*                                                                      *
*  N.B.  The main program of any program that uses this routine must   *
*  pound-define MAIN and OMIT_ROCKV followed by pound-include rocks.h  *
************************************************************************
*  V1A, 11/26/11, GNR - New program                                    *
*  ==>, 12/04/11, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "sysdef.h"
#include "rkprintf.h"
#include "rkxtra.h"

extern struct RKPFC NPC;

/*=====================================================================*
*                              spfoutcb                                *
*                                                                      *
*  This is the callback routine that rkprintf() calls to write output. *
*  When it finds newlines in the formatted output, there is nothing to *
*  do here.  When it receives a FLUSH call, that is an error.  When at *
*  the end of the format, and the NULLTERM flag is on, a terminating   *
*  '\0' must be added at the end of the string (not counted in ltot).  *
*=====================================================================*/

void spfoutcb(long lout) {

   if (NPC.ccode & RK_FLUSH) abexit(196);
   if ((NPC.pfflgs & (ENDITALL|NULLTERM)) == (ENDITALL|NULLTERM)) {
      if (NPC.pdat + 1 >= NPC.top) abexit(181);
      NPC.pdat[lout] = '\0';
      }

   } /* End spfoutcb() */

/*=====================================================================*
*                               sprintf                                *
*=====================================================================*/

int sprintf(char *str, const char *format, ...) {

   va_start(NPC.lptr, format);                  /* Pick up item list */
   rkprintf(spfoutcb, str, MXLSPFO, format);    /* Do all the work */
   va_end(NPC.lptr);                            /* Clean up the list */

   return (int)NPC.totl;

   } /* End sprintf() */
