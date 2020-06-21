/* (c) Copyright 2013-2015, The Rockefeller University */
/* $Id: mdks1s.c 9 2016-01-04 19:30:24Z  $ */
/***********************************************************************
*           Multi-Dimensional Kolmogorov-Smirnov Statistic             *
*                              mdks1s.c                                *
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
*                               mdks1s                                 *
*                                                                      *
*  This function computes the one-sided multi-dimensional Kolmogorov-  *
*  Smirnov statistic, which may be used to quantitate the degree to    *
*  which two distributions of points in k-dimensional space are        *
*  statistically distinguishable.  As a convenience, the ordinary      *
*  one-dimensional Kolmogorov-Smirnov statistic can also be computed.  *
*                                                                      *
*  This is generic code that can be compiled with different external   *
*  definitions to generate routines to handle different data types.    *
*  This provides a mechanism in C similar to C++ templates.  Each      *
*  routine is named with a terminal letter to indicate the types it    *
*  handles.  This letter is 'f' for floating-point variables or 'd'    *
*  for double-precision variables and is represented by an 'x' in the  *
*  comments.  The routines with the terminal letters are basically     *
*  wrappers that define the type selectors 'KTX' and 'KTN' and the     *
*  version letter 'VL' and then include the actual code from this      *
*  file.  These wrappers can also be compiled with 'I_AM_MATLAB'       *
*  defined to create a MATLAB mex file with the same name that         *
*  performs the same action.                                           *
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
*     s = mdks1sx(MDKSCOM, X1, X2)                                     *
*  C Synopses:                                                         *
*     double mdks1sx(void *pwk, Xdat const *X1, Xdat const *X2)        *
*                                                                      *
*  Arguments:                                                          *
*     X1,X2    Pointers to the sets of points to be compared.  X1 and  *
*              X2 must be arrays of whatever type the mdks1sx version  *
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
*              and with 'KC_MDKS' included in the 'g2' control bits    *
*              (which it is by default if 'g2' is 0 or not specified). *
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
*     mdks1sx() returns the one-sided MDKS statistic as defined above. *
*  A negative value is returned in case of an error as follows:        *
*     (-1) The argument 'pwk' apparently did not point to an area      *
*          allocated by an earlier call to mdksallox() or the 'g2'     *
*          argument did not include the 'KC_MDKS' method code.         *
*     (-2) All the coordinates in some dimension were identical.       *
*     (-3) Two-quad sums method request, but space was not allocated.  *
*  The MATLAB mex file gives a terminal error if the operation was not *
*  successful.                                                         *
*                                                                      *
*  Algorithm:                                                          *
*     This program uses an algorithm developed by GNR (possibly not    *
*  original).  The obvious algorithm runs in time proportional to      *
*  (n^2)*(2^k).  I was unable to find an algorithm that is provably    *
*  faster than this, but, depending on the distribution of the data,   *
*  the algorithm used here can run significantly faster than the       *
*  brute-force algorithm (timing data in the 'readme' file).  The new  *
*  algorithm divides the data space into a hierarchy of rectilinear    *
*  bricks.  At each successive hierarchy level, each coordinate axis   *
*  is divided so as to place roughly 1/(2^k) of the data points in     *
*  each child brick.  The user may specify the depth of the hierarchy  *
*  (usually only for test purposes) or accept the default, which is    *
*  computed according to a polynomial given in the mdksallo.c source   *
*  file.                                                               *
*     Two different methods of brick allocation are provided.  In the  *
*  method specified by the KC_QS1 option (g2 argument to mdksallox()), *
*  one set of bricks contains the differences between the number of    *
*  points of X1 and the number of points of X2 in that brick.  Corres- *
*  pondingly, there is one set of quad sums that sums the differences  *
*  from the bricks that end up in each quadrant for each X1 test point.*
*  In the method specified by the KC_QS2 option, there is a separate   *
*  hierarchy of bricks for each data set, and separate quad sums are   *
*  developed for each.  The differences are taken after summing over   *
*  bricks.  In all cases, the counts for the second data set are mul-  *
*  tiplied by n1/n2 if the two counts are different.  Tests indicated  *
*  that the KC_QS1 method was faster for k <= 3 and the KC_QS2 method  *
*  for k >= 4, and these are the methods chosen by default.  The g2    *
*  parameter allows these defaults to be overridden, usually for test  *
*  purposes.                                                           *
*     For use in the algorithm described next at the lowest level,     *
*  each brick is provided with a list of the enclosed points.          *
*     The MDKS is initialized to 0.  For each data point in set X1,    *
*  the populations of the 2^k quadrants around that point are set to   *
*  0.  The program then starts at the top of the hierarchy and recur-  *
*  sively scans the bricks at the current level.  If the brick does    *
*  not overlap the brick that contains the current target point, the   *
*  counts (KC_QS2) or difference count (KC_QS1) for that brick are/is  *
*  added into the total for the appropriate quadrant.  If there are    *
*  overlaps, the scan descends to the next lower level until either    *
*  nonoverlapping bricks, empty bricks, or the lowest level is reached.*
*  At the lowest level, individual points must be examined to deter-   *
*  mine the appropriate quadrant in which to count them.  The maximum  *
*  absolute values of the quadrant difference counts can then be used  *
*  to update the global MDKS.                                          *
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
*  MXK so the hypercube vertex numbers are positive signed ints.  The  *
*  code could be modified to handle larger numbers of points and/or    *
*  dimensions, but the execution time would probably be prohibitive    *
*  even with the fast algorithm used here.                             *
*     When k == 1, a faster algorithm based on sorting the coordinates *
*  is used.                                                            *
*     The code to scan the bricks is a subroutine so it can be used    *
*  for both argument orders by the mdks2sx() code.                     *
*                                                                      *
*  Reference:                                                          *
*     G. Fasano & A. Franceschini (1987).  A multidimensional version  *
*  of the Kolmogorov-Smirnov test.  Monthly Notices of the Royal       *
*  Astronomical Society, 225:155-170.                                  *
************************************************************************
*  V1A, 11/10/13, G.N. Reeke - New program                             *
*  V1B, 11/19/13, GNR - Add mex file wrapper                           *
*  V1C, 11/23/13, GNR - Add code to handle case k == 1                 *
*  V2A, 12/06/13, GNR - Modify for brick method (nonhierarchical)      *
*  V3A, 02/08/14, GNR - More uniform distribution of data into bricks  *
*  V4A, 02/18/14, GNR - Separate brick distibution for each band       *
*  Rev, 04/11/14, GNR - Correct bug in k==1 code                       *
*  ==>, 04/11/14, GNR - Last date before committing to svn repository  *
*  V5A, 04/19/14, GNR - Modify for V5 (partial quads) algorithm        *
*  V5B, 04/24/14, GNR - Store list of overlapping dims in each brick   *
*  V5C, 04/25/14, GNR - Store partial sums & overlapping bricks lists  *
*  Rev, 05/01/14, GNR - Add 'const' qualifiers to pointers to X1,X2    *
*  V5D, 05/02/14, GNR - Cleaner (high-->low) brick assignment method   *
*  Rev, 05/06/14, GNR - Add MD1SDBG outputs                            *
*  V6A, 05/13/14, GNR - Hierarchical bricks                            *
*  V6B, 05/22/14, GNR - Bricks keep index of next occupied brick       *
*  V6C, 05/27/14, GNR - Store ocube in Lvl, eliminate oprnt            *
*  Rev, 08/21/15, GNR - Store quad differences in one set of bricks    *
*  R09, 11/25/15, GNR - Add controls to force 1- or 2-quad sums        *
*  Rev, 12/02/15, GNR - Bug fix:  Force equal coords into same brick   *
*  Rev, 12/17/15, GNR - Bug fix:  Wrong byte count for brick zeroing   *
*  Rev, 12/19/15, GNR - Bug fix:  Negative numbers sorted high->low    *
***********************************************************************/

