/* (c) Copyright 2013-2015, The Rockefeller University */
/* $Id: bfks2sd.c 9 2016-01-04 19:30:24Z  $ */
/***********************************************************************
*           Multi-Dimensional Kolmogorov-Smirnov Statistic             *
*                              bfks2sd.c                               *
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
*                               bfks2sd                                *
*                                                                      *
*  This function computes the two-sample (symmetric) multi-dimensional *
*  Kolmogorov-Smirnov statistic, which may be used to quantitate the   *
*  degree to which two distributions of points are statistically       *
*  distinguishable.                                                    *
*                                                                      *
*  This is the brute-force version for double-precision floating point *
*  data and integer numbers of data points.                            *
*                                                                      *
*  Usage:                                                              *
*     This file defines data type codes, then #includes the file       *
*  bfks2s.c, which contains the actual code to compute the statistic.  *
*  Usage instructions are in the README file and in bfks2s.c           *
*                                                                      *
*  Reference:                                                          *
*     G. Fasano & A. Franceschini (1987).  A multidimensional version  *
*  of the Kolmogorov-Smirnov test.  Monthly Notices of the Royal       *
*  Astronomical Society, 225:155-170.                                  *
************************************************************************
*  V1A, 12/05/13, G.N. Reeke - New program                             *
*  ==>, 12/14/13, GNR - Last date before committing to svn repository  *
*  R09, 12/30/15, GNR - Remove redundant usage instructions            *
***********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define KTX 1              /* Generate for double-precision data */
#define KTN 0              /* Generate for integer array sizes */
#define VL  d              /* Generate bfks2sd() */

#include "bfks2s.c"

