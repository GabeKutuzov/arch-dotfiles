/* (c) Copyright 2014, The Rockefeller University *11115* */
/* $Id: mrsul.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                                mrsul                                 *
*                                                                      *
*  This function is the unsigned version of mrssl.  It multiplies a    *
*  32-bit unsigned integer by a 32-bit unsigned integer, scales the    *
*  63-bit intermediate product by specified right shift, and returns   *
*  the low-order 32 bits of the result without checking for overflow.  *
*  This routine should be used only when there is no possibility of    *
*  overflow.  Use mrsul[de] for full error checking.                   *
*                                                                      *
*  This is the C-code version for machines that do not have 64-bit     *
*  arithmetic.  A macro is used when 64-bit arithmetic is available.   *
*                                                                      *
*  Synopsis: ui32 mrsul(ui32 x, ui32 y)                                *
*                                                                      *
*  N.B.  Program requires 0 <= s < 64 without checking.                *
*                                                                      *
*  This routine is part of a basic 64-bit arithmetic package for which *
*  full C implementations are provided in order to get one started on  *
*  a new processor.  These versions can be reimplemented in Assembler  *
*  if greater speed is needed.                                         *
************************************************************************
*  V1A, 09/26/14, GNR - New routine, included for package symmetry.    *
*  ==>, 09/26/14, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include <stdlib.h>
#include <math.h>
#include "sysdef.h"
#include "rkarith.h"

ui32 mrsul(ui32 x, ui32 y, int s) {

#ifdef HAS_I64
#error mrsul() is a macro with HAS_I64, mrsul.c should not be compiled

#else          /* Not HAS_I64 */
#define LU16 0x0000ffffUL

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
   x0 = x & LU16;
   y1 = y >> BITSPERSHORT;
   y0 = y & LU16;

   q1 = x0 * y0;
   q2 = x0 * y1;
   /* p1 and p2 cannot overflow:  max is
   *  (2**32 - 2*(2**16) + 1) + 2*(2**16 - 1) = 2**32 - 1 */
   p1 = x1 * y0 + (q1>>BITSPERSHORT) + (q2 & LU16);
   p2 = x1 * y1 + (p1>>BITSPERSHORT) + (q2>>BITSPERSHORT);
   p0 = (q1 & LU16) + (p1<<BITSPERSHORT);

/* Perform the shift */

   if ((ds = s - BITSPERUI32) == 0)
      /* Shift is exactly 32, use the high-order result */
      p0 = p2;
   else if (ds > 0)
      p0 = p2 >> ds;
   else if (s)
      p0 = (p0 >> s) | (p2 << (-ds));
   return p0;
#endif

   } /* End mrsul() */
