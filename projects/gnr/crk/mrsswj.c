/* (c) Copyright 2014, The Rockefeller University *11115* */
/* $Id: mrsswj.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                               mrsswj                                 *
*                                                                      *
*  This function emulates the way fixed point multiplication should    *
*  be implemented in C--multiply two 32-bit numbers to get a 64-bit    *
*  result--then performs a right shift.  There is no possibility of    *
*  overflow.                                                           *
*                                                                      *
*  mrsswj multiplies a 32-bit signed integer by a 32-bit signed        *
*  integer, scales the 62-bit intermediate product by a specified      *
*  right shift, and returns the 64-bit signed result.                  *
*                                                                      *
*  Synopsis: si64 mrsswj(si32 x, si32 y, int s)                        *
*                                                                      *
*  N.B.  Program requires 0 <= s < 64.  Multiplication with no shift   *
*  will be slightly faster with jmsw().  Left shift requires using a   *
*  routine with overflow checking (mlsswj).  Values of 'x' or 'y'      *
*  equal to the most negative number are allowed.                      *
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
*  V1A, 09/05/14, GNR - New routine, based on mssl()                   *
*  ==>, 09/17/14, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include <stdlib.h>
#include <math.h>
#include "sysdef.h"
#include "rkarith.h"

si64 mrsswj(si32 x, si32 y, int s) {

#ifdef HAS_I64
#error mrsswj() is a macro with HAS_I64, mrsswj.c should not be compiled

#else          /* Not HAS_I64 */
#ifdef L16
#undef L16
#endif
#define L16 0x0000ffffUL

   si64 rv;
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

/* Perform the shift.  64-bit result cannot overflow. */

   ds = s - BITSPERUI32;
   if (ds >= 0) {
      rv.hi = SRA(p2.ps, (BITSPERUI32-1));
      rv.lo = (ui32)(SRA(p2.ps, ds));
      }
   else if (s > 0) {
      rv.hi = SRA(p2.ps, s);
      rv.lo = (p0.pu >> s) | (p2.pu << (-ds));
      }
   else {
      /* We need this case because a shift of 32 is a NOP */
      rv.hi = p2.ps;
      rv.lo = p0.pu;
      }

   return rv;
#endif

   } /* End mrsswj() */
