/* (c) Copyright 2011, George N. Reeke, Jr. */
/* $Id: si16perm.c 27 2011-12-24 03:32:56Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                              si16perm                                *
*                                                                      *
*     Routine to generate a random permutation of a stored list of n   *
*     si16 integers, given n with 2 <= n <= (2^31-1) and a udev seed.  *
*                                                                      *
*     Synopsis:  void si16perm(si16 *pval, si32 *seed, si32 n)         *
*                                                                      *
*     Arguments:                                                       *
*        pval     Pointer to the array to be permuted.                 *
*        seed     Pointer to a random number seed that will be         *
*                 updated with the result of n calls to udev(seed).    *
*        n        Size of the permutation.                             *
*                                                                      *
*     Errors:    Abexit 79 if n < 2.                                   *
*                                                                      *
************************************************************************
*  V1A, 11/14/11, GNR - New routine                                    *
*  ==>, 11/21/11, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include <math.h>
#include "sysdef.h"
#include "rkarith.h"

void si16perm(si16 *pval, si32 *seed, si32 n) {

   si32 i,j;
   si16 k;

   for (i=n; i>1; --i) {
      j = udev(seed) % i;
      k = pval[i-1];
      pval[i-1] = pval[j];
      pval[j] = k;
      }

   } /* End si16perm() */
