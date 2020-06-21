/* (c) Copyright 1999-2008, The Rockefeller University *11115* */
/* $Id: jm64sh.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                              jm64sh.c                                *
*                                                                      *
*  This program is a C implementation of jm64sh(), which is a routine  *
*  to perform a 32 x 32 bit signed multiplication followed by a shift  *
*  of the 64 bit product.  Only the high-order 32 bits of the result   *
*  are returned.  By traditional definition, this routine does not     *
*  check for overflow--it should be replaced by mrssle or mrsrsle for  *
*  safe programming (using 32 - old shift).  On many systems, an       *
*  assembly-language implementation of this routine may be provided    *
*  for greater speed.                                                  *
*                                                                      *
*  Synopsis:  si32 jm64sh(si32 mul1, si32 mul2, int ishft)             *
*                                                                      *
************************************************************************
*  Initial C version: 06/28/99, by G. N. Reeke                         *
*  ==>, 03/12/06, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include "sysdef.h"
#include "rkarith.h"

#undef jm64sh  /* Get rid of the macro */

si32 jm64sh(si32 mul1, si32 mul2, int ishft) {

   return swhi(jssw(jmsw(mul1,mul2),ishft));

   } /* End jm64sh() */

