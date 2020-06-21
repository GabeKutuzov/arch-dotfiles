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
*  Version 1, 06/09/92, ROZ                                            *
*  Rev, 07/15/93, GNR - Use mfbchk(), add switch for square case       *
*  Rev, 11/25/96, GNR - Delete native NCUBE plotting support           *
*  ==>, 02/27/08, GNR - Last mod before committing to svn repository   *
*  Rev, 10/11/11, GNR - _NCG to _RKG, gmf.h now in glu.h, etc.         *
***********************************************************************/

#include "plots.h"
#include "glu.h"
#include "rocks.h"
#include "rkxtra.h"

void rect(float x, float y, float width, float height, int krt) {

   if (_RKG.s.MFActive) {

      int isrect = (height != width);

      mfbchk(MFR_SIZE);

      if (isrect) *_RKG.MFCurrPos++ = 'R';
      else        *_RKG.MFCurrPos++ = 'Q';

      if (krt <= FILLED) *_RKG.MFCurrPos++ = 'F';
      else               *_RKG.MFCurrPos++ = 'O';

      i2a(_RKG.MFCurrPos,(int)(x*1000.0),5,DECENCD);
      i2a(_RKG.MFCurrPos,(int)(y*1000.0),5,DECENCD);
      i2a(_RKG.MFCurrPos,(int)(width*1000.0),5,DECENCD);

      if (isrect)
         i2a(_RKG.MFCurrPos,(int)(height*1000.0),5,DECENCD);

      *_RKG.MFCurrPos++ = '\n';
      }

} /* End rect() */