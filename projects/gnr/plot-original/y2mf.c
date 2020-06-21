/* (c) Copyright 2015, The Rockefeller University *11115* */
/* $Id: y2mf.c 5 2008-03-13 20:32:23Z  $ */
/***********************************************************************
*                     ROCKS metafile plot library                      *
*                               y2mf()                                 *
*                                                                      *
*  This routine converts a floating-point y coordinate into the fixed- *
*  point compressed form described in the documentation and emits the  *
*  result to the metafile buffer.  There are four output cases:        *
*     00 + lcf frac bits      (Positive change <= 1 from previous y)   *
*     01 + lcf frac bits      (Negative change <= 1 from previous y)   *
*     10                      (No change from previous y)              *
*     11 + sign bit + lci int bits + lcf frac bits  (New y value)      *
*                                                                      *
*  Design note:  I decided not to give an error for argument out-of-   *
*  range, but just mask the argument to prevent it from modifying the  *
*  case code, as the worst thing that can happen is that the error is  *
*  revealed by an absurd drawing.                                      *
************************************************************************
*  V2A, 03/28/15, GNR - New program                                    *
***********************************************************************/

#include "mfint.h"

void y2mf(float y) {

   Frame *pcf = _RKG.pcf;

   si32 idy,y32 = (si32)roundf(_RKG.s.fcsc * y) & _RKG.s.sxymask;
   if (pcf->kreset & ResetY) { pcf->kreset &= ~ResetY; goto Emit11; }
   if ((idy = y32 - pcf->cy) == 0) { mfbitpk(2,2); return; }
   if ((ui32)abs32(idy) <= _RKG.s.unitxy) {
      if (idy == _RKG.s.unitxy) idy = 0;
      mfbitpk(idy & _RKG.s.sfrmask, _RKG.s.lcf+2);
      goto SaveCurrY;
      }
Emit11:
   mfbitpk(y32 | 3<<(_RKG.s.lct+1), _RKG.s.lct+3);
SaveCurrY:
   pcf->cy = y32;
   } /* End y2mf() */
