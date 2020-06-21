/* (c) Copyright 2015, The Rockefeller University */
/* $Id: epks1s.c 9 2016-01-04 19:30:24Z  $ */
/***********************************************************************
*           Multi-Dimensional Kolmogorov-Smirnov Statistic             *
*                              epks1s.c                                *
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
*                         epks1sd and epks1sf                          *
*                                                                      *
*  These functions compute an "equi-partitioned" approximation to the  *
*  one-sided multidimensional Kolmogorov-Smirnov statistic.  The MDKS  *
*  may be used to quantitate the degree to which two distributions of  *
*  points in k-dimensional space are statistically distinguishable.    *
*  This version allocates points in "bricks" whose coordinates overlap *
*  those of the current test point equally among all quadrants that so *
*  overlap.  Slower versions that exactly compute the version of the   *
*  statistic defined in the Fasano and Franceschini reference given    *
*  below are named mdks1sd and mkds1sf, respectively.                  *
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
*     The program requires a work area that may be reused with multi-  *
*  ple calls to any of the one- or two-sided routines in the package   *
*  (except the "brute force" routines) that have the same ni, g1, and  *
*  k parameters.  This area is allocated (or modified for different    *
*  parameters) by a call to mdksallox() and freed by a call to         *
*  mdksfree().                                                         *
*                                                                      *
*  MATLAB Synopses:                                                    *
*     s = epks1sx(MDKSCOM, X1, X2)                                     *
*  C Synopses:                                                         *
*     double epks1sx(void *pwk, Xdat const *X1, Xdat const *X2)        *
*                                                                      *
*  Arguments:                                                          *
*     X1,X2    Pointers to the sets of points to be compared.  X1 and  *
*              X2 must be arrays of whatever type the epks1sx version  *
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
*              and with 'KC_EPKX' included in the 'g2 'control bits.   *
*     MDKSCOM  MATLAB equivalent of pwk.                               *
*                                                                      *
*  Return values:                                                      *
*     epks1sx() returns an approximation to the one-sided MDKS statis- *
*  tic (see "Algorithm" below).  MDKS is defined as the maximum over   *
*  the 'n1' points of data set X1 of the maximum over the 2^k general- *
*  ized quadrants in a k-dimensional coordinate system with origin at  *
*  each such point of the absolute value of the difference between the *
*  numbers of points in that quadrant in X1 and in X2, normalized as   *
*  if n2 == n1 and divided by sqrt(n1).  Note that with the one-sided  *
*  test, X1 should be the experimental data and X2 the model.          *
*  A negative value is returned in case of an error as follows:        *
*     (-1) The argument 'pwk' apparently did not point to an area      *
*          allocated by an earlier call to mdksallox() or the 'g2'     *
*          argument did not include the 'KC_EPKS' method code.         *
*     (-2) All the coordinates in some dimension were identical.       *
*  The MATLAB mex file gives a terminal error if the operation was not *
*  successful.                                                         *
*                                                                      *
*  Algorithm:                                                          *
*     This program uses an algorithm developed by GNR (possibly not    *
*  original).  The obvious algorithm runs in time proportional to      *
*  (n^2)*(2^k).  The exact algorithm in mdks1sx() improves on this     *
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
*  allocated equally to all the quadrants that have overlapping        *
*  coordinates.  For large numbers of equally distributed data points, *
*  on average this will give small differences in the quadrant counts  *
*  and the statistic will be approximately correct.                    *
*                                                                      *
*  Notes:                                                              *
*     The authors cited below propose the statistic that is computed   *
*  by mdks1sx() and give suggestions for computing significance        *
*  levels (p-values) from the MDKS statistic (not done here).  They    *
*  also show how to perform a two-sample test by averaging the results *
*  from the tests with both orderings of the two data sets as X1 and   *
*  X2, multiplied by sqrt(n1*n2/(n1+n2)).  This is done by the epks2sx *
*  routines.                                                           *
*     'n1' and 'n2' are usually 32 bit ints and 'k' must be less than  *
*  MXK so the hypercube vertex numbers are positive signed ints.  The  *
*  code could be modified to handle larger numbers of points and/or    *
*  dimensions, but the execution time would probably be prohibitive    *
*  even with the fast algorithm used here.                             *
*     When k == 1, a faster algorithm based on sorting the coordinates *
*  is used.  Code for this algorithm is included here.                 *
*     The code to scan the bricks is a subroutine so it can be used    *
*  for both argument orders by the epks2sx() code.                     *
*                                                                      *
*  Reference:                                                          *
*     G. Fasano & A. Franceschini (1987).  A multidimensional version  *
*  of the Kolmogorov-Smirnov test.  Monthly Notices of the Royal       *
*  Astronomical Society, 225:155-170.                                  *
************************************************************************
*  V7A, 07/18/15, G.N. Reeke - New program, modfied from mdks1s        *
*  Rev, 08/21/15, GNR - Store quad differences in one set of bricks    *
*  ==>, 08/22/15, GNR - Last date before committing to svn repository  *
*  Rev, 12/19/15, GNR - Bug fix:  Negative numbers sorted high->low    *
***********************************************************************/

