/* (c) Copyright 2017, The Rockefeller University *11115* */
/* $Id: dsrswwqd.c 63 2017-04-13 20:47:00Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                               dsrswwqd                               *
*                                                                      *
*  This function scales a 64-bit signed fixed-point value by a given   *
*  shift amount (0 <= shift <= 32) to get a 96-bit dividend, divides   *
*  by a 64-bit signed divisor, and rounds by adding the high bit of    *
*  the remainder to the quotient, with overflow checking on the result.*
*  This is the version that uses the error code stored by e64dec().    *
*                                                                      *
*  Synopsis: si64 dsrswwqd(si64 x, si64 y, int s)                      *
*                                                                      *
*  Arguments:                                                          *
*     x        64-bit numerator.                                       *
*     y        64-bit divisor.                                         *
*     s        left shift.                                             *
*                                                                      *
*  Method:  Program converts x and y to the unsigned versions, shifts  *
*  left by the specified amount, uses vdivl() to perform the division  *
*  and rounding, then assigns the correct sign to the result.  There   *
*  was a plan to call dsruwwqe() to save some redundant work, but this *
*  version saves call overhead and avoids a double call to e64dac() on *
*  quotient overflow.                                                  *
*                                                                      *
*  Method revised, R63, 02/05/17:  In 64-bit machine, if shift does    *
*  not extend dividend beyond 63 bits, can use faster hardware         *
*  division.                                                           *
*                                                                      *
*  Notes:                                                              *
*  (1) Program issues abexit 74 if s is outside the range 0 <= s <= 32.*
*  (2) Division is performed with vdivl().  A divisor of 0 is not      *
*  considered an error, but returns a result of 0.                     *
*                                                                      *
*  This routine is part of a basic 64-bit arithmetic package for which *
*  full C implementations are provided in order to get one started on  *
*  a new processor.  These versions can be reimplemented in Assembler  *
*  if greater speed is needed.                                         *
*                                                                      *
************************************************************************
*  V1A, 10/24/13, GNR - New routine                                    *
*  ==>, 10/24/13, GNR - Last date before committing to svn repository  *
*  R63, 02/05/17, GNR - Use hardware division if problem fits          *
***********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <math.h>
#include "sysdef.h"
#include "rkarith.h"

#define NX  3              /* Size of dividend in 32-bit words */
#define NY  2              /* Size of divisor in 32-bit words */

si64 dsrswwqd(si64 x, si64 y, int s) {

   union { ui64 xu64[3]; si64 xs64[3]; ui32 xu32[6]; } ux;
   ui32 wk[4*NX+2*NY+2];
   int  sa;                /* Sign of the answer */
   int  ixo;               /* Offset in ux array */

#ifdef HAS_I64
   sa = (x ^ y) < 0;
#else
   sa = (swhi(x) ^ swhi(y)) < 0;
#endif

   /* Check for shift out-of-range */
   if (s > 32 || s < 0) { abexitq(74); return jesl(0); }

   /* Get magnitude of dividend and divisor */
   ux.xs64[1] = abs64(x);
   y = abs64(y);

#ifdef HAS_I64
   if (y == 0) return 0;
#if LSIZE == 8
   ux.xu64[0] = 1L << (63-s);
#else
   ux.xu64[0] = 1LL << (63-s);
#endif
   if (ux.xu64[1] < ux.xu64[0]) {
      ux.xs64[1] = ((ux.xs64[1] << s) + (y >> 1))/y;
      return sa ? jnsw(ux.xs64[1]) : ux.xs64[1];
      }
#endif

#if BYTE_ORDRE > 0         /* BIG-ENDIAN */
   /* Shift dividend into xu array */
   if (s == 32)     ux.xu32[4] = 0, ixo = 2;
   else if (s == 0) ux.xu32[1] = 0, ixo = 1;
   else {
      ux.xu64[0] = jsruw(ux.xu64[1],64-s);
      ux.xu64[1] = jsluw(ux.xu64[1],s);
      ixo = 1; }

   /* Perform division and rounding */
   vdivl(ux.xu32+ixo, (ui32 *)&y, ux.xu32+1, NULL, wk, NX, NY, TRUE);

   /* Check for overflow into high-order */
   if (ux.xu32[1] | ux.xu32[2] & ~SI32_MAX) {
      e64dac("dsrswwqd"); ux.xs64[1] = SI64_MAX; }

#else                      /* LITTLE-ENDIAN */
   /* Shift dividend into xu array */
   if (s == 32)     ux.xu32[1] = 0, ixo = 1;
   else if (s == 0) ux.xu32[4] = 0, ixo = 2;
   else {
      ux.xu64[2] = jsruw(ux.xu64[1],64-s);
      ux.xu64[1] = jsluw(ux.xu64[1],s);
      ixo = 2; }

   /* Perform division and rounding */
   vdivl(ux.xu32+ixo, (ui32 *)&y, ux.xu32+2, NULL, wk, NX, NY, TRUE);

   /* Check for overflow into high-order */
   if (ux.xu32[4] | ux.xu32[3] & ~SI32_MAX) {
      e64dac("dsrswwqd"); ux.xs64[1] = SI64_MAX; }
#endif

   return sa ? jnsw(ux.xs64[1]) : ux.xs64[1];

   } /* End dsrswwqd() */
