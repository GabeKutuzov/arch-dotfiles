/* (c) Copyright 1997-2008, The Rockefeller University *11115* */
/* $Id: erfcf.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                                erfcf                                 *
*                                                                      *
*  Synopsis:  float erfcf(float x)                                     *
*                                                                      *
*  This function computes the single-precision complement of the error *
*  function of a single precision argument x, erfc(x) = 1.0 - erf(x).  *
*  Intermediate calculations are in double precision.  The name is     *
*  chosen to avoid conflict with any other erfc function on a system.  *
*                                                                      *
*  The approximation polynomial is taken from M. Abramowitz and I.A.   *
*  Stegun (1964), Handbook of Mathematical Functions, Applied Mathe-   *
*  matics Series, vol. 55 (Washington: National Bureau of Standards;   *
*  reprinted (1968) by Dover Publications, New York) Par. 7.1.26.      *
*                                                                      *
*  Error:  Absolute error is claimed to be less than 1.5E-7            *
*                                                                      *
************************************************************************
*  V1A, 02/15/97, GNR - Initial version                                *
*  ==>, 06/04/99, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include <math.h>

float erfcf(float x) {

   double xx = (double)x;
   double t = 1.0/(1.0 + 0.3275911*xx);

   return (float)(exp(-xx*xx) * t * (0.254829592 + t * (-0.284496736 +
      t * (1.421413741 + t * (-1.453152027 + t * 1.061405429)))));

   } /* End erfcf() */