/* Add following constants in MD1SDBG for desired debug outputs: */
#define MDBG_EXPT    1  /* Print experimental point and data set */
#define MDBG_LVLC    2  /* Print level and cube entered */
#define MDBG_OVLP    4  /* Print results of overlap tests */
#define MDBG_BOTL    8  /* Print actions at bottom level */
#define MDBG_BRCT   16  /* Print list of bricks and point counts */
#define MDBG_BRPL   32  /* Print list of points in bottom bricks */
#define MDBG_INPT   64  /* Print list of input data */
#define MDBG_BIGQ  128  /* Print test point with biggest quad diffs */
#define MDBG_CDTL  256  /* Make outputs conditional -- see kdbgp */

#define MD1SDBG 0
#ifndef MD1SDBG
#define MD1SDBG 0
#endif

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

/*=====================================================================*
*  N.B.  This program is intended to be included in a wrapper program  *
*  that defines the type specifiers KTX and KTN and the version iden-  *
*  tifier VL.  Alternatively, these variables can be defined in a      *
*  makefile.  If compiled without external definitions, mdks1s will    *
*  default to double-precision data with integer ni and name 'mdks1s'. *
*  (Appropriate typedefs are in the header file mdksint.h)             *
*=====================================================================*/

#ifdef I_AM_MATLAB
#define MyName vfnmex("mdks1s",VL)
#define printf mexPrintf

enum rhsargs { jwk, jX1, jX2 };

/*=====================================================================*
*                    mdks1sx mex function wrapper                      *
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

   s = vfnname(mdks1s,VL)(pwk, pX1, pX2);
   plhs[0] = mxCreateDoubleScalar(s);

   } /* End mexFunction() */

#endif /* I_AM_MATLAB */


/*=====================================================================*
*                              mdkssrtx                                *
*                                                                      *
*  Sort a list of data points in increasing order along some dimension *
*  using a radix sort (like an old card sorter) with time proportional *
*  to n*sizeof(Xdat).                                                  *
*=====================================================================*/

static void vfnname(mdkssrt,VL)(Wkhd *phd, int jx, int jk) {

   Link *ptl,**ppl;        /* Ptrs to data links */
   Bin  *pb0,*pbe;         /* Ptrs to sorting bin info start,end */
   int  i,ii,isz;          /* Indexes for loops over sort digits */
   int  qle = phd->qle;    /* 1 Little-endian, 0 Big-endian */
   /* Values for flipping values so negatives sort correctly */
   unsigned int pflip = 0, nflip = (1 << NBPB) - 1;

   isz = sizeof(Xdat)-1;
   pb0 = phd->pBin0;
   pbe = pb0 + NBINS;

   /* Loop over bytes in the radix sort */
   for (ii=0; ii<=isz; ++ii) {
      Bin *pb;                /* Ptr to a sorting bin */
      i = qle ? ii : isz - ii;
      if (ii == isz) pflip = 1 << (NBPB-1);

      /* Initialize sorting bins for this key digit */
      for (pb=pb0; pb<pbe; ++pb) {
         pb->head = NULL;     /* Empty bin signal */
         pb->tail = (Link *)&pb->head;
         }

      /* Traverse linked list, extracting current key digit and
      *  assigning each record to top of appropriate bin.  */
      for (ptl=phd->pLnk1[jx]; ptl; ptl=ptl->pnxt) {
         Xdat const *pdk = ptl->pdat + jk;
         unsigned int ibin = (unsigned int)((byte const *)pdk)[i];
         ibin ^= (*pdk < 0) ? nflip : pflip;
         pb = pb0 + ibin;
         pb->tail->pnxt = ptl;
         pb->tail = ptl;
         } /* End loop over data records */

      /* End of pass, stack up the bins in order */
      ppl = &phd->pLnk1[jx];
      for (pb=pb0; pb<pbe; ++pb) if (pb->head) {
         *ppl = pb->head;
         ppl = &pb->tail->pnxt; }
      *ppl = NULL;            /* Terminate linked list */
      } /* End loop over bytes in Xk coords */

   return;
   } /* End mdkssrtx() */


#if MD1SDBG & (MDBG_EXPT|MDBG_LVLC|MDBG_OVLP|MDBG_BOTL|MDBG_BRCT|MDBG_BRPL)
/*=====================================================================*
*        Routine to convert offsets to coords for debug output         *
*=====================================================================*/
static void dbgqk(Wkhd *phd, size_t oo) {
   int dd, kk, d=phd->d, k=phd->k;
   int ks=k-1;
   printf(" (");
   for (kk=0; kk<k; oo<<=1,++kk) {
      size_t qq = 0, tq = oo >> ks, km = 1;
      for (dd=0; dd<d; km<<=1,++dd)
         qq += tq & km, tq >>= ks;
      if (kk == ks) printf("%zd)", qq);
      else          printf("%zd,", qq);
      }
   } /* End dbgqk() */
