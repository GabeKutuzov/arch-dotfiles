/* (c) Copyright 2014, The Rockefeller University *11115* */
/* $Id: jsluwe.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                               jsluwe                                 *
*                                                                      *
*  This function scales a 64-bit unsigned integer by a given amount    *
*  to the left with full overflow checking.                            *
*  Routine requires without checking 0 < s < 64.                       *
*  This is the version that uses the error code passed as an argument. *
*                                                                      *
*  Synopsis: ui64 jsluwe(ui64 x, int s, int ec)                        *
*                                                                      *
*  This routine is part of a basic 64-bit arithmetic package for which *
*  full C implementations are provided in order to get one started on  *
*  a new processor.  These routines or the combined routines actually  *
*  used in applications can be reimplemented in Assembler if greater   *
*  speed is desired.                                                   *
*                                                                      *
************************************************************************
*  V1A, 10/08/14, GNR - New routine                                    *
*  ==>, 10/08/14, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include <stdlib.h>
#include <math.h>
#include "sysdef.h"
#include "rkarith.h"

ui64 jsluwe(ui64 x, int s, int ec) {

#ifdef HAS_I64
   if (x >> (64-s)) {
      e64act("jsluwe", ec);
      return UI64_MAX;
      }
   return (x << s);
#else
   ui64 rv;

   if (s >= BITSPERUI32) { /* Shift is more than one full word */
      if (s > BITSPERUI32 ? x.lo >> (BITSPERUI64-s) | x.hi : x.hi) {
         e64act("jsluwe", ec);
         rv.hi = rv.lo = UI32_MAX;
         }
      else {
         rv.lo = 0;
         rv.hi = x.lo << (s - BITSPERUI32);
         }
      }
   else {                  /* Shift is less than one full word */
      if (x.hi >> (BITSPERUI32-s)) {
         e64act("jsluwe", ec);
         rv.hi = rv.lo = UI32_MAX;
         }
      else {
         rv.lo = x.lo << s;
         rv.hi = (x.hi << s) | (x.lo >> (BITSPERUI32 - s));
         }
      }

   return rv;
#endif

   } /* End jsluwe() */
