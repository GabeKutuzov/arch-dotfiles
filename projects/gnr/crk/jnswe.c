/* (c) Copyright 1999-2013, The Rockefeller University *11115* */
/* $Id: jnswe.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                                jnswe                                 *
*                                                                      *
*  This function computes the two's complement of a 64-bit signed      *
*  integer with full overflow checking.                                *
*                                                                      *
*  Synopsis: si64 jnswe(si64 x, int ec)                                *
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
*  Rev, 10/25/13, GNR - Use SI64_SGN and SI32_SGN                      *
***********************************************************************/

#include "sysdef.h"
#include "rkarith.h"

si64 jnswe(si64 x, int ec) {

#ifdef HAS_I64
   if (x == SI64_SGN) {
      e64act("jnswe", ec);
      return x;
      }
   return -x;
#else
   si64 rv;

   if (x.hi == SI32_SGN && x.lo == (ui32)0) {
      e64act("jnswe", ec);
      return x;
      }
   rv.lo = -x.lo;
   rv.hi = ~x.hi + (x.lo == (ui32)0);

   return rv;
#endif

   } /* End jnswe() */
