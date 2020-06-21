/* (c) Copyright 2013, George N. Reeke, Jr. */
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
*  distributions and plots them so the 2DKS statistic can easily       *
*  be computed by hand, then calls the package routines to print       *
*  the result for comparison.  Float version is tested.                *
*                                                                      *
*  If DOPLOT == 1, requires plot library and mfdraw                    *
************************************************************************
*  V1A, 11/24/13, GNR - New program                                    *
*  ==>, 12/16/13, GNR - Last mod before committing to svn repository   *
*  Rev, 08/19/15, GNR - Add tests for epks and cgks methods            *
***********************************************************************/

#define DOPLOT 0

#define NDAT  10           /* Number of points */
#define NDIM   2           /* Number of dimensions */
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

int main() {

   void *pwk = NULL;
   double smd,scg;
   float X1[NDAT][NDIM] = { 0.3051, 0.7307, 0.6451, 0.8394, 0.5206, 0.3814,
      0.6192, 0.6048, 0.5356, 0.7301, 0.3763, 0.1217, 0.3669, 0.8108,
      0.8977, 0.3467, 0.0159, 0.0288, 0.8496, 0.9396 };
   float X2[NDAT][NDIM] = { 0.7627, 0.9658, 0.4927, 0.5549, 0.6522, 0.9731,
      0.9037, 0.0250, 0.5743, 0.7294, 0.1517, 0.0495, 0.5349, 0.0399,
      0.5000, 0.2103, 0.4760, 0.6540, 0.0490, 0.8583 };
#if DOPLOT
   float x0 = 0.5, y0 = 0.5, x1 = 7.5, y1 = 5.5;
   float xscl = 7.0;
   float yscl = 5.0;
   float dscl = 1.0/2147483648.0;
#endif
   int g1 = 0;
   int g2 = KC_MDKS|KC_EPKS;
   int i;

#if DOPLOT
   setmf(NULL, "localhost", "2DKS TEST", NULL,
      1024, 0, '8', 0, 0);
   setmovie(1,0,0);
   newplt(8.0, 6.0, 0, 0, "DEFAULT", "BLACK", "DEFAULT", 0);
#endif

#if DOPLOT
   /* Plot the data */
   pencol("GREEN");
   retrace(1);
   rect(x0, y0, x1-x0, y1-y0, 0);
   retrace(0);

   pencol("YELLOW");
   for (i=0; i<NDAT; ++i) {
      float xi = x0+xscl*X1[i][0], yi = y0+yscl*X1[i][1];
      line(xi, y0, xi, y1);
      line(x0, yi, x1, yi);
      rect(xi-.03, yi-.03, .06, .06, 0);
      }
   pencol("RED");
   for (i=0; i<NDAT; ++i) {
      float xi = x0+xscl*X2[i][0], yi = y0+yscl*X2[i][1];
      circle(xi, yi, 0.03, 0);
      }
#endif /* DOPLOT */

   /* Run the statistic */
   i = mdksallof(&pwk, &g1, &g2, NDAT, NDAT, NDIM);
   if (i > 0) abexit(i+680);
   smd = mdks1sf(pwk, (float *)X1, (float *)X2);
   if (smd < 0.0) abexit((int)(-smd)+685);
   scg = epks1sf(pwk, (float *)X1, (float *)X2);
   if (scg < 0.0) abexit((int)(-scg)+685);
   i = mdksfree(pwk);
   if (i > 0) abexit(i+687);

   /* Print the result */
   settit("TITLE Multidimensional Kolmogorov-Smirnov Statistic");
   convrt("(P1,'02D MDKS test2 returned',Q8.6)",&smd,NULL);
   convrt("(P1,'02D EPKS test2 returned',Q8.6)",&scg,NULL);
   convrt("(P1,' The correct result is 0.63246')",NULL);
#if DOPLOT
   endplt();
#endif
   cryout(RK_P1,"\0",RK_LN0+RK_FLUSH+1,NULL);
   return 0;
   }
