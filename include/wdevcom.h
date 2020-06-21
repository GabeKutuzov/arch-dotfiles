/* (c) Copyright 2009-2010, The Rockefeller University *11115* */
/* $Id: wdevcom.h 15 2009-12-30 22:02:21Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                              wdevcom.h                               *
*                                                                      *
*  This header is used to implement the 'wdev' family of pseudorandom  *
*  number generators for the ROCKS system (prototyped in rkwdev.h).    *
*  Each of these functions has as its basic operation the generation   *
*  of one or more uniformly distributed pseudorandom integers in the   *
*  range 1 <= ir < (2**31).  The various functions convert these into  *
*  floats, Gaussian variates, set selectors, and so on.  Each includes *
*  this header file, which is more like a macro or inline function, to *
*  assure that the basic operation is performed the same way in each   *
*  routine.  Also, any changes can be made just in this one place.     *
*                                                                      *
*  The seed is represented as a 'wseed' struct, defined in sysdef.h.   *
*  The period will be about 2**57 if both components of the wseed are  *
*  nonzero.  If the 'seed27' (hi-order) part of the seed is < 0, the   *
*  output is identical to that produced by the earlier routine udev(). *
*                                                                      *
*  For compatibility with earlier code, generation of signed random    *
*  numbers and Gaussian variates is somewhat grotty.  If WDEV_SIGN is  *
*  defined, a signed value is generated.  When seed27 is negative, we  *
*  are in compatibility mode and that seed is not used.  Instead, the  *
*  31-bit result is left-shifted into the sign bit.  When seed27 is    *
*  used, the sign is taken from a bit of that seed that is otherwise   *
*  discarded.  If WDEV_NORM is defined, a fixed-point, zero-mean,      *
*  unit-standard deviation normal variate is generated.  For compat    *
*  with ndev(), the code must multiply by sigma before setting the     *
*  sign.  Therefore, both steps are done in the wrappers.  The numeric *
*  value is left in r and the sign, as in old ndev(), is taken from    *
*  bit 22 of rand.  The first eight bits of r are used to select from  *
*  a look-up table an interval of width 0.125 in the range 0.0-3.0     *
*  with appropriate frequency.  The remaining bits specify the loca-   *
*  tion within the chosen interval and the sign.  The resulting value  *
*  is multiplied by 'sigma' and finally 'mean' is added.  The result   *
*  is a piecewise rectangular approximation to the true Gaussian       *
*  distribution, good enough for most purposes.  See Knuth, Vol.  2.   *
*                                                                      *
*  If an array of results is being generated, the updated seeds are    *
*  stored via the caller's pointer on each pass through the loop.      *
*  This may seem wasteful, but it is probably no slower than storing   *
*  them on the stack for transfer to the calling routine when the      *
*  entire array is complete.  The Intel architecture probably doesn't  *
*  have enough registers to keep them in registers the whole time.     *
*                                                                      *
*  It was decided to insert the same code inline in each function,     *
*  rather than have each call the basic wdev(), in order to reduce     *
*  call overhead in time-sensitive applications.  The total amount     *
*  of code is not large, and any one application is likely to use      *
*  only a few of the 16 wdev functions provided in any event.          *
*                                                                      *
*  Algorithm:  A 31-bit result is considered to be adequate for the    *
*  anticipated uses of this routine, but a period longer than 2**31    *
*  is necessary for some large-scale simulations.  A compatibility     *
*  mode giving results identical to udev() is another requirement,     *
*  as is a routine to skip ahead N numbers (needed to get identical    *
*  results when running on different numbers of nodes on a parallel    *
*  computer).  Accordingly, two independent linear-congruential ge-    *
*  nerators are used, one with base 2**31-1 and the other with base    *
*  2**27+1.  The results of the two generators are combined to give    *
*  the result that is returned.  The 27-bit number is broken into 12   *
*  low-order bits and 15 high-order bits and XOR'd with the 31-bit     *
*  number, thus improving the statistics of the low-and high-order     *
*  bits that are most often used.  The C code emulates a hardware      *
*  rotate operation to facilitate a faster assembly-code implement-    *
*  ation if desired.  Note is taken of Knuth's remark that the low-    *
*  order bits of linear-congruential random numbers are the least      *
*  random.                                                             *
*  The 31-bit generator uses:                                          *
*     result = new seed = seed*(7**5) MODULO (2**31-1)                 *
*  The 27-bit generator uses:                                          *
*     result = new seed = (seed*(3*19*87211+1)+28363543) MOD (2**27+1) *
*  (The 2**31-1 base is prime, therefore the full period is obtained   *
*  with the "classical" 7**5 multiplier and no additive constant.      *
*  The 2**27+1 (z27) base is composite, so in order to obtain the full *
*  period, the multiplier needs to be one more than a multiple of all  *
*  its prime factors and an additive constant is needed as well.  This *
*  is taken as z27*((1/2) - sqrt(3)/6) (but relatively prime to z27)   *
*  to minimize correlations per another suggestion of Knuth.)          *
*                                                                      *
*  Fortunately, the MODULO operations can be carried out without       *
*  actual division because of the special form of the divisors:        *
*     aX = q(w+1) + (r-q) ==> aX mod (w+1) = r-q or r-q+(w+1) and      *
*     aX = q(w-1) + (r+q) ==> aX mod (w-1) = r+q or r+q-(w-1).         *
*                                                                      *
*  This routine probably should be implemented in Assembly language,   *
*  at least on machines that do not have 64-bit arithmetic, because    *
*  the lack of access to the carry flag in C makes implementation      *
*  clumsy and execution slow.  The present C code is intended to       *
*  provide a working version to get started on a new port.  Exact      *
*  compatibility with the traditional udev routine is a main goal      *
*  here, as well as speed.  Examination of the code emitted by the C   *
*  compiler may provide a useful template for a new Assembler version. *
*                                                                      *
************************************************************************
*  V1A, 12/25/09, GNR - New program                                    *
*  V1B, 01/08/10, GNR - Extracted from wdev.c to make this header      *
*  ==>, 02/06/10, GNR - Last date before committing to svn repository  *
***********************************************************************/

