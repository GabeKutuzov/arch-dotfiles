/* (c) Copyright 2014, The Rockefeller University *11115* */
/* $Id: mlsswje.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                               mlsswje                                *
*                                                                      *
*  This function emulates the way fixed point multiplication should    *
*  be implemented in C--multiply two 32-bit numbers to get a 64-bit    *
*  result--then performs a left shift and checks for overflow.         *
*                                                                      *
*  mlsswje multiplies a 32-bit signed integer by a 32-bit signed inte- *
*  ger, scales the 62-bit intermediate product by a specified left     *
*  shift, checks for overflow, and returns the 64-bit signed result.   *
*  This is the version that uses the error code passed as an argument. *
*                                                                      *
*  Synopsis: si64 mlsswje(si32 x, si32 y, int s, int ec)               *
*                                                                      *
*  N.B.  Program requires 0 <= s < 64.  Multiplication with no shift   *
*  will be slightly faster with jmsw().  Values of 'x' or 'y' equal to *
*  the most negative number are allowed.                               *
*                                                                      *
*  This routine is part of a basic 64-bit arithmetic package for which *
*  full C implementations are provided in order to get one started on  *
*  a new processor.  These versions can be reimplemented in Assembler  *
*  if greater speed is needed.                                         *
************************************************************************
*  V1A, 10/03/14, GNR - New routine, based on msswj()                  *
*  ==>, 10/09/14, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include <stdlib.h>
#include <math.h>
#include "sysdef.h"
#include "rkarith.h"

si64 mlsswje(si32 x, si32 y, int s, int ec) {

#ifdef HAS_I64

   si64 prod = (si64)x * (si64)y;   /* Product */
   si64 ovt = SRA(prod, ((BITSPERUI64-1)-s));   /* Overflow test */
   si64 ovs = SRA(prod, (BITSPERUI64-1)) & SI64_01;
   if (ovt + ovs) {
      e64act("mlsswje", ec);
      return SI64_MAX + ovs;
      }
   return prod << s;

#else          /* Not HAS_I64 */
#define LU16 0x0000ffffUL

   si64 rv;
   union uus { ui32 pu; si32 ps; } p0,p1,p2;
   ui32 x0,x1,y0,y1;
   ui32 ax = abs32(x);
   ui32 ay = abs32(y);
   int ds = s - BITSPERUI32;
   int sr = (x ^ y) < 0;         /* Sign of the result */

/* Multiply (x1*(2**16) + x0) by (y1*(2**16) + y0),
*  where x1 and y1 are 15-bit quantities, x0 and y0 are 16-bit.
*  High 32 bits of result =
*        x1*y1 + <high order 16 bits of (x1*y0 + x0*y1)>
*  Low 32 bits of result =
*        x0*y0 + <low order 16 bits of (x1*y0 + x0*y1)>
*  Note that (x1*y0 + x0*y1) cannot overflow beyond 32 bits
*        because max value is 2**32 - 2**17 - 2**16 + 2. */

   x1 = ax >> BITSPERSHORT;
   x0 = ax & LU16;
   y1 = ay >> BITSPERSHORT;
   y0 = ay & LU16;

   p0.pu = x0 * y0;
   p1.pu = x0 * y1 + x1 * y0;
   ay = p1.pu << BITSPERSHORT;
   p2.pu = (x1 * y1) + (p1.pu >> BITSPERSHORT) + (~p0.pu < ay);
   p0.pu += ay;

/* Doing the complementation of negative results before the shift
*  makes the shift a little trickier, but allows overflow checking
*  to handle products involving the largest negative number.  */

   if (sr)
      p0.pu = ~p0.pu + 1, p2.pu = ~p2.pu + (p0.pu == 0);

/* Because the codes for both the overflow check and the actual shift
*  depend on whether s >= 32, combining them in separate blocks saves
*  doing this test twice.  Use x0,x1 for test variable. */

   if (ds >= 0) {                   /* s >= 32 */
      x0 = SRA(p0.ps, ((BITSPERUI64-1)-s)) + sr;
      x1 = p2.pu + sr;
      if (x0 | x1) {
         e64act("mlsswje", ec);
         if (sr) rv.lo = 0, rv.hi = ~SI32_MAX;
         else    rv.lo = UI32_MAX, rv.hi = SI32_MAX;
         }
      else
         rv.lo = 0, rv.hi = p0.ps << ds;
      }
   else {                           /* s < 32 */
      if (SRA(p2.ps, ((BITSPERUI32-1)-s)) + sr) {
         e64act("mlsswje", ec);
         if (sr) rv.lo = 0, rv.hi = ~SI32_MAX;
         else    rv.lo = UI32_MAX, rv.hi = SI32_MAX;
         }
      else if (s > 0) {
         rv.lo = p0.pu << s;
         rv.hi = (si32)(p2.pu << s | p0.pu >> (-ds));
         }
      else
         rv.lo = p0.pu, rv.hi = p2.ps;
      }

   return rv;
#endif

   } /* End mlsswje() */
