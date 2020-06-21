/* (c) Copyright 2016, The Rockefeller University *11115* */
/* $Id: sw2zd.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                                sw2zd                                 *
*                                                                      *
*  This function extracts a 32-bit size_t variable from a 64-bit       *
*  signed integer with overflow checking.  Note that if a size_t is    *
*  already 64-bit, rkarith.h will define sw2zd as a macro (overflow    *
*  is not possible).                                                   *
*  This is the version that uses the error code stored by e64dec().    *
*                                                                      *
*  Synopsis: size_t sw2zd(si64 x)                                      *
*                                                                      *
*  We assume size_t is a signed type, 32 bits if ZSIZE != WSIZE        *
*                                                                      *
************************************************************************
*  R60, 05/10/16, GNR - New routine, based on sw2le                    *
*  ==>, 05/10/16, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include <stdlib.h>
#include <math.h>
#include "sysdef.h"
#include "rkarith.h"

#if ZSIZE == WSIZE
#error sw2zd should not be compiled if ZSIZE == WSIZE
#endif

long sw2zd(si64 x) {

#ifdef HAS_I64
   if (x < 0) {
      e64dac("sw2zd");
      return (size_t)0; }
   if (x > jesl(SI32_MAX) {
      e64dac("sw2zd");
      return (size_t)SI32_MAX; }
   return (size_t)x;
#else
   if (x.hi < 0) {
      e64dac("sw2zd");
      return (size_t)0; }
   if (x.hi | x.lo >> (BITSPERUI32-1)) {
      e64dac("uw2ze");
      return (size_t)SI32_MAX; }
   return (size_t)x.lo;
#endif

   } /* End sw2zd() */