#ifndef WDEVCOM_DEFINED
#define WDEVCOM_DEFINED

#if defined(HAS_I64) && LSIZE == 4
#define m31 16807LL              /* Multiplier for seed31 */
#define w31 0x7fffffffLL         /* Base mask for seed31  */
#define m27 4971028LL            /* Multiplier for seed27 */
#define w27 0x07ffffffLL         /* Base mask for seed27  */
#define c27 28363543LL           /* Additive constant     */
#else
#define m31 16807L
#define w31 0x7fffffffL
#define m27 4971028L
#define w27 0x07ffffffL
#define c27 28363543L
#endif
#define mlo 55828
#define mhi 75
#define b22 0x00400000
#define f31 2147483648.0

/***********************************************************************
*  N.B.  The following code is assumed to be included inside a C code  *
*  block in the routine that includes this header file.  If WDEV_JDEF  *
*  is defined, just the above definitions are included.                *
***********************************************************************/

#ifndef WDEV_JDEF
#ifdef WDEV_NORM
   /* Table of probabilities for normal distribution generator */
   extern const unsigned char normtab[256];
#endif

   si32 r,rand;                  /* Remainder, result */

#ifdef HAS_I64                   /* --- 64-bit version --- */

   si64 sm;
   /* Update the 31-bit seed */
   sm = (si64)(seed->seed31) * m31;
   sm = (sm & w31) + (sm >> 31);    /* r + q */
   sm = (sm & w31) + (sm >> 31);    /* Correct if (r + q > w) */
   rand = seed->seed31 = (si32)sm;
   /* Update the 27-bit seed (if not negative).
   *  (Braces are closed after else--endif below) */
   if (seed->seed27 >= 0) {
      sm = (si64)(seed->seed27) * m27 + c27;
      sm = (sm & w27) - SRA(sm,27); /* r - q */
      sm = (sm & w27) - SRA(sm,27); /* Correct if (r - q < 0) */
      r = seed->seed27 = (si32)sm;

#else                            /* --- 32-bit version --- */

   /* Form the m31 product in low 16 and high 31 bit pieces.
   *  (m31 is < 2**16 so only two multiplies are needed.)  */
   ui32 tt1 = ((ui32)(seed->seed31) & 0xffffL) * m31;
   ui32 smlo = tt1 & 0xffffL;
   ui32 smhi = ((ui32)(seed->seed31) >> 16) * m31 + (tt1 >> 16);
   /* Form remainder modulo (2**31-1) using Knuth trick */
   smlo += ((smhi & 0x7fffL) << 16) + (smhi >> 15);
   /* Final adjustment--if the result exceeds (2**31-1),
   *  subtract (2**31-1) one more time.  This boils down to:
   *  if the sign bit is set, clear the sign and add one.  */
   rand = seed->seed31 = (si32)((smlo + (smlo >> 31)) & w31);
   /* Update the 27-bit seed (if not negative).  (m27 is > 2**16,
   *  so four multiplies are needed.  Code is taken from jmuw with
   *  minor simplifications--and no call overhead.)  */
   if (seed->seed27 >= 0) {
      /* Multiply (A*(2**16) + B) by (mhi*(2**16) + mlo),
      *  where A, B, mhi, and mlo are 16-bit quantities.
      *  High 32 bits of result =
      *        A*mhi + <high order 16 bits of (A*mlo + B*mhi)>
      *  Low 32 bits of result =
      *        B*mlo + <low order 16 bits of (A*mlo + B*mhi)>
      *  Unlike the case with jmuw, xx = (A*mlo + B*mhi) cannot
      *  overflow beyond 32 bits.  */
      ui32 ws27 = (ui32)seed->seed27;
      ui32 a = ws27 >> 16;
      ui32 b = ws27 & 0x0000ffffUL;
      ui32 bd = b * mlo + c27;
      ui32 xx = a * mlo + b * mhi;
      ui32 xl = xx << 16;
      smlo = bd + xl;
      smhi = (a * mhi) + (xx >> 16) + (~xl < bd);
      /* Separate the result into quotient and remainder */
      xx = (smhi << 5) | (smlo >> 27);
      r = (si32)(smlo & w27) - (si32)xx;
      r = seed->seed27 = (r & w27) - SRA(r,27);

#endif   /* 64- vs 32-bit seed manipulation */

      r = (r >> 15) | (r << 17); /* Rotate */
      rand ^= r;                 /* Make signed rand */
#ifdef WDEV_NORM                 /* Return normal variate */
      /* Use the left 8 bits as an index to pick up a rectangle.
      *  Combine 7 bits from rectangle, 22 from fraction, 3 zeros. */
      r = ((rand & 0x003fffff) << 3) |
          ((si32)normtab[(rand & w31) >> 23] << 25);
#elif !defined(WDEV_SIGN)        /* Return unsigned rand */
      rand &= w31;
#endif
      } /* End if seed27 block */
#ifdef WDEV_NORM                 /* Return compat normal variate */
   else {
      r = ((rand & 0x003fffff) << 3) |
         ((si32)normtab[rand >> 23] << 25);
      }
#elif defined(WDEV_SIGN)
   else
      rand <<= 1;
#endif
#endif /* !defined WDEV_JDEF */
#endif /* !defined WDEVCOM_DEFINED */
