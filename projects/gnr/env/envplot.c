/* (c) Copyright 1989-2016, The Rockefeller University *21116* */
/* $Id: envplot.c 24 2018-03-15 19:04:55Z  $ */
/***********************************************************************
*              Darwin III Environment Simulation Package               *
*                               ENVPLOT                                *
*                   Plot the Darwin III environment                    *
*                                                                      *
*  int envplot(Eptr, stim_type *inpary, float xstart, float ystart,    *
*        float xlong, float ylong, char *stitle)                       *
*                Eptr: pointer to current environment                  *
*              inpary: input object array to be plotted                *
*     (xstart,ystart): lower left corner coordinates                   *
*       (xlong,ylong): size of environment in inches                   *
*              stitle: plot title (NULL to omit)                       *
************************************************************************
*  V4A, 01/12/89, JWT - Translated into C                              *
*  V4B, 01/12/89, JWT - Add SER,CYC,and ONLINE parameters              *
*  Rev, 02/21/90, GNR - Atomize plotting operations                    *
*  Rev, 09/13/90, MC  - Add undocumented pointer trick for colors      *
*  Rev, 11/23/90, GNR - Change plot title to just string "IA"          *
*  Rev, 01/15/92, GNR - Handle ept correctly for colored objects       *
*  Rev, 05/08/92, GNR - Use new ROCKS plot calls for atomization,      *
*                       use standard letter height, retrace flags      *
*  Rev, 07/31/92, GNR - Omit pathological canonical arm tangents       *
*  Rev, 08/28/92, GNR - Add kludge for multiple colors with KSVOC      *
*  Rev, 09/10/92, GNR - Plot arm tip at true x,y unless OLDHANDV       *
*  Rev, 04/13/96, GNR - Remove OLDHANDV                                *
*  Rev, 04/10/97, GNR - Restore ser,cyc plotting, new ENVDEF controls  *
*  Rev, 12/20/97, GNR - Replace ser,cyc args with stitle               *
*  Rev, 02/22/03, GNR - Change s_type to stim_type                     *
*  Rev, 01/27/07, GNR - Convert octal colors to hex for new mfdraw     *
*  ==>, 01/27/07, GNR - Last mod before committing to svn repository   *
*  V8H, 12/01/10, GNR - Separate env.h,envglob.h w/ECOLMAXCH change    *
*  R22, 03/26/16, GNR - Add ARMTP option to plot touch pads            *
*  Rev, 11/06/17, GNR - Bug fix:  Divide tla/tlw by dSxy for ellips    *
***********************************************************************/

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include "sysdef.h"
#include "envglob.h"
#include "jntdef.h"
#include "armdef.h"
#include "wdwdef.h"
#include "nccolors.h"
#include "plots.h"
#include "rocks.h"

#define KARM 0.012         /* Fraction of env size for arm radius    */
#define KOBJ 1.953125e-3   /* EPR multiplier to (approximately) inscribe
                              objects in one grid square = 1/(256*2) */
#define KJNT 0.5           /* "Shoulder" plot multiplier = dia->rad  */
#define MAX_ITER 8         /* Max allowed iterations of tangent calc */
#define RESOLUTION 0.01    /* Resolution of plot device  */
#define dSxy 32768.0       /* Scale of tla,tll (see deglobal.h)      */

#define tracing(A)  ((A)->jatt&ARMTR)     /* Check for arm trace    */
#define gription(A) ((A)->jatt&ARMGP)     /* Check for arm gription */

/*=====================================================================*
*                               padplot                                *
*                                                                      *
*  This little routine is used to plot a touch pad, called either for  *
*  a normal joint or a universal joint.                                *
*  N.B.  cosjt,sinjt args are in the left-handed coord system of the   *
*     input array.  yinc < 0 is a flag to indicate arm is in trace     *
*     mode, touch pad is moved "up" tla/2 so centered on end of arm.   *
*=====================================================================*/

