/* (c) Copyright 1992-2011, The Rockefeller University *21114* */
/* $Id: plot.c 20 2011-12-24 02:59:03Z  $ */
/***********************************************************************
*                               plot()                                 *
*                                                                      *
*  SYNOPSIS:                                                           *
*     void plot(float x,float y,int ipen)                              *
*                                                                      *
*  DESCRIPTION:                                                        *
*     This function can perform two things.  One is to draw a line     *
*     from the current location to a new location given by 'x','y',    *
*     if 'ipen' is PENDOWN.  The second is to move the current         *
*     position to a new location (x,y), if 'ipen'is PENUP.             *
*                                                                      *
*  RETURN VALUES:                                                      *
*     None                                                             *
************************************************************************
*  Version 2, 10/24/18, GMK                                            *
***********************************************************************/

#include "mfint.h"
#include "plots.h"
#include "plotdefs.h"

void plot(float x,float y,int ipen) {

   Frame *pcf = _RKG.pcf;

   if (ipen == PENUP){ 
      mfbchk(_RKG.s.mxlMove);
      mfbitpk(OpMove, Lop);
   }
   else if (ipen == PENDOWN){
      mfbchk(_RKG.s.mxlDraw);
      mfbitpk(OpDraw, Lop);
   }
   x2mf(x);
   y2mf(y);
   if (!(_RKG.pcw->MFActive)){
      pcf->cx = (si32)roundf(_RKG.s.fcsc * x) & _RKG.s.sxymask;
      pcf->cy = (si32)roundf(_RKG.s.fcsc * y) & _RKG.s.sxymask;
   }
} /* End plot() */