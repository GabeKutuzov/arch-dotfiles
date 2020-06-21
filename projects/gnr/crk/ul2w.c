/* (c) Copyright 2016, The Rockefeller University *11115* */
/* $Id: ul2w.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                                ul2w                                  *
*                                                                      *
*  This function generates a 64-bit unsigned integer by extending an   *
*  unsigned long, which is 32 bits in the !HAS_I64 case handled here.  *
*  32 bit unsigned integer.                                            *
*                                                                      *
*  Synopsis: ui64 ul2w(ulng lo)                                        *
*                                                                      *
************************************************************************
*  V1A, 05/11/16, GNR - New routine                                    *
*  ==>, 05/11/16, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include "sysdef.h"
#include "rkarith.h"

#ifdef HAS_I64
#error Do not compile ul2w.c with HAS_I64, is a macro
#endif
#if LSIZE != 4
#error ul2w assumes sizeof(long) == 4, not true on this system.
#endif 

ui64 ul2w(ulng lo) {

   ui64 rv;
   rv.hi = 0;
   rv.lo = lo;
   return rv;

   } /* End ul2w() */
