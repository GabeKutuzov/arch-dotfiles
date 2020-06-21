/* (c) Copyright 2014, The Rockefeller University */
/* $Id: mdksdivs.c 6 2014-06-09 19:19:58Z  $ */
/***********************************************************************
*           Multi-Dimensional Kolmogorov Smirnov Statistic             *
*                             mdksdivs.c                               *
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
*                              mdksdivs                                *
*                                                                      *
*  This routine may be called by an application that is using the MDKS *
*  library to find out the depth of the brick hierarchy established by *
*  an earlier call to mdksallox().  It may be called at any time after *
*  the call to mdksallox() and before the work area is freed by a call *
*  to mdksfree(), (where, as described in the readme, 'x' is 'd' for   *
*  double precision and 'f' for float data).                           *
*                                                                      *
*  For compatibility with earlier versions of the mdks library, or to  *
*  accommodate possible future changes, data are returned as arrays of *
*  divisions for both X1 and X2 data sets, even though in the present  *
*  version these arrays have only one member (i.e. they are scalars)   *
*  and they are always the same for X1 and X2.                         *
*                                                                      *
*  MATLAB Synopsis:                                                    *
*     [DivsX1, DivsX2] = mdksdivs(MDKSCOM)                             *
*  The MATLAB mex version returns the hierarch depth directly as two   *
*  double-precision floating-point scalars, 'DivsX1' for data set X1   *
*  and 'DivsX2' for data set X2.  The argument 'MDKSCOM' is the work   *
*  area returned by a previous call to mdksallox().                    *
*                                                                      *
*  C or C++ Synopsis:                                                  *
*     #include "mdks.h"                                                *
*     int mdksdivs(void *pwk, int *pdivs[2]);                          *
*  The C version returns an array of two pointers to int stored in the *
*  user-supplied space pointed to by the argument 'pdivs'. Each points *
*  to a single integer which is the depth of hypercube brick bisection *
*  assigned for this problem.  The first pointer gives the depth for   *
*  data set X1 and the second for data set X2. In the present version, *
*  these are always identical, but may not be in future versions.  The *
*  values pointed to remain valid until the next call to mdksallox()   *
*  or mdksfree().  The argument 'pwk' must be the pointer stored at    *
*  *ppwk by the most recent call to mdksallox().  The value of the     *
*  function return is 0 for success and 1 (defined in the header file  *
*  mdks.h as MDKSA_ERR_BADWK) when the argument 'pwk' does not point   *
*  to a valid mdks work space block.                                   *
*                                                                      *
************************************************************************
*  V1A, 04/28/14, GNR - New program                                    *
*  ==>, 04/28/14, GNR - Last mod before committing to svn repository   *
***********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "mdks.h"
#include "mdksint.h"

#ifndef SVNVRS
#define SVNVRS Unknown
#endif
#define vcat(txt,vers) txt #vers

#ifdef I_AM_MATLAB
#include "mex.h"

void InnerDivs(int nlhs, mxArray *plhs[],
   int nrhs, const mxArray *prhs[]) {

   Wkhd *phd;
   double dd;
   int i,j;

/* Standard MATLAB check for proper numbers of arguments */

   if (nlhs != NDSET)
      mexErrMsgTxt("mdksdivs requires 2 left-hand args.");
   if (nrhs != 1)
      mexErrMsgTxt("mdksdivs requires 1 right-hand arg.");

/* Check that the right-hand argument is a pointer to Wkhd */

   if (!mxIsUint8(prhs[0]) ||
         mxGetNumberOfElements(prhs[0]) != sizeof(void *))
      mexErrMsgTxt("mdksdivs MDKSCOM arg is not expected "
         "type or size.");
   phd = *(Wkhd **)mxGetData(prhs[0]);
   if (phd->hdrlbl != HLBL)
      mexErrMsgTxt("mdksdivs MDKSCOM arg appears not to be "
         "a work area returned by mdksallox().");

/* Create and populate the result matrices */

   /* if k is 1, phd->pdiv ptrs are NULL, just return
   *  matrixes with scalar ones to avoid seg faults */
   dd = (phd->k <= 1) ? 1.0 : (double)phd->d;
   for (i=0; i<NDSET; ++i) {
      plhs[i] = mxCreateDoubleScalar(dd);
      } /* End for */
         
   } /* End mexFunction() */

/* Wrapper for the wrapper (so gdb can be used) */

void mexFunction(int nlhs, mxArray *plhs[],
   int nrhs, const mxArray *prhs[]) {

   InnerDivs(nlhs, plhs, nrhs, prhs);

   }

#else /* not I_AM_MATLAB */

int mdksdivs(void *pwk, int *pdivs[2]) {

   Wkhd *phd = (Wkhd *)pwk;
   int *pd;
   int i;

   if (phd->hdrlbl != HLBL) return MDKSA_ERR_BADWK;

   /* if k is 1, phd->pdiv ptrs are NULL, so instead
   *  return ptrs to k so caller sees 1 as divs.  */
   pd = (phd->k <= 1) ? &phd->k : &phd->d;
   for (i=0; i<NDSET; ++i) pdivs[i] = pd;

   return 0;

   } /* End mdksdivs() */

#endif
