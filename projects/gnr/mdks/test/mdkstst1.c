/* (c) Copyright 2013-2016, George N. Reeke, Jr. */
/* $Id: $ */

/***********************************************************************
*           Multi-Dimensional Kolmogorov Smirnov Statistic             *
*                             mdkstest.c                               *
*                                                                      *
*  This software was written by George N. Reeke in the Laboratory of   *
*  Biological Modelling at The Rockefeller University.  Please send    *
*  any corrections, suggestions, or improvements to the author by      *
*  email to reeke@mail.rockefeller.edu for possible incorporation in   *
*  future revisions.                                                   *
*                                                                      *
*  This software is distributed under GPL, version 2.  This program is *
*  free software; you can redistribute it and/or modify it under the   *
*  terms of the GNU General Public License as published by the Free    *
*  Software Foundation; either version 2 of the License, or (at your   *
*  option) any later version. Accordingly, this program is distributed *
*  in the hope that it will be useful, but WITHOUT ANY WARRANTY; with- *
*  out even the implied warranty of MERCHANTABILITY or FITNESS FOR A   *
*  PARTICULAR PURPOSE.  See the GNU General Public License for more    *
*  details.  You should have received a copy of the GNU General Public *
*  License along with this program.  If not, see                       *
*  <http://www.gnu.org/licenses/>.                                     *
*----------------------------------------------------------------------*
*                              mdkstest                                *
*                                                                      *
*  Test routine for multidimensional Kolmogorov-Smirnov package        *
*                                                                      *
*  This routine generates a small number of points in two linear       *
*  (or different normal) distributions and computes the mdks with      *
*  the brick and brute-force methods for comparison and debugging.     *
*  Optionally, the data can be plotted (best with k == 2 and small n)  *
*  so the MDKS statistic can easily be computed by hand.  Only the     *
*  Float version is tested.                                            *
*                                                                      *
*  If DOPLOT == 1, requires plot library and mfdraw                    *
************************************************************************
*  V1A, 11/24/13, GNR - New program                                    *
*  ==>, 12/15/13, GNR - Last mod before committing to svn repository   *
*  Rev, 12/05/15, GNR - Allow to force summation method                *
*  Rev, 12/05/15, GNR - Add option to make two different ndev sets     *
*  Rev, 01/01/16, GNR - Merge bfksallo function into mdksallo, test    *
*                       making and freeing two work areas, two N's     *
*  Rev, 01/30/16, GNR - Add cgks tests                                 *
***********************************************************************/

/* Define control parameters here */
#define DOPLOT 0
#define PRINT_DATA 0
#define KDIST 1            /* 0 for uniform, 1 for normal */
#define KCGKS 1            /* 1 to test cgks, 0 not */

#define NDAT1 1000         /* Number of points */
#define NDAT2 2000         /* (Larger than NDAT1) */
#define NDIM   3           /* Number of dimensions */

#define MAIN

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "sysdef.h"
#include "rocks.h"
#include "rkxtra.h"
#include "rkarith.h"
#include "mdks.h"
#include "plots.h"
#include "normtab.h"

