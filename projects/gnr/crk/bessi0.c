/* (c) Copyright 1997-2017, The Rockefeller University *11115* */
/* $Id: bessi0.c 63 2017-04-13 20:47:00Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                               bessi0                                 *
*                                                                      *
*  Calculate the modified Bessel function of the first kind of order 0 *
*  for any real argument, 'x', I0(x) = J0(ix).                         *
*                                                                      *
*  The approximation polynomials are taken from M. Abramowitz and I.A. *
*  Stegun (1964), Handbook of Mathematical Functions, Applied Mathe-   *
*  matics Series, vol. 55 (Washington: National Bureau of Standards;   *
*  reprinted (1968) by Dover Publications, New York) Par. 9.8.         *
*                                                                      *
************************************************************************
*  V1A, 02/03/97, GNR - Initial version                                *
*  ==>, 07/01/02, GNR - Last date before committing to svn repository  *
*  R63, 02/04/17, GNR - Delete XP8I dependency                         *
***********************************************************************/

#include <math.h>
#ifdef XP8I
#include <mathf.h>
#endif

#define I0CUTOFF 3.75      /* Cutoff between two approximations */

float bessi0(float x) {

   float bi0;              /* Result */
   float absx = fabs(x);   /* Absolute value of arg */
   double xx;              /* Polynomial variable */

   if (absx == 0.0F)
      bi0 = 1.0F;
   else if (absx < I0CUTOFF) {
      xx = absx/I0CUTOFF, xx*=xx;
      bi0 = (float)(1.0 + xx*(3.5156229 + xx*(3.0899424 +
         xx*(1.2067492 + xx*(0.2659732 + xx*(0.0360768 +
         xx*0.0045813))))));
      }
   else {
      xx = I0CUTOFF/absx;
      bi0 = (float)((exp(absx)/sqrt(absx)) * (0.39894228 +
         xx*(0.01328592 + xx*(0.00225319 + xx*(-0.00157565 +
         xx*(0.00916281 + xx*(-0.02057706 + xx*(0.02635537 +
         xx*(-0.01647633 + xx*0.00392377)))))))));
      }
   return bi0;
   } /* End bessi0() */

