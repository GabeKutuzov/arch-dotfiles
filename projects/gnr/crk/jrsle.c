/* (c) Copyright 2014, The Rockefeller University *11115* */
/* $Id: jrsle.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                                jrsle                                 *
*                                                                      *
*  This function subtracts a 32-bit signed integer from a 64-bit       *
*  signed integer with full overflow checking.                         *
*                                                                      *
*  Synopsis: si64 jrsle(si64 x, si32 y, int ec)                        *
*                                                                      *
*  This routine is part of a basic 64-bit arithmetic package for which *
*  full C implementations are provided in order to get one started on  *
*  a new processor.  These routines or the combined routines actually  *
*  used in applications can be reimplemented in Assembler if greater   *
*  speed is desired.                                                   *
*                                                                      *
************************************************************************
*  V1A, 05/25/14, GNR - New routine                                    *
*  ==>, 05/25/14, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include "sysdef.h"
#include "rkarith.h"

si64 jrsle(si64 x, si32 y, int ec) {

   si64 rv;

#ifdef HAS_I64
   rv = x - (si64)y;

   /* Check for overflow, avoiding multiple tests.  If e64act
   *  returns, set result to max value of correct sign.  */
   if (((rv ^ x) & (x ^ (si64)y)) < (si64)0) {
      e64act("jrsle", ec);
      rv = x > 0 ? SI64_MAX : ~SI64_MAX;
      }
#else
   rv.lo = x.lo - (ui32)y;
   rv.hi = x.hi + (y < 0) - ((ui32)y > x.lo);

   /* Check for overflow, same comments as above.  */
   if (((rv.hi ^ x.hi) & (x.hi ^ y)) < (si32)0) {
      e64act("jrsle", ec);
      if (x.hi > 0) rv.hi = SI32_MAX, rv.lo = UI32_MAX;
      else          rv.hi = SI32_SGN, rv.lo = 0;
      }
#endif

   return rv;
   } /* End jrsle() */
