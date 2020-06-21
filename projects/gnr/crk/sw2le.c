/* (c) Copyright 2012-2013, The Rockefeller University *11115* */
/* $Id: sw2le.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                                sw2le                                 *
*                                                                      *
*  This functions extract a 32-bit signed long integer from a 64-bit   *
*  signed integer with overflow checking.  Note that if a long is      *
*  already 64-bit, rkarith.h will define sw2le as a macro (overflow    *
*  is not possible).                                                   *
*                                                                      *
*  Synopsis: long sw2le(si64 x, int ec)                                *
*                                                                      *
*  This routine is part of a basic 64-bit arithmetic package for which *
*  full C implementations are provided in order to get one started on  *
*  a new processor.  These routines or the combined routines actually  *
*  used in applications can be reimplemented in Assembler if greater   *
*  speed is desired.                                                   *
*                                                                      *
************************************************************************
*  V1A, 08/21/12, GNR - New routine                                    *
*  ==>, 08/21/12, GNR - Last date before committing to svn repository  *
*  Rev, 06/09/13, GNR - Deal with return from abexit in tests          *
*  Rev, 10/25/13, GNR - Use jckslo()                                   *
***********************************************************************/

#include <stdlib.h>
#include <math.h>
#include "sysdef.h"
#include "rkarith.h"

#undef sw2le   /* Get rid of the macro */

long sw2le(si64 x, int ec) {

#if LSIZE == 8
   abexitq(87);
   return 0;
#else
   if (jckslo(x)) { e64act("sw2le", ec);
      return qsw(x) >= 0 ? SI32_MAX : -SI32_MAX; }
#ifdef HAS_I64
   return (long)x;
#else
   return (long)x.lo;
#endif
#endif

   } /* End sw2le() */
