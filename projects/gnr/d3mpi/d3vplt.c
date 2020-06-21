/* (c) Copyright 1993-2016, The Rockefeller University *11115* */
/* $Id: d3vplt.c 77 2018-03-15 21:08:14Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                              d3vplt.c                                *
*                                                                      *
*      Virtual cell (sense cell) printing and plotting routines        *
*     ---These functions should be called from host node only---       *
*                                                                      *
*  Routine to plot a sensor array:                                     *
*  int d3vplt(struct VCELL *pvc, si32 itr, float xl, float yl,         *
*     float width, float height)                                       *
*                                                                      *
*  pvc:    Ptr to VCELL block to be plotted                            *
*  itr:    Trial number (negative for config plots)                    *
*  xl,yl:  X,Y coords of lower left corner                             *
*  width:  Plot width                                                  *
*  height: Plot height                                                 *
*                                                                      *
*  Return value:  Return code from d3ngph, i.e. outnow code.           *
*                                                                      *
*  N.B.  This function currently runs serially on host node. The idea  *
*  is that it will not create too much of a bottleneck because it can  *
*  work while other nodes are plotting normal repertoires.  However,   *
*  if writing a parallel version, remember that VCELL blocks are       *
*  currently available only on host.  Function assumes it will not     *
*  be called at all if VKPPL flag is off.                              *
*                                                                      *
*  N.B.  Cells within a touch pad are plotted in the same order as     *
*  real cells and pixels in the input array, viz. top-to-bottom.       *
*  However, multiple touch pads are plotted proximal down, distal up,  *
*  to match the order of joints when the shoulder is at the bottom.    *
*                                                                      *
*  Routine to print all sensor arrays:                                 *
*  void d3snsprt(void)                                                 *
*                                                                      *
************************************************************************
*  V6A, 07/24/93, GNR - Initial version, based on d3rplt()             *
*  V7A, 04/30/94, GNR - Plot long name from VCELL block                *
*  V8A, 04/17/95, GNR - Changes for plotting VJ,VW,VT                  *
*  V8C, 08/09/03, GNR - Cell responses in millivolts, add conductances *
*  V8D, 05/04/07, GNR - Allow for R4 sense data                        *
*  Rev, 02/05/08, GNR - Move sense print from main, improve format     *
*  ==>, 02/28/08, GNR - Last mod before committing to svn repository   *
*  V8H, 11/08/10, GNR - Add KP=I to plot isn,ign below image (debug)   *
*  R66, 03/11/16, GNR - Rename nvx->nsxl, nvy->nsya so != ix->nv[xy]   *
*                       Plot multiple active joint pads if present     *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "sysdef.h"
#include "d3global.h"
#include "d3fdef.h"
#include "armdef.h"
#include "jntdef.h"
#include "rocks.h"
#include "rkxtra.h"
#include "bapkg.h"
#include "plots.h"
#include "swap.h"

/*=====================================================================*
*                               d3vplt                                 *
*=====================================================================*/

