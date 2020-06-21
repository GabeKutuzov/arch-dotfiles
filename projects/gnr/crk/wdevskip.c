/* (c) Copyright 2010-2013, The Rockefeller University *11115* */
/* $Id: wdevskip.c 15 2009-12-30 22:02:21Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                              wdevskip                                *
*                                                                      *
*  Skip n random numbers in a wdev sequence                            *
*                                                                      *
*  Usage: void wdevskip(wseed *seed, long n)                           *
*      seed  = random number seed to be updated                        *
*      n     = number of random deviates to skip over (deliberately    *
*              a long so can have 2**31-1 skips in a 32-bit system,    *
*              2**48-1 (easily expanded) in a 64-bit system).          *
*                                                                      *
*  Note: Thanks to Joe Brandenberg of Intel Scientific Computers       *
*     for the idea used in udevskip and expanded here for two seeds.   *
*                                                                      *
*     The formula for the k'th number that would be returned by a      *
*     a linear congruential generator without additive constant is     *
*        x(k) = (A**k)*x (mod M) = (A**k (mod M))*x (mod M)            *
*     where, for the 31-bit component,                                 *
*        A = 7**5 = 16807 and M = 2**31 - 1 = 2147483647.              *
*     With the additive constant c, we have                            *
*        y(k) = {(A**k)*y + c*Sum(j=0 to k-1)(A**j)} (mod M)           *
*     where, for the 27-bit component,                                 *
*        A = 4971028, c = 28363543, and M = 2**27 + 1 = 134217729.     *
*     The geometric series formula cannot be used for this sum because *
*     it requires modulo division, but the sum can be done recursively *
*     by Sum(to 2**(k-1)) = (A**(2**k) + 1)*Sum(to 2**(k-2)).  (See    *
*     Knuth's chapter on random number generators if this is not       *
*     obvious.)  These are expressed as x(k) = P(k)*x (31-bit) and     *
*     y(k) = S(k)*x + T(k) (27-bit).  The values of P, S and T are     *
*     tabulated for values of k that are exact powers of two.  For     *
*     NO_I64 machines, P and S are given as high-and low-order 16-bit  *
*     multipliers.  The following code then obtains x(k) and y(k) by   *
*     repeated application of the above formulae for each tabulated    *
*     value that corresponds to a one in the binary representation of  *
*     k.  This method runs in time proportional to log(2)(k), and      *
*     represents a compromise between speed and the space that would   *
*     be required to store larger tables of P, S, and T.  The tabu-    *
*     lated values were calculated using the 'dc' calculator, which    *
*     did the calculations with explicit multiprecision arithmetic.    *
*                                                                      *
************************************************************************
*  V1A, 01/16/10, GNR - New program                                    *
*  ==>, 01/16/10, GNR - Last date before committing to svn repository  *
*  Rev, 10/24/13, GNR - SRA() for signed '>>'                          *
***********************************************************************/

#include "rkarith.h"    /* (brings in sysdef.h) */

#define WDEV_JDEF
#include "wdevcom.h"

#if LSIZE == 8
/* This is arbitrarily set to 2**48 because it seems unlikely
*  more will ever be needed--if necessary to increase, be sure
*  to add the new elements to the three tables below.  */
#define MXSKIP 281474976710655
#define SZSKIP 48
#else
#define MXSKIP 2147483647
#define SZSKIP 32
#endif

