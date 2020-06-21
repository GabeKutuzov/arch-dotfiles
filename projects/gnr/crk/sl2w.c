/* (c) Copyright 2003-2016, The Rockefeller University *11115* */
/* $Id: sl2w.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                                sl2w                                  *
*                                                                      *
*  This functions generates a 64-bit signed integer by extending a     *
*  signed long, which is 32 bits in the !HAS_I64 case handled here.    *
*                                                                      *
*  Synopsis: si64 sl2w(long lo)                                        *
*                                                                      *
************************************************************************
*  V1A, 06/15/03, GNR - New routine                                    *
*  ==>, 03/12/06, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include "sysdef.h"
#include "rkarith.h"

#ifdef HAS_I64
#error Do not compile sl2w.c with HAS_I64, is a macro
#endif
#if LSIZE != 4
#error sl2w assumes sizeof(long) == 4, not true on this system.
#endif

si64 sl2w(long lo) {

   si64 rv;
   rv.hi = SRA(lo,31);
   rv.lo = (ui32)lo;
   return rv;

   } /* End sl2w() */
