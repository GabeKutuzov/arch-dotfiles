/* (c) Copyright 2010, The Rockefeller University *11115* */
/* $Id: nwdevs.c 15 2009-12-30 22:02:21Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                               nwdevs                                 *
*                                                                      *
*  This function is one of the 'wdev' family of pseudorandom number    *
*  generators for the ROCKS system.  It generates an array of n uni-   *
*  formly distributed pseudorandom signed integers in the range        *
*  1 <= |ir| < (2**31).  The seed is passed as a pointer to a 'wseed'  *
*  struct, defined in sysdef.h.  For use in large-scale simulations,   *
*  the period will be about 2**57 if both components of the wseed are  *
*  nonzero.                                                            *
*                                                                      *
*  Synopsis:  void nwdevs(si32 *ir, wseed *seed, long n)               *
*                                                                      *
*  Algorithm: The basic algorithm is implemented as in-line code       *
*  defined in the header wdevcom.h and is described there in detail.   *
*                                                                      *
************************************************************************
*  V1A, 01/16/10, GNR - New program                                    *
*  ==>, 01/16/10, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include <stddef.h>
#include <math.h>
#include "sysdef.h"
#include "rkwdev.h"

void nwdevs(si32 *ir, wseed *seed, long n) {

   while (--n >= 0) {

#define WDEV_SIGN
#include "wdevcom.h"

      *ir++ = rand;
      }

   } /* End nwdevs() */
