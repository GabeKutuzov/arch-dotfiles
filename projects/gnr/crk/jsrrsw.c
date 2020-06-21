/* (c) Copyright 2017, The Rockefeller University *11115* */
/* $Id: jsrrsw.c 64 2017-09-15 17:55:59Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                               jsrrsw                                 *
*                                                                      *
*  This function scales a 64-bit signed integer by a given amount to   *
*  the right with rounding.  Overflow is not possible, so not checked. *
*                                                                      *
*  Synopsis: si64 jsrrsw(si64 x, int s)                                *
*                                                                      *
*  Routine assumes without checking 0 < s < 63.                        *
*                                                                      *
*  Algorithm:  This program uses a shift of (s-1) followed by addition *
*  of the rounding bit followed by the final shift.  It is necessary   *
*  to convert arguments to unsigned so carries into the sign bit do    *
*  not result in changing a big positive number to a negative one.     *
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
*  R74, 08/28/17, GNR - Fix bug, wrong ans if 2^63-1 shifted one       *
***********************************************************************/

#include <stdlib.h>
#include "sysdef.h"
#include "rkarith.h"

si64 jsrrsw(si64 x, int s) {

   if (s <= 0) abexit(74);    /* Rare, no message */

#ifdef HAS_I64
   union { ui64 xu64; si64 xs64; } ux;
   if (x > 0) {               /* Positive, avoid sign bug */
      ux.xs64 = x;
      ux.xu64 = ((ux.xu64 >> (s-1)) + SI64_01) >> 1;
      return ux.xs64;
      }
   else {                     /* Negative or zero */
      si64 rv = SRA(x, (s-1)) + SI64_01;
      return SRA(rv, 1);
      }

#else
   si64 rv;
   int sm1 = s - 1;
   if (sm1 >= BITSPERUI32) {  /* Shift is more than one full word */
      rv.lo = SRA(x.hi,(sm1 - BITSPERUI32));
      rv.hi = SRA(x.hi,(BITSPERUI32-1));
      }
   else {                     /* Shift is less than one full word */
      rv.lo = x.lo >> sm1 | x.hi << (BITSPERUI32 - sm1);
      rv.hi = SRA(x.hi,sm1);
      }
   sm1 = rv.lo & 1;
   rv.lo += sm1, rv.hi += (rv.lo == 0) & sm1;
   rv.lo = rv.lo >> 1 | rv.hi << (BITSPERUI32-1);
   rv.hi = (si32)((ui32)rv.hi >> 1) | rv.hi & x.hi & SI32_SGN;
   return rv;
#endif

   } /* End jsrrsw() */
