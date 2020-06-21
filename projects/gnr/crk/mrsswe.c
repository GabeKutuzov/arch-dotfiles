/* (c) Copyright 2003-2014, The Rockefeller University *11115* */
/* $Id: mrsswe.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                               mrsswe                                 *
*                                                                      *
*  This function multiplies a 64-bit signed integer by a 32-bit        *
*  signed integer, scales the 94-bit intermediate product by a         *
*  specified right shift, and returns the signed 64-bit result with    *
*  overflow checking.  To keep a 94-bit product, the multiplication    *
*  has to be carried out in pieces even if the machine HAS_I64.        *
*  This is the version that uses the error code passed as an argument. *
*                                                                      *
*  Synopsis: si64 mrsswe(si64 x, si32 y, int s, int ec)                *
*                                                                      *
*  N.B.  Program assumes 0 <= s < 64 without checking.  Values of      *
*  'x' or 'y' equal to the most negative number are not necessarily    *
*  errors--they can be handled if 's' is large enough to bring the     *
*  answer back into the representable range.  The result of taking     *
*  the absolute value of such a number in C is undefined, but this     *
*  code simply assumes negation leaves these numbers unchanged.        *
*                                                                      *
*  N.B.  Shifting followed by negation of odd products gives an answer *
*  one bit different than SRA-type shifting of negatives, although one *
*  is hard-pressed to say which is "correct", e.g. SRA(-3,1) yields    *
*  -2, not -1.  Use mrsrswd to improve accuracy by rounding. This code *
*  has now been modified to perform negation before shifting, so the   *
*  results are the same in all cases as if a full SRA had been done.   *
*                                                                      *
*  This routine is part of a basic 64-bit arithmetic package for which *
*  full C implementations are provided in order to get one started on  *
*  a new processor.  These versions can be reimplemented in Assembler  *
*  if greater speed is needed.                                         *
*                                                                      *
************************************************************************
*  V1A, 04/13/03, GNR - New routine                                    *
*  ==>, 09/28/08, GNR - Last date before committing to svn repository  *
*  Rev, 11/19/08, GNR - Correct overflow adding partial products       *
*  V2A, 11/22/08, GNR - Keep 96-bit intermediate product               *
*  Rev, 12/26/08, GNR - Bug fix: False overflow if neg ans shifts to 0 *
*  Rev, 04/06/14, GNR - More careful handling of low-order sign bit    *
*  Rev, 09/17/14, GNR - Same neg results as SRA w/ or w/o 64-bit arith.*
***********************************************************************/

#include <stdlib.h>
#include <math.h>
#include "sysdef.h"
#include "rkarith.h"

