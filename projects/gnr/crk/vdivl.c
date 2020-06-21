/* (c) Copyright 2009, The Rockefeller University *11115* */
/* $Id: vdivl.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                                vdivl                                 *
*                                                                      *
*  This function divides an unsigned integer, x, composed of nx ui32   *
*  'digits', by an unsigned divisor, y, composed of ny ui32 'digits'.  *
*  It returns a quotient of nx 'digits' (the number of nonzero digits  *
*  is actually lx-ly+1 where lx and ly are the numbers of nonzero      *
*  digits in x and y, respectively), and optionally a remainder of ny  *
*  'digits', or the remainder may be used to round up the quotient.    *
*  vdivl() can handle leading zeros in the arguments x and y and       *
*  generates leading zeros as necessary in the quotient and remainder. *
*                                                                      *
*  vdivl() is designed to perform long division in a 32- or 64-bit     *
*  environment to support various other crk routines such as wbcdin,   *
*  udevw, jduwb, etc. with a single invariant and thoroughly tested    *
*  piece of code.  To handle arguments of various lengths, they are    *
*  all accessed with pointers.  The only requirements are that the     *
*  arguments all be multiples of 32 bits in length, and arranged in    *
*  memory in big-endian or little-endian order as specified by the     *
*  BYTE_ORDRE compile-time definition in sysdef.h.  The caller is      *
*  responsible for dealing with signed arithmetic.                     *
*                                                                      *
*  If divisor is larger than 2^16 (2^32 if machine HAS_I64), vdivl()   *
*  performs classical long division in base 2^16 (2^32 with HAS_I64)   *
*  using notation and a renormalizing trick found at                   *
*  http://www.imsc.res/in/~kapil/crypto/notes/node4.html               *
*  which assures that the 'guess' obtained by short division is off    *
*  by not more than 2 from the correct answer digit.  The minimum      *
*  number of iterations dictated by the sizes of the arguments is      *
*  performed.                                                          *
*                                                                      *
*  Synopsis: void vdivl(ui32 *x, ui32 *y, ui32 *pq, ui32 *pr, ui32 *u, *
*     int nx, int ny, int kr)                                          *
*                                                                      *
*  Arguments:                                                          *
*     x        Pointer to the dividend, x.  x is an array of nx        *
*              ui32 variables.  If the variables are other types,      *
*              e.g. ui64, the pointer can be cast to a ui32 pointer.   *
*     y        Pointer to the divisor, y.  y is array of ny ui32       *
*              variables.                                              *
*     pq       Pointer to an array of length nx ui32 variables         *
*              where the quotient will be returned.  Can be same       *
*              array as x.                                             *
*     pr       Pointer to an array of length ny ui32 variables where   *
*              the remainder will be returned.  May be same array as   *
*              y.  May be NULL if the remainder is not needed.         *
*     u        Pointer to a work area of size at least 4*nx+2*ny+2     *
*              ui32 variables.                                         *
*     nx       Size of x in units of 32 bits.                          *
*     ny       Size of y in units of 32 bits.                          *
*     kr       TRUE if the quotient should be rounded up when the      *
*              remainder is equal to or greater than 1/2 the           *
*              divisor.  FALSE to omit rounding.                       *
*                                                                      *
*  The order in storage of all argument arrays is specified by the     *
*  compile-time parameter BYTE_ORDRE.                                  *
*                                                                      *
*  By definition, the code returns quotient = remainder = 0 if the     *
*  divisor is 0.  This eliminates the need for zero checking in the    *
*  caller, for example, if an average statistic is being calculated.   *
*  If 0 divisor is an error, the caller needs to check for it.  There  *
*  are no other possible errors.                                       *
*                                                                      *
*  Results of tests with 1,000,000 randomly generated problems         *
*  (64-bit numbers divided by 32-bit numbers) on a 2400 MHz Pentium    *
*  were as follows:     Binary division      vdivl algorithm           *
*  -g   NO_I64             0.715 sec            0.698 sec              *
*  -O2  NO_I64             0.112                0.255                  *
*  -g   HAS_I64            0.459                0.574                  *
*  _O2  HAS_I64            0.104                0.199                  *
*  So the binary algorithm, when optimized, is faster, but as          *
*  written, it cannot handle general-length problems.                  *
*  Tests in which the work area was defined as a ui64 array (to get    *
*  rid of many of the explicit type conversions in the HAS_I64 code)   *
*  gave no improvement, so the work area was left as ui32.             *
*                                                                      *
*  This routine is part of a basic 64-bit arithmetic package for which *
*  full C implementations are provided in order to get one started on  *
*  a new processor.  These routines or the combined routines actually  *
*  used in applications can be reimplemented in Assembler if greater   *
*  speed is desired.                                                   *
*----------------------------------------------------------------------*
*  V1A, 06/07/09, GNR - New program                                    *
*  ==>, 06/14/09, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "sysdef.h"
#include "rksubs.h"
#include "rkarith.h"

void vdivl(ui32 *x, ui32 *y, ui32 *pq, ui32 *pr, ui32 *u,
   int nx, int ny, int kr) {

/*---------------------------------------------------------------------*
*                         64-bit calculation                           *
*---------------------------------------------------------------------*/

