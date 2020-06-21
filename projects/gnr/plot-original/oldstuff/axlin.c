/* (c) Copyright 1993-2014, The Rockefeller University *21114* */
/* $Id: axlin.c 20 2011-12-24 02:59:03Z  $ */
/***********************************************************************
*                               axlin()                                *
*                                                                      *
*  SYNOPSIS:                                                           *
*     void axlin(float x, float y, const char *label, float axlen,     *
*        float angle, float firstt, float deltat, float firstv,        *
*        float deltav, float height, int ktk, int nd, int nmti)        *
*                                                                      *
*  DESCRIPTION:                                                        *
*     This function draws a linear axis with labelled tick marks at    *
*     specified intervals.  The function also plots a centered label   *
*     along the axis as a whole.                                       *
*                                                                      *
*  ARGUMENTS:                                                          *
*     x,y     Coordinates of starting point of the axis.               *
*     label   Label to be plotted along axis.  The height of the       *
*                lettering is 1.33 times the 'height' parameter.       *
*                If 'label' is NULL or points to an empty string       *
*                or 'height' is <= 0, the label is not plotted.        *
*     axlen   Total length of the axis.  If <= 0, nothing is plotted.  *
*     angle   Angle of the axis counterclockwise from +x.              *
*     firstt  Offset of first tick mark from origin of axis.           *
*     deltat  Offset of each successive major tick mark.  If <= 0,     *
*                no tick marks or tick values are plotted.             *
*     firstv  Value of label at first tick mark.                       *
*     deltav  Increment added to 'firstv' for each major tick mark.    *
*                If 0, no tick values are plotted.                     *
*     height  Height of tick mark lettering.  If <= 0, tick values     *
*                and the axis label are not plotted.                   *
*     ktk     The sum of any desired codes from the following list:    *
*             AX_TKCCW  (=1)  Plot tick marks, tick values, and axis   *
*                label counterclockwise from the axis (appropriate     *
*                for the 'y' axis of a typical plot); default: plot    *
*                tick marks and labels clockwise from the axis         *
*                (appropriate for the 'x' axis of a typical plot).     *
*             AX_TKEFMT (=2)  Plot tick values in exponential format;  *
*                default: use decimal format unless numerical value    *
*                would require more than 12 digits to express.         *
*     nd      Number of decimals in the tick mark labels.              *
*                Use -1 to indicate that values should be plotted as   *
*                integers, with no decimal point.                      *
*     nmti    Number of minor tick intervals (one more than the num-   *
*                ber of tick marks) to be plotted between labelled     *
*                tick marks.  If 0, no minor tick marks are plotted.   *
*                                                                      *
*  CURRENT PLOT POSITION ON RETURN:                                    *
*     Undefined.                                                       *
*                                                                      *
*  RETURN VALUE:                                                       *
*     None.                                                            *
************************************************************************
*  Version 1, 01/22/93, ROZ - Based on axis function.                  *
*  Rev, 09/20/93, GNR - Add height parameter omitted by ROZ.           *
*  V2A, 12/29/98, GNR - Make binary metafile, eliminate geomdefs.h     *
*  Rev, 06/23/02, GNR - Remove n, add ktk and nmti                     *
*  Rev, 08/29/14, GNR - Add 'const' to pointer args                    *
*  Rev, 08/17/16, GNR - Avoid minor ticks off end of axis              *
***********************************************************************/

#include "glu.h"
#include "rocks.h"
#include "rkxtra.h"
#include "plots.h"