#endif


/*=====================================================================*
*                               mdksqdx                                *
*                                                                      *
*  Compute the MDKS.  This routine is called twice for the two-sided   *
*  test with X1 and X2 reversed so that repetition of the setup can be *
*  avoided.  The argument 'ix1' indicates which data set is to be      *
*  treated as the 'experimental' or 'master' data.                     *                                                         *
*=====================================================================*/

double vfnname(mdksqd,VL)(Wkhd *phd, int ix1) {

   Link *ptl0 = phd->pLnk0[ix1]; /* Ptr to test data */
   double mdks = 0.0;         /* The result */
   double nrat;               /* n[ix1]/n[ix2] */
#if MD1SDBG & MDBG_BIGQ
   size_t jbigq = 0;          /* Data for printing biggest diff */
   int ibigq = -1;
#endif
#if MD1SDBG & MDBG_CDTL
   int idcell = 8;            /* Cell selected for debug print */
   int kdbgp = 0;
#endif
   int k = phd->k;
   int kq;                    /* Method of summation */
   int ix2 = IX2 - ix1;       /* Index of second (model) data set */

   /* Not using phd->nrat because needs to be reversed for mdks2s */
   nrat = (double)phd->n[ix1]/(double)phd->n[ix2];
   if (k > 1) {

/*---------------------------------------------------------------------*
*  Calculation of MDKS for k > 1.  Separate code is provided for one-  *
*  and two-quad sum methods.  At top level, both methods loop over all *
*  points in the X1 set and for each point, zero the quad counts.      *
*  The Bin space is reused to keep track of the quadrant counts.       *
*---------------------------------------------------------------------*/

      Dim  *pdim = phd->pDim0;
      Link *ptl1,*ptl1e,*ptl2;      /* Ptrs to test data */
      Prec *pnq1,*pnq2;             /* Ptrs to quad sums */
      size_t lqds;                  /* Size of quad arrays */
      size_t nq = (size_t)1 << k;   /* Number of quadrants */
      size_t nsmask = nq - 1;       /* Not settled mask */
      int dm1 = phd->d - 1;
      int lshft, lshft0 = dm1*k;
      int jx;                       /* Data set index */

#if MD1SDBG & ~(MDBG_BIGQ|MDBG_CDTL)
      int kk;
#endif

      if (phd->kcalc & KC_2Q) {
         kq = 2;
         lqds = sizeof(Prec) << (k+1);
         pnq1 = phd->pQtot[ix1], pnq2 = phd->pQtot[ix2];
         }
      else {
         kq = 1;
         lqds = sizeof(Prec) << k;
         pnq1 = phd->pQtot[IXb];
         }

      /* Set up level 0 as if we've come down from a higher one */
      phd->pLvl0->ocube = phd->pLvl0->relqo = 0;
      phd->pLvl0->relqn = nsmask;
      /* Here is the main loop over test points (points in set ix1).
      *  Note that the Link pnxt fields are now in use as brick
      *  member list pointers, so treat Link list as an array.  */
      ptl1 = ptl0;
      ptl1e = ptl1 + phd->n[ix1];
      for ( ; ptl1<ptl1e; ++ptl1) {
         Brick *pbrkj;
         Lvl  *plvl,*plvle;
         Xdat const *pX1 = ptl1->pdat;
         size_t jq, jqq;
         size_t osubj,ochld;

#if MD1SDBG & MDBG_CDTL
kdbgp = (k == 2 && phd->n[0] == 50 && kq == 2 && phd->repct == 46 &&
   ptl1 - ptl0 == idcell);
if (kdbgp) {
#endif
#if MD1SDBG & MDBG_EXPT
   printf("==>Processing test point at brick");
   dbgqk(phd, ptl1->obrk);
   printf(", coords:\n  ");
   for (kk=0; kk<k; ++kk) printf(" %.5f", ptl1->pdat[kk]);
   printf(" \n");
#endif
#if MD1SDBG & MDBG_CDTL
   }
#endif

         /* Zero both sets of quadrant counts */
         memset(phd->pQtot[IXb], 0, lqds);

/*=====================================================================*
*  One level of indenting is suppressed for the two following methods  *
*=====================================================================*/

      if (phd->kcalc & KC_2Q) {

/*---------------------------------------------------------------------*
*  Calculation of MDKS with 2-quads method.  This is the V6 algorithm: *
*  Starting at level 1, loop over all cubes at the current level.      *
*  Check whether any of the coordinate bits of this cube overlap the   *
*  coordinate bits of the current test point.  If there is no overlap, *
*  add the number of points in the target cube into the proper quad    *
*  sum.  If there is overlap, the action depends on what is the cur-   *
*  rent level:  if at the bottom level, loop over the data points that *
*  are members of the current target cube, find out the relationship   *
*  of each target point to the test point and add one to the approp-   *
*  riate quadrant sum.  If not at the bottom level, descend one level  *
*  and perform the quad scan there.  Finally, find the maximum differ- *
*  ence in quadrant counts and set the result statistic to the global  *
*  maximum of this difference.                                         *
*---------------------------------------------------------------------*/

         /* Loop over two target data sets */
         for (jx=0; jx<NDSET; ++jx) {
            Link **ppbm = phd->ppBm0[jx];
            Prec *pqtot = phd->pQtot[jx];

#if MD1SDBG & MDBG_CDTL
if (kdbgp) {
#endif
#if MD1SDBG & MDBG_EXPT
   printf("  -->Processing data set %d\n", jx);
#endif
#if MD1SDBG & MDBG_CDTL
   }
#endif

            /* Initialize for recursive descent--do with nested loops
            *  rather than recursive calls to avoid call overhead.  */
            plvl = phd->pLvl0;
            plvle = plvl + dm1;           /* Test for bottom level */
            plvl->jcube = 0;
            lshft = lshft0;

            /* Loop over up to 2^k nonzero children of current cube */
NewLevel2:  osubj = ptl1->obrk >> lshft & nsmask;
            pbrkj = plvl->pbrk[jx];
            for (jq=plvl->jcube; jq<nq; ++jq) {
               size_t qo;                 /* Relative quadrant */
               size_t qn;                 /* Not-settled quad bits */
               jqq = pbrkj[plvl->ocube | jq].inob;
               if (jqq == nq) break;
               ochld = plvl->ocube | jqq;

#if MD1SDBG & MDBG_CDTL
if (kdbgp) {
#endif
#if MD1SDBG & MDBG_LVLC
   printf("    At level %zd, cube ", plvl - phd->pLvl0);
   dbgqk(phd, ochld);
   printf(", with %.0f points\n", pbrkj[ochld].npts);
#endif
#if MD1SDBG & MDBG_CDTL
   }
#endif

               /* Check for overlaps */
               qo = osubj ^ jqq;          /* Diffs at this level */
               qn = ~qo & plvl->relqn;    /* Clear settled diffs */
               qo = (jqq & qo & plvl->relqn) | plvl->relqo;
               if (!qn) {
                  /* No overlap, all coords different.  Add child npts
                  *  into quadrant determined by child cube coords */
                  pqtot[qo] += pbrkj[ochld].npts;

#if MD1SDBG & MDBG_CDTL
if (kdbgp) {
#endif
#if MD1SDBG & MDBG_OVLP
   printf("      No overlap, new quad %zd total is %.0f\n",
      qo, pqtot[qo]);
#endif
#if MD1SDBG & MDBG_CDTL
   }
#endif
                  continue;
                  }
               else if (plvl == plvle) {
                  /* At bottom level and still overlapping.  Now it is
                  *  necessary to scan the individual points in this
                  *  brick, checking the coords in which the two
                  *  bricks overlap.  */
                  size_t iqo, jjq = 1;
                  int jk, jjk, nkt = 0;

#if MD1SDBG & MDBG_CDTL
if (kdbgp) {
#endif
#if MD1SDBG & MDBG_OVLP
   printf("      Overlap at bottom level\n");
#endif
#if MD1SDBG & MDBG_CDTL
   }
#endif

                  /* Make a list of which coords need to be tested */
                  for (jk=0; plvl->relqn>=jjq; jjq<<=1,++jk) {
                     if (plvl->relqn & jjq) pdim[nkt++].ikt = jk;
                     }

                  for (ptl2=ppbm[ochld]; ptl2; ptl2=ptl2->pnxt) {
                     Xdat const *pX2 = ptl2->pdat;
                     iqo = plvl->relqo;
                     for (jjk=0; jjk<nkt; ++jjk) {
                        jk = pdim[jjk].ikt;
                        if (pX2[jk] == pX1[jk]) {

#if MD1SDBG & MDBG_CDTL
if (kdbgp) {
#endif
#if MD1SDBG & MDBG_BOTL
   printf("      Quad edge point skipped at: ");
   for (kk=0; kk<k; ++kk) printf(" %.5f", pX2[kk]);
   printf(" \n");
#endif
#if MD1SDBG & MDBG_CDTL
   }
#endif
                           goto PointIsOnQuadEdge2;
                           }
                        if (pX2[jk] > pX1[jk])
                           iqo |= 1<<jk;
                        } /* End loop over nkt dimensions */
                     pqtot[iqo] += (Prec)1;

#if MD1SDBG & MDBG_CDTL
if (kdbgp) {
#endif
#if MD1SDBG & MDBG_BOTL
   printf("      Adding one to quad %zd for point at: ", iqo);
   for (kk=0; kk<k; ++kk) printf(" %.5f", pX2[kk]);
   printf("\n      New quad total is %.0f\n", pqtot[iqo]);
#endif
#if MD1SDBG & MDBG_CDTL
   }
#endif

PointIsOnQuadEdge2:  ;
                     } /* End loop over matching coord */
                  }
               else {
                  /* Overlapping, but not yet at bottom level.
                  *  Descend to next lower level and scan there.  */
                  plvl->jcube = jq + 1;      /* Restart point */
                  (++plvl)->jcube = 0;
                  plvl->ocube = ochld << k;
                  plvl->relqo = qo;
                  plvl->relqn = qn;
                  lshft -= k;

#if MD1SDBG & MDBG_CDTL
if (kdbgp) {
#endif
#if MD1SDBG & MDBG_OVLP
   printf("      Overlap, descending to level %zd at brick ",
      plvl - phd->pLvl0);
   dbgqk(phd, ochld);
   printf(" \n");
#endif
#if MD1SDBG & MDBG_CDTL
   }
#endif
                  goto NewLevel2;
                  }
               }  /* End loop at current level */
            /* If already at top level, hierarchy scan is done.
            *  Otherwise, ascend to next quad at next higher
            *  level and continue from there.  */
            if (plvl > phd->pLvl0) {
               plvl -= 1;
               lshft += k;

#if MD1SDBG & MDBG_CDTL
if (kdbgp) {
#endif
#if MD1SDBG & MDBG_LVLC
   printf("      Finished cube, ascending to level %zd\n",
      plvl - phd->pLvl0);
#endif
#if MD1SDBG & MDBG_CDTL
   }
#endif
               goto NewLevel2;
               }
            } /* End loop over both data sets */

         /* Finally --- accumulate the max difference */
         for (jq=0; jq<nq; ++jq) {
            double dnq =
               fabs((double)pnq1[jq] - nrat*(double)pnq2[jq]);

#if MD1SDBG & MDBG_CDTL
if (kdbgp) {
#endif
#if MD1SDBG & MDBG_EXPT
   printf("  Adjusted quad %zd difference is %.3f - %.3f = %.3f\n",
      jq, pnq1[jq], pnq2[jq], dnq);
#endif
#if MD1SDBG & MDBG_CDTL
   }
#endif
            if (dnq > mdks) {
               mdks = dnq;
#if MD1SDBG & MDBG_BIGQ
               ibigq = ptl1 - ptl0;
               jbigq = jq;
#endif
               }
            } /* End loop over quadrants */

         } /* End KC_2Q calculation */

      else {

/*---------------------------------------------------------------------*
*  Calculation of MDKS with 1-quads method. This is the R09 algorithm: *
*  Starting at level 1, loop over all cubes at the current level.      *
*  Check whether any of the coordinate bits of this cube overlap the   *
*  coordinate bits of the current test point.  If there is no overlap, *
*  add the differential number of points in the target cube into the   *
*  proper quad sum.  If there is overlap, the action depends on what   *
*  is the current level:  if at the bottom level, loop over the data   *
*  points that are members of the current target cube, find out the    *
*  relationship of each target point to the test point and add (or     *
*  subtract) one to the appropriate quadrant sum.  If not at the bot-  *
*  tom level, descend one level and perform the quadrant scan there.   *
*  Finally, find the maximum |difference|in quadrant counts and set    *
*  the result statistic to the global maximum of this quantity.        *
*  -------------------------------------------------------------------*/

         /* Initialize for recursive descent--do with nested loops
         *  rather than recursive calls to avoid call overhead.  */
         plvl = phd->pLvl0;
         plvle = plvl + dm1;           /* Test for bottom level */
         plvl->jcube = 0;
         lshft = lshft0;

         /* Loop over up to 2^k nonzero children of current cube */
NewLevel1:  osubj = ptl1->obrk >> lshft & nsmask;
         pbrkj = plvl->pbrk1;
         for (jq=plvl->jcube; jq<nq; ++jq) {
            size_t qo;                 /* Relative quadrant */
            size_t qn;                 /* Not-settled quad bits */
            jqq = pbrkj[plvl->ocube | jq].inob;
            if (jqq == nq) break;
            ochld = plvl->ocube | jqq;

#if MD1SDBG & MDBG_CDTL
if (kdbgp) {
#endif
#if MD1SDBG & MDBG_LVLC
   printf("    At level %zd, cube ", plvl - phd->pLvl0);
   dbgqk(phd, ochld);
   printf(", with %.3f points different\n", pbrkj[ochld].npts);
#endif
#if MD1SDBG & MDBG_CDTL
   }
#endif

            /* Check for overlaps */
            qo = osubj ^ jqq;          /* Diffs at this level */
            qn = ~qo & plvl->relqn;    /* Clear settled diffs */
            qo = (jqq & qo & plvl->relqn) | plvl->relqo;
            if (!qn) {
               /* No overlap, all coords different.  Add child npts
               *  into quadrant determined by child cube coords */
               pnq1[qo] += pbrkj[ochld].npts;

#if MD1SDBG & MDBG_CDTL
if (kdbgp) {
#endif
#if MD1SDBG & MDBG_OVLP
   printf("      No overlap, new quad %zd total is %.3f\n",
      qo, pnq1[qo]);
#endif
#if MD1SDBG & MDBG_CDTL
   }
#endif
               continue;
               }
            else if (plvl == plvle) {
               /* At bottom level and still overlapping.  Now it is
               *  necessary to scan the individual points in this
               *  brick for both data sets, counting each point in the
               *  quadrant in which it falls relative to the test
               *  point.  The quadrant number is made up by combining
               *  the dimensions in which the bricks are already known
               *  to differ (plvl->relqo) with the individual
               *  difference in the other dimensions.  */
               size_t jjq = 1, iqo;
               int jk, jjk, jx, nkt = 0;     /* Loop indexes */

#if MD1SDBG & MDBG_CDTL
if (kdbgp) {
#endif
#if MD1SDBG & MDBG_OVLP
   printf("      Overlap at bottom level\n");
#endif
#if MD1SDBG & MDBG_CDTL
   }
#endif

               /* Make a list of which coords need to be tested */
               for (jk=0; plvl->relqn>=jjq; jjq<<=1,++jk) {
                  if (plvl->relqn & jjq) pdim[nkt++].ikt = jk;
                  }
               /* Loop over two data sets */
               for (jx=0; jx<NDSET; ++jx) {
                  Link **ppbm = phd->ppBm0[jx];
                  Prec npadd = jx ? -nrat : (Prec)1;

                  for (ptl2=ppbm[ochld]; ptl2; ptl2=ptl2->pnxt) {
                     Xdat const *pX2 = ptl2->pdat;
                     iqo = plvl->relqo;
                     for (jjk=0; jjk<nkt; ++jjk) {
                        jk = pdim[jjk].ikt;
                        if (pX2[jk] == pX1[jk]) {

#if MD1SDBG & MDBG_CDTL
if (kdbgp) {
#endif
#if MD1SDBG & MDBG_BOTL
   printf("         Quad edge point skipped at: ");
   for (kk=0; kk<k; ++kk) printf(" %.5f", pX2[kk]);
   printf(" \n");
#endif
#if MD1SDBG & MDBG_CDTL
   }
#endif
                           goto PointIsOnQuadEdge1;
                           }
                        if (pX2[jk] > pX1[jk])
                           iqo |= 1<<jk;
                        } /* End loop over k dimensions */
                     pnq1[iqo] += npadd;

#if MD1SDBG & MDBG_CDTL
if (kdbgp) {
#endif
#if MD1SDBG & MDBG_BOTL
   printf("         Adding %.3f to quad %zd for point at: ",
      npadd, iqo);
   for (kk=0; kk<k; ++kk) printf(" %.5f", pX2[kk]);
   printf("\n         New quad total is %.3f\n", pnq1[iqo]);
#endif
#if MD1SDBG & MDBG_CDTL
   }
#endif

PointIsOnQuadEdge1:  ;
                     } /* End loop over matching coord */
                  } /* End loop over jx */
               }
            else {
               /* Overlapping, but not yet at bottom level.
               *  Descend to next lower level and scan there.  */
               plvl->jcube = jq + 1;      /* Restart point */
               (++plvl)->jcube = 0;
               plvl->ocube = ochld << k;
               plvl->relqo = qo;
               plvl->relqn = qn;
               lshft -= k;

#if MD1SDBG & MDBG_CDTL
if (kdbgp) {
#endif
#if MD1SDBG & MDBG_OVLP
   printf("      Overlap, descending to level %zd at brick ",
      plvl - phd->pLvl0);
   dbgqk(phd, ochld);
   printf(" \n");
#endif
#if MD1SDBG & MDBG_CDTL
   }
#endif
               goto NewLevel1;
               }
            }  /* End loop at current level */
         /* If already at top level, hierarchy scan is done.
         *  Otherwise, ascend to next quad at next higher
         *  level and continue from there.  */
         if (plvl > phd->pLvl0) {
            plvl -= 1;
            lshft += k;

#if MD1SDBG & MDBG_CDTL
if (kdbgp) {
#endif
#if MD1SDBG & MDBG_LVLC
   printf("      Finished cube, ascending to level %zd\n",
      plvl - phd->pLvl0);
#endif
#if MD1SDBG & MDBG_CDTL
   }
#endif
            goto NewLevel1;
            }

         /* Finally --- accumulate the max difference */
         for (jq=0; jq<nq; ++jq) {
            double dnq = fabs((double)pnq1[jq]);

#if MD1SDBG & MDBG_CDTL
if (kdbgp) {
#endif
#if MD1SDBG & MDBG_EXPT
   printf("  Adjusted quad %zd difference is %.3f\n", jq, dnq);
#endif
#if MD1SDBG & MDBG_CDTL
   }
#endif
            if (dnq > mdks) {
               mdks = dnq;
#if MD1SDBG & MDBG_BIGQ
               ibigq = ptl1 - ptl0;
               jbigq = jq;
#endif
               }
            } /* End loop over quadrants */

         } /* End one-quad array calculation */

/*=====================================================================*
*                 End one-level indenting suppression                  *
*=====================================================================*/

         } /* End loop over bricks in ix1 set */

      } /* End k > 1 */

   else {

/*---------------------------------------------------------------------*
*  Calculation of MDKS for k == 1.  Scan through X1 list.  For each    *
*  point, compute number of points below this X in both lists, then    *
*  their difference and set the statistic to the global maximum of     *
*  this difference.  Be careful to avoid counting duplicates until     *
*  past them.                                                          *
*---------------------------------------------------------------------*/

      Link *plj1 = phd->pLnk1[ix1];
      Link *plj2 = phd->pLnk1[ix2];
      double dnq;
      Xdat Xprev = *plj1->pdat;
      Nint nq1 = 1, nq2 = 0;

      while (plj1) {

         /* Skip over any duplicates and increment nq1 */
         while (plj1->pnxt && *plj1->pnxt->pdat == Xprev)
            ++nq1, plj1 = plj1->pnxt;

         /* For the second data set a crawling pointer is used
         *  that keeps just inside the X of the current point in
         *  the first data set.  */
         while (plj2 && *plj2->pdat <= Xprev)
            ++nq2, plj2 = plj2->pnxt;

         /* Accumulate the max difference */
         dnq = fabs((double)nq1 - nrat*(double)nq2);
         if (dnq > mdks) {
            mdks = dnq;
#if MD1SDBG & MDBG_BIGQ
            ibigq = plj1 - ptl0;
            jbigq = 0;
#endif
            }

         /* Advance to next point in first list */
         if ((plj1 = plj1->pnxt))      /* Assignment intended */
            ++nq1, Xprev = *plj1->pdat;

         } /* End loop over X1 points */
      } /* End k == 1 */

#if MD1SDBG & MDBG_BIGQ
   printf("Big diff at point %d, quad %zd\n", ibigq, jbigq);
#endif
   return mdks/phd->sqrtn[ix1];

   } /* End mdksqdx() */


