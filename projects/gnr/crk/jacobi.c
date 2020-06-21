/* (c) Copyright 1995-2008, The Rockefeller University *11115* */
/* $Id: jacobi.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                               JACOBI                                 *
*                                                                      *
*  Routine for computing eigenvalues and, optionally, eigenvectors of  *
*  a double-precision symmetric real matrix using Jacobi's method as   *
*  adapted by Von Neumann and described in  "Mathematical Methods for  *
*  Digital Computers", A. Ralston and H.S. Wilf, eds, John Wiley and   *
*  Sons, New York, 1962, Chapter 7, with improvements included in the  *
*  Jacobi routine in "Numerical Recipes in C", 2nd edition, W.H. Press *
*  et al., Cambridge University Press, 1992.  Corrections to diagonal  *
*  elements are accumulated in an auxiliary vector to reduce errors.   *
*                                                                      *
*  It should be noted that more efficient methods (e.g. Householder)   *
*  are available for large matrices.  The present routine could be     *
*  improved by changing all subscripting to pointer constructs.        *
*                                                                      *
*  C code based in part on ideas from the routine 'JACOBI' in "Nume-   *
*  rical Recipes in C" and using the triangular matrix storage scheme  *
*  of the IBM "Scientific Subroutines Package", 1966 (in FORTRAN).     *
*                                                                      *
*  Usage:                                                              *
*     int jacobi(double *a, double *val, double *vec, double *dd,      *
*        int N, int kvec, int ksort)                                   *
*                                                                      *
*  Description of parameters:                                          *
*     a     Double precision upper triangular part of input symmetric  *
*           N x N matrix stored columnwise (A11, A12, A22, A13, etc).  *
*           On return, the diagonal elements of a are replaced with    *
*           the eigenvalues of a in arbitrary order.  The rest of      *
*           matrix a is destroyed (set to 0) during the computation.   *
*     val   Double precision vector of order N to contain the eigen-   *
*           values of a.  Sorting is controlled by ksort (see below).  *
*     vec   Double precision matrix of order N x N to contain the      *
*           eigenvectors of a (may be a NULL pointer if kvec == 0).    *
*           Vectors are stored columnwise in same order as             *
*           corresponding eigenvalues.                                 *
*     dd    Work vector of order N for internal use by the routine.    *
*     N     Order of matrix a and resulting eigenvalues and vectors.   *
*     kvec  Control code:                                              *
*              0     Compute eigenvalues only                          *
*              1     Compute eigenvalues and eigenvectors.             *
*     ksort Control code:                                              *
*              0     Do not sort the results                           *
*              1     Sort results in decreasing order of               *
*                       eigenvalue magnitude.                          *
*                                                                      *
*  Return values:                                                      *
*     -1    Calculation failed to converge after MaxJacobiIter         *
*           iterations                                                 *
*     n     Convergence was achieved after n iterations                *
*                                                                      *
*  Remarks:                                                            *
*     The upper triangular part of matrix a is assumed to be stored    *
*     columnwise in N*(N+1)/2 successive storage locations.  Storage   *
*     for a, val, vec, and dd must not overlap in memory.              *
*                                                                      *
*     The routine will work in a 64-bit environment.  Note that N is   *
*     restricted to the largest value that will fit in an int, but     *
*     anything anywhere near this large really requires a more         *
*     sophisticated treatment.                                         *
*                                                                      *
*  Other routines required:                                            *
*     sqrt                                                             *
*                                                                      *
************************************************************************
*  V1A, 12/07/95, Written by G.N. Reeke                                *
*  V2A, 11/16/08, Comments revised for 64-bit operation                *
*  ==>, 11/16/08, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include <stdlib.h>
#include <math.h>
#include "rkarith.h"

#define MaxJacobiIter       50
#define ThreshJacobiIter     4
#define RatioSkipIter    100.0
#define FailReturnVal       -1

/* Macro to perform rotation */
#define ROTATE(a,jp,jq) g=a[jp]; h=a[jq]; \
   a[jp]-=sinx*(h+g*tau); a[jq]+=sinx*(g-h*tau);

