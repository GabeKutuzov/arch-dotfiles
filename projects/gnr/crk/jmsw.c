/* (c) Copyright 1999-2014, The Rockefeller University *11115* */
/* $Id: jmsw.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                                jmsw                                  *
*                                                                      *
*  This function multiplies two 32-bit signed integers and returns     *
*  a signed 64-bit product.  Overflow is not possible.                 *
*                                                                      *
*  Synopsis: si64 jmsw(si32 x, si32 y)                                 *
*                                                                      *
*  N.B. Products involving negative multiplicands give correct results *
*  if both arguments are left extended with the sign bit to the length *
*  of the product and then multiplied as if unsigned.  However, it was *
*  deemed that with the algorithm used here and elsewhere to perform   *
*  64-bit multiplications on 32-bit hardware, the extra work to sign-  *
*  extend and include an extra word in the multiplication is greater   *
*  than the work to multiply the absolute values and apply the sign at *
*  the end.                                                            *
*                                                                      *
*  This routine is part of a basic 64-bit arithmetic package for which *
*  full C implementations are provided in order to get one started on  *
*  a new processor.  These routines or the combined routines actually  *
*  used in applications can be reimplemented in Assembler if greater   *
*  speed is desired.                                                   *
*                                                                      *
************************************************************************
*  V1A, 06/20/99, GNR - New routine                                    *
*  ==>, 03/12/06, GNR - Last date before committing to svn repository  *
*  Rev, 09/15/14, GNR - Add comment about negative multiplicands       *
***********************************************************************/

#include <math.h>
#include <stdlib.h>
#include "sysdef.h"
#include "rkarith.h"

#ifdef HAS_I64
#error Do not compile jmsw.c with HAS_I64, is a macro
#endif 

si64 jmsw(si32 x, si32 y) {

   si64 rv;
   ui32 ax = abs32(x);
   ui32 ay = abs32(y);

/* Multiply (A*(2**16) + B) by (C*(2**16) + D),
*  where A and C are 15-bit quantities, B and D are 16-bit.
*  High 32 bits of result =
*        A*C + <high order 16 bits of (A*D + B*C)>
*  Low 32 bits of result =
*        B*D + <low order 16 bits of (A*D + B*C)>
*  Note that (A*D + B*C) cannot overflow beyond 32 bits
*        because max value is 2**32 - 2**17 - 2**16 + 2. */

   ui32 a = ax >> 16;
   ui32 b = ax & 0x0000ffffUL;
   ui32 c = ay >> 16;
   ui32 d = ay & 0x0000ffffUL;
   ui32 xx = b * c + a * d;

/* Break up low-order sum for carry check */

   ax = b * d;
   ay = xx << 16;
   rv.lo = ax + ay;
   rv.hi = (a * c) + (xx >> 16) + (~ax < ay);

/* Impose correct sign on the product */

   if ((x ^ y) < 0) {
      rv.lo = -rv.lo;
      rv.hi = ~rv.hi + (rv.lo == 0);
      }

   return rv;

   } /* End jmsw() */
