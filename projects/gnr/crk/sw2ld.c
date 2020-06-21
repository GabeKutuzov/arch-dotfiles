/* (c) Copyright 2013, The Rockefeller University *11115* */
/* $Id: sw2ld.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                                sw2ld                                 *
*                                                                      *
*  This functions extract a 32-bit signed long integer from a 64-bit   *
*  signed integer with overflow checking.  Note that if a long is      *
*  already 64-bit, rkarith.h will define sw2ld as a macro (overflow    *
*  is not possible).                                                   *
*  This is the version that uses the error code stored by e64dec().    *
*                                                                      *
*  Synopsis: long sw2ld(si64 x)                                        *
*                                                                      *
*  This routine is part of a basic 64-bit arithmetic package for which *
*  full C implementations are provided in order to get one started on  *
*  a new processor.  These routines or the combined routines actually  *
*  used in applications can be reimplemented in Assembler if greater   *
*  speed is desired.                                                   *
*                                                                      *
************************************************************************
*  V1A, 10/25/13, GNR - New routine                                    *
*  ==>, 10/25/13, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include <stdlib.h>
#include <math.h>
#include "sysdef.h"
#include "rkarith.h"

#undef sw2ld   /* Get rid of the macro */

long sw2ld(si64 x) {

#if LSIZE == 8
   abexitq(87);
   return 0;
#else
   if (jckslo(x)) { e64dac("sw2ld");
      return qsw(x) >= 0 ? SI32_MAX : -SI32_MAX; }
#ifdef HAS_I64
   return (long)x;
#else
   return (long)x.lo;
#endif
#endif

   } /* End sw2ld() */
