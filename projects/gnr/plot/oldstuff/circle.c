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
*     'krt' is a fill control switch. A filled circle is drawn if      *
*        krt = -1 (FILLED). An open circle is drawn if krt => 0.       *
*                                                                      *
*  RETURN VALUES:                                                      *
*     None.                                                            *
************************************************************************
*  Version 1, 04/03/92, ROZ                                            *
*  Rev, 07/15/93, GNR - Use mfbchk()                                   *
*  Rev, 11/25/96, GNR - Delete native NCUBE plotting support           *
*  ==>, 02/27/08, GNR - Last mod before committing to svn repository   *
*  Rev, 10/11/11, GNR - _NCG to _RKG, gmf.h now in glu.h, etc.         *
***********************************************************************/

#include "plots.h"
#include "glu.h"
#include "rocks.h"
#include "rkxtra.h"

void circle(float xc, float yc, float radius, int krt) {

   if (_RKG.s.MFActive) {
      mfbchk(MFC_SIZE);
      *_RKG.MFCurrPos++ = 'C';
      if (krt <= FILLED) {
         *_RKG.MFCurrPos++ = 'F';
         }
      else {
         *_RKG.MFCurrPos++ = 'O';
         }
      i2a(_RKG.MFCurrPos,(int)(xc*1000.0),5,DECENCD);
      i2a(_RKG.MFCurrPos,(int)(yc*1000.0),5,DECENCD);
      i2a(_RKG.MFCurrPos,(int)(radius*1000.0),5,DECENCD);
      *_RKG.MFCurrPos++ = '\n';
      }

   } /* End circle */
