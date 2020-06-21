/* (c) Copyright 2010, The Rockefeller University *11115* */
/* $Id: nwdevf.c 15 2009-12-30 22:02:21Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                               nwdevf                                 *
*                                                                      *
*  This function is one of the 'wdev' family of pseudorandom number    *
*  generators for the ROCKS system.  It generates an array of n uni-   *
*  formly distributed random float values in the range 0 < r < 1.0.    *
*  The seed is passed as a pointer to a 'wseed' struct, as defined     *
*  in sysdef.h.  For use in large-scale simulations, the period will   *
*  be about 2**57 if both components of the wseed are nonzero.         *
*                                                                      *
*  Synopsis:  void nwdevf(float *fr, wseed *seed, long n)              *
*                                                                      *
*  Algorithm: The basic algorithm is implemented as in-line code       *
*  defined in the header wdevcom.h and is described there in detail.   *
*  The random fixed-point numbers are simply converted to floats and   *
*  divided by 2**31.                                                   *
*                                                                      *
************************************************************************
*  V1A, 01/16/10, GNR - New program                                    *
*  ==>, 02/06/10, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include <stddef.h>
#include <math.h>
#include "sysdef.h"
#include "rkwdev.h"

void nwdevf(float *fr, wseed *seed, long n) {

   while (--n >= 0) {

#include "wdevcom.h"

      *fr++ = (float)rand/f31;
      }

   } /* End nwdevf() */
