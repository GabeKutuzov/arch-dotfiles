/* (c) Copyright 2013, The Rockefeller University *11115* */
/* $Id: jduwqd.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                               jduwqd                                 *
*                                                                      *
*  This function divides an unsigned 64-bit integer by an unsigned     *
*  ui32 divisor and returns just the quotient.  This routine termi-    *
*  nates on divide by 0 errors but records overflows via e64dac() and  *
*  returns max quotient.                                               *
*  This is the version that uses the error code stored by e64dac().    *
*                                                                      *
*  Synopsis: ui32 jduwqd(ui64 x, ui32 y)                               *
*                                                                      *
*  This routine is part of a basic 64-bit arithmetic package for which *
*  full C implementations are provided in order to get one started on  *
*  a new processor.  These routines or the combined routines actually  *
*  used in applications can be reimplemented in Assembler if greater   *
*  speed is desired.                                                   *
*                                                                      *
************************************************************************
*  V1A, 10/26/13, GNR - New routine, based on jduwq                    *
*  ==>, 10/26/13, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include "sysdef.h"
#include "rkarith.h"

ui32 jduwqd(ui64 x, ui32 y) {

/* Error if attempt to divide by zero */

   if (y == 0) { abexitmq(72,"Divide by zero."); return 0; }

#ifdef HAS_I64
   {  ui64 wy = (ui64)y;

      /* If dividend >= 2^32 * divisor, report an overflow */
      if (x >= wy<<(BITSPERUI32)) {
         e64dac("jduwqd");
         return UI32_MAX;
         }
      /* Otherwise, perform the division in the hardware */
      return (si32)(x / wy);
      } /* End wy local scope */

#else /* !HAS_I64 */

   {  ui32 c, dh, dl;
      int ct;

/* Overflow if high order word of the dividend is already larger
*  than the divisor.  */

      if (x.hi >= y) {
         e64dac("jduwqd");
         return UI32_MAX;
         }

/* Get working copy of the dividend */

      dh = x.hi;
      dl = x.lo;

/* Do an initial shift to position the dividend
*  for the first division step.  */

      c   = dh >> (BITSPERUI32-1);
      dh += dh + (dl >> (BITSPERUI32-1));
      dl += dl;

/* Perform long division.  Two divide steps are done in each loop
*  iteration in order to reduce loop overhead.  Each time dividend
*  is shifted left by addition, another quotient bit is brought in
*  at the right.  */

      for (ct=16; ct>0; --ct) {
         if (c || dh >= y) {        /* One division cycle */
            dh -= y;
            c   = dh >> (BITSPERUI32-1);
            dh += dh + (dl >> (BITSPERUI32-1));
            dl += dl + 1; }
         else {
            c   = dh >> (BITSPERUI32-1);
            dh += dh + (dl >> (BITSPERUI32-1));
            dl += dl; }
         if (c || dh >= y) {        /* Second division cycle */
            dh -= y;
            c   = dh >> (BITSPERUI32-1);
            dh += dh + (dl >> (BITSPERUI32-1));
            dl += dl + 1; }
         else {
            c   = dh >> (BITSPERUI32-1);
            dh += dh + (dl >> (BITSPERUI32-1));
            dl += dl; }
         }

      return dl;
      } /* End local scope */
#endif

   } /* End jduwqd() */