si64 mrsswe(si64 x, si32 y, int s, int ec) {

#ifdef HAS_I64
#if LSIZE == 8
#define LU32 0x00000000ffffffffUL
#else
#define LU32 0x00000000ffffffffULL
#endif

   union uus { ui64 pu; si64 ps; } p0,p1;
   ui64 ax = abs64(x);
   ui32 ay = abs32(y);
   int  ds, sr;

/* Multiply x * y in base 2^32 to get a 96-bit product.  Largest
*  possible result is 2^63 * 2^31 == 2^94.  This product cannot
*  overflow.  N.B.  p0(hi) is left as junk here.  */

   p0.pu = (ax & LU32)*ay;
   p1.pu = (ax >> BITSPERUI32)*ay + (p0.pu >> BITSPERUI32);

/* Perform the complementation.  N.B.  In order to guarantee that
*  the shift will produce the same result as an SRA for odd negative
*  results, it is necessary to complement before shifting (or do a
*  complicated rounding maneuver).  It is OK to complement a zero
*  result, because it remains zero.  */

   if ((sr = (x < 0)^(y < 0)))      /* Assignment intended */
      p0.pu = ~p0.pu + 1, p1.pu = ~p1.pu + ((p0.pu & LU32) == 0);

/* Scale according to argument s  */

   if ((ds = s - BITSPERUI32) >= 0) {
      /* If shift is >= 32, result cannot overflow */
      return SRA(p1.ps, ds);
      }
   /* With s < 32, it is necessary to test for overflow.  There is
   *  an overflow if any of the bits in p1 are not equal to the sign
   *  bit of p0 (valid for signed results).  N.B.  We will assume
   *  (and test) that in 64-bit machines a shift of 32 is executed,
   *  unlike the case in 32-bit machines (below) where a 0 shift must
   *  be handled separately because -ds is then 32.  */
   else {
      p0.pu = (p0.pu & LU32) >> s | (p1.pu << (-ds));
      p1.ps = SRA(p1.ps, (s+BITSPERUI32));
      if (p1.pu + (ax = p0.pu >> (BITSPERUI64-1))) {
         e64act("mrsswe", ec);
         return SI64_MAX + sr;
         }
      return p0.ps;
      }

#else             /* Not HAS_I64 */
#ifdef LU16
#undef LU16
#endif
#define LU16 (ui32)0x0000ffff

   si64 rv;
   union uus { ui32 pu; si32 ps; } p0,p2,p4;
   ui32 x0,x1,x2,x3,y0,y1;
   ui32 p1,p3,q1,q2,q3,q4;
   si32 xhi = swhi(x);
   ui32 axh,xlo = swlou(x),ay;
   int ds, sr = 0;

/* Get absolute values of x and y, store sign of result in sr */

   if (xhi < 0) {
      xlo = ~xlo + 1;
      axh = ~xhi + (xlo == 0);
      sr = 1;
      }
   else
      axh = xhi;
   ay = y < 0 ? (sr ^= 1, -y) : y;

/* Break x into 4 and y into 2 16-bit pieces */

   x3 = axh >> BITSPERSHORT;
   x2 = axh & LU16;
   x1 = xlo >> BITSPERSHORT;
   x0 = xlo & LU16;

   y1 = ay >> BITSPERSHORT;
   y0 = ay & LU16;

/* Multiply (x3,x2,x1,x0) * (y1,y0) in base 2^16 and hold result in
*  five ui32 words (p4,p3,p2,p1,p0), where p3(hi)-p0(hi) are carries
*  and p4 is entire high 32-bits of the result, which cannot overflow.
*  Be careful to avoid overflow when adding partial products.  */

   p0.pu = x0*y0;
   p1    = x1*y0 + (p0.pu>>BITSPERSHORT);
   p2.pu = x2*y0 + (p1>>BITSPERSHORT);
   p3    = x3*y0 + (p2.pu>>BITSPERSHORT);
   p4.pu =         (p3>>BITSPERSHORT);

   q1 = x0*y1;
   q2 = x1*y1 + (q1>>BITSPERSHORT);
   q3 = x2*y1 + (q2>>BITSPERSHORT);
   q4 = x3*y1 + (q3>>BITSPERSHORT);

   p1 = (p1 & LU16) + (q1 & LU16);
   p2.pu = (p2.pu & LU16) + (q2 & LU16) + (p1>>BITSPERSHORT);
   p3 = (p3 & LU16) + (q3 & LU16) + (p2.pu>>BITSPERSHORT);
   p4.pu = p4.pu + q4 + (p3>>BITSPERSHORT);

/* Combine odd halves in p1,p3 into p0,p2 for shifting */

   p0.pu = (p0.pu & LU16) | (p1<<BITSPERSHORT);
   p2.pu = (p2.pu & LU16) | (p3<<BITSPERSHORT);

/* Perform the complementation.  N.B.  In order to guarantee that
*  the shift will produce the same result as an SRA for odd negative
*  results, it is necessary to complement before shifting (or do a
*  complicated rounding maneuver).  It is OK to complement a zero
*  result, because it remains zero.  */

   if (sr) {
      p0.pu = ~p0.pu + 1;
      p2.pu = ~p2.pu + (p0.pu == 0);
      p4.pu = ~p4.pu + (p2.pu == 0);
      }

/* Scale according to argument s.  Be careful never to do a shift of
*  32, because it is a NOP on some machines (e.g.  Intel 486 and up).
*/

   ds = s - BITSPERUI32;
   if (ds >= 0) {
      /* If shift is >= 32, result cannot overflow */
      if (ds == 0) {
         rv.lo = p2.pu;
         rv.hi = p4.ps;
         }
      else {
         rv.lo = p2.pu >> ds | p4.pu << (BITSPERUI32 - ds);
         rv.hi = SRA(p4.ps, ds);
         }
      }
   else {
      if (s == 0) {
         rv.lo = p0.pu;
         rv.hi = p2.ps;
         }
      else {
         ds = -ds;
         rv.lo = p2.pu << ds | p0.pu >> s;
         rv.hi = (si32)(p4.pu << ds | p2.pu >> s);
         p4.ps = SRA(p4.ps, s);
         }
      /* After shifting, if p4 and the sign of rv.hi are not either
      *  all zeros or all ones, there has been an overflow.  This
      *  test specifically accepts the largest negative number.  */
      if (p4.ps + ((ui32)rv.hi >> (BITSPERUI32-1))) {
         e64act("mrsswe", ec);
         if (sr) rv.lo = 0, rv.hi = ~SI32_MAX;
         else    rv.lo = UI32_MAX, rv.hi = SI32_MAX;
         }
      } /* End else shift <= 32 */

   return rv;

#endif

   } /* End mrsswe() */
