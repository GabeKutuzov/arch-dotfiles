/* (c) Copyright 1989-2016, The Rockefeller University *21115* */
/* $Id: d3bxvp.c 77 2018-03-15 21:08:14Z  $ */
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
*  Rev, 12/24/12, GNR - New overflow checking                          *
*  Rev, 08/25/13, GNR - Go to 64-bit sums, eliminate ihsf, mxsum test, *
*                       restore max radius limit at min(gdx,gdy)       *
*  R67, 10/22/16, GNR - In a parallel computer, use on PAR0 node       *
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

#ifdef PARn
#error d3bxvp cannot be included on PARn nodes.
#else

extern struct CELLDATA CDAT;

int d3bxvp(struct INHIBBLK *ib) {

   si64 *bptr;                /* Boxsum pointer */
   si64 mxsum;                /* Max sum out at 128mV */
   si64 thresh;               /* Threshold boxsum for plotting */
   si32 csdiv;                /* Color scale divisor */
   si32 ic,jc;                /* Color of current,previous cell */
   si32 lnel;                 /* Number of cells in a box */
   float height = 10.0;       /* Height of drawn area (inches) */
   float width = 10.0;        /* Width of drawn area (inches) */
   float lettht = RP->stdlht; /* Standard letter height */
   float scale;               /* Scale factor for boxsum -> radius */
   float gdx, gdy;            /* Group delta X, group delta Y */
   float rmx;                 /* Max bubble radius */
   float xnr;                 /* Number of rings - 1 */
   float xb, yb;              /* Size of overhang border along x,y */
   float xg, yg;              /* Coords of group being plotted */
   float xlo, yhi;            /* Plotting coords of first cell */
   float xl=0.5,yl=0.5,xh,yh; /* X,Y coordinates of border */
   int idsf;                  /* Division scale factor */
   ui32 igx, igy;             /* X,Y loop counters */
   ui32 lngx = ib->l1xn2;     /* Number of boxes along x */
   ui32 lngy = ib->l1yn2;     /* Number of boxes along y */
   int ngrc;                  /* Return code from newplt */
   char bline[56];            /* Conversion buffer for sconvrt */
   char doclsq;               /* TRUE if plotting a color sequence */

/* Calculate plotting intervals and origin along X and Y */

   xh = xl + width;
   yh = yl + height;
   gdx = width/(float)lngx;
   gdy = height/(float)lngy;
   rmx = min(gdx,gdy);
   xlo = xl + 0.5*gdx;
   yhi = yh - 0.5*gdy;

/* Start graph, draw borders.  N.B.  Could take lname
*  from ib->gsrcnm, but may not be there forever.  */

   ngrc = d3ngph(width+1.0, height+1.0, 0, 0,
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

/* Prepare scale to convert boxsum (S20) to circle radius.
*  Max term in a boxum is (usually) S27 and there are lnel terms.
*/

   lnel = (si32)ib->pisrc->nel;
   mxsum  = jmsw(S27, lnel);
   scale = 0.5*RP0->GCtD.cpr*rmx/swflt(mxsum);

/* Switch to drawing color unless it is a sequence.  If using a color
*  sequence, compute the scale factor and divisor needed to compute
*  color number from sum.  Because division by a 64-bit csdiv would
*  be slow, numerator and csdiv are scaled just enough to fit csdiv
*  in 31 bits.
*  Rev, 08/25/13, GNR - It is just barely possible that a boxsum
*     term could exceed mxsum--this is taken care of by the test
*     for ic >= SEQNUM, so the adjustment to mxsum was removed.
*/
   doclsq = !strcmp(RP->colors[CI_BUB], "SEQUENCE");
   if (doclsq) {
      si64 tdiv;
      si32 tcpt = (si32)RP0->GCtD.cpt << (FBwk-FBsi);
      thresh = jmsw(tcpt, lnel);
      tdiv = jrsw(mxsum, thresh);
      if (qsw(tdiv) <= 0) tdiv = jesl(1);    /* Avoid divide by zero */
      idsf = bitszsw(tdiv) - (BITSPERUI32-1);
      if (idsf < 0) idsf = 0;
      csdiv = swlo(jsrsw(tdiv, idsf));
      idsf = SEQN_EXP - idsf;
      jc = -1;                   /* Force color change on first box */
      }
   else pencol(RP->colors[CI_BUB]);


/* Loop over boxsums in vertical, then horizontal direction.
*  Draw circles for boxsums over cpt threshold.
*  When doing a color sequence, use jc to remember previous color.  */

   bptr = RP->boxsum;
   for (yg=yhi,igy=1; igy<=lngy; igy++,yg-=gdy) {
      for (xg=xlo,igx=1; igx<=lngx; igx++,xg+=gdx) {
         si64 sum = jabs(*bptr);       /* Positive box sum */
         si64 tds = jrsw(sum, thresh);
         float r;                      /* Circle radius */
         if (qsw(tds) >= 0) {
            r = scale * swflt(sum);    /* Set circle radius */
            if (r < RP->mnbpr) r = RP->mnbpr;
            else if (r > rmx) r = rmx;
            if (doclsq) {              /* Set color if sequence */
               tds = jssw(tds, idsf);  /* Times SEQNUM/csdiv scale */
               ic = jdswq(tds, csdiv);
               if (ic > (SEQNUM-1)) ic = (SEQNUM-1);
               ic += CI_SEQ;
               if (ic != jc) pencol(RP->colors[jc=ic]);
               }
            circle(xg,yg,r,NORETRACE); /* Draw circle */
            }
         bptr++;
         } /* End x loop */
      } /* End y loop */

   return finplt();
   } /* End d3bxvp() */
#endif /* !PARn */
