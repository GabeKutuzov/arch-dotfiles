/* (c) Copyright 1999-2013, The Rockefeller University *11115* */
/* $Id: jsuwe.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                                jsuwe                                 *
*                                                                      *
*  This function scales a 64-bit unsigned integer by a given amount    *
*  to the left or right with full overflow checking (except assumes    *
*  the shift argument does not exceed 63).                             *
*                                                                      *
*  Synopsis: ui64 jsuwe(ui64 x, int s, int ec)                         *
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
*  Rev, 10/25/13, GNR - Get rid of 'L' constants                       *
***********************************************************************/

#include "sysdef.h"
#include "rkarith.h"

ui64 jsuwe(ui64 x, int s, int ec) {

#ifdef HAS_I64
   if (s > 0) {
      if (x >> (64-s)) {
         e64act("jsuwe", ec);
         return UI64_MAX;
         }
      return (x << s);
      }
   else if (s < 0)
      return (x >> -s);
   else /* s == 0 */
      return x;
#else
   ui64 rv;

   if (s > 0) {
      /* Perform positive (left) shift */
      if (s >= BITSPERUI32) { /* Shift is more than one full word */
         if (x.hi > (si32)0 || x.lo >> (64-s)) {
            e64act("jsuwe", ec);
            rv.hi = rv.lo = 0xffffffffUL;
            }
         else {
            rv.lo = 0;
            rv.hi = x.lo << (s - BITSPERUI32);
            }
         }
      else {                  /* Shift is less than one full word */
         if (x.hi >> (BITSPERUI32-s)) {
            e64act("jsuwe", ec);
            rv.hi = rv.lo = 0xffffffffUL;
            }
         else {
            rv.lo = x.lo << s;
            rv.hi = (x.hi << s) | (x.lo >> (BITSPERUI32 - s));
            }
         }
      } /* End case shift > 0 */
   else if (s < 0) {
      /* Perform negative (right) shift */
      int ss = -s;
      if (ss >= BITSPERUI32) {
         rv.lo = x.hi >> (ss - BITSPERUI32);
         rv.hi = 0;
         }
      else {
         rv.lo = (x.lo >> ss) | (x.hi << (BITSPERUI32 - ss));
         rv.hi = x.hi >> ss;
         }
      }
   else {
      /* Perform no shift at all.  It is necessary to
      *  handle this case separately because on some
      *  machines (e.g. 486 and up), a shift of 32
      *  as used above is a NOP.  */
      return x;
      }

   return rv;
#endif

   } /* End jsuwe() */
