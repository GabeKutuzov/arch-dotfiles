/* (c) Copyright 2013-2015, The Rockefeller University */
/* $Id: bfks1s.c 9 2016-01-04 19:30:24Z  $ */

/***********************************************************************
*           Multi-Dimensional Kolmogorov-Smirnov Statistic             *
*                              bfks1s.c                                *
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
*                               bfks1s                                 *
*                                                                      *
*  This function computes the one-sided multi-dimensional Kolmogorov-  *
*  Smirnov statistic, which may be used to quantitate the degree to    *
*  which two distributions of points in k-dimensional space are        *
*  statistically distinguishable.  As a convenience, the ordinary      *
*  one-dimensional Kolmogorov-Smirnov statistic can also be computed.  *
*                                                                      *
*  This is the "brute-force" version, whose purpose is to provide      *
*  timing information for comparison tests with various improved       *
*  algorithms.  It is a slight improvement over the original MATLAB    *
*  code in that each coordinate comparison is used to assign a point   *
*  to the appropriate generalized quadrant, rather than calculating    *
*  separately for every quadrant.                                      *
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
*     Samples of equal dimension (but not necessarily equal size) and  *
*  type ('double' or 'float') from two data distributions are compared *
*  by this program. The two data sets are referred to as 'X1' and 'X2' *
*  here and in the comments. If 'ni' is the number of points in data   *
*  set i (i={1,2}) and 'k' is the dimension of the problem, then the   *
*  data must be created by the caller in arrays of size ni x k (C      *
*  ordering, last index fast moving).  Note that if creating the data  *
*  in a MATLAB program, the order of the dimensions should be reversed *
*  to avoid the need for a transpose operation. No restriction is made *
*  that the values in each dimension need to cover the same range.     *
*     This program requires a work area that may be reused with multi- *
*  ple calls to any of the one- or two-sided routines in the package   *
*  that have the same ni, g1, and k parameters.  This area is allocat- *
*  ed (or modified for different parameters) by a call to mdksallox()  *
*  and freed by a call to mdksfree() (C programs) or a special call to *
*  mdksallox() (MATLAB programs).                                      *
*                                                                      *
*  MATLAB Synopses:                                                    *
*     s = bfks1sx(MDKSCOM, X1, X2)                                     *
*  C Synopses:                                                         *
*     double bfks1sx(void *pwk, Xdat const *X1, Xdat const *X2)        *
*                                                                      *
*  Arguments:                                                          *
*     X1,X2    Pointers to the sets of points to be compared.  X1 and  *
*              X2 must be arrays of whatever type the bfks1sx version  *
*              is compiled for.  (The type Xdat is determined by the   *
*              compile-time variable 'KTX' which is 0 for 'float' and  *
*              1 for 'double').  X1 has dimension n1 x k (C) or k x n1 *
*              (MATLAB) and X2 has dimension n2 x k (C) or k x n2      *
*              (MATLAB), where 'k', 'n1', and 'n2' are parameters in   *
*              a previous call to mdksallox().  The data points are    *
*              assumed to be stored in the native byte order of the    *
*              machine on which the program runs.                      *
*     pwk      This must be the pointer stored at *ppwk by a call to   *
*              mdksallox() with the 'ni', 'g1', and 'k' of X1 and X2   *
*              and with 'KC_BFKS' included in the 'g2' control bits.   *
*     MDKSCOM  MATLAB equivalent of pwk.                               *
*                                                                      *
*  Return values:                                                      *
*     The MDKS statistic is defined as the maximum over the n1 points  *
*  of data set X1 of the maximum over the 2^k generalized quadrants in *
*  a k-dimensional coordinate system with origin at each such point of *
*  the absolute value of the difference between the numbers of points  *
*  in that quadrant in X1 and in X2, normalized as if n2 == n1 and     *
*  divided by sqrt(n1).  Note that with the one-sided test, X1 should  *
*  be the experimental data and X2 the model.                          *
*     bfks1sx() returns the one-sided MDKS statistic as defined above. *
*  A negative value is returned in case of an error as follows:        *
*     (-1) The argument 'pwk' apparently did not point to an area      *
*          allocated by an earlier call to mdksallox() or the 'g2'     *
*          argument did not include the 'KC_BFKS' method code.         *
*     (-2) All the coordinates in some dimension were identical.       *
*  The MATLAB mex file gives a terminal error if the operation was not *
*  successful.                                                         *
*                                                                      *
*  Algorithm:                                                          *
*     This program simply loops over all the points in X1 and counts   *
*  how many points in X1 and how many points in X2 are in each of the  *
*  generalized quadrants with origin at that X1 point.  The X2 counts  *
*  are normalized by n2/n1 if the sizes of two data sets are different.*
*  The statistic returned is the largest difference over all points in *
*  X1 and all generalized coordinates.                                 *
*     This program retains the "two-quads" summation method so that no *
*  round-off error is introduced by adding multiple fractional values  *
*  to the quad sums when n1 != n2.  There should be little speed dif-  *
*  ference here relative to the "single-quads" method since no bricks. *
*                                                                      *
*  Notes:                                                              *
*     The authors cited below propose the statistic that is computed   *
*  by this routine and give suggestions for computing significance     *
*  levels (p-values) from the MDKS statistic (not done here).  They    *
*  also show how to perform a two-sample test by averaging the results *
*  from the tests with both orderings of the two data sets as X1 and   *
*  X2, multiplied by sqrt(n1*n2/(n1+n2)).  This is done by the mdks2sx *
*  routines.                                                           *
*     'n1' and 'n2' are usually 32 bit ints and 'k' must be less than  *
*  31 so the hypercube vertex numbers are positive signed ints.  The   *
*  code could be modified to handle larger numbers of points and/or    *
*  dimensions, but the execution time would probably be prohibitive.   *
*                                                                      *
*  Reference:                                                          *
*     G. Fasano & A. Franceschini (1987).  A multidimensional version  *
*  of the Kolmogorov-Smirnov test.  Monthly Notices of the Royal       *
*  Astronomical Society, 225:155-170.                                  *
************************************************************************
*  V1A, 12/05/13, G.N. Reeke - New program                             *
*  Rev, 04/11/14, GNR - Use established definition for case k == 1     *
*  ==>, 04/11/14, GNR - Last date before committing to svn repository  *
*  Rev, 05/01/14, GNR - Add 'const' qualifiers to pointers to X1,X2    *
*  Rev, 12/30/15, GNR - Eliminate bfksint.h, bfksallo.c, use mdks code.*
***********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#ifdef I_AM_MATLAB
#include "mex.h"
#endif
#include "mdks.h"
#include "mdksint.h"

#define BFDBG 0            /* Used to control debug outputs */

