/* (c) Copyright 2007-2018, The Rockefeller University *11115* */
/* $Id: d3vitp.c 78 2018-08-02 18:36:58Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                               d3vitp                                 *
*                                                                      *
*  Evaluate virtual image touchpad.  This is similar to d3vtch,        *
*     except works from a camera image rather than the IA and the      *
*     position and orientation of the touch pad within the image       *
*     can be specified by any other virtual sense.  The order of       *
*     looping over X,Y coords has been reversed, reflecting the        *
*     storage order in memory for images vs IA.                        *
*  Code was changed in R74 to access "working" tvix*tviy image at      *
*     pmyim rather than possibly cropped tvx*tvy network input image   *
*     at pbcst + otvi.  The idea is that maybe the results could be    *
*     used to generate a "saccade" (changes in tvxo,tvyo) to move      *
*     the center of attention across a larger image.  Note that in     *
*     a parallel computer this full image is currently available only  *
*     on PAR0, not on PARn nodes, so d3vitp cannot be made parallel.   *
*  This sense currently cannot be associated with a modality,          *
*     because there is no way currently to identify objects            *
*     touched in an arbitrary image.                                   *
*                                                                      *
************************************************************************
*                                                                      *
*  A FEW NOTES ON COORDINATE SYSTEMS:                                  *
*  - --- ----- -- ---------- --------                                  *
*  All working on the image array is in units of pixels, but it is     *
*     important to remember that these may be 1|2|3|6 bytes wide.      *
*  X goes to the right, Y goes down from the top, same as the IA.      *
*  The touchpad Y (a, axial) axis is along the direction defined by    *
*     the orientation angle (0 = horizontal to the right after adding  *
*     any angle offset).  The X (l, lateral) axis points left to right *
*     when the pad is viewed with the distal end up.  The orientation  *
*     angle is measured counterclockwise looking down on the image.    *
*  cosax and sinax are the cos and sin of the angle between touchpad   *
*     a and horizontal (+X in the image).  Because the image coordi-   *
*     nate system is left handed, at orientation 0 increasing pad X    *
*     maps onto increaing image Y.                                     *
************************************************************************
*  Written by George N. Reeke                                          *
*  V8D, 03/27/07, GNR - New routine based on d3vtch                    *
*  ==>, 12/21/07, GNR - Last mod before committing to svn repository   *
*  V8F, 02/15/10, GNR - Remove conditionals on image inputs            *
*  R66, 03/11/16, GNR - Rename nvx->nsxl, nvy->nsya so != ix->nv[xy]   *
*  R74, 07/26/17, GNR - Access tvix*tviy working image                 *
*  R78, 04/14/18, GNR - Allow scale to change for each image           *
*  R78, 04/25/18, GNR - Allow input from 
***********************************************************************/


#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "sysdef.h"
#include "d3global.h"
#include "tvdef.h"
#include "plots.h"
#include "swap.h"

#define NEDR         4           /* Number of edges of a rectangle */
#define BIGXYI  1.0E12           /* Bigger than any image size */
#define XYSLOP  1.0E-8           /* Slop in positional comparisons */

/***********************************************************************
*                              vitpxbnd                                *
*                                                                      *
*  This local routine finds the smallest and largest x intersections   *
*  in image coordinates for a single box of a touch pad at a given y   *
*  coordinate.  Having it as a function allows each calculation to be  *
*  used twice: as the top edge at some y and the bottom at that y+1.   *
*  The loop in which it is called assures that there will always be    *
*  at least one, and sometimes two solutions.                          *
***********************************************************************/

static void vitpxbnd(float *x1, float *x2,
      float *xb, float *yb, float *sb, int iy) {

   float xlit,xbig;              /* For accumulating results */
   float xlo,xhi;                /* X at ylo,yhi of j edge */
   float ylo,yhi;                /* Low,high Y of j edge */
   float y = (float)iy;          /* Y of line of interest */
   float ydn,yup;                /* Y with down,up slop */
   int   j;                      /* Edge loop index */

   xlit = BIGXYI, xbig = 0;      /* Initialize x limits */
   ydn = y - XYSLOP;
   yup = y + XYSLOP;

   /* Look at the four edges of the box in turn */
   for (j=0; j<NEDR; ++j) {
      if (yb[j] <= yb[j+1])
         xlo = xb[j], ylo = yb[j], xhi = xb[j+1], yhi = yb[j+1];
      else
         xlo = xb[j+1], ylo = yb[j+1], xhi = xb[j], yhi = yb[j];

      /* Matrix of actions for 0, 1, or 2 intersections */
      if (yhi < ydn || ylo > yup) continue;
      if (yhi <= yup) {          /* yhi is very close to y */
         if (ylo >= ydn) {       /* So is ylo, check both ends */
            if (xlo < xlit) xlit = xlo;
            if (xlo > xbig) xbig = xlo; }
         if (xhi < xlit) xlit = xhi;
         if (xhi > xbig) xbig = xhi;
         }
      else {                     /* yhi is above y */
         if (ylo >= ydn) {       /* But ylo is near, check xlo */
            if (xlo < xlit) xlit = xlo;
            if (xlo > xbig) xbig = xlo; }
         else {                  /* yhi is above, ylo is below */
            /* Calculate intersection.  The slope will have been
            *  stored, because the two y values must have been
            *  enough different to get to this case.  */
            xlo = xb[j] + sb[j]*(y - yb[j]);
            if (xlo < xlit) xlit = xlo;
            if (xlo > xbig) xbig = xlo;
            }
         }
      } /* End of j loop */

   *x1 = xlit;
   *x2 = xbig;

   } /* End vitpxbnd() */