int d3vplt(struct VCELL *pvc, si32 itr, float xl, float yl,
   float width, float height) {

   struct JNTDEF *pj;            /* Joint access if VT_SRC */
   float *psf;                   /* Ptr to vsi data (if float) */
   byte  *psi;                   /* Ptr to vsi data (if byte) */

   float cdx,cdy;                /* Cell spacing along x,y */
   float fgd,xgi,ygi;            /* Grid increments */
   float lcpt;                   /* Plot value threshold (S14) */
   float ccpt;                   /* SEQNUM/(1.0 (S14) - lcpt) */
   float scale;                  /* s(i) multiplier to get radius   */
   float si,abssi;               /* Cell activity (scaled to S14)   */
   float xh,yh;                  /* x,y coords of right,top edges   */
   float xc,yc;                  /* x,y coords of cell or grid line */

   int doclsq;                   /* Do color sequence switch */
   int iamR4;                    /* TRUE if VKR4 flag is set */
   int ic,jc;                    /* Color selectors */
   int kfill,krt,ksqre;          /* Fill, retrace, square switches */
   int knegs;                    /* TRUE if VKPNA flag is set */
   int lkvp;                     /* Option switches */
   int mxic,mxicm1;              /* Max value of ic, mxic-1 */
   int npads;                    /* Number of pads to plot */

   ui16 jbit;                    /* VT jntinVT selector bit */

/* Sanity check: jump out if no cells to plot or h,w not specified */

   if (!pvc->nvcells || width <= 0.0 || height <= 0.0) return D3_NORMAL;

/* Activate timing statistics */

   stattmr(OP_PUSH_TMR, GRPH_TMR);

/* Set option switches */

   lkvp   = (int)pvc->kvp;
   doclsq = (ssmatch(RP->colors[CI_BUB],"SEQUENCE",3) != 0);
   iamR4  = (pvc->vcflags & VKR4) != 0;
   kfill  = lkvp & VKPCO;        /* Draw color fills */
   knegs  = (lkvp & VKPNA) != 0;
   krt    = (lkvp & VKPPR | RP->kplot & PLRTR) ?
      RETRACE : NORETRACE;       /* Retrace plots    */
   ksqre  = (lkvp & VKPSS | RP->kplot & PLSQR);
                                 /* Plot squares */

/* Calculate geometric plotting constants */

   cdx = width/(float)pvc->nsxl;    /* Cell delta x */
   cdy = height/(float)pvc->nsya;   /* Cell delta y */
   xh  = xl + width;             /* x coord of right edge of plot */
   yh  = yl + height;            /* y coord of top edge of plot   */
   if (lkvp & VKPPG) {
      fgd = (float)RP0->grids;
      xgi = cdx*fgd;             /* x grid interval */
      ygi = cdy*fgd;             /* y grid interval */
      }

/* Start plot, set retrace switch, draw title string */

   /* If itr < 0, this is a view or configuration plot, omit title.
   *  If this is not the case, be careful not to call pencol or
   *  retrace until after d3ngph.  */
   if (itr < 0)
      ksqre = TRUE;
   else {
      float lettht = RP->stdlht;
      float ylabel = yl - lettht, lhtu = 0.7*lettht;
      if (!(RP->kplot & (PLSUP|PLVTS|PLDYN))) {
         /* Full title (not a superposition plot) */
         int ngrc = d3ngph(8.5, 11.0, 0, 0, "DEFAULT",
            RP->colors[CI_BOX],"DEFAULT", 0);
         if (ngrc > 0) {
            stattmr(OP_POP_TMR, GRPH_TMR);
            return ngrc;
            }
         /* Plot series/trial and repertoire titles */
         retrace(krt);
         if (!(lkvp & VKPOM)) {
            char *pvcn = fmtvcn(pvc);
            symbol(0.5, 11.0-lettht, lhtu, RP0->stitle+1,
               0.0, RP0->lst-1);
            symbol(xl, ylabel, lhtu, pvcn, 0.0, strlen(pvcn));
            } /* End title */
         } /* End non-superposition (full) plot title */
      else {
         /* Repertoire title only (is a superposition plot) */
         retrace(krt);
         pencol(RP->colors[CI_BOX]);
         if (!(lkvp & VKPOM)) {
            char *pvcn = fmtvcns(pvc);
            int   lvcn = strlen(pvcn);
            symbol(xl, ylabel, lhtu, pvcn, 0.0, lvcn);
            ylabel -= lettht;
            } /* End title */
         if (lkvp & VKPISG) {    /* Plot isn,ign */
            char ltxt[20];
            sconvrt(ltxt, "(4HS/G ,2*J1UH6)",
               &pvc->isn, &pvc->ign, NULL);
            symbol(xl, ylabel, lhtu, ltxt, 0.0, strlen(ltxt));
            }
         } /* End superposition title */
      } /* End if itr */

   /* Threshold and scale to test an S14 activity level and
   *  convert it to a circle radius.  Avoid divide by 0.  */
   lcpt = (float)pvc->vcpt;
   mxic = SEQNUM >> knegs, mxicm1 = mxic - 1;
   ccpt = (float)mxic;
   scale = dS14 - lcpt;
   if (scale <= 1.0) scale = 1.0;
   else              ccpt /= scale;
   scale = (0.5/dS14)*RP0->GCtD.cpr*min(cdx,cdy);

   /* Locate s(i) data */
   if (iamR4) psf = RP->paw + pvc->ovc;
   else       psi = RP->pbcst + pvc->ovc;

/* Determine how many pads are to be drawn and loop over them.
*  (If this is not VT_SRC, npads == 0 and do loop runs once.)  */

   npads = (int)bitcnt(&pvc->jntinVT, sizeof(ui16));
   if (pvc->vtype == VT_SRC) {
      /* Items needed for VT_SRC walk over touch pads */
      pj = pvc->pjnt0;
      jbit = pvc->jntinVT;
      }
   do {
      float dbox, xlo, xlob, xhib, yhi, yhib;
      int  ilc,lnxl,nlvc;        /* Items for cell counts */
      /* Make box narrower if ntl is smaller */
      if (pvc->vtype == VT_SRC) {
         while (jbit && !(jbit & 1)) ++pj, jbit>>=1;
         if (!jbit) break;
         dbox = 0.5*cdx*(float)(pvc->nsxl - pj->u.jnt.ntl);
         xlob = xl + dbox, xhib = xh - dbox;
         yhib = yl + cdy*(float)pj->u.jnt.nta;
         lnxl = (int)pj->u.jnt.ntl;
         nlvc = lnxl * (int)pj->u.jnt.nta;
         ++pj, jbit>>=1;         /* Ready for next pad */
         }
      else {
         dbox = 0;
         xlob = xl, xhib = xh;
         yhib = yh;
         lnxl = (int)pvc->nsxl;
         nlvc = (int)pvc->nvcells;
         }
      xlo = xlob + 0.5*cdx;      /* x coord of first cell */
      yhi = yhib - 0.5*cdy;      /* y coord of first cell */

      /* Draw the box around the pad */
      retrace(krt);
      pencol(RP->colors[CI_BOX]);
      rect(xlob,yl,width - dbox,height,THIN);

      /* Draw grid lines.
      *  In all cases, CI_BOX color is set from above code.  */
      if (lkvp & VKPPG) {
         float xmx = xhib - 0.5*cdx;  /* Safe test for end of x grids */

         /* Do not retrace grids */
         if (krt) retrace(OFF);

         /* Draw x (vertical) grid lines */
         for (xc=xlob+xgi; xc<xmx; xc+=xgi) line(xc,yl,xc,yh);

         /* Draw y (horizontal) grid lines */
         for (yc=yh+ygi; yc<yhi; yc+=ygi) line(xlob,yc,xhib,yc);
         } /* End grid lines */

      /* Set retrace mode and switch to initial drawing color.
      *  If color sequence, use jc to remember previous color.  */
      retrace(krt);
      if (doclsq) jc = -1;
      else pencol(RP->colors[CI_BUB]);

/* Loop over cells */

      for (ilc=0; ilc<nlvc; ++ilc) {

         if (iamR4) si = RP->fvcscl * (*psf++);
         else       si = (float)((ui32)*psi++ << Sv2mV);

         abssi = knegs ? fabsf(si) : si;
         if (abssi >= lcpt) {
            /* Calc plot radius and diameter */
            float r,d;              /* Radius, diameter of bubble */
            ldiv_t qrm;
            /* Limit big symbols for big si, not too small either */
            r = scale*max(abssi, 2.0);
            if (r < RP->mnbpr) r = RP->mnbpr;
            d = 2.0*r;
            /* Calc location of cell */
            qrm = ldiv(ilc, lnxl);
            xc = xlo + qrm.rem*cdx;
            yc = yhi - qrm.quot*cdy;
            /* Set plot color */
            if (doclsq) {   /* Color sequence */
               ic = (int)(ccpt*(abssi-lcpt));
               /* Note, 11/16/10, GNR:  Now that ccpt is guaranteed to
               *  be positive, and abssi < lcpt does't get here, old
               *  test for ic < 0 is unnecessary and has been removed.
               */
               if (ic > mxicm1) ic = mxicm1;
               if (knegs) {      /* Dual pos-neg si scale */
                  if (si < 0.0) ic = mxicm1 - ic;
                  else          ic += mxic;
                  }
               if (ic != jc) pencol(RP->colors[CI_SEQ+(jc=ic)]);
               }
            /* Plot the cell */
            if (kfill)       rect(xc-r,yc-r,d,d,FILLED);
            else if (!ksqre) circle(xc,yc,r,THIN);
            else             rect(xc-r,yc-r,d,d,THIN);
            } /* End if above threshold */

         } /* End cell loop */

      yl = yh;                /* Up to next pad */
      } while (--npads > 0);  /* End pads loop */

   stattmr(OP_POP_TMR, GRPH_TMR);
   return D3_NORMAL;
   } /* End d3vplt() */

