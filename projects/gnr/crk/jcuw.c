/* (c) Copyright 1999-2008, The Rockefeller University *11115* */
/* $Id: jcuw.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                                jcuw                                  *
*                                                                      *
*  This function generates a 64-bit unsigned integer by concatenating  *
*  the high-order 32 bits with the low-order 32 bits.                  *
*                                                                      *
*  Synopsis: ui64 jcuw(ui32 hi, ui32 lo)                               *
*                                                                      *
*  This routine is part of a basic 64-bit arithmetic package for which *
*  full C implementations are provided in order to get one started on  *
*  a new processor.  These routines or the combined routines actually  *
*  used in applications can be reimplemented in Assembler if greater   *
*  speed is desired.                                                   *
*                                                                      *
************************************************************************
*  V1A, 06/19/99, GNR - New routine                                    *
*  ==>, 03/12/06, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include "sysdef.h"
#include "rkarith.h"

#ifdef HAS_I64
#error Do not compile jcuw.c with HAS_I64, is a macro
#endif 

ui64 jcuw(ui32 hi, ui32 lo) {

   ui64 rv;
   rv.hi = hi;
   rv.lo = lo;
   return rv;

   } /* End jcuw() */
