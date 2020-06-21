/* (c) Copyright 1999-2013, The Rockefeller University *11115* */
/* $Id: jruwe.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                                jruwe                                 *
*                                                                      *
*  This function subtracts two 64-bit unsigned integers with full      *
*  overflow checking.                                                  *
*                                                                      *
*  Synopsis: ui64 jruwe(ui64 x, ui64 y, int ec)                        *
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
*  Rev, 10/25/13, GNR - Simplify arithmetic, use SI32_SGN              *
***********************************************************************/

#include "sysdef.h"
#include "rkarith.h"

ui64 jruwe(ui64 x, ui64 y, int ec) {

#ifdef HAS_I64

   /* Check for underflow.  If e64act returns,
   *  set result to min ui64 value.  */
   if (y > x) {
      e64act("jruwe", ec);
      return (ui64)0;
      }
   else
      return (x - y);

#else

   /* Check for underflow.  If e64act returns,
   *  set result to min ui64 value.  */
   ui32 ylb = y.lo > x.lo;
   if (y.hi > x.hi | y.hi == x.hi & ylb) {
      e64act("jruwe", ec);
      return jeul(0);
      }
   else {
      ui64 rv;
      rv.lo = x.lo - y.lo;
      rv.hi = x.hi - y.hi - ylb;
      return rv;
      }

#endif

   } /* End jruwe() */
