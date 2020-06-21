/* (c) Copyright 1992-2011, The Rockefeller University *211154 */
/* $Id: ellips.c 20 2011-12-24 02:59:03Z  $ */
/***********************************************************************
*                              ellips()                                *
*                                                                      *
*  SYNOPSIS:                                                           *
*     void ellips(float xc, float yc, float hw, float hh, float angle, *
*     int krt)                                                         *
*                                                                      *
*  DESCRIPTION:                                                        *
*     This function draws an ellipse using the current color and       *
*        thickness.                                                    *
*     Arguments:                                                       *
*        (xc,yc) are the coordinates for the center of the ellipse.    *
*        (hw,hh) are the half width and the half height of the ellipse.*
*        'angle' is the counterclockwise rotation angle between the    *
*           positive X axis and the axis defined by 'hw' parameter.    *
*        'krt' is the fill control switch. If 'krt' = -1 (FILLED), a   *
*           filled ellipse is drawn, otherwise an open one.            *
*                                                                      *
*  RETURN VALUES:                                                      *
*     None.                                                            *
************************************************************************
*  Rev, 06/19/92, ROZ - Add support for NSI graphic metafile           *
*  Rev, 07/15/93, GNR - Use mfbchk()                                   *
*  Rev, 12/16/93,  LC - Current plot pos. now consistent with doc.     *
*  Rev, 11/25/96, GNR - Delete native NCUBE plotting support           *
*  ==>, 02/27/08, GNR - Last mod before committing to svn repository   *
*  Rev, 10/11/11, GNR - _NCG to _RKG, gmf.h now in glu.h, etc.         *
***********************************************************************/

#include "plots.h"
#include "glu.h"
#include "rocks.h"
#include "rkxtra.h"

void ellips(float xc, float yc, float hw, float hh, float angle,
      int krt) {

   if (_RKG.s.MFActive) {
      mfbchk(MFE_SIZE);
      *_RKG.MFCurrPos++ = 'E';
      if (krt <= FILLED) *_RKG.MFCurrPos++ = 'F';
      else               *_RKG.MFCurrPos++ = 'O';
      i2a(_RKG.MFCurrPos,(int)(xc*1000.0),5,DECENCD);
      i2a(_RKG.MFCurrPos,(int)(yc*1000.0),5,DECENCD);
      i2a(_RKG.MFCurrPos,(int)(hw*1000.0),5,DECENCD);
      i2a(_RKG.MFCurrPos,(int)(hh*1000.0),5,DECENCD);
      i2a(_RKG.MFCurrPos,(int)(angle*1000.0),7,DECENCD);
      *_RKG.MFCurrPos++ = '\n';
      }

   _RKG.xcurr=xc;
   _RKG.ycurr=yc;

   } /* End ellips() */
