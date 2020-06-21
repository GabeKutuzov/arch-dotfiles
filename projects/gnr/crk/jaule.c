/* (c) Copyright 2003-2008, The Rockefeller University *11115* */
/* $Id: jaule.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                                jaule                                 *
*                                                                      *
*  This function adds an unsigned long integer to a 64-bit unsigned    *
*  integer with full overflow checking.                                *
*                                                                      *
*  Synopsis: ui64 jaule(ui64 x, ui32 y, int ec)                        *
*                                                                      *
*  This routine is part of a basic 64-bit arithmetic package for which *
*  full C implementations are provided in order to get one started on  *
*  a new processor.  These routines or the combined routines actually  *
*  used in applications can be reimplemented in Assembler if greater   *
*  speed is desired.                                                   *
*                                                                      *
************************************************************************
*  V1A, 03/22/03, GNR - New routine                                    *
*  ==>, 09/30/04, GNR - Last date before committing to svn repository  *
*  Rev, 11/23/08, GNR - Bug fix:  Add missing parens in overflow test  *
***********************************************************************/

#include "sysdef.h"
#include "rkarith.h"

ui64 jaule(ui64 x, ui32 y, int ec) {

   ui64 rv;

#ifdef HAS_I64
   rv = x + (ui64)y;

   /* Check for overflow.  The only possible overflow case here
   *  is when the high order starts out as ~0 and ends up 0.
   *  If e64act returns, set result to max ui64 value.  */
   if (uwhi(~x | rv) == 0) {
      e64act("jaule", ec);
      rv = UI64_MAX;
      }
#else

   rv.lo = x.lo + y;
   rv.hi = x.hi + (~y < x.lo);

   /* Check for overflow.  The only possible overflow case here
   *  is when the high order starts out as ~0 and ends up 0.
   *  If e64act returns, set result to max ui64 value.  */
   if ((~x.hi | rv.hi) == 0) {
      e64act("jaule", ec);
      rv.hi = rv.lo = UI32_MAX;
      }
#endif

   return rv;
   } /* End jaule() */
