/* (c) Copyright 2013-2016, The Rockefeller University */
/* $Id: rmks1s.c 6 2014-06-09 19:19:58Z  $ */
/***********************************************************************
*           Multi-Dimensional Kolmogorov-Smirnov Statistic             *
*                              rmks1s.c                                *
*                                                                      *
*  This software was written by Aparna Nair-Kanneganti and George N.   *
*  Reeke in the Laboratory of Biological Modelling at The Rockefel-    *
*  ler University.  Please send any corrections, suggestions, or       *
*  improvements to the author by email to reeke@rockefeller.edu for    *
*  possible incorporation in future revisions.                         *
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
*                               rmks1s                                 *
*                                                                      *
*  The Kolmogorov-Smirnov test may be used to quantitate the degree    *
*  to which two distributions of points in k-dimensional space are     *
*  statistically distinguishable.  This function computes the one-     *
*  sided two-dimensional version of the test using the "ribbon"        *
*  approximation described below.  Versions for exact and approximate  *
*  calculations for this and other dimensions are also available in    *
*  this MDKS package.                                                  *
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
*     The rmks component of this package is applicable only to two-    *
*  dimensional data sets.  Double- or single-precision floating point  *
*  data sets (not necessarily equal in size) are compared by this      *
*  program.  The two data sets are referred to as 'X1' and 'X2' here   *
*  and in the comments.  If 'ni' is the number of points in data set i *
*  (i={1,2}) then the data must be created by the caller in arrays of  *
*  size ni x 2 (C ordering, last index fast moving).  Note that if     *
*  creating the data in a MATLAB program, the order of the dimensions  *
*  should be reversed to avoid the need for a transpose operation.  No *
*  restriction is made that the values in each dimension need to cover *
*  the same range (but if they do not, it is likely that a high value  *
*  of the KS statistic will be obtained).                              *
*     The program requires a work area that may be reused with multi-  *
*  ple calls to rmks1sx() that have the same ni, gi, and k parameters. *
*  This area is allocated (or modified for different parameters) by a  *
*  call to mdksallox() and freed by a call to mdksfree().              *
*                                                                      *
*  MATLAB Synopses:                                                    *
*     s = rmks1sx(MDKSCOM, X1, X2)                                     *
*  C Synopses:                                                         *
*     double rmks1sx(void *pwk, Xdat const *X1, Xdat const *X2)        *
*                                                                      *
*  Arguments:                                                          *
*     X1,X2    Pointers to the sets of points to be compared.  X1 and  *
*              X2 must be arrays of whatever type the mdks1sx version  *
*              is compiled for.  (The type Xdat is determined by the   *
*              compile-time variable 'KTX' which is 0 for 'float' and  *
*              1 for 'double').  X1 has dimension n1 x 2 (C) or 2 x n1 *
*              (MATLAB) and X2 has dimension n2 x 2 (C) or 2 x n2      *
*              (MATLAB), where 'n1', and 'n2' are parameters in a      *
*              previous call to mdksallox().  The data points are      *
*              assumed to be stored in the native byte order of the    *
*              machine on which the program runs.                      *
*     pwk      This must be the pointer stored at *ppwk by a call to   *
*              mdksallox() with k == 2 and the 'ni' of X1 and X2 and   *
*              with 'KC_RMKS' included in the 'g2' control bits.       *
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
*     rmks1sx() returns an approximation to the two-dimensional MDKS   *
*  statistic, computed as described in the Algorithm section below.    *
*  A negative value is returned in case of an error as follows:        *
*     (-1) The argument 'pwk' apparently did not point to an area      *
*          allocated by an earlier call to mdksallox().                *
*     (-2) All the coordinates in some dimension were identical.       *
*     (-4) This routine was called, but k was not 2.                   *
*  The MATLAB mex file gives a terminal error if the operation was not *
*  successful.                                                         *
*                                                                      *
*  Algorithm:                                                          *
*     This program uses an algorithm developed by ANK based on the     *
*  binary tree method of GNR also included in this package (mdks1sx()  *
*  routines).  The obvious exact algorithm runs in time proportional   *
*  to (n^2)*(2^k).  We were unable to find an algorithm that is prov-  *
*  ably faster than this, but, depending on the distribution of the    *
*  data, the algorithm used here can run significantly faster than the *
*  brute-force algorithm (timing data in the 'readme' file).  The new  *
*  algorithm divides the data space into a hierarchy of rectilinear    *
*  bricks.  At each successive hierarchy level, each coordinate axis   *
*  is divided so as to place roughly 1/(2^k) of the data points in     *
*  each child brick.  It is key to the logic of the new algorithm that *
*  the coordinate boundaries of the bricks are identical across both   *
*  data sets.  The user may specify the depth of the hierarchy         *
*  (usually only for test purposes) or accept the default, which is    *
*  computed according to a polynomial given in the mdksallo.c source   *
*  file.  Beginning at the lowest level, each brick is provided with a *
*  list of the enclosed points and the count of such points.  Going up *
*  the hierarchy, each brick holds the count of the data points        *
*  contained in the 2^k (here 4) bricks contained in that brick at the *
*  next lower level.                                                   *
*     After this setup, the MDKS is initialized to 0.  For each data   *
*  point in set X1, the populations of two sets of 4 quadrants around  *
*  that point (one for X1, one for X2) are set to 0.  The program then *
*  starts at the top of the hierarchy and recursively scans the bricks *
*  at the current level.  If the brick does not overlap the brick that *
*  contains the current target point, the count for that brick is      *
*  added into the total for the appropriate quadrant.  If there are    *
*  overlaps, the scan descends to the next lower level until either    *
*  nonoverlapping bricks, empty bricks, or the lowest level is         *
*  reached.  At the lowest level, if the bricks overlap in both        *
*  coordinates, individual points are examined to determine the        *
*  appropriate quadrant in which to count them.                        *
*     If the overlap is in only one coordinate, the ribbon method      *
*  makes use of the fact that an entire "ribbon" (column or row of     *
*  bricks) constructed as is done here with identical sequences of     *
*  boundary values in each dimension, has the same overlap.  The       *
*  total number of points in each arm of the cross formed by two       *
*  intersection ribbons are allocated to the quadrant to the left      *
*  or right of that arm (if vertical) or to the quadrant above or      *
*  below that arm (if horizontal) in proportion to the position of     *
*  the mean coordinate of the points in the ribbon relative to the     *
*  edges of the current ribbon.                                        *
*     The quadrant counts for the two data sets can then be subtracted *
*  and used to update the global MDKS.                                 *
*                                                                      *
*  Notes:                                                              *
*     The authors cited below propose the statistic that is computed   *
*  by this routine and give suggestions for computing significance     *
*  levels (p-values) from the MDKS statistic (not done here).  They    *
*  also show how to perform a two-sample test by averaging the results *
*  from the tests with both orderings of the two data sets as X1 and   *
*  X2, multiplied by sqrt(n1*n2/(n1+n2)).  This is done by the rmks2sx *
*  routines.                                                           *
*     'n1' and 'n2' are usually 32 bit ints and 'k' must be less than  *
*  MXK so the hypercube vertex numbers are positive signed ints.  The  *
*  code could be modified to handle larger numbers of points and/or    *
*  dimensions, but the execution time would probably be prohibitive    *
*  even with the fast algorithm used here.                             *
*     The ribbon method code only implements the "two-quad sum" option.*
*  Because there are only four quads, some loops over quads in the     *
*     k-general routines are unrolled here to eliminate loop overhead. *
*     The code to scan the bricks is a subroutine so it can be used    *
*  for both argument orders by the rmks2sx() code.                     *
*                                                                      *
*  Reference:                                                          *
*     G. Fasano & A. Franceschini (1987).  A multidimensional version  *
*  of the Kolmogorov-Smirnov test.  Monthly Notices of the Royal       *
*  Astronomical Society, 225:155-170.                                  *
************************************************************************
*  V1A, ??/??/??, Nair Kanneganti and G.N. Reeke - New program         *
*  ==>, 04/16/16, GNR - Last date before committing to svn repository  *
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
*  makefile.  If compiled without external definitions, rmks1s will    *
*  default to double-Precision data with integer ni and name 'rmks1s'. *
*  (Appropriate typedefs are in the header file mdksint.h)             *
*=====================================================================*/

