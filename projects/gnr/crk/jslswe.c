/* (c) Copyright 2014, The Rockefeller University *11115* */
/* $Id: jslswe.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                               jslswe                                 *
*                                                                      *
*  This function scales a 64-bit signed integer by a given amount      *
*  to the left with full overflow checking.                            *
*  Routine requires without checking 0 < s < 64.                       *
*  This is the version that uses the error code passed as an argument. *
*                                                                      *
*  Synopsis: si64 jslswe(si64 x, int s, int ec)                        *
*                                                                      *
*  This routine is part of a basic 64-bit arithmetic package for which *
*  full C implementations are provided in order to get one started on  *
*  a new processor.  These routines or the combined routines actually  *
*  used in applications can be reimplemented in Assembler if greater   *
*  speed is desired.                                                   *
*                                                                      *
************************************************************************
*  V1A, 10/08/14, GNR - New routine, based on jsswd()                  *
*  ==>, 10/08/14, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include <stdlib.h>
#include <math.h>
#include "sysdef.h"
#include "rkarith.h"

si64 jslswe(si64 x, int s, int ec) {

#ifdef HAS_I64
   if (abs64(x) >> (63-s)) {
      e64act("jslswe", ec);
      return SRA(~x, 63) ^ SI64_SGN;
      }
   return (x << s);
#else
   si64 rv;

   if (s >= BITSPERUI32) { /* Shift is more than one full word */
      if (x.hi >= 0) {     /* First argument is positive */
         if (x.hi > (si32)0 || x.lo >> (63-s)) {
            e64act("jslswe", ec);
            rv.lo = UI32_MAX, rv.hi = SI32_MAX;
            }
         else {
            rv.lo = 0;
            rv.hi = x.lo << (s - BITSPERUI32);
            }
         }
      else {               /* First argument is negative */
         if (x.hi < -(si32)1 || ~x.lo >> (63-s)) {
            e64act("jslswe", ec);
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
         e64act("jslswe", ec);
         rv.lo = SRA(~x.hi, 31);
         rv.hi = rv.lo ^ UI32_SGN;
         }
      else {
         rv.lo = x.lo << s;
         rv.hi = (x.hi << s) | (x.lo >> (BITSPERUI32 - s));
         }
      }

   return rv;
#endif

   } /* End jslswe() */
