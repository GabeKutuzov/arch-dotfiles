/* (c) Copyright 2002-2008, The Rockefeller University *11115* */
/* $Id: dsinv.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                           Function dsinv                             *
*                                                                      *
*  Purpose:                                                            *
*     Invert a given symmetric positive definite matrix                *
*                                                                      *
*  Usage:                                                              *
*     int dsinv(double *a, long N, float eps)                          *
*                                                                      *
*  Description of parameters:                                          *
*     a    - Ptr to upper triangular part of given symmetric positive  *
*            definite N by N coefficient matrix.  On return 'a'        *
*            contains resultant upper triangular inverse matrix.       *
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
*                                                                      *
*  Other Functions required:                                           *
*     dmfsd                                                            *
*                                                                      *
*  Method:                                                             *
*     Solution is done using factorization by subroutine dmfsd.        *
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

int dsinv(double *a, long N, float eps) {

   double din,work;
   long ipiv,ind,min,kend,I,J,K,L,lhor,lver,lanf;
   int ier;

/* Factorize given matrix by means of function dmfsd:
*  If result is T, then A = Transpose(T) * T */

   if ((ier = dmfsd(a, N, eps)) < 0) return ier;

/* Invert upper triangular matrix T */

   /* Prepare inversion loop */
   ipiv = (N*(N+1))>>1;
   ind = ipiv;

   /* Perform inversion loop */
   for (I = 1; I <= N; I++) {
      din = 1./a[ipiv-1];
      a[ipiv-1] = din;
      min = N;
      kend = I-1;
      lanf = N-kend;
      if (kend > 0) {
         J = ind;

         /* Perform row loop */
         for (K = 1; K <= kend; K++) {
            work = 0.;
            min--;
            lhor = ipiv;
            lver = J;

            /* Perform inner loop */
            for (L = lanf; L <= min; L++) {
               lver++;
               lhor += L;
               work += a[lver-1]*a[lhor-1];
               } /* End inner loop */

            a[J-1] = -work*din;
            J -= min;
            } /* End row loop */

         } /* End if */
      ipiv -= min;
      ind--;
      } /* End inversion loop */

/* Calculate Inverse(A) by means of Inverse(T):
*  Inverse(A) = Inverse(T) * Transpose(Inverse(T)) */

   /* Perform multiplication loop */
   for (I = 1; I <= N; I++) {
      ipiv += I;
      J = ipiv;

      /* Perform row loop */
      for (K = I; K <= N; K++) {
         work = 0.;
         lhor = J;

         /* Perform inner loop */
         for (L = K; L <= N; L++) {
            lver = lhor+K-I;
            work += a[lhor-1]*a[lver-1];
            lhor += L;
            } /* End inner loop */

         a[J-1] = work;
         J += K;
         } /* End row loop */

      } /* End multiplication loop */

   return 0;
   } /* End dsinv */
