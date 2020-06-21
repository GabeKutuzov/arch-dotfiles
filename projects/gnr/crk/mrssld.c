/* (c) Copyright 2013-2014, The Rockefeller University *11115* */
/* $Id: mrssld.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                               mrssld                                 *
*                                                                      *
*  mrssld multiplies a 32-bit signed integer by a 32-bit signed        *
*  integer, scales the 62-bit intermediate product by a specified      *
*  right shift, and returns the low-order 32 bits of the result,       *
*  reporting an overflow if the result exceeds 32 bits.  This is       *
*  the version that uses the error code stored by e64dec().            *
*                                                                      *
*  This function is a replacement for jm64sh and jm64sl with error     *
*  checking.  When there is a left shift, use mlssw[de] or mlsswj[de]. *
*                                                                      *
*  Synopsis: si32 mrssld(si32 x, si32 y, int s)                        *
*                                                                      *
*  N.B.  Program requires 0 <= s < 64.                                 *
*                                                                      *
*  N.B.  Values of 'x' or 'y' equal to the most negative number are    *
*  allowed--they can be handled if the shift brings the results back   *
*  into the representable range.                                       *
*                                                                      *
*  N.B.  When replacing jm64sh with this routine, replace the jm64sh   *
*  left shift with a right shift of s - 32.                            *
*                                                                      *
*  N.B.  Shifting followed by negation of odd products gives an answer *
*  one bit different than SRA-type shifting of negatives, although one *
*  is hard-pressed to say which is "correct", e.g. SRA(-3,1) yields -2,*
*  not -1.  Use mrsrsle to improve accuracy by rounding. The code does *
*  assure that results are same for HAS_I64 and 32-bit compilations.   *
*                                                                      *
*  This routine is part of a basic 64-bit arithmetic package for which *
*  full C implementations are provided in order to get one started on  *
*  a new processor.  These versions can be reimplemented in Assembler  *
*  if greater speed is needed.                                         *
************************************************************************
*  V1A, 10/23/13, GNR - New routine, based on jm64sh and jmsw          *
*  ==>, 10/23/13, GNR - Last date before committing to svn repository  *
*  Rev, 09/13/14, GNR - Correct to permit shifts in range 32 - 63 and  *
*                       same neg results as SRA w/ or w/o 64-bit arith.*
***********************************************************************/

#include <stdlib.h>
#include <math.h>
#include "sysdef.h"
#include "rkarith.h"

si32 mrssld(si32 x, si32 y, int s) {

#ifdef HAS_I64
#if LSIZE == 8
#define LS32 0x000000007fffffffL
#define LM32 0xffffffff80000000L
#else
#define LS32 0x000000007fffffffLL
#define LM32 0xffffffff80000000LL
#endif

   si64 prod;                 /* Product */

   prod = (si64)x * (si64)y;  /* SRA may access prod twice */
   prod = SRA(prod, s);
   /* prod == -2^31 should be rare, so use compound 'if' test */
   if (abs64(prod) > LS32 && prod != LM32) {
      e64dac("mrssld");
      return (prod >= 0 ? SI32_MAX : SI32_SGN);
      }
   return (si32)prod;

#else          /* Not HAS_I64 */
#ifdef L16
#undef L16
#endif
#define L16 0x0000ffffUL

   union uus { ui32 pu; si32 ps; } p0,p1,p2;
   ui32 x0,x1,y0,y1;
   ui32 ax = abs32(x);
   ui32 ay = abs32(y);
   int ds;

/* Multiply (x1*(2**16) + x0) by (y1*(2**16) + y0),
*  where x1 and y1 are 15-bit quantities, x0 and y0 are 16-bit.
*  High 32 bits of result =
*        x1*y1 + <high order 16 bits of (x1*y0 + x0*y1)>
*  Low 32 bits of result =
*        x0*y0 + <low order 16 bits of (x1*y0 + x0*y1)>
*  Note that (x1*y0 + x0*y1) cannot overflow beyond 32 bits
*        because max value is 2**32 - 2**17 - 2**16 + 2. */

   x1 = ax >> BITSPERSHORT;
   x0 = ax & L16;
   y1 = ay >> BITSPERSHORT;
   y0 = ay & L16;

   p0.pu = x0 * y0;
   p1.pu = x0 * y1 + x1 * y0;
   ay = p1.pu << BITSPERSHORT;
   p2.pu = (x1 * y1) + (p1.pu >> BITSPERSHORT) + (~p0.pu < ay);
   p0.pu += ay;

/* Perform the complementation.  N.B.  In order to guarantee that
*  the shift will produce the same result as an SRA (as used in the
*  64-bit macro) for odd negative results, it is necessary to com-
*  plement before shifting (or do a complicated rounding maneuver).
*  It is OK to complement a zero result, because it remains zero.  */

   if ((x ^ y) < 0)
      p0.pu = ~p0.pu + 1, p2.pu = ~p2.pu + (p0.pu == 0);

/* Perform the shift */

   if ((ds = s - BITSPERUI32) >= 0)
      /* If s >= 32, overflow is not possible */
      return SRA(p2.ps, ds);
   /* With s < 32, it is necessary to test for overflow.  There
   *  is an overflow if any of the bits in p2 are not equal to the
   *  sign bit of p0 (valid for signed results).  */
   else {
      if (s > 0) {
         p0.pu = (p0.pu >> s) | (p2.pu << (-ds));
         p2.ps = SRA(p2.ps, s);
         }
      if (p2.pu + (p0.pu >> (BITSPERUI32-1))) {
         e64dac("mrssld");
         return (p2.ps >= 0 ? SI32_MAX : SI32_SGN);
         }
      return p0.ps;
      }
#endif

   } /* End mrssld() */
