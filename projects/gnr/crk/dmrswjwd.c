/* (c) Copyright 2013-2014, The Rockefeller University *11115* */
/* $Id: dmrswjwd.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                              dmrswjwd                                *
*                                                                      *
*  This function multiplies a 64-bit signed integer 'x' by a 32-bit    *
*  signed integer 'y' to get a 94-bit intermediate product, divides    *
*  by a 32-bit divisor 'd', rounds by adding one-half the smallest     *
*  bit that will be retained after scaling, scales by a specified      *
*  right shift 's', and returns the signed 64-bit result with full     *
*  overflow checking.  (This sequence of operations might be useful    *
*  where a sum is to be converted to an average, then scaled with      *
*  rounding.)                                                          *
*  This is the version that uses the error code stored by e64dec().    *
*                                                                      *
*  Synopsis: si64 dmrswjwd(si64 x, si32 y, si32 d, int s)              *
*                                                                      *
*  Notes:                                                              *
*  (1) Program requires |s| < 64, generates abexit 74 if |s| >= 64.    *
*  's' may be coded as a negative value for consistency with the       *
*  rkarith routines that use this to indicate a right shift.           *
*  (2) If the divisor is 0, a result of 0 is returned and no error     *
*  occurs.                                                             *
*  (2) To keep a 94-bit product, the multiplication has to be carried  *
*  out in pieces even if the machine HAS_I64.  This is done using the  *
*  magnitudes of the multiplicands.                                    *
*  (3) Shifting followed by negation gives an answer one bit different *
*  than SRA-type shifting of negatives, but because of rounding here,  *
*  this is not really an error, as it just affects cases where the     *
*  discarded fraction is exactly 1/2.  The code does assure that the   *
*  results are the same for HAS_I64 and 32-bit compilations.           *
*  (4) Values of 'x' or 'y' equal to the most negative number are not  *
*  necessarily errors--they can be handled if 's' is large enough to   *
*  bring the answer back into the representable range.  The result of  *
*  taking the absolute value of such a number in C is undefined, but   *
*  this code simply assumes negation leaves these numbers unchanged.   *
*                                                                      *
*  This routine is part of a basic 64-bit arithmetic package for which *
*  full C implementations are provided in order to get one started on  *
*  a new processor.  These versions can be reimplemented in Assembler  *
*  if greater speed is needed.                                         *
*                                                                      *
************************************************************************
*  V1A, 10/21/13, GNR - New routine, based on msrswe                   *
*  ==>, 10/21/13, GNR - Last date before committing to svn repository  *
*  Rev, 04/06/14, GNR - More careful handling of low-order sign bit    *
***********************************************************************/

#include <stdlib.h>
#include <math.h>
#include "sysdef.h"
#include "rkarith.h"

