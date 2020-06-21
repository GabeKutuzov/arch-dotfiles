/* (c) Copyright 2017, The Rockefeller University *11115* */
/* $Id: dmrswwqd.c 64 2017-09-15 17:55:59Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                               dmrswwqd                               *
*                                                                      *
*  This function multiplies a 64-bit signed fixed-point value by a     *
*  given 32-bit multiplier to get a 96-bit dividend, divides by a      *
*  64-bit signed divisor, and rounds by adding one to the quotient     *
*  if the remainder is more than half the divisor, with overflow       *
*  checking on the result.  There is no built-in shift--result will    *
*  have number of fraction bits of x+m-y.                              *
*  This is the version that uses the error code stored by e64dec().    *
*                                                                      *
*  Synopsis: si64 dmrswwqd(si64 x, si64 y, si32 m)                     *
*                                                                      *
*  Arguments:                                                          *
*     x        64-bit numerator.                                       *
*     y        64-bit divisor.                                         *
*     m        32-bit multiplier.                                      *
*                                                                      *
*  Method:  Program converts x and y to the unsigned versions, does    *
*  the specified multiplication, uses vdivl() to perform the division  *
*  and rounding, then assigns the correct sign to the result.          *
*                                                                      *
*  In 64-bit machine, if the multiplication does not extend dividend   *
*  beyond 63 bits, can use faster hardware division.                   *
*                                                                      *
*  Notes:                                                              *
*  (1) Division is performed with vdivl().  A divisor of 0 is not      *
*  considered an error, but returns a result of 0.                     *
*  (2) ux indexing is arranged to end up with three adjacent ui32s     *
*  in the dividend for vdivl() as follows:                             *
*     xu32 element   BIG-ENDIAN           LITTLE-ENDIAN                *
*           0        xhi      ,quot       (xhi*m)lo,quot               *
*           1        xlo      ,(x*m)hi    (xhi*m)hi                    *
*           2        (xlo*m)hi,(x*m)mid   (xlo*m)lo                    *
*           3        (xlo*m)lo            (xlo*m)hi,(x*m)mid           *
*           4        (xhi*m)hi,rem        xlo      ,(x*m)hi, rem       *
*           5        (xhi*m)lo            xhi                          *
*                                                                      *
*  This routine is part of a basic 64-bit arithmetic package for which *
*  full C implementations are provided in order to get one started on  *
*  a new processor.  These versions can be reimplemented in Assembler  *
*  if greater speed is needed.                                         *
*                                                                      *
************************************************************************
*  V1A, 08/26/17, GNR - New routine, based on dsrswwqd                 *
*  ==>, 08/26/17, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <math.h>
#include "sysdef.h"
#include "rkarith.h"

#define NX  3              /* Size of dividend in 32-bit words */
#define NY  2              /* Size of divisor in 32-bit words */

si64 dmrswwqd(si64 x, si64 y, si32 m) {

   union { ui64 xu64[3]; si64 xs64[3]; ui32 xu32[6]; } ux;
   ui32 wk[4*NX+2*NY+2];
   int  sa;                /* Sign of the answer */

   if (quw(x) == 0 || quw(y) == 0 || m == 0) return jesl(0);

#ifdef HAS_I64
   sa = (x ^ y ^ (si64)m) < 0;
#else
   sa = (swhi(x) ^ swhi(y) ^ m) < 0;
#endif

   /* Get magnitude of dividend, multiplier, and divisor */
   y = abs64(y);
   m = abs32(m);

#if BYTE_ORDRE > 0         /* BIG_ENDIAN */
   ux.xs64[0] = abs64(x);
   /* Low-order and hi-order products */
   ux.xu64[1] = jmuw(ux.xu32[1], m);
   ux.xu64[2] = jmuw(ux.xu32[0], m);
   /* Add to form 96-bit product (with carry) */
   ux.xu32[1] = ux.xu32[4] + (ux.xu32[2] > ~ux.xu32[5]);
   ux.xu32[2] += ux.xu32[5];
   /* Check for overflow into high 32, then for quotient overflow.
   *  Always use vdivl() if no native 64-bit arithmetic.  */
#ifdef HAS_I64
   if (ux.xu32[1]) {
#endif
      /* Overflow or no I64, perform division and rounding with vdivl */
      vdivl(ux.xu32+1, (ui32 *)&y, ux.xu32+1, NULL, wk, NX, NY, TRUE);
      if (ux.xu32[1] | ux.xu32[2] & SI32_SGN) {
         e64dac("dmrswwqd"); ux.xs64[1] = SI64_MAX; }
#ifdef HAS_I64
   } else {
      /* Can use native 64-bit division, quotient cannot overflow.
      *  But now must do rounding explicitly.  Note:  There is no
      *  div-type function in clib for unsigned values, but it is
      *  claimed that most compilers will optimize the /% pair into
      *  the corresponding single machine instruction.  */
      ux.xu64[1] = ux.xu64[1] / y + (ux.xu64[1] % y >= y >> 1);
      }
#endif

#else                      /* LITTLE-ENDIAN */
   ux.xs64[2] = abs64(x);
   /* Low-order and hi-order products */
   ux.xu64[1] = jmuw(ux.xu32[4], m);
   ux.xu64[0] = jmuw(ux.xu32[5], m);
   /* Add to form 96-bit product (with carry) */
   ux.xu32[4] = ux.xu32[1] + (ux.xu32[3] > ~ux.xu32[0]);
   ux.xu32[3] += ux.xu32[0];
   /* Check for overflow into high 32, then for quotient overflow.
   *  Always use vdivl() if no native 64-bit arithmetic.  */
#ifdef HAS_I64
   if (ux.xu32[4]) {
#endif
      /* Overflow or no I64, perform division and rounding with vdivl */
      vdivl(ux.xu32+2, (ui32 *)&y, ux.xu32+2, NULL, wk, NX, NY, TRUE);
      if (ux.xu32[4] | ux.xu32[3] & SI32_SGN) {
         e64dac("dmrswwqd"); ux.xs64[1] = SI64_MAX; }
#ifdef HAS_I64
   } else {
      /* See comment above */
      ux.xu64[1] = ux.xu64[1] / y + (ux.xu64[1] % y >= y >> 1);
      }
#endif

#endif         /* End BIG-ENDIAN, LITTLE-ENDIAN distinction */

   /* Assign final sign to answer and return */
   return sa ? jnsw(ux.xs64[1]) : ux.xs64[1];

   } /* End dmrswwqd() */
