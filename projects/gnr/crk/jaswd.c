/* (c) Copyright 2013, The Rockefeller University *11115* */
/* $Id: jaswd.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                                jaswd                                 *
*                                                                      *
*  This function adds two 64-bit signed integers with full overflow    *
*  checking.                                                           *
*  This is the version that uses the error code stored by e64dec().    *
*                                                                      *
*  Synopsis: si64 jaswd(si64 x, si64 y)                                *
*                                                                      *
*  This routine is part of a basic 64-bit arithmetic package for which *
*  full C implementations are provided in order to get one started on  *
*  a new processor.  These routines or the combined routines actually  *
*  used in applications can be reimplemented in Assembler if greater   *
*  speed is desired.                                                   *
*                                                                      *
************************************************************************
*  V1A, 10/25/13, GNR - New routine                                    *
*  ==>, 10/25/13, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include "sysdef.h"
#include "rkarith.h"

si64 jaswd(si64 x, si64 y) {

   si64 rv;

#ifdef HAS_I64
   rv = x + y;

   /* Check for overflow, avoiding multiple tests.  If e64dac
   *  returns, set result to max value of correct sign.  */
   if (jckss(rv,x,y)) {
      e64dac("jaswd");
      rv = SRA(rv, 63) ^ SI64_SGN;
      }
#else
   rv.lo = x.lo + y.lo;
   rv.hi = x.hi + y.hi + (~y.lo < x.lo);

   /* Check for overflow, same comments as above.  */
   if (((rv.hi ^ y.hi) & (x.hi ^ ~y.hi)) < (si32)0) {
      e64dac("jaswd");
      if (qsw(x) < 0)
         rv.hi = ~SI32_MAX, rv.lo = 0;
      else
         rv.hi = SI32_MAX, rv.lo = UI32_MAX;
      }
#endif

   return rv;
   } /* End jaswd() */
