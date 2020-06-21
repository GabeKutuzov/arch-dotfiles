/* (c) Copyright 2010, The Rockefeller University *11115* */
/* $Id: nwdevi.c 15 2009-12-30 22:02:21Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                               nwdevi                                 *
*                                                                      *
*  This function is one of the 'wdev' family of pseudorandom number    *
*  generators for the ROCKS system.  It generates an array of n uni-   *
*  formly distributed pseudorandom integers in the range 0 <= ir <     *
*  setsz, where setsz is an argument.  This is the general method      *
*  for picking random items with replacement from a set of setsz       *
*  items.                                                              *
*                                                                      *
*  The seed is passed as a pointer to a 'wseed' struct, defined in     *
*  sysdef.h.  For use in large-scale simulations, the period will be   *
*  about 2**57 if both components of the wseed are nonzero.            *
*                                                                      *
*  Synopsis:  void nwdevi(ui32 *ir, wseed *seed, long n, ui32 setsz)   *
*                                                                      *
*  Algorithm: The basic algorithm is implemented as in-line code       *
*  defined in the header wdevcom.h and is described there in detail.   *
*  The random number is multiplied by the setsz and the fraction is    *
*  discarded.                                                          *
*                                                                      *
************************************************************************
*  V1A, 01/16/10, GNR - New program                                    *
*  ==>, 02/07/10, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include <stddef.h>
#include <math.h>
#include "sysdef.h"
#include "rkarith.h"

void nwdevi(ui32 *ir, wseed *seed, long n, ui32 setsz) {

#ifdef HAS_I64
   ui64 setsz64 = (ui64)setsz;
#else
   ui64 rs;
#endif
   while (--n >= 0) {

#include "wdevcom.h"

#ifdef HAS_I64
      *ir++ = (ui32)(((ui64)rand * setsz64) >> 31);
#else
      rs = jmuw((ui32)rand << 1, setsz);
      *ir++ = uwhi(rs);
#endif
      }

   } /* End nwdevi() */
