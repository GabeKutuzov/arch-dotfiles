/* (c) Copyright 2008, The Rockefeller University *11115* */
/* $Id: mrsruwe.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                               mrsruwe                                *
*                                                                      *
*  This function multiplies a 64-bit unsigned integer by a 32-bit      *
*  unsigned integer to get a 96-bit intermediate product, rounds by    *
*  adding one-half the smallest bit that will be retained in the       *
*  scaling, scales by a specified right shift, and returns the         *
*  unsigned 64-bit result with full overflow checking.                 *
*  This is the version that uses the error code passed as an argument. *
*                                                                      *
*  Synopsis: ui64 mrsruwe(ui64 x, ui32 y, int s, int ec)               *
*                                                                      *
*  Notes:                                                              *
*  (1) Program assumes 0 <= s < 64 without checking.                   *
*  (2) To keep a 94-bit product, the multiplication has to be carried  *
*  out in pieces even if the machine HAS_I64.                          *
*                                                                      *
*  This routine is part of a basic 64-bit arithmetic package for which *
*  full C implementations are provided in order to get one started on  *
*  a new processor.  These versions can be reimplemented in Assembler  *
*  if greater speed is needed.                                         *
*                                                                      *
************************************************************************
*  V1A, 11/26/08, GNR - New routine, based on msuwe                    *
*  ==>, 12/06/08, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include <math.h>
#include "sysdef.h"
#include "rkarith.h"

ui64 mrsruwe(ui64 x, ui32 y, int s, int ec) {

#ifdef HAS_I64
#if LSIZE == 8
#define L32 0x00000000ffffffffUL
#else
#define L32 0x00000000ffffffffULL
#endif

   ui64 p0,p1;
   int ds;

/* Multiply x * y in base 2^32 to get a 96-bit product.
*  This product cannot overflow.  */

   p0 = (x & L32)*y;
   p1 = (x >> BITSPERUI32)*y;

/* Round and scale according to argument s  */

   if ((ds = s - BITSPERUI32) >= 0) {
      /* Combine p0(hi) into p1(low) */
      p1 += (p0 >> BITSPERUI32);
      if (ds == 0) {
         /* Shift is exactly 32, add 2^31 bit from p0.
         *  N.B.:  p0(hi) is junk here.
         *  The sum cannot overflow and there is no shift.  */
         return (p1 + (p0 >> (BITSPERUI32-1) & UI64_01));
         }
      else {
         /* Shift is > 32, can ignore p0 portion.
         *  The rounding sum cannot overflow (because ds < 32).  */
         return ((p1 + (UI64_01 << (ds-1))) >> ds);
         }
      }
   /* Shift is < 32, rounding might carry into p0(hi), so
   *  round before combining p1,p0 and checking overflow.  */
   p0 += UI64_01 << (s-1);
   p1 += (p0 >> BITSPERUI32);
   if (p1 >> s > L32) {
      /* Overflow if p1 >> s still has high-order bits */
      e64act("mrsruwe", ec);
      return UI64_MAX;
      }
   else {
      return (p1 << (-ds) | (p0 & L32) >> s);
      }

#else                /* Does not have I64 arithmetic */
#ifdef L16
#undef L16
#endif
#define L16 0x0000ffffUL

   ui64 rv;
   ui32 x0,x1,x2,x3,y0,y1;
   ui32 p0,p1,p2,p3,p4,q1,q2,q3,q4;
   ui32 xhi = uwhi(x);
   ui32 xlo = uwlo(x);
   int ds;

/* Break x into 4 and y into 2 16-bit pieces */

   x3 = xhi >> BITSPERSHORT;
   x2 = xhi & L16;
   x1 = xlo >> BITSPERSHORT;
   x0 = xlo & L16;

   y1 = y >> BITSPERSHORT;
   y0 = y & L16;

/* Multiply (x3,x2,x1,x0) * (y1,y0) in base 2^16 and hold result in
*  five ui32 words (p4,p3,p2,p1,p0), where p3(hi)-p0(hi) are carries
*  and p4 is entire high 32-bits of the result, which cannot overflow.
*  Be careful to avoid overflow when adding partial products.  */

   p0 = x0*y0;
   p1 = x1*y0 + (p0>>BITSPERSHORT);
   p2 = x2*y0 + (p1>>BITSPERSHORT);
   p3 = x3*y0 + (p2>>BITSPERSHORT);
   p4 =         (p3>>BITSPERSHORT);

   q1 = x0*y1;
   q2 = x1*y1 + (q1>>BITSPERSHORT);
   q3 = x2*y1 + (q2>>BITSPERSHORT);
   q4 = x3*y1 + (q3>>BITSPERSHORT);

   p1 = (p1 & L16) + (q1 & L16);
   p2 = (p2 & L16) + (q2 & L16) + (p1>>BITSPERSHORT);
   p3 = (p3 & L16) + (q3 & L16) + (p2>>BITSPERSHORT);
   p4 = p4 + q4 + (p3>>BITSPERSHORT);

/* Get middle 32 bits of result in p2 for shifting.
*  (Low bits are not needed until final case below.)  */

   p2 = (p2 & L16) | (p3<<BITSPERSHORT);

/* Round and scale according to argument s.  Be careful never to do a
*  shift of 32, because it is a NOP on some machines (e.g. Intel 486
*  and up).  */

   if ((ds = s - BITSPERUI32) >= 0) {
      if (ds == 0) {
         /* Shift is exactly 32, add 2^15 bit from p1.  The sum
         *  can carry, but not overflow, and there is no shift.  */
         rv.lo = p2 + (p1 >> (BITSPERSHORT-1) & 1UL);
         rv.hi = p4 + ((p2 & ~rv.lo) >> (BITSPERUI32-1));
         }
      else {
         /* Shift is > 32, can ignore p1,p0 portion.
         *  The rounding sum cannot overflow (because ds < 32).  */
         rv.lo = p2 + (1UL << (ds-1));
         p4 += (p2 & ~rv.lo) >> (BITSPERUI32-1);
         rv.lo = rv.lo >> ds | p4 << (BITSPERUI32 - ds);
         rv.hi = p4 >> ds;
         }
      }
   else if (s == 0) {
      /* Shift is 0.  There is no rounding to be done.  */
      if (p4 > 0) goto GotOverflow;
      rv.lo = (p0 & L16) | (p1<<BITSPERSHORT);
      rv.hi = p2;
      }
   else {
      /* 0 < shift < 32.  Rounding cannot overflow p4, but can carry
      *  all the way into p4 (e.g. if product was (2^64-1)*1), so it
      *  is necessary to add rounding amount first, then check for
      *  whether p4 will be nonzero after scaling.  Use p1 and p3 as
      *  temps to hold carries.  */
      ds = -ds;
      p1 = (p0 & L16) | (p1<<BITSPERSHORT);
      p0 = p1 + (1UL << (s-1));
      p3 = p2;
      p2 += (p1 & ~p0) >> (BITSPERUI32-1);
      p4 += (p3 & ~p2) >> (BITSPERUI32-1);
      if (p4 >> s > 0) goto GotOverflow;
      rv.lo = p2 << ds | p0 >> s;
      rv.hi = p4 << ds | p2 >> s;
      }
   return rv;

GotOverflow:
   e64act("mrsruwe", ec);
   rv.lo = rv.hi = UI32_MAX;
   return rv;

#endif

   } /* End mrsruwe() */
