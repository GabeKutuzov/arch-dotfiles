/* (c) Copyright 2006-2016, The Rockefeller University *11115* */
/* $Id: lmulup.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                               lmulup                                 *
*                                                                      *
*     Routine to calculate an signed long integer that is an exact     *
*        multiple of a given base value and having the same sign as    *
*        but greater than or equal in magnitude to a given value.      *
*        Because the main use of this routine is expected to be        *
*        rounding up values for memory allocation, we go to some       *
*        trouble to detect overflow errors.                            *
*                                                                      *
*     Synopsis:  long lmulup(long val, int base)                       *
*                                                                      *
*     Arguments:                                                       *
*        val     Starting value                                        *
*        base    Value of which return must be an exact multiple       *
*                (must be positive)                                    *
*                                                                      *
*     Returns:   ((val+base-1)/base)*base                              *
*                                                                      *
*     Note:  The revised overflow check assures both that the divi-    *
*        dend can not overflow ((base - 1) + |val| <= LONG_MAX, making *
*        sure 1 is subtracted before the two big numbers are added),   *
*        and the result can not overflow (because it is <= the incre-  *
*        mented dividend).  The old test (mul <= LONG_MAX/base) took   *
*        care of the second, but not the first, requirement, and was   *
*        slower because of the extra division.                         *
*                                                                      *
*     Errors:    Abexit 77 if overflow occurs or base is zero.         *
*                                                                      *
************************************************************************
*  V1A, 02/06/06, GNR - New routine                                    *
*  ==>, 03/09/06, GNR - Last date before committing to svn repository  *
*  V1B, 12/14/08, GNR - Catch overflow when val + base > LONG_MAX      *
*  R60, 05/14/16, GNR - Make second argument a plain integer           *
*  Rev, 06/27/16, GNR - Return 0 if val is 0                           *
***********************************************************************/

#include <stdlib.h>
#include <math.h>
#include "rkarith.h"

long lmulup(long val, int base) {

   long aval, lbase, bm1;

   if (val == 0L) return 0L;
   if (base <= 0) abexit(77);
   aval = labs(val);
   bm1 = (lbase = (long)base) - 1;
   if (aval > LONG_MAX - bm1) abexit(77);
   aval = ((aval+bm1)/lbase)*lbase;
   return (val < 0 ? -aval : aval);

   } /* End lmulup() */
