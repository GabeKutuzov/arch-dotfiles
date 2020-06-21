/* (c) Copyright 1989-2017, The Rockefeller University *21116* */
/* $Id: d3ijpl.c 74 2017-09-18 22:18:33Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                       d3ijpl - Serial version                        *
*              Plot Cij, Cij0, Mij, PPF, Rj, Sj, or ESj                *
*                                                                      *
************************************************************************
*  V4A, 06/05/89, JWT - Translated into C                              *
*  Rev, 03/28/90, JWT - Added standard plot letter height              *
*  V5C, 03/22/92, GNR - Break out d3ijpi, reconstruct serial version   *
*  Rev, 05/09/92, GNR - Use new ROCKS plot calls for atomization       *
*  Rev, 08/20/92, GNR - Fix bug accessing Cij with nbc > 8             *
*  V7A, 04/28/94, GNR - Fix bug not incrementing pcij1 in conn loop    *
*  V8A, 09/28/97, GNR - Independent dynamic allocations per conntype   *
*  V8B, 11/10/01, GNR - Cell lists, unaveraged plots, OPTZZ, labels    *
*  V8C, 07/06/03, GNR - Cell responses in millivolts, add conductances *
*  V8D, 07/13/06, GNR - Use new d3lij to fetch Lij even if not stored  *
*  Rev, 09/24/07, GNR - Add MABCIJ, etc. options, RJPLOT, [E]SJPLOT    *
*  ==>, 12/24/07, GNR - Last mod before committing to svn repository   *
*  Rev, 06/14/08, GNR - Add CSELPLOT (color selectivity)               *
*  Rev, 01/17/09, GNR - Reorganize colors into a single array          *
*  Rev, 06/28/10, GNR - Move xlo,yhi calc to d3ijpi, fix shrink bug    *
*  V8I, 03/16/13, GNR - Begin implementing d3xxj macros                *
*  R66, 02/17/16, GNR - Change long random seeds to typedef si32 orns  *
*  R67, 12/08/16, GNR - Improved variable scaling, smshrink->smscale   *
*  R72, 03/15/17, GNR - Use CPLOC cell locators, add CMBOX plots       *
*                       Bug fix: CMSMAP was using Lij, not jsyn to map *
*  R72, 03/26/17, GNR - Removed addn of ix->osrc to Lij for VS_SRC     *
*  R72, 04/01/17, GNR - Added needle plots and scaled ex/in colors     *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "sysdef.h"
#include "d3global.h"
#include "ijpldef.h"
#include "clblk.h"
#include "lijarg.h"
#include "celldata.h"
#include "rocks.h"
#include "rkxtra.h"
#include "plots.h"

extern struct CELLDATA CDAT;
extern struct LIJARG Lija;

/* Common block for passing constant parameters */

static struct {
   float *pfnsc;                 /* Ptr to trig tables for needles */
   struct CPLOC Vloc;            /* Source variable locator */
   float szsx,szsy;              /* Size of symbol along x,y */
   si32  lcmamn;                 /* Min abs value of plot variable */
   si32  lcmmin;                 /* Min value of plot variable */
   si32  vard;                   /* Variable difference/color */
   si32  v2cseq;                 /* Variable to color seq mult */
   int   ifstx,ifsty;            /* Lija.fst[xy] or 0 as needed */
   int   icolor,jcolor;          /* Previous color, current color */
   int   lsncx;                  /* Source cells along x */
   int   lsnel;                  /* Source cells/group */
   int   lsngx;                  /* Source groups along x */
   bgrv  exbgr;                  /* Excitatory high color components */
   bgrv  inbgr;                  /* Inhibitory high color components */
   ui16  doseq;                  /* Type of color selection */
   ui16  lopt;                   /* Local copy of plot options */
   byte  lcmsmap;                /* Local copy of kcmsmap */
   } CM;

/* Table of hex output characters */
static char hextab[16] = "0123456789ABCDEF";