/***********************************************************************
*                               d3vitp                                 *
***********************************************************************/

void d3vitp(struct IMGTPAD *pitp) {

   gvin_fn vitpgett;             /* Ptr to color extraction routine */
   byte  *pim,*pym,*pxm,*pxme;   /* Ptrs to image data */
   byte  *pv,*pv0;               /* Ptrs to touchpad data */
   struct UTVDEF *putv;          /* Ptr to "user" camera info */
   float dax,day;                /* Projections of axial,lateral */
   float dlx,dly;                /* pad box edges onto image X,Y */
   float rorp;                   /* Orientation of pad (radians) */
   float sclxu,sclyu;            /* Used x,y scales */
   float sinax, cosax;           /* sin,cos of angle(axial to X) */
   float slope[NEDR];            /* Slopes of 4 bounds of pad */
   float xb[NEDR+1],yb[NEDR+1];  /* Coords of box corners */
   float xcur,ycur;              /* Current position of pad */
   size_t sovc;                  /* Sensor data offset */
   int   ipix,ipiy;              /* Image pixel increments */
   int   irow,icol;              /* Row, col loop counters */
   int   npx,npy;                /* Integer pad sizes */
   int   tvix1,tviy1;            /* X,Y image size minus 1 */

/* If not used, don't bother */

   if (!(pitp->vc.vcflags & VKUSED)) return;

/* Locate virtual cells for image, sensor, and touchpad */

   putv = &pitp->pcam->utv;
   pim = (putv->tvsflgs & TV_ONDIR ? RP->pbcst : RP0->pipimin) +
      (pitp->vc.vcxc.tvcsel & EXGfmC ?
      pitp->pcam->otvg : pitp->pcam->otvi);
   pv = pv0 = RP->pbcst + pitp->vc.ovc;
   sovc = ((struct VCELL *)pitp->vc.pvcsrc)->ovc;

/* Get a pointer to the correct color extraction routine */

   vitpgett = d3gvin(TV_SRC, &pitp->vc.vcxc);

/* Clear the virtual cells for this touchpad */

   memset((char *)pv0, 0, pitp->vc.nvcells);

/* A few handy constants */

   ipix = pitp->vc.vcxc.tvlpxgc;
   ipiy = ipix*putv->tvix;
   npx =  pitp->vc.nsxl;
   npy =  pitp->vc.nsya;
   tvix1 = putv->tvix - 1;
   tviy1 = putv->tviy - 1;

/* Locate the center of the touchpad and scale to image units.
*  Pick up orientation of touchpad, offset, and scale to radians.
*  Use standard values for coordinates that do not change.  */

   rorp = pitp->offa;
   ycur = pitp->offy;
   xcur = pitp->offx;

   sclxu = pitp->sclx ? pitp->sclx : (float)putv->tvix;
   sclyu = pitp->scly ? pitp->scly : (float)putv->tviy;

   /* N.B.  It is intentional that each case in these switches
   *  falls through to the next.  */
   if (pitp->vc.vcflags & VKR4) {
      float *pfs = (float *)RP->paw + sovc;
      switch (pitp->itchng) {
      case 3: rorp += pitp->usca * pfs[pitp->ovso];
      case 2: ycur += pitp->scly * pfs[pitp->ovsy];
      case 1: xcur += pitp->sclx * pfs[pitp->ovsx];
      case 0: ;
         } /* End itchng switch */
      } /* End if float sensor */
   else {
      byte *pbs = (byte *)RP->pbcst + sovc;
      switch (pitp->itchng) {
      case 3: rorp += pitp->usca * (float)pbs[pitp->ovso];
      case 2: ycur += pitp->scly * (float)pbs[pitp->ovsy];
      case 1: xcur += pitp->sclx * (float)pbs[pitp->ovsx];
      case 0: ;
         } /* End itchng switch */
      } /* End if byte sensor */

/* Compute sine and cosine of axial orientation of touchpad */

   cosax = cos(rorp);
   sinax = sin(rorp);

   if (fabsf(cosax) > (0.1*XYSLOP)) {
      slope[1] = -sinax/cosax;
      slope[3] = slope[1]; }

   if (fabsf(sinax) > (0.1*XYSLOP)) {
      slope[0] = cosax/sinax;
      slope[2] = slope[0]; }

/* Compute axial and lateral sizes of the pad boxes in image
*  coords and their projections along the rows and columns.  */

   dax =  cosax * pitp->vc.vcmax;
   day = -sinax * pitp->vc.vcmax;
   dlx =  sinax * pitp->vc.vcmin;
   dly =  cosax * pitp->vc.vcmin;

/* Compute location of upper left hand corner of tactile box */

   xcur -= 0.5*(pitp->tla*cosax + pitp->tll*sinax);
   ycur -= 0.5*(pitp->tla*sinax + pitp->tll*cosax);

/* Loop over the npy boxes along the axial direction.
*  Loop over the npx boxes along the lateral direction.  */

   for (irow=0; irow<npy; ++irow) {
      xb[0] = xcur;
      yb[0] = ycur;
      for (icol=0; icol<npx; ++icol) {
         float xlit1,xbig1;      /* Small,large x at y = iyp     */
         float xlit2,xbig2;      /* Small,large x at y = iyp + 1 */
         float ylit,ybig;        /* Top, bottom edges of one box */
         int   ixlit,ixbig;      /* Integer xlit,xbig */
         int   iylit,iybig;      /* Integer ylit,ybig */
         int   i;                /* Edge loop index */
         int   iyp;              /* Pixel y loop index */
         si32  tt,touch = 0;     /* Computed touch value */

         /* Locate the four corners of the current small box
         *  (corners are numbered in counterclockwise order 0 to 3).
         *  Make an extra copy of xb[0] to facilitate edge loop.  */
         xb[1] = xb[0] + dlx;    yb[1] = yb[0] + dly;
         xb[2] = xb[1] + dax;    yb[2] = yb[1] + day;
         xb[3] = xb[0] + dax;    yb[3] = yb[0] + day;
         xb[4] = xb[0];          yb[4] = yb[0];

         /* Find smallest and largest Y (image) coordinate,
         *  convert to integers for inner loop control, and
         *  locate start of the Y row in the image.  */
         ylit = BIGXYI, ybig = 0.0;
         for (i=0; i<NEDR; ++i) {
            if (yb[i] < ylit) ylit = yb[i];
            if (yb[i] > ybig) ybig = yb[i];
            }
         iylit = (int)ylit; if (iylit < 0)     iylit = 0;
         iybig = (int)ybig; if (iybig > tviy1) iybig = tviy1;
         pym = pim + ipiy*iylit;

         /* Perform the x intersection calculation for the lowest
         *  y coordinate.  By doing this outside the loop below,
         *  half the work is saved, because when y increments, the
         *  old iyp+1 becomes the new iyp.  */
         vitpxbnd(&xlit1, &xbig1, xb, yb, slope, iylit);

         /* Loop over the rows of pixels in the Y range.
         *  Determine the X range in each row.  This is done
         *  by checking all vertices and all intersections of
         *  lines against the left and right edge of the box.
         *
         *  (Following may be a zero-trip loop)  */
         for (iyp=iylit; iyp<=iybig; pym+=ipiy,iyp++) {

            vitpxbnd(&xlit2, &xbig2, xb, yb, slope, iyp+1);
            ixlit = (int)min(xlit1, xlit2);
            ixbig = (int)max(xbig1, xbig2);
            if (ixlit < 0)    ixlit = 0;
            if (ixbig > tvix1) ixbig = tvix1;

            /* Loop over all pixels in this row that touch
            *  the current box.  Take touch value = largest
            *  value touched.  Do separately for each color.  */
            pxm  = pym + ipix*ixlit;
            pxme = pym + ipix*ixbig;
            while (pxm <= pxme) {
               tt = vitpgett(pxm, &pitp->vc.vcxc);
               if (tt > touch) touch = tt;
               pxm += ipix;
               }

            /* Advance to the next y row */
            xlit1 = xlit2;
            xbig1 = xbig2;
            } /* End of iyp loop */

         xb[0] = xb[1];
         yb[0] = yb[1];
         *pv++ = (byte)(touch >> Sv2mV);
         } /* End of icol loop */

      xcur += dax;
      ycur += day;
      } /* End of irow loop */

/* If touch receptors are to be pressure-sensitive,
*  now find how many touches there were and scale accordingly.  */

   if (pitp->itop & ITOP_PRES) {
      byte *pvp;
      long psarea = 0;
      long padarea = pitp->vc.nvcells;
      for (pvp=pv0; pvp<pv; pvp++) if (*pvp >= 128) psarea++;
      if (psarea > 0) {
         register long psmult = (padarea<<Ss2hi)/(padarea+psarea);
         for (pvp=pv0; pvp<pv; pvp++) {
            *pvp = (s_type)((((long)*pvp)*psmult)>>Ss2hi);
            }
         }
      } /* End pressure-sensitive calculation */

   } /* End of d3vitp() */

