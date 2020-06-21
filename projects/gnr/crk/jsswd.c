/* (c) Copyright 2013, The Rockefeller University *11115* */
/* $Id: jsswd.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                                jsswd                                 *
*                                                                      *
*  This function scales a 64-bit signed integer by a given amount      *
*  to the left or right with full overflow checking (except assume     *
*  shift argument does not exceed 63).                                 *
*  This is the version that uses the error code stored by e64dec().    *
*                                                                      *
*  Synopsis: si64 jsswd(si64 x, int s)                                 *
*                                                                      *
*  This routine is part of a basic 64-bit arithmetic package for which *
*  full C implementations are provided in order to get one started on  *
*  a new processor.  These routines or the combined routines actually  *
*  used in applications can be reimplemented in Assembler if greater   *
*  speed is desired.                                                   *
*                                                                      *
************************************************************************
*  V1A, 10/24/13, GNR - New routine                                    *
*  ==>, 10/24/13, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include <math.h>
#include <stdlib.h>
#include "sysdef.h"
#include "rkarith.h"

si64 jsswd(si64 x, int s) {

#ifdef HAS_I64
   if (s > 0) {
      si64 ax = abs64(x);
      if (ax >> (63-s)) {
         e64dac("jsswd");
         return SRA(~x, 63) ^ SI64_SGN;
         }
      return (x << s);
      }
   else if (s < 0)
      return SRA(x,-s);
   else /* s == 0 */
      return x;
#else
   si64 rv;

   if (s > 0) {
      /* Perform positive (left) shift.
      *  Since we have no abs(si64 x) function, it seems best to
      *  test and shift the two signs separately.  */
      if (s >= BITSPERUI32) { /* Shift is more than one full word */
         if (x.hi >= 0) {     /* First argument is positive */
            if (x.hi > (si32)0 || x.lo >> (63-s)) {
               e64dac("jsswd");
               rv.lo = UI32_MAX, rv.hi = SI32_MAX;
               }
            else {
               rv.lo = 0;
               rv.hi = x.lo << (s - BITSPERUI32);
               }
            }
         else {               /* First argument is negative */
            if (x.hi < -(si32)1 || ~x.lo >> (63-s)) {
               e64dac("jsswd");
               rv.lo = 0, rv.hi = SI32_SGN;
               }
            else {
               rv.lo = 0;
               rv.hi = x.lo << (s - BITSPERUI32);
               }
            }
         } /* End case s >= BITSPERUI32 */
      else {                  /* Shift is less than one full word */
         if (abs32(x.hi) >> (31 - s)) {
            e64dac("jsswd");
            rv.lo = SRA(~x.hi, 31);
            rv.hi = rv.lo ^ UI32_SGN;
            }
         else {
            rv.lo = x.lo << s;
            rv.hi = (x.hi << s) | (x.lo >> (BITSPERUI32 - s));
            }
         }
      } /* End case shift > 0 */
   else if (s < 0) {
      /* Perform negative (right) shift */
      int ss = -s;
      if (ss >= BITSPERUI32) {
         rv.lo = SRA(x.hi,(ss - BITSPERUI32));
         rv.hi = SRA(x.hi,31);
         }
      else {
         rv.lo = (x.lo >> ss) | (x.hi << (BITSPERUI32 - ss));
         rv.hi = SRA(x.hi,ss);
         }
      }
   else {
      /* Perform no shift at all.  It is necessary to
      *  handle this case separately because on some
      *  machines (e.g. 486 and up), a shift of 32
      *  as used above is a NOP.  */
      return x;
      }

   return rv;
#endif

   } /* End jsswd() */
