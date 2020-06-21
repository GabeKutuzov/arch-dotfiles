/* (c) Copyright 1991-2010 Neurosciences Research Foundation, Inc. */
/* $Id: tvplot.c 44 2011-03-04 22:36:14Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                               tvplot                                 *
*                                                                      *
*  If there are TV direct or preprocessed inputs to display, display   *
*  them.  Assumes otvi in each TVDEF or PREPROC block gives pbcst      *
*  offset to a tvx*tvy image on all nodes.                             *
*                                                                      *
*  Image is divided into strips and plotted in parallel if PAR system. *
*                                                                      *
*  N.B.  Per documentation, if tvwd,tvht are not specified (i.e. 0),   *
*  an unscaled image is drawn (may be faster).                         *
*                                                                      *
*  Written by Michael Cook, undated                                    *
*  V5C, 11/22/91, GNR - Incorporate tvlab, correct myx, restructure    *
*  Rev, 05/09/92, GNR - Use new ROCKS plot calls for atomization       *
*  Rev, 05/20/92, ABP - Add stattmr() calls                            *
*  Rev, 04/10/94, GNR - Implement host graphics via bitmap()           *
*  V8A, 11/28/96, GNR - Remove support for non-hybrid version          *
*  V8B, 12/27/00, GNR - Move to new memory management routines         *
*  V8D, 03/29/07, GNR - Add 16- and 24-bit color modes, scaling        *
*  ==>, 12/21/07, GNR - Last mod before committing to svn repository   *
*  V8F, 04/27/10, GNR - Major rewrite to accommodate preprocessors     *
*  V8G, 09/08/10, GNR - Add OPT=I to plot isn,ign below image (debug)  *
***********************************************************************/

#define TVTYPE  struct TVDEF
#define PPTYPE  struct PREPROC

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "sysdef.h"
#include "d3global.h"
#include "tvdef.h"
#ifndef PARn
#include "rocks.h"
#endif
#include "plots.h"

#ifdef PARn
static int tvpnused;          /* Total nodes used */
static int tvpnode;           /* Number of current node */
#endif

/*=====================================================================*
*                              tvpimg()                                *
*  Graph the image in vertical stripes if parallel, all at once if     *
*     serial or not used directly.                                     *
*  Display the stripe with starting coordinates:                       *
*    xstart = (iwd/nused)*node (corrected for remainder)               *
*    ystart = 0                                                        *
*    xwidth = iwd/nused                                                *
*    ywidth = iht                                                      *
*=====================================================================*/

static void tvpimg(byte *pimg, int iwd, int iht, int kcol,
      float plx, float ply, float plw, float plh) {

   /* Array that maps BBD color modes to plot library color modes */
   static const int bbdcmap[4] = { BM_GS, BM_C8, BM_C16, BM_C24 };

/* If number of stripes is not a multiple of the number of nodes,
*  distribute the remaining lines to the first bunch of nodes.
*  If there are fewer stripes than nodes, distribute one stripe
*  per node while they last.  */

   int myx, mywd;             /* startx and width of stripe */
#ifdef PARn
   div_t qrm = div(iwd, tvpnused);
   mywd = qrm.quot;
   if (tvpnode < qrm.rem)
      myx = tvpnode*(++mywd);
   else
      myx = tvpnode*mywd + qrm.rem;
#else
   mywd = iwd;
   myx = 0;
#endif

   if (plw)
      bitmaps(pimg, iwd, iht, plx, ply, plw, plh,
         myx, 0, mywd, iht, bbdcmap[kcol], GM_SET);
   else
      bitmap(pimg, iwd, iht, plx, ply,
         myx, 0, mywd, iht, bbdcmap[kcol], GM_SET);

   } /* End tvpimg() */


/*=====================================================================*
*                              tvplot()                                *
*  (This routine now loops separately over raw cameras and image       *
*   preprocessors, extracting arguments needed to plot images and      *
*   titles, and calling tvpimg() and tvptt() to do so.                 *
*=====================================================================*/

