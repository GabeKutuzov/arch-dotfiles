/* (c) Copyright 1992-2011, The Rockefeller University *21114* */
/* $Id: rect.c 20 2011-12-24 02:59:03Z  $ */
/***********************************************************************
*                               rect()                                 *
*                                                                      *
*  SYNOPSIS:                                                           *
*     void rect(float x, float y, float width, float height, int krt)  *
*                                                                      *
*  DESCRIPTION:                                                        *
*     rect() draws a rectangle in the current color with lower left-   *
*     hand corner at 'x' 'y', with width 'width' and height 'height'.  *
*     The rectangle is drawn, then retraced at a slighly larger size,  *
*     if _RKG.curretrac > 0.  The rectangle is filled in with solid    *
*     color if 'krt' < 0.                                              *
*                                                                      *
*  RETURN VALUES:                                                      *
*     None                                                             *
************************************************************************
*  Version 2, 10/24/18, GMK                                            *
***********************************************************************/

#include "mfint.h"
#include "plots.h"

void rect(float x, float y, float wd, float ht, int kf) {

   if (wd==ht){
      square(x,y,wd*ht,kf);
      return;
   }

   if (!(_RKG.pcw->MFActive)) return;
   mfbchk(_RKG.s.mxlRect);
   c2mf(hexch_to_int(OpRect));
   mfbitpk(kf, 2);
   x2mf(x);
   y2mf(y);
   w2mf(wd);
   h2mf(ht);
} /* End rect() */