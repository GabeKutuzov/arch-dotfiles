/* (c) Copyright 2010, The Rockefeller University *11115* */
/* $Id: wndev.c 15 2009-12-30 22:02:21Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                                wndev                                 *
*                                                                      *
*  This function is one of the 'wdev' family of pseudorandom number    *
*  generators for the ROCKS system.  It generates a single normally    *
*  distributed pseudorandom number with a given mean and standard      *
*  deviation.  The seed is passed as a pointer to a 'wseed' struct,    *
*  defined in sysdef.h.  For use in large-scale simulations, the       *
*  period will be about 2**57 if both components of the wseed are      *
*  nonzero.  If the 'seed27' (hi-order) part of the seed is 0, the     *
*  output is identical to that produced by the earlier routine ndev(). *
*                                                                      *
*  Synopsis:  si32 wndev(wseed *seed, si32 mean, si32 sigma)           *
*                                                                      *
*  Algorithm: The basic algorithm is implemented as in-line code       *
*  defined in the header wdevcom.h and is described there in detail.   *
*                                                                      *
*  Scaling:  'mean' can have any desired binary scale (number of frac- *
*  tion bits, Sa).  'sigma' must have four more fraction bits than     *
*  'mean'.  The result has the same scale as 'mean'.                   *
*                                                                      *
************************************************************************
*  V1A, 01/16/10, GNR - New program, based on ndev.c and wdev.c        *
*  ==>, 02/06/10, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include <stddef.h>
#include <math.h>
#include "sysdef.h"
#include "rkarith.h"
#include "rkwdev.h"

si32 wndev(wseed *seed, si32 mean, si32 sigma) {

#define WDEV_NORM
#include "wdevcom.h"

/* Multiply by standard deviation (S28 + (Sa+4) - 32 = Sa) */

   r = jm64nh(sigma, r);
   if (rand & b22) r = -r;

/* Add mean (Sa) */

   return (r + mean);

   } /* End wndev() */
