/* (c) Copyright 1999-2008, The Rockefeller University *11115* */
/* $Id: jsuw.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                                jsuw                                  *
*                                                                      *
*  This function scales a 64-bit unsigned integer by a given amount    *
*  to the left or right with no overflow checking.                     *
*                                                                      *
*  Synopsis: ui64 jsuw(ui64 x, int s)                                  *
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
***********************************************************************/

#include "sysdef.h"
#include "rkarith.h"

ui64 jsuw(ui64 x, int s) {

#ifdef HAS_I64
   if (s >= 0)
      return (x << s);
   else
      return (x >> -s);
#else
   ui64 rv;

   if (s > 0) {
      /* Perform positive (left) shift.
      *  This being the non-error-checking version, there
      *  is no test for shifts >= 64 positions.  */
      if (s >= BITSPERUI32) {
         rv.lo = 0;
         rv.hi = x.lo << (s - BITSPERUI32);
         }
      else {
         rv.lo = x.lo << s;
         rv.hi = (x.hi << s) | (x.lo >> (BITSPERUI32 - s));
         }
      }
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

   } /* End jsuw() */