si64 dmrswjwd(si64 x, si32 y, si32 d, int s) {

#define BU32 BITSPERUI32
#define BU16 BITSPERSHORT

#ifdef HAS_I64

/*---------------------------------------------------------------------*
*                    SYSTEM HAS 64-BIT ARITHMETIC                      *
*---------------------------------------------------------------------*/

#if LSIZE == 8
#define LS32 0x000000007fffffffUL
#define LU32 0x00000000ffffffffUL
#define wdiv(u,v) ldiv((u),(v));
   ldiv_t wqr;
#else
#define LS32 0x000000007fffffffULL
#define LU32 0x00000000ffffffffULL
#define wdiv(u,v) lldiv((u),(v));
   lldiv_t wqr;
#endif

   ui64 ax,p0,p1,ovt;
   ui32 ay,ad;
   int  ds,sr,ss = abs(s);

   if (ss >= 64) { abexitq(74); return jesl(0); }
   if (d == 0) return jesl(0);

   sr = 0;
   ax = x < 0 ? (sr = 1, -x) : x;
   ay = y < 0 ? (sr^= 1, -y) : y;
   ad = d < 0 ? (sr^= 1, -d) : d;
   ovt = LS32;

/* Multiply x * y in base 2^32 to get a 96-bit product.
*  Largest possible result is (2^63-1) * (2^31-1) < 2^94.
*  This product cannot overflow.  */

   p0 = (ax & LU32) * ay;
   p1 = (ax >> BU32) * ay + (p0 >> BU32);

/* Divide */

   wqr = wdiv(p1, ad);
   p1 = wqr.quot;
   wqr = wdiv((p0 & LU32) + (wqr.rem << BU32), ad);
   p0 = wqr.quot;

/* Round and scale, right-shifting by |s|.  */

   if ((ds = ss - BU32) >= 0) {
      /* Shift is 32 or more.  The high-order 32 result bits are
      *  always zero and there is no need to test for overflow.  */
      if (ds == 0) {
         /* Shift is exactly 32, add 2^31 bit from p0.
         *  The sum cannot overflow and there is no shift.  */
         p1 += p0 >> (BU32-1);
         }
      else {
         /* Shift is > 32, can ignore p0 portion.
         *  The rounding sum cannot overflow (because ds < 32).  */
         p1 = (p1 + (UI64_01 << (ds-1))) >> ds;
         }
      }
   /* Shift is < 32, rounding might carry into p0(hi).
   *  N.B.  In the following test, LS32 needs to be assigned
   *  to a variable ('ovt') rather than used directly in the 'if'--
   *  otherwise with -O2 compilation, the test fails.
   *  But the assignment p1 = SI64_MAX + sr below works OK.  */
   else if (ss == 0) {
      /* Shift is zero, rounding needs to be based on whether
      *  the division remainder is more than half the divisor. */
      if (wqr.rem > ad >> 1) {
         p0 += 1; p1 += p0 >> BU32; }
      if (p1 > ovt + sr) {
         e64dac("dmrswjwd");
         p1 = SI64_MAX + sr; }
      else
         p1 = p1 << BU32 | (p0 & LU32);
      }
   else {
      p0 += UI64_01 << (ss-1);
      p1 += (p0 >> BU32);
      if (p1 >> ss > ovt + sr) {
         /* Overflow if p1 >> |s| still has high-order bits */
         e64dac("dmrswjwd");
         p1 = SI64_MAX + sr; }
      else
         p1 = p1 << (-ds) | (p0 & LU32) >> ss;
      }

/* Impose correct sign on the product */

   return (sr ? -p1 : p1);

#else

/*---------------------------------------------------------------------*
*               SYSTEM DOES NOT HAVE 64-BIT ARITHMETIC                 *
*---------------------------------------------------------------------*/


#if BYTE_ORDRE > 0
#define JH  0
#define JM  1
#define JL  2
#else
#define JH  2
#define JM  1
#define JL  0
#endif

#ifdef L16
#undef L16
#endif
#define L16 0x0000ffffUL

   si64 rv;
   ui32 xy[3], dv[3], wk[20];
   ui32 x0,x1,x2,x3,y0,y1;
   ui32 p0,p1,p2,p3,p4,q1,q2,q3,q4;
   si32 xhi;
   ui32 axh,xlo,ay,ad,ovt;
   int  ndv,ds,sr,ss = abs(s);

   if (ss >= 64) { abexitq(74); return jesl(0); }
   if (d == 0) return jesl(0);

/* Get absolute values of x, y ,and d, store sign of result in sr */

   xhi = swhi(x); xlo = swlou(x);
   sr = 0;
   if (xhi < 0) {
      xlo = ~xlo + 1; axh = ~xhi + (xlo == 0);
      sr = 1;
      }
   else
      axh = xhi;
   ay = y < 0 ? (sr ^= 1, -y) : y;
   ad = d < 0 ? (sr ^= 1, -d) : d;
   ovt = SI32_MAX + sr;

/* Break x into 4 and y into 2 16-bit pieces */

   x3 = axh >> BU16;
   x2 = axh & L16;
   x1 = xlo >> BU16;
   x0 = xlo & L16;

   y1 = ay >> BU16;
   y0 = ay & L16;

/* Multiply (x3,x2,x1,x0) * (y1,y0) in base 2^16 and hold result in
*  three ui32 words (xy).  The product can not overflow.
*  Be careful to avoid overflow when adding partial products.  */

   p0 = x0*y0;
   p1 = x1*y0 + (p0>>BU16);
   p2 = x2*y0 + (p1>>BU16);
   p3 = x3*y0 + (p2>>BU16);
   p4 =         (p3>>BU16);

   q1 = x0*y1;
   q2 = x1*y1 + (q1>>BU16);
   q3 = x2*y1 + (q2>>BU16);
   q4 = x3*y1 + (q3>>BU16);

   p1 = (p1 & L16) + (q1 & L16);
   p2 = (p2 & L16) + (q2 & L16) + (p1>>BU16);
   p3 = (p3 & L16) + (q3 & L16) + (p2>>BU16);

   xy[JH] = p4 + q4 + (p3>>BU16);
   xy[JM] = (p2 & L16) + (p3 << BU16);
   xy[JL] = (p0 & L16) + (p1 << BU16);

/* Divide and round using vdivl.  By first scaling the divisor left
*  by the amount that the quotient would be scaled right, vdivl can
*  take care of the special case of zero scale where otherwise the
*  remainder has to be checked.  Be careful never to do a shift of
*  32, because it is a NOP on some machines (e.g.  Intel 486 and up).
*/

   if ((ds = ss - BU32) >= 0) {
      if (ds == 0) {          /* Shift is exactly 32 */
         dv[JM] = ad, dv[JL] = 0, ndv = 2;
         }
      else {                  /* Shift is > 32 */
         dv[JH] = ad>>(BU32-ds), dv[JM] = ad<<ds, dv[JL] = 0, ndv = 3;
         }
      }
   else if (ss == 0) {        /* Shift is exactly 0 */
      dv[JL] = ad, ndv = 1;
      }
   else {                     /* 0 < Shift < 32 */
      dv[JM] = ad>>(-ds), dv[JL] = ad<<ss, ndv = 2;
      }

#if BYTE_ORDRE > 0
   vdivl(xy, dv+3-ndv, xy, NULL, wk, 3, ndv, TRUE);
#else
   vdivl(xy, dv, xy, NULL, wk, 3, ndv, TRUE);
#endif

/* After dividing and rounding, if xy[JH] is not zero or xy[JM]
*  exceeds ovt, there has been an overflow.  This test specifically
*  accepts the largest negative number as valid, i.e.  it is the only
*  negative number that retains its sign when flipped by the block
*  above.  */

   if (xy[JH] || xy[JM] > ovt) {
      e64dac("dmrswjwd");
      xy[JM] = SI32_MAX, xy[JL] = UI32_MAX;
      }

/* Impose correct sign on the product */

   if (sr)
      rv.lo = -xy[JL], rv.hi = ~xy[JM] + (xy[JL] == 0);
   else
      rv.lo = xy[JL], rv.hi = xy[JM];

   return rv;

#endif

   } /* End dmrswjwd() */