#ifdef HAS_I64
   ui64 gu,tt;
   ui32 *v,*w;
   ui32 *pa,*pe;
   ui32 d,g;
   int  i,j,p,q,r;

#define BPL BITSPERUI32
#if LSIZE == 8
#define M32 4294967296UL
#else
#define M32 4294967296ULL
#endif

   /* Allocate quotient and normalized dividend work areas.
   *  The normalized divisor, u, will hold one more variable
   *  than needed so we can use indexing that starts at 1
   *  instead of 0 as in the cited article.  */
   v = u + ny + 1;
   w = v + nx + 1;

   /* Pack nonzero part of y into u in q digits to base M32
   *  in high-to-low order */
   q = 0;
#if BYTE_ORDRE > 0               /* Big-endian machine */
   pa = y; pe = pa + ny;
   for ( ; pa<pe; ++pa) if (*pa) break;
   for ( ; pa<pe; ++pa) u[++q] = *pa;
#else                            /* Little-endian machine */
   pa = y; pe = pa + ny - 1;
   for ( ; pe>=pa; --pe) if (*pe) break;
   for ( ; pe>=pa; --pe) u[++q] = *pe;
#endif
   /* If y is exactly 0, by definition return q = r = 0.  */
   if (q == 0) goto ReturnZero;

   /* Pack x same way into p digits in v */
   p = 0;
#if BYTE_ORDRE > 0               /* Big-endian machine */
   pa = x; pe = pa + nx;
   for ( ; pa<pe; ++pa) if (*pa) break;
   for ( ; pa<pe; ++pa) v[++p] = *pa;
#else                            /* Little-endian machine */
   pa = x; pe = pa + nx - 1;
   for ( ; pe>=pa; --pe) if (*pe) break;
   for ( ; pe>=pa; --pe) v[++p] = *pe;
#endif
   /* If x is exactly 0, by definition return q = r = 0.  */
   if (p == 0) goto ReturnZero;

   /* Number of digits in quotient (less 1) */
   r = p - q;

   /* If divisor y is <= 2^32, short division can be used
   *  to get the answer much more quickly.  */
   if (q == 1) {
      for (tt=j=0; j<p; ++j) {
         tt = (tt << BPL) | v[j+1];
         w[j] = tt/u[1], tt %= u[1];
         }
      v[p] = (ui32)tt;
      d = 1;         /* Just to eliminate warning */
      }

   /* Otherwise, must perform long division */
   else {
      /* Determine normalizing multiplier */
      if (u[1] > SI32_MAX) {
         d = 1;
         v[0] = 0;
         }
      else {
         d = M32/(u[1]+1);
         for (tt=0,i=q; i>0; --i) {    /* u *= d */
            tt = (tt>>BPL) + (ui64)d*(ui64)u[i];
            u[i] = tt & UI32_MAX; }
         for (tt=0,i=p; i>0; --i) {    /* v *= d */
            tt = (tt>>BPL) + (ui64)d*(ui64)v[i];
            v[i] = tt & UI32_MAX; }
         v[0] = (tt>>BPL);
         }

      /* Perform long division */
      for (j=0; j<=r; ++j) {
         /* Obtain guess */
         gu = (ui64)v[j]<<BPL | (ui64)v[j+1];
         g = (ui32)(gu/u[1]);
         /* Multiply guess by full divisor
         *  and subtract from dividend */
         for (gu=tt=0,i=q; i>0; --i) {
            gu = (gu>>BPL) + (ui64)g*(ui64)u[i];
            tt = (ui64)v[j+i] - (gu & UI32_MAX) - tt;
            v[j+i] = (ui32)(tt & UI32_MAX);
            /* Detect borrow w/o an 'if' */
            tt = (((tt>>BPL) + 1) >> BPL);
            }
         tt = (ui64)v[j] - (gu>>BPL) - tt;
         /* Quotient digit is g, g-1, or g-2 */
         if ((tt>>BPL) == 0) {
            w[j] = g;      /* Didn't borrow */
            continue;
            }
         for (tt=0,i=q; i>0; --i) {
            tt = (tt>>BPL) + (ui64)v[j+i] + (ui64)u[i];
            v[j+i] = (ui32)(tt & UI32_MAX);
            }
         if (tt>>BPL) {
            w[j] = g - 1;
            continue;
            }
         for (tt=0,i=q; i>0; --i) {
            tt = (tt>>BPL) + (ui64)v[j+i] + (ui64)u[i];
            v[j+i] = (ui32)(tt & UI32_MAX);
            }
         w[j] = g - 2;
         } /* End r+1 cycles of division */
      } /* End long division method */

   /* If rounding is requested, now is the time to do it */
   if (kr) {
      for (tt=0,i=1; i<=q; ++i) {
         tt |= (ui64)u[i] >> 1;
         if ((ui64)v[r+i] > tt) goto DoRounding;
         if ((ui64)v[r+i] < tt) goto NoRounding;
         tt = (ui64)(u[i] & 1) << (BPL-1);
         }
      /* All equal, must test low-order divisor bit */
      if (tt) goto NoRounding;
DoRounding:
      for (tt=0,i=r; i>=0; --i) {
         gu = (ui64)w[i] + tt + 1;
         if (gu < M32) { w[i] = (ui32)gu; break; }
         w[i] = 0, tt = 1;
         }
NoRounding: ;
      } /* End rounding */

   /* Recombine the quotient into pq array */
   i = nx - j;                /* No. of fill-in chunks */
