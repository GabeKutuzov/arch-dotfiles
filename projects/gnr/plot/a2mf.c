/* (c) Copyright 2015, The Rockefeller University *11115* */
/* $Id: a2mf.c 5 2008-03-13 20:32:23Z  $ */
/***********************************************************************
*                     ROCKS metafile plot library                      *
*                               a2mf()                                 *
*                                                                      *
*  This routine converts a floating-point angle, 'a', given in degrees,*
*  into the fixed-point compressed form, given in cycles as described  *
*  in the documentation, and emits the result to the metafile buffer.  *
*  There are two output cases:                                         *
*     0 + 4 frac bits  (Accommodates 16 principal angles)              *
*     1 + sign + 15 frac bits (Accommodates angles with approx.        *
*                             1 arc-minute precision)                  *
************************************************************************
*  V2A, 03/28/15, GNR - New program                                    *
***********************************************************************/

#include "mfint.h"

void a2mf(float a) {

   ui32 a32 = (ui32)roundf(ANGRES * a);
   if ((a32 & ANGLOW) == 0)
      mfbitpk((a32 >> 11) & 15, 5);
   else
      mfbitpk(a32 & ANGMASK | (1 << 16), 17);
   } /* End a2mf() */
