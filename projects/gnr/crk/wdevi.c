/* (c) Copyright 2010, The Rockefeller University *11115* */
/* $Id: wdevi.c 15 2009-12-30 22:02:21Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                                wdevi                                 *
*                                                                      *
*  This function is one of the 'wdev' family of pseudorandom number    *
*  generators for the ROCKS system.  It generates a random value in    *
*  the range 0 <= ir < setsz, where setsz is an argument.  This is     *
*  the general method for picking a random item from a set of setsz    *
*  items.                                                              *
*                                                                      *
*  The seed is passed as a pointer to a 'wseed' struct, defined in     *
*  sysdef.h.  For use in large-scale simulations, the period will be   *
*  about 2**57 if both components of the wseed are nonzero.            *
*                                                                      *
*  Synopsis:  ui32 wdevi(wseed *seed, ui32 setsz)                      *
*                                                                      *
*  Algorithm: The basic algorithm is implemented as in-line code       *
*  defined in the header wdevcom.h and is described there in detail.   *
*  The random number is multiplied by the setsz and the fraction is    *
*  discarded.                                                          *
*                                                                      *
************************************************************************
*  V1A, 01/08/10, GNR - New program                                    *
*  ==>, 02/07/10, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include <stddef.h>
#include <math.h>
#include "sysdef.h"
#include "rkarith.h"
#include "rkwdev.h"

ui32 wdevi(wseed *seed, ui32 setsz) {

#include "wdevcom.h"

#ifdef HAS_I64

   return (ui32)(((ui64)rand * (ui64)setsz) >> 31);

#else

   ui64 rs = jmuw(((ui32)rand) << 1, setsz);
   return uwhi(rs);

#endif

   } /* End wdevi() */
