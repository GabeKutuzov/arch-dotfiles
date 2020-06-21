/* (c) Copyright 1993-2014, The Rockefeller University *21114* */
/* $Id: axlog.c 20 2011-12-24 02:59:03Z  $ */
/***********************************************************************
*                               axlog()                                *
*                                                                      *
*  SYNOPSIS:                                                           *
*     void axlog(float x, float y, const char *label, float axlen,     *
*          float angle, float firstt, float deltat, float firstv,      *
*          float vmult, float height, int ktk, int nd, int nmti)       *
*                                                                      *
*  DESCRIPTION:                                                        *
*     This function draws a logarithmic axis with labelled tick marks  *
*     at specified intervals.  The function also plots a centered      *
*     label along the axis as a whole.                                 *
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
*     vmult   Value by which 'firstv' is multiplied at each successive *
*             tick mark.  The base of the logarithm scale.  If <0, no  *
*             tick marks are plotted.                                  *
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
*             AX_TKEXP  (=4)  Tick values are plotted as base value    *
*                ('vmult') to some power.  In this case, 'firstv'      *
*                should be the power indicated on the first tick,      *
*                and 1 will be added (subtracted if vmult < 1) to this *
*                exponent for each successive tick.  Code AX_TKEFMT is *
*                ignored.                                              *
*     nd      Number of decimals in the tick mark labels.              *
*                Use -1 to indicate that values should be plotted as   *
*                integers, with no decimal point.                      *
*     nmti    Number of minor tick intervals (one more than the num-   *
*                ber of tick marks, typically one less than the base)  *
*                to be plotted between labelled tick marks.  If 0, no  *
*                minor tick marks are plotted.                         *
*                                                                      *
*  CURRENT PLOT POSITION ON RETURN:                                    *
*     Undefined.                                                       *
*                                                                      *
*  RETURN VALUE:                                                       *
*     None.                                                            *
************************************************************************
*  Version 1, 01/25/93, ROZ - Based on axis function.                  *
*  V2A, 12/29/98, GNR - Make binary metafile, eliminate geomdefs.h     *
*  Rev, 06/29/02, GNR - Remove n, add firstt, vmult, ktk, and nmti     *
*  ==>, 02/29/08, GNR - Last mod before committing to svn repository   *
*  Rev, 10/11/11, GNR - _NCG to _RKG, gmf.h now in glu.h, etc.         *
*  Rev, 08/29/14, GNR - Add 'const' to pointer args                    *
***********************************************************************/

#include "mfint.h"                 /* (Includes math.h etc.) */
#include "rocks.h"
#include "rkxtra.h"
#include "plots.h"

void axlog(float x, float y, const char *label, float axlen,
      float angle, float firstt, float deltat, float firstv,
      float vmult, float height, int ktk, int nd, int nmti) {

   if (_RKG.pcw->MFActive && axlen > 0.0) {
      double ra = TORADS*angle;
      float *dxm = NULL,*dym;    /* Rotated offsets of minor ticks */
      float dx = cos(ra);
      float dy = sin(ra);
      float dxt,dyt;             /* Delta x,y of major ticks */
      float lblh;                /* Axis label height */
      float loff = 0.0;          /* Axis label offset */
      float oxm,oym;             /* Rotated length of minor tick */
      float rxt1,rxt2,ryt1,ryt2; /* Rotated x,y of major ticks */
      float rxm,rym;             /* Rotated x,y of minor ticks */
      float stk,stv;             /* Tick and tick value directions */
      float tx,ty;               /* Temp coords for labels */
      float tval;                /* Value at tick */
      float ux;                  /* Unrotated x coord of tick mark */
      float wbcd;                /* Width of one character */
      int   nl;                  /* Number of characters to plot */
      char  printme[MXTVLEN];    /* Tick value as string */
      char  tickfmt[16];         /* Format for converting tick value */

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
         loff = stk*(MAJORHT+VALUGARD);
         ty   = stk*MAJORHT;
         rxt1 = x + firstt*dx;
         rxt2 = rxt1 - ty*dy;
         ryt1 = y + firstt*dy;
         ryt2 = ryt1 + ty*dx;
         dxt  = deltat*dx;
         dyt  = deltat*dy;
         if (vmult >= 0.0) {
            if (nmti > 1) {         /* Prepare for minor ticks */
               double dti = ((double)vmult - 1.0)/(double)nmti;
               double dtm = (double)deltat/log((double)vmult);
               double tmv;          /* Value at minor tick mark */
               tx  = stk*MINORHT;
               oxm = tx*dx;
               oym = tx*dy;
               dxm = mallocv((nmti+nmti)*sizeof(float), "Minor ticks");
               dym = dxm + nmti;
               for (nl=1,tmv=1.0; nl<nmti; nl++) {
                  ux = (float)(log(tmv+=dti)*dtm);
                  dxm[nl] = ux*dxt;
                  dym[nl] = ux*dyt;
                  }
               }
            if (height > 0.0) {     /* Prepare for tick labels */
               ty = loff + stv*height;
               loff += stk*height;
               wbcd = 0.5*ASPECT*height;
               nl = MXTVLEN;
               if (++nd < 0) nd = 0;
               if (ktk & AX_TKEXP) {         /* Base to exponent */
                  int iavm = abs((int)vmult);
                  sconvrt(tickfmt, "(2H('J0I3,10H\x12',J0F3.0))",
                     &iavm, NULL);
                  tx = SUPEROFF*height;
                  ty += stv*tx;
                  loff += stk*tx;
                  vmult = (vmult >= 1.0) ? 1.0 : -1.0;
                  }
               else if (ktk & AX_TKEFMT) {   /* Exponential */
                  sconvrt(tickfmt, "(2H(EJ0I3,H.J0I3,H))",
                     &nl, &nd, NULL);
                  }
               else {                        /* Fixed point default */
                  sconvrt(tickfmt, "(3H(VFJ0I3,H.J0I3,H))",
                     &nl, &nd, NULL);
                  }
               }
            else
               nd = -1;
            } /* End vmult setups */
         for (tval=firstv,ux=firstt; ux<=axlen;
               rxt1+=dxt,rxt2+=dxt,ryt1+=dyt,ryt2+=dyt,ux+=deltat) {
            line(rxt1,ryt1,rxt2,ryt2);    /* Plot major tick mark */
            if (nd >= 0) {                /* Plot tick value */
               sconvrt(printme,tickfmt,&tval,NULL);
               nl = strnlen(printme,MXTVLEN);
               tx = ux - wbcd*(float)(nl);
               symbol(x+tx*dx-ty*dy,y+ty*dx+tx*dy,height,
                  printme,angle,nl);
               if (ktk & AX_TKEXP)
                  tval += vmult;
               else
                  tval *= vmult;
               }
            if (nmti > 1) {               /* Plot minor tick marks */
               for (nl=1; nl<nmti; nl++) {
                  rxm = rxt1 + dxm[nl];
                  rym = ryt1 + dym[nl];
                  line(rxm,rym,rxm-oym,rym+oxm);
                  }
               }
            } /* End major tick loop */
         }

/* Label the axis */

      if (label && height > 0.0) {
         if ((nl = strlen(label)) > 0) {
            lblh = AXLBLH*height;
            tx = 0.5*(axlen - ASPECT*lblh*(float)nl);
            ty = loff + (stk*LABELOFF + stv) * lblh;
            symbol(x+tx*dx-ty*dy,y+ty*dx+tx*dy,lblh,label,angle,nl);
            }
         }

      freev(dxm, "Minor ticks");  /* Free minor tick table */

      } /* End if MFMode */

   } /* End axlog() */
