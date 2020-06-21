/* (c) Copyright 2011, The Rockefeller University *11115* */
/* $Id: printf.c 19 2011-02-23 22:56:28Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                               PRINTF                                 *
*                                                                      *
*      Standard C library printf() emulation with added features       *
*                                                                      *
*  Synopsis: int printf(const char *format, ...)                       *
*                                                                      *
*  See description in rkprintf.c for use of added format codes.        *
*  The idea is that this version of printf() can be linked with any    *
*  program that already has printf calls, or uses library functions    *
*  that have printf calls, but can now use the features of convrt(),   *
*  such as array output, noninteger fixed-point numbers, etc. and      *
*  the pagination and spout output provided by cryout().               *
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
*                               pfoutcb                                *
*                                                                      *
*  This is the callback routine that rkprintf() uses to write output   *
*  via cryout().  Note that rkprintf always stores the line count in   *
*  NPC.cnln (possibly > 6) and sets NPC.ccode to RK_LN0, so here we    *
*  must not only test RK.rmlns like tlines(), but also update it.      *
*=====================================================================*/

void pfoutcb(long lout) {

   if (lout >= RK_SKIP) abexit(181);
   if (NPC.cspt > 0) spout(NPC.cspt);
   if ((RK.rmlns -= NPC.cnln) < 0) RK.rmlns = -1;
   if (NPC.totl == 0) {
      *NPC.pdat++ = '\0'; NPC.totl = 1; }
   cryout(NPC.cprty | RK_PF, NPC.line, NPC.ccode | lout, NULL);
   NPC.pdat = NPC.line;       /* Start a new line */

   } /* End pfoutcb() */

/*=====================================================================*
*                               printf                                 *
*=====================================================================*/

int printf(const char *format, ...) {

   /* First time, allocate a line buffer big enough for any subtitles */
   if (!NPC.pfline)
      NPC.pfline = mallocv(LPFLINE, "printf buffer");

   va_start(NPC.lptr, format);               /* Pick up item list */
   rkprintf(pfoutcb, NPC.pfline, LPFLINE, format); /* Do the work */
   va_end(NPC.lptr);                         /* Clean up the list */

   return (int)NPC.totl;

   } /* End printf() */
