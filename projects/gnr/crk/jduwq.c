/* (c) Copyright 1999-2013, The Rockefeller University *11115* */
/* $Id: jduwq.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                                jduwq                                 *
*                                                                      *
*  This function divides an unsigned 64-bit integer by an unsigned     *
*  ui32 divisor and returns just the quotient.  This routine termi-    *
*  nates on all errors--use jduwqe() to record overflows via e64act(). *
*                                                                      *
*  Synopsis: ui32 jduwq(ui64 x, ui32 y)                                *
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
***********************************************************************/

#include "sysdef.h"
#include "rkarith.h"

#ifdef HAS_I64
#error Do not compile jduwq.c with HAS_I64, is a macro
#endif 

ui32 jduwq(ui64 x, ui32 y) {

   ui32 c, dh, dl;
   int ct;

/* Error if attempt to divide by zero or if the high order
*  word of the dividend is already larger than the divisor.  */

   if (y == 0) { abexitmq(72,"Divide by zero."); return 0; }
   if (x.hi >= y) { abexitmq(73,"Divide check."); return 0; }

/* Get working copy of the dividend */

   dh = x.hi;
   dl = x.lo;

/* Do an initial shift to position the dividend
*  for the first division step.  */

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

   return dl;

   } /* End jduwq() */
