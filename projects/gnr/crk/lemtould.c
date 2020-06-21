/* (c) Copyright 2016, The Rockefeller University *11115* */
/* $Id: lemtould.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                              lemtould                                *
*                                                                      *
*  ROCKS library routine for retrieving unsigned long integer data     *
*  from a binary data file in standard little-endian format.  It is    *
*  assumed that the msg pointer may point to an unaligned storage      *
*  area, which means the data must be retrieved byte-by-byte even      *
*  if the storage order already matches.                               *
*                                                                      *
*  This routine is the same as lemtou8 when longs are 64-bits, but     *
*  when longs are only 32 bits, it has to contract the 8 byte message  *
*  word to 4 bytes and check for overflow.  This is the 'd' version    *
*  that reports errors via a call to e64dac.                           *
************************************************************************
*  V2D, 03/02/16, GNR - New routine                                    *
*  ==>, 03/02/16, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include <string.h>
#include "sysdef.h"
#include "swap.h"

ulng lemtould(char *m) {

   register byte *um = (byte *)m;

#if LSIZE == 8
/* A long is 8 bytes */
   register ulng tu8 = ((((((
      (ulng)um[7]  << BITSPERBYTE |
      (ulng)um[6]) << BITSPERBYTE |
      (ulng)um[5]) << BITSPERBYTE |
      (ulng)um[4]) << BITSPERBYTE |
      (ulng)um[3]) << BITSPERBYTE |
      (ulng)um[2]) << BITSPERBYTE |
      (ulng)um[1]) << BITSPERBYTE |
      (ulng)um[0];
   return tu8;

#else
/* A long is 4 bytes, but the message has 8 */
   register ulng tu4 = ((
      (ulng)um[3]  << BITSPERBYTE |
      (ulng)um[2]) << BITSPERBYTE |
      (ulng)um[1]) << BITSPERBYTE |
      (ulng)um[0];
   if (um[4] | um[5] | um[6] | um[7]) {
      e64dac("lemtould"); return UI32_MAX; }
   return tu4;

#endif

   } /* End lemtould() */
