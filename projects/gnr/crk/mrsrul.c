/* (c) Copyright 2013, The Rockefeller University *11115* */
/* $Id: mrsrul.c 67 2018-05-07 22:08:53Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                               mrsrul                                 *
*                                                                      *
*  This function adds rounding to mrsul. It multiplies a 32-bit unsign-*
*  ed integer by a 32-bit unsigned integer, adds 1/2 of what will be   *
*  the ones bit after scaling, scales the 62-bit intermediate product  *
*  by a specified right shift, and returns the low-order 32 bits of    *
*  the result.  This version assumes that the shift is known to be     *
*  large enough that overflow is impossible, so no overflow checking   *
*  is done.                                                            *
*                                                                      *
*  Synopsis: ui32 mrsrul(ui32 x, ui32 y, int s)                        *
*                                                                      *
*  N.B.  Program assumes 0 <= s < 64 without checking.                 *
*                                                                      *
*  This routine is part of a basic 64-bit arithmetic package for which *
*  full C implementations are provided in order to get one started on  *
*  a new processor.  These versions can be reimplemented in Assembler  *
*  if greater speed is needed.                                         *
************************************************************************
*  V1A, 04/21/18, GNR - New routine, based on mrsruld                  *
*  ==>, 04/21/18, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include <stdlib.h>
#include <math.h>
#include "sysdef.h"
#include "rkarith.h"

ui32 mrsrul(ui32 x, ui32 y, int s) {

#ifdef HAS_I64

   ui64 prod = (ui64)x * (ui64)y;
   /* If there is no shift, there is no rounding either */
   if (s > 0) prod = (prod + (SI64_01 << (s-1))) >> s;
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

/* Perform the round and shift (but no round if no shift) */

   if ((ds = s - BITSPERUI32) == 0)
      /* Shift is exactly 32, add high order bit from p0 */
      p0 = p2 + (p0 >> (BITSPERUI32-1));
   else if (ds > 0)
      /* Shift > 32, use high order shifted by (s-32) */
      p0 = (p2 + (1UL << (ds-1))) >> ds;
   else if (s) {
      /* Shift < 32, must bring s bits from p2 into left of p0 */
      q1 = 1UL << (s-1);
      p2 += (~p0 < q1);
      p0 = ((p0 + q1) >> s) | (p2 << (-ds));
      }
   return p0;
#endif

   } /* End mrsrul() */
