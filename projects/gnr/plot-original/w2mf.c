/* (c) Copyright 2015, The Rockefeller University *11115* */
/* $Id: w2mf.c 5 2008-03-13 20:32:23Z  $ */
/***********************************************************************
*                     ROCKS metafile plot library                      *
*                               w2mf()                                 *
*                                                                      *
*  This routine converts a floating-point width, 'w', into the fixed-  *
*  point compressed form described in the documentation and emits the  *
*  result to the metafile buffer.  There are four output cases:        *
*     00 + lcf frac bits      (Positive change <= 1 from previous w)   *
*     01 + lcf frac bits      (Negative change <= 1 from previous w)   *
*     10                      (No change from previous w)              *
*     11 + lci int bits + lcf frac bits  (New w value)                 *
*                                                                      *
*  Design note:  I decided not to give an error for negative argument  *
*  or argument out-of-range, but just mask the argument to prevent it  *
*  from modifying the case code, as the worst thing that can happen is *
*  that the error is revealed by an absurd drawing.                    *
************************************************************************
*  V2A, 03/28/15, GNR - New program                                    *
***********************************************************************/

#include "mfint.h"

void w2mf(float w) {

   Frame *pcf = _RKG.pcf;

   ui32 w32 = (ui32)roundf(_RKG.s.fcsc * w) & _RKG.s.xymask;
   si32 idw;
   if (pcf->kreset & ResetW) goto Emit11;
   if ((idw = (si32)w32 - (si32)pcf->cw) == 0) {
      mfbitpk(2,2); return; }
   if ((ui32)abs32(idw) <= _RKG.s.unitxy) {
      if (idw == _RKG.s.unitxy) idw = 0;
      mfbitpk(idw & _RKG.s.sfrmask, _RKG.s.lcf+2);
      goto SaveCurrW;
      }
Emit11:
   mfbitpk(w32 | 3<<_RKG.s.lct, _RKG.s.lct+2);
SaveCurrW:
   pcf->cw = w32;
   } /* End w2mf() */
