/* (c) Copyright 2003-2014, The Rockefeller University *11115* */
/* $Id: mrssw.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                                mrssw                                 *
*                                                                      *
*  This function multiplies a 64-bit signed integer by a 32-bit sign-  *
*  ed integer, keeps only the low-order 64 bits of the product, scales *
*  these bits by a specified right shift, and returns the signed 64-   *
*  bit result.  To get a 96-bit intermediate product and full error    *
*  checking, use mrsswe or mrsswd.                                     *
*                                                                      *
*  Note added, 11/24/08, GNR - This routine and its HAS_I64 macro re-  *
*  placement are now considered obsolete and are retained solely for   *
*  compatibility purposes.  They should be replaced by mrsswe(), which *
*  provides a 96-bit intermediate product and full overflow checking,  *
*  or by mrsrswe(), which also rounds before scaling. The restrictions *
*  to right shifting only and ignoring any overflows beyond 64 bits    *
*  were intended to allow this routine to be replaced by a fast macro  *
*  on machines that have intrinsic 64-bit arithmetic, but this led to  *
*  unfortunate undetected errors in applications.  For one thing, the  *
*  macro shifted negative numbers, but the routine shifted positive    *
*  numbers, then negated, resulting in a one-bit discrepancy in the    *
*  final answer.  Also, if a positive product overflowed into just the *
*  sign bit, this routine returned a correct result, but the macro     *
*  made it negative.  These were not worth fixing, because many        *
*  overflows were never detected at all, and the macro was never used  *
*  in any application.  Indeed, any errors found in this routine in    *
*  the future should not be corrected, as its sole purpose is now to   *
*  allow exact recapitulation of old runs when needed.                 *
*                                                                      *
*  Synopsis: si64 mrssw(si64 x, si32 y, int s)                         *
*                                                                      *
*  N.B.  Program assumes 0 <= s < 64 without checking.                 *
*                                                                      *
*  This routine is part of a basic 64-bit arithmetic package for which *
*  full C implementations are provided in order to get one started on  *
*  a new processor.  These versions for 32-bit systems can be reimple- *
*  mented in Assembler if greater speed is needed.                     *
*                                                                      *
************************************************************************
*  V1A, 04/13/03, GNR - New routine                                    *
*  Rev, 11/24/08, GNR - Now obsolete, see comment above                *
*  ==>, 11/26/08, GNR - Last date before committing to svn repository  *
*  Rev, 10/25/13, GNR - Get rid of 'L' constants                       *
*  Rev, 04/06/14, GNR - More careful handling of low-order sign bit    *
***********************************************************************/

#include <math.h>
#include <stdlib.h>
#include "sysdef.h"
#include "rkarith.h"

#ifdef HAS_I64
#error mrssw.c not compiled with HAS_I64, is a macro
#endif

#ifdef L16
#undef L16
#endif
#define L16 0x0000ffffUL

si64 mrssw(si64 x, si32 y, int s) {

   si64 rv;
   si32 xhi = swhi(x);
   ui32 xlo = swlou(x);
   ui32 ay  = abs32(y);
   ui32 axh = xhi;
   ui32 x0,x1,x2,x3,y0,y1;
   ui32 p0,p1,p2,p3;

/* Get absolute value of x */

   if (xhi < 0) {
      xlo = ~xlo + 1;
      axh = ~axh + (xlo == 0);
      }

/* Break x into 4 and y into 2 16-bit pieces */

   x3 = axh >> 16;
   x2 = axh & L16;
   x1 = xlo >> 16;
   x0 = xlo & L16;

   y1 = ay >> 16;
   y0 = ay & L16;

/* Multiply (x3,x2,x1,x0) * (y1,y0) in base 2^16 and recombine into
*  two ui32 words (p2,p0).  Any overflow beyond 64 bits is ignored,
*  consistent with the usual implementation of 64-bit hardware
*  arithmetic.  The intermediate product sums cannot overflow.  */

   p0 = x0*y0;
   p1 = x1*y0 + x0*y1 + (p0>>16);
   p2 = x2*y0 + x1*y1 + (p1>>16);
   p3 = x3*y0 + x2*y1 + (p2>>16);
   p2 &= L16; p2 |= (p3<<16);
   p0 &= L16; p0 |= (p1<<16);

/* Scale according to argument s.  Note that cases involving s == 0
*  must be handled separately because on some machines (e.g. Intel
*  486 and up), a shift of 32 is a NOP.  */

   if (s) {
      if (s >= BITSPERUI32) {
         rv.lo = p2 >> (s - BITSPERUI32);
         rv.hi = 0;
         }
      else {
         rv.lo = (p2 << (BITSPERUI32 - s)) | (p0 >> s);
         rv.hi = p2 >> s;
         }
      }
   else {
      /* Perform no shift at all */
      rv.lo = p0;
      rv.hi = p2;
      }

/* Impose correct sign on the product */

   if ((xhi ^ y) < (si32)0) {
      rv.lo = -rv.lo;
      rv.hi = ~rv.hi + (rv.lo == 0);
      }

   return rv;

   } /* End mrssw() */
