/* (c) Copyright 1989-2013, The Rockefeller University *11115* */
/* $Id: udevskip.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                              udevskip                                *
*                                                                      *
*  Skip n random numbers in a udev sequence                            *
*                                                                      *
*  Usage: void udevskip(si32 *seed, si32 n)                            *
*      seed  = random number seed to be updated                        *
*      n     = number of random deviates to skip over                  *
*                                                                      *
*  Note: Thanks to Joe Brandenberg of Intel Scientific Computers       *
*     for the idea used in this routine.  The formula for the k'th     *
*     number that would be returned by the 'udev' function is          *
*        x(k) = (P**k)*x (mod M) = (P**k (mod M))*x (mod M)            *
*     where P = 7**5 = 16807 and M = 2**31 - 1 = 2147483647            *
*     (See Knuth's chapter on random number generators if this is      *
*     not obvious.)  This is expressed as x(k) = P(k)*x and the        *
*     values of P are tabulated for values of k that are exact pow-    *
*     ers of two.  The following code then obtains x(k) by repeated    *
*     application of the above formula for each tabulated value        *
*     that corresponds to a one in the binary representation of k.     *
*     This method runs in time proportional to log(2)(k), and repre-   *
*     sents a compromise between speed and the space that would be     *
*     required to store larger tables of P values.  The tabulated      *
*     values of P were calculated using the REXX program 'udevtabl     *
*     exec', which did the calculation with explicit multiprecision    *
*     arithmetic.                                                      *
*                                                                      *
************************************************************************
*  V1A, 07/24/89, GNR - New program                                    *
*  ==>, 03/09/06, GNR - Last date before committing to svn repository  *
*  Rev, 08/09/09, GNR - Add quick return if n == 0                     *
*  Rev, 01/23/10, GNR - Explicit arithmetic instead of calls           *
*  Rev, 06/09/13, GNR - Deal with return from abexit in tests          *
*  Rev, 10/24/13, GNR - SRA() for signed '>>'                          *
***********************************************************************/

#include "rkarith.h"
#if defined(HAS_I64) && LSIZE == 4
#define w31 0x7fffffffLL         /* Base mask for seed31  */
#else
#define w31 0x7fffffffL
#endif

void udevskip(si32 *seed, si32 n) {

#ifdef HAS_I64
   si64 ws31;                    /* Working copy of seed */
#else
   ui32 ws31,a,b,c,d,bd,xx,xh,smlo,smhi;
#endif
   register int k;               /* Power of two being applied */

   static ui32 P[32] = {
              16807,  282475249,  984943658, 1457850878,
         1137522503, 1636807826,  685118024,  515204530,
          897054849, 2038299453, 1836275591,  349037107,
          149796865, 1186652285, 2106880871,  877809922,
         1682791109, 1900685356, 2080563572,  612544882,
         1295048709, 1987420232,  868966365, 1331238991,
         1550655590,  766698560, 1154667137,  901595110,
         1008653149, 1821072732, 2147466840,  282475249 };

   if (n == 0) return;                    /* Quick exit */
   if (n < 0) { abexitq(70); return; }    /* Arg check */

#ifdef HAS_I64
   ws31 = (si64)(*seed);
#else
   ws31 = (ui32)(*seed);
#endif
   k = 0;
   while (n) {
      if (n & 1) {
#ifdef HAS_I64
         ws31 *= (si64)P[k];
         ws31 = (ws31 & w31) + SRA(ws31,31);
         ws31 = (ws31 & w31) + SRA(ws31,31);
#else
         /* Multiplication of two 32-bit numbers with 32-bit arith-
         *  metic requires splitting into four 16 x 16-bit products,
         *  Prod = (A*(2**16) + B) * (C*(2**16) + D).
         *  High 32 bits of result =
         *        A*C + <high order 16 bits of (A*D + B*C)>
         *  Low 32 bits of result =
         *        B*D + <low order 16 bits of (A*D + B*C)>
         *  This code is taken from jmsw(), because unlike the case
         *  with jmuw(), (A*D + B*C) cannot overflow beyond 32 bits.
         *  Avoid call overhead and omit setting the final sign,
         *  because all the operands are known to be positive.  */
         a = ws31 >> 16;
         b = ws31 & 0x0000ffffUL;
         c = (ui32)P[k] >> 16;
         d = (ui32)P[k] & 0x0000ffffUL;
         xx = b * c + a * d;
         xh = xx << 16;
         /* Break up low-order sum for carry check */
         bd = b * d;
         smlo = bd + xh;
         smhi = (a * c) + (xx >> 16) + (~bd < xh);
         /* Form remainder modulo (2**31-1) using Knuth trick */
         ws31 = (smlo & w31) + (smlo >> 31) + (smhi << 1);
         /* Final adjustment--if the result exceeds (2**31-1),
         *  subtract (2**31-1) one more time.  This boils down to:
         *  if the sign bit is set, clear the sign and add one.  */
         ws31 = (ws31 + (ws31 >> 31)) & w31;
#endif
         }
      k++; n >>= 1; }
   *seed = (si32)ws31;           /* Store result back to caller */
   } /* End udevskip() */
