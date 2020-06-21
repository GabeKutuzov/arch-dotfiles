/* (c) Copyright 2016, The Rockefeller University *11115* */
/* $Id: ymulup.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                               ymulup                                 *
*                                                                      *
*     Routine to calculate an unsigned 64-bit integer that is an exact *
*        multiple of a given base value and greater than or equal to a *
*        given value. Because the main use of this routine is expected *
*        to be rounding up values for memory allocation, we go to some *
*        trouble to detect overflow.                                   *
*                                                                      *
*     Synopsis:  ui64 ymulup(ui64 val, int base)                       *
*                                                                      *
*     Arguments:                                                       *
*        val     Starting value                                        *
*        base    Value of which return must be an exact multiple       *
*                (must be positive)                                    *
*                                                                      *
*     Returns:   ((val+base-1)/base)*base                              *
*                                                                      *
*     Note:  The revised overflow check assures both that the divi-    *
*        dend can not overflow ((base - 1) + val <= UI64_MAX, making   *
*        sure 1 is subtracted before the two big numbers are added),   *
*        and the result can not overflow (because it is <= the incre-  *
*        mented dividend).                                             *
*                                                                      *
*     Errors:    Abexit 77 if overflow occurs or base is zero.         *
*                                                                      *
************************************************************************
*  R60, 05/15/16, GNR - New routine, based on lmulup()                 *
*  Rev, 06/27/16, GNR - Return 0 if val is 0                           *
*  ==>, 06/27/16, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include <math.h>
#include "sysdef.h"
#include "rkarith.h"

ui64 ymulup(ui64 val, int base) {

   ui32 ubase, bm1;

   if (quw(val) == 0) return jeul(0);
   if (base <= 0) abexit(77);
   bm1 = (ubase = (ui32)base) - 1;
   if (qcuw(jrul(UI64_MAX, bm1), val)) abexit(77);
   val = jaul(val, bm1);
   return jmuwj(jduwjq(val,ubase),ubase);

   } /* End yvalup() */
