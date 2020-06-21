/* (c) Copyright 2016, The Rockefeller University *11115* */
/* $Id: jmuzje.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                               jmuzje                                 *
*                                                                      *
*  This function multiplies a size_t value by a 32-bit unsigned        *
*  integer and returns the unsigned size_t product with full           *
*  overflow checking.                                                  *
*  This is the version that calls e64act() when there is an error.     *
*                                                                      *
*  Synopsis: size_t jmuzje(size_t z, ui32 y, int ec)                   *
*                                                                      *
*  This routine is part of a basic 64-bit arithmetic package for which *
*  full C implementations are provided in order to get one started on  *
*  a new processor.  These routines or the combined routines actually  *
*  used in applications can be reimplemented in Assembler if greater   *
*  speed is desired.                                                   *
*                                                                      *
************************************************************************
*  V1A, 05/20/16, GNR - New routine, based on jmuwjd                   *
*  ==>, 05/20/16, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include "sysdef.h"
#include "rkarith.h"

#if ZSIZE == 8
#define Z_MAX (size_t)SI64_MAX
#else
#define Z_MAX (size_t)SI32_MAX
#endif

size_t jmuzje(size_t z, ui32 y, int ec) {

   size_t mxz = Z_MAX/y;

   if (z > mxz) {
      e64act("jmuzje", ec);
      return Z_MAX;
      }
   return z*y;

   } /* End jmuzje() */