#if BYTE_ORDRE > 0               /* Big-endian machine */
   pa = pq + nx;
   while ( j ) *--pa = w[--j];
   while (i--) *--pa = 0;
#else                            /* Little-endian machine */
   pa = pq;
   while ( j ) *pa++ = w[--j];
   while (i--) *pa++ = 0;
#endif

   /* If the remainder is wanted, denormalize
   *  and recombine into pr array */
   if (pr) {
      i = ny - q;             /* No. of fill-in chunks */
#if BYTE_ORDRE > 0               /* Big-endian machine */
      pa = pr;
      while (i--) *pa++ = 0;
      if (q == 1) *pa = v[p];    /* No denormalizing needed */
      else for (tt=0,i=r+1; i<=p; ++i) {
         tt = (tt << BPL) | (ui64)v[i];
         *pa++ = tt/d, tt %= d;
         }
#else                            /* Little-endian machine */
      pa = pr + ny;
      while (i--) *--pa = 0;
      if (q == 1) *--pa = v[p];  /* No denormalizing needed */
      else for (tt=0,i=r+1; i<=p; ++i) {
         tt = (tt << BPL) | (ui64)v[i];
         *--pa = tt/d, tt %= d;
         }
#endif
      }

/*---------------------------------------------------------------------*
*                         32-bit calculation                           *
*---------------------------------------------------------------------*/

#else
   ui32 *v,*w;
   ui16 *pa,*pe;
   ui32 d,g,gu,tt;
   int  i,j,p,q,r;
   int  nx2 = 2*nx;
   int  ny2 = 2*ny;

#define BPS BITSPERSHORT
#define M16 (UI16_MAX+1)

   /* Allocate quotient and normalized dividend work areas.
   *  The normalized divisor, u, will hold one more variable
   *  than needed so we can use indexing that starts at 1
   *  instead of 0 as in the cited article.  */
   v = u + ny2 + 1;
   w = v + nx2 + 1;

   /* Pack nonzero part of y into u in q digits to base M16
   *  in high-to-low order */
   q = 0;
#if BYTE_ORDRE > 0               /* Big-endian machine */
   pa = (ui16 *)y; pe = pa + ny2;
   for ( ; pa<pe; ++pa) if (*pa) break;
   for ( ; pa<pe; ++pa) u[++q] = *pa;
#else                            /* Little-endian machine */
   pa = (ui16 *)y; pe = pa + ny2 - 1;
   for ( ; pe>=pa; --pe) if (*pe) break;
   for ( ; pe>=pa; --pe) u[++q] = *pe;
#endif
   /* If y is exactly 0, by definition return q = r = 0.  */
   if (q == 0) goto ReturnZero;

   /* Pack x same way into p digits in v */
   p = 0;
#if BYTE_ORDRE > 0               /* Big-endian machine */
   pa = (ui16 *)x; pe = pa + nx2;
   for ( ; pa<pe; ++pa) if (*pa) break;
   for ( ; pa<pe; ++pa) v[++p] = *pa;
#else                            /* Little-endian machine */
   pa = (ui16 *)x; pe = pa + nx2 - 1;
   for ( ; pe>=pa; --pe) if (*pe) break;
   for ( ; pe>=pa; --pe) v[++p] = *pe;
