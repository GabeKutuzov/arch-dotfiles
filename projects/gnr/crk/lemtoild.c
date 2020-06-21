/* (c) Copyright 2016, The Rockefeller University *11115* */
/* $Id: lemtoild.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                              lemtoild                                *
*                                                                      *
*  ROCKS library routine for retrieving long integer data from a       *
*  binary data file in standard little-endian format.  It is assumed   *
*  that the msg pointer may point to an unaligned storage area, which  *
*  means the data must be retrieved byte-by-byte even if the storage   *
*  order already matches.                                              *
*                                                                      *
*  This routine is the same as lemtoi8 when longs are 64-bits, but     *
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

long lemtoild(char *m) {

   register byte *um = (byte *)m;

#if LSIZE == 8
/* A long is 8 bytes */
   register long ti8 = ((((((
      (long)um[7]  << BITSPERBYTE |
      (long)um[6]) << BITSPERBYTE |
      (long)um[5]) << BITSPERBYTE |
      (long)um[4]) << BITSPERBYTE |
      (long)um[3]) << BITSPERBYTE |
      (long)um[2]) << BITSPERBYTE |
      (long)um[1]) << BITSPERBYTE |
      (long)um[0];
   return ti8;

#else
/* A long is 4 bytes, but the message has 8 */
   register long ti4 = ((
      (long)um[3]  << BITSPERBYTE |
      (long)um[2]) << BITSPERBYTE |
      (long)um[1]) << BITSPERBYTE |
      (long)um[0];
   if (ti4 >= 0) {
      if (um[4] | um[5] | um[6] | um[7]) {
         e64dac("lemtoild"); return SI32_MAX; }
      }
   else {
      if ((um[4] & um[5] & um[6] & um[7]) != BYTE_MAX) {
         e64dac("lemtoild"); return -SI32_MAX; }
      }
   return ti4;

#endif

   } /* End lemtoild() */
