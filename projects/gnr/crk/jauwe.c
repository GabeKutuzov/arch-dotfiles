/* (c) Copyright 1999-2013, The Rockefeller University *11115* */
/* $Id: jauwe.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                                jauwe                                 *
*                                                                      *
*  This function adds two 64-bit unsigned integers with full overflow  *
*  checking.                                                           *
*                                                                      *
*  Synopsis: ui64 jauwe(ui64 x, ui64 y, int ec)                        *
*                                                                      *
*  This routine is part of a basic 64-bit arithmetic package for which *
*  full C implementations are provided in order to get one started on  *
*  a new processor.  These routines or the combined routines actually  *
*  used in applications can be reimplemented in Assembler if greater   *
*  speed is desired.                                                   *
*                                                                      *
************************************************************************
*  V1A, 06/19/99, GNR - New routine                                    *
*  ==>, 07/01/02, GNR - Last date before committing to svn repository  *
*  Rev, 10/25/13, GNR - Use SI32_SGN, UI32_SGN                         *
***********************************************************************/

#include "sysdef.h"
#include "rkarith.h"

ui64 jauwe(ui64 x, ui64 y, int ec) {

   ui64 rv;

#ifdef HAS_I64
   rv = x + y;

   /* Check for overflow.  If e64act returns,
   *  set result to max ui64 value.  */
   if (~y < x) {
      e64act("jauwe", ec);
      rv = UI64_MAX;
      }
#else

   rv.lo = x.lo + y.lo;
   rv.hi = x.hi + y.hi + (~y.lo < x.lo);

   /* Check for overflow.  A little extra arithmetic is probably
   *  faster than a compound 'if'.  (Can't use (~y < x) formula
   *  above, as low order can carry into high and this carry can
   *  overflow if added to either the x or y term in the test.)  */
   if ((((x.hi ^ y.hi) & ~rv.hi) | (x.hi & y.hi)) & SI32_SGN) {
      e64act("jauwe", ec);
      rv.hi = rv.lo = UI32_MAX;
      }
#endif

   return rv;
   } /* End jauwe() */
