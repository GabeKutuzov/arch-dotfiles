/* (c) Copyright 2015, The Rockefeller University *11115* */
/* $Id: h2mf.c 5 2008-03-13 20:32:23Z  $ */
/***********************************************************************
*                     ROCKS metafile plot library                      *
*                               h2mf()                                 *
*                                                                      *
*  This routine converts a floating-point height, 'h', into the fixed- *
*  point compressed form described in the documentation and emits the  *
*  result to the metafile buffer.  There are four output cases:        *
*     00 + lcf frac bits      (Positive change < 1= from previous h)   *
*     01 + lcf frac bits      (Negative change < 1= from previous h)   *
*     10                      (No change from previous h)              *
*     11 + lci int bits + lcf frac bits  (New h value)                 *
*                                                                      *
*  Design note:  I decided not to give an error for negative argument  *
*  or argument out-of-range, but just mask the argument to prevent it  *
*  from modifying the case code, as the worst thing that can happen is *
*  that the error is revealed by an absurd drawing.                    *
************************************************************************
*  V2A, 03/28/15, GNR - New program                                    *
***********************************************************************/

#include "mfint.h"

void h2mf(float h) {

   Frame *pcf = _RKG.pcf;

   ui32 h32 = (ui32)roundf(_RKG.s.fcsc * h) & _RKG.s.xymask;
   si32 idh;
   if (pcf->kreset & ResetH) goto Emit11;
   if ((idh = (si32)h32 - (si32)pcf->ch) == 0) {
      mfbitpk(2,2); return; }
   if ((ui32)abs32(idh) <= _RKG.s.unitxy) {
      if (idh == _RKG.s.unitxy) idh = 0;
      mfbitpk(idh & _RKG.s.sfrmask, _RKG.s.lcf+2);
      goto SaveCurrH;
      }
Emit11:
   mfbitpk(h32 | 3<<_RKG.s.lct, _RKG.s.lct+2);
SaveCurrH:
   pcf->ch = h32;
   } /* End h2mf() */
