/* (c) Copyright 2010, The Rockefeller University *11115* */
/* $Id: wdevf.c 15 2009-12-30 22:02:21Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                                wdevf                                 *
*                                                                      *
*  This function is one of the 'wdev' family of pseudorandom number    *
*  generators for the ROCKS system.  It generates a random float       *
*  value in the range 0 < r < 1.0.                                     *
*                                                                      *
*  The seed is passed as a pointer to a 'wseed' struct, as defined in  *
*  sysdef.h.  For use in large-scale simulations, the period will be   *
*  about 2**57 if both components of the wseed are nonzero.            *
*                                                                      *
*  Synopsis:  float wdevf(wseed *seed)                                 *
*                                                                      *
*  Algorithm: The basic algorithm is implemented as in-line code       *
*  defined in the header wdevcom.h and is described there in detail.   *
*  The random fixed-point number is simply converted to a float and    *
*  divided by 2**31.                                                   *
*                                                                      *
************************************************************************
*  V1A, 01/08/10, GNR - New program                                    *
*  ==>, 02/06/10, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include <stddef.h>
#include <math.h>
#include "sysdef.h"
#include "rkwdev.h"

float wdevf(wseed *seed) {

#include "wdevcom.h"

   return (float)rand/f31;

   } /* End wdevf() */
