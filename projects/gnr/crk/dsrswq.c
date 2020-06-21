/* (c) Copyright 2003-2013, The Rockefeller University *11115* */
/* $Id: dsrswq.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                               dsrswq                                 *
*                                                                      *
*  This function performs scaled and rounded division of a signed      *
*  64-bit dividend by an si32 divisor.  It first scales the dividend   *
*  by a specified shift amount, then performs rounding by adding       *
*  abs(divisor)/2 to abs(dividend).  Then it performs the division,    *
*  applies the correct sign to the quotient, and returns it.           *
*                                                                      *
*  This routine terminates on all errors--call dsrswqe() if overflow   *
*  errors are to be reported via e64act (except divide by 0 is always  *
*  a terminal error).  There is no version to return the remainder,    *
*  because that is of no interest after rounding has been applied.     *
*                                                                      *
*  Synopsis: si32 dsrswq(si64 x, int s, si32 y)                        *
*                                                                      *
*  This routine is part of a basic 64-bit arithmetic package for which *
*  full C implementations are provided in order to get one started on  *
*  a new processor.  These routines or the combined routines actually  *
*  used in applications can be reimplemented in Assembler if greater   *
*  speed is desired.                                                   *
*                                                                      *
************************************************************************
*  V1A, 03/22/03, GNR - New routine                                    *
*  Rev, 11/17/08, GNR - Correct syntax error in ds declaration that    *
*                       only affected acc compilation                  *
*  ==>, 11/17/08, GNR - Last date before committing to svn repository  *
*  Rev, 11/24/08, GNR - Add error checks to HAS_I64 per documentation  *
*  Rev, 11/30/12, GNR - Modify comment to note addition of dsrswqe()   *
*  Rev, 06/09/13, GNR - Deal with return from abexit in tests          *
*  Rev, 10/25/13, GNR - Eliminate 'L' constants                        *
***********************************************************************/

#include <math.h>
#include <stdlib.h>
#include "sysdef.h"
#include "rkarith.h"

si32 dsrswq(si64 x, int s, si32 y) {

#ifdef HAS_I64
   ui64 ux,uy;

   /* Error if attempt to divide by zero */
   if (y == 0) { abexitmq(72,"Divide by zero."); return 0; }

   /* Scale */
   ux = abs64(x);
   if (s > 0) {
      /* Error if scaling would overflow--no need to
      *  test if s == 0, because sign bit is now clear */
      if (ux >> (63-s)) {
         abexitmq(75,"Fixed-point scaling overflow.");
         return 0;
         }
      ux <<= s;
      }
   else
      ux >>= -s;

   /* Round--overflow into sign detected w/divide check */
   uy = (ui64)(abs32(y));
   ux += (uy >> 1);

   /* Divide */
   if (ux >= uy<<31) { abexitmq(73,"Divide check."); return 0; }
   ux /= uy;
   return (si32)(((x ^ (si64)y) < 0) ? -ux : ux);

# else               /* System does not have I64 */
   ui32 dh, dl, div;
   int ct;

   /* Error if attempt to divide by zero */
   if (y == 0) { abexitmq(72,"Divide by zero."); return 0; }

/* Get absolute value of dividend and divisor */

   if (x.hi >= (si32)0) {
      dh = x.hi;
      dl = x.lo; }
   else {
      dh = ~x.hi + (x.lo == 0);
      dl = -x.lo; }
   div = abs32(y);

/* Since we need to double the dividend for the long division
*  algorithm, perform a shift one more to the left than
*  requested.  This also allows us to add the full divisor
*  after the shift, rather than half of it, for rounding.
*
*  N.B.  If performing no shift at all, it is necessary to
*  avoid the (s > 0) code because on some machines, a shift
*  of 32 as used in that code is a NOP.  */

   s += 1;
   if (s > 0) {
      /* Perform positive (left) shift */
      int ds = s - BITSPERUI32;
      if (ds >= 0) {          /* Shift is more than one full word */
         if (dh > (ui32)0 || dl >> (64-s)) {
            abexitmq(75,"Fixed-point scaling overflow.");
            return 0; }
         else {
            dh = dl << ds;
            dl = 0;
            }
         }
      else {                  /* Shift is less than one full word */
         ds = -ds;
         if (dh >> ds) {
            abexitmq(75,"Fixed-point scaling overflow.");
            return 0; }
         else {
            dh = (dh << s) | (dl >> ds);
            dl = dl << s;
            }
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

/* Now add the divisor in order to round the quotient */

   dh += (~div < dl);
   dl += div;

/* If the high order word of the dividend is now larger
*  than the divisor, report an overflow.  (With signed
*  division, the first quotient bit must be zero.)  */

   if (dh >= div) { abexitmq(73,"Divide check."); return 0; }

/* Perform long division.  Two divide steps are done in each loop
*  iteration in order to reduce loop overhead.  Each time dividend
*  is shifted left by addition, another quotient bit is brought in
*  at the right.  */

   for (ct=16; ct>0; --ct) {
      if (dh >= div) {           /* One division cycle */
         dh -= div;
         dh += dh + (dl >> 31);
         dl += dl + 1; }
      else {
         dh += dh + (dl >> 31);
         dl += dl; }
      if (dh >= div) {           /* Second division cycle */
         dh -= div;
         dh += dh + (dl >> 31);
         dl += dl + 1; }
      else {
         dh += dh + (dl >> 31);
         dl += dl; }
      }

/* Set the sign of the quotient and return it to the caller.  */

   return ((x.hi ^ y) < (si32)0) ? -(si32)dl : (si32)dl;

#endif
   } /* End dsrswq() */
