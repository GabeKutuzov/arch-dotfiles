/* (c) Copyright 2013, The Rockefeller University */
/* $Id: mdksvers.c 9 2016-01-04 19:30:24Z  $ */
/***********************************************************************
*           Multi-Dimensional Kolmogorov Smirnov Statistic             *
*                             mdksvers.c                               *
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
*                              mdksvers                                *
*                                                                      *
*  This routine may be called at any time by an application that is    *
*  using the MDKS library.  It returns a pointer to a static string    *
*  containing the current subversion revision number of the package    *
*  in the form "MDKS Library Version nnn", useful for documentation.   *
*                                                                      *
*  MATLAB Synopsis:                                                    *
*     Vers = mdksvers()                                                *
*  C or C++ Synopsis:                                                  *
*     char *mdksvers(void)                                             *
************************************************************************
*  V1A, 11/24/13, GNR - New program--uses version passed from makefile *
*  ==>, 11/29/13, GNR - Last mod before committing to svn repository   *
*  R09, 12/01/15, GNR - Correct incorrect vcat macro                   *
***********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "mdks.h"

#ifndef SVNVRS
#define SVNVRS Unknown
#endif
#define vcatin(txt,vers) txt #vers
#define vcat(txt,vers) vcatin(txt,vers)

#ifdef I_AM_MATLAB
#include "mex.h"

void mexFunction(int nlhs, mxArray *plhs[],
   int nrhs, const mxArray *prhs[]) {

/* Standard MATLAB check for proper numbers of arguments */

   if (nlhs != 1)
      mexErrMsgTxt("mdksvers requires 1 left-hand arg.");
   if (nrhs != 0)
      mexErrMsgTxt("mdksvers requires no right-hand args.");

   plhs[0] = mxCreateString(vcat("MDKS Library Version ",SVNVRS));

   } /* End mexFunction() */

#else /* not I_AM_MATLAB */

char *mdksvers(void) {

   static char mdkslv[] = vcat("MDKS Library Version ",SVNVRS);

   return mdkslv;

   } /* End mdksvers() */

#endif
