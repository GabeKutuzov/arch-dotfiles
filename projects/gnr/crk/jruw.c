/* (c) Copyright 1999-2008, The Rockefeller University *11115* */
/* $Id: jruw.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                                jruw                                  *
*                                                                      *
*  This function subtracts two 64-bit unsigned integers with no        *
*  overflow checking.  It is the same as jrsw except for arg types.    *
*                                                                      *
*  Synopsis: ui64 jruw(ui64 x, ui64 y)                                 *
*                                                                      *
*  This routine is part of a basic 64-bit arithmetic package for which *
*  full C implementations are provided in order to get one started on  *
*  a new processor.  These routines or the combined routines actually  *
*  used in applications can be reimplemented in Assembler if greater   *
*  speed is desired.                                                   *
*                                                                      *
************************************************************************
*  V1A, 06/19/99, GNR - New routine                                    *
*  ==>, 07/01/02, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include "sysdef.h"
#include "rkarith.h"

#ifdef HAS_I64
#error Do not compile jruw.c with HAS_I64, is a macro
#endif 

ui64 jruw(ui64 x, ui64 y) {

   ui64 rv;

   rv.lo = x.lo - y.lo;
   rv.hi = x.hi - y.hi - (y.lo > x.lo);

   return rv;

   } /* End jruw() */
