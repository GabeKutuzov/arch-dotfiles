/* (c) Copyright 2013-2015, The Rockefeller University */
/* $Id: mdks2s.c 9 2016-01-04 19:30:24Z  $ */
/***********************************************************************
*           Multi-Dimensional Kolmogorov-Smirnov Statistic             *
*                              mdks2s.c                                *
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
*                               mdks2s                                 *
*                                                                      *
*  This function computes the two-sample (symmetric) multi-dimensional *
*  Kolmogorov-Smirnov statistic, which may be used to quantitate the   *
*  degree to which two distributions of points are statistically       *
*  distinguishable.                                                    *
*                                                                      *
*  This is generic code that can be compiled with different external   *
*  definitions to generate routines to handle different data types.    *
*  This provides a mechanism in C similar to C++ templates.  Each      *
*  routine is named with a terminal letter to indicate the types it    *
*  handles.  This letter is represented by an 'x' in the comments.     *
*  The routines with the terminal letters are basically wrappers that  *
*  define the type selectors 'KTX' and 'KTN' and the version letter    *
*  'VL' and then include the actual code from this file.  These        *
*  wrappers can also be compiled with 'I_AM_MATLAB' defined to create  *
*  a MATLAB mex file with the same name that performs the same action. *
*                                                                      *
*  Usage:                                                              *
*     This file computes the MDKS statistic twice, using each data set *
*  as the model, and combines the results according to the formula     *
*  given in the reference cited below.  The user must first call       *
*  mdksallox() to create a work area that is passed to this program    *
*  along with pointers to the two data sets to be compared.  mdks2sx() *
*  can be called repeatedly for data sets with the same mdksallox()    *
*  parameters, but mdksallox() must be called each time any of those   *
*  parameters change.  When the work area is no longer needed,         *
*  mdksfree() (C programs) or a special call to mdksallx() (MATLAB     *
*  programs) can be used to free the space for other use.              *
*                                                                      *
*  MATLAB Synopsis:                                                    *
*     s = mdks2sx(MDSKCOM, X1, X2)                                     *
*  C or C++ Synopsis:                                                  *
*     double mdks2sx(void *pwk, Xdat const *X1, Xdat const *X2)        *
*                                                                      *
*  Arguments:                                                          *
*     pwk         Pointer to work area returned by mdksallox().        *
*     X1,X2       Pointers to the two data sets.                       *
*     MDKSCOM     MATLAB equivalent of pwk.                            *
*                                                                      *
*  Value returned:                                                     *
*     If Z1 and Z2 are respectively the one-sided statistic with X1    *
*  and X2 taken as the experimental data set, and n1 and n2 are the    *
*  numbers of points in X1 and X2, then the value returned is the      *
*  two-sided statistic (1/2)*(Z1*sqrt(n2) + Z2*sqrt(n1))/sqrt(n1+n2).  *
*  In case of errors, one of the codes (negative values) documented    *
*  in the file mdks.h is returned instead.  The MATLAB mex file gives  *
*  a terminal error if the operation was not successful.               *
*                                                                      *
*  Reference:                                                          *
*     G. Fasano & A. Franceschini (1987).  A multidimensional version  *
*  of the Kolmogorov-Smirnov test.  Monthly Notices of the Royal       *
*  Astronomical Society, 225:155-170.                                  *
************************************************************************
*  V1A, 11/16/13, G.N. Reeke - New program                             *
*  V1B, 11/20/13, GNR - Add mex file wrapper                           *
*  V2A, 12/14/13, GNR - Modify for brick method                        *
*  V3A, 02/08/14, GNR - More uniform distribution of data into bricks  *
*  Rev, 04/15/14, GNR - Give the formula for the result in a comment   *
*  ==>, 04/15/14, GNR - Last date before committing to svn repository  *
*  Rev, 05/01/14, GNR - Add 'const' qualifiers to pointers to X1,X2    *
*  V6A, 05/15/14, GNR - Hierarchical bricks                            *
***********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#ifdef I_AM_MATLAB
#include "mex.h"
#endif
#include "mdks.h"
#include "mdksint.h"

/*=====================================================================*
*  N.B.  This program is intended to be included in a wrapper program  *
*  that defines the type specifiers KTX and KTN and the version iden-  *
*  tifier VL.  Alternatively, these variables can be defined in a      *
*  makefile.  If compiled without external definitions, mdks2s will    *
*  default to double-precision data with integer ni and name 'mdks2s'. *
*  (This indirect method is used because #if cannot test strings.)     *
*=====================================================================*/

#ifdef I_AM_MATLAB
#define MyName vfnmex("mdks2s",VL)

enum rhss2args { jwk, jX1, jX2 };

/*=====================================================================*
*                    mdks2sx mex function wrapper                      *
*=====================================================================*/

