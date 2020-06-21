/* (c) Copyright 2015, The Rockefeller University *11115* */
/* $Id: xp2mf.c 5 2008-03-13 20:32:23Z  $ */
/***********************************************************************
*                     ROCKS metafile plot library                      *
*                               xp2mf()                                *
*                                                                      *
*  This routine converts a floating-point x coordinate into the fixed- *
*  point compressed form described in the documentation and emits the  *
*  result to the metafile buffer.  There are four output cases:        *
*     00 + lcf frac bits      (Positive change <= 1 from previous x)   *
*     01 + lcf frac bits      (Negative change <= 1 from previous x)   *
*     10                      (No change from previous x)              *
*     11 + sign bit + lci int bits + lcf frac bits  (New x value)      *
*                                                                      *
*  This is the version that is used to implement the notation x' in    *
*  the documentation--it is the same as x2mf() except the current      *
*  position is not updated.  (This is used, for example, when drawing  *
*  an arc so the center remains as the current drawing position after  *
*  the end of the arc is given as another x,y coordinate pair.)        *
************************************************************************
*  V2A, 05/28/15, GNR - New program                                    *
***********************************************************************/

#include "mfint.h"

void xp2mf(float x) {

   Frame *pcf = _RKG.pcf;

   si32 idx,x32 = (si32)roundf(_RKG.s.fcsc * x) & _RKG.s.sxymask;
   if ((idx = x32 - pcf->cx) == 0) { mfbitpk(2,2); return; }
   if ((ui32)abs32(idx) <= _RKG.s.unitxy) {
      if (idx == _RKG.s.unitxy) idx = 0;
      mfbitpk(idx & _RKG.s.sfrmask, _RKG.s.lcf+2);
      return;
      }
   mfbitpk(x32 | 3<<(_RKG.s.lct+1), _RKG.s.lct+3);
   } /* End xp2mf() */