static void padplot(struct JNTDEF *pj, float xstart, float ystop,
      float xinc, float yinc, float cosjt, float sinjt) {

   float tpl = (float)pj->u.jnt.tll/dSxy;
   float tpa = (float)pj->u.jnt.tla/dSxy;
   float tplc = tpl*cosjt;
   float tpls = tpl*sinjt;
   float tpac = tpa*cosjt;
   float tpas = tpa*sinjt;
   float xj,yj,x1,x10,y1,y10,x2,y2,delx,dely,mul;
   int   il;
   xj = xstart + pj->u.jnt.jx*xinc;
   if (yinc < 0) {
      yinc = fabsf(yinc); yj =  ystop -
         yinc*(pj->u.jnt.jy - 0.5*(float)pj->u.jnt.tla/dSxy); }
   else
      yj =  ystop - pj->u.jnt.jy*yinc;
   /* Draw lines parallel to lateral axis */
   delx = 0.5*xinc*tpls, dely = 0.5*yinc*tplc;
   x10 = x1 = xj - delx, y10 = y1 = yj + dely;
         x2 = xj + delx,       y2 = yj - dely;
   mul = 1.0/(float)pj->u.jnt.nta;
   delx = mul*xinc*tpac, dely = mul*yinc*tpas;
   for (il=0; il<=pj->u.jnt.nta; ++il) {
      line(x1,y1,x2,y2);
      x1 -= delx, x2 -= delx;
      y1 -= dely, y2 -= dely;
      }
   /* Draw lines parallel to axial axis */
   x1 = x10,            y1 = y10;
   x2 = x1 - xinc*tpac, y2 = y1 - yinc*tpas;
   mul = 1.0/(float)pj->u.jnt.ntl;
   delx = mul*xinc*tpls, dely = mul*yinc*tplc;
   for (il=0; il<=pj->u.jnt.ntl; ++il) {
      line(x1,y1,x2,y2);
      x1 += delx, x2 += delx;
      y1 -= dely, y2 -= dely;
      }
   } /* End padplot() */


/*=====================================================================*
*                               envplot                                *
*=====================================================================*/