/*=====================================================================*
*  N.B.  This program is intended to be included in a wrapper program  *
*  that defines the type specifiers KTX and KTN and the version iden-  *
*  tifier VL.  Alternatively, these variables can be defined in a      *
*  makefile.  If compiled without external definitions, bfks1s will    *
*  default to double-precision data with integer ni and name 'bfks1s'. *
*  (This indirect method is used because #if cannot test strings.)     *
*=====================================================================*/

#ifdef I_AM_MATLAB
#define MyName vfnmex("bfks1s",VL)
#define printf mexPrintf

enum rhsargs { jwk, jX1, jX2 };

/*=====================================================================*
*                    bfks1sx mex function wrapper                      *
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
      mexErrMsgTxt(MyName " MDKSCOM arg is not type or size returned "
         "by mdksallo routine.");
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

   s = vfnname(bfks1s,VL)(pwk, pX1, pX2);
   plhs[0] = mxCreateDoubleScalar(s);

   } /* End mexFunction() */

#endif /* I_AM_MATLAB */

/*=====================================================================*
*                               bfks1sx                                *
*=====================================================================*/

double vfnname(bfks1s,VL)(void *pwk, Xdat const *X1, Xdat const *X2) {

   Wkhd *phd = (Wkhd *)pwk;
   Xdat const *pX1,*pX1e;  /* Ptrs to start, end of X1 data */
   Xdat const *pX2e;       /* Ptr to end of X2 data */
   Xdat const *pX;         /* Ptr to data being checked */
   double bfks;            /* The result */
   double nrat;            /* n1/n2 */
   int  k;                 /* Number of dimensions */

/* Preliminaries */

   /* Validate the work area and pick up constants & pointers */
#ifndef I_AM_MATLAB        /* Test already done if MATLAB */
   if (phd->hdrlbl != HLBL) return MDKS_ERR_BADWK;
#endif
   k = phd->k;
   pX1e = X1 + k*phd->n[IX1];
   pX2e = X2 + k*phd->n[IX2];
   bfks = 0.0;             /* Initial value of result */
   /* Not using phd->nrat because needs to be reversed for mdks2s */
   nrat = (double)phd->n[IX1]/(double)phd->n[IX2];

   if (k > 1) {
      Prec *pnq1,*pnq2;       /* Ptrs to quadrant counts */
      size_t nqt;             /* Total size of quad count arrays */
      int  nq;                /* Number of generalized quadrants */
      int  jk;                /* Index for loops over k */
      int  jq;                /* Index for loops over nq */
      pnq1 = phd->pQtot[IX1];
      pnq2 = phd->pQtot[IX2];
      nq = 1 << k;
      nqt = nq*(NDSET*sizeof(Prec));

#if defined(BFDBG) && BFDBG != 0
/*** DEBUG ***/
   if (k == 2 && phd->n[0] == 50) ++phd->repct;
/*** ENDDEBUG ***/
#endif

      /* Loop over all the points in X1 */
      for (pX1=X1; pX1<pX1e; pX1+=k) {

#if defined(BFDBG) && BFDBG != 0
/*** DEBUG ***/
   if (k == 2 && phd->n[0] == 50 && phd->repct == 46) {
      printf("==>bfks processing test point at coords:\n  ");
      for (jk=0; jk<k; ++jk) printf(" %.5f", pX1[jk]);
      printf(" \n");
      }
/*** ENDDEBUG ***/
#endif

         /* Zero the quadrant counts */
         memset((char *)pnq1, 0, nqt);

         /* Count points of X1 in each quadrant */
         for (pX=X1; pX<pX1e; pX+=k) {
            int iq = 0;       /* What quad am I? */
            for (jk=0,jq=1; jk<k; jq<<=1,++jk) {
               if (pX[jk] == pX1[jk]) goto SkipThisX1;
               if (pX[jk] >  pX1[jk]) iq += jq;
               }
            pnq1[iq] += 1.;
SkipThisX1: ;
            }

         /* Count points of X2 in each quadrant */
         for (pX=X2; pX<pX2e; pX+=k) {
            int iq = 0;       /* What quad am I? */
            for (jk=0,jq=1; jk<k; jq<<=1,++jk) {
               if (pX[jk] == pX1[jk]) goto SkipThisX2;
               if (pX[jk] >  pX1[jk]) iq += jq;
               }
            pnq2[iq] += 1.;
SkipThisX2: ;
            }

         /* Find largest normalized difference */
         for (jq=0; jq<nq; ++jq) {
            double bdiff = fabs((double)(pnq1[jq] - nrat*pnq2[jq]));
            if (bdiff > bfks) bfks = bdiff;

#if defined(BFDBG) && BFDBG != 0
/*** DEBUG ***/
   if (k == 2 && phd->n[0] == 50 && phd->repct == 46) {
      printf("  Adjusted quad %d difference is %.3f - %.3f = %.3f\n",
         jq, pnq1[jq], pnq2[jq], bdiff);
      }
/*** ENDDEBUG ***/
#endif
            }

         } /* End loop over X1 */

      } /* End k > 1 */

   else {                     /* k == 1 */
      double dnq1,dnq2;       /* Cumulative counts */
      double bdiff;

      /* Loop over all the points in X1 */
      for (pX1=X1; pX1<pX1e; ++pX1) {

         /* Zero the cumulative counts */
         dnq1 = dnq2 = 0.0;

         /* Count points of X1 below or at this point */
         for (pX=X1; pX<pX1e; ++pX)
            if (*pX <= *pX1) dnq1 += 1.;

         /* Count points of X2 below or at this point */
         for (pX=X2; pX<pX2e; ++pX)
            if (*pX <= *pX1) dnq2 += 1.;

         /* Find largest normalized difference */
         bdiff = fabs(dnq1 - nrat*dnq2);
         if (bdiff > bfks) bfks = bdiff;

         } /* End loop over X1 */

      }

   return bfks/phd->sqrtn[IX1];

   } /* End bfks1sx() */

