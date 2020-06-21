/* (c) Copyright 2010, The Rockefeller University *11115* */
/* $Id: nwndevf.c 15 2009-12-30 22:02:21Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                               nwndevf                                *
*                                                                      *
*  This function is one of the 'wdev' family of pseudorandom number    *
*  generators for the ROCKS system.  It generates an array of normally *
*  distributed pseudorandom floats with a given mean and standard      *
*  deviation.  The seed is passed as a pointer to a 'wseed' struct,    *
*  defined in sysdef.h.  For use in large-scale simulations, the       *
*  period will be about 2**57 if both components of the wseed are      *
*  nonzero.                                                            *
*                                                                      *
*  Synopsis:  void nwndevf(float *fr, wseed *seed, long n,             *
*                 float mean, float sigma)                             *
*                                                                      *
*  Algorithm: The basic algorithm is implemented as in-line code       *
*  defined in the header wdevcom.h and is described there in detail.   *
*                                                                      *
************************************************************************
*  V1A, 01/16/10, GNR - New program, based on ndev.c and wndev.c       *
*  ==>, 01/16/10, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include <stddef.h>
#include <math.h>
#include "sysdef.h"
#include "rkarith.h"
#include "rkwdev.h"

void nwndevf(float *fr, wseed *seed, long n,
      float mean, float sigma) {

   while (--n >= 0) {

#define WDEV_NORM
#include "wdevcom.h"

/* Multiply by standard deviation and add mean (r is S28) */

      if (rand & b22) r = -r;
      *fr++ = (((float)r * sigma / 268435456.0) + mean);
      }

   } /* End nwndevf() */
