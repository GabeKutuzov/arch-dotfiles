/* (c) Copyright 2012-2013, The Rockefeller University *11115* */
/* $Id: jdswbe.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                               jdswbe                                 *
*                                                                      *
*  This function divides a signed 64-bit integer by an si32 divisor    *
*  and returns the quotient.  The remainder is also returned, via a    *
*  pointer in the argument list.  This routine terminates on divide    *
*  by 0 errors but records overflows via e64act() and returns max q.   *
*                                                                      *
*  Synopsis: si32 jdswbe(si64 x, si32 y, int ec, si32 *pr)             *
*                                                                      *
*  This routine is part of a basic 64-bit arithmetic package for which *
*  full C implementations are provided in order to get one started on  *
*  a new processor.  These routines or the combined routines actually  *
*  used in applications can be reimplemented in Assembler if greater   *
*  speed is desired.                                                   *
*                                                                      *
************************************************************************
*  V1A, 12/01/12, GNR - New routine, based on jdswb                    *
*  ==>, 12/01/12, GNR - Last date before committing to svn repository  *
*  Rev, 06/09/13, GNR - Deal with return from abexit in tests          *
*  Rev, 10/25/13, GNR - Get rid of 'L' constants                       *
***********************************************************************/

#include <math.h>
#include <stdlib.h>
#include "sysdef.h"
#include "rkarith.h"

si32 jdswbe(si64 x, si32 y, int ec, si32 *pr) {

/* Error if attempt to divide by zero */

   if (y == 0) { abexitmq(72,"Divide by zero."); return 0; }

#ifdef HAS_I64
   {  si64 wy = (si64)y;

/* If abs(dividend) >= 2^31 * abs(divisor), report an overflow.
*  (With signed division, the first quotient bit must be zero.)  */

      if (abs64(x) >= abs64(wy)<<(BITSPERUI32-1)) {
         e64act("jdswbe", ec);
         *pr = 0;
         return (x ^ wy) < 0 ? -SI32_MAX : SI32_MAX;
         }

#if LSIZE == 8
      {  ldiv_t qrm = ldiv(x, wy);
         *pr = (si32)qrm.rem;
         return (si32)qrm.quot;
         } /* End qrm local scope */
#else    /* LSIZE == 4 */
      /* lldiv should be available for this case, but is not found
      *  on a RedHat EL5 system even with #define __USE_ISOC99.
      *  Accordingly, we resort to two separate division operations. */
      *pr = (si32)(x % wy);
      return (si32)(x / wy);
#endif
      } /* End wy local scope */

#else    /* !HAS_I64 */
   {  ui32 dh, dl, div;
      int ct;

/* Get absolute value of dividend and divisor */

      if (x.hi >= (si32)0) {
         dh = x.hi;
         dl = x.lo; }
      else {
         dh = ~x.hi + (x.lo == 0);
         dl = -x.lo; }
      div = abs32(y);

/* Do an initial shift to position the dividend
*  for the first division step.  */

      dh += dh + (dl >> (BITSPERUI32-1));
      dl += dl;

/* If the high order word of the dividend is now larger
*  than the divisor, report an overflow.  (With signed
*  division, the first quotient bit must be zero.)  */

      if (dh >= div) {
         e64act("jdswbe", ec);
         *pr = 0;
         return (x.hi ^ y) < 0 ? -SI32_MAX : SI32_MAX;
         }

/* Perform long division.  Two divide steps are done in each loop
*  iteration in order to reduce loop overhead.  Each time dividend
*  is shifted left by addition, another quotient bit is brought in
*  at the right.  */

      for (ct=16; ct>0; --ct) {
         if (dh >= div) {           /* One division cycle */
            dh -= div;
            dh += dh + (dl >> (BITSPERUI32-1));
            dl += dl + 1; }
         else {
            dh += dh + (dl >> (BITSPERUI32-1));
            dl += dl; }
         if (dh >= div) {           /* Second division cycle */
            dh -= div;
            dh += dh + (dl >> (BITSPERUI32-1));
            dl += dl + 1; }
         else {
            dh += dh + (dl >> (BITSPERUI32-1));
            dl += dl; }
         }

/* Undo the extra shift in the remainder.
*  Return it with the original sign of the dividend.  */

      dh = dh >> 1;
      *pr = (x.hi < (si32)0) ? -(si32)dh : (si32)dh;

/* Set the sign of the quotient and return it to the caller.  */

      return ((x.hi ^ y) < (si32)0) ? -(si32)dl : (si32)dl;
      } /* End local scope */

#endif

   } /* End jdswbe() */
