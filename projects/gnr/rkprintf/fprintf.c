/* (c) Copyright 2011, The Rockefeller University *11115* */
/* $Id: fprintf.c 19 2011-02-23 22:56:28Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                               FPRINTF                                *
*                                                                      *
*     Standard C library fprintf() emulation with added features       *
*                                                                      *
*  Synopsis: int fprintf(FILE *stream, const char *format, ...)        *
*                                                                      *
*  See description in rkprintf.c for use of added format codes.        *
*  The idea is that this version of fprintf() can be linked with any   *
*  program that already has fprintf calls, or uses library functions   *
*  that have fprintf calls, but can now use the features of convrt(),  *
*  such as array output, noninteger fixed-point numbers, etc.          *
*                                                                      *
*  N.B.  The main program of any program that uses this routine must   *
*  pound-define MAIN and OMIT_ROCKV followed by pound-include rocks.h  *
************************************************************************
*  V1A, 11/27/11, GNR - New program                                    *
*  ==>, 12/03/11, GNR - Last date before committing to svn repository  *
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
*                              fpfoutcb                                *
*                                                                      *
*  This is the callback routine that rkprintf() uses to write output.  *
*=====================================================================*/

void fpfoutcb(long lout) {

   if (NPC.ccode & RK_FLUSH && fflush(NPC.uf.fpfile))
      abexitme(195, "Error flushing fprintf file");
   if (fwrite(NPC.line, 1, lout, NPC.uf.fpfile) < lout)
      abexitme(195, "Error writing fprintf file");
   NPC.pdat = NPC.line;       /* Start a new line */
   } /* End fpfoutcb() */

/*=====================================================================*
*                               fprintf                                 *
*=====================================================================*/

int fprintf(FILE *stream, const char *format, ...) {

   /* First time, allocate a line buffer big enough for any subtitles */
   if (!NPC.pfline)
      NPC.pfline = mallocv(LPFLINE, "printf buffer");
   NPC.uf.fpfile = stream;               /* Save FILE for fpfoutcb */

   va_start(NPC.lptr, format);                /* Pick up item list */
   rkprintf(fpfoutcb, NPC.pfline, LPFLINE, format); /* Do the work */
   va_end(NPC.lptr);                          /* Clean up the list */

   return (int)NPC.totl;

   } /* End fprintf() */
