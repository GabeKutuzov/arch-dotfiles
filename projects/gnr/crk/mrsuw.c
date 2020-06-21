/* (c) Copyright 2003-2014, The Rockefeller University *11115* */
/* $Id: mrsuw.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                                mrsuw                                 *
*                                                                      *
*  This function multiplies a 64-bit unsigned integer by a 32-bit un-  *
*  signed integer, keeps only the low-order 64 bits of the product,    *
*  scales these bits by a specified right shift, and returns the un-   *
*  signed 64-bit result.                                               *
*                                                                      *
*  Note added, 11/24/08, GNR - This routine and its HAS_I64 macro re-  *
*  placement are now considered obsolete and are retained solely for   *
*  compatibility purposes.  They should be replaced by mrsuw[de](),    *
*  which keeps a 96-bit intermediate product and provides full over-   *
*  flow checking, or by mrsruw[de](), which also rounds before scaling.*
*  The restrictions to right shifting only and ignoring any overflows  *
*  beyond 64 bits were intended to allow this routine to be replaced   *
*  by a fast macro on machines that have intrinsic 64-bit arithmetic,  *
*  but this led to unfortunate undetected errors in applications.      *
*                                                                      *
*  Synopsis: ui64 mrsuw(ui64 x, ui32 y, int s)                         *
*                                                                      *
*  N.B.  Program assumes 0 <= s < 64 without checking                  *
*                                                                      *
*  This routine is part of a basic 64-bit arithmetic package for which *
*  full C implementations are provided in order to get one started on  *
*  a new processor.  These versions for 32-bit systems can be reimple- *
*  mented in Assembler if greater speed is needed.                     *
************************************************************************
*  V1A, 04/13/03, GNR - New routine                                    *
*  Rev, 11/24/08, GNR - Now obsolete, see comment above                *
*  ==>, 11/26/08, GNR - Last date before committing to svn repository  *
*  Rev, 09/27/14, GNR - Correct error in intermediate product carries  *
***********************************************************************/

#include <stdlib.h>
#include <math.h>
#include "sysdef.h"
#include "rkarith.h"

#ifdef HAS_I64
#error mrsuw.c not compiled with HAS_I64, is a macro
#endif

#define LU16 0x0000ffffUL

ui64 mrsuw(ui64 x, ui32 y, int s) {

   ui64 rv;
   ui32 x0,x1,x2,x3,y0,y1;
   ui32 p0,p1,p2,p3,q1,q2,q3;
   ui32 xhi = uwhi(x);
   ui32 xlo = uwlo(x);
   int ds;

/* Break x into 4 and y into 2 16-bit pieces */

   x3 = xhi >> BITSPERSHORT;
   x2 = xhi & LU16;
   x1 = xlo >> BITSPERSHORT;
   x0 = xlo & LU16;

   y1 = y >> BITSPERSHORT;
   y0 = y & LU16;

/* Multiply (x3,x2,x1,x0) * (y1,y0) in base 2^16 and, discarding
*  high-order 32 bits per spec, hold parts of result in four ui32
*  words (p3,p2,p1,p0), then recombine to give 64 bits in (p2,p0).
*  Be careful to avoid overflow when adding partial products.  */

   p0 = x0*y0;
   p1 = x1*y0 + (p0>>BITSPERSHORT);
   p2 = x2*y0 + (p1>>BITSPERSHORT);
   p3 = x3*y0 + (p2>>BITSPERSHORT);

   q1 = x0*y1;
   q2 = x1*y1 + (q1>>BITSPERSHORT);
   q3 = x2*y1 + (q2>>BITSPERSHORT);

   p1 = (p1 & LU16) + (q1 & LU16);
   p2 = (p2 & LU16) + (q2 & LU16) + (p1>>BITSPERSHORT);
   p3 = (p3 & LU16) + (q3 & LU16) + (p2>>BITSPERSHORT);

/* Get middle 32 bits of result in p2 for shifting.
*  (Low bits are not needed until final case below.)  */

   p2 = (p2 & LU16) | (p3<<BITSPERSHORT);

/* Scale according to argument s.  Be careful never to do a shift of
*  32, because it is a NOP on some machines (e.g.  Intel 486 and up).
*/

   ds = s - BITSPERUI32;
   if (ds >= 0) {
      rv.lo = p2 >> ds;
      rv.hi = 0;        /* Incorrect, but per spec */
      }
   else {
      p0 = (p0 & LU16) | (p1<<BITSPERSHORT);
      if (s == 0) {
         rv.lo = p0;
         rv.hi = p2;
         }
      else {
         rv.lo = p2 << (-ds) | p0 >> s;
         rv.hi = p2 >> s;
         }
      }
   return rv;

   } /* End mrsuw() */
