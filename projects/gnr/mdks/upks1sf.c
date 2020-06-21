/* (c) Copyright 2015, The Rockefeller University */
/* $Id: upks1sf.c 8 2015-11-25 20:04:01Z  $ */
/***********************************************************************
*           Multi-Dimensional Kolmogorov-Smirnov Statistic             *
*                               epks1sf                                *
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
*                               upks1sf                                *
*                                                                      *
*  This function computes the two-sample (symmetric) multi-dimensional *
*  statistic using the "unequal-partitioned" approximation that dis-   *
*  tributes data points in bricks that overlap the current test point  *
*  among quadrants in proportion to the position of the test point in  *
*  its brick.                                                          *
*                                                                      *
*  This is the single-sample version for single-precision floating     *
*  point data and integer number of data points.                       *
*                                                                      *
*  Usage:                                                              *
*     This file defines data type codes, then #includes the file       *
*  upks1s.c, which contains the actual code to compute the statistic.  *
*  Usage instructions are in the README file and upks1s.c              *
*                                                                      *
*  Reference:                                                          *
*     G. Fasano & A. Franceschini (1987).  A multidimensional version  *
*  of the Kolmogorov-Smirnov test.  Monthly Notices of the Royal       *
*  Astronomical Society, 225:155-170.                                  *
************************************************************************
*  V1A, 08/23/15, G.N. Reeke - New program                             *
*  ==>, 08/23/15, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define KTX 0              /* Generate for single-precision data */
#define KTN 0              /* Generate for integer array sizes */
#define VL  f              /* Generate upks1sf() */

#include "upks1s.c"