int jacobi(double *a, double *val, double *vec, double *dd,
      int N, int kvec, int ksort) {

   double anorm, g, h, thresh, theta, tau, t, sinx, cosx;
   double dN = (double)N;

   long ij, jp, jq, pp, pq, qq;
   int iter;
   int i, j, p, q, pn, qn;

/* Generate identity matrix to initialize eigenvectors */

   if (kvec) {
      ij = 0;
      for (i=0; i<N; i++) {
         for (j=0; j<N; j++) {
            vec[ij++] = (i == j) ? 1.0 : 0.0;
            }
         }
      } /* End if kvec */

/* Hold initial diagonal elements in val and clear dd to receive
*  incremental corrections as the calculation proceeds.  */

   ij = 0;
   for (i=0; i<N; i++) {
      val[i] = a[ij];
      dd[i] = 0.0;
      ij += i+2;
      }

/* Iterate to convergence or for MaxJacobiIter attempts */

   for (iter=0; iter<MaxJacobiIter; iter++) {

/* Compute off-diagonal matrix norm */

      anorm = 0.0;
      ij = 1;
      for (i=1; i<N; i++,ij++) {
         for (j=0; j<i; j++,ij++)
            anorm += fabs(a[ij]);
         }

/* Normal return occurs when anorm has been reduced to 0.0
*  (or when matrix is already diagonal on entry).  */

      if (anorm <= 0.0) goto FinalSort;

/* In the first three sweeps, the pq transformation is carried out
*  only if |apq| is above some threshold, to save unnecessary work.
*  After three sweeps, rotation occurs for all nonzero elements.  */

      thresh = (iter < ThreshJacobiIter) ? 0.2*anorm/(dN*dN) : 0.0;

/* Perform one iteration of the Jacobi procedure */

      pq = 1;
      for (q=1,qq=2; q<N; q=qn,pq++) {
         qn = q + 1;
         for (p=0,pp=0; p<q; p=pn,pq++) {
            pn = p + 1;
            g = RatioSkipIter * fabs(a[pq]);

            /* After ThreshJacobiIterations, skip the rotation
            *  if the off-diagonal element is small.  */
            if (iter > ThreshJacobiIter &&
               fabs(a[pp]) + g == fabs(a[pp]) &&
               fabs(a[qq]) + g == fabs(a[qq])) a[pq] = 0.0;
            else if (fabs(a[pq]) > thresh) {

               h = a[qq] - a[pp];
               if ((fabs(h) + g) == fabs(h))
                  t = a[pq]/h;
               else {
                  theta = 0.5*h/a[pq];
                  t = 1.0/(fabs(theta) + sqrt(1.0 + theta*theta));
                  if (theta < 0.0) t = -t;
                  }
               cosx = 1.0/sqrt(1.0 + t*t);
               sinx = t * cosx;
               tau = sinx/(1.0 + cosx);
               h = t*a[pq];

               dd[p] -= h;
               dd[q] += h;
               a[pp] -= h;
               a[qq] += h;
               a[pq] = 0.0;

               /* Start at tops of p,q columns */
               jp = pp - p;
               jq = qq - q;
               for (j=0; j<p; j++) {
                  ROTATE(a,jp,jq);
                  jp += 1, jq += 1;
                  }
               jp += pn, jq += 1;   /* Skip pp diagonal cell */
               for (j=pn; j<q; j++) {
                  ROTATE(a,jp,jq);
                  jp += j + 1, jq += 1;
                  }
               jp += qn, jq += qn;  /* Skip qq diagonal cell */
               for (j=qn; j<N; j++) {
                  ROTATE(a,jp,jq);
                  jp += j + 1, jq += j + 1;
                  }

               /* Rotate eigenvectors too */
               if (kvec) {
                  long pr = N*p;
                  long qr = N*q;
                  long prn = pr + N;
                  for ( ; pr<prn; pr++,qr++) {
                     ROTATE(vec,pr,qr);
                     }
                  } /* End eigenvector rotation */

               } /* End rotation */
            pp += (pn+1);
            } /* End p loop */
         qq += (qn+1);
         } /* End q loop */

/* Update diagonal elements with more accurate sums */

      ij = 0;
      for (i=0; i<N; i++) {
         a[ij] = val[i] += dd[i];
         dd[i] = 0.0;
         ij += i+2;
         }

/* If loop ends without jumping out to FinalSort,
*  the calculation failed to converge.  */

      } /* End iteration loop */

   return FailReturnVal;

/* Sort eigenvalues and eigenvectors.  By moving down from the
*  top in the j loop, much of the work of later passes will be
*  done in earlier passes, making it a little less bad than the
*  published version.  */

FinalSort:
   if (ksort) {
      double t;                  /* Temp for swapping elements */
      long ir,irn,irk,jrk;       /* Indices of vector elements */
      int Nm1 = N - 1;
      for (i=0,ir=0; i<Nm1; i++,ir=irn) {
         irn = ir + N;
         for (j=Nm1; j>i; j--) {

            if (val[i] < val[j]) {
               t = val[i];
               val[i] = val[j];
               val[j] = t;

               if (kvec) {    /* Swap eigenvectors */
                  jrk = j*N;
                  for (irk=ir; irk<irn; irk++,jrk++) {
                     t = vec[irk];
                     vec[irk] = vec[jrk];
                     vec[jrk] = t;
                     }
                  }
               } /* End swap */
            } /* End j loop */
         } /* End i loop */
      } /* End sort */

   return iter;                  /* Calculation completed OK */

   } /* End jacobi */
