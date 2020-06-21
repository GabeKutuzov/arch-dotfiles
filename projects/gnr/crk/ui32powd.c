/* (c) Copyright 2013, The Rockefeller University *11115* */
/* $Id: ui32powd.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                              ui32powd                                *
*                                                                      *
*    ROCKS Mathematics Library -- Routine to calculate a positive      *
*       integer power of an unsigned general fixed-point number        *
*  This is the version that uses the error code stored by e64dec().    *
*                                                                      *
*     Synopsis:  ui32 ui32powd(ui32 val, int pow, int scale)           *
*                                                                      *
*     Arguments:                                                       *
*        val     Value that is to be raised to a power.                *
*        pow     The desired power.                                    *
*        scale   The number of fraction bits in val.                   *
*                                                                      *
*     Returns:   val**pow                                              *
*                                                                      *
*     Errors:    If the result overflows an unsigned 32-bit word,      *
*                calls e64dac.  Action depends on previous calls to    *
*                e64set() and e64dec() (default: abexit 76)            *
*                                                                      *
*     Algorithm: Adds successive squares of 'val' selected by bits     *
*                that are set to 1 in 'pow'.  Maintains one extra      *
*                bit (if it will fit) for a little more accuracy.      *
*                                                                      *
************************************************************************
*  V1A, 10/23/13, GNR - New routine                                    *
*  ==>, 10/23/13, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include <math.h>
#include "rkarith.h"

ui32 ui32powd(ui32 val, int pow, int scale) {

   ui64 t;
   ui32 r,h;
   int  xs = (val < UI32_SGN && scale > 0);

   if (scale > BITSPERUI32 - 1) e64dac("ui32powd");

   /* Scale up one if value will still fit in a ui32 */
   val <<= xs, scale += xs;

   r = (ui32)1 << scale;
   h = r >> 1;

   while (1) {
      if (pow & 1) {
         t = jmuw(r, val);
         t = jaul(t, h);
         t = jsruw(t, scale);
         if (uwhi(t)) e64dac("ui32powd");
         r = uwlo(t); }
      if (!(pow >>= 1)) break;
      t = jmuw(val, val);
      t = jaul(t, h);
      t = jsruw(t, scale);
      if (uwhi(t)) e64dac("ui32powd");
      val = uwlo(t);
      }

   return (r >> xs);

   } /* End ui32powd() */
