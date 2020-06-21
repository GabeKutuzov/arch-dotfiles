/* (c) Copyright 2014, The Rockefeller University *11115* */
/* $Id: jsrruw.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                               jsrruw                                 *
*                                                                      *
*  This function scales a 64-bit unsigned integer by a given amount    *
*  to the right with rounding.  Overflow is not possible, so not       *
*  checked.                                                            *
*                                                                      *
*  Synopsis: ui64 jsrruw(ui64 x, int s)                                *
*                                                                      *
*  Routine assumes without checking 0 < s < 63.                        *
*                                                                      *
*  Algorithm:  This program uses a shift of (s-1) followed by addition *
*  of the rounding bit followed by the final shift in order to capture *
*  any carry in the sign bit when x is the largest unsigned number.    *
*  This avoids any 'if' statements, hopefully for better speed.        *  
*                                                                      *
*  This routine is part of a basic 64-bit arithmetic package for which *
*  full C implementations are provided in order to get one started on  *
*  a new processor.  These routines or the combined routines actually  *
*  used in applications can be reimplemented in Assembler if greater   *
*  speed is desired.  Specifically in this case, access to a hardware  *
*  overflow flag would greatly simplify the logic.                     *
*                                                                      *
************************************************************************
*  V1A, 10/09/14, GNR - New routine                                    *
*  ==>, 10/10/14, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include <stdlib.h>
#include "sysdef.h"
#include "rkarith.h"

ui64 jsrruw(ui64 x, int s) {

#ifdef HAS_I64

   ui64 rv = x >> (s-1);
   rv += rv & UI64_01;
   return (rv >> 1);

#else
   ui64 rv;
   int sm1 = s - 1;
   if (sm1 >= BITSPERUI32) {  /* Shift is more than one full word */
      rv.lo = x.hi >> (sm1 - BITSPERUI32);
      rv.hi = 0;
      }
   else {                     /* Shift is less than one full word */
      rv.lo = x.lo >> sm1 | x.hi << (BITSPERUI32 - sm1);
      rv.hi = x.hi >> sm1;
      }
   sm1 = rv.lo & 1;
   rv.lo += sm1, rv.hi += (rv.lo == 0) & sm1;
   rv.lo = rv.lo >> 1 | rv.hi << (BITSPERUI32-1);
   rv.hi >>= 1;
   return rv;
#endif

   } /* End jsrruw() */
