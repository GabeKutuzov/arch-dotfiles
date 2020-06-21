/* (c) Copyright 2013, The Rockefeller University *11115* */
/* $Id: bitcnt.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*          BITCNT - Count the number of '1' bits in an array           *
*                                                                      *
*  This routine returns the number of bits set to '1' in 'bytlen'      *
*  bytes starting at 'array'.                                          *
*                                                                      *
*  Usage:                                                              *
*                                                                      *
*  long bitcnt(void *array, long bytlen)                               *
*                                                                      *
************************************************************************
*  V1A, 01/21/13, GNR - Broken out from bitpkg.c                       *
*  ==>, 01/21/13, GNR - Last date before committing to svn repository  *
***********************************************************************/
#include <stdlib.h>
#include "sysdef.h"
#include "bapkg.h"            /* Make sure prototypes get checked */

/*=====================================================================*
*                               bitcnt                                 *
*=====================================================================*/

long bitcnt(void *array, long bytlen) {
   static unsigned char count[256] = {
      0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4,
      1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
      1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
      2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
      1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
      2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
      2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
      3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
      1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
      2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
      2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
      3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
      2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
      3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
      3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
      4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8};

   register unsigned char *pa = (unsigned char *)array;
   unsigned char *paend = pa + bytlen;
   long total = 0;

   while (pa < paend) total += (long)count[*pa++];
   return total;
   }

