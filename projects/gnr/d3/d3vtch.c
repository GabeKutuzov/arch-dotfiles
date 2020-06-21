/* (c) Copyright 1989-2016, The Rockefeller University *11115* */
/* $Id: d3vtch.c 74 2017-09-18 22:18:33Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                               d3vtch                                 *
*                                                                      *
*  Evaluate virtual touch cells.  If environment is colored, largest   *
*     value of each color is accumulated separately, because multiple  *
*     conntypes with different color sensitivity may pick up VT input. *
*     Of course, using color for touch is really just a way to make    *
*     touch responses different from visual for same object.           *
*  WARNING: Current code only handles old 8-bit color IA.              *
*  WARNING: Returns immediately if there is no input array.            *
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
*                                                                      *
*  This code rewritten, 03/06/16, to do all working in fixed-point     *
*  coordinates.  This is to deal with a subtle problem that arose with *
*  floating-point coordinates:  A y value of 15.99999 was passed from  *
*  the caller.  When 1 was added to get another box vertex, this went  *
*  to 16.99999, which meant an increment in the exponent and hardware  *
*  rounding, yielding 17.000000 and a gap between two boxes.  Scale    *
*  S15 was chosen to allow enough integer bits (16) for the largest    *
*  allowed nsx,nsy IA size.  To deal with slopes too large for the     *
*  S24 scale chosen, those cases are handled by doing (dx*dboxy)/dboxx *
*  instead of dx*slope.                                                *
*                                                                      *
*  R66 code also checks IA boundaries on an individual scan basis, not *
*  if any vertex was outside, as in old code.  Results may differ.     *
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
*  R66, 02/29/16, GNR - Some longs to si32 for 64-bit compatibility    *
*  Rev, 03/05/16, GNR - Improve rounding for slopes near vertical      *
*  R66, 03/11/16, GNR - Rename nvx->nsxl, nvy->nsya so != ix->nv[xy],  *
*                       Add DBG_VTOUCH output                          *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "sysdef.h"
#include "d3global.h"
#include "jntdef.h"
#include "armdef.h"
#include "objdef.h"     /* (Needed to access object z coord) */
#include "rocks.h"
#include "rkxtra.h"
#include "rkarith.h"
#include "bapkg.h"

#define NBXE        4      /* Number of edges a box has */
#define NOEP        2      /* Number of opposite-edge pairs */
#define E02         0      /* Index for edges 0 and 2 */
#define E13         1      /* Index for edges 1 and 3 */
#define OFBIT       1      /* Bit to set for overflow */
#define RMxy ((1<<FBxy)-1) /* Mask to round coords to IA grid */
#define IGxy (1 << FBxy)   /* Increment a coord by 1 IA grid */
#define HGxy (1<<(FBxy-1)) /* Half grid (for rounding) (Sxy) */
#define FBtr       24      /* Fraction bits in trig ratios */
#define SafeSlope  64      /* Largest safe slope */
#define dStr 16777216.0    /* Scale of trig ratios */

