/* (c) Copyright 1992-2011, The Rockefeller University *21114* */
/* $Id: plot.c 30 2017-01-16 19:30:14Z  $ */
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
*  Rev, 06/19/92, ROZ - Add support for NSI graphic metafile           *
*  Rev, 11/25/96, GNR - Delete native NCUBE plotting support           *
*  ==>, 02/27/08, GNR - Last mod before committing to svn repository   *
*  Rev, 10/11/11, GNR - _NCG to _RKG, gmf.h now in glu.h, etc.         *
***********************************************************************/

#include "plots.h"
#include "glu.h"
#include "rocks.h"
#include "rkxtra.h"

void plot(float x,float y,int ipen) {

   if (_RKG.s.MFActive) {
#ifdef PAR
      if (mfbchk(MFD_SIZE))
#else
      mfbchk(MFD_SIZE);
#endif
         {
         if (ipen == PENUP) *_RKG.MFCurrPos++ = 'M';
         else               *_RKG.MFCurrPos++ = 'D';
         i2a(_RKG.MFCurrPos,(int)(x*1000.0),5,DECENCD);
         i2a(_RKG.MFCurrPos,(int)(y*1000.0),5,DECENCD);
         *_RKG.MFCurrPos++ = '\n';
         }
      }

   _RKG.xcurr = x; _RKG.ycurr = y;

   } /* End plot */
