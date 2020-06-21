/* (c) Copyright 1992-2008, The Rockefeller University *11115* */
/* $Id: strncpy0.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                             strncpy0.c                               *
*                                                                      *
*  This routine performs an strncpy, then appends a terminating 0      *
*  to the result, which strncpy does not do when the max length is     *
*  reached.  (It stores the 0 every time, probably quicker than        *
*  testing for max length.)  The intended use is for cases where a     *
*  receiving array of size one greater than the max length is known    *
*  to exist, but the source may be longer.  Use of the standard        *
*  strncpy() in this case can produce strings with garbage on the end. *
*                                                                      *
************************************************************************
*  V1A, 04/03/92, GNR - Newly written                                  *
*  Rev, 12/13/07, GNR - Change int arg and result to size_t            *
*  ==>, 12/07/08, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include <stddef.h>
#include <string.h>
#include "sysdef.h"
#include "rksubs.h"

void strncpy0(char *d, const char *s, size_t mxl) {

   strncpy(d, s, mxl);
   d[mxl] = '\0';

   } /* End strncpy0() */
