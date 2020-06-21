/* (c) Copyright 2016, The Rockefeller University *11115* */
/* $Id: imulup.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                               imulup                                 *
*                                                                      *
*     Routine to calculate a signed integer that is an exact multiple  *
*        of a given base value and having the same sign as but greater *
*        than or equal in magnitude to an argument value.  Because the *
*        main use of this routine is expected to be rounding up values *
*        for memory allocation, we go to some trouble to detect over-  *
*        flow errors.                                                  *
*                                                                      *
*     Synopsis:  int imulup(int val, int base)                         *
*                                                                      *
*     Arguments:                                                       *
*        val     Starting value                                        *
*        base    Value of which return must be an exact multiple       *
*                (must be positive)                                    *
*                                                                      *
*     Returns:   ((val+base-1)/base)*base                              *
*                                                                      *
*     Note:  The revised overflow check assures both that the divi-    *
*        dend can not overflow ((base - 1) + |val| <= INT_MAX, making  *
*        sure 1 is subtracted before the two big numbers are added),   *
*        and the result can not overflow (because it is <= the incre-  *
*        mented dividend).                                             *
*                                                                      *
*     Errors:    Abexit 77 if overflow occurs or base is zero.         *
*                                                                      *
************************************************************************
*  V1A, 05/03/16, GNR - New routine, combo of lmulup and umulup        *
*  V1B, 06/27/16, GNR - Return 0 if val is 0                           *
*  ==>, 06/27/16, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include <math.h>
#include "sysdef.h"
#include "rkarith.h"

int imulup(int val, int base) {

   int aval, bm1;

   if (val == 0) return 0;
   if (base <= 0) abexit(77);
   aval = abs(val);
   bm1 = base - 1;
   if (aval > INT_MAX - bm1) abexit(77);
   aval = ((aval+bm1)/base)*base;
   return (val < 0 ? -aval : aval);

   } /* End imulup() */
