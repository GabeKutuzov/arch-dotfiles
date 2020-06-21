/* (c) Copyright 1997-2008, The Rockefeller University *11115* */
/* $Id: rerfc.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                                rerfc                                 *
*                                                                      *
*  This routine calculates the complement of the standard error        *
*  integral, erfc(x) for any real argument x, erfc(x) = 1.0 - erf(x).  *
*                                                                      *
*  Synopsis:  float y = rerfc(float x)                                 *
*                                                                      *
*  Prototyped in:  rkarith.h                                           *
*                                                                      *
*  The Chebyshev approximation polynomials are taken from W.J. Cody,   *
*  "Rational Chebyshev Approximations for the Error Function",         *
*  Mathematics of Computation, 23(107) 631-637 (1969).                 *
*                                                                      *
************************************************************************
*  V1A, 03/08/97, GNR - Initial version                                *
*  Rev, 12/07/08, GNR - Check OK for 64-bit use                        *
*  ==>, 12/07/08, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include <math.h>
#include "rkarith.h"

#define TINY_X   0.000001F
#define RANGE_1  0.5F
#define RANGE_2  4.0F
#define RANGE_3 10.0F
#define RSQRTPI  0.56418958354776

float rerfc(float xarg) {

   /* Coefficients for polynomials for 0.0 <= x < RANGE_1 */
   static double p1[3] = {
         2.13853322378E+1, 1.72227577039E+0, 3.16652890658E-1 };
   static double q1[2] = { 1.89522572415E+1, 7.84374570830E+0 };

   /* Coefficients for polynomials for RANGE_1 <= x < RANGE_2 */
   static double p2[5] = { 7.3738883116E+0,  6.8650184849E+0,
         3.0317993362E+0,  5.6316961891E-1,  4.3187787405E-5 };
   static double q2[4] = { 7.3739608908E+0,  1.5184908190E+1,
         1.2795529509E+1,  5.3542167949E+0 };

   /* Coefficients for polynomials for RANGE_2 <= x < RANGE_3 */
   static double p3[3] = {
        -4.25799643553E-2,-1.96068973726E-1,-5.16882262185E-2 };
   static double q3[2] = { 1.50942070545E-1, 9.21452411694E-1 };

   float xabs = fabs(xarg);
   double num,den,xsq,rxsq,res;

   if (xabs < RANGE_2) {   /* Binary search on ranges */

      if (xabs < RANGE_1) {
         /* Calculate approximation to erf(x) in first range */
         if (xabs >= TINY_X) {
            xsq = xarg*xarg;
            num = p1[0] + xsq*(p1[1] + xsq*p1[2]);
            den = q1[0] + xsq*(q1[1] + xsq);
            res = 1.0 - xarg*num/den;
            }
         /* If xsq < precision of p1, can skip all but first term. */
         else
            res = 1.0 - xarg*(p1[0]/q1[0]);
         }

      else {
         /* Calculate approximation to erfc(x) in second range */
         xsq = xarg*xarg;
         num = p2[0] +
            xabs*(p2[1] + xabs*(p2[2] + xabs*(p2[3] + xabs*p2[4])));
         den = q2[0] +
            xabs*(q2[1] + xabs*(q2[2] + xabs*(q2[3] + xabs)));
         res = exp(-xsq)*num/den;
         }

      } /* End lower two ranges */

   else {

      if (xabs < RANGE_3) {
         /* Calculate approximation to erfc(x) in third range */
         xsq = xarg*xarg;
         rxsq = 1.0/xsq;
         num = p3[0] + rxsq*(p3[1] + rxsq*p3[2]);
         den = q3[0] + rxsq*(q3[1] + rxsq);
         res = exp(-xsq)*(RSQRTPI + rxsq*num/den)/xabs;
         }

      else
         res = 0.0;

      } /* End higher two ranges */

   return (float)((xarg < 0.0F) ? (2.0 - res) : res);
   } /* End rerfc() */

