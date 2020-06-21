/* (c) Copyright 2003-2013, The Rockefeller University *11115* */
/* $Id: swloe.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                                swloe                                 *
*                                                                      *
*  This function extracts the low order part of a signed 64-bit        *
*  value as a signed 32-bit value and signals overflow.                *
*                                                                      *
*  Synopsis: si32 swloe(si64 x, int ec)                                *
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
*  Rev, 10/22/13, GNR - Use jckslo, return max value on overflow       *
***********************************************************************/

#include <stdlib.h>
#include "sysdef.h"
#include "rkarith.h"

si32 swloe(si64 x, int ec) {

   if (jckslo(x)) { e64act("swloe", ec);
      return (qsw(x) >= 0) ? SI32_MAX : -SI32_MAX; }
   return swlo(x);

   } /* End swloe() */
