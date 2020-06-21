/* (c) Copyright 1992-1997 Neurosciences Research Foundation, Inc. */
/* $Id: d3rplt.c 7 2008-05-02 22:16:44Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                              d3rplt.c                                *
*                                                                      *
*                     Repertoire plotting routine                      *
*                                                                      *
*          ---This function should be called on all nodes---           *
*            (It may or may not do anything on comp nodes)             *
*                                                                      *
*  void d3rplt(struct REPBLOCK *ir, int icyc, float xl, float yl,      *
*     float width, float height)                                       *
*                                                                      *
*  ir:     Ptr to repertoire to be plotted                             *
*  icyc:   Cycle number.  0 to plot rep name, -1 to omit altogether.   *
*  xl,yl:  X,Y coords of lower left corner                             *
*  width:  Plot width                                                  *
*  height: Plot height                                                 *
*                                                                      *
*  N.B.  Following the restructuring of V6D, this routine plots only   *
*  the outline box, grids, and titles that are proper to the entire    *
*  repertoire.  Routine d3lplt plots the cells and anatomy.  This      *
*  division allows each layer to be plotted individually, at a time    *
*  when the actual s(j) inputs are still available, unswapped.  This   *
*  new division of effort should also give greater speed, because the  *
*  graphics pipeline to the host will be filled more uniformly.        *
*                                                                      *
*  V4A, 03/20/89, JWT - Translated into C                              *
*  V4B, 02/01/90, JWT - Parallelized                                   *
*  Rev, 10/28/91, GNR - Add cpdia parameter for fixed radius           *
*  Rev, 05/09/92, GNR - Always use stdlht                              *
*  Rev, 05/20/92, ABP - Add stattmr() calls                            *
*  Rev, 08/25/92, GNR - Use individual cpr,cpt from CELLTYPE           *
*  V5E, 08/29/92, GNR - Major rewrite: plot cells in natural order,    *
*                       merge two modes, separate x,y grid intervals,  *
*                       remove layr line UPWITH test, add KRPPJ, CTPSJ *
*  Rev, 02/27/93, GNR - Remove retrace, color calls if nothing drawn   *
*  V6A, 07/24/93, GNR - Handle neuroanatomy plots from platform senses *
*  V6D, 02/03/94, GNR - Restructured to form d3rplt and d3lplt         *
*  V8A, 12/11/97, GNR - Stratified: vertical spacing proportional to   *
*                       number of cells.  Interleaved: fix offset bug  *
*  ==>, 12/28/07, GNR - Last mod before committing to svn repository   *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "sysdef.h"
#include "d3global.h"
#ifndef PARn
#include "rocks.h"
#endif
#include "rkxtra.h"
#include "plots.h"

void d3rplt(struct REPBLOCK *ir, int icyc, float xl, float yl,
   float width, float height) {

#ifndef PARn
   struct CELLTYPE *il;          /* Ptr to current cell block */

   float xh;                     /* x coord of right edge of plot */
   float yh;                     /* y coord of top edge of plot */
   float cdy;                    /* Cell delta y  */
   float gdx,gdy;                /* Group delta x,y */
   float xg,yg,ygl;              /* x,y coords of a grid,layer lines */

   long lkrp;                    /* Local copy of plot options */
   long lngx,lngy;               /* Size of repertoire */
   long lnqg;                    /* Rows per group */
   long igx,igy;                 /* Group counters along x,y */

   int interleaved;              /* Interleaved plot switch */
   int kgrid;                    /* Grid plot switch */
   int klayr;                    /* Layer divisions plot  */
   int krt;                      /* Retrace switch */
   int lnxgd,lnygd;              /* Grid intervals */

/* Activate timing statistics */

   stattmr(OP_PUSH_TMR, GRPH_TMR);

/* Pick up local copies of repertoire variables */

   yh = yl + height;             /* y coord of top edge of plot */
   xh = xl + width;              /* x coord of right edge of plot */
   lkrp = ir->Rp.krp;          
   lngx = ir->ngx, lngy = ir->ngy;
   lnxgd = ir->Rp.ngridx, lnygd = ir->Rp.ngridy;
   /* In repertoire plots, group boxes are not allowed to coalesce
   *  as they do in the printed output--if only one cell per group,
   *  set lnqg accordingly.  */
   lnqg = (ir->ncpg == 1) ? 1 : ir->nqg;

/* Set option switches */

   krt = ((lkrp & KRPPR) || (RP->ksv & PLRTR)) ? RETRACE : NORETRACE;
   interleaved = ((lkrp & KRPIL) != 0);
   kgrid  = ((lkrp&KRPPG) != 0) && ((lnxgd|lnygd) != 0);
   klayr  = ir->nlyr > 1;

/* Calculate group and cell plotting increments.  In STRATIFIED
*  plots, gdy is different for each cell type, therefore it is
*  not calculated until cell type loops, and user has no way to
*  disable layer lines.  */

   gdx = width/lngx;                   /* Group delta x */
   cdy = -height/(float)(lngy*lnqg);   /* Cell delta y  */
   if (interleaved) {                  /* INTERLEAVED   */
      gdy = -height/(float)lngy;       /* Group delta y */
      klayr = klayr && (lkrp&KRPDL);   /* Layer line switch */
      }

