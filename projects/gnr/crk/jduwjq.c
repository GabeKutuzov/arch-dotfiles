/* (c) Copyright 2016, The Rockefeller University *11115* */
/* $Id: jduwjq.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                               jduwjq                                 *
*                                                                      *
*  This function divides an unsigned 64-bit integer by an unsigned     *
*  32-bit divisor and returns the 64-bit quotient.  There are no       *
*  overflow conditions.  See jduwb() to get also the remainder.        *
*                                                                      *
*  Synopsis: ui64 jduwjq(ui64 x, ui32 y)                               *
*                                                                      *
*  By definition, the code returns quotient = 0 if the divisor is 0.   *
*  This eliminates need for zero checking in the caller, for example,  *
*  if an average statistic is being calculated.   If 0 divisor is an   *
*  error, caller needs to check for it.                                *
*                                                                      *
*  Note:  This routine is implemented by a macro if system HAS_I64.    *
*                                                                      *
*                                                                      *
*  This routine is part of a basic 64-bit arithmetic package for which *
*  full C implementations are provided in order to get one started on  *
*  a new processor.  These routines or the combined routines actually  *
*  used in applications can be reimplemented in Assembler if greater   *
*  speed is desired.                                                   *
************************************************************************
*  R60, 05/15/16, GNR - New routine, based on jwuwb                    *
*  ==>, 05/15/16, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include "sysdef.h"
#include "rkarith.h"

ui64 jduwjq(ui64 x, ui32 y) {

#ifdef HAS_I64
#error Routine jduwjq should not be compiled if system HAS_I64.
#else   /* NO_I64 */
   ui64 rv;                   /* Will hold result */

   if (y == 0) {              /* Return 0 if divisor == 0 */
      rv.hi = rv.lo = 0; return rv; }

/* If divisor y is <= 2^16, we can use machine division
*  to get the answer much more quickly.  */

   if (y <= (UI16_MAX+1)) {
#define BPS BITSPERSHORT
      ui32 d1 = x.hi % y << BPS | x.lo >> BPS;
      ui32 d2 = d1 % y << BPS | x.lo & UI16_MAX;
      rv.hi = x.hi/y;
      rv.lo = d1/y << BPS | d2/y;
      return rv;
      }

/* If divisor y is larger than 2^16, it seems just as well to perform
*  classical long division.  Added test to skip half the divide cycles
*  in the perhaps common case that the high-order dividend is 0.  */

   else {
#define S2U (BITSPERUI32-1)   /* Shift high bit to units bit */
      ui32 dc, d0 = 0;
      int  ct;                /* Division cycle counter */

      /* Build a 96 bit dividend register */
      if (x.hi == 0) rv.hi = x.lo, rv.lo =    0, ct = BITSPERUI32/2;
      else           rv.hi = x.hi, rv.lo = x.lo, ct = BITSPERUI32;

      /* Perform long division.  Two divide steps are done in each
      *  loop iteration in order to reduce loop overhead.  Each time
      *  the dividend is shifted left by addition, another quotient
      *  bit is brought in at the right.  */
      while (ct--) {
         dc = d0 >> S2U;               /* One division cycle */
         d0 += d0 + (rv.hi >> S2U);
         rv.hi += rv.hi + (rv.lo >> S2U);
         rv.lo += rv.lo;
         if (dc | d0 >= y) d0 -= y, rv.lo += 1;
         dc = d0 >> S2U;               /* Second division cycle */
         d0 += d0 + (rv.hi >> S2U);
         rv.hi += rv.hi + (rv.lo >> S2U);
         rv.lo += rv.lo;
         if (dc | d0 >= y) d0 -= y, rv.lo += 1;
         }

      return rv;
      } /* End long division method */
#endif

   } /* End jduwjq() */