#ifdef I_AM_MATLAB
#define MyName vfnmex("rmks1s",VL)
#define printf mexPrintf

enum rhsargs { jwk, jX1, jX2 };

/*=====================================================================*
*                    rmks1sx mex function wrapper                      *
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
      mexErrMsgTxt(MyName " X1 arg must be a 2D float array.");
   pDimX = mxGetDimensions(prhs[jX1]);
   if (pDimX[0] != phd->k || pDimX[1] != phd->n[IX1])
      mexErrMsgTxt(MyName " X1 arg must be a 2 x n1 array.");
   pX1 = (Xdat const *)mxGetData(prhs[jX1]);

   if (!mxIsSingle(prhs[jX2]) ||
         mxGetNumberOfDimensions(prhs[jX2]) != NDIMS)
      mexErrMsgTxt(MyName " X2 arg must be a 2D float array.");
   pDimX = mxGetDimensions(prhs[jX2]);
   if (pDimX[0] != phd->k || pDimX[1] != phd->n[IX2])
      mexErrMsgTxt(MyName " X2 arg must be a 2 x n2 array.");
   pX2 = (Xdat const *)mxGetData(prhs[jX2]);
#else
   if (!mxIsDouble(prhs[jX1]) ||
         mxGetNumberOfDimensions(prhs[jX1]) != NDIMS)
      mexErrMsgTxt(MyName " X1 arg must be a 2D double array.");
   pDimX = mxGetDimensions(prhs[jX1]);
   if (pDimX[0] != phd->k || pDimX[1] != phd->n[IX1])
         mexErrMsgTxt(MyName " X1 arg must be a 2 x n1 array.");
   pX1 = (Xdat const *)mxGetData(prhs[jX1]);

   if (!mxIsDouble(prhs[jX2]) ||
         mxGetNumberOfDimensions(prhs[jX2]) != NDIMS)
      mexErrMsgTxt(MyName " X2 arg must be a 2D double array.");
   pDimX = mxGetDimensions(prhs[jX2]);
   if (pDimX[0] != phd->k || pDimX[1] != phd->n[IX2])
      mexErrMsgTxt(MyName " X2 arg must be a 2 x n2 array.");
   pX2 = (Xdat const *)mxGetData(prhs[jX2]);
#endif

/* Perform the statistical computation and return the result */

   s = vfnname(rmks1s,VL)(pwk, pX1, pX2);
   plhs[0] = mxCreateDoubleScalar(s);

   } /* End mexFunction() */

