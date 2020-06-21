/* (c) Copyright 1999-2013, The Rockefeller University *11115* */
/* $Id: jdswq.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                                jdswq                                 *
*                                                                      *
*  This function divides a signed 64-bit integer by an si32 divisor    *
*  and returns just the quotient.  This routine terminates on all      *
*  errors--use jdswqe() to record overflows via e64act().              *
*                                                                      *
*  Synopsis: si32 jdswq(si64 x, si32 y)                                *
*                                                                      *
*  Note:  This routine is a macro in the HAS_I64 case                  *
*                                                                      *
*  This routine is part of a basic 64-bit arithmetic package for which *
*  full C implementations are provided in order to get one started on  *
*  a new processor.  These routines or the combined routines actually  *
*  used in applications can be reimplemented in Assembler if greater   *
*  speed is desired.                                                   *
*                                                                      *
************************************************************************
*  V1A, 06/20/99, GNR - New routine                                    *
*  ==>, 10/03/08, GNR - Last date before committing to svn repository  *
*  Rev, 06/09/13, GNR - Deal with return from abexit in tests          *
*  Rev, 10/25/13, GNR - Get rid of 'L' constants                       *
***********************************************************************/

#include <math.h>
#include <stdlib.h>  /* labs() proto for icc */
#include "sysdef.h"
#include "rkarith.h"

#ifdef HAS_I64
#error Do not compile jdswq.c with HAS_I64, is a macro
#endif 

si32 jdswq(si64 x, si32 y) {

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

/* Do an initial shift to position the dividend
*  for the first division step.  */

   dh += dh + (dl >> 31);
   dl += dl;

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

   } /* End jdswq() */
