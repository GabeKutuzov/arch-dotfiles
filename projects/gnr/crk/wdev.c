/* (c) Copyright 2009-2010, The Rockefeller University *11115* */
/* $Id: wdev.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                                wdev                                  *
*                                                                      *
*  This function is one of the 'wdev' family of pseudorandom number    *
*  generators for the ROCKS system.  It generates a single uniformly   *
*  distributed pseudorandom integer in the range 1 <= ir < (2**31).    *
*  The seed is passed as a pointer to a 'wseed' struct, defined in     *
*  sysdef.h.  For use in large-scale simulations, the period will be   *
*  about 2**57 if both components of the wseed are nonzero.  If the    *
*  'seed27' (hi-order) part of the seed is 0, the output is identical  *
*  to that produced by the earlier routine udev().                     *
*                                                                      *
*  Synopsis:  si32 wdev(wseed *seed)                                   *
*                                                                      *
*  Algorithm: The basic algorithm is implemented as in-line code       *
*  defined in the header wdevcom.h and is described there in detail.   *
*                                                                      *
************************************************************************
*  V1A, 12/25/09, GNR - New program                                    *
*  ==>, 12/25/09, GNR - Last date before committing to svn repository  *
*  V1B, 01/08/10, GNR - Move base code out to wdevcom header           *
***********************************************************************/

#include <stddef.h>
#include <math.h>
#include "sysdef.h"
#include "rkwdev.h"

si32 wdev(wseed *seed) {

#include "wdevcom.h"

   return rand;

   } /* End wdev() */
