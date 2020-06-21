/* (c) Copyright 2013, The Rockefeller University *11115* */
/* $Id: jauwd.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                                jauwd                                 *
*                                                                      *
*  This function adds two 64-bit unsigned integers with full overflow  *
*  checking.                                                           *
*  This is the version that uses the error code stored by e64dec().    *
*                                                                      *
*  Synopsis: ui64 jauwd(ui64 x, ui64 y)                                *
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

ui64 jauwd(ui64 x, ui64 y) {

   ui64 rv;

#ifdef HAS_I64
   rv = x + y;

   /* Check for overflow.  If e64dac returns,
   *  set result to max ui64 value.  */
   if (~y < x) {
      e64dac("jauwd");
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
      e64dac("jauwd");
      rv.hi = rv.lo = UI32_MAX;
      }
#endif

   return rv;
   } /* End jauwd() */