#endif /* I_AM_MATLAB */


/*=====================================================================*
*                              rmkssrtx                                *
*                                                                      *
*  Sort a list of data points in increasing order along some dimension *
*  using a radix sort (like an old card sorter) with time proportional *
*  to n*sizeof(Xdat).                                                  *
*=====================================================================*/

static void vfnname(rmkssrt,VL)(Wkhd *phd, int jx, int jk) {

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
   } /* End rmkssrtx() */


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
*                               rmksqdx                                *
*                                                                      *
*  Compute the MDKS.  This routine is called twice for the two-sided   *
*  test with X1 and X2 reversed so that repetition of the setup can be *
*  avoided.  The argument 'ix1' indicates which data set is to be      *
*  treated as the 'experimental' or 'master' data.                     *                                                         *
*=====================================================================*/

#define JX  0                    /* Index to access x */
#define JY  1                    /* Index to access y */

double vfnname(rmksqd,VL)(Wkhd *phd, int ix1) {

   Ribn *prib0 = phd->pRib0;     /* Ptr to ribbon info */
   Link *ptl0 = phd->pLnk0[ix1]; /* Ptr to test data */
   Link *ptl1,*ptl1e,*ptl2;      /* Ptrs to test data */
   Prec *pnq1,*pnq2;             /* Ptr to quad sums */
   double rmks = 0.0;            /* The result */
   double nrat;                  /* n[ix1]/n[ix2] */
   Prec gapxlength, gapylength;
   Prec usum, dsum, rsum, lsum;
   Prec right, down, left, up;
   Prec ribct[NRQ];
   int dm1 = phd->d - 1;         /* Depth (fill gap to next size_t) */
   int k = phd->k;               /* (Error check below for k != 2) */
   size_t lqds = sizeof(Prec) << (k+1);
   size_t nq = (size_t)1 << k;   /* Number of quadrants */
   size_t nsmask = nq - 1;       /* Not settled mask */
   size_t ixrib,iyrib;           /* x,y ribbon indices */
#if MD1SDBG & MDBG_BIGQ
   int jbigq = 0;                /* Data for printing biggest diff */
   int ibigq = -1;
#endif
#if MD1SDBG & MDBG_CDTL
   int idcell = 8;            /* Cell selected for debug print */
   int kdbgp = 0;
#endif
   int ix2 = IX2 - ix1;       /* Index of second (model) data set */
   int jx;                    /* Data set index */
   int lshft, lshft0 = dm1*k;
#if MD1SDBG > 0
   int kk;
#endif

   /* Not using phd->nrat because needs to be reversed for mdks2s */
   nrat = (double)phd->n[ix1]/(double)phd->n[ix2];

/*---------------------------------------------------------------------*
*  Calculation of MDKS for k == 2.  This is the ribbon approximation   *
*  algorithm:  Loop over all points in the ix1 set.  For each point,   *
*  zero both sets of quadrant counts.  Starting at level 1, loop over  *
*  all cubes at the current level.  Check whether any of the coordi-   *
*  nate bits of this cube overlap the coordinate bits of the current   *
*  source point.  If there is no overlap, add the number of points in  *
*  the target cube into the proper quad sum.  If there is overlap, the *
*  action depends on what is the current level:  if at the bottom      *
*  level, and not in the same cube location, apply the ribbon algo-    *
*  rithm described below.  If in the same brick, test the individual   *
*  points as in the plain mdks algorithm.  If not at the bottom level, *
*  descend one level and perform the octant scan there.  Finally, find *
*  the maximum difference in quadrant counts and set the result        *
*  statistic to the global maximum of this difference.                 *
*  The Bin space is reused to keep track of the quadrant counts.       *
*---------------------------------------------------------------------*/

   pnq1 = phd->pQtot[ix1], pnq2 = phd->pQtot[ix2];
   /* Set up level 0 as if we've come down from a higher one */
   phd->pLvl0->ocube = phd->pLvl0->relqo = 0;
   phd->pLvl0->relqn = nsmask;

   /* Here is the main loop over test points (points in set ix1).
   *  Note that the Link pnxt fields are now in use as brick
   *  member list pointers, so treat Link list as an array.  */
   ptl1 = ptl0;
   ptl1e = ptl1 + phd->n[ix1];
   for ( ; ptl1<ptl1e; ++ptl1) {
      Xdat const *pX1 = ptl1->pdat;
      size_t jq, jqq;

#if MD1SDBG & MDBG_CDTL
   kdbgp = (phd->n[0] == 50 && phd->repct == 46 &&
      ptl1 - ptl0 == idcell);
   if (kdbgp) {
#endif
#if MD1SDBG & MDBG_EXPT
      printf("\n==>Processing exptl point at brick");
      dbgqk(phd, ptl1->obrk);
      printf(", coords:\n  ");
      for (kk=0; kk<k; ++kk) printf(" %.4f", ptl1->pdat[kk]);
      printf(" \n");
#endif
#if MD1SDBG & MDBG_CDTL
      }
#endif

      /* Xero both sets of quadrant counts */
      memset(phd->pQtot[IXb], 0, lqds);

      /* Find position of current test point in its ribbon.
      *  Note:  (NRD = 2) << d Ribns are allocated.  These are indexed
      *  as a linear array of x,y pairs.  The ribbon numbers can be
      *  obtained with no tests by rearranging the obrk bits.  */
      {  size_t xymask, orib = ptl1->obrk << 1;
         ixrib = JX, iyrib = JY;
         for (xymask = 2; xymask <= orib; xymask <<= 1) {
            ixrib |= (orib & xymask);
            orib >>= 1;
            iyrib |= (orib & xymask);
            }
         } /* End xymask, orib local scope */
      gapxlength = pX1[JX] - prib0[ixrib].qmin;
      left = gapxlength * prib0[ixrib].rdiff;
      right = 1.0 - left;
      gapylength = pX1[JY] - prib0[iyrib].qmin;
      up = gapylength * prib0[iyrib].rdiff;
      down = 1.0 - up;

      /* Loop over two target data sets */
      for (jx=0; jx<NDSET; ++jx) {
         Brick *pbrkj;
         Lvl  *plvl,*plvle;
         Link **ppbm = phd->ppBm0[jx];
         Prec *pqtot = phd->pQtot[jx];
         Dim *pdim = phd->pDim0;
         size_t osubj,ochld;
         size_t tbrkx,tbrky;     /* This brick x,y offsets */

#if MD1SDBG & MDBG_CDTL
if (kdbgp) {
#endif
#if MD1SDBG & MDBG_EXPT
   printf("  -->Processing data set %d\n", jx);
#endif
#if MD1SDBG & MDBG_CDTL
   }
#endif
         memset(ribct, 0, sizeof(ribct));
         usum = dsum = rsum = lsum = 0;

         /* Initialize for recursive descent--do with nested loops
         *  rather than recursive calls to avoid call overhead.  */
         plvl = phd->pLvl0;
         plvle = plvl + dm1;           /* Test for bottom level */
         plvl->jcube = 0;
         lshft = lshft0;

         /* Loop over up to 2^k nonzero children of current cube */
NewLevel: osubj = ptl1->obrk >> lshft & nsmask;
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

            else if (plvl == plvle)
               continue;

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
               goto NewLevel;
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
            goto NewLevel;
            }

         /* If the current level is also the lowest level, begin
         *  approximation algorithm, which assumes that within bricks
         *  the data is uniformly distributed, and therefore uses a
         *  geometric approach to assign points to quadrants.  */

#if MD1SDBG & MDBG_CDTL
if (kdbgp) {
#endif
#if MD1SDBG & MDBG_OVLP
   printf("      Overlap at bottom level\n");
#endif
#if MD1SDBG & MDBG_CDTL
   }
#endif

         /* Calculate the numbers of points in the four arms of the
         *  cross centered on the current IX1 brick.  */
         tbrkx = ptl1->obrk & pdim[JX].mask;
         tbrky = ptl1->obrk & pdim[JY].mask;
         if (prib0[iyrib].rdiff) {
            size_t xbrk = 0;
            while (xbrk < tbrkx) {  /* Left sum */
               lsum += pbrkj[xbrk|tbrky].npts;
               xbrk = (xbrk + pdim[JX].oadd) & pdim[JX].mask;
               }
            /* Skip brick test point is in */
            xbrk = (xbrk + pdim[JX].oadd) & pdim[JX].mask;
            while (xbrk != 0) {     /* Right sum */
               rsum += pbrkj[xbrk|tbrky].npts;
               xbrk = (xbrk + pdim[JX].oadd) & pdim[JX].mask;
               }
            } /* End horizontal arms */

#if MD1SDBG & MDBG_CDTL
if (kdbgp) {
#endif
#if MD1SDBG & MDBG_BOTL
   printf ("      Left arm sum = %.4f, right arm sum = %.4f\n",
      lsum, rsum);
#endif
#if MD1SDBG & MDBG_CDTL
   }
#endif

         if (prib0[ixrib].rdiff) {
            size_t ybrk = 0;
            while (ybrk < tbrky) {  /* Upper sum */
               usum += pbrkj[ybrk|tbrkx].npts;
               ybrk = (ybrk + pdim[JY].oadd) & pdim[JY].mask;
               }
            /* Skip brick test point is in */
            ybrk = (ybrk + pdim[JY].oadd) & pdim[JY].mask;
            while (ybrk != 0) {     /* Lower sum */
               dsum += pbrkj[ybrk|tbrkx].npts;
               ybrk = (ybrk + pdim[JY].oadd) & pdim[JY].mask;
               }
            } /* End vertical arms */

#if MD1SDBG & MDBG_CDTL
if (kdbgp) {
#endif
#if MD1SDBG & MDBG_BOTL
   printf ("      Upper arm sum = %.4f, lower arm sum = %.4f\n",
      usum, dsum);
#endif
#if MD1SDBG & MDBG_CDTL
   }
#endif

         if (phd->kcalc & KC_ROKS) {
            /* Apportion points in overlap brick according to
            *  test point fractional distances */
            Prec novb = pbrkj[ptl1->obrk].npts;
            pqtot[0] += novb * left * up;
            pqtot[1] += novb * right * up;
            pqtot[2] += novb * left * down;
            pqtot[3] += novb * right * down;
            }

         else {
            /* Assign points in overlap brick exactly according
            *  to which quadrant each point falls in.  */
            for (ptl2 = ppbm[ptl1->obrk]; ptl2; ptl2 = ptl2->pnxt) {
               Xdat const *pX2 = ptl2->pdat;
               int iqo = 0;
               if (pX2[JX] == pX1[JX]) goto PointIsOnQuadEdge;
               if (pX2[JY] == pX1[JY]) goto PointIsOnQuadEdge;
               if (pX2[JX] > pX1[JX]) iqo += (1<<JX);
               if (pX2[JY] > pX2[JY]) iqo += (1<<JY);
               pqtot[iqo] += 1;

#if MD1SDBG & MDBG_CDTL
if (kdbgp) {
#endif
#if MD1SDBG & MDBG_BOTL
   printf("      Adding one to quad %d for point at: "
      "%.5f, %.5f\n", iqo, pX2[JX], pX2[JY]);
   printf("      New quad total is %.5f\n", pqtot[iqo]);
#endif
#if MD1SDBG & MDBG_CDTL
   }
#endif

PointIsOnQuadEdge: ;
               } /* End loop over matching coords */
            } /* End else not KC_ROKS */

         ribct[0] = usum*left  + lsum*up;
         ribct[1] = usum*right + rsum*up;
         ribct[2] = dsum*left  + lsum*down;
         ribct[3] = dsum*right + rsum*down;

         pqtot[0] += ribct[0];
         pqtot[1] += ribct[1];
         pqtot[2] += ribct[2];
         pqtot[3] += ribct[3];

#if MD1SDBG & MDBG_CDTL
if (kdbgp) {
#endif
#if MD1SDBG & MDBG_BOTL
   printf("     Quad 0, data set %d: %.4f = stuff + %.4f\n",
      jx, pqtot[0], ribct[0]);
   printf("     Quad 1, data set %d: %.4f = stuff + %.4f\n",
      jx, pqtot[1], ribct[1]);
   printf("     Quad 2, data set %d: %.4f = stuff + %.4f\n",
      jx, pqtot[2], ribct[2]);
   printf("     Quad 3, data set %d: %.4f = stuff + %.4f\n",
      jx, pqtot[3], ribct[3]);
#endif
#if MD1SDBG & MDBG_CDTL
   }
#endif
         } /* End loop over both data sets */


      /* Finally --- accumulate the max difference */

      for (jq=0; jq<nq; ++jq) {
         double dnq = fabs((double)pnq1[jq] - nrat*(double)pnq2[jq]);

#if MD1SDBG & MDBG_CDTL
if (kdbgp) {
#endif
#if MD1SDBG & MDBG_DIFF
   printf("  Adjusted quad %zd difference is %.3f - %.3f = %.3f\n",
      jq, pnq1[jq], pnq2[jq], dnq);
#endif
#if MD1SDBG & MDBG_CDTL
   }
#endif

         if (dnq > rmks) {
            rmks = dnq;
#if MD1SDBG & MDBG_BIGQ
            ibigq = ptl1 - ptl0;
            jbigq = jq;
#endif
            }
         } /* End loop over quadrants */

      } /* End loop over bricks in ix1 set */