/*---------------------------------------------------------------------*
*                              cmpltvar                                *
*                                                                      *
*  This routine plots a symbol for either averaged or unaveraged plots *
*---------------------------------------------------------------------*/

static void cmpltvar(si32 var) {

   float xc,yc;                  /* Location of plotted symbol */
   float xs,ys;                  /* X,Y size of symbol */
   si32  sivar;                  /* Variable scaled to S23 or S26 */
   int   ivar;                   /* Var reduced to positive range */
   int   icig;                   /* Index of cell in its group */

/* Confine variable to range or skip if out of range */

   if (abs32(var) < CM.lcmamn) return;
   if ((ivar = (int)(var - CM.lcmmin)) < 0) {
      if (CM.lopt & CMNOX) return;
      ivar = 0;
      }
   else if (ivar >= CM.vard) {
      if (CM.lopt & CMNOX && ivar > CM.vard) return;
      ivar = CM.vard-1;    /* So color stays in SEQNUM range */
      }

/* Calculate location of symbol.  It is presumed here that out-of-
*  bounds connections result in exit from the variable-gathering loops
*  and are not passed to us.  Note that CM.Vloc.xlo,yhi have already
*  been set to the location for plotting the current target cell--here
*  we are just offsetting according to the source location.  This code
*  now uses the lcmsmap code to determine which formula to use.  For
*  window scans, the items are now (R72) spread over a box the size of
*  the window and the d3woff window offset in ix->osrc is not used.
*  Do the 'if' in two stages to save a little time for KCM_CFJSYN.
*  Subtract 0 when KCM_FSTXYA is not needed, saving an 'if'.  */

   if (CM.lcmsmap & (KCM_GCFLIJ|KCM_COFLIJ|KCM_IAFLIJ)) {
      int Lij = (int)Lija.lijval;   /* Spec says lijval < 31 bits */
      if (CM.lcmsmap & KCM_GCFLIJ) {
         div_t gc  = div(Lij, CM.lsnel);
         div_t gxy = div(gc.quot, CM.lsngx);
         div_t cxy = div(gc.rem, CM.lsncx);
         icig = gc.rem;             /* In case needed for needles */
         gxy.rem -= CM.ifstx, gxy.quot -= CM.ifsty;
         xc = CM.Vloc.xlo + (float)gxy.rem*CM.Vloc.gdx +
                           (float)cxy.rem*CM.Vloc.cdx;
         yc = CM.Vloc.yhi + (float)gxy.quot*CM.Vloc.gdy +
                           (float)cxy.quot*CM.Vloc.cdy;
         }
      else if (CM.lcmsmap & KCM_COFLIJ) {
         div_t sxy = div(Lij, CM.lsncx);
         sxy.rem -= CM.ifstx, sxy.quot -= CM.ifsty;
         xc = CM.Vloc.xlo + (float)sxy.rem*CM.Vloc.cdx;
         yc = CM.Vloc.yhi + (float)sxy.quot*CM.Vloc.cdy;
         }
      else {      /* Must be KCM_IAFLIJ */
         int tx,ty;
         tx = Lij >> RP->ky1, ty = Lij & RP->ymask;
         tx -= CM.ifstx, ty -= CM.ifsty;
         xc = CM.Vloc.xlo + (float)tx*CM.Vloc.cdx;
         yc = CM.Vloc.yhi + (float)ty*CM.Vloc.cdy;
         }
      }
   else if (CM.lcmsmap & KCM_CFJSYN) {
      div_t sxy = div(Lija.jsyn, CM.lsncx);
      xc = CM.Vloc.xlo + (float)sxy.rem*CM.Vloc.cdx;
      yc = CM.Vloc.yhi + (float)sxy.quot*CM.Vloc.cdy;
      }

/* Choose the color of the symbol */

   /* (Note that PPF is (S12), others are (S7/14) or (S14).
   *  This is accounted for in setting up the v2seq constant.)
   *  With new color scale, we just let pencol() decide whether
   *  color has changed or not.  */

   sivar = ivar * CM.v2cseq;
   if (CM.doseq == DCS_CSEQ) {   /* Old color sequence */
      CM.jcolor = CI_SEQ + (sivar >> CM_V2CSeqS);
      pencol(RP->colors[CM.jcolor]);
      }
   else {                        /* Color scaled to ivar */
      bgrv *peics;               /* Ptr to excit or inhib scale */
      si32 bb,gg,rr;             /* Scaled colors */
      char cspec[COLMAXCH];      /* Color spec */
      if (var >= 0) {            /* Excitatory */
         if (CM.doseq & DCS_EFIX) {
            pencol(RP->colors[CI_SPEX]); goto PenColorDone; }
         peics = &CM.exbgr;
         }
      else {                     /* Inhibitory */
         if (CM.doseq & DCS_IFIX) {
            pencol(RP->colors[CI_SPIN]); goto PenColorDone; }
         peics = &CM.inbgr;
         }
      bb = (sivar * peics->bb) >> CM_V2MaxCM;
      gg = (sivar * peics->gg) >> CM_V2MaxCM;
      rr = (sivar * peics->rr) >> CM_V2MaxCM;
      /* Deal with old convention for black/white reversal */
      if ((bb|gg|rr) == 0)
         pencol("Z010101");
      else {
         cspec[0] = 'Z';
         cspec[1] = hextab[bb >> 4];
         cspec[2] = hextab[bb & 15];
         cspec[3] = hextab[gg >> 4];
         cspec[4] = hextab[gg & 15];
         cspec[5] = hextab[rr >> 4];
         cspec[6] = hextab[rr & 15];
         cspec[7] = '\0';
         pencol(cspec);
         }
      }
PenColorDone: ;

/* Now draw a symbol of the requested shape, size, and color */

   if (CM.lopt & CMONDL) {       /* Draw oriented needles */
      /* Choose the size of the symbol */
      xs = CM.pfnsc[icig], ys = CM.pfnsc[icig+CM.lsnel];
      if (CM.lopt & CMVARY) {
         float fvar = (float)ivar;
         xs *= fvar, ys *= fvar; }
      line(xc-xs, yc-ys, xc+xs, yc+ys);
      }

   else {                        /* Not needles */
      /* Choose the size of the symbol */
      xs = CM.szsx, ys = CM.szsy;
      if (CM.lopt & CMVARY) {
         float fvar = (float)ivar;
         xs *= fvar, ys *= fvar; }
      if (CM.lopt & CMSIGN) {    /* Draw plus or minus sign */
         /* Start with minus sign in either case */
         line(xc-xs, yc, xc+xs, yc);
         /* If nonnegative, add vertical bar to make plus sign */
         if (var >= 0) line(xc, yc-ys, xc, yc+ys);
         } /* End plus/minus */
      else if (CM.lopt & CMCOL)  /* Draw filled box */
         rect(xc-xs, yc-ys, 2.0*xs, 2.0*ys, FILLED);
      else                       /* Draw outline box */
         rect(xc-xs, yc-ys, 2.0*xs, 2.0*ys, THIN);
      } /* End not needles */

   } /* End cmpltvar() */


