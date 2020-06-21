/* (c) Copyright 1989-2018, The Rockefeller University *21116* */
/* $Id: d3rplt.c 77 2018-03-15 21:08:14Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                              d3rplt.c                                *
*                 Region/repertoire plotting routine                   *
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
************************************************************************
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
*  R65, 01/02/16, GNR - n[gq][xyg] to ui16, ngrp to ui32, ncpg to ubig *
*  R77, 03/12/18, GNR - Add kctp=H, hold plot                          *
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
   float fnxgd,fnygd;            /* Grid intervals */
   float xg,yg;                  /* x,y coords of grid group lines */

   int interleaved;              /* Interleaved plot switch */
   int kgrid;                    /* Grid plot switch */
   int klayr;                    /* Layer divisions plot  */
   int krt;                      /* Retrace switch */
   int lngx,lngy;                /* Size of repertoire */
   int lnqy;                     /* Rows per group */
   ui16 lkrp;                    /* Local copy of plot options */

/* Activate timing statistics */

   stattmr(OP_PUSH_TMR, GRPH_TMR);

/* Pick up local copies of repertoire variables */

   yh = yl + height;             /* y coord of top edge of plot */
   xh = xl + width;              /* x coord of right edge of plot */
   lkrp = ir->Rp.krp;
   lngx = ir->ngx, lngy = ir->ngy, lnqy = ir->nqy;
   fnxgd = (float)ir->Rp.ngridx, fnygd = (float)ir->Rp.ngridy;

/* Set option switches */

   krt = ((lkrp & KRPPR) || (RP->ksv & PLRTR)) ? RETRACE : NORETRACE;
   interleaved = ((lkrp & KRPIL) != 0);
   kgrid  = (lkrp&KRPPG) != 0 && (ir->Rp.ngridx|ir->Rp.ngridy) != 0;
   klayr  = ir->nlyr > 1;

/* Calculate group and cell plotting increments.  In STRATIFIED
*  plots, gdy is different for each cell type, therefore it is
*  not calculated until cell type loops, and user has no way to
*  disable layer lines.  Note:  gdx,gdy in il->Lp take gaps into
*  account, so we are recomputing them here without gaps.  */

   gdx = width/lngx;                   /* Group delta x */
   cdy = -height/(float)(lngy*lnqy);   /* Cell delta y  */
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
         wbcdwt(&icyc, cyclab, RK_IORF|RK_LFTJ|RK_NINT|3);
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
      float gri,grie,ygl,ygn;    /* Grid intervals and top values */

      /* Draw x (vertical) grid lines */
      if ((gri = gdx*fnxgd)) {         /* Assignment intended */
         grie = xh - 0.9*gri;
         for (xg=xl+gri; xg<grie; xg+=gri)
            line(xg, yl, xg, yh);
         }

      /* Draw y (horizontal) grid lines (gdy is neg!) */
      if (interleaved) {         /* INTERLEAVED */
         if ((gri = gdy*fnygd)) {      /* Assignment intended */
            grie = yl - 0.9*gri;
            for (yg=yh+gri; yg>grie; yg+=gri)
               line(xl, yg, xh, yg);
            }
         } /* End interleaved */
      else {                     /* STRATIFIED */
         for (ygl=yh,il=ir->play1; il; ygl=ygn,il=il->play) {
            if (il->Ct.kctp & CTPHP) continue;
            gdy = cdy*(float)il->lnqy;
            ygn = ygl + gdy*(float)lngy;  /* Next ygl */
            if ((gri = gdy*fnygd)) {  /* Assignment intended */
               grie = ygn - 0.9*gri;
               for (yg=ygl+gri; yg>grie; yg+=gri)
                  line(xl, yg, xh, yg);
               }
            } /* End layer loop */
         } /* End stratified */
      } /* End grid lines */

/* Draw layer division lines if requested (and nlayr > 1).
*  (These delineate layers of cells vertically and are plotted
*  as dashed lines to distinguish them from group grid lines.
*  The dashes delineate also the horizontal groups.  The lines
*  are omitted at bottom of a group if a grid line is already
*  there.  */

   if (klayr) {
      float xge = xh - 0.1*gdx;
      float dxl = 0.3*gdx, dxh = 0.7*gdx;
      if (interleaved) {         /* INTERLEAVED */
         float yge = yl - cdy;
         for (yg=yh; yg>yge; ) {
            int igy = 0;
            for (il=ir->play1; il; ) {
               if (il->Ct.kctp & CTPHP) continue;
               yg += cdy*(float)il->lnqy;
               /* If ngridy == 0, always draws layer lines */
               if (!(il=il->play) && ++igy == ir->Rp.ngridy) break;
               for (xg=xl; xg<xge; xg+=gdx)
                  line(xg+dxl, yg, xg+dxh, yg);
               } /* End layer loop */
            } /* End group loop */
         } /* End interleaved */
      else {                     /* STRATIFIED */
         for (yg=yh,il=ir->play1; il && il->play; il=il->play) {
            if (il->Ct.kctp & CTPHP) continue;
            yg += cdy*(float)(lngy*il->lnqy);
            for (xg=xl; xg<xge; xg+=gdx)
               line(xg+dxl, yg, xg+dxh, yg);
            } /* End layer loop */
         } /* End stratified */
      } /* End if klayr */

/* Discontinue timing */

   stattmr(OP_POP_TMR, GRPH_TMR);

#endif
   } /* End d3rplt() */
