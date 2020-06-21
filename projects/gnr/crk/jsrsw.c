/* (c) Copyright 2014, The Rockefeller University *11115* */
/* $Id: jsrsw.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                                jsrsw                                 *
*                                                                      *
*  This function scales a 64-bit signed integer by a given amount      *
*  to the right.  Overflow is not possible, so not checked.            *
*                                                                      *
*  Synopsis: si64 jsrsw(si64 x, int s)                                 *
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

si64 jsrsw(si64 x, int s) {

#ifdef HAS_I64
#error jsrsw.c not compiled with HAS_I64, is a macro
#else
   si64 rv;
   if (s >= BITSPERUI32) {    /* Shift is more than one full word */
      rv.lo = SRA(x.hi,(s - BITSPERUI32));
      rv.hi = SRA(x.hi,(BITSPERUI32-1));
      }
   else {                     /* Shift is less than one full word */
      rv.lo = (x.lo >> s) | (x.hi << (BITSPERUI32 - s));
      rv.hi = SRA(x.hi,s);
      }

   return rv;
#endif

   } /* End jsrsw() */