void d3vtch(struct VCELL *pvc, stim_mask_t *inpary) {

   struct ARMDEF *pa = (struct ARMDEF *)pvc->pvcsrc;
   struct JNTDEF *pj = pa->pjnt;    /* Ptr to joints of this arm */
   struct JNTDEF *pje;              /* Ptr for end of joint loop */
   struct MODALITY *pmdlt;          /* Ptr to modality block */
   byte *pv, *pv0, *ipv;

   long oflag;                /* Overflow flag for big slopes */

   si32 dl, da;               /* Lateral, axial box size (Sxy) */
   si32 dlx, dly;             /* Projections of dl on x,y (Sxy) */
   si32 dax, day;             /* Projections of da on x,y (Sxy) */
   si32 jlen;                 /* Joint length (Sxy) */
   si32 lnta,lntl;            /* Number of axial,lateral units */
   si32 sinax, cosax;         /* Sin,cos axis relative to y (Str) */
   si32 abssin, abscos;       /* abs(sinax), abs(cosax) */
   si32 xb[NBXE], yb[NBXE];   /* Vertex coords of one pad box (Sxy) */
   si32 slope[NOEP];          /* Slopes of box edges (Str) */
   si32 xprv, yprv;           /* Previous end pad coords (Sxy) */
   si32 xcur, ycur;           /* Current end pad coords (Sxy) */
   si32 xrow, yrow;           /* Start of current row coords (Sxy) */

   si32 irow, icol;           /* Row, col counters */
   si32 lxm1,lym1;            /* nsx-1, nsy-1 (Sxy) */
   si32 lzi;                  /* Z coord of joint */
   si32 nsy,nsy2;             /* RP->nsy, 2*nsy */

   int dodebug;               /* TRUE to turn on debug output */
   int kcanm;                 /* TRUE if in canonical mode */
   int kintt;                 /* TRUE if touching only one pixel */
   int kcol;                  /* TRUE if environment colored */
   ui16 jbit;                 /* Test bit for jntinVT switches */
   enum { UseSlope, MulDiv02, MulDiv13, Vertical } kyact[NOEP];

   if (!RP->pbcst) return;    /* A little safety check */
   e64push(E64_FLAGS, &oflag);/* Overflow check setup */
   e64dec(OFBIT);

   /* Print title for debug output --
   *  Assignment intended in next line  */
   if ((dodebug = RP->CP.dbgflags & DBG_VTOUCH)) {
      char armnum[4];
      int  alen;
      wbcdwt(&pa->armno, armnum, RK_LFTJ+RK_IORF+RK_NBYTE+3);
      alen = RK.length;
      cryout(RK_P2, "0-->Debug output for ", RK_LN2, fmtvcns(pvc),
         RK_CCL, " arm ", 5, armnum, alen+1, NULL);
      }

/* Locate the modality block, only if used for XRSTATS */

   pmdlt = pvc->pvcmdlt;
   if (pmdlt && !(pmdlt->mdltflgs & MDLTFLGS_RQST)) pmdlt = NULL;

/* Clear the virtual touch cells for this arm */

   pv = RP->pbcst + pvc->ovc;
   memset((char *)pv, 0, pvc->nvcells);

/* A few handy constants */

   xprv = (si32)roundf(dSxy*pa->aattx);
   yprv = (si32)roundf(dSxy*pa->aatty);
   lxm1 = (si32)(RP->nsx - 1) << FBxy;
   lym1 = (si32)(RP->nsy - 1) << FBxy;
   nsy = (si32)RP->nsy, nsy2 = nsy + nsy;
   kcanm = pa->jatt & ARMTR;
   kintt = pa->jatt & ARMIT;
   kcol = RP->kcolor;

/* Loop over all joints on this arm */

   pje = pj + pa->njnts;
   for (jbit=1; pj<pje; jbit<<=1,++pj) {

      /* Needed as xprv,yprv even if this joint is skipped */
      xcur = (si32)roundf(dSxy*pj->u.jnt.jx);
      ycur = (si32)roundf(dSxy*pj->u.jnt.jy);

      /* Omit this joint if it is not used for VT */
      if (!(pvc->jntinVT & jbit)) goto NXT_JNT;

      /* Pick up dimensions that depend on the individual joint */
      jlen = (si32)roundf(dSxy*pj->jl);
      if (jlen == 0) goto NXT_JNT;
      lnta = (si32)pj->u.jnt.nta,  lntl = (si32)pj->u.jnt.ntl;

/* Compute sine and cosine of axial orientation of touch box */

      if (kcanm) {
         cosax = 0;
         sinax = dStr; }
      else {
         /* These cannot overflow, so oflag not tested */
         cosax = dsrsjqd(xprv-xcur, FBtr, jlen);
         sinax = dsrsjqd(yprv-ycur, FBtr, jlen); }

/* Determine what method to use to get y-axis bounds for
*  horizontal scan lines depending on magnitude of slope.  */

      abssin = abs32(sinax), abscos = abs32(cosax);

      /* Set up for slope on edges 0 and 2 (cos/sin) */
      if (abssin == 0)
         kyact[E02] = Vertical;
      else if (abscos < SafeSlope*abssin) {
         slope[E02] = dsrsjqd(cosax, FBtr, -sinax);
         kyact[E02] = UseSlope; }
      else
         kyact[E02] = MulDiv02;

      /* Set up for slope on edges 1 and 3 (sin/cos) */
      if (abscos == 0)
         kyact[E13] = Vertical;
      else if (abssin < SafeSlope*abscos) {
         slope[E13] = dsrsjqd(sinax, FBtr, cosax);
         kyact[E13] = UseSlope; }
      else
         kyact[E13] = MulDiv13;

/* Compute axial and lateral sizes of the small boxes.
*  Compute their projections along the rows and columns.  */

      da = pj->u.jnt.tla/lnta;
      dl = pj->u.jnt.tll/lntl;
      dax = mrssl(da, cosax, FBtr);
      day = mrssl(da, sinax, FBtr);
      dlx = mrssl(dl, sinax, FBtr);
      dly = -mrssl(dl, cosax, FBtr);

/* Compute location of upper left hand corner of tactile box */

      xrow = xcur - lntl*dlx/2, yrow = ycur - lntl*dly/2;
      if (kcanm) yrow -= pj->u.jnt.tla/2;

      if (dodebug) {
         int jnum = pj - pa->pjnt + 1;
         convrt("(P2,4X,'Joint ',J0I4,', x = ',J0F10.3,', y = ',J0F"
            "10.3,', cosax = ',J0B24IJ10.6,', sinax = ',J0B24IJ10.6)",
            &jnum, &pj->u.jnt.jx, &pj->u.jnt.jy, &cosax, &sinax, NULL);
         for (jnum=0; jnum<NOEP; ++jnum) {
            switch (kyact[jnum]) {
            case Vertical:
               convrt("(P2,6X,'Side ',J0I4,' is vertical')",
                  &jnum, NULL);
               break;
            case UseSlope:
               convrt("(P2,6X,'Side ',J0I4,' has slope ',J0B24IJ8.A)",
                  &jnum, slope+jnum, NULL);
               break;
            default:
               convrt("(P2,6X,'Side ',J0I4,' uses MulDiv method')",
                  &jnum, NULL);
               break;
               } /* End kyact switch */
            } /* End loop over two sides of box */
         convrt("(P2,6X,'ULHC is at x = ',J0B15IJ10.3,',"
            " y = ',J0B15IJ10.3)", &xrow, &yrow, NULL);
         } /* End joint debug output */

/* Loop over the lnta boxes along the axial direction.
*  Loop over the lntl boxes along the lateral direction.  */

      pv0 = pv;
      lzi = pj->u.jnt.tzi;
      for (irow=0; irow<lnta; irow++) {
         xb[0] = xrow;
         yb[0] = yrow;
         for (icol=0; icol<lntl; icol++) {
            si32 xlit, xbig, ylit, ybig;
            si32 jgx;
            int i, j, ixp;
            stim_mask_t touch;         /* B&W touch value */
            stim_mask_t BB,GG,RR;      /* Colored touch values */
            touch = BB = GG = RR = 0;

            /* Locate the four corners of the current small box */

            xb[1] = xb[0] + dlx;
            yb[1] = yb[0] + dly;
            xb[2] = xb[1] + dax;
            yb[2] = yb[1] + day;
            xb[3] = xb[0] + dax;
            yb[3] = yb[0] + day;

            if (dodebug) convrt("(P2,8X,'Checking small box at '"
                  "4<J0B15IJ10.3,2H, JB15IJ10.3,3X>)", xb, yb, NULL);

            /* Find smallest and largest X(IA) coordinate */

            xlit = lxm1, xbig = 0;
            ylit = lym1, ybig = 0;
            for (i=0; i<NBXE; i++) {
               if (xb[i] < xlit) xlit = xb[i];
               if (xb[i] > xbig) xbig = xb[i];
               if (yb[i] < ylit) ylit = yb[i];
               if (yb[i] > ybig) ybig = yb[i];
               }
            if (kintt) {
               /* Use center of box and round to nearest pixel */
               ylit = ybig = (ylit + ybig + HGxy) >> 1;
               xlit = xbig = (xlit + xbig + HGxy) >> 1;
               }
            /* Stay inside the IA */
            if (xbig < 0 || xlit > lxm1) goto NXT_BOX;
            if (xlit < 0) xlit = 0;
            if (xbig > lxm1) xbig = lxm1;

/*---------------------------------------------------------------------*
*    Three levels of indenting suppressed, here to end of jgx loop     *
*---------------------------------------------------------------------*/

   /* Loop over the columns of pixels in the X range.  Determine the Y
   *  range in each column.  This is done by checking all intersections
   *  of lines of constant x against the four edges of the box that are
   *  inside the limits established by the four vertices of the box. */
   xlit += RMxy;              /* Round up to next grid line in box */
   ixp = nsy2*(xlit >> FBxy);
   /* (Following may be a zero-trip loop) */
   for (jgx=xlit&(~RMxy); jgx<=xbig; ixp+=nsy2,jgx+=IGxy) {
      si32 yclit, ycbig;      /* y limits in this column */
      si32 jgylo, jgyhi;      /* y limits (integer grids) */
      si32 index, jgy;
      if (kintt)
         yclit = ycbig = ylit;
      else {
         /* y limits for intersections with edges:
         *  Loop over two slopes, check 2 edges each */
         ycbig = ylit, yclit = ybig;
         for (j=E02; j<=E13; j++) {
            si32 ty1,ty2;
            switch (kyact[j]) {
            case UseSlope:
               ty1 = yb[ j ] + mrssl(jgx - xb[ j ], slope[j], FBtr);
               ty2 = yb[j+2] + mrssl(jgx - xb[j+2], slope[j], FBtr);
               break;
            case MulDiv02:
               /* It is not necessary to test whether oflag is set--
               *  jdswqd sets overflow result to +/- SI32_MAX, which
               *  is always out-of-bounds.  */
               ty1 = yb[ j ] + dmsjqd(jgx - xb[ j ], cosax, -sinax);
               ty2 = yb[j+2] + dmsjqd(jgx - xb[j+2], cosax, -sinax);
               break;
            case MulDiv13:
               ty1 = yb[ j ] + dmsjqd(jgx - xb[ j ], sinax, cosax);
               ty2 = yb[j+2] + dmsjqd(jgx - xb[j+2], sinax, cosax);
               break;
            case Vertical:
               ty1 = ylit;
               ty2 = ybig;
               break;
               } /* End kyact switch */
            /* Here we want the largest and smallest ty values that
            *  are inside the box.  Due to various orientations of the
            *  box, they could be any of the four intersections.  */
            if (ty1 >= ylit && ty1 <= ybig) {
               if (ty1 > ycbig) ycbig = ty1;
               if (ty1 < yclit) yclit = ty1; }
            if (ty2 >= ylit && ty2 <= ybig) {
               if (ty2 > ycbig) ycbig = ty2;
               if (ty2 < yclit) yclit = ty2; }
            } /* End loop over two slopes */
         } /* End !kintt */

      /* Stay inside the IA */
      if (ycbig < 0 || yclit > lym1) goto NXT_BOX;
      if (yclit <    0) yclit =    0;
      if (ycbig > lym1) ycbig = lym1;
      yclit += RMxy;       /* Round up to next grid line in box */
      jgylo = yclit >> FBxy;
      jgyhi = ycbig >> FBxy;

      if (dodebug)
         convrt("(P2,8X,'For x at ',J0B15IJ10.0,', y range is '"
            "J0IJ10,' to ',J0IJ10)", &jgx, &jgylo, &jgyhi, NULL);


      /* Loop over all pixels in this column that touch
      *  the current box.  Take touch value = largest
      *  value touched.  Do separately for each color.
      *  (SRA not needed here, all coords are positive.)  */
      for (jgy=jgylo; jgy<=jgyhi; ++jgy) {
         /* Check z coord of related object (if any) */
         if ((index = inpary[jgy+ixp+nsy])   /* Assignment intended */
               && (lzi >= envgob(RP0->Eptr, index)->iz)) {
            stim_mask_t tt = inpary[jgy+ixp];
            if (kcol) {
               register stim_mask_t vv;
               vv = tt & 0xc0;   /* BLUE */
               BB = max(BB,vv);
               vv = tt & 0x38;   /* GREEN */
               GG = max(GG,vv);
               vv = tt & 0x07;   /* RED */
               RR = max(RR,vv);

               if (dodebug) convrt("(P2,8X,'At y = ',J0IJ10,', IA is '"
                  "ZC5,', BGR is',3*ZC5)", &jgy, &tt, &BB, &GG, &RR,
                  NULL);
               }
            else {
               touch = max(touch,tt);

               if (dodebug) convrt("(P2,8X,'At y = ',J0IJ10,', IA is '"
                  "ZC5)", &jgy, &tt, NULL);
               }
            if (pmdlt) {
               register struct OBJDEF *po =
                  envgob(RP0->Eptr, index);
               d3mark1(pmdlt, po->isn, po->ign, po->iadapt);
               }
            } /* End if z in range */
         } /* End jgy loop */
      } /* End of jgx loop */

/*---------------------------------------------------------------------*
*                 End three-level indent suppression                   *
*---------------------------------------------------------------------*/

NXT_BOX:    xb[0] = xb[1];
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
         si32 padarea = lnta * lntl;

         if (kcol) { /* Colored input array */
            si32 psbba = 0, psgga = 0, psrra = 0;
            si32 bb,gg,rr;
            for (ipv=pv0; ipv<pv; ipv++) {
               if (((int)*ipv & 0xc0) >= 0x80) psbba += 1;
               if (((int)*ipv & 0x38) >= 0x20) psgga += 1;
               if (((int)*ipv & 0x07) >= 0x04) psrra += 1;
               }
            psbba = dsrsjqd(padarea, Ss2hi, padarea+psbba);
            psgga = dsrsjqd(padarea, Ss2hi, padarea+psgga);
            psrra = dsrsjqd(padarea, Ss2hi, padarea+psrra);
            for (ipv=pv0; ipv<pv; ipv++) {
               bb = ((int)*ipv & 0xc0)*psbba + 0x200000 >> Ss2hi & 0xc0;
               gg = ((int)*ipv & 0x38)*psgga + 0x040000 >> Ss2hi & 0x38;
               rr = ((int)*ipv & 0x07)*psrra + 0x008000 >> Ss2hi & 0x07;
               *ipv = (s_type)(bb|gg|rr);
               }
            }

         else {      /* Black and white */
            si32 psarea = 0;
            for (ipv=pv0; ipv<pv; ipv++) if (*ipv >= 128) psarea++;
            if (psarea > 0) {
               si32 psmult = dsrsjqd(padarea, Ss2hi, padarea+psarea);
               for (ipv=pv0; ipv<pv; ipv++) {
                  *ipv = (s_type)((((si32)*ipv)*psmult)>>Ss2hi);
                  }
               }
            }

         }
NXT_JNT: xprv = xcur, yprv = ycur;
      }  /* End of pj loop */

   e64pop();                  /* Resume caller's overflow handling */
   } /* End of d3vtch() */
