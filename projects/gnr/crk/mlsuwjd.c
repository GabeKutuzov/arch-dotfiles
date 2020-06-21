/* (c) Copyright 2014, The Rockefeller University *11115* */
/* $Id: mlsuwjd.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                               mlsuwjd                                *
*                                                                      *
*  This function emulates the way fixed point multiplication should    *
*  be implemented in C--multiply two 32-bit numbers to get a 64-bit    *
*  result--then performs a left shift and checks for overflow.         *
*                                                                      *
*  mlsuwjd multiplies a 32-bit unsigned integer by a 32-bit unsigned   *
*  integer, scales the 63-bit intermediate product by a specified left *
*  shift, checks for overflow, and returns the 64-bit unsigned result. *
*  This is the version that uses the error code stored by e64dec().    *
*                                                                      *
*  Synopsis: ui64 mlsuwjd(ui32 x, ui32 y, int s)                       *
*                                                                      *
*  N.B.  Program requires 0 <= s < 64.  Multiplication with no shift   *
*  will be slightly faster with jmsw().                                *
*                                                                      *
*  This routine is part of a basic 64-bit arithmetic package for which *
*  full C implementations are provided in order to get one started on  *
*  a new processor.  These versions can be reimplemented in Assembler  *
*  if greater speed is needed.                                         *
************************************************************************
*  V1A, 10/04/14, GNR - New routine, based on mlsswjd()                *
*  ==>, 10/04/14, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include <stdlib.h>
#include <math.h>
#include "sysdef.h"
#include "rkarith.h"

ui64 mlsuwjd(ui32 x, ui32 y, int s) {

#ifdef HAS_I64

   ui64 prod = (ui64)x * (ui64)y;            /* Product */
   if (s > 0 && prod >> (BITSPERUI64-s)) {   /* Overflow test */
      e64dac("mlsuwjd");
      return UI64_MAX;
      }
   return prod << s;

#else          /* Not HAS_I64 */
#define LU16 0x0000ffffUL

   ui64 rv;
   ui32 p0,p1,p2,q1,q2;
   ui32 x0,x1,y0,y1;
   int ds = s - BITSPERUI32;

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

/* Because the codes for both the overflow check and the actual shift
*  depend on whether s >= 32, combining them in separate blocks saves
*  doing this test twice.  */

   if (ds >= 0) {                   /* s >= 32 */
      if (ds > 0 ? p0 >> (BITSPERUI32 - ds) | p2 : p2) {
         e64dac("mlsuwjd");
         rv.lo = rv.hi = UI32_MAX;
         }
      else
         rv.lo = 0, rv.hi = p0 << ds;
      }
   else if (s > 0) {                /* s < 32 */
      ds = -ds;
      if (p2 >> ds) {
         e64dac("mlsuwjd");
         rv.lo = rv.hi = UI32_MAX;
         }
      else {
         rv.lo = p0 << s;
         rv.hi = p2 << s | p0 >> ds;
         }
      }
   else                             /* s == 0 */
      rv.lo = p0, rv.hi = p2;

   return rv;
#endif

   } /* End mlsuwjd() */
