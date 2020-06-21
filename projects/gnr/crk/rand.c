/* (c) Copyright 1988-2008, The Rockefeller University *11115* */
/* $Id: rand.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                             rand, srand                              *
*                                                                      *
*  Crude random number generator - taken from Kernigan & Ritchie,      *
*  second edition, page 46.  This is very portable, but generates      *
*  only 15-bit random numbers, which is good enough for benchmarking.  *
*                                                                      *
************************************************************************
*  Rev, 10/22/88, GNR - randskip added                                 *
*  Rev, 08/08/92, GNR - shift,AND used in place of division,remainder  *
*  ==>, 07/01/02, GNR - Last date before committing to svn repository  *
***********************************************************************/

#define RAND_MAX 32767

unsigned long int next = 1;

/*-------------------------------------------------------------------*/
/*                    srand: set seed for rand()                     */
/*-------------------------------------------------------------------*/

void srand(unsigned int seed) {

   next = seed;
   }

/*-------------------------------------------------------------------*/
/*          rand: return pseudo-random integer on 0..32767           */
/*-------------------------------------------------------------------*/

int rand(void) {

   next = next * 1103515245 + 12345;
   return (unsigned int)(next>>16) & RAND_MAX;
   }

/*-------------------------------------------------------------------*/
/*          randskip: skip n random numbers in the sequence          */
/*-------------------------------------------------------------------*/

/* Note: Thanks to Joe Brandenberg of Intel Scientific Computers for
   the idea used in this routine.  The formula for the k'th number
   that would be returned by the 'rand' function is
      x(k) = (A**k)*x + C*Sum(j=0 to k-1)(k**j) (mod 2**32)
   where A = 1103515245, C = 12345 (See Knuth's chapter on random
   number generators if this is not obvious.)  This is expressed as
   x(k) = P(k)*x + Q(k) and the values of P and Q are tabulated for
   values of k that are exact powers of two.  The following code
   then obtains x(k) by repeated application of the above formula
   for each tabulated value that corresponds to a one in the binary
   representation of k.  This method runs in time proportional to
   log(2)(k), and represents a compromise between speed and the
   space that would be required to store larger tables of P and Q
   values.  The tabulated values of P and Q were calculated using
   the program 'randtabl c' and the smaller ones were checked with a
   REXX program that did the same calculation with explicit multi-
   precision arithmetic.  The Q coefficients are obtained by adding
   up the geometric series for Q rather than by using the well-known
   closed form for the sum in order to avoid the need to perform a
   multi-precision quotient by A-1.  Note that the last two values
   of P are both 1, which means that after 2**30 calls, this random
   number generator enters a simple arithmetic sequence from which
   it never escapes.  This condition is not important for the uses
   to which the this routine will be put.  The values are included
   in the table so the program can operate with no danger of having
   an excessive subscript into one of these tables.  No test is made
   for reaching the arithmetic sequence region.                      */

void randskip(unsigned long n) {

   static unsigned long P[32] = {
      1103515245ul, 3265436265ul, 3993403153ul, 3487424289ul,
      1601471041ul, 2335052929ul, 1979738369ul,  387043841ul,
      3194463233ul, 3722397697ul, 1073647617ul, 2432507905ul,
      1710899201ul, 3690233857ul, 4159242241ul, 4023517185ul,
      3752067073ul, 3209166849ul, 2123366401ul, 4246732801ul,
      4198498305ul, 4102029313ul, 3909091329ul, 3523215361ul,
      2751463425ul, 1207959553ul, 2415919105ul,  536870913ul,
      1073741825ul, 2147483649ul,          1ul,          1ul } ;
   static unsigned long Q[32] = {
           12345ul, 3554416254ul, 3596950572ul, 3441282840ul,
      1695770928ul, 1680572000ul,  422948032ul, 3058047360ul,
       519516928ul,  530212352ul, 2246364160ul,  646551552ul,
      3088265216ul,  472276992ul, 3897344000ul, 2425978880ul,
       556990464ul, 1113980928ul, 2227961856ul,  160956416ul,
       321912832ul,  643825664ul, 1287651328ul, 2575302656ul,
       855638016ul, 1711276032ul, 3422552064ul, 2550136832ul,
       805306368ul, 1610612736ul, 3221225472ul, 2147483648ul } ;
   register int k;            /* Power of two being considered */
   register unsigned long rn; /* Working copy of n */

   k = 0;
   rn = n;
   while (rn) {
      if (rn & 1) next = P[k]*next + Q[k];
      k++; rn >>= 1;
      }
   }