/* Set CI_BOX color for all box, grid, and label plotting */

   pencol(RP->colors[CI_BOX]);

/* Set retrace switch and draw title string.
*
*  If icyc < 0, we are doing a configuration plot, so omit title.
*  If not doing a superposition plot, also omit anatomy plot.
*  This code previously turned off kfill for speed--removed,
*  11/01/92, because modern display hardware generally implements
*  fill efficiently.  There is no need to check for retrace state
*  same as previous plot--ROCKS library does this internally.  */

   retrace(krt);
   if ((icyc >= 0) && !(lkrp & KRPOM)) {
      float lettht = RP->stdlht;
      if (!(RP->kplot & (PLSUP|PLVTS|PLDYN))) {
         /* Not a superposition plot:
         *  Plot full series/trial and repertoire titles.  */
         symbol(0.5, 11.0-lettht, 0.7*lettht, RP0->stitle+1,
            0.0, RP0->lst-1);
         symbol(xl, yl-lettht, 0.7*lettht, getrktxt(ir->hrname),
            0.0, ir->lrn);
         } /* End non-superposition (full) plot title */
      else if (icyc == 0) {
         /* Superposition plot main cycle:  Plot repertoire name.  */
         symbol(xl, yl-lettht, 0.7*lettht, getrktxt(ir->hrname),
            0.0, ir->lrn);
         } /* End superposition main cycle title */
      else {
         /* Superposition plot inner cycle:  Plot cycle number only.  */
         char cyclab[4];
         ibcdwt(RK_IORF+RK_LFTJ+3, cyclab, icyc);
         symbol(xl, yl-lettht, 0.7*lettht, cyclab, 0.0, RK.length+1);
         } /* End superposition inner cycle title */
      } /* End plotting titles */

/* Draw the border box */

   rect(xl, yl, width, height, THIN);

/* Draw grid lines if reqested.  Grid lines delineate borders
*  of groups.  Every ngridx'th line along x and every ngridy'th
*  line along y is drawn, as established by REPERTOIRE card.
*  Grid lines are drawn once only for interleaved plots, once
*  per cell type for stratified plots.  Grid lines are not
*  retraced.  (Note that gdy is negative, so add to move down.)  */

   if (krt) retrace(OFF);        /* For grids and/or layer lines */
   if (kgrid) {
      float gri = gdx*(float)lnxgd; /* Grid intervals */

      /* Draw x (vertical) grid lines */
      for (xg=xl+gri,igx=lnxgd; igx<lngx; xg+=gri,igx+=lnxgd)
         line(xg, yl, xg, yh);

      /* Draw y (horizontal) grid lines */
      if (interleaved) {         /* INTERLEAVED */   
         gri = gdy*(float)lnygd;
         for (yg=yh+gri,igy=lnygd; igy<lngy; yg+=gri,igy+=lnygd)
            line(xl, yg, xh, yg);
         } /* End interleaved */
      else {                     /* STRATIFIED */
         for (yg=yh,il=ir->play1; il; il=il->play) {
            gdy = cdy*(float)il->lnqy;
            gri = gdy*(float)lnygd;
            for (ygl=yg+gri,igy=lnygd; igy<lngy; ygl+=gri,igy+=lnygd)
               line(xl, ygl, xh, ygl);
            yg += gdy*(float)lngy;
            } /* End layer loop */
         } /* End stratified */
      } /* End grid lines */

/* Draw layer division lines if requested (and nlayr > 1).
*  (These delineate layers of cells vertically and are plotted
*  as dashed lines to distinguish them from group grid lines.
*  The dashes delineate also the horizontal groups.  The lines
*  are omitted at bottom of a group if a grid line is already
*  there.  Note that g2rep guarantees that ir->Rp.ngridy > 0.  */

   if (klayr) {
      if (interleaved) {         /* INTERLEAVED */   
         long jgy = lnygd;
         for (yg=yh,igy=1; igy<=lngy; igy++) {
            if (kgrid) jgy--;
            if (igy==lngy) jgy = 0;
            for (il=ir->play1; il; ) {
               yg += cdy*(float)il->lnqy;
               if (((il=il->play)==0) && !jgy) jgy = lnygd;
               else for (xg=xl,igx=1; igx<=lngx; xg+=gdx,igx++)
                  line(xg+0.3*gdx, yg, xg+0.7*gdx, yg);
               } /* End layer loop */
            } /* End group loop */
         } /* End interleaved */
      else {                     /* STRATIFIED */
         for (yg=yh,il=ir->play1; il; il=il->play) {
            yg += cdy*(float)(lngy*(long)il->lnqy);
            for (xg=xl,igx=1; igx<=lngx; xg+=gdx,igx++)
               line(xg+0.3*gdx, yg, xg+0.7*gdx, yg);
            } /* End layer loop */
         } /* End stratified */
      } /* End if klayr */

/* Discontinue timing */

   stattmr(OP_POP_TMR, GRPH_TMR);

#endif
   } /* End d3rplt() */
