/* (c) Copyright 1992-2011, The Rockefeller University *21114* */
/* $Id: circle.c 20 2011-12-24 02:59:03Z  $ */
/***********************************************************************
*                              circle()                                *
*                                                                      *
*  SYNOPSIS:                                                           *
*     void circle(float xc, float yc, float radius, int krt)           *
*                                                                      *
*  DESCRIPTION:                                                        *
*     This function draws a circle using the current color and line    *
*        thickness.                                                    *
*  Arguments:                                                          *
*     (xc,yc) are the coordinates of the center of the circle.         *
*     'radius' is the desired radius of the circle in inches.          *
*     'kf' is a fill control switch. A filled circle is drawn if       *
*        kf = 01 (FILLED). An open circle is drawn if kf => 00.        *
*                                                                      *
*  RETURN VALUES:                                                      *
*     None.                                                            *
************************************************************************
*  Version 2, 10/24/18, GMK                                            *
***********************************************************************/

#include "mfint.h"
#include "plots.h"

void circle(float xc, float yc, float radius, int kf) {
   
   if (!(_RKG.pcw->MFActive)) return;
   mfbchk(_RKG.s.mxlCirc);
   mfbitpk(OpCirc, Lop);
   mfbitpk(kf, 2);
   x2mf(xc);
   y2mf(yc);
   r2mf(radius);
}/* End circle() */