/* (c) Copyright 2013, The Rockefeller University */
/* $Id: mdks2sd.c 1 2014-04-21 19:53:00Z  $ */
/***********************************************************************
*           Multi-Dimensional Kolmogorov-Smirnov Statistic             *
*                              mdks2sd.c                               *
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
*                               mdks2sd                                *
*                                                                      *
*  This function computes the two-sample (symmetric) multi-dimensional *
*  Kolmogorov-Smirnov statistic, which may be used to quantitate the   *
*  degree to which two distributions of points are statistically       *
*  distinguishable.                                                    *
*                                                                      *
*  This is the version for double-precision floating point data and    *
*  integer numbers of data points.                                     *
*                                                                      *
*  Usage:                                                              *
*     This file computes the MDKS statistic twice, using each data set *
*  as the model, and combines the results according to the formula     *
*  given in the reference cited below.  The user must first call       *
*  mdksallod() to create a work area that is passed to this program    *
*  along with pointers to the two data sets to be compared.  This can  *
*  be repeated for multiple data sets of the same dimensionality.      *
*  When the work area is no longer needed, mdksfree() can be called    *
*  to free the space for other use.                                    *
*                                                                      *
*  Usage:                                                              *
*     This file defines data type codes, then #includes the file       *
*  mdks2s.c, which contains the actual code to compute the statistic.  *
*  Usage instructions are in the README file and in mdks2s.c           *
*                                                                      *
*  Reference:                                                          *
*     G. Fasano & A. Franceschini (1987).  A multidimensional version  *
*  of the Kolmogorov-Smirnov test.  Monthly Notices of the Royal       *
*  Astronomical Society, 225:155-170.                                  *
************************************************************************
*  V1A, 11/16/13, G.N. Reeke - New program                             *
*  ==>, 11/29/13, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define KTX 1              /* Generate for double-precision data */
#define KTN 0              /* Generate for integer array sizes */
#define VL  d              /* Generate mdks2sd() */

#include "mdks2s.c"