void axlin(float x, float y, const char *label, float axlen,
      float angle, float firstt, float deltat, float firstv,
      float deltav, float height, int ktk, int nd, int nmti) {

   if (_RKG.s.MFMode && axlen > 0.0) {
      double ra = TORADS*angle;
      double dtv;                /* Tick value increment */
      double tval;               /* Value at current tick mark */
      float axl2;                /* Axis length squared */
      float dx = cos(ra);
      float dy = sin(ra);
      float dxt,dyt;             /* Delta x,y of major ticks */
      float dxm,dym;             /* Delta x,y of minor ticks */
      float loff = 0.0;          /* Axis label offset */
      float oxm,oym;             /* Rotated length of minor tick */
      float rxt1,rxt2,ryt1,ryt2; /* Rotated x,y of major ticks */
      float stk,stv;             /* Tick and tick value directions */
      float tx,ty;               /* Temp coords for labels */
      float ux;                  /* Unrotated x coord of tick mark */
      float wbcd;                /* Width of one character */
      long  kbcd;                /* Mode code for bcdout */
      int   nl;                  /* Number of characters to plot */
      char  printme[MXTVLEN];    /* Tick value as string */

/* Draw the axis */

      line(x,y,x+axlen*dx,y+axlen*dy);

/* Draw and label the tick marks.
*  (We figure there is less overhead in having a loop with a
*  test inside for drawing value labels than in having separate
*  loops for ticks and value labels.  Also, it is very unlikely
*  that firstt > axlen, so we go through the initialization and
*  let the 'for' loop test kill the plotting in this case.  The
*  minor tick mark positions are restarted from each major tick
*  mark position to assure that the two kinds of ticks keep in
*  step.  */

      if (ktk & AX_TKCCW)
         stk = 1.0, stv = 0.0;
      else
         stk = stv = -1.0;
      if (deltat > 0.0) {
         axl2 = axlen*axlen;
         loff = stk*(MAJORHT+VALUGARD);
         ty   = stk*MAJORHT;
         rxt1 = x + firstt*dx;
         rxt2 = rxt1 - ty*dy;
         ryt1 = y + firstt*dy;
         ryt2 = ryt1 + ty*dx;
         dxt  = deltat*dx;
         dyt  = deltat*dy;
         kbcd = 0;
         if (deltav != 0.0 && height > 0.0) {
            /* Prepare for tick labels */
            dtv  = (double)deltav;
            kbcd = (nd+1)*RK_D + (ktk&AX_TKEFMT ? RK_EFMT : RK_IORF) +
               (RK_SNGL+RK_LFTJ+RK_UFLW+MXTVLEN-1);
            wbcd = 0.5*ASPECT*height;
            ty = loff + stv*height;
            loff += stk*height;
            }
         if (nmti > 1) {
            /* Prepare for minor ticks */
            tx  = stk*MINORHT;
            oxm = tx*dx;
            oym = tx*dy;
            tx  = 1.0/(float)nmti;
            dxm = tx*dxt;
            dym = tx*dyt;
            }
         for (tval=(double)firstv,ux=firstt; ux<=axlen;
               rxt1+=dxt,rxt2+=dxt,ryt1+=dyt,ryt2+=dyt,ux+=deltat) {
            line(rxt1,ryt1,rxt2,ryt2);    /* Plot major tick mark */
            if (kbcd) {                   /* Plot tick value */
               bcdout(kbcd,printme,tval);
               nl = RK.length + 1;
               tx = ux - wbcd*(float)(nl);
               symbol(x+tx*dx-ty*dy,y+ty*dx+tx*dy,height,
                  printme,angle,nl);
               tval += dtv;
               }
            if (nmti > 1) {               /* Plot minor tick marks */
               float rxm = rxt1, rym = ryt1;
               for (nl=1; nl<nmti; nl++) {
                  float drxm,drym;
                  rxm += dxm, rym += dym;
                  drxm = rxm - x, drym = rym - y;
                  if (drxm*drxm + drym*drym > axl2) break;
                  line(rxm,rym,rxm-oym,rym+oxm);
                  }
               }
            } /* End major tick loop */
         }

/* Label the axis */

      if (label && height > 0.0) {
         if ((nl = strlen(label)) > 0) {
            float lblh = AXLBLH*height;
            tx = 0.5*(axlen - ASPECT*lblh*(float)nl);
            ty = loff + (stk*LABELOFF + stv) * lblh;
            symbol(x+tx*dx-ty*dy,y+ty*dx+tx*dy,lblh,label,angle,nl);
            }
         }

      } /* End if MFMode */

   } /* End axlin() */

