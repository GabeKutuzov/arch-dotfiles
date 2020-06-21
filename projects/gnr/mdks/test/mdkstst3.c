/* (c) Copyright 2015, George N. Reeke, Jr. */
/* $Id: $ */

/***********************************************************************
*           Multi-Dimensional Kolmogorov Smirnov Statistic             *
*                             mdkstst3.c                               *
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
*                              mdkstst3                                *
*                                                                      *
*  Test routine for multidimensional Kolmogorov-Smirnov package        *
*                                                                      *
*  This is the tree-dimensional version of mdkstst2.  It skips the     *
*  graphics because hard to do in 3D...  It just calls one of the      *
*  mdks family of routines with 10 data points, assuming a lot of      *
*  debug print is turned on, and prints the result.  The float         *
*  version is tested.                                                  *
************************************************************************
*  V1A, 08/19/15, GNR - New program, derived from mdkstst2             *
*  ==>, 08/19/15, GNR - Last mod before committing to svn repository   *
***********************************************************************/

#define NDAT  10           /* Number of points */
#define NDIM   3           /* Number of dimensions */
#define MAIN

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "sysdef.h"
#include "mdks.h"

int main() {

   void *pwk = NULL;
   double smd,scg,sbf;
   float X1[NDAT][NDIM] = { { 0.3051, 0.7307, 0.7616},  /* 1 */
                            { 0.6451, 0.8394, 0.3666},  /* 2 */
                            { 0.5206, 0.3814, 0.8001},  /* 3 */
                            { 0.6192, 0.6048, 0.5468},  /* 4 */
                            { 0.5356, 0.7301, 0.6313},  /* 5 */
                            { 0.3763, 0.1217, 0.6095},  /* 6 */
                            { 0.3669, 0.8108, 0.0553},  /* 7 */
                            { 0.8977, 0.3467, 0.9350},  /* 8 */
                            { 0.0159, 0.0288, 0.4723},  /* 9 */
                            { 0.8496, 0.9396, 0.1414}   /* 10 */ };
   float X2[NDAT][NDIM] = { { 0.7627, 0.9658, 0.2696},  /* 1 */
                            { 0.4927, 0.5549, 0.4280},  /* 2 */
                            { 0.6522, 0.9731, 0.4828},  /* 3 */
                            { 0.9037, 0.0250, 0.2599},  /* 4 */
                            { 0.5743, 0.7294, 0.8246},  /* 5 */
                            { 0.1517, 0.0495, 0.7282},  /* 6 */
                            { 0.5349, 0.0399, 0.3936},  /* 7 */
                            { 0.5000, 0.2103, 0.1150},  /* 8 */
                            { 0.4760, 0.6540, 0.3494},  /* 9 */
                            { 0.0490, 0.8583, 0.6364}   /* 10 */ };
   int g1 = 2;
   int g2 = KC_MDKS|KC_EPKS;
   int i;

   /* Get the brute-force answer */
   i = bfksallof(&pwk, NDAT, NDAT, NDIM);
   if (i > 0) exit(i+70);
   sbf = bfks1sf(pwk, (float *)X1, (float *)X2);
   if (sbf < 0.0) exit((int)(-sbf)+75);
   i = bfksfree(pwk);
   if (i > 0) exit(i+77);

   /* Run the statistic */
   pwk = NULL;
/*** DEBUG ***/ g2 |= KC_QS1;
   i = mdksallof(&pwk, &g1, &g2, NDAT, NDAT, NDIM);
   if (i > 0) exit(i+80);
   smd = mdks1sf(pwk, (float *)X1, (float *)X2);
   if (smd < 0.0) exit((int)(-smd)+85);
   scg = epks1sf(pwk, (float *)X1, (float *)X2);
   if (scg < 0.0) exit((int)(-scg)+85);
   i = mdksfree(pwk);
   if (i > 0) exit(i+87);

   /* Print the result */
   printf("Multidimensional Kolmogorov-Smirnov Statistic 3D Test\n");
   printf("3D MDKS test returned %8.5f\n",smd);
   printf("3D EPKS test returned %8.5f\n",scg);
   printf("The brute-force result is %8.5f\n",sbf);
   return 0;
   }
