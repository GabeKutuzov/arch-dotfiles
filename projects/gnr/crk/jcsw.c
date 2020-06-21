/* (c) Copyright 1999-2008, The Rockefeller University *11115* */
/* $Id: jcsw.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                                jcsw                                  *
*                                                                      *
*  This function generates a 64-bit signed integer by concatenating    *
*  the high-order 32 bits with the low-order 32 bits.                  *
*                                                                      *
*  Synopsis: si64 jcsw(si32 hi, ui32 lo)                               *
*                                                                      *
*  This routine is part of a basic 64-bit arithmetic package for which *
*  full C implementations are provided in order to get one started on  *
*  a new processor.  These routines or the combined routines actually  *
*  used in applications can be reimplemented in Assembler if greater   *
*  speed is desired.                                                   *
*                                                                      *
************************************************************************
*  V1A, 06/19/99, GNR - New routine                                    *
*  ==>, 09/27/08, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include "sysdef.h"
#include "rkarith.h"

#ifdef HAS_I64
#error Do not compile jcsw.c with HAS_I64, is a macro
#endif 

si64 jcsw(si32 hi, ui32 lo) {

   si64 rv;
   rv.hi = hi;
   rv.lo = lo;
   return rv;

   } /* End jcsw() */
