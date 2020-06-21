/* (c) Copyright 2003-2008, The Rockefeller University *11115* */
/* $Id: jeul.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                                jeul                                  *
*                                                                      *
*  This function generates a 64-bit unsigned integer by extending a    *
*  32 bit unsigned integer.                                            *
*                                                                      *
*  Synopsis: ui64 jeul(ui32 lo)                                        *
*                                                                      *
*  This routine is part of a basic 64-bit arithmetic package for which *
*  full C implementations are provided in order to get one started on  *
*  a new processor.  These routines or the combined routines actually  *
*  used in applications can be reimplemented in Assembler if greater   *
*  speed is desired.                                                   *
*                                                                      *
************************************************************************
*  V1A, 06/15/03, GNR - New routine                                    *
*  ==>, 03/12/06, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include "sysdef.h"
#include "rkarith.h"

#ifdef HAS_I64
#error Do not compile jeul.c with HAS_I64, is a macro
#endif 

ui64 jeul(ui32 lo) {

   ui64 rv;
   rv.hi = 0;
   rv.lo = lo;
   return rv;

   } /* End jeul() */
