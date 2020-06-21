/* (c) Copyright 2012-2013, The Rockefeller University *11115* */
/* $Id: dsruwqe.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                               dsruwqe                                *
*                                                                      *
*  This function performs scaled and rounded division of an unsigned   *
*  64-bit dividend by an unsigned ui32 divisor.  It first scales the   *
*  dividend by a specified shift amount, then performs rounding by     *
*  adding divisor/2 to the dividend.  Then it performs the division    *
*  and returns the quotient.                                           *
*                                                                      *
*  This routine reports errors via e64act() except divide by 0 is a    *
*  terminal error.  Use dsruwq to make all errors terminal.  There     *
*  is no version to return the remainder, which is of no interest      *
*  after rounding has been applied.                                    *
*                                                                      *
*  Synopsis: ui32 dsruwqe(ui64 x, int s, ui32 y, int ec)               *
*                                                                      *
*  This routine is part of a basic 64-bit arithmetic package for which *
*  full C implementations are provided in order to get one started on  *
*  a new processor.  These routines or the combined routines actually  *
*  used in applications can be reimplemented in Assembler if greater   *
*  speed is desired.                                                   *
*                                                                      *
************************************************************************
*  V1A, 11/30/12, GNR - New routine, based on dsruwq()                 *
*  ==>, 11/30/12, GNR - Last date before committing to svn repository  *
*  Rev, 06/09/13, GNR - Deal with return from abexit in tests          *
*  Rev, 10/25/13, GNR - Eliminate 'L' constants                        *
***********************************************************************/

#include <math.h>
#include <stdlib.h>
#include "sysdef.h"
#include "rkarith.h"

ui32 dsruwqe(ui64 x, int s, ui32 y, int ec) {

#ifdef HAS_I64
   ui64 yw;

   /* Error if attempt to divide by zero */
   if (y == 0) { abexitmq(72,"Divide by zero."); return 0; }

   /* Scale.  Note:  If the scaling would overflow, then the
   *  division also must overflow, no need to perform it.  */
   if (s > 0) {
      if (x >> (64-s)) goto ReportOvflow;
      x <<= s;
      }
   else
      x >>= -s;

   /* Round */
   yw = y >> 1;
   if (~yw < x) goto ReportOvflow;
   x += yw;

   /* Divide */
   yw = (ui64)y;
   if (x >= yw<<32) goto ReportOvflow;
   return (ui32)(x/yw);

# else               /* System does not have I64 */
   ui32 c, dh, dl;
   int ct;

   /* Error if attempt to divide by zero */
   if (y == 0) { abexitmq(72,"Divide by zero."); return 0; }

/* Get components of dividend */

   dh = x.hi;
   dl = x.lo;

/* Perform the requested shift.  We cannot use the trick from
*  the signed case of shifting one more than the request,
*  because the test for divide check must be done just before
*  the last shift.
*
*  N.B.  If performing no shift at all, it is necessary to
*  avoid the (s > 0) code because on some machines, a shift
*  of 32 as used in that code is a NOP.  */

   if (s > 0) {
      /* Perform positive (left) shift */
      int ds = s - BITSPERUI32;
      if (ds >= 0) {          /* Shift is more than one full word */
         if (dh > (ui32)0 || dl >> (64-s)) goto ReportOvflow;
         dh = dl << ds;
         dl = 0;
         }
      else {                  /* Shift is less than one full word */
         ds = -ds;
         if (dh >> ds) goto ReportOvflow;
         dh = (dh << s) | (dl >> ds);
         dl = dl << s;
         }
      } /* End case shift > 0 */
   else if (s < 0) {
      /* Perform negative (right) shift */
      int ds;
      s = -s;
      ds = s - BITSPERUI32;
      if (ds >= 0) {
         dl = dh >> ds;
         dh = 0;
         }
      else {
         dl = (dl >> s) | (dh << (-ds));
         dh = dh >> s;
         }
      } /* End preliminary shift */

/* Now add the half the divisor in order to round the quotient.
*  A further overflow is possible at this point.  */

   c = y >> 1;
   if (~c < dl && (dh += 1) == 0) goto ReportOvflow;
   dl += c;

/* If the high order word of the dividend is now already
*  larger than the divisor, report a divide check.  */

   if (dh >= y) goto ReportOvflow;

/* Position the dividend for the first division step
*  and extract the first quotient bit */

   c   = dh >> 31;
   dh += dh + (dl >> 31);
   dl += dl;

/* Perform long division.  Two divide steps are done in each loop
*  iteration in order to reduce loop overhead.  Each time dividend
*  is shifted left by addition, another quotient bit is brought in
*  at the right.  */

   for (ct=16; ct>0; --ct) {
      if (c || dh >= y) {        /* One division cycle */
         dh -= y;
         c   = dh >> 31;
         dh += dh + (dl >> 31);
         dl += dl + 1; }
      else {
         c   = dh >> 31;
         dh += dh + (dl >> 31);
         dl += dl; }
      if (c || dh >= y) {        /* Second division cycle */
         dh -= y;
         c   = dh >> 31;
         dh += dh + (dl >> 31);
         dl += dl + 1; }
      else {
         c   = dh >> 31;
         dh += dh + (dl >> 31);
         dl += dl; }
      }

/* Return the quotient to the caller */

   return dl;
#endif

ReportOvflow:
   e64act("dsruwqe", ec);
   return UI32_MAX;
   } /* End dsruwqe() */
