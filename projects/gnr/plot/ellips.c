/* (c) Copyright 1992-2011, The Rockefeller University *211154 */
/* $Id: ellips.c 20 2011-12-24 02:59:03Z  $ */
/***********************************************************************
*                              ellips()                                *
*                                                                      *
*  SYNOPSIS:                                                           *
*     void ellips(float xc, float yc, float hw, float hh, float angle, *
*     int kf)                                                          *
*                                                                      *
*  DESCRIPTION:                                                        *
*     This function draws an ellipse using the current color and       *
*        thickness.                                                    *
*     Arguments:                                                       *
*        (xc,yc) are the coordinates for the center of the ellipse.    *
*        (hw,hh) are the half width and the half height of the ellipse.*
*        'angle' is the counterclockwise rotation angle between the    *
*           positive X axis and the axis defined by 'hw' parameter.    *
*        'kf' is the fill control switch. If 'kf' = -1 (FILLED), a     *
*           filled ellipse is drawn, otherwise an open one.            *
*                                                                      *
*  RETURN VALUES:                                                      *
*     None.                                                            *
************************************************************************
*  Version 2, 10/28/18, GMK                                            *
***********************************************************************/

#include "mfint.h"
#include "plots.h"

void ellips(float xc, float yc, float hw, float hh, float angle,
      int kf) {

   if (!(_RKG.pcw->MFActive)) return;
   mfbchk(_RKG.s.mxlEllps);
   mfbitpk(OpEllps, Lop);
   mfbitpk(kf, 2);
   x2mf(xc);
   y2mf(yc);
   w2mf(hw);
   h2mf(hh);
   a2mf(angle);    
} /* End ellips() */