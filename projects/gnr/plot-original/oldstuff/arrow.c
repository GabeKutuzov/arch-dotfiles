/* (c) Copyright 1992-2011, The Rockefeller University *21114* */
/***********************************************************************
*                               arrow()                                *
*                                                                      *
*  SYNOPSIS:                                                           *
*     void arrow(float x1,float y1,float x2,float y2,                  *
*        float barb, float angle)                                      *
*                                                                      *
*  DESCRIPTION:                                                        *
*     This function draws an arrow from (x1,y1) to (x2,y2) in the      *
*     current color and thickness with barbs at the (x2,y2) end.       *
*     The barbs of length 'barb' are drawn at an angle 'angle' away    *
*     from the central line.                                           *
*                                                                      *
*  CURRENT PLOT POSITION ON RETURN:                                    *
*     (x2,y2)                                                          *
*                                                                      *
*  RETURN VALUE:                                                       *
*     None.                                                            *
************************************************************************
*  Rev, 06/19/92, ROZ - Add support for NSI graphic metafile           *
*  Rev, 01/20/93, ROZ - Changed low level graphic functions to ROCKS   *
*                       plotting routines.                             *
*  Rev, 12/16/93,  LC - Current plot pos. now consistent with doc.     *
*  V2A, 12/29/98, GNR - Make binary metafile                           *
*  V2B, 05/25/02, GNR - Add 'angle' argument                           *
*  ==>, 02/29/08, GNR - Last mod before committing to svn repository   *
*  Rev, 10/10/11, GNR - Version from new plot revised to work as old   *
*  Rev, 10/11/11, GNR - _NCG to _RKG, gmf.h now in glu.h, etc.         *
***********************************************************************/

#include "glu.h"
#include "plots.h"

void arrow(float x1, float y1, float x2, float y2,
      float barb, float angle) {

   if (_RKG.s.MFActive) {
      double d2;
      float bpar,bper,rd,rsin,rcos,xb,yb;

      rcos = x2 - x1;
      rsin = y2 - y1;
      if ((d2 = rcos*rcos + rsin*rsin) < (MINARROW*MINARROW)) return;
      rd = 1.0/(float)sqrt(d2);
      rcos *= rd;
      rsin *= rd;

      /* Locate projection of barbs on center line */
      d2 = TORADS*angle;
      bpar = barb*(float)cos(d2);
      bper = barb*(float)sin(d2);
      xb = x2 - bpar*rcos;
      yb = y2 - bpar*rsin;
      rcos *= bper;
      rsin *= bper;

      /* Draw barbs */
      line(xb-rsin, yb+rcos, x2, y2);
      line(x2, y2, xb+rsin, yb-rcos);

      /* Draw arrow shaft, leaving (xc,yc) at (x2,y2) */
      line(x1, y1, x2, y2);

      /* Save new current position */
      _RKG.xcurr = x2;
      _RKG.ycurr = y2;

      } /* End if MFMode */

   } /* End arrow() */
