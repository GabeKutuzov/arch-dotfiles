/* (c) Copyright 2014, The Rockefeller University *11115* */
/* $Id: jruld.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                                jruld                                 *
*                                                                      *
*  This function subtracts a 32-bit unsigned integer from a 64-bit     *
*  unsigned integer with full overflow checking.                       *
*  This is the version that uses the error code stored by e64dec().    *
*                                                                      *
*  Synopsis: ui64 jruld(ui64 x, ui32 y)                                *
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

ui64 jruld(ui64 x, ui32 y) {

   ui64 rv;

#ifdef HAS_I64
   rv = x - (ui64)y;

   /* Check for overflow.  The only possible overflow case here
   *  is when the high order starts out as 0 and ends up ~0.
   *  If e64dac returns, set result to 0.  */
   if (uwhi(x | ~rv) == 0) {
      e64dac("jruld");
      rv = (ui64)0;
      }
#else

   rv.lo = x.lo - y;
   rv.hi = x.hi - (y > x.lo);

   /* Check for overflow.  The only possible overflow case here
   *  is when the high order starts out as 0 and ends up ~0.
   *  If e64dac returns, set result to 0.  */
   if ((x.hi | ~rv.hi) == 0) {
      e64dac("jruld");
      rv.hi = rv.lo = 0;
      }
#endif

   return rv;
   } /* End jruld() */
