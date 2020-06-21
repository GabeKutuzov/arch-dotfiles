/* (c) Copyright 2015, The Rockefeller University *11115* */
/* $Id: g2mf.c 5 2008-03-13 20:32:23Z  $ */
/***********************************************************************
*                     ROCKS metafile plot library                      *
*                               g2mf()                                 *
*                                                                      *
*  This routine converts a general floating-point number, 'g', into    *
*  the fixed-point compressed form described in the documentation and  *
*  emits the result to the metafile buffer.                            *
*  There are five output cases:                                        *
*     00       (The value is 0.0)                                      *
*     010      (The value is +1.0)                                     *
*     011      (The value is -1.0)                                     *
*     10 + sign + 16 frac bits                                         *
*     110 + sign + 8 integer + 16 frac bits                            *
*     111 + sign + 20 integer + 16 frac bits                           *
*                                                                      *
*  Note:  The last form expresses more accuracy than can be encoded    *
*  in an ordinary float, but allows a wide range of exponents or       *
*  possible future use with double-precision floats.                   *
************************************************************************
*  V2A, 03/29/15, GNR - New program                                    *
***********************************************************************/

#include "mfint.h"

void g2mf(float g) {

   float ag;
   ui32  gs16;
   if (g == 0.0)       { mfbitpk(0,2); return; }
   else if (g ==  1.0) { mfbitpk(2,3); return; }
   else if (g == -1.0) { mfbitpk(3,3); return; }
   ag = fabsf(g), gs16 = (ui32)(S16 * g);
   if (ag < 1.0)
      mfbitpk(gs16 & GMSK10 | GTYP10, 19);
   else if (ag < S8)
      mfbitpk(gs16 & GMSK110 | GTYP110, 28);
   else {
      mfbitpk((ui32)floorf(g) | GTYP111, 24);
      mfbitpk(gs16, 16);
      }
   } /* End g2mf() */
