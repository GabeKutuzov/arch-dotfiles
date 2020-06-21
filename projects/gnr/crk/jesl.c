/* (c) Copyright 2003-2008, The Rockefeller University *11115* */
/* $Id: jesl.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                                jesl                                  *
*                                                                      *
*  This functions extends a 32-bit signed integer to form a 64-bit     *
*  signed integer.                                                     *
*                                                                      *
*  Synopsis: si64 jesl(si32 lo)                                        *
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
#error Do not compile jesl.c with HAS_I64, is a macro
#endif 

si64 jesl(si32 lo) {

   si64 rv;
   rv.hi = SRA(lo,31);
   rv.lo = lo;
   return rv;

   } /* End jesl() */
