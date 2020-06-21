/* (c) Copyright 2011, The Rockefeller University *11115* */
/* $Id: rfprintf.c 19 2011-02-23 22:56:28Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                              RFPRINTF                                *
*                                                                      *
*  Formatted output routine similar to standard C library fprintf()    *
*  but with output to an RFdef file and added convrt() features.       *
*                                                                      *
*  Synopsis: int rfprintf(rkfd *rffile, const char *format, ...)       *
*                                                                      *
*  See description in rkprintf.c for use of added format codes.        *
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
*                              rfpoutcb                                *
*                                                                      *
*  This is the callback routine that rkprintf() uses to write output.  *
*=====================================================================*/

void rfpoutcb(long lout) {

   if (NPC.ccode & RK_FLUSH) rfflush(NPC.uf.rffile, ABORT);
   rfwrite(NPC.uf.rffile, NPC.line, lout, ABORT);
   NPC.pdat = NPC.line;       /* Start a new line */
   } /* End rfpoutcb() */

/*=====================================================================*
*                               rfprintf                                 *
*=====================================================================*/

long rfprintf(rkfd *rffile, const char *format, ...) {

   /* First time, allocate a line buffer big enough for any subtitles */
   if (!NPC.pfline)
      NPC.pfline = mallocv(LPFLINE, "printf buffer");
   NPC.uf.rffile = rffile;               /* Save FILE for rfpoutcb */

   va_start(NPC.lptr, format);                /* Pick up item list */
   rkprintf(rfpoutcb, NPC.pfline, LPFLINE, format); /* Do the work */
   va_end(NPC.lptr);                          /* Clean up the list */

   return NPC.totl;

   } /* End rfprintf() */
