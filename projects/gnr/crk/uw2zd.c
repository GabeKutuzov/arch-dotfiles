/* (c) Copyright 2016, The Rockefeller University *11115* */
/* $Id: uw2zd.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                                uw2zd                                 *
*                                                                      *
*  This function extracts a 32-bit size_t variable from a 64-bit       *
*  unsigned integer with overflow checking.  Note that if a size_t is  *
*  already 64-bit, rkarith.h will define uw2zd as a macro (ignoring    *
*  the slight possibility of overflow into the sign bit).              *
*  This is the version that uses the error code stored by e64dec().    *
*                                                                      *
*  Synopsis: size_t uw2zd(ui64 x)                                      *
*                                                                      *
*  We assume size_t is a signed type, 32 bits if ZSIZE != WSIZE        *
*                                                                      *
************************************************************************
*  R60, 05/10/16, GNR - New routine, based on uw2ld                    *
*  ==>, 05/10/16, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include <stdlib.h>
#include <math.h>
#include "sysdef.h"
#include "rkarith.h"

#if ZSIZE == WSIZE
#error uw2zd should not be compiled if ZSIZE == WSIZE
#endif

size_t uw2zd(ui64 x) {

#ifdef HAS_I64
   if (x > jeul(SI32_MAX)) {
      e64dac("uw2zd");
      return (size_t)SI32_MAX; }
   return (size_t)x;
#else
   if (x.hi | x.lo >> (BITSPERUI32-1)) {
      e64dac("uw2zd", ec);
      return (size_t)SI32_MAX; }
   return (size_t)x.lo;
#endif

   } /* End uw2zd() */
