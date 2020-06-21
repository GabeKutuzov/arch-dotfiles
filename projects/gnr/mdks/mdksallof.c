/* (c) Copyright 2013, The Rockefeller University */
/* $Id: mdksallof.c 1 2014-04-21 19:53:00Z  $ */
/***********************************************************************
*           Multi-Dimensional Kolmogorov-Smirnov Statistic             *
*                             mdksallof.c                              *
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
*                              mdksallof                               *
*                                                                      *
*  This function allocates and initializes an mdks work area that is   *
*  used by the other routines in the mdks package.  This function is   *
*  performed by a separate routine so that the same work area can be   *
*  reused for multiple MDKS calculations.                              *
*                                                                      *
*  This is the version for single-precision floating point data and    *
*  integer number of data points.                                      *
*                                                                      *
*  Usage:                                                              *
*     This file defines data type codes, then #includes the file       *
*  mdksallo.c, which contains the actual code to do the allocation.    *
*  Usage instructions are in the README file and in mdksallo.c.        *
*                                                                      *
*  Reference:                                                          *
*     G. Fasano & A. Franceschini (1987).  A multidimensional version  *
*  of the Kolmogorov-Smirnov test.  Monthly Notices of the Royal       *
*  Astronomical Society, 225:155-170.                                  *
************************************************************************
*  V1A, 11/18/13, G.N. Reeke - New program                             *
*  ==>, 11/29/13, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define KTX 0              /* Generate for single-precision data */
#define KTN 0              /* Generate for integer array sizes */
#define VL  f              /* Generate mdksallof() */

#include "mdksallo.c"

