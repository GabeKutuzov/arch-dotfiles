/* (c) Copyright 1992-2011, The Rockefeller University *21114* */
/* $Id: line.c 20 2011-12-24 02:59:03Z  $ */
/***********************************************************************
*                               line()                                 *
*                                                                      *
*  SYNOPSIS:                                                           *
*     void line(float x1, float y1, float x2, float y2)                *
*                                                                      *
*  DESCRIPTION:                                                        *
*     This function draws a line from (x1,y1) to (x2,y2) in the        *
*     current color and thichness.                                     *
*                                                                      *
*  RETURN VALUES:                                                      *
*     None                                                             *
************************************************************************
*  Version 1, 06/09/92, ROZ                                            *
*  Rev, 07/15/93, GNR - Use mfbchk()                                   *
*  Rev, 11/25/96, GNR - Delete native NCUBE plotting support           *
*  ==>, 02/27/08, GNR - Last mod before committing to svn repository   *
*  Rev, 10/11/11, GNR - _NCG to _RKG, gmf.h now in glu.h, etc.         *
***********************************************************************/

#include "plots.h"
#include "glu.h"
#include "rocks.h"
#include "rkxtra.h"

void line(float x1, float y1, float x2, float y2) {

   if (_RKG.s.MFActive) {
      mfbchk(MFL_SIZE);
      *_RKG.MFCurrPos++ = 'L';
      i2a(_RKG.MFCurrPos,(int)(x1*1000.0),5,DECENCD);
      i2a(_RKG.MFCurrPos,(int)(y1*1000.0),5,DECENCD);
      i2a(_RKG.MFCurrPos,(int)(x2*1000.0),5,DECENCD);
      i2a(_RKG.MFCurrPos,(int)(y2*1000.0),5,DECENCD);
      *_RKG.MFCurrPos++ = '\n';
      }

   } /* End line() */
