/* (c) Copyright 2008-2014, The Rockefeller University *11115* */
/* $Id: mrsrswe.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                               mrsrswe                                *
*                                                                      *
*  This function multiplies a 64-bit signed integer by a 32-bit        *
*  signed integer to get a 94-bit intermediate product, rounds by      *
*  adding one-half the smallest bit that will be retained in the       *
*  scaling, scales by a specified right shift, and returns the signed  *
*  64-bit result with full overflow checking.                          *
*  This is the version that uses the error code passed as an argument. *
*                                                                      *
*  Synopsis: si64 mrsrswe(si64 x, si32 y, int s, int ec)               *
*                                                                      *
*  Notes:                                                              *
*  (1) Program assumes 0 <= s < 64 without checking.                   *
*  (2) To keep a 94-bit product, the multiplication has to be carried  *
*  out in pieces even if the machine HAS_I64.                          *
*  (3) Shifting followed by negation of odd products gives an answer   *
*  one bit different than SRA-type shifting of negatives.  This code   *
*  has now been modified to perform negation before shifting, so the   *
*  results are the same in all cases as if a full SRA had been done.   *
*  (4) Values of 'x' or 'y' equal to the most negative number are not  *
*  necessarily errors--they can be handled if 's' is large enough to   *
*  bring the answer back into the representable range.  The result of  *
*  taking the absolute value of such a number in C is undefined, but   *
*  this code simply assumes negation leaves these numbers unchanged.   *
*                                                                      *
*  This routine is part of a basic 64-bit arithmetic package for which *
*  full C implementations are provided in order to get one started on  *
*  a new processor.  These versions can be reimplemented in Assembler  *
*  if greater speed is needed.                                         *
*                                                                      *
************************************************************************
*  V1A, 11/26/08, GNR - New routine, based on msswe                    *
*  ==>, 12/07/08, GNR - Last date before committing to svn repository  *
*  Rev, 12/26/08, GNR - Bug fix: False overflow if neg ans shifts to 0 *
*  Rev, 04/06/14, GNR - More careful handling of low-order sign bit    *
*  Rev, 09/28/14, GNR - Same neg results as SRA w/ or w/o 64-bit arith.*
***********************************************************************/

#include <stdlib.h>
#include <math.h>
#include "sysdef.h"
#include "rkarith.h"

si64 mrsrswe(si64 x, si32 y, int s, int ec) {

#ifdef HAS_I64
#if LSIZE == 8
#define LU32 0x00000000ffffffffUL
#define LU01 0x0000000000000001UL
#else
#define LU32 0x00000000ffffffffULL
#define LU01 0x0000000000000001ULL
#endif

   union uus { ui64 pu; si64 ps; } p0,p1;
   ui64 ax = abs64(x);
   ui32 ay = abs32(y);
   int  ds, sr;

/* Multiply x * y in base 2^32 to get a 96-bit product.  Largest
*  possible result is 2^63 * 2^31 == 2^94.  This product cannot
*  overflow.  N.B.  p0(hi32) is left as junk here.  */

   p0.pu = (ax & LU32)*ay;
   p1.pu = (ax >> BITSPERUI32)*ay + (p0.pu >> BITSPERUI32);

/* Perform the complementation.  N.B.  In order to guarantee that
*  the shift will produce the same result as an SRA for odd negative
*  results, it is necessary to complement before shifting (or do a
*  complicated rounding maneuver).  It is OK to complement a zero
*  result, because it remains zero.  */

   if ((sr = (x < 0)^(y < 0)))      /* Assignment intended */
      p0.pu = ~p0.pu + 1, p1.pu = ~p1.pu + ((p0.pu & LU32) == 0);

/* Round and scale according to argument s  */

   if ((ds = s - BITSPERUI32) >= 0) {
      if (ds == 0) {
         /* Shift is exactly 32, add 2^31 bit from p0.
         *  The sum cannot overflow and there is no shift.  */
         return p1.ps + (p0.pu >> (BITSPERUI32-1) & LU01);
         }
      else {
         /* Shift is > 32, can ignore p0 portion.
         *  The rounding sum cannot overflow (because ds < 32).  */
         p1.ps += LU01 << (ds-1);
         return SRA(p1.ps, ds);
         }
      }
   /* Shift is < 32, rounding might carry into p0(hi), so round before
   *  combining p1,p0 and checking overflow.  If shift is 0, there is
   *  no rounding.  There is an overflow if any of the bits in p1 are
   *  not equal to the sign bit of p0 (valid for signed results).  */
   else if (s == 0)                    /* No round, no shift */
      p0.ps = (si32)(p0.pu & LU32 | p1.pu << BITSPERUI32);
   else {
      p0.pu = (p0.pu & LU32) + (LU01 << (s-1));
      p1.ps += p0.pu >> BITSPERUI32;   /* Carry */
      p0.pu = (p0.pu & LU32) >> s | p1.ps << (-ds);
      }
   p1.ps = SRA(p1.ps, (s+BITSPERUI32));
   if (p1.pu + (p0.pu >> (BITSPERUI64-1))) {
      e64act("mrsrswe", ec);
      return SI64_MAX + sr;
      }
   return p0.ps;

#else                   /* Does not have I64 arithmetic */
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

/* Round and scale according to argument s.  Be careful never to do a
*  shift of 32, because it is a NOP on some machines (e.g. Intel 486
*  and up).  */

   if ((ds = s - BITSPERUI32) >= 0) {
      if (ds == 0) {
         /* Shift is exactly 32, add 2^31 bit from p0.  The sum
         *  can carry, but not overflow, and there is no shift.  */
         rv.lo = p2.pu + (p0.pu >> (BITSPERUI32-1));
         rv.hi = p4.ps + (rv.lo == 0);
         }
      else {
         /* Shift is > 32, can ignore p0 portion.
         *  The rounding sum cannot overflow (because ds < 32).  */
         rv.lo = p2.pu + (1UL << (ds-1));
         p4.pu += (p2.pu & ~rv.lo) >> (BITSPERUI32-1);
         rv.lo = rv.lo >> ds | p4.pu << (BITSPERUI32 - ds);
         rv.hi = SRA(p4.ps, ds);
         }
      }
   else {
      if (s == 0)
         /* Shift is 0.  There is no rounding to be done.  */
         rv.lo = p0.pu, rv.hi = p2.ps;
      else {
         /* 0 < shift < 32.  Rounding cannot overflow p4, but can
         *  carry all the way into p4, so it is necessary to add
         *  rounding amount first, then check for overflow after
         *  scaling.  */
         ds = -ds;
         p1 = 1UL << (s-1);      /* Rounding constant */
         p2.pu += (~p0.pu < p1); /* Carry into p2 */
         p4.pu += (p2.pu == 0);  /* Carry into p4 */
         p0.pu = p0.pu + p1;
         rv.lo = p2.pu << ds | p0.pu >> s;
         rv.hi = (si32)(p4.pu << ds | p2.pu >> s);
         p4.ps = SRA(p4.ps, s);
         }
      if (p4.pu + ((ui32)rv.hi >> (BITSPERUI32-1))) {
         e64act("mrsrswe", ec);
         if (sr)
            rv.lo = 0, rv.hi = ~SI32_MAX;
         else
            rv.lo = UI32_MAX, rv.hi = SI32_MAX;
         }
      }

   return rv;

#endif

   } /* End mrsrswe() */
