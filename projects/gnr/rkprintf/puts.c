/* (c) Copyright 2012, The Rockefeller University *11115* */
/* $Id: puts.c 19 2011-02-23 22:56:28Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                                PUTS                                  *
*                                                                      *
*        Standard C library puts() emulation that counts lines         *
*                     for integration with cryout                      *
*                                                                      *
*  Synopsis: int puts(const char *string)                              *
*                                                                      *
*  This program uses cryout() to write a string to stdout with line    *
*  counting and possibly printing page titles and subtitles.  It is    *
*  needed for our implementation of printf() to work correctly, even   *
*  if there are no puts() calls in the application, because it has     *
*  been demonstrated that the gcc compiler replaces printf calls       *
*  with no conversions (i.e. of the form printf("string")) with calls  *
*  instead to puts() and I could find no documented option to turn     *
*  this feature off -- way too clever!                                 *
*                                                                      *
*  Because the library puts() is documented as adding a NL, we do the  *
*  same and don't bother counting whether there are more NLs in the    *
*  argument string.  Default (highest) output priority is used.        *
*                                                                      *
*  N.B.  The main program of any program that uses this routine must   *
*  pound-define MAIN and OMIT_ROCKV followed by pound-include rocks.h  *
************************************************************************
*  V1A, 01/28/12, GNR - New program                                    *
*  ==>, 01/28/12, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "sysdef.h"
#include "rocks.h"
#include "rkxtra.h"

/*=====================================================================*
*                                puts                                  *
*=====================================================================*/

int puts(const char *string) {

   cryout(RK_PF, string, RK_LN1, "\n", 1, NULL);

   return 0;

   } /* End puts() */