#endif
   /* If x is exactly 0, by definition return q = r = 0.  */
   if (p == 0) goto ReturnZero;

   /* Number of digits in quotient (less 1) */
   r = p - q;

   /* If divisor y is <= 2^16, short division can be used
   *  to get the answer much more quickly.  */
   if (q == 1) {
      for (tt=j=0; j<p; ++j) {
         tt = (tt << BPS) | v[j+1];
         w[j] = tt/u[1], tt %= u[1];
         }
      v[p] = tt;
      d = 1;         /* Just to eliminate warning */
      }

   /* Otherwise, must perform long division */
   else {
      /* Determine normalizing multiplier */
      if (u[1] > SHRT_MAX) {
         d = 1;
         v[0] = 0;
         }
      else {
         d = M16/(u[1]+1);
         for (tt=0,i=q; i>0; --i)      /* u *= d */
            tt = (tt>>BPS) + d*u[i], u[i] = tt & UI16_MAX;
         for (tt=0,i=p; i>0; --i)      /* v *= d */
            tt = (tt>>BPS) + d*v[i], v[i] = tt & UI16_MAX;
         v[0] = (tt>>BPS);
         }

      /* Perform long division */
      for (j=0; j<=r; ++j) {
         /* Obtain guess */
         g = (v[j]<<BPS|v[j+1])/u[1];
         /* Multiply guess by full divisor
         *  and subtract from dividend */
         for (gu=tt=0,i=q; i>0; --i) {
            gu = (gu>>BPS) + g*u[i];
            tt = v[j+i] - (gu & UI16_MAX) - tt;
            v[j+i] = tt & UI16_MAX;
            /* Detect borrow w/o an 'if' */
            tt = (((tt>>BPS) + 1) >> BPS);
            }
         tt = v[j] - (gu>>BPS) - tt;
         /* Quotient digit is g, g-1, or g-2 */
         if ((tt>>BPS) == 0) {
            w[j] = g;      /* Didn't borrow */
            continue;
            }
         for (tt=0,i=q; i>0; --i) {
            tt = (tt>>BPS) + v[j+i] + u[i];
            v[j+i] = tt & UI16_MAX;
            }
         if (tt>>BPS) {
            w[j] = g - 1;
            continue;
            }
         for (tt=0,i=q; i>0; --i) {
            tt = (tt>>BPS) + v[j+i] + u[i];
            v[j+i] = tt & UI16_MAX;
            }
         w[j] = g - 2;
         } /* End r+1 cycles of division */
      } /* End long division method */

   /* If rounding is requested, now is the time to do it */
   if (kr) {
      for (tt=0,i=1; i<=q; ++i) {
         tt |= u[i] >> 1;
         if (v[r+i] > tt) goto DoRounding;
         if (v[r+i] < tt) goto NoRounding;
         tt = (u[i] & 1) << (BPS-1);
         }
      /* All equal, must test low-order divisor bit */
      if (tt) goto NoRounding;
DoRounding:
      for (tt=0,i=r; i>=0; --i) {
         if ((w[i] += tt + 1) < M16) break;
         w[i] = 0, tt = 1;
         }
NoRounding: ;
      } /* End rounding */

   /* Recombine the quotient into pq array */
   i = nx2 - j;               /* No. of fill-in chunks */
#if BYTE_ORDRE > 0               /* Big-endian machine */
   pa = (ui16 *)pq + nx2;
   while ( j ) *--pa = (ui16)w[--j];
   while (i--) *--pa = 0;
#else                            /* Little-endian machine */
   pa = (ui16 *)pq;
   while ( j ) *pa++ = (ui16)w[--j];
   while (i--) *pa++ = 0;
#endif

   /* If the remainder is wanted, denormalize
   *  and recombine into pr array */
   if (pr) {
      i = ny2 - q;            /* No. of fill-in chunks */
#if BYTE_ORDRE > 0               /* Big-endian machine */
      pa = (ui16 *)pr;
      while (i--) *pa++ = 0;
      if (q == 1) *pa = v[p];    /* No denormalizing needed */
      else for (tt=0,i=r+1; i<=p; ++i) {
         tt = (tt << BPS) | v[i];
         *pa++ = tt/d, tt %= d;
         }
#else                            /* Little-endian machine */
      pa = (ui16 *)pr + ny2;
      while (i--) *--pa = 0;
      if (q == 1) *--pa = v[p];  /* No denormalizing needed */
      else for (tt=0,i=r+1; i<=p; ++i) {
         tt = (tt << BPS) | v[i];
         *--pa = tt/d, tt %= d;
         }
#endif
      }
#endif   /* !HAS_I64 */
   return;

ReturnZero:
   for (i=0; i<nx; ++i) pq[i] = 0;
   if (pr) for (i=0; i<ny; ++i) pr[i] = 0;
   return;

   } /* End vdivl() */
