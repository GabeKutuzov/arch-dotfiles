/* (c) Copyright 2005-2013, The Rockefeller University *11115* */
/* $Id: ui32powe.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                              ui32powe                                *
*                                                                      *
*    ROCKS Mathematics Library -- Routine to calculate a positive      *
*       integer power of an unsigned general fixed-point number        *
*                                                                      *
*     Synopsis:  ui32 ui32powe(ui32 val, int pow, int scale, int ec)   *
*                                                                      *
*     Arguments:                                                       *
*        val     Value that is to be raised to a power.                *
*        pow     The desired power.                                    *
*        scale   The number of fraction bits in val.                   *
*        ec      e64set error code for overflow error.                 *
*                                                                      *
*     Returns:   val**pow                                              *
*                                                                      *
*     Errors:    If the result overflows an unsigned 32-bit word,      *
*                calls e64act with error code 'ec'.  Action depends    *
*                on previous call to e64set (default: abexit 76)       *
*                                                                      *
*     Algorithm: Adds successive squares of 'val' selected by bits     *
*                that are set to 1 in 'pow'.  Maintains one extra      *
*                bit (if it will fit) for a little more accuracy.      *
*                                                                      *
************************************************************************
*  V1A, 10/04/05, GNR - New routine                                    *
*  ==>, 09/28/08, GNR - Last date before committing to svn repository  *
*  Rev, 10/25/13, GNR - Replace 0x80000000UL with UI32_SGN             *
***********************************************************************/

#include <math.h>
#include "rkarith.h"

ui32 ui32powe(ui32 val, int pow, int scale, int ec) {

   ui64 t;
   ui32 r,h;
   int  xs = (val < UI32_SGN && scale > 0);

   if (scale > BITSPERUI32 - 1) e64act("ui32powe", ec);

   /* Scale up one if value will still fit in a ui32 */
   val <<= xs, scale += xs;

   r = (ui32)1 << scale;
   h = r >> 1;

   while (1) {
      if (pow & 1) {
         t = jmuw(r, val);
         t = jaul(t, h);
         t = jsruw(t, scale);
         if (uwhi(t)) e64act("ui32powe", ec);
         r = uwlo(t); }
      if (!(pow >>= 1)) break;
      t = jmuw(val, val);
      t = jaul(t, h);
      t = jsruw(t, scale);
      if (uwhi(t)) e64act("ui32powe", ec);
      val = uwlo(t);
      }

   return (r >> xs);

   } /* End ui32powe() */
