/* (c) Copyright 1999-2008, The Rockefeller University *11115* */
/* $Id: jm64sl.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                              jm64sl.c                                *
*                                                                      *
*  This program is a C implementation of jm64sl(), which is a routine  *
*  to perform a 32 x 32 bit signed multiplication followed by a shift  *
*  of the 64 bit product.  Only the low-order 32 bits of the result    *
*  are returned.  By traditional definition, this routine does not     *
*  check for overflow--it should be replaced by mrssle or mrsrsle for  *
*  safe programming.  On many systems, an assembly-language imple-     *
*  mentation of this routine may be provided for greater speed.        *
*                                                                      *
*  Synopsis:  unsigned si32 jm64sl(si32 mul1, si32 mul2, int ishft)    *
*                                                                      *
************************************************************************
*  Initial C version: 06/28/99, by G. N. Reeke                         *
*  ==>, 05/24/06, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include "sysdef.h"
#include "rkarith.h"

#undef jm64sl  /* Get rid of the macro */

ui32 jm64sl(si32 mul1, si32 mul2, int ishft) {

   return swlo(jssw(jmsw(mul1,mul2),ishft));

   } /* End jm64sl() */

