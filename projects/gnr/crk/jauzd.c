/* (c) Copyright 2016, The Rockefeller University *11115* */
/* $Id: jauzd.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                                jauzd                                 *
*                                                                      *
*  This function adds a size_t value to an unsigned 64-bit integer     *
*  with full overflow checking.                                        *
*  This is the version that uses the error code stored by e64dec().    *
*                                                                      *
*  Synopsis: ui64 jauzd(ui64 w, size_t z)                              *
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
*  R60, 05/14/16, GNR - New routine                                    *
*  ==>, 05/14/16, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include "sysdef.h"
#include "rkarith.h"

ui64 jauzd(ui64 w, size_t z) {

   ui64 rv;

#ifdef HAS_I64
   rv = w + (ui64)z;

   /* Check for overflow.  If e64dac returns,
   *  set result to max ui64 value.  */
   if (~(ui64)z < w) {
      e64dac("jauzd");
      rv = UI64_MAX;
      }
#else

   rv.lo = w.lo + (ui32)z;
   rv.hi = w.hi + (~(ui32)z < w.lo);

   /* Check for overflow.  Unlike above, the only possible overflow
   *  case here is when the high order starts out as ~0 and ends up 0.
   *  If e64dac returns, set result to max ui64 value.  */
   if ((~w.hi | rv.hi) == 0) {
      e64dac("jauzd");
      rv.hi = rv.lo = UI32_MAX;
      }
#endif

   return rv;
   } /* End jauzd() */
