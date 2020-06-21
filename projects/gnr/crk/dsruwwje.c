/* (c) Copyright 2013, The Rockefeller University *11115* */
/* $Id: dsruwwje.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                               dsruwwje                               *
*                                                                      *
*  This function scales a 64-bit unsigned fixed-point value by a given *
*  shift amount (0 <= shift <= 32) to get a 96-bit dividend, divides   *
*  by a 64-bit unsigned divisor, and rounds by adding the high bit of  *
*  the remainder to the quotient.  A 32-bit quotient is returned,      *
*  with overflow checking on the result.                               *
*                                                                      *
*  Synopsis: ui32 dsruwwje(ui64 x, ui64 y, int s, int ec)              *
*                                                                      *
*  Arguments:                                                          *
*     x        64-bit numerator.                                       *
*     y        64-bit divisor.                                         *
*     s        left shift.                                             *
*     ec       error reporting code (e64act argument).                 *
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
*  V1A, 05/28/13, GNR - New routine                                    *
*  ==>, 05/28/13, GNR - Last date before committing to svn repository  *
*  Rev, 06/09/13, GNR - Deal with return from abexit in tests          *
***********************************************************************/

#include <stddef.h>
#include <math.h>
#include "sysdef.h"
#include "rkarith.h"

#define NX  3              /* Size of dividend in 32-bit words */
#define NY  2              /* Size of divisor in 32-bit words */

ui32 dsruwwje(ui64 x, ui64 y, int s, int ec) {

   union { ui64 xu64[3]; ui32 xu32[6]; } ux;
   ui32 wk[4*NX+2*NY+2];
   int  ixo;

   /* Check for shift out-of-range */
   if (s > 32 || s < 0) { abexitq(74); return 0; }

#if BYTE_ORDRE > 0         /* BIG-ENDIAN */
   /* Shift dividend into xs array */
   if (s == 32)
      ux.xu64[1] = x, ux.xu32[4] = 0, ixo = 2;
   else if (s == 0)
      ux.xu64[1] = x, ux.xu32[1] = 0, ixo = 1;
   else
      ux.xu64[0] = jsruw(x,64-s), ux.xu64[1] = jsluw(x,s), ixo = 1;
   /* Perform division and rounding */
   vdivl(ux.xu32+ixo, (ui32 *)&y, ux.xu32+1, NULL, wk, NX, NY, TRUE);
   /* Check for overflow into high-order */
   if (ux.xu32[1] | ux.xu32[2] ) {
      e64act("dsruwwje", ec);
      return UI32_MAX;
      }

   return ux.xu32[3];

#else                      /* LITTLE-ENDIAN */
   /* Shift dividend into xs array */
   if (s == 32)
      ux.xu64[1] = x, ux.xu32[1] = 0, ixo = 1;
   else if (s == 0)
      ux.xu64[1] = x, ux.xu32[4] = 0, ixo = 2;
   else
      ux.xu64[2] = jsruw(x,64-s), ux.xu64[1] = jsluw(x,s), ixo = 2;
   /* Perform division and rounding */
   vdivl(ux.xu32+ixo, (ui32 *)&y, ux.xu32+2, NULL, wk, NX, NY, TRUE);
   /* Check for overflow into high-order */
   if (ux.xu32[4] | ux.xu32[3] ) {
      e64act("dsruwwje", ec);
      return UI32_MAX;
      }

   return ux.xu32[2];
#endif

   } /* End dsruwwje() */
