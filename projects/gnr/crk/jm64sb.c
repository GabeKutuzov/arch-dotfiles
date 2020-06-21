/* (c) Copyright 1999-2014, The Rockefeller University *11115* */
/* $Id: jm64sb.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                              jm64sb.c                                *
*                                                                      *
*  This program is a C implementation of jm64sb(), which is a routine  *
*  to perform a 32 x 32 bit signed multiplication followed by a shift  *
*  of the 64-bit product.  The high-order 32 bits of the result are    *
*  returned as the function value and the low-order 32 bits are        *
*  returned via a pointer.  By traditional definition, this routine    *
*  does not check for overflow, but this is easy to fix by replacing   *
*  the jssw shift routine with its error-checking cousin, jsswe.  On   *
*  many systems, an assembly-language implementation of this routine   *
*  provided for greater speed.                                         *
*                                                                      *
*  Synopsis:                                                           *
*    si32 jm64sb(si32 mul1, si32 mul2, int ishft, unsigned si32 *lo32) *
*                                                                      *
************************************************************************
*  Initial C version: 06/28/99, by G. N. Reeke                         *
*  ==>, 05/24/06, GNR - Last date before committing to svn repository  *
*  Rev, 04/06/14, GNR - More careful handling of low-order sign bit    *
***********************************************************************/

#include "sysdef.h"
#include "rkarith.h"

si32 jm64sb(si32 mul1, si32 mul2, int ishft, ui32 *lo32) {

   si64 rv;

   rv = jssw(jmsw(mul1,mul2),ishft);
   *lo32 = swlou(rv);
   return swhi(rv);

   } /* End jm64sb() */

