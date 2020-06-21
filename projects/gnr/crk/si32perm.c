/* (c) Copyright 2011, The Rockefeller University *11115* */
/* $Id: si32perm.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                              si32perm                                *
*                                                                      *
*     Routine to generate a random permutation of a stored list of n   *
*     si32 integers, given n with 2 <= n <= (2^31-1) and a udev seed.  *
*                                                                      *
*     Synopsis:  void si32perm(si32 *pval, si32 *seed, si32 n)         *
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

void si32perm(si32 *pval, si32 *seed, si32 n) {

   si32 i,j,k;

   for (i=n; i>1; --i) {
      j = udev(seed) % i;
      k = pval[i-1];
      pval[i-1] = pval[j];
      pval[j] = k;
      }

   } /* End si32perm() */
