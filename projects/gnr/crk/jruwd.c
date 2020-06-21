/* (c) Copyright 2013, The Rockefeller University *11115* */
/* $Id: jruwd.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                                jruwd                                 *
*                                                                      *
*  This function subtracts two 64-bit unsigned integers with full      *
*  overflow checking.                                                  *
*                                                                      *
*  Synopsis: ui64 jruwd(ui64 x, ui64 y)                                *
*  This is the version that uses the error code stored by e64dec().    *
*                                                                      *
*  This routine is part of a basic 64-bit arithmetic package for which *
*  full C implementations are provided in order to get one started on  *
*  a new processor.  These routines or the combined routines actually  *
*  used in applications can be reimplemented in Assembler if greater   *
*  speed is desired.                                                   *
*                                                                      *
************************************************************************
*  V1A, 10/24/13, GNR - New routine                                    *
*  ==>, 10/24/13, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include "sysdef.h"
#include "rkarith.h"

ui64 jruwd(ui64 x, ui64 y) {

#ifdef HAS_I64

   /* Check for underflow.  If e64dac returns,
   *  set result to min ui64 value.  */
   if (y > x) {
      e64dac("jruwd");
      return (ui64)0;
      }
   else
      return (x - y);
#else

   /* Check for underflow.  If e64dac returns,
   *  set result to min ui64 value.  */
   ui32 ylb = y.lo > x.lo;
   if (y.hi > x.hi | y.hi == x.hi & ylb) {
      e64dac("jruwd");
      return jeul(0);
      }
   else {
      ui64 rv;
      rv.lo = x.lo - y.lo;
      rv.hi = x.hi - y.hi - ylb;
      return rv;
      }

#endif

   } /* End jruwd() */
