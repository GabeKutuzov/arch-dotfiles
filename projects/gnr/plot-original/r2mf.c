/* (c) Copyright 2015, The Rockefeller University *11115* */
/* $Id: r2mf.c 5 2008-03-13 20:32:23Z  $ */
/***********************************************************************
*                     ROCKS metafile plot library                      *
*                               r2mf()                                 *
*                                                                      *
*  This routine converts a floating-point radius, 'r', into the fixed- *
*  point compressed form described in the documentation and emits the  *
*  result to the metafile buffer.  There are four output cases:        *
*     00 + lcf frac bits      (Positive change <= 1 from previous r)   *
*     01 + lcf frac bits      (Negative change <= 1 from previous r)   *
*     10                      (No change from previous r)              *
*     11 + lci int bits + lcf frac bits  (New r value)                 *
*                                                                      *
*  Design note:  I decided not to give an error for negative argument  *
*  or argument out-of-range, but just mask the argument to prevent it  *
*  from modifying the case code, as the worst thing that can happen is *
*  that the error is revealed by an absurd drawing.                    *
************************************************************************
*  V2A, 03/28/15, GNR - New program                                    *
***********************************************************************/

#include "mfint.h"

void r2mf(float r) {

   Frame *pcf = _RKG.pcf;

   ui32 r32 = (ui32)roundf(_RKG.s.fcsc * r) & _RKG.s.xymask;
   si32 idr;
   if (pcf->kreset & ResetR) goto Emit11;
   if ((idr = (si32)r32 - (si32)pcf->cr) == 0) {
      mfbitpk(2,2); return; }
   if ((ui32)abs32(idr) <= _RKG.s.unitxy) {
      if (idr == _RKG.s.unitxy) idr = 0;
      mfbitpk(idr & _RKG.s.sfrmask, _RKG.s.lcf+2);
      goto SaveCurrR;
      }
Emit11:
   mfbitpk(r32 | 3<<_RKG.s.lct, _RKG.s.lct+2);
SaveCurrR:
   pcf->cr = r32;
   } /* End r2mf() */
