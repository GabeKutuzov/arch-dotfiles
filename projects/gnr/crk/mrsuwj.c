/* (c) Copyright 2014, The Rockefeller University *11115* */
/* $Id: mrsuwj.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                               mrsuwj                                 *
*                                                                      *
*  This function emulates the way fixed point multiplication should    *
*  be implemented in C--multiply two 32-bit numbers to get a 64-bit    *
*  result--then performs a right shift.  There is no possibility of    *
*  overflow.                                                           *
*                                                                      *
*  mrsuwj multiplies a 32-bit unsigned integer by a 32-bit unsigned    *
*  integer, scales the 63-bit intermediate product by a specified      *
*  right shift, and returns the 64-bit unsigned result.                *
*                                                                      *
*  Synopsis: ui64 mrsuwj(ui32 x, ui32 y, int s)                        *
*                                                                      *
*  N.B.  Program requires 0 <= s < 64.  Multiplication with no shift   *
*  will be slightly faster with jmuw().  Left shift requires using a   *
*  routine with overflow checking (mlsuwj).                            *
*                                                                      *
*  This routine is part of a basic 64-bit arithmetic package for which *
*  full C implementations are provided in order to get one started on  *
*  a new processor.  These versions can be reimplemented in Assembler  *
*  if greater speed is needed.                                         *
************************************************************************
*  V1A, 09/26/14, GNR - New routine, based on msswj and msul           *
*  ==>, 09/27/14, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include <stdlib.h>
#include <math.h>
#include "sysdef.h"
#include "rkarith.h"

ui64 mrsuwj(ui32 x, ui32 y, int s) {

#ifdef HAS_I64
#error mrsuwj() is a macro with HAS_I64, mrsuwj.c should not be compiled

#else          /* Not HAS_I64 */
#define LU16 0x0000ffffUL

   ui64 rv;
   ui32 p0,p1,p2,q1,q2;
   ui32 x0,x1,y0,y1;
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

/* Perform the shift.  64-bit result cannot overflow. */

   ds = s - BITSPERUI32;
   if (ds >= 0) {
      rv.hi = 0;
      rv.lo = p2 >> ds;
      }
   else if (s > 0) {
      rv.hi = p2 >> s;
      rv.lo = (p0 >> s) | (p2 << (-ds));
      }
   else {
      /* We need this case because a shift of 32 is a NOP */
      rv.hi = p2;
      rv.lo = p0;
      }

   return rv;
#endif

   } /* End mrsuwj() */
