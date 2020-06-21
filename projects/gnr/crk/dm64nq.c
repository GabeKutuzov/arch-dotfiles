/* (c) Copyright 1999-2008, The Rockefeller University *11115* */
/* $Id: dm64nq.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                              dm64nq.c                                *
*                                                                      *
*  This program is a C implementation of dm64nq(), which is a routine  *
*  to perform 32 x 32 bit signed multiplication followed by a 64 / 32  *
*  bit signed division.  Only the quotient is returned.  Execution is  *
*  terminated by the underlying jdswq routine if the division would    *
*  overflow.  On many systems, as assembly-language implementation of  *
*  this routine is provided for greater speed.                         *
*                                                                      *
*  Synopsis:  si32 dm64nq(si32 mul1, si32 mul2, si32 div)              *
*                                                                      *
************************************************************************
*  Initial C version: 06/28/99, by G. N. Reeke                         *
*  ==>, 03/12/06, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include "sysdef.h"
#include "rkarith.h"

#undef dm64nq  /* Get rid of the macro */

si32 dm64nq(si32 mul1, si32 mul2, si32 div) {

   return jdswq(jmsw(mul1, mul2), div);

   } /* End dm64nq() */

