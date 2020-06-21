/* (c) Copyright 2016, The Rockefeller University *11115* */
/* $Id: zmulup.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                               zmulup                                 *
*                                                                      *
*     Routine to calculate a size_t integer that is an exact multiple  *
*        of a given base value and greater than or equal to a given    *
*        value.  Because the main use of this routine is expected to   *
*        be rounding up values for memory allocation, we go to some    *
*        trouble to detect overflow errors.                            *
*                                                                      *
*     Synopsis:  size_t zmulup(size_t val, int base)                   *
*                                                                      *
*     Arguments:                                                       *
*        val     Starting value                                        *
*        base    Value of which return must be an exact multiple       *
*                (must be positive)                                    *
*                                                                      *
*     Returns:   ((val+base-1)/base))*base                             *
*                                                                      *
*     Note:  The revised overflow check assures both that the divi-    *
*        dend can not overflow ((base - 1) + val <= largest possible   *
*        size_t, making sure 1 is subtracted before the two big        *
*        numbers are added), and the result can not overflow (because  *
*        it is <= the incremented dividend).  We note that size_t is   *
*        probably a signed type, but valuse should never be negative.  *
*                                                                      *
*     Errors:    Abexit 77 if overflow occurs or base is zero.         *
*                                                                      *
************************************************************************
*  R60, 05/14/16, GNR - New routine, based on lmulup                   *
*  Rev, 06/27/16, GNR - Return 0 if val is 0                           *
*  ==>, 06/27/16, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include <stdlib.h>
#include <math.h>
#include "rkarith.h"

size_t zmulup(size_t val, int base) {

   size_t zbase, bm1;

   if (val <= 0) return (size_t)0;
   if (base <= 0) abexit(77);
   bm1 = (zbase = (size_t)base) - 1;
#if ZSIZE == 8
   if (val > SI64_MAX - bm1) abexit(77);
#else
   if (val > SI32_MAX - bm1) abexit(77);
#endif
   return ((val+bm1)/zbase)*zbase;

   } /* End zmulup() */
