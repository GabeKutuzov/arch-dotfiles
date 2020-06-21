/* (c) Copyright 2014, The Rockefeller University *11115* */
/* $Id: jsruw.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                                jsruw                                 *
*                                                                      *
*  This function scales a 64-bit unsigned integer by a given amount    *
*  to the right.  Overflow is not possible, so not checked.            *
*                                                                      *
*  Synopsis: ui64 jsruw(ui64 x, int s)                                 *
*                                                                      *
*  Routine requires without checking 0 < s < 63.                       *
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
#include "sysdef.h"
#include "rkarith.h"

ui64 jsruw(ui64 x, int s) {

#ifdef HAS_I64
#error jsruw.c not compiled with HAS_I64, is a macro
#else
   ui64 rv;
   if (s >= BITSPERUI32) {    /* Shift is more than one full word */
      rv.lo = x.hi >> (s - BITSPERUI32);
      rv.hi = 0;
      }
   else {                     /* Shift is less than one full word */
      rv.lo = (x.lo >> s) | (x.hi << (BITSPERUI32 - s));
      rv.hi = x.hi >> s;
      }

   return rv;
#endif

   } /* End jsruw() */