int envplot(struct ENVDEF *Eptr, stim_type *inpary, float xstart,
      float ystart, float xlong, float ylong, char *stitle) {

   /* Masks for isolating colors from 2-3-3 packed pixels */
   static stim_type icm[4] = {0xff, 0xc0, 0x07, 0x38};
   static char ic2[2] = {0,3};

   struct WDWDEF *pw;      /* Pointer to current window */
   struct ARMDEF *pa;      /* Pointer to current arm    */
   float dx,dy;            /* Delta X,Y used in plot calculations   */
   float xinc,yinc;        /* X,Y grid square size (x,ylong / lx,y) */
   float xstop,ystop;      /* max X,Y values for plot   */
   float radius,radstm;    /* Radii of plot objects     */
   float x,y,x1,y1,        /* X,Y values used for plot() calls */
         xj,yj,xp,yp;
   long lp1,lp2,index2,ic; /* Loop counters */
   int kfill;              /* Drawing flag: FILLED or THIN    */
   char rgb;               /* TRUE if using pixel colors      */
   char oc;                /* TRUE if "OR"-color mode is set  */
#if 1  /* For temp color manipulating code below */
   char oldpjx = 100,xcol[5];
#endif

   /* Local copies of environment variables */
   float rlx1 = Eptr->rlx;
   float rly1 = Eptr->rly;
   long egrd = Eptr->egrid;
   long ltotal1 = Eptr->ltotal;
   long lymul21 = Eptr->lymul2;

   /* Initialize graph boundaries and intervals */
   xinc = xlong/rlx1;
   yinc = ylong/rly1;
   xstop = xstart + xlong;
   ystop = ystart + ylong;
   radius = KARM * min(xlong,ylong);
   radstm = KOBJ * Eptr->epr*min(xinc,yinc);

   /* Set retrace control */
   retrace(Eptr->kretrace ? RETRACE : NORETRACE);

   /* Draw the outside input array border */
   pencol(Eptr->ecolgrd);
   rect(xstart,ystart,xlong,ylong,THIN);

   /* Draw grid lines if requested */
   if (egrd) {
      int i;
      float xgi = egrd*xinc;
      float ygi = egrd*yinc;
      for (y=ystart+ygi,i=egrd; i<Eptr->ly; y+=ygi,i+=egrd)
         line(xstart,y,xstop,y);
      for (x=xstart+xgi,i=egrd; i<Eptr->lx; x+=xgi,i+=egrd)
         line(x,ystart,x,ystop);
      } /* End if egrd */

   /* Label the plot using letter height passed from client program */
   if (!Eptr->komit) {
      symbol(xstart, ystart-Eptr->eslht, 0.7*Eptr->eslht, "IA", 0.0, 2);
      if (stitle)
         symbol(xstart, ystop+0.3*Eptr->eslht, 0.7*Eptr->eslht,
            stitle, 0.0, strlen(stitle));
      }

/*---------------------------------------------------------------------*
*                        Draw stimulus objects                         *
*---------------------------------------------------------------------*/

   /* Assign kfill to draw filled or thin objects */
   kfill = Eptr->kfill ? FILLED : THIN;

   /* rgb is assigned to use pixel color or fixed color */
   if (!strcmp(Eptr->ecolstm,"RGB"))
      rgb = TRUE;
   else {
      pencol(Eptr->ecolstm);
      rgb = FALSE;
      }

   /* oc assigned according to whether colors should OR together */
   oc = (Eptr->kcolor == ENV_ORCOL) ? 1 : 0;

   x = xstart + 0.5*xinc;
   lp2 = Eptr->ly;
   for (lp1=0; lp1<ltotal1; x+=xinc,lp2+=lymul21,lp1+=lymul21) {
      y = ystop - 0.5*yinc;
      for (index2=lp1; index2<lp2; y-=yinc,index2++) {
         float r,d;
         stim_type pjx,pix = inpary[index2];

         /* Test magnitude of pixel against ept */
         if (Eptr->kcolor) {
            if (GrayFromRGB((int)pix) < Eptr->ept) continue;
            }
         else if ((long)pix < Eptr->ept) continue;

         /* Calculate radius of pixel, using constant max radius
         *  if environment is colored.  */
         r = radstm*(rgb ? 256.0 : (float)pix);

         /* Now draw up to three objects if KSVOC specified,
         *  otherwise one object of selected color.  This messy
         *  little loop avoids making a subroutine actually to
         *  draw one of three kinds of object.  */
         for (ic=oc; ic <= ic2[oc]; ic++)
               if (pjx = pix & icm[ic]) { /* Assignment OK */
            d = 2.0*r;        /* Diameter of plot object */
            /* If COLOR STIMULUS=RGB specified...
            *  (Otherwise, color was set above, outside loop) */
            if (rgb) {
#if 1 /* Temporary version--convert color index to hex */
               if (!Eptr->kcolor) pjx = RGBFromGray((int)pix);
               if (pjx != oldpjx) {
                  xcol[0] = 'X';
                  xcol[1] = '0' + (pjx >> 6);
                  xcol[2] = '0' + ((pjx >> 3) & 7);
                  xcol[3] = '0' + (pjx & 7);
                  xcol[4] = '\0';
                  pencol(xcol);
                  oldpjx = pjx;
                  }
#else /* Original code */
               if (Eptr->kcolor)
                  color((int)pjx);
               else
                  color(RGBFromGray((int)pix));
#endif
               }
            /* Draw a representation of the pixel */
            switch (Eptr->kpixel) {
            case ENV_CIRCLE: /* Draw a good ole' circle */
               circle(x, y, r, kfill);
               break;
            case ENV_SQUARE: /* Draw a square */
               rect(x-r, y-r, d, d, kfill);
               break;
               } /* End switch */
            /* Make next object smaller */
            r *= 0.6;
            } /* End color loop */
         } /* End inner loop */
      } /* End outer loop */

/*---------------------------------------------------------------------*
*                   Draw the windows (view fields)                     *
*---------------------------------------------------------------------*/

   for (pw=Eptr->pwdw1; pw; pw=pw->pnxwdw) {
      /* Get window position and size */
      float xlo = pw->wx;
      float xhi = xlo + pw->wwd;
      float yhi = pw->wy;
      float ylo = yhi - pw->wht;
      pencol(pw->wcolr);
      if (pw->kchng != WDWLCK) {
         /* Ordinary window... draw all four edges even if touching;
         * (ENVCYC checks for edge overlap when moving window) */
         rect(xstart + xlo*xinc,
              ystop  - yhi*yinc,
              pw->wwd*xinc,pw->wht*yinc,THIN);
         } /* End regular window */
      else {
         /* Locked window... draw if up to or on edges but not beyond
         * (ENVCYC does not check locked windows for edge overlap) */
         x  = xstart + max(0.0,xlo)*xinc;
         y  =  ystop - max(0.0,ylo)*yinc;
         x1 = xstart + min(rlx1,xhi)*xinc;
         y1 =  ystop - min(rly1,yhi)*yinc;
         if (xlo >= 0.0)  line(x,y, x, y1);
         if (yhi <= rly1) line(x,y1,x1,y1);
         if (xhi <= rlx1) line(x1,y1,x1,y);
         if (ylo >= 0.0)  line(x1,y,x, y );
         } /* End locked window */
      } /* End loop over windows */

/*---------------------------------------------------------------------*
*                            Draw the arms                             *
*---------------------------------------------------------------------*/

   /* Experiment:  Offset coords by 0.5 grid so touch pad squares
   *  overlap the pixels they touch */
#if 0
   xstart += 0.5*xinc;
   ystop  -= 0.5*yinc;
#endif

   for (pa=Eptr->parm1; pa; pa=pa->pnxarm) {
      struct JNTDEF *pj, *pje = pa->pjnt + pa->njnts;
      pencol(pa->acolr);
      /* Move to starting position for this arm.  xj,yj are
      *  redundantly set here to eliminate uninit warning.  */
      xj = xp = xstart + pa->aattx*xinc;
      yj = yp =  ystop - pa->aatty*yinc;
      circle(xp,yp,radius,THIN);
      if (!tracing(pa)) {
         /* Plot bones and joint, proximal to distal.
         *  XINC may not equal YINC, so can't use SINJT = DX/EJL... */
         for (pj=pa->pjnt; pj<pje; pj++) {
            float dxy2;
            xj = xstart + pj->u.jnt.jx*xinc;
            yj = ystop - pj->u.jnt.jy*yinc;
            dx = xj - xp;
            dy = yj - yp;
            if ((dxy2 = dx*dx + dy*dy) > RESOLUTION) {
               float dxy = sqrt((double)dxy2);
               float cosjt = dx/dxy;
               float sinjt = dy/dxy;
               /* Draw a line along the segment axis */
               line(xp,yp,xj,yj);
               if (pa->jatt & ARMTP && pj->u.jnt.tla) {
                  /* Plot touch pad */
                  padplot(pj, xstart, ystop, xinc, yinc, cosjt, sinjt);
                  }
               else {
                  /* Make the segment thicker */
                  float armsin = 0.875*radius*sinjt;
                  float armcos = 0.875*radius*cosjt;
                  line(xj+armsin,yj-armcos,xp+armsin,yp-armcos);
                  line(xp+armsin,yp-armcos,xp-armsin,yp+armcos);
                  line(xp-armsin,yp+armcos,xj-armsin,yj+armcos);
                  line(xj-armsin,yj+armcos,xj+armsin,yj-armcos);
                  }
               circle(xj,yj,radius,THIN);
               } /* End if dxy2 */
            xp=xj, yp=yj;
            } /* End loop over joints */
         } /* End if !tracing */

      else /* Plot universal joint canonical arm (shoulder) */ {
         struct JNTDEF *pjn = pje - 1;
         xj = xstart + xinc*pa->armx;
         yj =  ystop - yinc*pa->army;
         line(xp,yp,xj,yj);
         if (pa->jatt & ARMTP && pjn->u.jnt.tla) {
            /* Plot touch pad (not rotated) */
            padplot(pjn, xstart, ystop, xinc, -yinc, 0.0, 1.0);
            }
         else {
            /* Plot the traditional ellipse and tangent lines */
            float hh = KJNT*xinc*pjn->u.jnt.tla/dSxy;
            float hw = KJNT*yinc*pjn->u.jnt.tll/dSxy;
            float dec;             /* Distance(ellipse to circle)     */
            float ctau,stau;       /* cos(tau), sin(tau)              */
            float xc1,xc2,yc1,yc2; /* X,Y intersections with circle   */
            float xe1,xe2,ye1,ye2; /* X,Y intersections with ellipse  */
            float uc1,uc2,vc1,vc2; /* Old x,y intersects with circle  */
            float ue1,ue2,ve1,ve2; /* Old x,y intersects with ellipse */
            int iter = 0;          /* Iteration count */
            pencol("BLACK");
            circle(xj,yj,radius,THIN);
            pencol(pa->acolr);
            ellips(xj,yj,hw,hh,0.0,THIN);

            /* The following algorithm finds decent tangents connect-
            *  ing the ellipse to the circle (see design notes).  */

            /* "Prime" the algorithm with data from the center line...
            */
            xe1 = xe2 = xj;
            ye1 = ye2 = yj;
            xc1 = xc2 = xp;
            yc1 = yc2 = yp;
            do {
               /* Omit drawing tangents if alg. does not converge */
               if (++iter > MAX_ITER) goto NextArm;
               /* Store previous coordinates for convergence test */
               ue1 = xe1;  ue2 = xe2;
               ve1 = ye1;  ve2 = ye2;
               uc1 = xc1;  uc2 = xc2;
               vc1 = yc1;  vc2 = yc2;

               /* Perform one iteration.  Note: in tests, always
               *  converges in exactly 3 iterations(!) */
               dx = xe1 - xc1;
               dy = ye1 - yc1;
               if ((dec = sqrt(dx*dx + dy*dy)) > RESOLUTION) {
                  xe1 = xj - hw*(stau=dy/dec);
                  ye1 = yj + hh*(ctau=dx/dec);
                  xc1 = xp - radius*stau;
                  yc1 = yp + radius*ctau;
                  }
               dx = xe2 - xc2;
               dy = ye2 - yc2;
               if ((dec = sqrt(dx*dx + dy*dy)) > RESOLUTION) {
                  xe2 = xj + hw*(stau=dy/dec);
                  ye2 = yj - hh*(ctau=dx/dec);
                  xc2 = xp + radius*stau;
                  yc2 = yp - radius*ctau;
                  }
               } /* End do loop */

            /* Now just have to test for convergence (defined as change
            *  in all coordinates less than plot device resolution) */
            while (fabs(xe1-ue1) > RESOLUTION ||
                   fabs(xe2-ue2) > RESOLUTION ||
                   fabs(ye1-ve1) > RESOLUTION ||
                   fabs(ye2-ve2) > RESOLUTION ||
                   fabs(xc1-uc1) > RESOLUTION ||
                   fabs(xc2-uc2) > RESOLUTION ||
                   fabs(yc1-vc1) > RESOLUTION ||
                   fabs(yc2-vc2) > RESOLUTION) ;

            line(xj,yj,xe1,ye1);
            line(xe1,ye1,xc1,yc1);
            line(xc1,yc1,xp,yp);
            line(xp,yp,xc2,yc2);
            line(xc2,yc2,xe2,ye2);
            line(xe2,ye2,xj,yj);
            } /* End shoulder plot */
         } /* End if tracing */

      /* If arm is acting as a gripper, draw an extra thingy */
NextArm:
      if (gription(pa)) circle(xj,yj,0.5*radius,THIN);
      } /* End loop over arms */

   return ENV_NORMAL;
   } /* End envplot */