#if MD1SDBG & MDBG_BIGQ
   printf("Big diff at point %d, quad %d\n", ibigq, jbigq);
#endif
   return rmks/phd->sqrtn[ix1];

   } /* End rmksqdx() */


/*=====================================================================*
*                               rmks1sx                                *
*=====================================================================*/

double vfnname(rmks1s,VL)(void *pwk, Xdat const *X1, Xdat const *X2) {

  Wkhd *phd = (Wkhd *)pwk;
  int  k;                 /* Number of dimensions */

  /* Preliminaries */

  /* Validate the work area and pick up dimensionality */
#ifndef I_AM_MATLAB        /* Test already done if MATLAB */
  if (phd->hdrlbl != HLBL) return MDKS_ERR_BADWK;
#endif
   if (!(phd->kcalc & (KC_RMKS|KC_ROKS))) return MDKS_ERR_BADWK;
   k = phd->k;
#if MD1SDBG & MDBG_CDTL
if (phd->n[0] == 50) ++phd->repct;
#endif

/*---------------------------------------------------------------------*
*            Set up brick data structure for case k == 2.              *
*---------------------------------------------------------------------*/

   if (k == 2) {
      Dim  *pdim;
      Link *pnl,*ptl;
      Lvl  *plvl,*plvle,*plvl0;
      Ribn *prib0 = phd->pRib0;
      size_t nq = (size_t)1 << k;   /* Number of quadrants */
      Nint ndd;               /* Number of divisions at depth d */
      Nint tn;                /* Total points in both data sets */
      int  td = phd->d;       /* Depth of brick hierarchy */
      int  jk;                /* Index for loops over k */
      int  jx;                /* Index for loops over X1 and X2 */

#if MD1SDBG & MDBG_CDTL
      int kdbgp;
#endif
#if MD1SDBG & (MDBG_BRCT|MDBG_BRPL|MDBG_INPT)
      int ii;
#endif

      /* Initialize all the bricks for both data sets */
      plvl = plvle = (plvl0 = phd->pLvl0) + td - 1;
      memset(plvl0->pbrk[IXb], 0, phd->lbrks);
      memset(phd->ppBm0[IXb], 0, phd->lbml);

      /* Manufacture a linked list spanning both data sets.
      *  Note: k*n cannot overflow because the input data
      *  are known to fit in memory.  */
      ptl = pnl = phd->pLnk1[IXb] = phd->pLnk0[IXb];
      for (jx=0; jx<NDSET; ++jx) {
         Xdat const *pX,*pXe,*pX0 = jx ? X2 : X1;

#if MD1SDBG & MDBG_CDTL
kdbgp = (phd->n[0] == 50 && phd->repct == 46);
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
         size_t irib = 0;     /* Index of current ribbon */
         Nint jjd = ndd;      /* Axial division counter */
         Nint nppd;           /* Number points per division */
         Nint tnr = tn;       /* Total number remaining */

         /* Sort the points on the current dimension */
         vfnname(rmkssrt,VL)(phd, IXb, jk);

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
            Xdat ribdx;
            prib0[irib+jk].qmin = ptl->pdat[jk];
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
            ribdx = pnl->pdat[jk] - prib0[irib+jk].qmin;
            prib0[irib+jk].rdiff = (ribdx != 0.0) ? 1.0/ribdx : 0;
            irib += NRD;
            jjd -= 1;
            obrtd = (obrtd + pdim->oadd) & pdim->mask;
            } /* End loop over points in both data sets */

         } /* End loop over dimensions */

/* Stage II.  If this is a KC_ROKS run, for each of the two data sets,
*  make a list at each brick of the data points that belong to that
*  brick and count them.  If this is a KC_RMKS run, we just need the
*  counts, not the stored lists.  Because the linked list now spans
*  the merged data, we just loop over the Links treated as arrays.  */

      ptl = phd->pLnk0[IXb];
      for (jx=0; jx<NDSET; ++jx) {
         Link **ppbr,**ppbm0 = phd->ppBm0[jx];
         Brick *pbrkj = (plvl = plvle)->pbrk[jx];
         Link *ptle = ptl + phd->n[jx];
         for ( ; ptl<ptle; ++ptl) {
            /* Increment count of brick members */
            pbrkj[ptl->obrk].npts += 1;
            if (!(phd->kcalc & KC_ROKS)) {
               /* Insert this point at the head of the member list of
               *  the selected brick (might as well be at head, order
               *  does not matter).  */
               ppbr = ppbm0 + ptl->obrk;
               ptl->pnxt = *ppbr;
               *ppbr = ptl;
               }
            } /* End loop over data points */

/* Stage III.  Assemble brick count hierarchy working up from bottom.
*  Also store the indices of the occupied (nonzero count) bricks so
*  the algorithm in rmksqdx can skip empty bricks.  */

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
            } /* End consolidation local scope */

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

      } /* End case k == 2 */
   /* Error if not k == 2 */
   else  return MDKS_ERR_RMKN2;

   /* Compute and return the single-sided test */
   return vfnname(rmksqd,VL)(phd, 0);

   } /* End rmks1sx() */

