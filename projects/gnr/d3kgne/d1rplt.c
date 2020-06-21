/* (c) Copyright 1991-2009, Neurosciences Research Foundation, Inc. */
/* $Id: d1rplt.c 42 2011-01-03 21:37:10Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                              d1rplt.c                                *
*                                                                      *
*                   D1 repertoire plotting routine                     *
*                                                                      *
*  Synopsis: void d1rplt(void)                                         *
*  Call in parallel on all nodes only if D1 exists and PLOT cndx fit.  *
*                                                                      *
*  V5C, 01/18/92, GNR - Initial version                                *
*  Rev, 02/26/92, GNR - Switch to normalized response scores, map      *
*                       color seq to range 0-1.0, not cpt-1.0.         *
*  Rev, 05/09/92, GNR - Use new ROCKS plot calls for atomization       *
*  Rev, 05/20/92, ABP - Add stattmr() calls                            *
*  Rev, 10/18/01, GNR - Separate kd1 into three components             *
*  V8C, 02/22/03, GNR - Cell responses in millivolts, add conductances *
*  ==>, 11/01/07, GNR - Last mod before committing to svn repository   *
*  Rev, 01/17/09, GNR - Reorganize colors into a single array          *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "d3global.h"
#include "d1def.h"
#include "rocks.h"
#include "rkxtra.h"
#include "plots.h"
#include "bapkg.h"

#define D1GL   6           /* Group number length */

