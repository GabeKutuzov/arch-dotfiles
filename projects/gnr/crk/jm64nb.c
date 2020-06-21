/* (c) Copyright 1999-2014, The Rockefeller University *11115* */
/* $Id: jm64nb.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                              jm64nb.c                                *
*                                                                      *
*  This routine is a C implementation of jm64nb(), which is a routine  *
*  to perform a 32 x 32 bit signed multiplication and return the high  *
*  32 bits as the function value and the low 32 bits via a pointer.    *
*  No overflow is possible.  On many systems, an assembly-language     *
*  implementation of this routine is provided for greater speed.       *
*                                                                      *
*  The xx64xx routines are implemented in Assembly language on most    *
*  machines because the lack of provision for arithmetic carry in      *
*  the C language makes implementation clumsy and execution slow.      *
*  The present C versions are intended to provide working versions     *
*  to get started on a new port.  Examination of the code emitted      *
*  by the C compiler may provide a useful template for an Assembler    *
*  implementation.                                                     *
*                                                                      *
*  Synopsis:  si32 jm64nb(si32 mul1, si32 mul2, unsigned si32* lo32)   *
*                                                                      *
************************************************************************
*  Initial C version: 06/28/99, by G. N. Reeke                         *
*  ==>, 05/24/06, GNR - Last date before committing to svn repository  *
*  Rev, 04/06/14, GNR - More careful handling of low-order sign bit    *
***********************************************************************/

#include "sysdef.h"
#include "rkarith.h"

si32 jm64nb(si32 mul1, si32 mul2, ui32 *lo32) {

   si64 prod = jmsw(mul1,mul2);
   *lo32 = swlou(prod);
   return swhi(prod);

   } /* End jm64nb() */

