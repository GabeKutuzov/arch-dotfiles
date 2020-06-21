/* (c) Copyright 1995-2009, The Rockefeller University *11115* */
/* $Id: bitior.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                               bitior                                 *
*                                                                      *
*  This function computes the logical 'OR' of two bit strings.  The    *
*  strings may begin at arbitrary bit offsets from their starting      *
*  addresses.  Bits shifted to the right always enter the byte at the  *
*  next higher address, regardless of whether the machine is big- or   *
*  little-endian.  It is guaranteed that bytes beyond the specified    *
*  length are not affected, even if in the same byte as part of the    *
*  result string.                                                      *
*                                                                      *
*  Synopsis: void bitior(byte *t1, int jt, byte *s1, int js, long len) *
*                                                                      *
*  Arguments:                                                          *
*     t1    Address of target bit array.                               *
*     jt    Offset of first bit in target from left bit of byte at t1, *
*           counting from 0.  May exceed size of one byte.             *
*     s1    Address of source bit array.                               *
*     js    Offset of first bit in source from left bit of byte at s1, *
*           counting from 0.  May exceed size of one bute.             *
*     len   Length of source and target bit arrays in bits.            *
*                                                                      *
*  Caution: Bit array offsets in ROCKS routines bitset(), etc.         *
*           count from 1 rather than from 0, as here.                  *
*                                                                      *
************************************************************************
*  V1A, 06/03/95, GNR - Initial version                                *
*  ==>, 11/09/08, GNR - Last date before committing to svn repository  *
*  Rev, 01/07/09, GNR - Change name to bitior -- bitor is C++ keyword  *
***********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "bapkg.h"            /* Where bitior is prototyped */

void bitior(unsigned char *t1, int jt,
            unsigned char *s1, int js, long len) {

   /* Mask to isolate 1...BITSPERBYTE bits at left of a byte */
   static unsigned char lmask[BITSPERBYTE] = {
      0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe, 0xff };
   /* Mask to isolate BITSPERBYTE ...1 bits at right of a byte */
   static unsigned char rmask[BITSPERBYTE] = {
      0xff, 0x7f, 0x3f, 0x1f, 0x0f, 0x07, 0x03, 0x01 };

   unsigned char *s = s1 + ByteOffset(js),*s2;
   unsigned char *t = t1 + ByteOffset(jt),*t2;
   long jt2;                     /* Bit offset in final target byte */
   register unsigned char ac;    /* Intermediate byte accumulator */

   jt &= (BITSPERBYTE-1);
   js &= (BITSPERBYTE-1);
   jt2 = jt + len - 1;

/* First case:  Offsets in the two bit arrays are equal */

   if (jt == js) {
      s2 = s + ByteOffset(jt2);
      ac = *s & rmask[js];
      while (s < s2) {
         *t++ |= ac;
         ac = *(++s);
         } /* End while */
      }

/* Second case:  jt < js, strings must be left shifted */

   else if (jt < js) {
      int ls = js - jt;          /* Left shift */
      int rs = BITSPERBYTE - ls; /* Right shift */
      s2 = s + ByteOffset(js + len - 1);
      t2 = t + ByteOffset(jt2);
      ac = (*s & rmask[js])<<ls;
      if (s < s2) ac |= (*(++s)>>rs);
      while (t < t2) {
         *t++ |= ac;
         ac = (*s << ls) | (*(s+1) >> rs);
         ++s;
         }
      }

/* Third case:  jt > js, strings must be right shifted */

   else {
      int rs = jt - js;          /* Right shift */
      int ls = BITSPERBYTE - rs; /* Left shift */
      s2 = s + ByteOffset(js + len - 1);
      t2 = t + ByteOffset(jt2);
      ac = (unsigned int)(*s & rmask[js])>>rs;
      while (s < s2) {
         *t++ |= ac;
         ac = (*s << ls) | (*(s+1) >> rs);
         ++s;
         }
      if (t < t2) {
         *t++ |= ac;
         ac = (*s << ls);
         }
      }

   *t |= ac & lmask[BitRemainder(jt2)];
   } /* End bitior() */
