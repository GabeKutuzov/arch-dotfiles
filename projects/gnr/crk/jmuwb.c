/* (c) Copyright 2009, The Rockefeller University *11115* */
/* $Id: jmuwb.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                                jmuwb                                 *
*                                                                      *
*  This function multiplies a 64-bit unsigned integer by a 32-bit      *
*  unsigned integer and returns the low-order 64 bits of the product   *
*  as the function value.  It also returns the high-order 32 bits of   *
*  the product via a pointer argument.  There are no error conditions. *
*  To keep a 96-bit product, the multiplication has to be carried out  *
*  in pieces even if the machine HAS_I64.                              *
*                                                                      *
*  Synopsis: ui64 jmuwb(ui64 x, ui32 y, ui32 *phi)                     *
*                                                                      *
*  Note:  This routine is mainly useful for converting the fraction    *
*  part of a fixed-point number to decimal.                            *
*                                                                      *
*  This routine is part of a basic 64-bit arithmetic package for which *
*  full C implementations are provided in order to get one started on  *
*  a new processor.  These versions can be reimplemented in Assembler  *
*  if greater speed is needed.                                         *
************************************************************************
*  V1A, 01/15/09, GNR - New routine based on msuwe()                   *
*  ==>, 01/15/09, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include <math.h>
#include "sysdef.h"
#include "rkarith.h"

ui64 jmuwb(ui64 x, ui32 y, ui32 *phi) {

#ifdef HAS_I64       /* System has I64 arithmetic */
#ifdef L32
#undef L32
#endif
#if LSIZE == 8
#define L32 0x00000000ffffffffUL
#else
#define L32 0x00000000ffffffffULL
#endif

   ui64 p0,p1;

/* Multiply x * y in base 2^32 to get a 96-bit product.
*  This product cannot overflow.  */

   p0 = (x & L32)*y;
   p1 = (x >> BITSPERUI32)*y;
   *phi = (ui32)((p1 + (p0 >> BITSPERUI32)) >> BITSPERUI32);
   return (p0 + (p1 << BITSPERUI32));

#else                /* Does not have I64 arithmetic */
#ifdef L16
#undef L16
#endif
#define L16 0x0000ffffUL

   ui64 rv;
   ui32 x0,x1,x2,x3,y0,y1;
   ui32 p0,p1,p2,p3,p4,q1,q2,q3,q4;
   ui32 xhi = uwhi(x);
   ui32 xlo = uwlo(x);

/* Break x into 4 and y into 2 16-bit pieces */

   x3 = xhi >> BITSPERSHORT;
   x2 = xhi & L16;
   x1 = xlo >> BITSPERSHORT;
   x0 = xlo & L16;

   y1 = y >> BITSPERSHORT;
   y0 = y & L16;

/* Multiply (x3,x2,x1,x0) * (y1,y0) in base 2^16 and hold partial
*  products in (p4,p3,p2,p1,p0) and (q4,q3,q2,q1), where p3-p0(hi)
*  are carries.  Sum the p's and q's to get results for return.
*  Be careful to avoid overflow when adding partial products.  */

   p0 = x0*y0;
   p1 = x1*y0 + (p0>>BITSPERSHORT);
   p2 = x2*y0 + (p1>>BITSPERSHORT);
   p3 = x3*y0 + (p2>>BITSPERSHORT);
   p4 =         (p3>>BITSPERSHORT);

   q1 = x0*y1;
   q2 = x1*y1 + (q1>>BITSPERSHORT);
   q3 = x2*y1 + (q2>>BITSPERSHORT);
   q4 = x3*y1 + (q3>>BITSPERSHORT);

   p1 = (p1 & L16) + (q1 & L16);
   p2 = (p2 & L16) + (q2 & L16) + (p1>>BITSPERSHORT);
   p3 = (p3 & L16) + (q3 & L16) + (p2>>BITSPERSHORT);

   *phi = p4 + q4 + (p3>>BITSPERSHORT);
   rv.lo = (p0 & L16) | (p1<<BITSPERSHORT);
   rv.hi = (p2 & L16) | (p3<<BITSPERSHORT);
   return rv;
#endif

   } /* End jmuwb() */
