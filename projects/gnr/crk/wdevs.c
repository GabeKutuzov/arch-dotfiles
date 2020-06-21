/* (c) Copyright 2010, The Rockefeller University *11115* */
/* $Id: wdevs.c 15 2009-12-30 22:02:21Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                                wdevs                                 *
*                                                                      *
*  This function is one of the 'wdev' family of pseudorandom number    *
*  generators for the ROCKS system.  It generates a signed uniformly   *
*  distributed pseudorandom integer in the range 1 <= |ir| < (2**31).  *
*  The seed is passed as a pointer to a 'wseed' struct, defined in     *
*  sysdef.h.  For use in large-scale simulations, the period will be   *
*  about 2**57 if both components of the wseed are nonzero.            *
*                                                                      *
*  Synopsis:  si32 wdevs(wseed *seed)                                  *
*                                                                      *
*  Algorithm: The basic algorithm is implemented as in-line code       *
*  defined in the header wdevcom.h and is described there in detail.   *
*                                                                      *
************************************************************************
*  V1A, 01/15/10, GNR - New program                                    *
*  ==>, 01/16/10, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include <stddef.h>
#include <math.h>
#include "sysdef.h"
#include "rkwdev.h"

si32 wdevs(wseed *seed) {

#define WDEV_SIGN
#include "wdevcom.h"

   return rand;

   } /* End wdevs() */