/* Add following constants in MD1SDBG for desired debug outputs: */
#define MDBG_EXPT    1  /* Print experimental point and data set */
#define MDBG_LVLC    2  /* Print level and cube entered */
#define MDBG_OVLP    4  /* Print results of overlap tests */
#define MDBG_BOTL    8  /* Print actions at bottom level */
#define MDBG_BRCT   16  /* Print list of bricks and point counts */
#define MDBG_INPT   64  /* Print list of input data */

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

size_t ncbitcnt(void *, size_t);    /* Count on bits in a string */

/*=====================================================================*
*  N.B.  This program is intended to be included in a wrapper program  *
*  that defines the type specifiers KTX and KTN and the version iden-  *
*  tifier VL.  Alternatively, these variables can be defined in a      *
*  makefile.  If compiled without external definitions, epks1s will    *
*  default to double-precision data with integer ni and name 'epks1s'. *
*  (Appropriate typedefs are in the header file mdksint.h)             *
*=====================================================================*/

#ifdef I_AM_MATLAB
#define MyName vfnmex("epks1s",VL)
#define printf mexPrintf

enum rhsargs { jwk, jX1, jX2 };

/*=====================================================================*
*                    epks1sx mex function wrapper                      *
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

   s = vfnname(epks1s,VL)(pwk, pX1, pX2);
   plhs[0] = mxCreateDoubleScalar(s);

   } /* End mexFunction() */

#endif /* I_AM_MATLAB */


/*=====================================================================*
*                              epkssrtx                                *
*                                                                      *
*  Sort a list of data points in increasing order along some dimension *
*  using a radix sort (like an old card sorter) with time proportional *
*  to n*sizeof(Xdat).                                                  *
*=====================================================================*/

static void vfnname(epkssrt,VL)(Wkhd *phd, int jx, int jk) {

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
   } /* End epkssrtx() */


#if MD1SDBG & (MDBG_LVLC|MDBG_OVLP|MDBG_BOTL|MDBG_BRCT)
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
*                               epksqdx                                *
*                                                                      *
*  Compute the MDKS.  This routine is called twice for the two-sided   *
*  test with X1 and X2 reversed so that repetition of the setup can be *
*  avoided.  The argument 'ix1' indicates which data set is to be      *
*  treated as the 'experimental' or 'master' data.                     *                                                         *
*=====================================================================*/

