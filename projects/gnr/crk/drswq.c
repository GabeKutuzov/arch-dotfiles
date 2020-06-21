/* (c) Copyright 2016, The Rockefeller University *11115* */
/* $Id: drswq.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                                drswq                                 *
*                                                                      *
*  This function performs rounded division of a signed 64-bit dividend *
*  by an si32 divisor.  It first performs rounding by adding           *
*  abs(divisor)/2 to abs(dividend).  Then it performs the division,    *
*  applies the correct sign to the quotient, and returns it.           *
*                                                                      *
*  This routine assumes that overflow errors cannot occur because the  *
*  dividend is the sum of divisor si32 quantities, i.e. it is being    *
*  used to compute a rounded average.  For convenience in this case,   *
*  division by 0 just reports a zero quotient.  (Versions with error   *
*  checking need to be written if needed.)                             *
*                                                                      *
*  Synopsis: si32 drswq(si64 x, si32 y)                                *
*                                                                      *
*  This routine is part of a basic 64-bit arithmetic package for which *
*  full C implementations are provided in order to get one started on  *
*  a new processor.  These routines or the combined routines actually  *
*  used in applications can be reimplemented in Assembler if greater   *
*  speed is desired.                                                   *
*                                                                      *
************************************************************************
*  V1A, 12/27/16, GNR - New routine, shortened version of dsrswq()     *
*  ==>, 12/27/16, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include <math.h>
#include <stdlib.h>
#include "sysdef.h"
#include "rkarith.h"

si32 drswq(si64 x, si32 y) {

#ifdef HAS_I64
   ui64 ux,uy;

   /* Return zero if attempt to divide by zero */
   if (y == 0) return 0;

   /* Round and divide */
   ux = abs64(x);
   uy = (ui64)(abs32(y));
   ux += (uy >> 1);
   ux /= uy;
   return (si32)(((x ^ (si64)y) < 0) ? -ux : ux);

# else               /* System does not have I64 */
   ui32 dh, dl, div;
   int ct;

   /* Return zero if attempt to divide by zero */
   if (y == 0) return 0;

/* Get absolute value of dividend and divisor */

   if (x.hi >= (si32)0) {
      dh = x.hi;
      dl = x.lo; }
   else {
      dh = ~x.hi + (x.lo == 0);
      dl = -x.lo; }
   div = abs32(y);

/* After doubling the dividend for the long division algorithm,
*  add the full divisor, rather than half of it, for rounding.  */

   dh = dh + dh + (dl >> (BITSPERUI32 - 1));
   dl = dl + dl;

/* Now add the divisor in order to round the quotient */

   dh += (~div < dl);
   dl += div;

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
   } /* End drswq() */
