/* (c) Copyright 2016, The Rockefeller University *11115* */
/* $Id: jaslod.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                               jaslod                                 *
*                                                                      *
*  This function adds a signed long integer to a signed 64-bit integer *
*  with full overflow checking.                                        *
*  This is the version that uses the error code stored by e64dec().    *
*                                                                      *
*  Synopsis: si64 jaslod(si64 x, long y)                               *
*                                                                      *
*  Note:  The addend here must be 32-bits if !HAS_I64, but could be    *
*  32 or 64 if HAS_I64                                                 *
*                                                                      *
*  This routine is part of a basic 64-bit arithmetic package for which *
*  full C implementations are provided in order to get one started on  *
*  a new processor.  These routines or the combined routines actually  *
*  used in applications can be reimplemented in Assembler if greater   *
*  speed is desired.                                                   *
*                                                                      *
************************************************************************
*  R60, 05/13/16, GNR - New routine                                    *
*  ==>, 05/13/16, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include "sysdef.h"
#include "rkarith.h"

si64 jaslod(si64 x, long y) {

   si64 rv;

#ifdef HAS_I64
   rv = x + (si64)y;

   /* Check for overflow, avoiding multiple tests.  If e64dac
   *  returns, set result to max value with correct sign.  */
   if (((rv ^ x) & (x ^ ~(si64)y)) < (si64)0) {
      e64dac("jaslod");
      rv = SRA(rv, 63) ^ SI64_SGN;
      }
#else
   rv.lo = x.lo + (ui32)y;
   rv.hi = x.hi - (y < 0) + (~(ui32)y < x.lo);

   /* Check for overflow, same comments as above.  */
   if (((rv.hi ^ x.hi) & (x.hi ^ ~(si32)y)) < (si32)0) {
      e64dac("jaslod");
      rv.lo = SRA(rv.hi, 31);
      rv.hi = rv.lo ^ UI32_SGN;
      }
#endif

   return rv;

   } /* End jaslod() */
