/* (c) Copyright 1999-2008, The Rockefeller University *11115* */
/* $Id: ds64nq.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                              ds64nq.c                                *
*                                                                      *
*  This program is a C implementation of ds64nq(), which is a routine  *
*  to perform 64 / 32 bit signed division on the shifted concatenation *
*  of two long operands.  Only the quotient is returned.  Execution is *
*  terminated by the underlying jdswq routine if the division would    *
*  overflow.  Overflow in the shift operation are traditionally not    *
*  detected, but this is easy to fix by using jsswe instead of jssw.   *
*  On many systems, an assembly-language implementation of this        *
*  routine is provided for greater speed.                              *
*                                                                      *
*  Synopsis:                                                           *
*    si32 ds64nq(si32 hi32, unsigned si32 lo32, int ishft, si32 div)   *
*                                                                      *
************************************************************************
*  Initial C version: 06/28/99, by G. N. Reeke                         *
*  ==>, 10/02/08, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include "sysdef.h"
#include "rkarith.h"

#undef ds64nq  /* Get rid of the macro */

si32 ds64nq(si32 hi32, ui32 lo32, int ishft, si32 div) {

   return jdswq(jssw(jcsw(hi32,lo32),ishft),div);

   } /* End ds64nq() */
