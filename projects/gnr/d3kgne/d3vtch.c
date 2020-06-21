/* (c) Copyright 1991-2000, Neurosciences Research Foundation, Inc. */
/* $Id: d3vtch.c 44 2011-03-04 22:36:14Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                               d3vtch                                 *
*                                                                      *
*  Evaluate virtual touch cells.  If environment is colored, largest   *
*     value of each color is accumulated separately, because multiple  *
*     conntypes with different color sensitivity may pick up VT input. *
*     Of course, using color for touch is really just a way to make    *
*     touch responses different from visual for same object.           *
*  WARNING: Don't even call it if there is no input array!!            *
*                                                                      *
************************************************************************
*                                                                      *
*  A FEW NOTES ON COORDINATE SYSTEMS:                                  *
*  - --- ----- -- ---------- --------                                  *
*  All working is on the input array in units of pixels.               *
*  X(IA) goes to the right, Y(IA) goes down.                           *
*  The virtual group Y axis is along an arm segment, distal            *
*     to proximal, and the X axis is lateral, left to right            *
*     when the segment is viewed with the distal end up and            *
*     the proximal end down.  This way arm Y goes down like            *
*     Y(IA).                                                           *
*  For arm segments, cosax and sinax are the cos and sin of            *
*     the axial direction (virtual group Y, distal to proximal)        *
*     relative to the IA coordinate system.                            *
*  Since the coordinate system is left-handed, rotation from           *
*     axial to lateral is by -pi/2 rather than +pi/2.                  *
*  Since arm coords range from (0,0) to (nsx,nsy), while pixel         *
*     coords range from (0,0) to (nsx-1,nsy-1), it nicely works        *
*     out that all noninteger box coords can be dropped to next        *
*     lower integer, except if a box coord is an exact integer,        *
*     it should be dropped for range high, not for range low.          *
*  See figure in design notes, p. 70.                                  *
************************************************************************
*  Written by George N. Reeke                                          *
*  V4A, 05/10/89, Translated to C                                      *
*  Rev, 04/19/90, Add z-dimension control of touching                  *
*  Rev, 08/20/92, GNR - Handle colored environments properly           *
*  Rev, 09/10/92, GNR - Center touch on arm tip if canonical           *
*  V5F, 12/10/92, GNR - Add AOP=I to force touch on single grid point  *
*  V8A, 04/28/95, GNR - One touch block per call, fix normalization,   *
*                       add marking of modality bits for objects felt, *
*                       remove COMPAT=H                                *
*  Rev, 12/02/00, GNR - Allow ARMPT with colored environment           *
*  ==>, 08/08/07, GNR - Last mod before committing to svn repository   *
***********************************************************************/

#define NEWARB          /* Use new EFFARB mechanism */

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "d3global.h"
#include "jntdef.h"
#include "armdef.h"
#include "objdef.h"     /* (Needed to access object z coord) */
#include "rocks.h"
#include "nccolors.h"
#include "bapkg.h"

