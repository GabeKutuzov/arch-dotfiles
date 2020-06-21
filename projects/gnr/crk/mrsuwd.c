/* (c) Copyright 2013, The Rockefeller University *11115* */
/* $Id: mrsuwd.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                               mrsuwd                                 *
*                                                                      *
*  This function multiplies a 64-bit unsigned integer by a 32-bit      *
*  unsigned integer, scales the 96-bit intermediate product by a       *
*  specified right shift, and returns the unsigned 64-bit result with  *
*  overflow checking.  To keep a 96-bit product, the multiplication    *
*  has to be carried out in pieces even if the machine HAS_I64.        *
*  This is the version that uses the error code stored by e64dec().    *
*                                                                      *
*  Synopsis: ui64 mrsuwd(ui64 x, ui32 y, int s)                        *
*                                                                      *
*  N.B.  Program assumes 0 <= s < 64 without checking.                 *
*                                                                      *
*  This routine is part of a basic 64-bit arithmetic package for which *
*  full C implementations are provided in order to get one started on  *
*  a new processor.  These versions can be reimplemented in Assembler  *
*  if greater speed is needed.                                         *
*                                                                      *
************************************************************************
*  V1A, 10/23/13, GNR - New routine                                    *
*  ==>, 10/23/13, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include <math.h>
#include "sysdef.h"
#include "rkarith.h"

ui64 mrsuwd(ui64 x, ui32 y, int s) {

#ifdef HAS_I64
#if LSIZE == 8
#define LU32 0x00000000ffffffffUL
#else
#define LU32 0x00000000ffffffffULL
#endif

   ui64 p0,p1;
   int ds;

/* Multiply x * y in base 2^32 to get a 96-bit product.
*  This product cannot overflow.  */

   p0 = (x & LU32)*y;
   p1 = (x >> BITSPERUI32)*y + (p0 >> BITSPERUI32);

/* Scale according to argument s.
*  N.B.:  p0(hi) is junk here.  */

   if ((ds = s - BITSPERUI32) >= 0) {
      /* If shift is >= 32, result cannot overflow */
      return (p1 >> ds);
      }
   else if (p1 >> s > LU32) {
      /* Overflow if p1 >> s still has high-order bits */
      e64dac("mrsuwd");
      return UI64_MAX;
      }
   else {
      return (p1 << (-ds) | (p0 & LU32) >> s);
      }

#else                /* Does not have I64 arithmetic */
#define LU16 0x0000ffffUL

   ui64 rv;
   ui32 x0,x1,x2,x3,y0,y1;
   ui32 p0,p1,p2,p3,p4,q1,q2,q3,q4;
   ui32 xhi = uwhi(x);
   ui32 xlo = uwlo(x);
   int ds;

/* Break x into 4 and y into 2 16-bit pieces */

   x3 = xhi >> BITSPERSHORT;
   x2 = xhi & LU16;
   x1 = xlo >> BITSPERSHORT;
   x0 = xlo & LU16;

   y1 = y >> BITSPERSHORT;
   y0 = y & LU16;

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

   p1 = (p1 & LU16) + (q1 & LU16);
   p2 = (p2 & LU16) + (q2 & LU16) + (p1>>BITSPERSHORT);
   p3 = (p3 & LU16) + (q3 & LU16) + (p2>>BITSPERSHORT);
   p4 = p4 + q4 + (p3>>BITSPERSHORT);

/* Get middle 32 bits of result in p2 for shifting.
*  (Low bits are not needed until final case below.)  */

   p2 = (p2 & LU16) | (p3<<BITSPERSHORT);

/* Scale according to argument s.  Be careful never to do a shift of
*  32, because it is a NOP on some machines (e.g.  Intel 486 and up).
*/

   ds = s - BITSPERUI32;
   if (ds >= 0) {
      /* If shift is >= 32, result cannot overflow */
      if (ds == 0) {
         rv.lo = p2;
         rv.hi = p4;
         }
      else {
         rv.lo = p2 >> ds | p4 << (BITSPERUI32 - ds);
         rv.hi = p4 >> ds;
         }
      }
   else if (p4 >> s > 0) {
      /* Overflow if p4 >> s is still nonzero */
      e64dac("mrsuwd");
      rv.lo = rv.hi = UI32_MAX;
      }
   else if (s == 0) {
      rv.lo = (p0 & LU16) | (p1<<BITSPERSHORT);
      rv.hi = p2;
      }
   else {
      ds = -ds;
      p0 = (p0 & LU16) | (p1<<BITSPERSHORT);
      rv.lo = p2 << ds | p0 >> s;
      rv.hi = p4 << ds | p2 >> s;
      }
   return rv;
#endif

   } /* End mrsuwd() */
