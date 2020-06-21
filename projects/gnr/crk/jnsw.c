/* (c) Copyright 1999-2008, The Rockefeller University *11115* */
/* $Id: jnsw.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                                jnsw                                  *
*                                                                      *
*  This function computes the two's complement of a 64-bit signed      *
*  integer with no overflow checking.                                  *
*                                                                      *
*  Synopsis: si64 jnsw(si64 x)                                         *
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
#error Do not compile jnsw.c with HAS_I64, is a macro
#endif 

si64 jnsw(si64 x) {

   si64 rv;

   rv.lo = -x.lo;
   rv.hi = ~x.hi + (x.lo == 0);

   return rv;

   } /* End jnsw() */
