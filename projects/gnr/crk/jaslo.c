/* (c) Copyright 2016, The Rockefeller University *11115* */
/* $Id: jaslo.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                                jaslo                                 *
*                                                                      *
*  This function adds a signed long integer to a signed 64-bit integer *
*  with no overflow checking.                                          *
*                                                                      *
*  Synopsis: si64 jaslo(si64 x, long y)                                *
*                                                                      *
*  Note:  In the !HAS_I64 case handled here, a long must be 32 bits    *
*                                                                      *
************************************************************************
*  R60, 05/13/16, GNR - New routine                                    *
*  ==>, 05/13/16, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include "sysdef.h"
#include "rkarith.h"

#ifdef HAS_I64
#error Do not compile jaslo.c with HAS_I64, is a macro
#endif 

si64 jaslo(si64 x, long y) {

   si64 rv;

   rv.lo = x.lo + (ui32)y;
   rv.hi = x.hi - (y < 0) + (~(ui32)y < x.lo);

   return rv;

   } /* End jaslo() */
