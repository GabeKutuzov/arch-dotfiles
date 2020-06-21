/* (c) Copyright 2006-2016, The Rockefeller University *11115* */
/* $Id: umulup.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                               umulup                                 *
*                                                                      *
*     Routine to calculate an unsigned 32-bit integer that is an exact *
*        multiple of a given base value and greater than or equal to a *
*        given value. Because the main use of this routine is expected *
*        to be rounding up values for memory allocation, we go to some *
*        trouble to detect overflow.                                   *
*                                                                      *
*     Synopsis:  ui32 umulup(ui32 val, int base)                       *
*                                                                      *
*     Arguments:                                                       *
*        val     Starting value                                        *
*        base    Value of which return must be an exact multiple       *
*                (must be positive)                                    *
*                                                                      *
*     Returns:   ((val+base-1)/base))*base                             *
*                                                                      *
*     Note:  The revised overflow check assures both that the divi-    *
*        dend can not overflow ((base - 1) + val <= UI32_MAX, making   *
*        sure 1 is subtracted before the two big numbers are added),   *
*        and the result can not overflow (because it is <= the incre-  *
*        mented dividend).  The old test (mul <= UI32_MAX/base) took   *
*        care of the second, but not the first, requirement, and was   *
*        slower because of the extra division.                         *
*                                                                      *
*     Errors:    Abexit 77 if overflow occurs or base is zero.         *
*                                                                      *
************************************************************************
*  V1A, 02/06/06, GNR - New routine                                    *
*  ==>, 02/08/06, GNR - Last date before committing to svn repository  *
*  V1B, 12/14/08, GNR - Catch overflow when val + base > UI32_MAX      *
*  R60, 05/14/16, GNR - Make second argument a plain integer           *
*  Rev, 06/27/16, GNR - Return 0 if val is 0                           *
***********************************************************************/

#include <math.h>
#include "sysdef.h"
#include "rkarith.h"

ui32 umulup(ui32 val, int base) {

   ui32 ubase, bm1;

   if (val == 0) return 0;
   if (base <= 0) abexit(77);
   bm1 = (ubase = (ui32)base) - 1;
   if (val > UI32_MAX - bm1) abexit(77);
   return ((val+bm1)/ubase)*ubase;

   } /* End uvalup() */
