/* (c) Copyright 2013, The Rockefeller University *11115* */
/* $Id: dbl2uwd.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                               dbl2uwd                                *
*                                                                      *
*  This routine converts a double-precision floating-point number to   *
*  a ui64 unsigned fixed-point number with overflow checking.          *
*  This is the version that uses the error code stored by e64dec().    *
*                                                                      *
*  Synopsis: ui64 dbl2uwd(double dx)                                   *
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
#define TWO64  18446744073709551616.0

ui64 dbl2uwd(double dx) {

   if (dx < 0.0 || dx >= TWO64) {
      e64dac("dbl2uwd");
      return UI64_MAX;
      }
#ifdef HAS_I64
   return (ui64)dx;
#else
   {  ui64 r;
      r.hi = (unsigned long)(dx / TWO32);
      r.lo = (unsigned long)(dx - TWO32*(double)r.hi);
      return r; }
#endif

   } /* End dbl2uwd() */
