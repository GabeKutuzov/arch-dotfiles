/* (c) Copyright 1993-2017, The Rockefeller University *21114* */
/* $Id: axpol.c 33 2017-05-03 16:14:57Z  $ */
/***********************************************************************
*                               axpol()                                *
*                                                                      *
*  SYNOPSIS:                                                           *
*     void axpol(float x, float y, const char *label, float radius,    *
*          float firstc, float deltac, float firstv, float deltav,     *
*          float angle, float cvoffx, float cvoffy, float height,      *
*          int ns, int nl, int ktk, int nd, int nmti)                  *
*                                                                      *
*  DESCRIPTION:                                                        *
*     This function draws a polar axis set with circles and optional   *
*     value labels at specified radial intervals and with short and    *
*     long spokes at specified angular intervals.  The function also   *
*     plots a centered master label at the top or bottom of the plot.  *
*                                                                      *
*  ARGUMENTS:                                                          *
*     x,y     Coordinates of starting point of the axis.               *
*     label   Master label to be centered at the top or bottom of the  *
*                diagram (see 'ktk').  The height of the lettering is  *
*                1.33 times the 'height' parameter.  If 'label' is     *
*                NULL or points to an empty string, or 'height' is     *
*                <= 0, the label is not plotted.                       *
*     radius  Radius of the polar diagram (defined as radius of the    *
*                short spokes).  Plot nothing if radius <= 0.0.        *
*     firstc  Radius of the first circle out from the origin.          *
*     deltac  Radius increment out to each successive full circle.     *
*             If <= 0, no circles are plotted.                         *
*     firstv  Value of label at first full circle.                     *
*     deltav  Increment added to 'firstv' for each successive full     *
*                circle.  If 0, no circle values are plotted.          *
*     angle   Angle in degrees counterclockwise from +x of an          *
*             imaginary radial line relative to which circle value     *
*             labels are plotted.  This parameter can be used to       *
*             place the labels along or between spokes, or even to     *
*             have circle labels when there are no spokes.             *
*     cvoffx  X offset of each circle label from the intersection of   *
*             its circle with the radial line defined by 'angle'.      *
*     cvoffy  Y offset of each circle label from the intersection of   *
*             its circle with the radial line defined by 'angle'.      *
*     height  The height of the circle value and spoke lettering.      *
*             If <= 0, all lettering is omitted.                       *
*     ns      Number of short spokes.                                  *
*     nl      Number of long spokes.                                   *
*     ktk     The sum of any desired codes from the following list:    *
*             AX_TKEFMT (=2)  Plot circle values in exponential        *
*                format;  default: use decimal format unless value     *
*                would require more than 12 digits to express.         *
*             AX_LBLLS  (=8)  Label long spokes with angle values.     *
*             AX_LBLTOP (=16) Center master label at top of axis set.  *
*                Default:  Center master label at bottom of axis set.  *
*     nd      Number of decimals in the circle labels.                 *
*                Use -1 to indicate that values should be plotted as   *
*                integers, with no decimal point.                      *
*     nmti    Number of minor tick intervals (one more than the num-   *
*                ber of tick marks) to be plotted between circles.     *
*                If no circles, plots from 'forstc' to 'radius'.       *
*                If 0, no minor tick marks are plotted.                *
*                                                                      *
*  PREDEFINED (COMPILE-TIME) PARAMETERS:                               *
*     Width of long-spoke tick marks, distance lettering is separated  *
*     from spokes or tick marks, assumed aspect ratio of lettering.    *
*                                                                      *
*  CURRENT PLOT POSITION ON RETURN:                                    *
*     Undefined                                                        *
*                                                                      *
*  RETURN VALUE:                                                       *
*     None.                                                            *
*                                                                      *
*  DESIGN NOTES:                                                       *
*     Calculations were worked out automatically to place the circle   *
*     labels relative to the spokes and circles.  However, these get   *
*     quite complicated and, given the uncertainty in lettering size   *
*     with different fonts, it was decided to let the user determine   *
*     label placement explicitly with the 'cvoffx' and 'cvoffy' args.  *
*     Long spoke angle labels, however, are moved out along their      *
*     spokes as needed to clear the largest circle (if there is one).  *
************************************************************************
*  Version 1, 01/25/93, ROZ - New routine, based on axis() function    *
*  V2A, 12/29/98, GNR - Make binary metafile, eliminate geomdefs.h     *
*  Rev, 06/29/02, GNR - Remove n & nc, add firstc, deltac, angle,      *
*                       cvoffx, cvoffy, ktk, & nmti                    *
*  ==>, 02/29/08, GNR - Last mod before committing to svn repository   *
*  Rev, 10/11/11, GNR - _NCG to _RKG, gmf.h now in glu.h, etc.         *
*  Rev, 08/29/14, GNR - Add 'const' to pointer args                    *
*  Rev, 05/02/17, GNR - Modify for PARn nodes (no RK or RKC)           *
***********************************************************************/

