/* (c) Copyright 2013, The Rockefeller University *11115* */
/* $Id: mrsruld.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                               mrsruld                                *
*                                                                      *
*  This function is the unsigned version of mrsrsld.  It multiplies a  *
*  32-bit unsigned integer by a 32-bit unsigned integer, adds 1/2 of   *
*  what will be the ones bit after scaling, scales the 63-bit inter-   *
*  mediate product by specified right shift, and returns the low-      *
*  order 32 bits of the result, reporting an overflow if the result    *
*  exceeds 32 bits.                                                    *
*  This is the version that uses the error code stored by e64dec().    *
*                                                                      *
*  Synopsis: ui32 mrsruld(ui32 x, ui32 y, int s)                       *
*                                                                      *
*  N.B.  Program assumes 0 <= s < 64 without checking.                 *
*                                                                      *
*  This routine is part of a basic 64-bit arithmetic package for which *
*  full C implementations are provided in order to get one started on  *
*  a new processor.  These versions can be reimplemented in Assembler  *
*  if greater speed is needed.                                         *
************************************************************************
*  V1A, 10/24/13, GNR - New routine, based on msrsle                   *
*  ==>, 10/24/13, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include <stdlib.h>
#include <math.h>
#include "sysdef.h"
#include "rkarith.h"

ui32 mrsruld(ui32 x, ui32 y, int s) {

#ifdef HAS_I64
#if LSIZE == 8
#define LU32 0x00000000ffffffffUL
#define L01  0x0000000000000001UL
#else
#define LU32 0x00000000ffffffffULL
#define L01  0x0000000000000001ULL
#endif

   ui64 prod;                 /* Product */

   prod = (ui64)x * (ui64)y;
   /* If there is no shift, there is no rounding either */
   if (s)
      prod = (prod + (L01 << (s-1))) >> s;
   if (prod > LU32) {
      e64dac("mrsruld");
      return (ui32)LU32;
      }
   return (ui32)prod;

#else          /* Not HAS_I64 */
#ifdef L16
#undef L16
#endif
#define L16 0x0000ffffUL

   ui32 x0,x1,y0,y1;
   ui32 p0,p1,p2,q1,q2;
   int ds;

/* Multiply (x1*(2**16) + x0) by (y1*(2**16) + y0),
*  where x1, x0, y1, y0 are 16-bit quantities.
*  High 32 bits of result = x1*y1 + <high order 16 bits of x1*y0> +
*     <high order 16 bits of x0*y1> + possible carry from low order.
*  Low 32 bits of result =  x0*y0 + <low order 16 bits of x1*y0> +
*     <low order 16 bits of x0*y1>.  */

   x1 = x >> BITSPERSHORT;
   x0 = x & L16;
   y1 = y >> BITSPERSHORT;
   y0 = y & L16;

   q1 = x0 * y0;
   q2 = x0 * y1;
   /* p1 and p2 cannot overflow:  max is
   *  (2**32 - 2*(2**16) + 1) + 2*(2**16 - 1) = 2**32 - 1 */
   p1 = x1 * y0 + (q1>>BITSPERSHORT) + (q2 & L16);
   p2 = x1 * y1 + (p1>>BITSPERSHORT) + (q2>>BITSPERSHORT);
   p0 = (q1 & L16) + (p1<<BITSPERSHORT);

/* Perform the round and shift */

   if ((ds = s - BITSPERUI32) == 0)
      /* Shift is exactly 32, add high order bit from p0 */
      p0 = p2 + (p0 >> (BITSPERUI32-1));
   else if (ds > 0)
      p0 = (p2 + (1UL << (ds-1))) >> ds;
   else {
      /* If there is no shift, there is no rounding either */
      if (s) {
         q1 = 1UL << (s-1);
         p2 += (~p0 < q1);
         p0 = ((p0 + q1) >> s) | (p2 << (-ds));
         p2 >>= s; }

      /* There is an overflow if any bits are left in p2
      *  after scaling */
      if (p2) {
         e64dac("mrsruld");
         return UI32_MAX;
         }
      }
   return p0;
#endif

   } /* End mrsruld() */