/*---------------------------------------------------------------------*
*                              cmpltcsel                               *
*                                                                      *
*  This routine plots color selectivity for either averaged or         *
*  unaveraged plots on every third call.                               *
*---------------------------------------------------------------------*/

static void cmpltcsel(si32 var) {

   float xc,yc;                  /* Location of plotted symbol */
   float xs,ys;                  /* X,Y size of symbol */
   si32  bb,gg,rr;               /* Colors modified for opponency */
   int   ic = Lija.jsyn % NColorDims;
   char  cspec[COLMAXCH];        /* Color spec */

   static si32 v[NColorDims];    /* Storage for 3 Cij per plot */

   /* Starting place in jrgb as fn of tvcsel for excit,inhib color */
   static byte jcpe[NColorSels] = { 0, 0, 2, 1, 1, 0, 2, 2, 1, 0 };
   static byte jcpi[NColorSels] = { 0, 0, 0, 0, 2, 1, 0, 1, 0, 2 };
   /* Permutations of colors for Cij selection below */
   static byte jrgb[2*NColorDims-1] = { 0, 2, 1, 0, 2 };

   v[ic] = var;

   if (ic < (NColorDims-1)) return;

/* Calculate location of symbol.  Real cells do not have color, so
*  there is no KCM_GCFLIJ case here.  Otherwise same as cmpltvar.  */

   if (CM.lcmsmap & (KCM_COFLIJ|KCM_IAFLIJ)) {
      int Lij = (int)Lija.lijval;   /* Spec says lijval < 31 bits */
      if (CM.lcmsmap & KCM_COFLIJ) {
         div_t sxy = div(Lij, CM.lsncx);
         sxy.rem -= CM.ifstx, sxy.quot -= CM.ifsty;
         xc = CM.Vloc.xlo + (float)sxy.rem*CM.Vloc.cdx;
         yc = CM.Vloc.yhi + (float)sxy.quot*CM.Vloc.cdy;
         }
      else {      /* Must be KCM_IAFLIJ */
         int tx,ty;
         tx = Lij >> RP->ky1, ty = Lij & RP->ymask;
         tx -= CM.ifstx, ty -= CM.ifsty;
         xc = CM.Vloc.xlo + (float)tx*CM.Vloc.cdx;
         yc = CM.Vloc.yhi + (float)ty*CM.Vloc.cdy;
         }
      }
   else {         /* Must be KCM_CFJSYN */
      div_t sxy = div(Lija.jsyn/NColorDims, CM.lsncx);
      xc = CM.Vloc.xlo + (float)sxy.rem*CM.Vloc.cdx;
      yc = CM.Vloc.yhi + (float)sxy.quot*CM.Vloc.cdy;
      }

/* Choose the color of the symbol.  For a CSELPLOT, there are no
*  limits tests and the Cij are passed to us on S14.  These are
*  scaled to S8 to generate a hex rgb color.  The hex conversions
*  are done by brute force to get the right combination of leading
*  zeros (missing with ssprintf) and no overflow indication
*  (problem with sconvrt or wbcdwt).  Saves a few nsec too.  */

   if (CM.doseq >= RmG) {
      int jce = jcpe[CM.doseq], jci = jcpi[CM.doseq];
      bb = v[jrgb[ jce ]] - v[jrgb[ jci ]];
      gg = v[jrgb[jce+1]] - v[jrgb[jci+1]];
      rr = v[jrgb[jce+2]] - v[jrgb[jci+2]];
      }
   else if (CM.doseq >= Red) {
      int jce = jcpe[CM.doseq];
      bb = v[jrgb[ jce ]];
      gg = v[jrgb[jce+1]];
      rr = v[jrgb[jce+2]];
      }
   else
      bb = gg = rr = S14/2;

   bb = (bb < 0) ? 0 : bb >> CM_Cij2rgb;
   gg = (gg < 0) ? 0 : gg >> CM_Cij2rgb;
   rr = (rr < 0) ? 0 : rr >> CM_Cij2rgb;

   CM.jcolor =
      (bb << BITSPERBYTE | gg) << BITSPERBYTE | rr;
   if (CM.jcolor == 0) return;
   if (CM.jcolor != CM.icolor) {
      cspec[0] = 'Z';
      cspec[1] = hextab[bb >> 4];
      cspec[2] = hextab[bb & 15];
      cspec[3] = hextab[gg >> 4];
      cspec[4] = hextab[gg & 15];
      cspec[5] = hextab[rr >> 4];
      cspec[6] = hextab[rr & 15];
      cspec[7] = '\0';
      pencol(cspec);
      CM.icolor = CM.jcolor;
      }

/* Choose the size of the symbol */

   if (CM.lopt & CMVARY) {
      si32 w2 = bb*bb + gg*gg + rr*rr;
      float wd = sqrtf((float)w2);
      xs = wd*CM.szsx;
      ys = wd*CM.szsy;
      }
   else {
      xs = CM.szsx;
      ys = CM.szsy;
      }

/* Now draw a symbol of the requested shape, size, and color */

   if (CM.lopt & CMCOL)       /* Draw filled box */
      rect(xc-xs, yc-ys, 2.0*xs, 2.0*ys, FILLED);
   else                       /* Draw outline box */
      rect(xc-xs, yc-ys, 2.0*xs, 2.0*ys, THIN);

   } /* End cmpltcsel() */


