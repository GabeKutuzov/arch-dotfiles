/* (c) Copyright 2015, The Rockefeller University */
/* $Id: cgks2s.c 8 2015-11-25 20:04:01Z  $ */
/***********************************************************************
*           Multi-Dimensional Kolmogorov-Smirnov Statistic             *
*                              cgks2s.c                                *
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
*                               cgks2s                                 *
*                                                                      *
*  This function computes the two-sample (symmetric) multi-dimensional *
*  Kolmogorov-Smirnov statistic, which may be used to quantitate the   *
*  degree to which two distributions of points are statistically       *
*  distinguishable.  This version computes the "center-of-gravity"     *
*  approximation (see "Algorithm" below).                              *
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
*  along with pointers to the two data sets to be compared.  cgks2sx() *
*  can be called repeatedly for data sets with the same mdksallox()    *
*  parameters, but mdksallox() must be called each time any of those   *
*  parameters change.  When the work area is no longer needed,         *
*  mdksfree() can be called to free the space for other use.           *
*                                                                      *
*  MATLAB Synopsis:                                                    *
*     s = cgks2sx(MDSKCOM, X1, X2)                                     *
*  C or C++ Synopsis:                                                  *
*     double cgks2sx(void *pwk, Xdat const *X1, Xdat const *X2)        *
*                                                                      *
*  Arguments:                                                          *
*     X1,X2    Pointers to the sets of points to be compared.  X1 and  *
*              X2 must be arrays of whatever type the cgks1sx version  *
*              is compiled for, where 'x' is 'f' for 'float' or 'd'    *
*              for double-precision.  X1 has dimension n1 x k (C) or   *
*              k x n1 (MATLAB) and X2 has dimension n2 x k (C) or      *
*              k x n2 (MATLAB), where 'k', 'n1', and 'n2' are para-    *
*              meters in a previous call to mdksallox().  The data     *
*              points are assumed to be stored in the native byte      *
*              order of the machine on which the program runs.         *
*     pwk      This must be the pointer stored at *ppwk by a call to   *
*              mdksallox() with the 'ni', 'g1', and 'k' of X1 and X2   *
*              and with 'KC_CGKS' included in the 'g2 'control bits.   *
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
*  Algorithm:                                                          *
*     This program uses an algorithm developed by GNR (possibly not    *
*  original).  The obvious algorithm runs in time proportional to      *
*  (n^2)*(2^k).  The exact algorithm in mdks[12]sx() improves on this  *
*  by dividing the data space into a hierarchy of rectilinear bricks   *
*  and obtaining quadrant counts from the larger, higher-level bricks  *
*  when there is no overlap of coordinates with the the current test   *
*  point.  This algorithm is described in detail in the mdks1s.c       *
*  source file.                                                        *
*     The mdks1sx() algorithm becomes increasingly less useful as the  *
*  dimension 'k' increases, because there are more ways for bricks to  *
*  overlap in one or more dimensions.  Finally, at the lowest level,   *
*  individual points must be examined to determine the appropriate     *
*  quadrant in which to count them.  In the approximation implemented  *
*  here, the points in an overlapping bottom-level brick are simply    *
*  allocated to that quadrant the center-of-gravity of whose data is   *
*  closest to the test point.  For equally distributed data points,    *
*  on average this will give small differences in the quadrant counts  *
*  with less computation than the equi-partitioned method and the      *
*  statistic will be approximately correct.                            *
*                                                                      *
*  Reference:                                                          *
*     G. Fasano & A. Franceschini (1987).  A multidimensional version  *
*  of the Kolmogorov-Smirnov test.  Monthly Notices of the Royal       *
*  Astronomical Society, 225:155-170.                                  *
************************************************************************
*  V7A, 08/16/15, G.N. Reeke - New program, modfied from mdks2s        *
*  ==>, 08/16/15, GNR - Last date before committing to svn repository  *
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
*  makefile.  If compiled without external definitions, cgks2s will    *
*  default to double-precision data with integer ni and name 'cgks2s'. *
*  (This indirect method is used because #if cannot test strings.)     *
*=====================================================================*/

#ifdef I_AM_MATLAB
#define MyName vfnmex("cgks2s",VL)

enum rhss2args { jwk, jX1, jX2 };

/*=====================================================================*
*                    cgks2sx mex function wrapper                      *
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

   s = vfnname(cgks2s,VL)(pwk, pX1, pX2);
   plhs[0] = mxCreateDoubleScalar(s);

   } /* End mexFunction */

/* Include the needed subroutines here so no linking needed */

#undef I_AM_MATLAB
#include "cgks1s.c"
#else
double vfnname(cgksqd,VL)(Wkhd *phd, int ix1);
#endif /* I_AM_MATLAB */

/*=====================================================================*
*                               cgks2sd                                *
*=====================================================================*/

double vfnname(cgks2s,VL)(void *pwk, Xdat const *X1, Xdat const *X2) {

   Wkhd *phd = (Wkhd *)pwk;
   double s1,s2;              /* KS stats in the two orders */
   double dn1,dn2;            /* Sizes of the two data sets */

#ifndef I_AM_MATLAB           /* Test already done if MATLAB */
   if (phd->hdrlbl != HLBL) return MDKS_ERR_BADWK;
#endif
   if (!(phd->kcalc & KC_CGKS)) return MDKS_ERR_BADWK;

   /* Set bit in kcalc so cgks1s will do cogs for both data sets */
   phd->kcalc |= KC_2S;
   if ((s1 = vfnname(cgks1s,VL)(pwk, X1, X2)) <= 0.0) return s1;
   if ((s2 = vfnname(cgksqd,VL)(pwk, IX2)) <= 0.0) return s1;
   phd->kcalc &= ~KC_2S;

   /* Combine the results */
   dn1 = (double)phd->n[IX1], dn2 = (double)phd->n[IX2];
   return 0.5*(sqrt(dn2)*s1 + sqrt(dn1)*s2)/sqrt(dn1+dn2);

   } /* End cgks2sx() */
