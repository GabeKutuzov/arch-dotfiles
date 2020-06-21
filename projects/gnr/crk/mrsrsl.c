/* (c) Copyright 2018, The Rockefeller University *11115* */
/* $Id: mrsrsl.c 67 2018-05-07 22:08:53Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                               mrsrsl                                 *
*                                                                      *
*  This function adds rounding to mrssl. It multiplies a 32-bit sign-  *
*  ed integer by a 32-bit signed integer, adds 1/2 of what will be the *
*  ones bit after scaling, scales the 62-bit intermediate product by a *
*  specified right shift, and returns the low-order 32 bits of the     *
*  result.  This version assumes that the shift is known to be large   *
*  enough that overflow is impossible, so no overflow checking is done.*
*                                                                      *
*  Synopsis: si32 mrsrsl(si32 x, si32 y, int s)                        *
*                                                                      *
*  N.B.  Program assumes 0 <= s < 64 without checking.  Values of 'x'  *
*  or 'y' equal to the most negative number are allowed--they can be   *
*  handled if 's' is large enough to bring the result back into the    *
*  representable range.                                                *
*                                                                      *
*  N.B.  When replacing jm64sh with this routine, replace the jm64sh   *
*  left shift with a right shift of 32 - s.                            *
*                                                                      *
*  N.B.  Shifting followed by negation of odd products gives an answer *
*  one bit different than SRA-type shifting of negatives, although one *
*  is hard-pressed to say which is "correct", e.g. SRA(-3,1) yields -2,*
*  not -1.  The code does assure that results are same for HAS_I64 and *
*  32-bit compilations with SRA.                                       *
*                                                                      *
*  This routine is part of a basic 64-bit arithmetic package for which *
*  full C implementations are provided in order to get one started on  *
*  a new processor.  These versions can be reimplemented in Assembler  *
*  if greater speed is needed.                                         *
************************************************************************
*  V1A, 02/21/18, GNR - New routine, based on mrsrsle                  *
*  ==>, 02/21/18, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include <stdlib.h>
#include <math.h>
#include "sysdef.h"
#include "rkarith.h"

si32 mrsrsl(si32 x, si32 y, int s) {

#ifdef HAS_I64

   si64 prod = (si64)x * (si64)y;   /* Get product */
   /* If there is no shift, there is no rounding either */
   if (s > 0) {
      prod += SI64_01 << (s-1);
      prod = SRA(prod,s); }
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

/* Perform the round and shift */

   if ((ds = s - BITSPERUI32) >= 0) {
      if (ds == 0)
         /* Shift is exactly 32, add high order bit from p0.pu */
         p0.ps = p2.pu + (p0.pu >> (BITSPERUI32-1));
      else {
         p0.pu = p2.pu + (1UL << (ds-1));
         p0.ps = SRA(p0.ps, ds);
         }
      }

   else {
      /* If there is no shift, there is no rounding either */
      if (s > 0) {
         ay = 1UL << (s-1);
         p2.pu += (~p0.pu < ay);
         p0.pu = ((p0.pu + ay) >> s) | (p2.pu << (-ds));
         }
      }
   return p0.ps;
#endif

   } /* End mrsrsl() */