void d3vtch(struct VCELL *pvc, stim_mask_t *inpary) {

   struct ARMDEF *pa = (struct ARMDEF *)pvc->pvcsrc;
   struct JNTDEF *pj = pa->pjnt;    /* Ptr to joints of this arm */
   struct MODALITY *pmdlt;          /* Ptr to modality block */
   byte *pv, *pv0, *ipv;

   float dl;
   float da;
   float dlx, dly;
   float dax, day;
   float sinax, cosax;
   float rlx1, rly1;
   float xb[5], yb[4];
   float slope[4];
   float xprv, yprv;
   float xcur, ycur;
   float anta, antl;
   float xrow, yrow;
   long ly1,nsy2;
   long lzi;                  /* Z coord */
   int ij, irow, icol;        /* Joint, row, col counters */
   int kcanm;                 /* TRUE if in canonical mode */
   int kintt;                 /* TRUE if touching only one pixel */
   int kcol = RP->kcolor;     /* TRUE if environment colored */
   short lnta, lntl;          /* Number of axial, lateral receptors */

/* Locate the modality block, only if used for XRSTATS */

   pmdlt = pvc->pvcmdlt;
   if (pmdlt && !(pmdlt->mdltflgs & MDLTFLGS_RQST)) pmdlt = NULL;

/* Clear the virtual touch cells for this arm */

   if (!RP->pbcst) return;          /* A little safety check */
   pv = RP->pbcst + pvc->ovc;
   memset((char *)pv, 0, pvc->ncells);

/* A few handy constants */

   nsy2 = (ly1 = RP->nsy)<<1;
   rlx1 = (float)(RP->nsx - 1);
   rly1 = (float)(RP->nsy - 1);
   xprv = pa->aattx;
   yprv = pa->aatty;
   lnta = pvc->nvy;
   lntl = pvc->nvx;
   anta = (float)lnta;
   antl = (float)lntl;
   kcanm = pa->jatt & ARMTR;
   kintt = pa->jatt & ARMIT;

/* Loop over all joints on this arm */

   for (ij=1; ij<=pa->njnts; ij++,pj++) {

/* Omit this joint if its tactile area is zero */

      if (pj->u.jnt.tla<=0.0 || pj->u.jnt.tll<=0.0) goto NXT_JNT;

/* Compute sine and cosine of axial orientation of touch box */

      xcur = pj->u.jnt.jx;
      ycur = pj->u.jnt.jy;

      if (kcanm) {
         cosax = 0.0;
         sinax = 1.0; }
      else {
         cosax = (xprv-xcur)/pj->jl;
         sinax = (yprv-ycur)/pj->jl; }

      if (cosax != 0.0) {
         slope[1] = sinax/cosax;
         slope[3] = slope[1]; }

      if (sinax != 0.0) {
         slope[0] = -cosax/sinax;
         slope[2] = slope[0]; }

/* Compute axial and lateral sizes of the small boxes.
*  Compute their projections along the rows and columns.  */

      da = pj->u.jnt.tla/anta;
      dl = pj->u.jnt.tll/antl;
      dax = da*cosax;
      day = da*sinax;
      dlx = dl*sinax;
      dly = -dl*cosax;

/* Compute location of upper left hand corner of tactile box */

      xrow = xcur - (0.5*antl)*dlx;
      yrow = ycur - (0.5*antl)*dly;
      if (kcanm)
         yrow -= 0.5*pj->u.jnt.tla;

/* Loop over the lnta boxes along the axial direction.
*  Loop over the lntl boxes along the lateral direction.  */

      pv0 = pv;
      lzi = pj->u.jnt.tzi;
      for (irow=1; irow<=lnta; irow++) {
         xb[0] = xrow;
         yb[0] = yrow;
         for (icol=1; icol<=lntl; icol++) {
            float xlit, xbig;
            long lowx, topx, ipx, ipy;
            int i, ixp;
            stim_mask_t touch;         /* B&W touch value */
            stim_mask_t BB,GG,RR;      /* Colored touch values */
            touch = BB = GG = RR = 0;

            /* Locate the four corners of the current small box.
            *  Make an extra copy of xb[0] to facilitate edge loop.
            */

            xb[1] = xb[0] + dlx;
            yb[1] = yb[0] + dly;
            xb[2] = xb[1] + dax;
            yb[2] = yb[1] + day;
            xb[3] = xb[0] + dax;
            yb[3] = yb[0] + day;
            xb[4] = xb[0];

            /* Find smallest and largest X(IA) coordinate */

            xlit = rlx1;
            xbig = 0.0;
            for (i=0; i<4; i++) {
               if (xb[i] < xlit) xlit = xb[i];
               if (xb[i] > xbig) xbig = xb[i];
               }
            if (kintt) {
               /* Use center of box and round to nearest pixel */
               topx = (long)(0.5*(xlit + xbig + 1.0));
               if (topx < 0 || topx >= RP->nsx) goto NXT_BOX;
               lowx = topx;
               }
            else {
               /* Scan all pixels covered by the box */
               topx = (long)(min(xbig-(1e-12f),rlx1));
               lowx = (long)xlit;
               if (lowx < 0) lowx = 0;
               }
            ipx = nsy2*lowx;

            /* Loop over the columns of pixels in the X range.
            *  Determine the Y range in each column.  This is done
            *  by checking all vertices and all intersections of
            *  lines against the left and right edge of the box.
            *
            *  (Following may be a zero-trip loop)  */

            for (ixp=lowx; ixp<=topx; ipx+=nsy2,ixp++) {
               float ylit = rly1;
               float ybig = 0.0;
               long lowy, topy;
               long index, ixe, iyp;
               for (ixe=ixp; ixe<=ixp+1; ixe++) {
                  int j;
                  float xe = (float)ixe;
                  for (j=0; j<=3; j++) {
                     float yint;
                     if ((xe-xb[j+1])*(xe-xb[j]) < 0.0) {
                        yint = yb[j] + (xe-xb[j])*slope[j];
                        if (yint < ylit) ylit = yint;
                        if (yint > ybig) ybig = yint;
                        }
                     if ((long)xb[j] == ixe) {
                        if (yb[j] < ylit) ylit = yb[j];
                        if (yb[j] > ybig) ybig = yb[j];
                        }
                     } /* End of j loop */
                  }  /* End of ixe loop */
               if (kintt) {
                  /* Use center and round to nearest pixel */
                  topy = (long)(0.5*(ylit + ybig + 1.0));
                  if (topy < 0 || topy >= RP->nsy) goto NXT_BOX;
                  lowy = topy;
                  }
               else {
                  /* Scan all pixels covered by the box */
                  lowy = max(ylit,0.0);
                  topy = min(ybig-1e-12,rly1);
                  }

               /* Loop over all pixels in this column that touch
               *  the current box.  Take touch value = largest
               *  value touched.  Do separately for each color.  */

               for (ipy=ipx+lowy,iyp=lowy; iyp<=topy; ipy++,iyp++)
                  /* Check z coord of related object (if any) */
                  if ((index = inpary[ipy+ly1]) /*Assignment intended*/
                        && (lzi >= envgob(RP0->Eptr, index)->iz)) {
                     register stim_mask_t tt = inpary[ipy];
                     if (kcol) {
                        register stim_mask_t vv;
                        vv = tt & 0xc0;   /* BLUE */
                        BB = max(BB,vv);
                        vv = tt & 0x38;   /* GREEN */
                        GG = max(GG,vv);
                        vv = tt & 0x07;   /* RED */
                        RR = max(RR,vv);
                        }
                     else
                        touch = max(touch,tt);
                     if (pmdlt) {
                        register struct OBJDEF *po =
                           envgob(RP0->Eptr, index);
                        d3mark1(pmdlt, po->isn, po->ign, po->iadapt);
                        }
                     } /* End if z in range */

               } /* End of ixp loop */

NXT_BOX:
            xb[0] = xb[1];
            yb[0] = yb[1];
            *pv++ = kcol ? (BB|GG|RR) : touch;
            } /* End of icol loop */

         xrow += dax;
         yrow += day;
         } /* End of irow loop */

/* If touch receptors are to be pressure-sensitive,
*  now find how many touches there were and scale accordingly.
*  Revised, 04/28/95, to normalize according to area of single
*  touch pad, not all touch pads on the arm.  As far as I know,
*  no existing runs were affected by this bug.  -GNR.  */

      if (pa->jatt & ARMPT) {
         long padarea = (int)pvc->nvx*(int)pvc->nvy;

         if (kcol) { /* Colored input array */
            long psbba = 0, psgga = 0, psrra = 0;
            long bb,gg,rr;
            for (ipv=pv0; ipv<pv; ipv++) {
               if (((int)*ipv & 0xc0) >= 0x80) psbba += 1;
               if (((int)*ipv & 0x38) >= 0x20) psgga += 1;
               if (((int)*ipv & 0x07) >= 0x04) psrra += 1;
               }
            psbba = (padarea<<Ss2hi)/(padarea+psbba);
            psgga = (padarea<<Ss2hi)/(padarea+psgga);
            psrra = (padarea<<Ss2hi)/(padarea+psrra);
            for (ipv=pv0; ipv<pv; ipv++) {
               bb = ((int)*ipv & 0xc0)*psbba + 0x200000 >> Ss2hi & 0xc0;
               gg = ((int)*ipv & 0x38)*psgga + 0x040000 >> Ss2hi & 0x38;
               rr = ((int)*ipv & 0x07)*psrra + 0x008000 >> Ss2hi & 0x07;
               *ipv = (s_type)(bb|gg|rr);
               }
            }

         else {      /* Black and white */
            long psarea = 0;
            for (ipv=pv0; ipv<pv; ipv++) if (*ipv >= 128) psarea++;
            if (psarea > 0) {
               register long psmult = (padarea<<Ss2hi)/(padarea+psarea);
               for (ipv=pv0; ipv<pv; ipv++) {
                  *ipv = (s_type)((((long)*ipv)*psmult)>>Ss2hi);
                  }
               }
            }

         }
NXT_JNT: xprv = pj->u.jnt.jx;
      yprv = pj->u.jnt.jy;
      }  /* End of pj loop */
   } /* End of d3vtch() */
