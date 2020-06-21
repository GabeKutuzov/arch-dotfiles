/* (c) Copyright 1999-2008, The Rockefeller University *11115* */
/* $Id: jm64nh.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                              jm64nh.c                                *
*                                                                      *
*  This program is a C implementation of jm64nh(), which is a routine  *
*  to perform a 32 x 32 bit signed multiplication and return the high- *
*  order 32 bits of the product.  Overflow is not possible.  On many   *
*  systems. as assembly-language implementation of this routine is     *
*  provided for greater speed.                                         *
*                                                                      *
*  Synopsis:  si32 jm64nh(si32 mul1, si32 mul2)                        *
*                                                                      *
************************************************************************
*  Initial C version: 06/28/99, by G. N. Reeke                         *
*  ==>, 03/12/06, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include "sysdef.h"
#include "rkarith.h"

#undef jm64nh  /* Get rid of the macro */

si32 jm64nh(si32 mul1, si32 mul2) {

   return swhi(jmsw(mul1,mul2));

   } /* End jm64nh() */

