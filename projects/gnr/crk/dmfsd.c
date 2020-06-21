/* (c) Copyright 2002-2008, The Rockefeller University *11115* */
/* $Id: dmfsd.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                           Function dmfsd                             *
*                                                                      *
*  Purpose:                                                            *
*     Factor a given symmetric positive definite matrix                *
*                                                                      *
*  Usage:                                                              *
*     int dmfsd(double *, long N, float eps)                           *
*                                                                      *
*  Description of parameters:                                          *
*     a    - Ptr to upper triangular part of given symmetric positive  *
*            definite N by N coefficient matrix.  On return 'a'        *
*            contains resultant factored upper triangular matrix.      *
*     N    - The number of rows (columns) in the given matrix.         *
*     eps  - Relative tolerance for test on loss of significance       *
*            (float to avoid stack hole when long is 32 bits).         *
*  The function returns an integer value:                              *
*     ier  - Error parameter coded as follows                          *
*            ier = 0   - No error                                      *
*            ier = RK_POSDERR (-1)  - No result because of negative    *
*                    input parameter N or because some radicand is     *
*                    non-positive (matrix a is not positive definite,  *
*                    possibly due to loss of significance).            *
*            ier = K > 0  - Warning indicating loss of significance.   *
*                    The radicand formed at factorization step K+1     *
*                    was still positive but no longer greater than     *
*                    abs(eps*a[k+1][k+1]).                             *
*                                                                      *
*  Remarks:                                                            *
*     The upper triangular part of the given matrix is assumed to be   *
*     stored columnwise in N*(N+1)/2 successive storage locations.     *
*     The resulting upper triangular matrix is stored columnwise       *
*     in the same locations.  The procedure gives results if N is      *
*     greater than 0 and all calculated radicands are positive.        *
*     The product of the returned diagonal terms is equal to the       *
*     square root of the determinant of the given matrix.              *
*                                                                      *
*  Other Functions required:                                           *
*     None.                                                            *
*                                                                      *
*  Method:                                                             *
*     Solution is done using the square-root method of Cholesky.       *
*     The given matrix is represented as product of two triangular     *
*     matrices, where the left hand factor is the transpose of         *
*     the returned right hand factor.                                  *
*                                                                      *
************************************************************************
*  Date of original version not recorded, GNR                          *
*  ==>, 07/01/02, GNR - Last date before committing to svn repository  *
*  Rev, 12/22/08, GNR - Revise for 64-bit N                            *
***********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <math.h>
#include "rkarith.h"

int dmfsd(double *a, long N, float eps) {

   double dpiv,dsum,tol;
   long K,L,I,kpiv,ind,lend,lanf,lind;
   int ier;

/* Test for invalid (negative) input parameter N */

   if (N <= 0) return RK_POSDERR;

/* Perform diagonal loop */

   ier = 0;
   kpiv = 0;
   for (K = 1; K <= N; K++) {
      kpiv += K;
      ind = kpiv;
      lend = K-1;

/* Calculate tolerance */

      tol = fabs((double)eps*a[kpiv-1]);

/* Perform factorization loop over K-th row */

      for (I = K; I <= N; I++) {
         dsum = 0.;
         if (lend != 0) {

/* Perform inner loop */

            for (L = 1; L <= lend; L++) {
               lanf = kpiv-L;
               lind = ind-L;
               dsum += a[lanf-1]*a[lind-1];
               } /* End inner loop */
            } /* End if */

/* Transform element a[ind] */

         dsum = a[ind-1] - dsum;
         if (I-K) a[ind-1] = dsum*dpiv;
         else {

/* Test for negative pivot element and for loss of significance */

            if ((dsum - tol) <= 0.) {
               if (dsum <= 0.) return RK_POSDERR;
               if (ier <= 0) ier = K-1;
               }

/* Compute pivot element */

            dpiv = sqrt(dsum);
            a[kpiv-1] = dpiv;
            dpiv = 1./dpiv;

/* Calculate terms in row */

            } /* End else */
         ind += I;
         } /* End diagonal loop */
      }

   return ier;
   } /* End dmfsd */
