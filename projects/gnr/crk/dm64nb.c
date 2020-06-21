/* (c) Copyright 1999-2008, The Rockefeller University *11115* */
/* $Id: dm64nb.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                              dm64nb.c                                *
*                                                                      *
*  This program is a C implementation of dm64nb(), which is a routine  *
*  to perform 32 x 32 bit signed multiplication followed by a 64 / 32  *
*  bit signed division.  Both quotient and remainder are returned.     *
*  Execution is terminated by the underlying jdswb routine if the      *
*  division would overflow.  On many systems, an assembly-language     *
*  implementation of this routine is provided for greater speed.       *
*                                                                      *
*  Note that division rounds towards zero, so the remainder always     *
*  has the sign of the product.                                        *
*                                                                      *
*  Synopsis:  si32 dm64nb(si32 mul1, si32 mul2, si32 div, si32 *rem)   *
*                                                                      *
************************************************************************
*  Initial C version: 06/28/99, by G. N. Reeke                         *
*  ==>, 10/02/08, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include "sysdef.h"
#include "rkarith.h"

#undef dm64nb  /* Get rid of the macro */

si32 dm64nb(si32 mul1, si32 mul2, si32 div, si32 *rem) {

   return jdswb(jmsw(mul1, mul2), div, rem);

   } /* End dm64nb() */
