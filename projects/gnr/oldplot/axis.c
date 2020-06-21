/* (c) Copyright 1990-2011, The Rockefeller University *21114* */
/* $Id: axis.c 30 2017-01-16 19:30:14Z  $ */
/***********************************************************************
*                               axis()                                 *
*                                                                      *
* SYNOPSIS:                                                            *
*     void axis(float x, float y, char *label, int n,                  *
*        float axlen, float angle, float firstv, float deltav)         *
* DESCRIPTION:                                                         *
*  This function draws a linear axis with labelled tick marks per      *
*  the original CALCOMP specification and the ROCKS plotting manual.   *
*                                                                      *
*  Arguments:                                                          *
*     x,y      Coordinates of starting point of the axis.              *
*     label    Label to be plotted along axis with height 0.14 inches. *
*     n        Number of characters in label.  If 'n' is positive, the *
*                 label is plotted counterclockwise from the axis (use *
*                 with y axis). If 'n' is negative, the label is plot- *
*                 ted clockwise from the axis (use with x axis).       *
*     axlen    Total length of the axis                                *
*     angle    Angle of the axis counterclockwise from +x.             *
*     firstv   Value of label at first tick mark.                      *
*     deltav   Increment added to 'firstv' for each tick mark.         *
* RETURN VALUES:                                                       *
*  None.                                                               *
************************************************************************
*  Rev, 06/19/90, JMS - Broken out of glu.c, use bcdout not sconvrt    *
*  Rev, 09/10/92, GNR - Revise to call basic ROCKS plotting routines   *
*  Rev, 09/10/92, ROZ - Add metafile, NOGDEV, SUN4 support             *
*  Rev, 01/21/93, ROZ - Changed it to generic form.                    *
*  ==>, 02/29/08, GNR - Last mod before committing to svn repository   *
*  Rev, 10/11/11, GNR - _NCG to _RKG, gmf.h now in glu.h, etc.         *
***********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <math.h>
#include "sysdef.h"
#include "rocks.h"
#include "rkxtra.h"
#include "plots.h"
#include "glu.h"

void axis(float x, float y, char *label, int n, float axlen,
         float angle, float firstv, float deltav) {

   float dx,dy;
   float rx1,rx2,ry1,ry2;     /* Rotated x,y coords of tick marks */
   float sn;                  /* Sign of n times tick height */
   float tval = firstv;       /* Value at current tick mark */
   float tx,ty;               /* Temp coords for labels */
   float ux;                  /* Unrotated x coord of tick mark */
   int nn;                    /* Number of characters to plot */
   char printme[16];

   /* Skip whole thing if no plots active */

   if (!_RKG.s.MFActive) return;

   dx = cos(TORADS*angle);
   dy = sin(TORADS*angle);

   /* Draw the axis */

   line(x,y,x+axlen*dx,y+axlen*dy);

   /* Label the axis */

   if (n) {
      nn = abs(n);
      tx = 0.5*(axlen - ASPECT*LABELHT*(float)nn);
      ty = (n > 0) ? LABELOFF : -(LABELOFF + LABELHT);
      symbol(x+tx*dx-ty*dy,y+ty*dx+tx*dy,LABELHT,label,angle,nn);
      }

   /* Draw and label the tick marks */

   sn = SCALEHT; if (n < 0) sn = -sn;
   rx1 = x; rx2 = x - sn*dy;
   ry1 = y; ry2 = y + sn*dx;
   ty = (n > 0) ? VALUEOFF : -(VALUEOFF + VALUEHT);
   for (ux=0.0; ux<=axlen;
         rx1+=dx,rx2+=dx,ry1+=dy,ry2+=dy,tval+=deltav,ux+=1.0) {
      line(rx1,ry1,rx2,ry2);
      bcdout(3*RK_D+RK_SNGL+RK_IORF+RK_LFTJ+15,printme,tval);
      nn = RK.length + 1;
      tx = ux - 0.5*ASPECT*VALUEHT*(float)(nn);
      symbol(x+tx*dx-ty*dy,y+ty*dx+tx*dy,VALUEHT,printme,angle,nn);
      }
   } /* End axis */
