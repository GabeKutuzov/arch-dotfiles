/* (c) Copyright 1999-2013, The Rockefeller University *11115* */
/* $Id: jrswe.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                                jrswe                                 *
*                                                                      *
*  This function subtracts two 64-bit signed integers with full        *
*  overflow checking.                                                  *
*                                                                      *
*  Synopsis: si64 jrswe(si64 x, si64 y, int ec)                        *
*                                                                      *
*  This routine is part of a basic 64-bit arithmetic package for which *
*  full C implementations are provided in order to get one started on  *
*  a new processor.  These routines or the combined routines actually  *
*  used in applications can be reimplemented in Assembler if greater   *
*  speed is desired.                                                   *
*                                                                      *
************************************************************************
*  V1A, 06/19/99, GNR - New routine                                    *
*  ==>, 11/16/08, GNR - Last date before committing to svn repository  *
*  Rev, 10/25/13, GNR - Use SI64_SGN and SI32_SGN                      *
***********************************************************************/

#include "sysdef.h"
#include "rkarith.h"

si64 jrswe(si64 x, si64 y, int ec) {

   si64 rv;

#ifdef HAS_I64
   rv = x - y;

   /* Check for underflow, avoiding multiple tests.  If e64act
   *  returns, set result to max value of correct sign.  */
   if (((rv ^ x) & (x ^ y)) < (si64)0) {
      e64act("jrswe", ec);
      rv = SRA(rv, 63) ^ SI64_SGN;
      }
#else
   rv.lo = x.lo - y.lo;
   rv.hi = x.hi - y.hi - (y.lo > x.lo);

   /* Check for overflow, same comments as above.  */
   if (((rv.hi ^ x.hi) & (x.hi ^ y.hi)) < (si32)0) {
      e64act("jrswe", ec);
      rv.lo = SRA(rv.hi, 31);
      rv.hi = rv.lo ^ UI32_SGN;
      }
#endif

   return rv;
   } /* End jrswe() */
