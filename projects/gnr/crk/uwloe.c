/* (c) Copyright 2003-2013, The Rockefeller University *11115* */
/* $Id: uwloe.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                                uwloe                                 *
*                                                                      *
*  This function extracts the low order part of an unsigned 64-bit     *
*  value as an unsigned 32-bit value and signals overflow.             *
*                                                                      *
*  Synopsis: ui32 uwloe(ui64 x, int ec)                                *
*                                                                      *
*  This routine is part of a basic 64-bit arithmetic package for which *
*  full C implementations are provided in order to get one started on  *
*  a new processor.  These routines or the combined routines actually  *
*  used in applications can be reimplemented in Assembler if greater   *
*  speed is desired.                                                   *
*                                                                      *
************************************************************************
*  V1A, 07/12/03, GNR - New routine                                    *
*  ==>, 09/30/04, GNR - Last date before committing to svn repository  *
*  V1B, 12/07/08, GNR - Correct overflow check to allow x >= 2^31      *
*  Rev, 10/22/13, GNR - Use jckulo, return max value on overflow       *
***********************************************************************/

#include "sysdef.h"
#include "rkarith.h"

ui32 uwloe(ui64 x, int ec) {

   if (jckulo(x)) { e64act("uwloe", ec); return UI32_MAX; }
   return uwlo(x);

   } /* End uwloe() */