/*=====================================================================*
*                               mdks1sx                                *
*=====================================================================*/

double vfnname(mdks1s,VL)(void *pwk, Xdat const *X1, Xdat const *X2) {

   Wkhd *phd = (Wkhd *)pwk;
   int  k;                 /* Number of dimensions */

/* Preliminaries */

   /* Validate the work area and pick up dimensionality */
#ifndef I_AM_MATLAB        /* Test already done if MATLAB */
   if (phd->hdrlbl != HLBL) return MDKS_ERR_BADWK;
#endif
   if (!(phd->kcalc & KC_MDKS)) return MDKS_ERR_BADWK;
   k = phd->k;
#if MD1SDBG & MDBG_CDTL
if (k == 2 && phd->n[0] == 50) ++phd->repct;
#endif

/*---------------------------------------------------------------------*
*             Set up brick data structure for case k > 1.              *
*---------------------------------------------------------------------*/

   if (k > 1) {
      Dim  *pdim;
      Link *pnl,*ptl;
      Lvl  *plvl,*plvle,*plvl0;
      size_t nq = (size_t)1 << k;   /* Number of quadrants */
      Nint ndd;               /* Number of divisions at depth d */
      Nint tn;                /* Total points in both data sets */
      int  kc = phd->kcalc;   /* Calculation method control */
      int  td = phd->d;       /* Depth of brick hierarchy */
      int  jk;                /* Index for loops over k */
      int  jx;                /* Index for loops over X1 and X2 */
      int  kq;                /* Type of run: 1 or 2 quad arrays */
#if MD1SDBG & MDBG_CDTL
      int kdbgp;
#endif
#if MD1SDBG & (MDBG_BRCT|MDBG_BRPL|MDBG_INPT)
      int ii;
#endif

      /* Determine whether this is a 1-quad or 2-quad run */
      if (kc & KC_QS2) kq = 2;
      else if (kc & KC_QS1) kq = 1;
      else if (k >= K2Q_MD) kq = 2;
      else kq = 1;
      if (kq == 2) {
         if (!(kc & KC_QS2A)) return MDKS_ERR_No2QA;
         phd->kcalc |= KC_2Q;
         }
      else {
         phd->kcalc &= ~KC_2Q;
         }

      /* Initialize all the bricks for both data sets */
      plvl = plvle = (plvl0 = phd->pLvl0) + td - 1;
      memset(plvl0->pbrk1, 0, phd->lbrks);
      memset(phd->ppBm0[IXb], 0, phd->lbml);

      /* Manufacture a linked list spanning both data sets.
      *  Note: k*n cannot overflow because the input data
      *  are known to fit in memory.  */
      ptl = pnl = phd->pLnk1[IXb] = phd->pLnk0[IXb];
      for (jx=0; jx<NDSET; ++jx) {
         Xdat const *pX,*pXe,*pX0 = jx ? X2 : X1;

#if MD1SDBG & MDBG_CDTL
kdbgp = (k == 2 && phd->n[0] == 50 && kq == 2 && phd->repct == 46);
if (kdbgp) {
#endif
#if MD1SDBG & MDBG_INPT
   printf("\n==>Input data for set %d\n", jx);
   ii  = 0;
#endif
#if MD1SDBG & MDBG_CDTL
   }
#endif
         pXe = pX0 + (size_t)k*(size_t)phd->n[jx];
         for (pX=pX0; pX<pXe; pX+=k) {
            ptl = pnl++;
            ptl->pnxt = pnl;
            ptl->pdat = pX;
            ptl->obrk = 0;

#if MD1SDBG & MDBG_CDTL
if (kdbgp) {
#endif
#if MD1SDBG & MDBG_INPT
   {  int kk;
      printf("  X%d[%d]:", jx,ii++);
      for (kk=0; kk<k; ++kk) printf(" %9.5f", pX[kk]);
      printf(" \n");
      }
#endif
#if MD1SDBG & MDBG_CDTL
   }
#endif
            }
         } /* End loop over both data sets */
      ptl->pnxt = NULL;

/* Stage I.  For each of k dimensions, sort the points from both
*  data sets into one single sequence and then loop over the
*  sorted data to determine which brick each point belongs to.  */

      tn = phd->n[IX1] + phd->n[IX2];  /* Total data */
      ndd = (Nint)1 << td;
      pdim = phd->pDim0;
      for (jk=0; jk<k; ++pdim,++jk) {
         size_t obrtd = 0;    /* Offset to current brick */
         Nint jjd = ndd;      /* Axial division counter */
         Nint nppd;           /* Number points per division */
         Nint tnr = tn;       /* Total number remaining */

         /* Sort the points on the current dimension */
         vfnname(mdkssrt,VL)(phd, IXb, jk);

#if MD1SDBG & MDBG_CDTL
if (kdbgp) {
#endif
#if MD1SDBG & MDBG_INPT
   printf("\n==>Sorted data for dimension %d\n", jk);
   for (ptl=phd->pLnk1[IXb]; ptl; ptl=ptl->pnxt) {
      const Xdat *pX = ptl->pdat;
      int kk,jjx,jii = ptl - phd->pLnk1[IXb];
      if ((jjx = jii > phd->n[0])) jii -= phd->n[0];
      printf("  X%d[%d]:", jjx,jii);
      for (kk=0; kk<k; ++kk) printf(" %9.5f", pX[kk]);
      printf(" \n");
      }
#endif
#if MD1SDBG & MDBG_CDTL
   }
#endif
         /* Scan the sorted list of data points, dividing it into 2^td
         *  regions.  Start with (rem data)/(rem rgns) points in each
         *  region but keep points with identical coords in same region.
         *  Accumulate the brick number in the Link obrk field.  */
         ptl = phd->pLnk1[IXb];
         while (tnr) {
            nppd = tnr/jjd;
            tnr -= nppd;
            while (nppd-- > 0) {
               ptl->obrk |= obrtd;
               ptl = (pnl = ptl)->pnxt;
               }
            /* Must put more points in this brick if coord is same */
            while (ptl) {
               if (ptl->pdat[jk] == pnl->pdat[jk]) {
                  ptl->obrk |= obrtd;
                  tnr -= 1;
                  ptl = ptl->pnxt;
                  }
               else break;
               } /* End assignment to one brick */
            /* Advance to next brick */
            jjd -= 1;
            obrtd = (obrtd + pdim->oadd) & pdim->mask;
            } /* End loop over points in both data sets */

         } /* End loop over dimensions */

/* Stages II and III.  At this point the setup differs according to
*  whether there will be two sets of quad sums (kcalc & KC_2Q) or one
*  set.  Because the linked list now spans the merged data, both cases
*  can just loop over the Links treated as arrays.  */

      ptl = phd->pLnk0[IXb];
      if (phd->kcalc & KC_2Q) {

/* Stage II for two sets of quad sums:  Now working separately on the
*  two data sets, at each brick make a list of the data points that
*  belong to that brick and count them.  */

         for (jx=0; jx<NDSET; ++jx) {
            Link **ppbm0 = phd->ppBm0[jx];
            Brick *pbrkj = (plvl = plvle)->pbrk[jx];
            Link *ptle = ptl + phd->n[jx];
            for ( ; ptl<ptle; ++ptl) {
               Link **ppbr = ppbm0 + ptl->obrk;
               /* Increment count of brick members */
               pbrkj[ptl->obrk].npts += (Prec)1;
               /* Insert this point at the head of the member list of
               *  the selected brick (might as well be at head, order
               *  does not matter).  */
               ptl->pnxt = *ppbr;
               *ppbr = ptl;
               } /* End loop over data points */

/* Stage III for KC_2Q:  Assemble brick count hierarchy working up
*  from bottom.  Also store the indices of the occupied (nonzero
*  count) bricks so the algorithm in mdksqdx can skip empty bricks.
*/

            {  Brick *pdown,*pdwne,*pinob,*pup,*pupe;
               Nint jnob;
               while (plvl > plvl0) {
                  pdown = plvl->pbrk[jx];
                  pup = (--plvl)->pbrk[jx];
                  pupe = pup + (nq << (plvl-plvl0)*k);
                  for ( ; pup<pupe; ++pup) {
                     pdwne = (pinob = pdown) + nq;
                     for (jnob=0; pdown<pdwne; ++jnob,++pdown) {
                        if (pdown->npts > 0) (pinob++)->inob = jnob;
                        pup->npts += pdown->npts;
                        }
                     /* Mark end of list */
                     if (pinob < pdown) pinob->inob = nq;
                     }
                  } /* End nontop-level brick consolidation */
               /* Compute top-level inobs */
               pdown = plvl->pbrk[jx];
               pdwne = (pinob = pdown) + nq;
               for (jnob=0; pdown<pdwne; ++jnob,++pdown)
                  if (pdown->npts > 0) (pinob++)->inob = jnob;
               if (pinob < pdown) pinob->inob = nq;
               } /* End cosolidation local scope */

#if MD1SDBG & MDBG_CDTL
if (kdbgp) {
#endif
#if MD1SDBG & MDBG_BRCT
   for (ii=0; ii<td; ++ii) {
      Brick *pbr = phd->pLvl0[ii].pbrk[jx];
      size_t ibrk, nbrks = (size_t)1 << (ii+1)*k;
      printf("\n==>Level %d brick counts for set %d\n", ii, jx);
      for (ibrk=0; ibrk<nbrks; ++pbr,++ibrk) {
         printf("  At");
         dbgqk(phd, ibrk);
#if KTN == 0
         printf(" %.3f, inob = %d\n", pbr->npts, pbr->inob);
#else
         printf(" %.3f, inob = %ld\n", pbr->npts, pbr->inob);
#endif
         }
      }
#endif
#if MD1SDBG & MDBG_CDTL
   }
#endif
            } /* End loop over data sets X1 and X2 */
         } /* End KC_2Q case--two sets of quad sums */

      else {
         double nrat = (double)phd->n[IX1]/(double)phd->n[IX2];

/* Stage II for one set of quad sums.  Now working separately on the
*  two data sets, at each brick on the bottom level make lists of the
*  data points that belong to that brick.  Also record the difference
*  in counts between the two data sets and use the inob field
*  temporarily as a flag to indicate when npts == 0 because the two
*  data sets have the same number of points in the current brick (when
*  the mdksqd routine needs to descend to the bottom level to skip
*  edge points).  */

         for (jx=0; jx<NDSET; ++jx) {
            Link **ppbm0 = phd->ppBm0[jx];
            Brick *pbrkj = (plvl = plvle)->pbrk1;
            Link *ptle = ptl + phd->n[jx];
            Prec npadd = jx ? -nrat : (Prec)1;
            for ( ; ptl<ptle; ++ptl) {
               Link **ppbr = ppbm0 + ptl->obrk;
               /* Increment difference count of brick members */
               pbrkj[ptl->obrk].npts += npadd;
               pbrkj[ptl->obrk].inob = 1;
               /* Insert this point at the head of the member list of
               *  the selected brick (might as well be at head, order
               *  does not matter).  */
               ptl->pnxt = *ppbr;
               *ppbr = ptl;
               } /* End loop over data points */
            } /* End loop over data sets X1 and X2 */

/* Stage III for 1-quad.  Assemble brick count hierarchy working up
*  from bottom.  Also store the indices of the occupied (nonzero
*  count) bricks so the algorithm in mdksqdx can skip empty bricks.
*/

         {  Brick *pdown,*pdwne,*pinob,*pup,*pupe;
            Nint jnob;
            while (plvl > plvl0) {
               pdown = plvl->pbrk1;
               pup = (--plvl)->pbrk1;
               pupe = pup + (nq << (plvl-plvl0)*k);
               for ( ; pup<pupe; ++pup) {
                  pdwne = (pinob = pdown) + nq;
                  for (jnob=0; pdown<pdwne; ++jnob,++pdown) {
                     pup->npts += pdown->npts;
                     pup->inob |= pdown->inob;
                     if (pdown->inob) (pinob++)->inob = jnob;
                     }
                  /* Mark end of list */
                  if (pinob < pdown) pinob->inob = nq;
                  }
               } /* End nontop-level brick consolidation */
            /* Compute top-level inobs */
            pdown = plvl->pbrk1;
            pdwne = (pinob = pdown) + nq;
            for (jnob=0; pdown<pdwne; ++jnob,++pdown)
               if (pdown->inob) (pinob++)->inob = jnob;
            if (pinob < pdown) pinob->inob = nq;
            } /* End consolidation local scope */

#if MD1SDBG & MDBG_CDTL
if (kdbgp) {
#endif
#if MD1SDBG & MDBG_BRCT
   for (ii=0; ii<td; ++ii) {
      Brick *pbr = phd->pLvl0[ii].pbrk1;
      size_t ibrk, nbrks = (size_t)1 << (ii+1)*k;
      printf("\n==>Level %d brick difference counts\n", ii);
      for (ibrk=0; ibrk<nbrks; ++pbr,++ibrk) {
         printf("  At");
         dbgqk(phd, ibrk);
#if KTN == 0
         printf(" %.3f, inob = %d\n", pbr->npts, pbr->inob);
#else
         printf(" %.3f, inob = %ld\n", pbr->npts, pbr->inob);
#endif
         }
      }
#endif
#if MD1SDBG & MDBG_CDTL
   }
#endif
         } /* End !KC_2Q case--one set of quad sums */

#if MD1SDBG & MDBG_CDTL
if (kdbgp) {
#endif
#if MD1SDBG & MDBG_BRPL
   /* DEBUG to print brick membership lists */
   for (jx=0; jx<NDSET; ++jx) {
      Link **ppbm0 = phd->ppBm0[jx];
      size_t ibrk,nbrks = (size_t)1 << td*k;
      printf("\n==>Bottom-level (%d) brick members for set %d\n",
         td-1, jx);
      for (ibrk=0; ibrk<nbrks; ++ibrk) {
         Link *ptl, *pbm = ppbm0[ibrk];
         if (!pbm) continue;
         printf("   At");
         dbgqk(phd, ibrk);
         printf(":\n");
         for (ptl=pbm; ptl; ptl=ptl->pnxt) {
            Xdat const *pX = ptl->pdat;
            int kk;
            printf("     ");
            for (kk=0; kk<k; ++kk) printf(" %.5f", pX[kk]);
            printf(" \n");
            } /* End data point loop */
         } /* End brick loop */
      } /* End data set loop */
#endif
#if MD1SDBG & MDBG_CDTL
   }
#endif

      } /* End case k > 1 */

/*---------------------------------------------------------------------*
*      With k == 1, there is effectively just one quadrant (2^0)       *
*---------------------------------------------------------------------*/

   else {
      int  jx;                /* Index for loops over X1 and X2 */

      for (jx=0; jx<NDSET; ++jx) {     /* Loop over 2 data sets */
         Xdat const *pX,*pXe,*pX0 = jx ? X2 : X1;
         Link *pnl,*ptl;
         Nint tn = phd->n[jx];

         /* Manufacture linked list for this data set */
         pXe = pX0 + (size_t)tn;
         ptl = pnl = phd->pLnk1[jx] = phd->pLnk0[jx];
         for (pX=pX0; pX<pXe; ++pX) {
            ptl = pnl++;
            ptl->pnxt = pnl;
            ptl->pdat = pX;
            }
         ptl->pnxt = NULL;

         /* Sort the data */
         vfnname(mdkssrt,VL)(phd, jx, 0);

         } /* End loop over X1 and X2 */

      } /* End case k == 1 */

   /* Compute and return the single-sided test */
   return vfnname(mdksqd,VL)(phd, 0);

   } /* End mdks1sx() */

