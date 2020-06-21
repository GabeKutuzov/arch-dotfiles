/* (c) Copyright 2015-2016, The Rockefeller University */
/* $Id: rmks1sd.c 8 2015-11-25 20:04:01Z  $ */
/***********************************************************************
*           Multi-Dimensional Kolmogorov-Smirnov Statistic             *
*                              rmks1sd.c                               *
*                                                                      *
*  This software was written by Aparna Nair-Kanneganti and George N.   *
*  Reeke in the Laboratory of Biological Modelling at The Rockefel-    *
*  ler University.  Please send any corrections, suggestions, or       *
*  improvements to the author by email to reeke@mail.rockefeller.edu   *
*  for possible incorporation in future revisions.                     *
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
*                               rmks1sd                                *
*                                                                      *
*  This function computes the two-dimensional Kolmogorov-Smirnov       *
*  statistic using the "ribbon" approximation.  Mean coordinates of    *
*  ribbons of bricks that share one coordinate with the current test   *
*  point are computed and the points in the ribbon are distributed     *
*  into quadrants according to the fractional position of the mean     *
*  across the width of the ribbon.  In this version, points in bricks  *
*  that overlap the brick that contains the test point are distributed *
*  individually as in the exact brick method.                          *
*                                                                      *
*  This is the single-sample version for double-precision floating     *
*  point data and integer number of data points.                       *
*                                                                      *
*  Usage:                                                              *
*     This file defines data type codes, then #includes the file       *
*  rmks1s.c, which contains the actual code to compute the statistic.  *
*  Usage instructions are in the README file and in rmks1s.c           *
*                                                                      *
*  Reference:                                                          *
*     G. Fasano & A. Franceschini (1987).  A multidimensional version  *
*  of the Kolmogorov-Smirnov test.  Monthly Notices of the Royal       *
*  Astronomical Society, 225:155-170.                                  *
************************************************************************
*  V1A, 04/16/16, G.N. Reeke - New program                             *
*  ==>, 04/16/16, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define KTX 1              /* Generate for double-precision data */
#define KTN 0              /* Generate for integer array sizes */
#define VL  d              /* Generate rmks1sd() */

#include "rmks1s.c"

