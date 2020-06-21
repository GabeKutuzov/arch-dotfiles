/* (c) Copyright 1999-2008, The Rockefeller University *11115* */
/* $Id: dm64nr.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                              dm64nr.c                                *
*                                                                      *
*  This program is a C implementation of dm64nr(), which is a routine  *
*  to perform 32 x 32 bit signed multiplication followed by a 64 / 32  *
*  bit signed division. Only the remainder is returned.  Execution is  *
*  terminated by the underlying jdswb routine if the division would    *
*  overflow.  On many systems, an assembly-language implementation of  *
*  this routine is provided for greater speed.                         *
*                                                                      *
*  Note that division rounds towards zero, so the remainder always     *
*  has the sign of the product.                                        *
*                                                                      *
*  Synopsis:  si32 dm64nr(si32 mul1, si32 mul2, si32 div)              *
*                                                                      *
************************************************************************
*  Initial C version: 06/28/99, by G. N. Reeke                         *
*  ==>, 03/12/06, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include "sysdef.h"
#include "rkarith.h"

#ifdef HAS_I64
#error Do not compile dm64nr.c with HAS_I64, is a macro
#endif 

si32 dm64nr(si32 mul1, si32 mul2, si32 div) {

   si32 r;
   jdswb(jmsw(mul1, mul2), div, &r);
   return r;

   } /* End dm64nr() */

