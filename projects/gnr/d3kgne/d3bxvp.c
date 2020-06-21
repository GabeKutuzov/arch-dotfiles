/* (c) Copyright 1992-2009 Neurosciences Research Foundation, Inc. */
/* $Id: d3bxvp.c 48 2012-04-23 18:40:15Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                               d3bxvp                                 *
*                                                                      *
*  Plot a boxsum array for verifying gconn boundary conditions         *
*                                                                      *
*  Argument: ib = pointer to current INHIBBLK                          *
*                                                                      *
*  Since this is a debug routine, the size and position of the         *
*     graph are invariably set to make a full-screen plot.             *
*                                                                      *
************************************************************************
*                                                                      *
*  Written by George N. Reeke                                          *
*  V4A, 3/2/89 Translated to C from version V2A                        *
*  Rev, 05/09/92, GNR - Use new ROCKS plot calls for atomization       *
*  Rev, 06/29/92, ABP - Include d3fdef.h, fix line[] vs line() bug     *
*  Rev, 09/29/96, GNR - Handle dynamic boxsum scaling, simplify call,  *
*                       fix bug: boxsums have nel, not just one term   *
*  V8C, 08/09/03, GNR - Cell responses in millivolts, add conductances,*
*                       remove d3fdef.h and BVMXRAD max radius limit.  *
*  ==>, 10/29/07, GNR - Last mod before committing to svn repository   *
*  Rev, 12/31/08, GNR - Replace jm64sl with mssle                      *
*  Rev, 01/17/09, GNR - Reorganize colors into a single array          *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "d3global.h"
#include "celldata.h"
#include "rocks.h"
#include "rkarith.h"
#include "plots.h"

extern struct CELLDATA CDAT;

int d3bxvp(struct INHIBBLK *ib) {

   float height = 10.0;       /* Height of drawn area (inches) */
   float width = 10.0;        /* Width of drawn area (inches) */
   float lettht = RP->stdlht; /* Standard letter height */
   float scale;               /* Scale factor for boxsum */
   float gdx, gdy;            /* Group delta X, group delta Y */
   float xnr;                 /* Number of rings - 1 */
   float xb, yb;              /* Size of overhang border along x,y */
   float xg, yg;              /* Coords of group being plotted */
   float xlo, yhi;            /* Plotting coords of first cell */
   float xl=0.5,yl=0.5,xh,yh; /* X,Y coordinates of border */
   long *bptr;                /* Boxsum pointer */
   long lngx = ib->l1xn2;     /* Number of boxes along x */
   long lngy = ib->l1yn2;     /* Number of boxes along y */
   long csdiv;                /* Color scale divisor */
   long mxsum;                /* Max sum out at 128mV */
   long thresh;               /* Threshold boxsum for plotting */
   int ic,jc;                 /* Color of current,previous cell */
   int igx, igy;              /* X,Y loop counters */
   int ngrc;                  /* Return code from newplt */
   char bline[56];            /* Conversion buffer for sconvrt */
   char doclsq;               /* TRUE if plotting a color sequence */

/* Calculate plotting intervals and origin along X and Y */

   xh = xl + width;
   yh = yl + height;
   gdx = width/(float)lngx;
   gdy = height/(float)lngy;
   xlo = xl + 0.5*gdx;
   yhi = yh - 0.5*gdy;

/* Start graph, draw borders.  N.B.  Could take lname
*  from ib->gsrcnm, but may not be there forever.  */

   ngrc = d3ngph(width, height, 0, 0,
      "DEFAULT", RP->colors[CI_BOX], "DEFAULT", 0);
   if (ngrc > 0) return ngrc;
   retrace(OFF);
   symbol(xl, 11.0-lettht, 0.7*lettht, RP0->stitle+1,
      0.0, RP0->lst-1);
   sconvrt(bline, "('BOXSUMS FOR ',J0A24,', GCONN TYPE ',J0IH6)",
      fmturlnm(CDAT.cdlayr), &ib->igt, NULL);
   symbol(xl, yl-lettht, 0.7*lettht, bline, 0, strlen(bline));
   rect(xl, yl, width, height, NORETRACE);

/* Draw lines delineating the border areas */

   xnr = (float)ib->l1n1;
   xb = xnr*gdx;
   yb = xnr*gdy;
   line(xl, yl+yb, xh, yl+yb);
   line(xh, yh-yb, xl, yh-yb);
   line(xl+xb, yh, xl+xb, yl);
   line(xh-xb, yl, xh-xb, yh);

/* Switch to drawing color unless it is a sequence */

   doclsq = !strcmp(RP->colors[CI_BUB], "SEQUENCE");
   if (doclsq) jc = -1;
   else pencol(RP->colors[CI_BUB]);

/* Prepare constants for scaling boxsums, taking care to avoid
*     overflow when combining srcnel and binary scales.
*  Max term in a boxum is S14+S16 >> ihsf and there are lnel terms.
*  Note: thresh > mxsum is not a problem, can still plot biggies. */

   {  si32 lnel = ib->pisrc->nel;
      int lihsf = -(int)ib->ihsf;
      mxsum  = mssle(S30, lnel, lihsf, OVF_GEN);
      thresh = mssle((si32)RP0->GCtD.cpt<<Ss2hi, lnel, lihsf, OVF_GEN);
      csdiv  = mxsum - thresh;
      if (csdiv <= 0) csdiv = 1;       /* Avoid divide by zero */
      } /* End lnel,lihsf local scope */
   /* Scale to convert an S14 sum to a max size circle */
   scale = 0.5*RP0->GCtD.cpr*
      (RP->cpdia ? RP->cpdia : min(gdx,gdy))/(float)mxsum;
   mxsum = max(mxsum-1,thresh+1);      /* Keeps 0 <= ic < SEQNUM */

/* Loop over boxsums in vertical, then horizontal direction.
*  Draw circles for boxsums over cpt threshold.
*  When doing a color sequence, use jc to remember previous color.  */

   bptr = ib->boxsum;
   for (yg=yhi,igy=1; igy<=lngy; igy++,yg-=gdy) {
      for (xg=xlo,igx=1; igx<=lngx; igx++,xg+=gdx) {
         float r;                      /* Circle radius */
         long sum = labs(*bptr);       /* Positive box sum */
         if (sum >= thresh) {
            sum = min(sum, mxsum);     /* Control largest circle */
            r = scale * (float)sum;    /* Set circle radius */
            r = max(r, MINBPRAD);      /* Expand to minimum radius */
            if (doclsq) {              /* Set color if sequence */
               ic = ((sum - thresh)<<SEQN_EXP)/csdiv;
               if (ic > (SEQNUM-1)) ic = (SEQNUM-1);
               ic += CI_SEQ;
               if (ic != jc) pencol(RP->colors[jc=ic]);
               }
            circle(xg,yg,r,NORETRACE); /* Draw circle */
            }
         bptr++;
         } /* End x loop */
      } /* End y loop */

   return D3_NORMAL;
   } /* End d3bxvp() */