void d1rplt(void) {

   struct D1BLK *pd1;      /* Ptr to current D1 block */
   float xl,yl,xh,yh;      /* Low and high box coordinates */
   float cdx,cdy;          /* Cell increments along x and y */
   float dxr;              /* Repertoire box spacing */
#ifndef PAR0
   d1s_type *psi,s;        /* Ptr to score, score */
   ldiv_t qrm;
   float size;             /* Size of largest circle or bar
                           *  (in 1/256ths of an inch) */
   float xc,yc;            /* Coordinates of current cell */
   float xr0,xr1;          /* Repertoire x coords, low and high */
   long icna;              /* Absolute cell index */
   long icr0,icr1;         /* First cell in this,next repertoire */
   long lnepr;             /* Local copy of nepr */
   long mylo,myhi;         /* Low and high cells on my node */
   int doclsq;             /* TRUE if doing color sequence */
   int ic,jc;              /* New, current color index */
   int ksqr;               /* TRUE if plotting squares */
   unsigned int lpst;      /* Local plot score threshold */
#endif
   unsigned short lkd1p;   /* Local copy of kd1p */
#ifndef PARn
   byte *pq;               /* Ptr to current input data */
   int jrep;               /* Repertoire index */
#endif
   int iblk;               /* Block index */
   int irep;               /* Repertoire index */
   int lnbpe;              /* Local copy of pd1->nbpe */

   stattmr(OP_PUSH_TMR, GRPH_TMR);

/* Set global parameters */

#ifndef PAR0
   /* Determine whether to draw squares or circles */
   ksqr = RP->kplot & PLSQR;
   /* Determine whether doing a color sequence */
   doclsq = ssmatch(RP->colors[CI_BAR],"SEQUENCE",3) != 0;
#endif

/* Loop over all D1 blocks--all nodes */

   for (pd1=RP->pd1b1,iblk=1; pd1; pd1=pd1->pnd1b,iblk++) {
      lkd1p = pd1->kd1p;   /* Make local copy of kd1 */
      if (!(lkd1p & (KD1_PLOT|KD1_BUBB)) ||
          !(lkd1p & KD1_ALLS || RP->CP.runflags & RP_FREEZE)) continue;

      retrace((lkd1p & KD1_RETR | RP->kplot & PLRTR) ?
         RETRACE : NORETRACE);
      lnbpe = pd1->nbpe;   /* Make local copy of nbpe */
      xh = (xl = pd1->d1x) + pd1->d1w; /* Get box coords */
      yh = (yl = pd1->d1y) + pd1->d1h;
      dxr = pd1->d1w + pd1->hspacing;

/*---------------------------------------------------------------------*
*            Node 0 code to draw boxes for all repertoires             *
*---------------------------------------------------------------------*/

#ifndef PARn

/* Locate the input data.
*  If in indexed mode, pick up repertoire number. */

      pq = RP->pbcst + pd1->odin;
      if (pd1->kd1o & KD1_INDX) {
         memcpy((char *)&irep,(char *)pq,
            sizeof(int)); /* Get repertoire index */
         pq += sizeof(int); }

/* Unless turned off, make a single label for all repertoires.
*  N.B.  Do before repertoire loop changes xl,xh coords.
*  (In anticipation of a new general policy, standard letter
*  height is used exclusively.)  */

      if (!(lkd1p & KD1_OMIT)) {
         char ltxt[10];    /* Space for generating label */
         float lettht = RP->stdlht;
         pencol(RP->colors[CI_BOX]);
         sconvrt(ltxt,"(O7HD1 Blk JI3)",&iblk,NULL);
         symbol(xl,yl-lettht,0.7*lettht,ltxt,0.0,sizeof(ltxt));
         } /* End label */

/* Now draw the boxes */

      for (jrep=1; jrep<=pd1->nd1r; jrep++) {

         /* Draw box of requested size */
         pencol(RP->colors[CI_BOX]);
         rect(xl,yl,pd1->d1w,pd1->d1h,THIN);

/* Draw the stimulus at the top.  If in indexed mode, there will
*  be a stimulus only for one repertoire, otherwise for all.
*  If KP=C specified and have V2 input, plot group number.  */

         if (!(pd1->kd1o & KD1_INDX) || jrep == irep) {
            float xsc;     /* Center of a stimulus bar */
            int ibit;      /* Bit counter */
            /* Draw input data in stimulus color */
            pencol(RP0->ecolstm);
            /* Calculate spacing parameters */
            cdx = pd1->d1w/(float)lnbpe;
            cdy = 0.7*pd1->vspacing;
            xsc = xl + 0.5*cdx;
            /* Plot group number if exists and is requested */
            if (!(pd1->kd1o & KD1_VER1)) {
               if (lkd1p & KD1_GRPS) {
                  char ltxt[D1GL];  /* ASCII group number  */
                  short id1gn;      /* Binary group number */
                  float lht = min(RP->stdlht,cdy);
                  memcpy((char *)&id1gn,(char *)pq,sizeof(short));
                  ibcdwt(RK_IORF+RK_LFTJ+D1GL-1,ltxt,(long)id1gn);
                  symbol(xh+0.5*lht,yh,lht,ltxt,0.0,D1GL);
                  } /* End plotting group number */
               pq += sizeof(short); /* Skip over grp to bits */
               } /* End handling group number */
            /* Loop over bits, plotting bars for 1's */
            for (ibit=1; ibit<=lnbpe; ibit++) {
               if (bittst(pq,ibit)) line(xsc,yh,xsc,yh+cdy);
               xsc += cdx;
               } /* End bit loop */
            pq += pd1->nbyt;
            } /* End if input to this rep */

         xl += dxr;
         xh += dxr;
         } /* End repertoire loop */

#endif   /* End host code */

/*---------------------------------------------------------------------*
*             Comp node code to draw repertoire elements               *
*---------------------------------------------------------------------*/

/* Implementation note:  Code used to calculate xr0, etc. parameters
*  at start of a new repertoire is inside the cell loop so only one
*  copy is needed.  This appears to replace an efficient setup outside
*  and add inside the loop with an inefficient multiply inside, but
*  this is not really so bad since typical node will only encompass
*  one repertoire anyway and thus only execute this code once.  */

#ifndef PAR0

      /* Locate match score data */
      psi = pd1->pd1s;
      /* Determine starting repertoire on this node */
      lnepr = pd1->nepr;   /* Make local copy of nepr */
      myhi = (mylo = pd1->lod1e) + pd1->mynd1;
      irep = mylo/lnepr;
      icr1 = irep*lnepr;   /* Trigger new-repertoire code below */
      /* Set for single color vs color sequence */
      lpst = (unsigned)pd1->cpt; /* Pick up plot score threshold */
      if (doclsq)          /* Using color sequence */
         jc = -1;          /* Initialize to unknown color */
      else                 /* Using single fixed color */
         pencol(RP->colors[CI_BAR]);

/* Draw "bubble" plot */

      if (lkd1p & KD1_BUBB) {
         float r;          /* Radius of current circle */

         /* Set cell increments along x and y.
         *  N.B. Sign convention for cdy is opposite to d3rplt */
         cdx = pd1->d1w/(float)pd1->ngxd1;
         cdy = pd1->d1h/(float)pd1->ngyd1;
         /* Calc radius of maximal bubble in 256ths of an inch */
         size = 1.953125e-3*pd1->cpr*
            (RP->cpdia ? RP->cpdia : min(cdx,cdy));
         size = min(0.001465,size);
         /* Set starting coordinates for x and y */
         xl += 0.5*cdx; yh -= 0.5*cdy;

         /* Loop over my cells and plot them */
         for (icna=mylo; icna<myhi; icna++) {
            /* Check for entering new repertoire */
            if (icna >= icr1) {
               xr0 = xl + (float)(irep++)*dxr;
               icr0 = icr1; icr1 += lnepr; }
            /* Check whether cell exceeds plot threshold */
            if ((s = psi[icna]) < lpst) continue;
            /* Calculate coordinates of cell */
            qrm = ldiv(icna - icr0, pd1->ngxd1);
            xc = xr0 + (float)qrm.rem*cdx;
            yc = yh  - (float)qrm.quot*cdy;
            /* Calculate radius of cell */
            r = size*(float)s; r = max(r,0.01);
            /* Set color */
            if (doclsq) {
               ic = s >> 5;   /* Select color indep. of cpt */
               if (ic != jc) pencol(RP->colors[CI_SEQ+(jc=ic)]);
               } /* End color sequence */
            if (ksqr) {
               float d = 2.0*r;
               rect(xc-r,yc-r,d,d,THIN);
               }
            else
               circle(xc,yc,r,THIN);
            } /* End cell loop */
         } /* End bubble plot */

/* Draw "bar-code" plot */

      else {
         byte *pr;         /* Ptr to repertoire data */
         int ibit;         /* Bit counter */

         /* Set bar code increments along x and y and height */
         cdx = pd1->d1w/(float)lnbpe;
         cdy = pd1->vspacing;
         size = 0.7*cdy;

         /* Loop over my cells and plot them.
         *  N.B.  Incrementing of icna is tricky. */
         for (icna=mylo; icna<myhi; ) {
            /* Check for entering new repertoire */
            if (icna >= icr1) {
               long ii,mgtt=0;   /* Threshold count */
               icr0 = icr1; icr1 += lnepr;   /* New rep limits */
               /* Count cells above threshold that are before
               *  the current cell in this repertoire. */
               for (ii=icr0; ii<icna; ii++)
                  if (psi[ii] >= lpst) mgtt++;
               yc = yh - cdy*((float)mgtt + 1.0);
               /* Could check yc < yl here, but mem is costly... */
               /* Set x boundaries for this repertoire */
               xr0 = xl + (float)(irep++)*dxr;
               xr1 = xr0 + pd1->d1w;
               } /* End new repertoire setup */
            /* If run off bottom of plot, go to next repertoire */
            if (yc < yl) { icna = icr1; continue; }
            /* Check whether cell exceeds plot threshold */
            if ((s = psi[icna]) >= lpst) {
               /* Draw horizontal line in box color */
               pencol(RP->colors[CI_BOX]);
               line(xr0,yc,xr1,yc);
               /* Put score or cell number on plot if requested */
               if (lkd1p & (KD1_SCPL | KD1_NUMB)) {
                  char ltxt[8];  /* Space for ASCII cell number */
                  float lht = min(RP->stdlht,size);
                  if (lkd1p & KD1_SCPL)
                     ibcdwt(8*RK_BSCL+3*RK_D+RK_GFP+RK_IORF+RK_LFTJ+7,
                        ltxt,(long)s);
                  else
                     ibcdwt(RK_IORF+RK_LFTJ+7,ltxt,icna-icr0);
                  symbol(xr1+0.5*lht,yc,lht,ltxt,0.0,8);
                  } /* End labelling bar */
               /* Set color */
               if (doclsq) {
                  ic = s >> 5;
                  pencol(RP->colors[CI_SEQ+ic]);
                  } /* End color sequence */
               else              /* Using single fixed color */
                  pencol(RP->colors[CI_BAR]);
               /* Locate data for current cell */
               pr = pd1->pd1r + (icna - mylo)*pd1->nbyt;
               xc = xr0 + 0.5*cdx;
               /* Loop over bits, plotting bars for 1's */
               for (ibit=1; ibit<=lnbpe; ibit++) {
                  if (bittst(pr,ibit)) line(xc,yc,xc,yc+size);
                  xc += cdx;
                  } /* End bit loop */
               /* Move to next line down */
               yc -= cdy;
               } /* End if above threshold */
            /* Move to next cell */
            icna++;
            } /* End cell loop */
         } /* End "bar-code" plot */

#endif   /* End comp node code */

      } /* End loop over D1 blocks */

   stattmr(OP_POP_TMR,0);
   } /* End d1rplt */