void wdevskip(wseed *seed, long n) {

#ifdef HAS_I64
   si64 ws31,ws27;               /* Working copies of seeds */
   static si32 P[SZSKIP] = {
              16807,  282475249,  984943658, 1457850878,
         1137522503, 1636807826,  685118024,  515204530,
          897054849, 2038299453, 1836275591,  349037107,
          149796865, 1186652285, 2106880871,  877809922,
         1682791109, 1900685356, 2080563572,  612544882,
         1295048709, 1987420232,  868966365, 1331238991,
         1550655590,  766698560, 1154667137,  901595110,
         1008653149, 1821072732, 2147466840,  282475249,
#if LSIZE == 8
          984943658, 1457850878, 1137522503, 1636807826,
          685118024,  515204530,  897054849, 2038299453,
         1836275591,  349037107,  149796865, 1186652285,
         2106880871,  877809922, 1682791109, 1900685356,
#endif
      };
   static si32 S[SZSKIP] = {
            4971028,   24855136,   19884109,    9942055,
           79536433,   84507460,   49710271,  114333622,
           64623352,   99420541,  124275676,   39768217,
           94449514,   69594379,  109362595,   54681298,
           34797190,  129246703,    4971028,   24855136,
           19884109,    9942055,   79536433,   84507460,
           49710271,  114333622,   64623352,   99420541,
          124275676,   39768217,   94449514,   69594379,
#if LSIZE == 8
          109362595,   54681298,   34797190,  129246703,
            4971028,   24855136,   19884109,    9942055,
           79536433,   84507460,   49710271,  114333622,
           64623352,   99420541,  124275676,   39768217,
#endif
      };
#else
#define LO    0
#define HI    1
#define nLOHI 2
   si32 ws31,ws27;
   ui32 a,b,c,d,bd,xx,xh,smlo,smhi;
   /* For NO_I64 machines, provide 16-low and 16-high bits */
   static ui16 PP[SZSKIP][nLOHI] = {
      16807,     0, 15089,  4310,  3114, 15029,  2558, 22245,
      14151, 17357, 46226, 24975,  4680, 10454, 26034,  7861,
      63617, 13687, 64317, 31101, 22407, 28019, 57907,  5325,
      47105,  2285, 57469, 18106, 29543, 32148, 20738, 13394,
      23237, 25677, 10284, 29002, 57716, 31746, 45426,  9346,
      57349, 19760, 41032, 30325, 24541, 13259,  6223, 20313,
       8294, 23661, 58432, 11698, 53889, 17618, 16358, 13757,
      54109, 15390, 23900, 27787, 48728, 32767, 15089,  4310,
      };
   static ui16 SS[SZSKIP][nLOHI] = {
      55828,    75, 16992,   379, 26701,   303, 46119,   151,
      41265,  1213, 31556,  1289, 33983,   758, 38838,  1744,
       4856,   986,  2429,  1517, 19420,  1896, 53401,   606,
      12138,  1441, 60683,  1061, 48547,  1668, 24274,   834,
      63110,   530,  9711,  1972, 55828,    75, 16992,   379,
      26701,   303, 46119,   151, 41265,  1213, 31556,  1289,
      33983,   758, 38838,  1744,  4856,   986,  2429,  1517,
      19420,  1896, 53401,   606, 12138,  1441, 60683,  1061,
      };
#endif
   static si32 T[SZSKIP] = {
           28363543,    2045789,  113454172,   23096237,
           95902744,   17819543,   70436275,   41452010,
           13309642,   46503392,   97977811,  111448163,
           78736543,   13313303,   46510714,   68166293,
           96564370,   63882038,   73082779,   91484261,
           23895658,  112414667,    6104146,  106657805,
          113895070,  128369600,   52927093,  125738294,
          122229886,   25734584,   41527114,   73112174,
#if LSIZE == 8
           31890727,   38926319,   38084422,   81139871,
          107598445,   26297864,   27740593,  120104537,
           21483886,    3199556,   41196301,  117189791,
           30567475,   81019058,   32791414,  115293098,
#endif
      };
   int  k;                       /* Power of two being applied */

   if (n == 0) return;           /* Quick exit */

#ifdef HAS_I64
   if (n < 0 || n > MXSKIP)     /* Arg check */
      abexit(70);
   ws31 = (si64)seed->seed31;
   ws27 = (si64)seed->seed27;
#else
   if (n < 0) abexit(70);        /* Arg check */
   ws31 = (si32)seed->seed31;
   ws27 = (si32)seed->seed27;    /* Must be signed for r - q */
#endif
   k = 0;
   while (n) {
      if (n & 1) {

#ifdef HAS_I64                   /* --- 64-bit version --- */
         /* Update the 31-bit seed */
         ws31 *= (si64)P[k];
         ws31 = (ws31 & w31) + SRA(ws31,31); /* r + q */
         ws31 = (ws31 & w31) + SRA(ws31,31); /* Correct if r+q > w */
         /* Update the 27-bit seed (if not negative) */
         if (ws27 >= 0) {
            ws27 = ws27 * (si64)S[k] + T[k];
            ws27 = (ws27 & w27) - SRA(ws27,27);
            ws27 = (ws27 & w27) - SRA(ws27,27);
            }
#else                            /* --- 32-bit version --- */
         /* Update the 31-bit seed */
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
         a = (ui32)ws31 >> 16;
         b = (ui32)ws31 & 0x0000ffff;
         c = (ui32)PP[k][HI];
         d = (ui32)PP[k][LO];
         xx = b * c + a * d;
         xh = xx << 16;
         /* Break up low-order sum for carry check */
         bd = b * d;
         smlo = bd + xh;
         smhi = (a * c) + (xx >> 16) + (~bd < xh);
         /* Form remainder modulo (2**31-1) using Knuth trick */
         smlo = (smlo & w31) + (smlo >> 31) + (smhi << 1);
         /* Final adjustment--if the result exceeds (2**31-1),
         *  subtract (2**31-1) one more time.  This boils down to:
         *  if the sign bit is set, clear the sign and add one.  */
         ws31 = (si32)((smlo + (smlo >> 31)) & w31);
         /* Update the 27-bit seed (if not negative)--same method as
         *  for ws31 except with additive constant.  Note that with
         *  S[k] as multiplier, rather than mlo when doing a single
         *  update, the possibility now exists of a carry into smhi
         *  when adding T[k].  */
         if (ws27 >= 0) {
            a = (ui32)ws27 >> 16;
            b = (ui32)ws27 & 0x0000ffff;
            c = (ui32)SS[k][HI];
            d = (ui32)SS[k][LO];
            xx = b * c + a * d;
            xh = xx << 16;
            bd = b * d;
            smlo = bd + xh;
            smhi = (a * c) + (xx >> 16) + (~bd < xh) + (~smlo < T[k]);
            smlo += T[k];
            /* Subtract the quotient from the remainder mod (2**27+1) */
            ws27 = (si32)(smlo & w27) - (si32)(smhi << 5 | smlo >> 27);
            ws27 = (ws27 & w27) - SRA(ws27, 27);
            }
#endif

         } /* End if (n & 1) */
      k++;                       /* To next power of 2 */
      n >>= 1;
      } /* End while (n) */

   seed->seed31 = (si32)ws31;    /* Store result back to caller */
   seed->seed27 = (si32)ws27;

   } /* End wdevskip() */
