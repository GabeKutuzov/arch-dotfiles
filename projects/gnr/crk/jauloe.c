/* (c) Copyright 2016, The Rockefeller University *11115* */
/* $Id: jauloe.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                               jauloe                                 *
*                                                                      *
*  This function adds an unsigned long integer to an unsigned 64-bit   *
*  integer with full overflow checking.                                *
*  This is the version that calls e64act() when an overflow occurs.    *
*                                                                      *
*  Synopsis: ui64 jauloe(ui64 x, ulng y, int ec)                       *
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

ui64 jauloe(ui64 x, ulng y, int ec) {

   ui64 rv;

#ifdef HAS_I64
   rv = x + (ui64)y;

   /* Check for overflow.  If e64dac returns,
   *  set result to max ui64 value.  */
   if (~(ui64)y < x) {
      e64act("jauloe", ec);
      rv = UI64_MAX;
      }
#else

   rv.lo = x.lo + (ui32)y;
   rv.hi = x.hi + (~(ui32)y < x.lo);

   /* Check for overflow.  Unlike above, the only possible overflow
   *  case here is when the high order starts out as ~0 and ends up 0.
   *  If e64dac returns, set result to max ui64 value.  */
   if ((~x.hi | rv.hi) == 0) {
      e64act("jauloe", ec);
      rv.hi = rv.lo = UI32_MAX;
      }
#endif

   return rv;
   } /* End jauloe() */