void tvplot (void) {

   struct TVDEF   *cam;       /* Ptr to current camera */
   struct PREPROC *pip;       /* Ptr to current preprocessor */

#ifdef PARn
   tvpnused = NC.total - NC.headnode;
   tvpnode =  NC.node - NC.headnode;
#else
   float lettht = RP->stdlht;
#endif

   stattmr(OP_PUSH_TMR, TVPL_TMR);

/* Loop over all defined cameras.  If camera is used directly, then
*  the image is on all nodes and the plot can be executed in stripes.
*  If the camera is used only as an input to preprocessors, or this
*  is a serial computer, then the image is stored only on node 0 and
*  the plotting must be done there.  */

   for (cam=RP->pftv; cam; cam=cam->pnxtv) {
      if (cam->utv.tvsflgs & TV_ON && cam->utv.tvuflgs & TV_PLOT) {
#ifdef PARn
         /* On PARn nodes, cam->pmyin is not a valid pointer */
         if (cam->utv.tvsflgs & TV_ONDIR)
            tvpimg(RP->pbcst+cam->otvi, (int)cam->utv.tvx,
               (int)cam->utv.tvy, (int)cam->utv.tvkcol,
               cam->tvplx, cam->tvply, cam->tvwd, cam->tvht);
#else
         /* But on serial or PAR0 nodes, use cam->pmyin, because
         *  image may be in pbcst or pipimin areas.  */
#ifdef PAR0
         if (!(cam->utv.tvsflgs & TV_ONDIR)
#endif
            tvpimg(cam->pmyin, (int)cam->utv.tvx,
               (int)cam->utv.tvy, (int)cam->utv.tvkcol,
               cam->tvplx, cam->tvply, cam->tvwd, cam->tvht);
#endif
         } /* End if (plotting this camera) */
      } /* End loop over cameras */

/* Plot all active preprocessors that have requested plotting */

#ifndef PAR0
   for (pip=RP->pfip; pip; pip=pip->pnip) {
      if (pip->ipsflgs & PP_ON && pip->upr.ipuflgs & PP_PLOT) {
         byte *pimg = RP->pbcst + pip->otvi;
         float dcplx = pip->ipplx;
         long dotvi;
         ui32 crows,trows;             /* Column, total rows */
         int  ncol = (int)pip->dcols;  /* Number of columns  */
         if (ncol <= 1) ncol = 1;
         dotvi = (long)pip->kpc*(long)pip->upr.nipx3*
            (long)pip->upr.nipy;
         crows = (ui32)pip->kpc*(ui32)pip->upr.nipy;
         trows = (ui32)pip->nytot;
         while (ncol-- > 0) {
            if (pip->ipwd > 0.0) pip->ipht = pip->ipwd *
               (float)crows / (float)pip->upr.nipx;
            tvpimg(pimg, (int)pip->upr.nipx, crows,
               (int)pip->upr.ipkcol,
               dcplx, pip->ipply, pip->ipwd, pip->ipht);
            pimg += dotvi;
            dcplx += pip->dcoff;
            trows -= crows;
            if (trows < crows) crows = trows;
            }
         } /* End if (plotting this preprocessor) */
      } /* End loop over preprocessors */
#endif

#ifndef PARn
/*---------------------------------------------------------------------*
*                          Plot image labels                           *
*---------------------------------------------------------------------*/

   /* Set retrace switch */
   retrace(RP->kplot & PLRTR ? RETRACE : NORETRACE);

   for (cam=RP->pftv; cam; cam=cam->pnxtv)
         if (cam->utv.tvsflgs & TV_ON && cam->utv.tvuflgs & TV_PLOT) {
      float ylabel = cam->tvply - lettht;
      char ltxt[20];             /* Plot label text */
      pencol(RP->colors[CI_BOX]);
      if (!(cam->utv.tvuflgs & TV_OTITL)) {
         sconvrt(ltxt, "(3HTV ,J0A16)", getrktxt(cam->hcamnm), NULL);
         symbol(cam->tvplx, ylabel, 0.7*lettht,
            ltxt, 0.0, strlen(ltxt));
         ylabel -= 1.2*lettht;
         } /* End if not omit title */
      if (cam->utv.tvuflgs & TV_PISG) {
         sconvrt(ltxt, "(4HS/G ,2*J1UH6)",
            &cam->utv.isn, &cam->utv.ign, NULL);
         symbol(cam->tvplx, ylabel, lettht, ltxt, 0.0, strlen(ltxt));
         } /* End plotting isn,ign */
      } /* End loop over cameras */

   for (pip=RP->pfip; pip; pip=pip->pnip)
         if (pip->ipsflgs & PP_ON &&
            ((pip->upr.ipuflgs & (PP_PLOT|PP_OTITL)) == PP_PLOT)) {
      char ltxt[20];          /* Plot label */
      sconvrt(ltxt, "(3HPP ,J0A16)", getrktxt(pip->hipnm), NULL);
      pencol(RP->colors[CI_BOX]);
      symbol(pip->ipplx, pip->ipply-lettht, 0.7*lettht,
         ltxt, 0.0, strlen(ltxt));
      } /* End loop over preprocessors */
#endif

   stattmr(OP_POP_TMR, TVPL_TMR);

   } /* End tvplot() */