#include "glu.h"
#include "plots.h"
#include "rksubs.h"

#define CircleDegs 360.0

extern struct RKPndef {
   short length;
   byte expwid;
   } RKPn;

void axpol(float x, float y, const char *label, float radius,
      float firstc, float deltac, float firstv, float deltav,
      float angle, float cvoffx, float cvoffy, float height,
      int ns, int nl, int ktk, int nd, int nmti) {

   if (_RKG.s.MFActive && radius > 0.0) {
      double spkang,spkrad;      /* Spoke angle and increment */
      float dx,dy;               /* Cosine, sine of spoke angle */
      float hlblht,hlblwd;       /* Half label height, width */
      float rc,rce;              /* Radius of current,last circles */
      float spkdeg;              /* Spoke increment (degrees) */
      float tval;                /* Tick value */
      float x2,y2;               /* Coordinate for spoke or label */
      float ytop =  radius,
            ybot = -radius;      /* Location for axis set label */
      long  kbcd;                /* Mode code for bcdout */
      int   dovlbls;             /* TRUE to plot value labels */
      int   i,j;                 /* Tick or spoke index */
      int   nn;                  /* Number of chars or spokes */
      char  printme[MXTVLEN];    /* Graph label */

/* Plot circles and circle labels (leaving radius)
*  of last circle in rc for spoke lettering check) */

      if (deltac > 0.0) {
         rce = radius + 1E-6;    /* Allow for round-off in test */
         dovlbls = deltav != 0.0 && height > 0.0;
         if (dovlbls) {          /* Setup for circle value labels */
            spkang = TORADS*angle;
            dx = cos(spkang);
            dy = sin(spkang);
            hlblht = 0.5*height;
            kbcd = (nd+1)*RK_D + (ktk&AX_TKEFMT ? RK_EFMT : RK_IORF) +
               (RK_SNGL+RK_LFTJ+RK_UFLW+MXTVLEN-1);
            tval = firstv;
            }
         for (rc=firstc; rc<=rce; rc+=deltac) {
            circle(x, y, rc, THIN);
            if (dovlbls) {       /* Plot circle value labels */
               bcdoutPn(kbcd, printme, tval);
               nn = RKPn.length + 1;
               hlblwd = (float)nn*ASPECT*hlblht;
               x2 = x + rc*dx + cvoffx - hlblwd;
               y2 = y + rc*dy + cvoffy - hlblht;
               symbol(x2,y2,height,printme,0.0,nn);
               if (y2 < ybot) ybot = y2;
               y2 += height;
               if (y2 > ytop) ytop = y2;
               tval += deltac;
               }
            }
         } /* End plotting circles */

/* Plot long spokes */

      if (nl > 0) {
         float rlspk = radius + POLARLS*deltac;
         spkang = 0.0, spkrad = (2.0*PI)/(double)nl;
         dovlbls = ktk & AX_LBLLS && height > 0.0;
         if (dovlbls) {          /* Setup for spoke labels */
            hlblht = 0.5*height;
            spkdeg = CircleDegs/(float)nl;
            tval = 0.0;
            }
         for (i=0; i<nl; i++,spkang+=spkrad) {
            dx = cos(spkang);
            dy = sin(spkang);
            x2 = x + rlspk * dx;
            y2 = y + rlspk * dy;
            line(x,y,x2,y2);     /* Draw the spoke */
            if (nmti > 1) {      /* Plot minor tick marks */
               float dmjtk = deltac > 0.0 ? deltac : radius;
               float dmntk = dmjtk/(float)nmti;
               float tdx = POLARHT*dx, tdy = POLARHT*dy;
               float rctk,rmtk,tx1,ty1,tx2,ty2;
               tx1 = x - tdy, tx2 = x + tdy;
               ty1 = y + tdx, ty2 = y - tdx;
               tdy = 0.0;        /* Flag to detect no tick drawn */
               for (rctk=firstc; ; rctk+=dmjtk) {
                  for (rmtk=rctk,j=1; j<nmti; j++) {
                     if ((rmtk += dmntk) > radius)
                        goto DoneMinorTicks;
                     tdx = rmtk*dx, tdy = rmtk*dy;
                     line(tx1+tdx,ty1+tdy,tx2+tdx,ty2+tdy);
                     }
                  }
DoneMinorTicks:
               /* Test for very unlikely (but possible) cases
               *  where ticks extend beyond long radii into label */
               if (tdy) {        /* Zero if no ticks drawn */
                  tx1 = max(ty1,ty2) + tdy;
                  tx2 = min(ty1,ty2) + tdy;
                  if (tx1 > ytop) ytop = tx1;
                  if (tx2 < ybot) ybot = tx2;
                  }
               }
            if (dovlbls) {       /* Plot long spoke angle labels */
               float sdx = dx >= 0.0 ? 1.0 : -1.0;
               float sdy = dy >= 0.0 ? 1.0 : -1.0;
               float adx = dx*sdx, ady = dy*sdy;
               float b,d,xt,yt,xrad;
               bcdoutPn(RK_D+RK_D+RK_IORF+RK_SNGL+RK_LFTJ+MXTVLEN-1,
                  printme, tval);
               nn = RKPn.length +
                  (printme[RKPn.length] == '0' ? -1 : 1);
               hlblwd = (float)nn*ASPECT*hlblht;
               /* Intersecting on side or top/bottom? */
               xrad = (hlblwd*ady > hlblht*adx) ?
                  (hlblht + VALUGARD)/ady :  /* Top/bottom */
                  (hlblwd + VALUGARD)/adx;   /* Side */
               x2 += xrad*dx, y2 += xrad*dy;
               /* Additional radius needed to clear circles? */
               if (deltac <= 0.0) {
                  rc += VALUGARD - deltac;   /* Correct for loop test */
                  xt = x2 - x - hlblwd*sdx;
                  yt = y2 - y - hlblht*sdy;
                  b  = xt*dx + yt*dy;
                  d  = rc*rc + b*b - xt*xt - yt*yt;
                  if (d > 0.0 && (xrad = sqrt(d) - b) > 0.0)
                     x2 += xrad*dx, y2 += xrad*dy;
                  }
               symbol(x2-hlblwd,y2-hlblht,height,printme,0.0,nn);
               y2 += sdy*hlblht;
               tval += spkdeg;
               }
            if (y2 > ytop) ytop = y2;
            if (y2 < ybot) ybot = y2;
            }
         }

/* Plot short spokes */

      if (ns > 0) {
         spkang = 0.0, spkrad = (2.0*PI)/(double)ns;
         for (i=0,nn=0; i<ns; i++,spkang+=spkrad,nn+=nl) {
            /* Omit if short spoke coincides with a long one */
            if (nl > 0 && nn % ns == 0) continue;
            x2 = x + radius * cos(spkang);
            y2 = y + radius * sin(spkang);
            line(x,y,x2,y2);
            }
         }

/* Plot graph's label */

      if (label && height > 0.0) {
         if ((nn = strlen(label)) > 0) {
            float lblh = AXLBLH*height;
            x2 = x - 0.5*ASPECT*lblh*(float)nn;
            y2 = (ktk & AX_LBLTOP) ?
               ytop + VALUGARD : ybot - VALUGARD - lblh;
            symbol(x2,y2,lblh,label,0.0,nn);
            }
         }

      } /* End if MFMode */

   } /* End axpol() */
