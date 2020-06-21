/* (c) Copyright 1991-2017, The Rockefeller University *21115* */
/* $Id: tvplot.c 78 2018-08-02 18:36:58Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                               tvplot                                 *
*                                                                      *
*  As of 07/26/17, image plot requests are interpreted as follows:     *
*  If there are images being used directly (inputs to network), the    *
*  possibly cropped tvx*tvy image is plotted.  Accesses images via     *
*  offset otvi in pbcst block on all nodes.                            *
*  If images are used only indirectly (inputs to preprocessors or      *
*  Gray-from-color), then the intermediate image of size tvix*tviy     *
*  located at cam->pmyim is plotted.                                   *
*  If there is a need to do both kinds of plots on the same mfdraw     *
*  frame, CAMERA input card would have to be modified to input two     *
*  different plot locations and sizes.  Need is not obvious now.       *
*  Parallel plotting of strips was eliminated--probably not efficient  *
*  with large numbers of nodes.                                        *
*                                                                      *
*  N.B.  Per documentation, if tvwd,tvht are not specified (i.e. 0),   *
*  an unscaled image is drawn (may be faster).                         *
************************************************************************
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
*  R67, 09/15/16, GNR - Correct to access npi[xy] etc. in PREPROC      *
*  R74, 06/19/17, GNR - utv.tvkocol codes are now BM_xxxx codes        *
*  Rev, 07/28/17, GNR - Remove parallel plotting from comp nodes       *
*  R78, 04/09/18, GNR - Remove TV_ONDIR test--always plot pmyim image  *
***********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "sysdef.h"
#include "d3global.h"
#include "tvdef.h"
#include "celldata.h"
#include "rocks.h"
#include "rkxtra.h"
#include "plots.h"

#ifdef PARn
#error tvplot should not be compiled or called on PARn nodes
#else

extern struct CELLDATA CDAT;


/*=====================================================================*
*                               tvpimg                                 *
*                                                                      *
*  Call bitmap() or bitmaps() according to whether width is specified  *
*=====================================================================*/

static void tvpimg(byte *pimg, int rowlen, int colht,
      float xc, float yc, float bwd, float bht, int xoff,
      int yoff, int iwd, int iht, int type, int mode) {

   if (bwd <= 0 || bht <= 0)
      /* Do the supposedly faster, non-scaled version */
      bitmap(pimg, rowlen, colht, xc, yc, xoff, yoff,
         iwd, iht, type, mode);
   else
      bitmaps(pimg, rowlen, colht, xc, yc, bwd, bht,
         xoff, yoff, iwd, iht, type, mode);
   return;
   }


/*=====================================================================*
*                               tvplot                                 *
*=====================================================================*/

