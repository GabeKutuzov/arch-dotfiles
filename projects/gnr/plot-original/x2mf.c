/* (c) Copyright 2015, The Rockefeller University *11115* */
/* $Id: x2mf.c 5 2008-03-13 20:32:23Z  $ */
/***********************************************************************
*                     ROCKS metafile plot library                      *
*                               x2mf()                                 *
*                                                                      *
*  This routine converts a floating-point x coordinate into the fixed- *
*  point compressed form described in the documentation and emits the  *
*  result to the metafile buffer.  There are four output cases:        *
*     00 + lcf frac bits      (Positive change <= 1 from previous x)   *
*     01 + lcf frac bits      (Negative change <= 1 from previous x)   *
*     10                      (No change from previous x)              *
*     11 + sign bit + lci int bits + lcf frac bits  (New x value)      *
*                                                                      *
*  Design note:  I decided not to give an error for argument out-of-   *
*  range, but just mask the argument to prevent it from modifying the  *
*  case code, as the worst thing that can happen is that the error is  *
*  revealed by an absurd drawing.                                      *
************************************************************************
*  V2A, 03/28/15, GNR - New program                                    *
***********************************************************************/

#include "mfint.h"

void x2mf(float x) {

   Frame *pcf = _RKG.pcf;

   si32 idx,x32 = (si32)roundf(_RKG.s.fcsc * x) & _RKG.s.sxymask;
   if (pcf->kreset & ResetX) { pcf->kreset &= ~ResetX; goto Emit11; }
   if ((idx = x32 - pcf->cx) == 0) { mfbitpk(2,2); return; }
   if ((ui32)abs32(idx) <= _RKG.s.unitxy) {
      if (idx == _RKG.s.unitxy) idx = 0;
      mfbitpk(idx & _RKG.s.sfrmask, _RKG.s.lcf+2);
      goto SaveCurrX;
      }
Emit11:
   mfbitpk(x32 | 3<<(_RKG.s.lct+1), _RKG.s.lct+3);
SaveCurrX:
   pcf->cx = x32;
   } /* End x2mf() */