int main() {

   void *pwk1 = NULL, *pwk2 = NULL;
   double s1,s2,sbf;
#if KCGKS
   double scg1,scg2;
#endif
   float X1[NDAT2][NDIM],X2[NDAT2][NDIM];
#if DOPLOT
   float x0 = 0.5, y0 = 0.5, x1 = 7.5, y1 = 5.5;
   float xscl = 7.0;
   float yscl = 5.0;
#endif
   float dscl;
   si32 seed = 12345;
   int g21,g22,i;
   int N1 = NDAT1, N2 = NDAT2;

#if DOPLOT
   setmf(NULL, "localhost", "2DKS TEST", NULL,
      1024, 0, '8', 0, 0);
   setmovie(1,0,0);
   newplt(8.0, 6.0, 0, 0, "DEFAULT", "BLACK", "DEFAULT", 0);
#endif

   /* Generate the data */
#if KDIST == 1
   dscl = 1.0/16777216.0;
   cryout(RK_P1,"0Using normally distributed data", RK_LN2, NULL);
   {  si32 cmn = 20*(1<<24);
      si32 csg1 = 5*(1<<28), csg2 = 6*(1<<28);
      for (i=0; i<NDAT2; ++i) {
         int k;
         for (k=0; k<NDIM; ++k) {
            X1[i][k] = dscl*(float)ndev(&seed, cmn, csg1);
            X2[i][k] = dscl*(float)ndev(&seed, cmn, csg2);
            }
         }
      }
#else
   dscl = 1.0/2147483648.0;
   cryout(RK_P1,"0Using uniformly distributed data", RK_LN2, NULL);
   for (i=0; i<NDAT2; ++i) {
      int k;
      for (k=0; k<NDIM; ++k) {
         X1[i][k] = dscl*(float)udev(&seed);
         X2[i][k] = dscl*(float)udev(&seed);
         }
      }
#endif

#if DOPLOT
   /* Plot the data */
   pencol("GREEN");
   retrace(1);
   rect(x0, y0, x1-x0, y1-y0, 0);
   retrace(0);

   pencol("YELLOW");
   for (i=0; i<NDAT1; ++i) {
      float xi = x0+xscl*X1[i][0], yi = y0+yscl*X1[i][1];
      line(xi, y0, xi, y1);
      line(x0, yi, x1, yi);
      rect(xi-.03, yi-.03, .06, .06, 0);
      }
   pencol("RED");
   for (i=0; i<NDAT1; ++i) {
      float xi = x0+xscl*X2[i][0], yi = y0+yscl*X2[i][1];
      circle(xi, yi, 0.03, 0);
      }
#endif /* DOPLOT */

   settit("TITLE Multidimensional Kolmogorov-Smirnov Statistic");
#if PRINT_DATA
   for (i=0; i<NDAT2; ++i) {
      convrt("(P1,' X1[',J0I3,']: ',2F8.4,'   X2: ',2F8.4)",
         &i, X1[i], X2[i], NULL);
      }
#endif

   /* Run the statistic for N1 */
   g21 = KC_MDKS | KC_BFKS | KC_QS1;
   g22 = KC_MDKS | KC_QS2;
#if KCGKS
   g21 |= KC_CGKS; g22 |= KC_CGKS;
#endif
   i = mdksallof(&pwk1, NULL, &g21, NDAT1, NDAT1, NDIM);
   if (i > 0) abexitm(i+680,"mdksallof(1) returned error");
   i = mdksallof(&pwk2, NULL, &g22, NDAT1, NDAT1, NDIM);
   if (i > 0) abexitm(i+680,"mdksallof(2) returned error");
   s1 = mdks1sf(pwk1, (float *)X1, (float *)X2);
   if (s1 < 0.0) abexitm((int)(-s1)+685,"mdks1sf(1) returned error");
   s2 = mdks1sf(pwk2, (float *)X1, (float *)X2);
   if (s2 < 0.0) abexitm((int)(-s1)+685,"mdks1sf(2) returned error");
   sbf = bfks1sf(pwk1, (float *)X1, (float *)X2);
   if (sbf < 0.0) abexitm((int)(-sbf)+685,"bfks1sf returned error");
#if KCGKS
   scg1 = cgks1sf(pwk1, (float *)X1, (float *)X2);
   if (scg1 < 0.0) abexitm((int)(-s1)+685,"cgks1sf(1) returned error");
   scg2 = cgks1sf(pwk2, (float *)X1, (float *)X2);
   if (scg2 < 0.0) abexitm((int)(-s1)+685,"cgks1sf(2) returned error");
#endif

   /* Print the result */
   convrt("(P1,'0Results for N = ',J0I6)",&N1,NULL);
   convrt("(P1,'0MDKS(1 quads) test returned',Q8.6)",&s1,NULL);
   convrt("(P1,' MDKS(2 quads) test returned',Q8.6)",&s2,NULL);
#if KCGKS
   convrt("(P1,'0CGKS(1 quads) test returned',Q8.6)",&scg1,NULL);
   convrt("(P1,' CGKS(2 quads) test returned',Q8.6)",&scg2,NULL);
#endif
   convrt("(P1,' The brute force result is',Q8.6)",&sbf,NULL);

   /* Repeat the whole thing for larger N2 to test work area realloc */
   i = mdksallof(&pwk1, NULL, &g21, NDAT2, NDAT2, NDIM);
   if (i > 0) abexitm(i+680,"mdksallof(1) returned error");
   i = mdksallof(&pwk2, NULL, &g22, NDAT2, NDAT2, NDIM);
   if (i > 0) abexitm(i+680,"mdksallof(2) returned error");
   s1 = mdks1sf(pwk1, (float *)X1, (float *)X2);
   if (s1 < 0.0) abexitm((int)(-s1)+685,"mdks1sf(1) returned error");
   s2 = mdks1sf(pwk2, (float *)X1, (float *)X2);
   if (s2 < 0.0) abexitm((int)(-s1)+685,"mdks1sf(2) returned error");
   sbf = bfks1sf(pwk1, (float *)X1, (float *)X2);
   if (sbf < 0.0) abexitm((int)(-sbf)+685,"bfks1sf returned error");
#if KCGKS
   scg1 = cgks1sf(pwk1, (float *)X1, (float *)X2);
   if (scg1 < 0.0) abexitm((int)(-s1)+685,"cgks1sf(1) returned error");
   scg2 = cgks1sf(pwk1, (float *)X1, (float *)X2);
   if (scg2 < 0.0) abexitm((int)(-s1)+685,"cgks1sf(2) returned error");
#endif

   /* Print the result */
   convrt("(P1,'0Results for N = ',J0I6)",&N2,NULL);
   convrt("(P1,'0MDKS(1 quads) test returned',Q8.6)",&s1,NULL);
   convrt("(P1,' MDKS(2 quads) test returned',Q8.6)",&s2,NULL);
#if KCGKS
   convrt("(P1,'0CGKS(1 quads) test returned',Q8.6)",&scg1,NULL);
   convrt("(P1,' CGKS(2 quads) test returned',Q8.6)",&scg2,NULL);
#endif
   convrt("(P1,' The brute force result is',Q8.6)",&sbf,NULL);

   /* Test ability to free more than one work area */
   i = mdksfree(pwk1);
   if (i > 0) abexitm(i+687,"mdksfree(1) returned error");
   i = mdksfree(pwk2);
   if (i > 0) abexitm(i+687,"mdksfree(2) returned error");
#if DOPLOT
   endplt();
#endif
   cryocls();
   return 0;
   }