/*---------------------------------------------------------------------*
*                              cmaddvar                                *
*                                                                      *
*  This routine adds a variable into sums for averaging.  Putting      *
*  this bit of code in a subroutine allows two sets of connection      *
*  loops to be merged into one that calls either cmpltvar or cmaddvar. *
*---------------------------------------------------------------------*/

static void cmaddvar(si32 var) {

   struct CMVSUMS *pcmv = (struct CMVSUMS *)CDAT.psws;
   int ndx = CM.lcmsmap & (KCM_GCFLIJ|KCM_COFLIJ|KCM_IAFLIJ) ?
      Lija.lijval : Lija.jsyn;
   pcmv[ndx].sumvar = jasl(pcmv[ndx].sumvar, var);
   pcmv[ndx].numvar += 1;

   } /* End cmaddvar() */


/*---------------------------------------------------------------------*
*                               d3ijpl                                 *
*---------------------------------------------------------------------*/

void d3ijpl(void) {

   register struct IJPLDEF *pc;  /* Ptr to current IJPLDEF block */

/* Loop over linked list of IJPLDEF blocks */

   for (pc=RP0->pfijpl; pc; pc=pc->pnijpl) {

      void (*AddOrPlot)(si32);   /* Ptrs to sum or plot routine */
      void (*PlotFn)(si32);
      struct CELLTYPE *il;       /* Ptr to target cell type */
      struct CONNTYPE *ix;       /* Ptr to target conn type */
      struct CMVSUMS  *pcmv;     /* Ptr to averaging sums */
      ilst            *pil;      /* Ptr to iteration list */
      rd_type         *pvar0;    /* Ptr to 1st plot var for cell 0 */
      rd_type         *pvar;     /* Ptr to current plot variable */

      ilst  cmilst;              /* Fake ilst to plot all cells */
      iter  cmiter;              /* Cell list iterator */

      float szmult;              /* Symbol size multiplier */

      size_t coff;               /* Offset to prd data for this cell */
      long  nconns;              /* Number of conns in cell */
      long  nuk;                 /* Number of connections used */
      int   icell,jcell;         /* Current cell number */
      ui32  llc;                 /* Length of connection data */
      int   ltnel,ltncx,ltngx;   /* For locating target cell */
      int   ncells;              /* Number of cells in plot */
      si32  Mijtest;             /* Test for Mij a clock */
      si32  var;                 /* Variable to be plotted */

/* Omit plot if disabled */

      if (pc->kcmsmap & KCM_DISABL) continue;

/*---------------------------------------------------------------------*
*                             Plot setup                               *
*---------------------------------------------------------------------*/

      il = pc->pcmtl;            /* Ptr to target cell type */
      ix = pc->pcmtx;            /* Ptr to target conn type */
      CM.lopt = pc->cmopt;       /* Option controls */
      CM.lcmsmap = pc->kcmsmap;  /* Cell mapping variations */
      llc = (ui32)ix->lc;        /* Length of connection data  */

      /* Set current color and retrace mode */
      CM.icolor = -1;
      retrace((CM.lopt & CMRTR | RP->kplot & PLRTR) ?
         RETRACE : NORETRACE);
      /* Probably faster to copy than test whether will be needed */
      memcpy(RP->colors[CI_SPEX], pc->cmeico[EXCIT], NEXIN*COLMAXCH);
      CM.doseq = pc->doseq;
      CM.exbgr = pc->exbgr;
      CM.inbgr = pc->inbgr;
      CM.pfnsc = pc->pfnsc;

      /* Generate constants needed to compute x,y from Lij */
      CM.Vloc = pc->Sloc;
      CM.lsncx = (int)pc->srcncx;
      CM.lsnel = (int)ix->srcnel;   /* Cannot overflow */
      CM.lsngx = (int)ix->srcngx;   /* Cannot overflow */
      if (pc->kcmplt == KCIJSEL) {
         szmult = 0.375*pc->smscale;
         PlotFn = cmpltcsel;
         }
      else {
         switch (pc->kcmplt) {
         case KCIJC0:
            pvar0 = ix->psyn0 + ix->lxn + ix->cnoff[CIJ0];
            break;
         case KCIJM:
            pvar0 = ix->psyn0 + ix->lxn + ix->cnoff[MIJ];
            Mijtest = ix->Cn.mticks - S15;
            break;
         case KCIJPPF:
            pvar0 = ix->psyn0 + ix->lxn + ix->cnoff[PPF];
            break;
         case KCIJR:
         case KCIJES:
            pvar0 = ix->psyn0 + ix->lxn + ix->cnoff[RBAR];
            break;
            } /* End kcmplt switch */

         /* Variable range tests are saved in IJPLDEF.  As of R67,
         *  these are on the same scale as the plot variable, noting
         *  that Cij and Cij0 are scaled to S14 so the input limits
         *  can include +/- 1.0.  */
         CM.lcmmin = pc->cmmin;     /* Min var */
         CM.lcmamn = pc->cmamn;     /* Min abs(var) */
         /* Variable range and multiplier to get color bin */
         CM.vard = pc->cmmax - pc->cmmin;
         if (CM.vard <= 0) CM.vard = 1;   /* JIC */
         CM.v2cseq = (CM.doseq & DCS_CSEQ) ?
            (SEQNUM << CM_V2CSeqS)/CM.vard :
            (1 << CM_V2MaxCM)/CM.vard;

         /* Set parameters for determining symbol size, either con-
         *  stant or as a multiple of value of plot variable (multi-
         *  plier is scaled up to make average symbol bigger).  */
         szmult = (CM.lopt & CMVARY ? 0.5/(float)CM.vard : 0.375);
         PlotFn = cmpltvar;
         } /* End else not KCIJSEL */
      CM.szsx =  szmult*pc->Sloc.cdx;
      CM.szsy = -szmult*pc->Sloc.cdy;

/* Locate user's cell list or create one to select all cells.
*  Initiate iteration over this list.  */

      if (pc->pcmcl) {           /* Have an explicit cell list */
         pil = pc->pcmcl->pclil;
         ncells = (int)ilstitct(pil, (long)il->nelt);
         }
      else {                     /* Plot all cells */
         pil = &cmilst;
         ncells = il->nelt;
         memset((char *)&cmilst, 0, sizeof(cmilst));
         cmilst.nusm1 = (long)(ncells > 1);
         cmilst.pidl = (ilstitem *)&cmilst.frst;
         cmilst.last = IL_REND | (long)(ncells - 1);
         }
      ilstset(&cmiter, pil, (long)il->locell);

/* Set to do plots of individual cells or averages over cells */

      nconns = (long)(CM.lcmsmap & (KCM_GCFLIJ|KCM_COFLIJ|KCM_IAFLIJ) ?
         ix->srcnelt : ix->nc);
      if (CM.lopt & CMTAVG) {
         /* Clear arrays for accumulating sums of plot variable */
         pcmv = (struct CMVSUMS *)CDAT.psws;
         memset((char *)pcmv, 0, nconns*sizeof(struct CMVSUMS));
         AddOrPlot = cmaddvar;
         }
      else {
         /* Pick up variables needed to locate target cell */
         ltncx = (int)pc->tgtncx;
         if (CM.lopt & CMTMAP)
            icell = 0;
         else {
            ltnel = il->nel;
            ltngx = il->pback->ngx;
            }
         AddOrPlot = PlotFn;
         }

/*---------------------------------------------------------------------*
*                           Loop over cells                            *
*        Either accumulate sums for averaging or plot directly         *
*---------------------------------------------------------------------*/

      while ((jcell = (int)ilstiter(&cmiter)) >= 0) {

         /* Locate origin of source box for this cell */
         if (!(CM.lopt & CMTAVG)) {
            if (CM.lopt & CMTMAP) {
               div_t sxy = div(icell, ltncx);
               CM.Vloc.xlo = pc->Sloc.xlo +
                  (float)sxy.rem*pc->Tloc.cdx;
               CM.Vloc.yhi = pc->Sloc.yhi +
                  (float)sxy.quot*pc->Tloc.cdy;
               ++icell;
               }
            else {
               div_t gc  = div(jcell, ltnel);
               div_t gxy = div(gc.quot, ltngx);
               div_t cxy = div(gc.rem, ltncx);
               CM.Vloc.xlo = pc->Sloc.xlo +
                  (float)gxy.rem*pc->Tloc.gdx +
                  (float)cxy.rem*pc->Tloc.cdx;
               CM.Vloc.yhi = pc->Sloc.yhi +
                  (float)gxy.quot*pc->Tloc.gdy +
                  (float)cxy.quot*pc->Tloc.cdy;
               }
            }

         /* Locate center of C,K, or Q distribution.
         *  Position is stored in CM.ifst[xy] for PlotFn */
         if (CM.lcmsmap & KCM_FSTXYA) {
            d3kijf(il, ix, jcell);
            ix->lijbr1(ix);
            CM.ifstx = (int)Lija.fstx;
            CM.ifsty = (int)Lija.fsty;
            }
         else
            CM.ifstx = CM.ifsty = 0;

         /* Initialize to obtain Lij,Cij,Sj as needed */
         d3kiji(il, jcell);      /* Set Lij retrieval for this */
         d3kijx(ix);             /*   cell and connection type */
         d3lijx(ix);
         coff = Lija.lcoff;
         nuk = Lija.nuk;

/* We use a separate connection loop for each type of variable in
*  order to avoid a switch inside the loop.  Even with CMSMAP, we
*  call Lija.lijbr in order to detect skipped connections.  */

         switch (pc->kcmplt) {

         case KCIJC:          /* Cij plot */
         case KCIJSEL:        /* Color sensitivity plot */
            d3cijx(ix);
            for (Lija.isyn=0; Lija.isyn<nuk; ++Lija.isyn) {
               if (!Lija.lijbr(ix)) break;
               if (ix->cijbr(ix))   /* Cij, S31 --> S14 */
                  AddOrPlot(SRA(Lija.cijval, CM_Cij2S14));
               Lija.psyn += llc;
               } /* End loop over connections */
            break;

         case KCIJC0:         /* Cij0 plot */
            pvar = pvar0 + coff;
            for (Lija.isyn=0; Lija.isyn<nuk; ++Lija.isyn) {
               if (!Lija.lijbr(ix)) break;
               d3gtjs2(var, pvar);
               if (var == SI32_SGN) var = 0;
               AddOrPlot(var);
               pvar += llc;
               Lija.psyn += llc;
               } /* End loop over connections */

         case KCIJM:          /* Mij plot */
            pvar = pvar0 + coff;
            for (Lija.isyn=0; Lija.isyn<nuk; ++Lija.isyn) {
               if (!Lija.lijbr(ix)) break;
               /* If Mij is acting as a timer, use mxmij */
               d3gtjs2(var, pvar);
               if (var <= Mijtest) var = (si32)ix->Cn.mxmij;
               AddOrPlot(var);
               pvar += llc;
               Lija.psyn += llc;
               } /* End loop over connections */
            break;

         case KCIJS:          /* Sj plot */
            d3sjx(ix);
            for (Lija.isyn=0; Lija.isyn<nuk; ++Lija.isyn) {
               if (!Lija.lijbr(ix)) break;
               if (ix->sjbr(ix))
                  AddOrPlot(Lija.sjval);
               Lija.psyn += llc;
               } /* End loop over connections */
            break;

         case KCIJES:         /* Effective Sj plot */
            d3sjx(ix);
            pvar = pvar0 + coff;
            for (Lija.isyn=0; Lija.isyn<nuk; ++Lija.isyn) {
               if (!Lija.lijbr(ix)) break;
               if (ix->sjbr(ix)) {
                  d3gtjs2(var, pvar);
                  var = Lija.sjval - var;
                  AddOrPlot(var);
                  }
               pvar += llc;
               Lija.psyn += llc;
               } /* End loop over connections */
            break;

         default:             /* PPF or Rj plot */
            pvar = pvar0 + coff;
            for (Lija.isyn=0; Lija.isyn<nuk; ++Lija.isyn) {
               if (!Lija.lijbr(ix)) break;
               d3gtjs2(var, pvar);
               AddOrPlot(var);
               pvar += llc;
               Lija.psyn += llc;
               } /* End loop over connections */
            } /* End kcmplt switch */

         } /* End loop over cells */

/* If averaging, loop over connections a second time, now plotting
*  the average value of the plot variable for each connection.  By
*  making separate loops to count in Lija.jsyn or Lija.lijval we can
*  avoid checking this every time in the PlotFn.  */

      if (CM.lopt & CMTAVG) {
         if (CM.lcmsmap & (KCM_GCFLIJ|KCM_COFLIJ|KCM_IAFLIJ)) {
            for (Lija.lijval=0; Lija.lijval<nconns; ++Lija.lijval) {
               if (pcmv[Lija.lijval].numvar) {
                  var = jdswq(pcmv[Lija.lijval].sumvar,
                     pcmv[Lija.lijval].numvar);
                  PlotFn(var);
                  }
               }
            }
         else {
            for (Lija.jsyn=0; Lija.jsyn<nconns; ++Lija.jsyn) {
               if (pcmv[Lija.jsyn].numvar) {
                  var = jdswq(pcmv[Lija.jsyn].sumvar,
                     pcmv[Lija.jsyn].numvar);
                  PlotFn(var);
                  }
               }
            }
         } /* End averaging plot */

/*---------------------------------------------------------------------*
*                         Draw box and title                           *
*---------------------------------------------------------------------*/

/* Draw a box...
*  This code now follows the drawing of the connections so that
*     the boundary will supervene when symbols overlap it.  */

      pencol(RP->colors[CI_BOX]);
      rect(pc->cmpltx, pc->cmplty, pc->cmpltw, pc->cmplth, THIN);

/* Add source grid lines if requested */

      if (CM.lopt & CMSGRD) {
         float xyg,xygi,xyge,xtop;
         retrace(NORETRACE);
         /* Draw x (vertical) grid lines */
         xtop = pc->cmpltx + pc->cmpltw;
         xygi = pc->Tloc.gdx > 0 ? pc->Tloc.gdx : pc->Tloc.cdx;
         xyge = xtop - 0.9*xygi;
         for (xyg=pc->cmpltx+xygi; xyg<xyge; xyg+=xygi)
            line(xyg, pc->cmplty, xyg, pc->cmtopy);
         /* Draw y (horizontal) grid lines */
         xygi = pc->Tloc.gdy > 0 ? pc->Tloc.gdy : pc->Tloc.cdy;
         xyge = pc->cmtopy + 0.9*xygi;
         for (xyg=pc->cmplty-xygi; xyg<xyge; xyg-=xygi)
            line(pc->cmpltx, xyg, xtop, xyg);
         }

/* Add a title unless suppressed */

      if (!(CM.lopt & CMNTTL)) {
         if (pc->cmltc[0]) pencol(pc->cmltc);
         symbol(pc->cmpltx, pc->cmplty-RP->stdlht, 0.7*RP->stdlht,
            getrktxt(pc->cmltxt), 0, pc->cmltln);
         }

      } /* End pc loop */

   } /* End d3ijpl() */

