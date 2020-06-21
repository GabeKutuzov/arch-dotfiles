/* (c) Copyright 1992-2008, The Rockefeller University *11115* */
/* $Id: strnlen.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                              strnlen.c                               *
*                                                                      *
*      Calculate length of a string with a given maximum length        *
*                                                                      *
************************************************************************
*  V1A, 04/03/92, GNR - Newly written                                  *
*  Rev, 12/13/07, GNR - Change int arg and result to size_t            *
*  ==>, 02/23/08, GNR - Last date before committing to svn repository  *
*  Rev, 12/07/08, GNR - Make it safe for 64-bit use                    *
***********************************************************************/

#include <stddef.h>
#include "sysdef.h"
#include "rksubs.h"

size_t strnlen(const char *s, size_t mxl) {

   size_t n = 0;

   while (*s++ != '\0' && n < mxl) n++;

   return n;

   } /* End strnlen() */
