/* (c) Copyright 2013, The Rockefeller University *11115* */
/* $Id: dbl2swd.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                               dbl2swd                                *
*                                                                      *
*  This routine converts a double-precision floating-point number to   *
*  an si64 fixed-point number with overflow checking.                  *
*  This is the version that uses the error code stored by e64dec().    *
*                                                                      *
*  Synopsis: si64 dbl2swd(double dx)                                   *
*                                                                      *
*  This routine is part of a basic 64-bit arithmetic package for which *
*  full C implementations are provided in order to get one started on  *
*  a new processor.  These routines or the combined routines actually  *
*  used in applications can be reimplemented in Assembler if greater   *
*  speed is desired.                                                   *
*                                                                      *
************************************************************************
*  V1A, 10/21/13, GNR - New routine                                    *
*  ==>, 10/21/13, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include <stdlib.h>
#include <math.h>
#include "sysdef.h"
#include "rkarith.h"

#define TWO32  4294967296.0
#define TWO63  9223372036854775808.0

si64 dbl2swd(double dx) {

   if (fabs(dx) >= TWO63) {
      e64dac("dbl2swd");
      return (dx > 0) ? SI64_MAX : jnsw(SI64_MAX);
      }
#ifdef HAS_I64
   return (si64)dx;
#else
   {  double adx = abs(dx);
      si64 r;
      r.hi = (long)(adx / TWO32);
      r.lo = (unsigned long)(adx - TWO32*(double)r.hi);
      if (dx < 0.0) r = jnsw(r);
      return r; }
#endif

   } /* End dbl2swd() */