double vfnname(epksqd,VL)(Wkhd *phd, int ix1) {

   double epks = 0.0;         /* The result */
   double nrat;               /* n[ix1]/n[ix2] */
   int k = phd->k;
   int ix2 = IX2 - ix1;       /* Index of second (model) data set */

   nrat = (double)phd->n[ix1]/(double)phd->n[ix2];
   if (k > 1) {

/*---------------------------------------------------------------------*
*  Calculation of MDKS for k > 1.  This is the modified V7A algorithm: *
*  Loop over all points in the X1 set.  For each point, zero the quad- *
*  rant counts.  Starting at level 1, loop over all cubes at the cur-  *
*  rent level.  Check whether any of the coordinate bits of this cube  *
*  overlap the coordinate bits of the current test point.  If there is *
*  no overlap, add the differential number of points in the target     *
*  cube into the proper quad sum.  If there is overlap, the action     *
*  depends on what is the current level:  If at the bottom level,      *
*  divide the count evenly over the quadrants that overlap the quad-   *
*  rant that the current test point belongs to.  If not at the bottom  *
*  level, descend one level and perform the quadrant scan there.       *
*  Finally, find the maximum difference in quadrant counts and set     *
*  the result statistic to the global maximum of this quantity.        *
*  The Bin space is reused to keep track of the quadrant counts.       *
*---------------------------------------------------------------------*/

      Prec *pqtot = phd->pQtot1;    /* Quad difference totals */
      Link *ptl1,*ptl1e;
      size_t lqds = (size_t)sizeof(Prec) << k;
      size_t nq = (size_t)1 << k;   /* Number of quadrants */
      size_t nsmask = nq - 1;       /* Not settled mask */
      int dm1 = phd->d - 1;
      int lshft, lshft0 = dm1*k;    /* Level brick index shift */

#if MD1SDBG > 0
   int kk;
#endif

      phd->pLvl0->ocube = phd->pLvl0->relqo = 0;
      phd->pLvl0->relqn = nsmask;

/* Here is the main loop over test points (points in set ix1).
*  Note that the Link pnxt fields are now in use as brick
*  member list pointers, so treat Link list as an array.  */

      ptl1 = phd->pLnk0[ix1];
      ptl1e = ptl1 + phd->n[ix1];
      for ( ; ptl1<ptl1e; ++ptl1) {
         Brick *pbrkj;
         Lvl  *plvl,*plvle;
         size_t osubj,ochld;
         size_t jq, jqq;

#if MD1SDBG & MDBG_EXPT
   printf("\n==>Processing test point at brick");
   dbgqk(phd, ptl1->obrk);
   printf(", coords:\n  ");
   for (kk=0; kk<k; ++kk) printf(" %.5f", ptl1->pdat[kk]);
   printf(" \n");
#endif

         /* Zero quadrant counts */
         memset(phd->pQtot1, 0, lqds);

         /* Initialize for recursive descent--do with nested loops
         *  rather than recursive calls to avoid call overhead.  */
         plvl = phd->pLvl0;
         plvle = plvl + dm1;           /* Test for bottom level */
         plvl->jcube = 0;
         lshft = lshft0;

         /* Loop over up to 2^k nonzero children of current cube */
NewLevel:   osubj = ptl1->obrk >> lshft & nsmask;
         pbrkj = plvl->pbrk1;
         for (jq=plvl->jcube; jq<nq; ++jq) {
            size_t qo;                 /* Relative quadrant */
            size_t qn;                 /* Not-settled quad bits */
            jqq = pbrkj[plvl->ocube | jq].inob;
            if (jqq == nq) break;
            ochld = plvl->ocube | jqq;

#if MD1SDBG & MDBG_LVLC
   printf("    At level %zd, cube ", plvl - phd->pLvl0);
   dbgqk(phd, ochld);
   printf(", with %.3f points different\n", pbrkj[ochld].npts);
#endif

            /* Check for overlaps */
            qo = osubj ^ jqq;          /* Diffs at this level */
            qn = ~qo & plvl->relqn;    /* Clear settled diffs */
            qo = (jqq & qo & plvl->relqn) | plvl->relqo;
            if (!qn) {
               /* No overlap, all coords different.  Add child npts
               *  into quadrant determined by child cube coords */
               pqtot[qo] += pbrkj[ochld].npts;

#if MD1SDBG & MDBG_OVLP
   printf("      No overlap, new quad %zd total is %.3f\n",
      qo, pqtot[qo]);
#endif
               continue;
               }
            else if (plvl == plvle) {
               /* At bottom level and still overlapping.  Divide the
               *  points in the current brick among the quads where
               *  there is coordinate overlap.  If all coordinates
               *  overlap, this brick overlaps the test point, so
               *  subtract one from the difference for the edge over-
               *  lap.  In the epks approximation, there is no way
               *  to catch overlaps in coordinates of other data
               *  points (which should not happen with continuous
               *  data).  */
               size_t iqo,magic;    /* For loop over qn bits */
               Prec nppq;           /* Points per quad */
               Prec novl;           /* Number of overlap dims */

               novl = (Prec)(1<<ncbitcnt(&qn, sizeof(qn)));
               nppq = (pbrkj[ochld].npts - (qn == nsmask))/ novl;
               magic = (~qn + 1);

#if MD1SDBG & MDBG_OVLP
   printf("      Overlap at bottom level, novl = %.0f, nppq = %.3f\n",
      novl, nppq);
#endif

               iqo = 0;
               do {
                  pqtot[qo | iqo] += nppq;

#if MD1SDBG & MDBG_BOTL
   printf("         Adding %.3f to quad %zd for brick %zd: ",
      nppq,qo|iqo,ochld);
   printf("\n         New quad total is %.3f\n", pqtot[qo|iqo]);
#endif
                  iqo = (iqo + magic) & qn;
                  } while (iqo);
               }
            else {
               /* Overlapping, but not yet at bottom level.
               *  Descend to next lower level and scan there.  */
               plvl->jcube = jq + 1;
               (++plvl)->jcube = 0;
               plvl->ocube = ochld << k;
               plvl->relqo = qo;
               plvl->relqn = qn;
               lshft -= k;

#if MD1SDBG & MDBG_OVLP
   printf("      Overlap, descending to level %zd at brick ",
      plvl - phd->pLvl0);
   dbgqk(phd, ochld);
   printf(" \n");
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

#if MD1SDBG & MDBG_LVLC
   printf("      Finished cube, ascending to level %zd\n",
      plvl - phd->pLvl0);
#endif
            goto NewLevel;
            }

         /* Finally --- accumulate the max difference */
         for (jq=0; jq<nq; ++jq) {
            double dnq = fabs((double)pqtot[jq]);

#if MD1SDBG & MDBG_EXPT
   printf("  Adjusted quad %zd difference is %.3f\n", jq, dnq);
#endif
            if (dnq > epks) epks = dnq;
            } /* End loop over quadrants */

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
         if (dnq > epks) epks = dnq;

         /* Advance to next point in first list */
         if ((plj1 = plj1->pnxt))      /* Assignment intended */
            ++nq1, Xprev = *plj1->pdat;

         } /* End loop over X1 points */
      } /* End k == 1 */

   return epks/phd->sqrtn[ix1];

   } /* End epksqdx() */


/*=====================================================================*
*                               epks1sx                                *
*=====================================================================*/

double vfnname(epks1s,VL)(void *pwk, Xdat const *X1, Xdat const *X2) {

   Wkhd *phd = (Wkhd *)pwk;
   int  k;                 /* Number of dimensions */

/* Preliminaries */

   /* Validate the work area and pick up dimensionality */
#ifndef I_AM_MATLAB        /* Test already done if MATLAB */
   if (phd->hdrlbl != HLBL) return MDKS_ERR_BADWK;
#endif
   if (!(phd->kcalc & KC_EPKS)) return MDKS_ERR_BADWK;
   k = phd->k;

/*---------------------------------------------------------------------*
*             Set up brick data structure for case k > 1.              *
*---------------------------------------------------------------------*/

   if (k > 1) {
      Dim  *pdim;
      Link *pnl,*ptl;
      Lvl  *plvl,*plvle,*plvl0;
      size_t nq = (size_t)1 << k;   /* Number of quadrants */
      Nint tn;                /* Total points in both data sets */
      Nint ndpd;              /* Number points per division */
      Nint rdiv;              /* Number divisions with an extra point */
      int  td = phd->d;       /* Depth of brick hierarchy */
      int  jk;                /* Index for loops over k */
      int  jx;                /* Index for loops over X1 and X2 */

#if MD1SDBG & (MDBG_BRCT|MDBG_INPT)
      int ii;
#endif

      /* Initialize all the bricks for both data sets */
      plvl = plvle = (plvl0 = phd->pLvl0) + td - 1;
      memset(plvl0->pbrk1, 0, phd->lbrks);

      /* Manufacture a linked list spanning both data sets.
      *  Note: k*n cannot overflow because the input data
      *  are known to fit in memory.  */
      ptl = pnl = phd->pLnk1[IXb] = phd->pLnk0[IXb];
      for (jx=0; jx<NDSET; ++jx) {
         Xdat const *pX,*pXe,*pX0 = jx ? X2 : X1;

#if MD1SDBG & MDBG_INPT
   printf("\n==>Input data for set %d\n", jx);
   ii  = 0;
#endif
         pXe = pX0 + (size_t)k*(size_t)phd->n[jx];
         for (pX=pX0; pX<pXe; pX+=k) {
            ptl = pnl++;
            ptl->pnxt = pnl;
            ptl->pdat = pX;
            ptl->obrk = 0;

#if MD1SDBG & MDBG_INPT
   {  int kk;
      printf("  X%d[%d]:", jx,ii++);
      for (kk=0; kk<k; ++kk) printf(" %9.5f", pX[kk]);
      printf(" \n");
      }
#endif
            }
         } /* End loop over both data sets */
      ptl->pnxt = NULL;

/* Stage I.  For each of k dimensions, sort the points from both
*  data sets into one single sequence and then loop over the
*  sorted data to determine which brick each point belongs to.  */

      /* Calculate number of points in each axial division */
      tn = phd->n[IX1] + phd->n[IX2];
      ndpd = tn >> td;
      rdiv = tn - (ndpd << td);

      /* Loop over dimensions, compute brick offsets */
      pdim = phd->pDim0;
      for (jk=0; jk<k; ++pdim,++jk) {
         size_t obrtd = 0;    /* Offset to current brick */
         Nint idiv,jdiv;      /* Indexes for axial division */
         Nint jjd;            /* Axial division counter */

         /* Sort the points on the current dimension */
         vfnname(epkssrt,VL)(phd, IXb, jk);

         /* Scan the sorted list of data points, dividing it into 2^td
         *  regions with ndpd+1 points in the first rdiv regions and
         *  ndpd points in the rest.  Accumulate the brick number in
         *  the Link obrk field.  */
         ptl = phd->pLnk1[IXb];
         jjd = (size_t)1 << td;
         jdiv = rdiv;
         while (jjd-- > 0) {
            idiv = ndpd + (jdiv-- > 0);
            while (idiv-- > 0) {
               ptl->obrk |= obrtd;
               ptl = ptl->pnxt;
               } /* End assignment to one brick */
            obrtd = (obrtd + pdim->oadd) & pdim->mask;
            } /* End loop over axial divisions */

         } /* End loop over dimensions */

/* Stage II.  At each brick on the bottom level record the difference
*  in counts between the two data sets.  The lists of points used with
*  KC_MDKS method are not needed here.  Use the inob field temporarily
*  as a flag to indicate when npts == 0 because the two data sets have
*  the same number of points in the current brick (when the mdksqd
*  routine needs to descend to the bottom level to skip edge points).
*  Because the linked list now spans the merged data, we just loop
*  over the Links treated as arrays.  */

      ptl = phd->pLnk0[IXb];
      for (jx=0; jx<NDSET; ++jx) {
         Brick *pbrkj = (plvl = plvle)->pbrk1;
         Link *ptle = ptl + phd->n[jx];
         Prec npadd = (Prec)(1 - (jx<<1));
         for ( ; ptl<ptle; ++ptl) {
            pbrkj[ptl->obrk].npts += npadd;
            pbrkj[ptl->obrk].inob = 1;
            } /* End loop over data points */
         } /* End loop over data sets X1 and X2 */

/* Stage III.  Assemble brick count hierarchy working up from bottom.
*  Also store the indices of the occupied (nonzero count) bricks so
*  the algorithm in epksqdx can skip empty bricks.  */

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
         } /* End cosolidation local scope */

#if MD1SDBG & MDBG_BRCT
   for (ii=0; ii<td; ++ii) {
      Brick *pbr = phd->pLvl0[ii].pbrk1;
      size_t ibrk, nbrks = (size_t)1 << (ii+1)*k;
      printf("\n==>Level %d brick difference counts\n", ii);
      for (ibrk=0; ibrk<nbrks; ++pbr,++ibrk) {
         printf("  At");
         dbgqk(phd, ibrk);
         printf(" %.3f, inob = %d\n", pbr->npts, pbr->inob);
         }
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
         vfnname(epkssrt,VL)(phd, jx, 0);

         } /* End loop over X1 and X2 */

      } /* End case k == 1 */

   /* Compute and return the single-sided test */
   return vfnname(epksqd,VL)(phd, 0);

   } /* End epks1sx() */