void mexFunction(int nlhs, mxArray *plhs[],
   int nrhs, const mxArray *prhs[]) {

   void *pwk;
   Wkhd *phd;
   Xdat const *pX1;
   Xdat const *pX2;
   const mwSize *pDimX;
   double s;               /* Return value */

/* Standard MATLAB check for proper numbers of arguments */

   if (nlhs != 1)
      mexErrMsgTxt(MyName " requires 1 left-hand arg.");
   if (nrhs != 3)
      mexErrMsgTxt(MyName " requires 3 right-hand args.");

/* Extract pointers to data from right-hand args */

   if (!mxIsUint8(prhs[jwk]) ||
         mxGetNumberOfElements(prhs[jwk]) != sizeof(void *))
      mexErrMsgTxt(MyName " MDKSCOM arg is not expected type,size.");
   pwk = *(void **)mxGetData(prhs[jwk]);
   phd = (Wkhd *)pwk;
   if (phd->hdrlbl != HLBL)
      mexErrMsgTxt(MyName " MDKSCOM arg appears not to be a work "
         "area returned by mdksallo routine.");

#if KTX == 0
   if (!mxIsSingle(prhs[jX1]) ||
         mxGetNumberOfDimensions(prhs[jX1]) != NDIMS)
      mexErrMsgTxt(MyName " X1 arg must be 2D float array.");
   pDimX = mxGetDimensions(prhs[jX1]);
   if (pDimX[0] != phd->k || pDimX[1] != phd->n[IX1])
      mexErrMsgTxt(MyName " X1 arg must be k x n1 array.");
   pX1 = (Xdat const *)mxGetData(prhs[jX1]);

   if (!mxIsSingle(prhs[jX2]) ||
         mxGetNumberOfDimensions(prhs[jX2]) != NDIMS)
      mexErrMsgTxt(MyName " X2 arg must be 2D float array.");
   pDimX = mxGetDimensions(prhs[jX2]);
   if (pDimX[0] != phd->k || pDimX[1] != phd->n[IX2])
      mexErrMsgTxt(MyName " X2 arg must be k x n2 array.");
   pX2 = (Xdat const *)mxGetData(prhs[jX2]);
#else
   if (!mxIsDouble(prhs[jX1]) ||
         mxGetNumberOfDimensions(prhs[jX1]) != NDIMS)
      mexErrMsgTxt(MyName " X1 arg must be 2D double array.");
   pDimX = mxGetDimensions(prhs[jX1]);
   if (pDimX[0] != phd->k || pDimX[1] != phd->n[IX1])
      mexErrMsgTxt(MyName " X1 arg must be k x n1 array.");
   pX1 = (Xdat const *)mxGetData(prhs[jX1]);

   if (!mxIsDouble(prhs[jX2]) ||
         mxGetNumberOfDimensions(prhs[jX2]) != NDIMS)
      mexErrMsgTxt(MyName " X2 arg must be 2D double array.");
   pDimX = mxGetDimensions(prhs[jX2]);
   if (pDimX[0] != phd->k || pDimX[1] != phd->n[IX2])
      mexErrMsgTxt(MyName " X2 arg must be k x n2 array.");
   pX2 = (Xdat const *)mxGetData(prhs[jX2]);
#endif

/* Perform the statistical computation and return the result */

   s = vfnname(mdks2s,VL)(pwk, pX1, pX2);
   plhs[0] = mxCreateDoubleScalar(s);

   } /* End mexFunction */

/* Include the needed subroutines here so no linking needed */

#undef I_AM_MATLAB
#include "mdks1s.c"
#else
double vfnname(mdksqd,VL)(Wkhd *phd, int ix1);
#endif /* I_AM_MATLAB */

/*=====================================================================*
*                               mdks2sd                                *
*=====================================================================*/

double vfnname(mdks2s,VL)(void *pwk, Xdat const *X1, Xdat const *X2) {

   Wkhd *phd = (Wkhd *)pwk;
   double s1,s2;              /* KS stats in the two orders */
   double dn1,dn2;            /* Sizes of the two data sets */
   int    k;                  /* Number of dimensions */

#ifndef I_AM_MATLAB           /* Test already done if MATLAB */
   if (phd->hdrlbl != HLBL) return MDKS_ERR_BADWK;
#endif
   k = phd->k;

   if ((s1 = vfnname(mdks1s,VL)(pwk, X1, X2)) <= 0.0) return s1;
   if ((s2 = vfnname(mdksqd,VL)(pwk, IX2)) <= 0.0) return s1;

   /* Combine the results */
   dn1 = (double)phd->n[IX1], dn2 = (double)phd->n[IX2];
   return 0.5*(sqrt(dn2)*s1 + sqrt(dn1)*s2)/sqrt(dn1+dn2);

   } /* End mdks2sx() */