/*=====================================================================*
*                              d3snsprt                                *
*=====================================================================*/

#define SNSPROW   10          /* Sensors per row (for max 96 cols) */
#define qSNSPROW "10"

void d3snsprt(void) {

   struct VCELL *pvc;         /* Ptr to current VCELL block */
   char *plbl;                /* Array or pad label */
   static char kpsnfmtr[] = "(P2,1X,A16#|(" qSNSPROW "F8.<4))";
   static char kpsnfmti[] = "(P2,1X,A16#|(" qSNSPROW "B8IC8.4))";
   static char blnm[] = "    ";

   if (RP0->pvc1) {
      for (pvc=RP0->pvc1; pvc; pvc=pvc->pnvc) {
         plbl = fmtvcns(pvc);
         if (pvc->vcflags & VKUSED) {
            struct JNTDEF *pj;/* Joint access if VT_SRC */
            size_t ovcd;      /* Offset to cell data */
            int  lnxl;        /* Local copy of nsxl */
            int  irow,nrow;   /* Sensor row loop control */
            int  npads;       /* Number of pads to print */
            int  rowlen;      /* Items in one full row */
            ui16 jbit;        /* VT jntinVT selector bit */
            if (pvc->nsxl <= 0 || pvc->nsya <= 0) continue; /* TILT */
            if (RP->kdprt & PRSPT) spout(1);
            cryout(RK_P2, "0Sensory array data for ", RK_LN2,
               plbl, RK_CCL, NULL);
            npads = (int)bitcnt(&pvc->jntinVT, sizeof(ui16));
            ovcd = pvc->ovc;
            if (pvc->vtype == VT_SRC) {
               pj = pvc->pjnt0;
               jbit = pvc->jntinVT;
               }
            do {
               if (pvc->vtype == VT_SRC) {
                  while (jbit && !(jbit & 1)) ++pj, jbit>>=1;
                  if (!jbit) break;
                  lnxl = (int)pj->u.jnt.ntl;
                  nrow = (int)pj->u.jnt.nta;
                  irow = (int)(pj - pvc->pjnt0) + 1;
                  sconvrt(plbl,"('   Touch pad',J1I4)", &irow, NULL);
                  ++pj, jbit>>=1;   /* Ready for next pad */
                  }
               else {
                  lnxl = (int)pvc->nsxl;
                  nrow = (int)pvc->nsya;
                  plbl = blnm;
                  }
               rowlen = min(lnxl, SNSPROW);
               if (RP->kdprt & PRSPT)
                  spout(nrow*((lnxl + rowlen - 1)/rowlen));
               /* Print one row of cells, may be > 1 rows of print */
               if (pvc->vcflags & VKR4)
                     for (irow=0; irow<nrow; ++irow) {
                  convrt(kpsnfmtr, plbl, &lnxl,
                     (float *)RP->paw + ovcd, NULL);
                  plbl = blnm; ovcd += (size_t)lnxl;
                  }
               else for (irow=0; irow<nrow; ++irow) {
                  convrt(kpsnfmti, plbl, &lnxl,
                     RP->pbcst + ovcd, NULL);
                  plbl = blnm; ovcd += (size_t)lnxl;
                  }
               } while (--npads > 0);  /* End pads loop */
            }
         else {   /* Not used */
            /* Use convrt, not cryout, for easier expansion of
            *  sensor name to 16 chars to match format above.  */
            if (RP->kdprt & PRSPT) spout(1);
            convrt("(1X,A16,' Not printed--No data available')",
               plbl, NULL);
            }
         } /* End sense loop */
      } /* End if pvc1 */

   } /* End d3snsprt() */
