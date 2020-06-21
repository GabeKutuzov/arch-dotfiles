/* (c) Copyright 2012-2013, The Rockefeller University *11115* */
/* $Id: uw2le.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                                uw2le                                 *
*                                                                      *
*  This functions extract a 32-bit unsigned long integer from a 64-bit *
*  unsigned integer with overflow checking.  Note that if a long is    *
*  already 64-bit, rkarith.h will define uw2le as a macro (overflow    *
*  is not possible).                                                   *
*                                                                      *
*  Synopsis: unsigned long uw2le(ui64 x, int ec)                       *
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
***********************************************************************/

#include "sysdef.h"
#include "rkarith.h"

#undef uw2le   /* Get rid of the macro */

unsigned long uw2le(ui64 x, int ec) {

#if LSIZE == 8
   abexitq(87);
   return 0;
#else
   if (uwhi(x)) {
      e64act("uw2le", ec);
      return UI32_MAX; }
   return uwlo(x);
#endif

   } /* End uw2le() */
