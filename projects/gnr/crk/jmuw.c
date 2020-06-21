/* (c) Copyright 1999-2008, The Rockefeller University *11115* */
/* $Id: jmuw.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                                jmuw                                  *
*                                                                      *
*  This function multiplies two 32-bit unsigned integers and returns   *
*  an unsigned 64-bit product.  Overflow is not possible.              *
*                                                                      *
*  Synopsis: ui64 jmuw(ui32 x, ui32 y)                                 *
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
***********************************************************************/

#include "sysdef.h"
#include "rkarith.h"

#ifdef HAS_I64
#error Do not compile jmuw.c with HAS_I64, is a macro
#endif 

ui64 jmuw(ui32 x, ui32 y) {

   ui64 rv;

/* Multiply (A*(2**16) + B) by (C*(2**16) + D),
*  where A, B, C, and D are 16-bit quantities.
*  High 32 bits of result =
*        A*C + <high order 16 bits of (A*D + B*C)>
*  Low 32 bits of result =
*        B*D + <low order 16 bits of (A*D + B*C)>
*  Unlike the signed case, (A*D + B*C) can overflow
*  beyond 32 bits, so watch out for that case.  */

   ui32 a = x >> 16;
   ui32 b = x & 0x0000ffffUL;
   ui32 c = y >> 16;
   ui32 d = y & 0x0000ffffUL;

   /* Calc pieces needed for carry checking */
   ui32 ad = a * d;
   ui32 bc = b * c;
   ui32 bd = b * d;
   ui32 xx = ad + bc;
   ui32 xl = xx << 16;

   rv.lo = bd + xl;
   /* (The cast of (~ad<bc) is needed on 16-bit machines.) */
   rv.hi = (a * c) + ((ui32)(~ad < bc) << 16) +
      (xx >> 16) + (~xl < bd);

   return rv;

   } /* End jmuw() */