void tvplot (void) {

   struct TVDEF   *cam;       /* Ptr to current camera */
   struct PREPROC *pip;       /* Ptr to current preprocessor */
   float lettht = RP->stdlht;
   char  ltxt[20];            /* Plot label text */

   stattmr(OP_PUSH_TMR, TVPL_TMR);

   /* Set retrace switch and label color */
   retrace(RP->kplot & PLRTR ? RETRACE : NORETRACE);
   pencol(RP->colors[CI_BOX]);

/*---------------------------------------------------------------------*
*                            Plot cameras                              *
*---------------------------------------------------------------------*/

/* Loop over all defined cameras */

   for (cam=RP->pftv; cam; cam=cam->pnxtv) {
      if (cam->utv.tvsflgs & TV_ON && cam->utv.tvuflgs & TV_PLOT) {
         float ylabel = cam->tvply - lettht;
         /* R78, 04/17/18, GNR - Now always plot image at pmyim, from
         *  which all used variants (TV_Filt, TV_IFit, TV_GfmC) are
         *  derived.  */
         cam->tvht = cam->tvwd *
            (float)cam->utv.tviy/(float)cam->utv.tvix;
         tvpimg(cam->pmyim, (int)cam->utv.tvix, (int)cam->utv.tviy,
            cam->tvplx, cam->tvply, cam->tvwd, cam->tvht,
            0, 0, (int)cam->utv.tvix, (int)cam->utv.tviy,
            (int)(cam->utv.tvkocol & BM_MASK), GM_SET);
         /* Plot camera name unless coded to omit */
         if (!(cam->utv.tvuflgs & TV_OTITL)) {
            ssprintf(ltxt, "TV %16s", getrktxt(cam->hcamnm));
            symbol(cam->tvplx, ylabel, 0.7*lettht,
               ltxt, 0.0, strlen(ltxt));
            ylabel -= 1.2*lettht;
            } /* End if not omit title */
         if (cam->utv.tvuflgs & TV_PISG) {
            sconvrt(ltxt, "(4HS/G ,2*J1UH6)",
               &cam->utv.isn, &cam->utv.ign, NULL);
            symbol(cam->tvplx, ylabel, lettht,
               ltxt, 0.0, strlen(ltxt));
            } /* End plotting isn,ign */
         } /* End if (plotting this camera) */
      } /* End loop over cameras */

/*---------------------------------------------------------------------*
*                         Plot preprocessors                           *
*---------------------------------------------------------------------*/

/* Plot all active preprocessors that have requested plotting.
*  Note:  d3nset assures CDAT.psws has the space needed here.
*  R74, 06/30/17, GNR - Revised to plot preprocessor outputs.  */

   for (pip=RP->pfip; pip; pip=pip->pnip) {
      if (pip->ipsflgs & PP_ON && pip->upr.ipuflgs & PP_PLOT) {
         struct UPREPR *pup = &pip->upr;
         byte *pimg = RP->pbcst + pip->oppoi;
         int  iwd = (int)pup->nppx;
         int  iht = (int)pup->nppy;
         int  ncols = (int)pip->dcols;
         int  nrows = (int)pip->kpc;
         int  icrsep = (int)pip->dcoff;
         int  iwdx = iwd + icrsep;
         int  ihtx = iht + icrsep;
         int  rowlen =  ncols*iwdx - icrsep;
         int  colht = nrows*ihtx - icrsep;
         int  type = (int)(pup->ipkcol & BM_MASK | BM_NSME);
         int  ik,ikrem;
         /* This calculation is harmless if ipwd == 0 */
         pip->ipht = pip->ipwd * (float)pip->kpc *
            (float)pup->nppy / (float)pup->nppx;

/* Two levels of indenting suppressed to end of prescale plotting */

   /* I.  Plot the output images for each kernel */
   for (ik=0; ik<pup->nker; ++ik) {
      div_t cr = div(ik, nrows);
      int xoff = cr.quot*iwdx;
      int yoff = colht - cr.rem*ihtx - iht;
      /* Handle grayscale with negatives specially -- make
      *  a colored copy of the image and plot positives in
      *  green and negatives in red.  */
      if (qGray(pup->ipkcol) && pip->ipsflgs & (PP_UPGM|PP_NEGS)) {
         ui16 *pcim = (ui16 *)CDAT.psws;
         si16 *pgim = (si16 *)pimg;
         si16 *pgime = pgim + pup->lppi3;
         si32 bigpix = 0;
         memset(CDAT.psws, 0, NColorDims*HSIZE*pup->lppi3);
         for ( ; pgim<pgime; ++pgim) {
            if ((si32)(*pgim) > bigpix) bigpix = (si32)(*pgim); }
         if (bigpix > 0) bigpix = UI16_MAX/bigpix;
         for (pgim=(si16 *)pimg; pgim<pgime; pcim+=NColorDims,++pgim) {
            if (*pgim > 0) pcim[1] = bigpix*(*pgim);
            if (*pgim < 0) pcim[0] = -bigpix*(*pgim);
            }
         tvpimg(CDAT.psws, rowlen, colht, pip->ipplx, pip->ipply,
            pip->ipwd, pip->ipht, xoff, yoff, iwd, iht,
            BM_C48|BM_NSME, GM_SET);
         }
      else {
         tvpimg(pimg, rowlen, colht, pip->ipplx, pip->ipply,
            pip->ipwd, pip->ipht, xoff, yoff, iwd, iht, type, GM_SET);
         }
      pimg += HSIZE*pip->upr.lppi3;
      } /* End loop over kernels */

   /* II.  Fill in any empty kernel slots with black */
   if ((ikrem = pup->nker % nrows) > 0) {
      int xoff = (ncols-1)*iwdx, yoff;
      int yoffe = ikrem*ihtx;
      memset(CDAT.psws, 0, HSIZE*pup->lppi3);
      for (yoff=0; yoff<yoffe; yoff+=ihtx)
         tvpimg(CDAT.psws, rowlen, colht, pip->ipplx, pip->ipply,
            pip->ipwd, pip->ipht, xoff, yoff, iwd, iht, type, GM_SET);
      }

   /* III.  Draw gutters */

   {  int bsz = HSIZE*icrsep*max(rowlen,colht);
      int gtype = type;
      if (qColored(pup->ipkcol) || pip->ipsflgs & (PP_UPGM|PP_NEGS)) {
         /* If colored, make gutters yellow */
         ui16 *pcim = (ui16 *)CDAT.psws;
         ui16 *pcime = pcim + NColorDims*bsz;
         for ( ; pcim<pcime; pcim+=NColorDims)
            pcim[0] = pcim[1] = 32767, pcim[2] = 0;
         gtype = BM_C48|BM_NSME;
         }
      else {
         /* If gray scale, make gutters black */
         memset(CDAT.psws, 0, bsz);
         }
      /* Column gutters */
      for (ik=1; ik<ncols; ++ik) {
         tvpimg(CDAT.psws, rowlen, colht, pip->ipplx, pip->ipply,
            pip->ipwd, pip->ipht, ik*iwd+(ik-1)*icrsep, 0,
            icrsep, colht, gtype, GM_SET);
         }
      /* Row gutters */
      for (ik=1; ik<nrows; ++ik) {
         tvpimg(CDAT.psws, rowlen, colht, pip->ipplx, pip->ipply,
            pip->ipwd, pip->ipht, 0, ik*iht+(ik-1)*icrsep,
            rowlen, icrsep, gtype, GM_SET);
         }
      }  /* End bsz local scope for gutters */

   /* Plot preprocessor title unless coded to omit */
   if (!(pip->upr.ipuflgs & PP_OTITL)) {
      ssprintf(ltxt, "PP %16s", getrktxt(pip->hipnm));
      symbol(pip->ipplx, pip->ipply-lettht, 0.7*lettht,
         ltxt, 0.0, strlen(ltxt));
      } /* End plotting title */
   } /* End if (plotting this preprocessor) */

/* End two levels of indenting suppressed */

      } /* End loop over preprocessors */

   stattmr(OP_POP_TMR, TVPL_TMR);
#endif

   } /* End tvplot() */
