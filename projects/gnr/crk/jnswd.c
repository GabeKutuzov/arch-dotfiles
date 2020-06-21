/* (c) Copyright 2013, The Rockefeller University *11115* */
/* $Id: jnswd.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                                jnswd                                 *
*                                                                      *
*  This function computes the two's complement of a 64-bit signed      *
*  integer with full overflow checking.                                *
*  This is the version that uses the error code stored by e64dec().    *
*                                                                      *
*  Synopsis: si64 jnswd(si64 x)                                        *
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

si64 jnswd(si64 x) {

#ifdef HAS_I64
   if (x == SI64_SGN) {
      e64dac("jnswd");
      return x;
      }
   return -x;
#else
   si64 rv;

   if (x.hi == SI32_SGN && x.lo == (ui32)0) {
      e64dac("jnswd");
      return x;
      }
   rv.lo = -x.lo;
   rv.hi = ~x.hi + (x.lo == (ui32)0);

   return rv;
#endif

   } /* End jnswd() */
