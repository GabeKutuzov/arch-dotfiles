/* (c) Copyright 2003-2008, The Rockefeller University *11115* */
/* $Id: jrul.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                                jrul                                  *
*                                                                      *
*  This function subtracts a 32-bit unsigned integer from a 64-bit     *
*  unsigned integers with no overflow checking.                        *
*                                                                      *
*  Synopsis: ui64 jrul(ui64 x, ui32 y)                                 *
*                                                                      *
*  This routine is part of a basic 64-bit arithmetic package for which *
*  full C implementations are provided in order to get one started on  *
*  a new processor.  These routines or the combined routines actually  *
*  used in applications can be reimplemented in Assembler if greater   *
*  speed is desired.                                                   *
*                                                                      *
************************************************************************
*  V1A, 06/28/03, GNR - New routine                                    *
*  ==>, 09/30/04, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include "sysdef.h"
#include "rkarith.h"

#ifdef HAS_I64
#error Do not compile jrul.c with HAS_I64, is a macro
#endif 

ui64 jrul(ui64 x, ui32 y) {

   ui64 rv;

   rv.lo = x.lo - y;
   rv.hi = x.hi - (y > x.lo);

   return rv;

   } /* End jrul() */
