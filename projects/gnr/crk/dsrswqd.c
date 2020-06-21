/* (c) Copyright 2013, The Rockefeller University *11115* */
/* $Id: dsrswqd.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                               dsrswqd                                *
*                                                                      *
*  This function performs scaled and rounded division of a signed      *
*  64-bit dividend by an si32 divisor.  It first scales the dividend   *
*  by a specified shift amount, then performs rounding by adding       *
*  abs(divisor)/2 to abs(dividend).  Then it performs the division,    *
*  applies the correct sign to the quotient, and returns it.           *
*  This is the version that uses the error code stored by e64dec().    *
*                                                                      *
*  This routine reports errors via e64dac() except divide by 0 is a    *
*  terminal error.  Use dsrswq() to make all errors terminal.  There   *
*  is no version to return the remainder, which is of no interest      *
*  after rounding has been applied.                                    *
*                                                                      *
*  Synopsis: si32 dsrswqd(si64 x, int s, si32 y)                       *
*                                                                      *
*  This routine is part of a basic 64-bit arithmetic package for which *
*  full C implementations are provided in order to get one started on  *
*  a new processor.  These routines or the combined routines actually  *
*  used in applications can be reimplemented in Assembler if greater   *
*  speed is desired.                                                   *
*                                                                      *
************************************************************************
*  V1A, 10/24/13, GNR - New routine, based on dsrswqe()                *
*  ==>, 10/24/13, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include <math.h>
#include <stdlib.h>
#include "sysdef.h"
#include "rkarith.h"

si32 dsrswqd(si64 x, int s, si32 y) {

#ifdef HAS_I64
   ui64 ux,uy;
   si32 sgnq;

   /* Error if attempt to divide by zero */
   if (y == 0) { abexitmq(72,"Divide by zero."); return 0; }

   /* Compute sign of quotient, which will also be overflow
   *  return value  */
   sgnq = (x ^ (si64)y) < 0 ? -SI32_MAX : SI32_MAX;

   /* Scale.  Note:  If the scaling would overflow, then the
   *  division also must overflow, no need to perform it.  */
   ux = abs64(x);
   if (s > 0) {
      /* Error if scaling would overflow--no need to do this
      *  test if s == 0, because sign bit is now clear */
      if (ux >> (63-s)) goto ReportOvflow;
      ux <<= s;
      }
   else
      ux >>= -s;

   /* Round--overflow into sign will detected w/divide check */
   uy = (ui64)(abs32(y));
   ux += (uy >> 1);

   /* Divide */
   if (ux >= uy<<31) goto ReportOvflow;
   else
      ux /= uy;
   return (sgnq < 0) ? -(si32)ux : (si32)ux;

# else               /* System does not have I64 */
   ui32 dh, dl, div;
   si32 sgnq;
   int ct;

   /* Error if attempt to divide by zero */
   if (y == 0) { abexitmq(72,"Divide by zero."); return 0; }

   /* Compute sign of quotient, which will also be overflow
   *  return value  */
   sgnq = (x.hi ^ y) < (si32)0 ? -SI32_MAX : SI32_MAX;

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

/* Now add the divisor in order to round the quotient */

   dh += (~div < dl);
   dl += div;

/* If the high order word of the dividend is now larger
*  than the divisor, report an overflow.  (With signed
*  division, the first quotient bit must be zero.)  */

   if (dh >= div) goto ReportOvflow;

/* Perform long division.  Two divide steps are done in each loop
*  iteration in order to reduce loop overhead.  Each time dividend
*  is shifted left by addition, another quotient bit is brought in
*  at the right.  */

   else for (ct=16; ct>0; --ct) {
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

   return (sgnq < (si32)0) ? -(si32)dl : (si32)dl;
#endif

ReportOvflow:
   e64dac("dsrswqd");
   return sgnq;
   } /* End dsrswqd() */
