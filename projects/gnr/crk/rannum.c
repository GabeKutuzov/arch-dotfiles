/* (c) Copyright 1989-2009, The Rockefeller University *11115* */
/* $Id: rannum.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                               RANNUM                                 *
*                                                                      *
*     RANNUM - Generate uniformly distributed pseudo-random numbers    *
*              in the interval (1,2**(31-bits)-1)                      *
*                                                                      *
************************************************************************
*  V1A, 02/27/89, G. N. REEKE                                          *
*  Rev, 04/19/89, GNR - Incorporate rkarith.h                          *
*  V1B, 06/22/89, GNR - Use udev instead of dm64nb if bits >= 0        *
*  ==>, 07/01/02, GNR - Last date before committing to svn repository  *
*  V1C, 11/16/09, GNR - Modify for 64-bit compilations, calc w/o call  *
*                       to udev, better algorithm if bits = -1         *
***********************************************************************/

/*
      Usage: void rannum(si32 *ir, int n, si32 *seed, int bits)

      Arguments:
         ir    = output array to contain n si32 integers.
         n     = number of values to be generated.
         seed  = any integer from 1 to 2**31-1 (it is replaced
                  on return by a new value for the next call).
         bits  = 31 - number of bits required in output values,
                  i.e. is RIGHT shift to apply to each random
                  number.  (This definition is backwards from
                  the usual convention, but is for compatibility
                  with the FORTRAN version.)

      Algorithm:  ir[i] = ir[i-1]*(7**5) MODULO (2**31-1)
         Result is right-shifted 'bits' bits.  'bits' < 0 is
         allowed and produces a signed result, using the low-
         order quotient bit as the sign of the result.  This
         algorithm is far from the most convenient to implement
         in C, but is used for compatibility with the FORTRAN
         implementation.

      Notes:  Since rannum may be very heavily used, one may want
         to implement the whole thing in Assembler.

      Errors:  The application is terminated with error code 070
         if n < 1.
*/

#include "sysdef.h"
#include "rkarith.h"

void rannum(si32 *ir, int n, si32 *seed, int bits) {

   ui32 q,r = (ui32)(*seed);     /* Quotient and remainder */
   int i;                        /* Counter */

   /* Check for invalid parameters */
   if (n < 1) abexit(71);

   /* Calculate n values */
   for (i=0; i<n; i++) {
#ifdef HAS_I64                   /* --- 64-bit version --- */
#if LSIZE == 8
#define m 16807L                 /* The magic multiplier */
#define w31 0x7fffffffL          /* Base, 2**31 - 1 */
#else
#define m 16807LL
#define w31 0x7fffffffLL
#endif

      /* Calculate the product and remainder with 64-bit arithmetic */
      ui64 sm = (ui64)r * m;
      q = (ui32)(sm >> 31);
      /* Form remainder modulo (2**31-1) using Knuth trick */
      r = (ui32)(sm & w31) + q;
#else                            /* --- 32-bit version --- */
#define m 16807L                 /* The magic multiplier */
#define w31 0x7fffffffL          /* Base, 2^31 */
      /* Form the product in low 16 and high 31 bit pieces */
      ui32 s0m = (r & 0xffffL) * m;
      ui32 s1m = (r >> 16) * m + (s0m >> 16);
      q = s1m >> 15;
      /* Form remainder modulo (2**31-1) using Knuth trick */
      r = (s0m & 0xffffL) + ((s1m & 0x7fffL) << 16) + q;
#endif
      /* If the remainder exceeds (2**31-1), add 1 to quotient
      *  and subtract (2**31-1) from remainder.  */
      if (r > w31)
         q += 1, r -= w31;
      /* Store shifted or signed result per bits arg */
      if (bits >= 0) *ir++ = r >> bits;
      else           *ir++ = (q & 1) ? -r : r;
      } /* End loop over n values */

   /* Store final seed back to caller */
   *seed = (si32)r;

   } /* End rannum() */
