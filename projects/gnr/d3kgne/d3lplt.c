/* (c) Copyright 1994-2010, Neurosciences Research Foundation, Inc. */
/* $Id: d3lplt.c 31 2010-08-30 18:57:56Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                              d3lplt.c                                *
*                                                                      *
*                     Cell layer plotting routine                      *
*                                                                      *
*   ---This function should be called in parallel from all nodes---    *
*               (It may or may not do anything on host)                *
*                                                                      *
*  void d3lplt(struct CELLTYPE *il, s_type *psi, int icyc, float xl,   *
*     float yl, float width, float height)                             *
*                                                                      *
*  il:     Ptr to layer to be plotted                                  *
*           (d3rplt must be called first for setup)                    *
*  psi:    Ptr to array containing s(i) starting at my low cell        *
*  icyc:   Cycle number.  0 to plot rep name, -1 to omit altogether.   *
*  xl,yl:  X,Y coords of lower left corner                             *
*  width:  Plot width                                                  *
*  height: Plot height                                                 *
*                                                                      *
*  This routine has been separated from d3rplt so that each cell       *
*  layer can be plotted individually, just after the new s(i) have     *
*  been calculated, but before they are exchanged out.  This assures   *
*  that the actual s(j) used in the calculation are still available,   *
*  so that "DRIVE" plots show the correct connections.  Furthermore,   *
*  this arrangement speeds up parallel execution by keeping the plot   *
*  pipeline more evenly filled.                                        *
*                                                                      *
*  V6D, 02/04/94, GNR - Broken out from d3rplt                         *
*  V7A, 05/02/94, GNR - Pass psi from caller to handle serial nonsuper *
*  V7B, 06/09/94, GNR - KRPDP gives anatomy only for DETAIL list cells *
*  V8A, 04/17/95, GNR - Changes for plotting VJ,VW,VT                  *
*  Rev, 09/28/97, GNR - Independent dynamic allocations per conntype   *
*  Rev, 10/31/97, GNR - Increase min cell radius from 0.006 to 0.021   *
*  V8A, 12/11/97, GNR - Stratified: vertical spacing proportional to   *
*                       number of cells, interleaved: fix offset bug,  *
*                       allow Cij == mxcij to plot                     *
*  V8B, 01/06/01, GNR - Implement detailed print via ilst package      *
*  Rev, 09/20/01, GNR - Mark probed cells with a "V"                   *
*  V8C, 03/16/03, GNR - Cell responses in millivolts, add conductances *
*  V8D, 07/29/06, GNR - Use lijbr() to access Lij even if not stored   *
*  Rev, 10/12/07, GNR - Add '3bar' option to display noise info        *
*  Rev, 12/27/07, GNR - Add CTPQB (color by QBAR)                      *
*  ==>, 12/28/07, GNR - Last mod before committing to svn repository   *
*  Rev, 03/26/08, GNR - Implement CELLTYPE SHAPES with triangles       *
*  Rev, 01/17/09, GNR - Reorganize colors into a single array          *
*  V8F, 02/21/10, GNR - Remove PLATFORM senses                         *
*  Rev, 06/30/10, GNR - Add TV_SRC and PP_SRC for neuroanatomy plots   *
*  Rev, 07/01/10, GNR - Add ix->napc and kctp CTPSB options            *
***********************************************************************/

#define CLTYPE  struct CLBLK
#define LIJTYPE struct LIJARG
#define PPTYPE  struct PREPROC
#define TVTYPE  struct TVDEF

#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "sysdef.h"
#include "d3global.h"
#include "celldata.h"
#include "tvdef.h"
#include "clblk.h"
#include "rksubs.h"
#include "nccolors.h"
#include "plots.h"

extern struct CELLDATA CDAT;
extern LIJTYPE Lija;

/* Phase to color shift = log2(PHASE_RES/SEQNUM) */
#define PTOC (PHASE_EXP - SEQN_EXP)

/* Ratio of size of anatomy plot arrow barb to size of cell */
#define ARBRAT 0.5
#define ARBANG 30.0
/* Ratio of size of probe "V" to size of largest cell */
#define PSZRAT 1.20
#define NTRV   3                 /* No. of vertices of a triangle */
#define COS30  0.8660254
#define SIN30  0.5

void d3lplt(struct CELLTYPE *il, s_type *psi, int icyc, float xl,
   float yl, float width, float height) {

#ifndef PAR0
   struct REPBLOCK *ir = il->pback; /* Ptr to current repertoire     */
   struct REPBLOCK *irs;         /* Ptr to source REPBLOCK           */
   struct CONNTYPE *ix;          /* Ptr to conntype block            */
   struct CONNTYPE *ix1;         /* Ptr to first neuroanat conntype  */
   struct NAPSLIM  *pnapsl;      /* Ptr to subarbor box limits       */
   struct PRBDEF   *pprb;        /* Ptr to probe info & plot switch  */
   rd_type *psbr;                /* Ptr to sbar, qbar, izhu, s(t-1)  */

   iter  dplit;                  /* Cell list iterator               */
   iter  prbit;                  /* Probe iterator                   */

   float yh;                     /* y coord of top edge of plot      */
   float cdx,cdy;                /* Cell delta x,y                   */
   float gdx,gdy;                /* Group delta x,y                  */

   float barbsz;                 /* Anatomy plot arrow barb size     */
   float bdx,bdy;                /* Width, height of 3Bar,2Bar bars  */
   float pszcos,pszsin;          /* Probe size times sin or cos(30)  */
   float rcpt;                   /* Circle plot threshold            */
   float size;                   /* Largest circle size (inches/S14) */
   float sxc,syc;                /* x,y coords of a source cell      */
   float trx[NTRV],try[NTRV];    /* Vertices of a triangle           */
   float xc,yc;                  /* x,y coords of a cell             */
   float xlo,yhi;                /* x,y coords of first cell         */
   float dy;                     /* Temp used in x,y calculations    */

   long ccpt;                    /* 1.0 - cell plot threshold (S14)  */
   long hts7;                    /* Local copy of il->ht (S7)        */
   long iac;                     /* Current absolute cell number     */
   long iace;                    /* Last cell on current node plus 1 */
   long isbr;                    /* Increment for psbr               */
   long Lij;                     /* Lij of current connection        */
   long lnel;                    /* Local copy of nel                */
   long lngx,lngy;               /* Number groups along x,y          */
   long lnqx,lnqg;               /* Cols,rows per group              */
   long si;                      /* Value of s(i)                    */

   si32 Cij;                     /* Cij of current connection        */
   si32 iv;                      /* Value that sets conn color       */
   ui32 lctp,rkrp;               /* Plot switches for layer,region   */

   int ic,jc;                    /* Current,previous color selectors */
   int kfill;                    /* Draw filled objects switch       */
   int krt;                      /* Retrace switch                   */
   int llc;                      /* Local copy of lc                 */

   enum SiColors ksico;          /* Color drawing switch             */
   enum SiShapes ksiop;          /* Shape drawing switch             */

   short mxmmn;                  /* Max Cij minus min Cij            */
   ui16  nctu;                   /* Number of conntype used (jijpl)  */
   short offMIJ;                 /* Offset to MIJ                    */

   char detlisted;               /* TRUE if cell on detail list      */
   char karrw;                   /* Layer arrow plot switch          */
   char kbp;                     /* Bubble or square plot            */
   char kdrve;                   /* Drive plot switch                */
   char klijp;                   /* Neuroanatomy plot on             */
   char klijq;                   /* Neuroanatomy plot final switch   */
   char klong;                   /* Long arrows switch               */
   byte knapc;                   /* Neuroanatomy plot color switch   */
   byte ksblk;                   /* Subarbor block plot              */
   char needsj;                  /* TRUE if need to fetch sj         */
   char dockdlplus;              /* Do check detail list for plus    */
   char nockdlanat;              /* Don't check detail list for anat */

/* Omit all calculations if no cells to plot */

   if (ir->Rp.krp & KRPNZ && il->mcells) {

/* Activate timing statistics */

      stattmr(OP_PUSH_TMR, GRPH_TMR);

/* Pick up information from cell and repertoire blocks */

      yh   = yl + height;
      hts7 = SRA(il->Ct.ht,(FBwk-FBsi)); /* Hit test value (S7) */
      lnel = il->nel;
      lctp = il->Ct.kctp;
      rkrp = ir->Rp.krp;
      lngx = ir->ngx;
      lngy = ir->ngy;

/* Set option switches */

      kbp    = (rkrp & (KRPPL|KRPBP)) != 0;
      kfill  = lctp & KRPCO ? FILLED : THIN;
      klijp  = (lctp & (KRPNP|KRPTP|CTPLV|KRPDR|KRPPJ|CTPSB)) != 0;
      krt    = (rkrp & KRPPR | RP->kplot & PLRTR) ?
                  RETRACE : NORETRACE;
      ksblk  = (lctp & CTPSB) != 0;
      ksico  = (enum SiColors)il->ksipcu;
      ksiop  = (enum SiShapes)il->ksiplu;
      dockdlplus = (lctp & CTPOM) == 0;
      nockdlanat = (lctp & KRPTP) == 0;

      /* Determine whether probed cells need to be marked */
      pprb = (RP->kevtu & EVTPRB) ?
         il->pctprb + il->ctclid[CTCL_PRBSL] : NULL;
      if (pprb && !(pprb->prbopt & PRBMARK)) pprb = NULL;
      ilstset(&prbit, pprb ? pprb->pclb->pclil : NULL, il->locell);

      /* Override bubble and anatomy plot options if config plot */
      if (icyc < 0) {
         kbp = TRUE; ksiop = SIS_Square; klijp = FALSE; }

/* Calculate origin and group and cell plotting increments */

      /* If only one cell per group, set lnqx, lnqg accordingly.
      *  In plots, group boxes are not allowed to coalesce as they
      *  do in the printed output  */
      if (ir->ncpg == 1) { lnqx = 1;       lnqg = 1;       }
      else               { lnqx = ir->nqx; lnqg = ir->nqg; }

      gdx = width/(float)lngx;            /* Group delta x */
      cdx = gdx/(float)lnqx;              /* Cell delta x  */
      cdy = -height/(float)(lngy*lnqg);   /* Cell delta y  */
      if (rkrp & KRPIL) {                 /* INTERLEAVED   */
         gdy = cdy * (float)lnqg;         /* Group delta y */
         dy  = cdy; }                     /* For yhi calc  */
      else {                              /* STRATIFIED    */
         gdy = cdy * (float)il->lnqy;     /* Group delta y */
         dy  = cdy * (float)lngy; }       /* For yhi calc  */

      /* Calc x,y coords of first cell, offsetting this layer
      *  along y according to sizes of previous layers.  */
      xlo = xl + 0.5*cdx;
      yhi = yh + 0.5*cdy + dy*(float)il->lnqyprev;

      /* Set retrace mode and force switch to drawing color */
      jc = -1;
      retrace(krt);

      /* Calculate radius of a maximal bubble (inches).
      *  V8C: Took out max size, let user set it via cpr */
      if (ksiop == SIS_3Bar || ksiop == SIS_2Bar) {
         bdx = 0.3333 * il->Ct.cpr * cdx;
         size = bdy = 0.5 * il->Ct.cpr * fabsf(cdy); }
      else size = 0.5 * il->Ct.cpr *
         (RP->cpdia ? RP->cpdia : min(cdx,fabs(cdy)));
      barbsz = ARBRAT*size;
      if (pprb) {
         pszsin = size * (PSZRAT * SIN30);
         pszcos = size * (PSZRAT * COS30); }
      ccpt = S14 - il->Ct.cpt;
      size /= dS14;                       /* Now it's radius scale */
      rcpt = max(size*il->Ct.cpt, MINBPRAD);

      /* Set pointer used to locate sbar, qbar, izhu for cell color */
      psbr = il->prd; isbr = il->lel;
      switch (ksico) {
      case SIC_SBar: psbr += il->ctoff[SBAR]; break;
      case SIC_QBar: psbr += il->ctoff[QBAR]; break;
      case SIC_Hist: psbr = il->pps[0]+spsize(il->locell,il->phshft);
                     isbr = il->lsp; break;
      case SIC_Izhu: psbr += il->ctoff[IZVU] + LSIZE; break;
      case SIC_Fixed: strncpy(RP->colors[CI_SPEC],
         il->Ct.sicolor, COLMAXCH);
         } /* End ksico switch */

      /* Set pointers and switches needed for neuroanatomy plots */
      if (klijq = klijp && il->jijpl) {      /* Assignment intended */
         klong = (lctp&KRPPJ) == 0;          /* Project vectors?    */
         karrw = (lctp&CTPAR) != 0;          /* Plotting arrows?    */
         kdrve = (lctp&KRPDR) != 0;          /* Drive plot?         */
         mxmmn = il->mxcij - il->mncij;     /* Range of plotted Cij */

         /* Preliminary conntype scan--save setup in cnwk.rplt    */
         nctu = il->nct;
         for (ix=ix1=il->pct1; ix&&ix->ict<=nctu; ix=ix->pct) {

            /* If jijpl<0, do all CONNTYPEs, else just jijpl'th one */
            if ((si16)ix->ict  < il->jijpl) continue;
            if ((si16)ix->ict == il->jijpl) ix1 = ix, nctu = ix->ict;

            /* Encode line coloring method for neuroanatomy plot  */
            ix->cnwk.rplt.knapc0 = ix->Cn.napc;
            /* If asks for Mij and Mij not there, color it red */
            if (ix->Cn.napc == NAPC_MIJ && !cnexists(ix,MIJ))
               ix->cnwk.rplt.knapc0 = NAPC_SOLID;

            /* Set up params needed to locate source cells for plotting.
            *  Geometry of source cells (interleaved or stratified) is
            *  same as used for plotting the source layer itself, irre-
            *  gardless of whether projection or long vectors are drawn.
            *  While this might lead to longer vectors if the two types
            *  are mixed in a projection plot, it is the only way that
            *  avoids confusion if both layers are plotted on the same
            *  superposition plot.  It would be permissible to match the
            *  target geometry to source in a non-superposition plot. If
            *  source has no valid location, set cnflgs SKPNAP.  */

            ix->cnwk.rplt.rppc = 0;    /* Kills PREPROC Lij anal */
            switch (ix->cnsrctyp) {

            case REPSRC: {             /* Input from REPERTOIRE */
               /* Set source position for repertoire input */
               struct REPBLOCK *is2;
               /* Get pointer to source celltype */
               struct CELLTYPE *ilsrc = (struct CELLTYPE *)ix->psrc;
               float sdy;              /* Source cell delta y */
               irs = ilsrc->pback;
               is2 = klong ? irs : ir;

               /* Calculate X coords */
               ix->cnwk.rplt.sxlo = is2->aplx + 0.5*
                  (ix->cnwk.rplt.scdx = (ix->cnwk.rplt.sgdx =
                     is2->aplw/irs->ngx) / irs->nqx);

               /* Calculate Y coords */
               sdy = ix->cnwk.rplt.scdy =
                  -is2->aplh/(float)(irs->ngy*irs->nqg);
               ix->cnwk.rplt.syhi = is2->aply + is2->aplh + 0.5*sdy;
               /* Check for INTERLEAVED (vs. STRATIFIED) layering */
               if (irs->Rp.krp & KRPIL) {    /* INTERLEAVED */
                  ix->cnwk.rplt.sgdy = sdy*(float)irs->nqg;
                  }
               else {                        /* STRATIFIED */
                  ix->cnwk.rplt.sgdy = sdy*(float)ilsrc->lnqy;
                  sdy *= (float)irs->ngy;  /* For syhi calc below */
                  }
               /* Correct SYHI according to position of target
               *  layer in its own repertoire */
               ix->cnwk.rplt.syhi += sdy*(float)ilsrc->lnqyprev;

               /* Correct coordinates if lines are from a source
               *  repertoire that is offset due to KP=V plotting. */
               if ((icyc > 0) && (RP->kplot & PLVTS) &&
                     (is2->Rp.krp & KRPVT)) {
                  float fcycle = (float)(RP->ntc - icyc);
                  ix->cnwk.rplt.sxlo -= fcycle*is2->vxo;
                  ix->cnwk.rplt.syhi -= fcycle*is2->vyo;
                  } /* End KP=V adjustment */

               } /* End case REPERTOIRE */
               break;

            case IA_SRC:               /* Input array */
               if (klong) {   /* Long vectors */
                  ix->cnwk.rplt.sxlo = RP->eplx +
                     0.5*(ix->cnwk.rplt.sgdx =  RP->eplw/RP->nsx);
                  ix->cnwk.rplt.syhi = RP->eply + RP->eplh +
                     0.5*(ix->cnwk.rplt.sgdy = -RP->eplh/RP->nsy); }
               else {         /* Short vectors */
                  ix->cnwk.rplt.sxlo = ir->aplx +
                     0.5*(ix->cnwk.rplt.sgdx =  ir->aplw/RP->nsx);
                  ix->cnwk.rplt.syhi = ir->aply + ir->aplh +
                     0.5*(ix->cnwk.rplt.sgdy = -ir->aplh/RP->nsy); }

               /* Get window offset for connections tied to windows */
               ix->osrc = d3woff(ix);
               break;

            case USRSRC:
            case VJ_SRC:
            case VW_SRC:
            case VT_SRC: {    /* Environment & external sensors */
               /* Get pointer to source vcell block */
               struct VCELL *pvc = (struct VCELL *)ix->psrc;
               if (klong) {   /* Long vectors */
                  ix->cnwk.rplt.sxlo = pvc->vcplx +
                     0.5*(ix->cnwk.rplt.scdx = pvc->vcplw/pvc->nvx);
                  ix->cnwk.rplt.syhi = pvc->vcply + pvc->vcplh +
                     0.5*(ix->cnwk.rplt.scdy = -pvc->vcplh/pvc->nvytot);
                  }
               else {         /* Short vectors */
                  ix->cnwk.rplt.sxlo = ir->aplx +
                     0.5*(ix->cnwk.rplt.scdx = ir->aplw/pvc->nvx);
                  ix->cnwk.rplt.syhi = ir->aply + ir->aplh +
                     0.5*(ix->cnwk.rplt.scdy = -ir->aplh/pvc->nvytot);
                  }
               } /* End sensors */
               break;

            case TV_SRC: {    /* Camera image source */
               struct TVDEF *ptv = (struct TVDEF *)ix->psrc;
               float fnx = (float)ptv->utv.tvx;
               float fny = (float)ptv->utv.tvy;
               if (klong) {   /* Long vectors */
                  /* Can't do this plot if image is 1:1 bitmap */
                  if (ptv->tvwd <= 0.0 || ptv->tvht <= 0.0)
                     ix->cnflgs |= SKPNAP;
                  else {
                     ix->cnwk.rplt.sxlo = ptv->tvplx +
                        0.5*(ix->cnwk.rplt.scdx = ptv->tvwd/fnx);
                     ix->cnwk.rplt.syhi = ptv->tvply + ptv->tvht +
                        0.5*(ix->cnwk.rplt.scdy = -ptv->tvht/fny);
                     }
                  }
               else {         /* Short vectors */
                  ix->cnwk.rplt.sxlo = ir->aplx +
                     0.5*(ix->cnwk.rplt.scdx = ir->aplw/fnx);
                  ix->cnwk.rplt.syhi = ir->aply + ir->aplh +
                     0.5*(ix->cnwk.rplt.scdy = -ir->aplh/fny);
                  }
               } /* End camera input */
               break;

            case PP_SRC: {    /* Image preprocessor source */
               struct PREPROC *pip = (struct PREPROC *)ix->psrc;
               float fnx = (float)pip->upr.nipx;
               if (klong) {   /* Long vectors */
                  /* Can't do this plot if image is 1:1 bitmap */
                  if (pip->ipwd <= 0.0)
                     ix->cnflgs |= SKPNAP;
                  else {
                     ix->cnwk.rplt.sgdx = pip->dcoff;
                     ix->cnwk.rplt.rppc =
                        (ui32)pip->kpc*(ui32)pip->upr.nipy;
                     ix->cnwk.rplt.sxlo = pip->ipplx +
                        0.5*(ix->cnwk.rplt.scdx = pip->ipwd/fnx);
                     ix->cnwk.rplt.syhi = pip->ipply + pip->ipht +
                        0.5*(ix->cnwk.rplt.scdy = -pip->ipht/
                        (float)ix->cnwk.rplt.rppc);
                     }
                  }
               else {         /* Short vectors */
                  ix->cnwk.rplt.sxlo = ir->aplx +
                     0.5*(ix->cnwk.rplt.scdx = ir->aplw/fnx);
                  ix->cnwk.rplt.syhi = ir->aply + ir->aplh +
                     0.5*(ix->cnwk.rplt.scdy = -ir->aplh/
                     (float)pip->nytot);
                  }
               } /* End image preprocessor input */
               break;

            default:
               ix->cnflgs |= SKPNAP;

               } /* End cnsrctyp switch */
            } /* End conntype loop */

         } /* End if neuroanatomy plot */

/*---------------------------------------------------------------------*
*                                                                      *
*                           Plot s(i) data                             *
*                                                                      *
*---------------------------------------------------------------------*/

      /* Loop over cells on my node */
      iac = il->locell;
      iace = iac + il->mcells;
      ilstset(&dplit, il->dpclb ? il->dpclb->pclil : NULL, iac);
      iv = 0;                    /* Clear unreferenced bits */
      for ( ; iac<iace; psi+=il->lsp,psbr+=isbr,iac++) {

         /* Calculate location of the current cell.  (Earlier
         *  versions of d3rplt avoided modulo operations here
         *  at cost of much more complex looping structure.)  */
         ldiv_t qrmgrp = ldiv(iac, lnel); /* Group and cell */
         ldiv_t qrmg = ldiv(qrmgrp.quot, lngx);
         ldiv_t qrmc = ldiv(qrmgrp.rem,  lnqx);

         xc = xlo + qrmg.rem*gdx  + qrmc.rem*cdx;
         yc = yhi + qrmg.quot*gdy + qrmc.quot*cdy;

         /* Get s(i) now, may be needed for KP=D option test
         *  as well as SIC_Si coloring */
         d3gts2(si, psi);        /* mV S7/14 */

         /* Set a flag if current cell selected for detail print.
         *  (This method is faster than just calling ilsttest().)  */
         detlisted = (ilstnow(&dplit) == iac);
         if (detlisted) ilstiter(&dplit);

         /* Perform neuroanatomy plot if requested.  The following
         *  tests are performed to screen connections for plotting:
         *  ALWAYS:  Lij in bounds, mncij <= Cij <= mxcij
         *  DRIVE:   si >= ht, sj-sjrev > et or sj-sjrev < etlo
         *  KRPDP:   cell was selected for detail print
         *  MIJ:     Mij > 0 (Neg Mij is a tick count or just an
         *           inconvenient negative value that would need
         *           special color selection code to plot.  */
         if (klijq && (nockdlanat | detlisted) &&
               (!kdrve || si >= hts7)) {

            d3kiji(il, iac);     /* Set to access current cell */

            /* Loop over all connection types or one type requested */
            for (ix=ix1; ix&&ix->ict<=nctu; ix=ix->pct) {
               if (ix->cnflgs & SKPNAP) continue;
               knapc = ix->cnwk.rplt.knapc0;
               needsj = (knapc == NAPC_SJ) || kdrve;
               irs = (ix->cnsrctyp == REPSRC) ?
                  ((struct CELLTYPE *)ix->psrc)->pback : NULL;
               llc = (int)ix->lc;
               offMIJ = ix->cnoff[MIJ];
               d3kijx(ix);
               d3lijx(ix);
               d3cijx(ix);
               if (needsj) d3sjx(ix);

               /* If doing CTPSB plot, initialize the block limits */
               if (ksblk) {
                  struct NAPSLIM *pnsl,*pnsle;
                  pnsl = pnapsl = (struct NAPSLIM *)CDAT.psws;
                  pnsle = pnsl + ix->nsarb;
                  for ( ; pnsl<pnsle; ++pnsl) {
                     pnsl->npxmn = pnsl->npymn = 1E12;
                     pnsl->npxmx = pnsl->npymx = -1E12;
                     }
                  }

               /* Loop over connections */
               for (Lija.isyn=0; Lija.isyn<Lija.nuk;
                     ++Lija.isyn,Lija.psyn += llc) {

                  /* Pick up Lij and exit if at end of list or skip
                  *  if in a movable window that is out-of-bounds. */
                  if (!Lija.lijbr(ix)) break;
                  Lij = Lija.lijval;
                  if (ix->kgen & KGNTP &&
                     (Lij += ix->osrc) & RP->xymaskc) continue;

                  /* Pick up Cij, scale to (S8), do range test.
                  *  N.B.  cijbr cannot return NO here, because
                  *  if it would, lijbr above would do it first. */
                  ix->cijbr(ix);
                  Cij = SRA(Lija.cijval, 31-8);
                  if (Cij > il->mxcij || Cij < il->mncij) continue;

                  /*  Pick up sj if needed for drive or sj color.
                  *  If testing drive level, do so now.  */
                  if (needsj) {
                     ix->sjbr(ix);
                     iv = Lija.sjval;
                     if (kdrve) {
                        si32 esj = iv - (si32)ix->Cn.sjrev;
                        if (esj <= (si32)ix->effet &&
                            esj >= (si32)ix->effnt) continue;
                        }
                     }

                  /* Set the color for this connection.
                  *  Note: sj is left in iv from code just above.
                  *     If using Cij or Mij, must pick it up.  */
                  switch (knapc) {
                  case NAPC_CIJ:       /* Color from Cij */
                     ic = ((Cij-il->mncij)<<SEQN_EXP)/mxmmn;
                     break;
                  case NAPC_SJ:        /* Color from sj */
                     ic = iv >> 11;    /* Gives 16 mV steps */
                     break;
                  case NAPC_MIJ:       /* Color from Mij */
                     /* Pick up Mij and be sure value is positive,
                     *  thus not currently encoding a time.  */
                     d3gts2(iv, Lija.psyn+offMIJ);
                     if (iv <= 0) continue;
                     ic = (iv<<SEQN_EXP)/ix->Cn.mxmij;
                     break;
                  case NAPC_SUBARB:    /* Color per subarbor */
                     /* This is sort of a permanent debug option */
                     ic = Lija.jsyn/ix->nsa;
                     break;
                  case NAPC_SOLID:     /* Make it red */
                     ic = 1;
                     break;
                     }  /* End knapc switch */
                  /* Prevent selecting a nonexistent color */
                  if (ic > SEQNUM-1) ic = SEQNUM-1;
                  if (ic != jc) pencol(RP->colors[CI_SEQ+(jc=ic)]);

                  /* Calculate plotting location of source cell */
                  switch (ix->cnsrctyp) {

                  case REPSRC: {       /* Input from REPERTOIRE */
                     ldiv_t qrmb,qrmg,qrmq;
                     qrmb = ldiv(Lij, ix->srcnel);
                     qrmg = ldiv(qrmb.quot, irs->ngx);
                     qrmq = ldiv(qrmb.rem , irs->nqx);
                     sxc = ix->cnwk.rplt.sxlo +
                        qrmg.rem*ix->cnwk.rplt.sgdx +
                        qrmq.rem*ix->cnwk.rplt.scdx;
                     syc = ix->cnwk.rplt.syhi +
                        qrmg.quot*ix->cnwk.rplt.sgdy +
                        qrmq.quot*ix->cnwk.rplt.scdy;
                     } /* End REPERTOIRE */
                     break;

                  case IA_SRC: {       /* Input array */
                     ldiv_t qrm = ldiv(Lij, RP->nsy);
                     sxc = ix->cnwk.rplt.sxlo +
                        (qrm.quot>>1)*ix->cnwk.rplt.sgdx;
                     syc = ix->cnwk.rplt.syhi +
                        qrm.rem*ix->cnwk.rplt.sgdy;
                     } /* End IA */
                     break;

                  case USRSRC:
                  case VJ_SRC:
                  case VW_SRC:
                  case VT_SRC:
                  case TV_SRC:
                  case PP_SRC: {       /* Sensors and Images */
                     float fy;
                     ldiv_t qrm = ldiv(Lij, ix->srcngx);
                     sxc = ix->cnwk.rplt.sxlo +
                        qrm.rem*ix->cnwk.rplt.scdx;
                     if (ix->cnwk.rplt.rppc) {
                        /* This case is complicated by multicolumn
                        *  plotting of PREPROCs in tvplot() */
                        long ly = qrm.quot;
                        qrm = ldiv(ly, (long)ix->cnwk.rplt.rppc);
                        sxc += (float)qrm.quot*ix->cnwk.rplt.sgdx;
                        fy = (float)qrm.rem;
                        }
                     else
                        fy = (float)qrm.quot;
                     syc = ix->cnwk.rplt.syhi + fy*ix->cnwk.rplt.scdy;
                     } /* End sensors and images */
                     break;

                     } /* End cnsrctyp switch */

                  /* If block plot, just keep track of limits */
                  if (ksblk) {
                     struct NAPSLIM *pnsl = pnapsl +
                        (Lija.jsyn/ix->nsa);
                     if (sxc < pnsl->npxmn) pnsl->npxmn = sxc;
                     if (syc < pnsl->npymn) pnsl->npymn = syc;
                     if (sxc > pnsl->npxmx) pnsl->npxmx = sxc;
                     if (syc > pnsl->npymx) pnsl->npymx = syc;
                     }
                  /* Draw an arrow or a simple line, as requested */
                  else if (karrw) arrow(sxc,syc,xc,yc,barbsz,ARBANG);
                  else            line(xc,yc,sxc,syc);
                  } /* End isyn for */

               /* Do box plots around subarbors */
               if (ksblk) {
                  float x1,x2,y1,y2;
                  int isub;
                  for (isub=0; isub<ix->nsarb; ++isub) {
                     struct NAPSLIM *pnsl = pnapsl + isub;
                     x1 = pnsl->npxmn, x2 = pnsl->npxmx;
                     y1 = pnsl->npymn, y2 = pnsl->npymx;
                     /* Skip if nothing there */
                     if (x1 > x2 || y1 > y2) continue;
#if SEQNUM == 8
                     ic = isub & (SEQNUM-1);
#else
                     ic = isub % SEQNUM;
#endif
                     if (ic != jc) pencol(RP->colors[CI_SEQ+(jc=ic)]);
                     rect(x1, y1, x2-x1, y2-y1, THIN);
                     line(xc, yc, 0.5*(x2+x1), 0.5*(y2+y1));
                     }
                  } /* End CTPSB plot */

               } /* End ict for */

            } /* End NEUROANATOMY plot */

         /* Perform bubble plot if requested */
         if (kbp) {
            float d,r;
            r = rcpt;         /* For DPRNT marking if no bubble */
            if (si >= il->Ct.cpt) {
               /* Set plot radius */
               r = size*(float)si;
               r = max(r, MINBPRAD);
               d = 2.0*r;

               /* Set plot color */
               switch (ksico) {
               case SIC_Fixed:         /* Use fixed color */
                  ic = CI_SPEC;
                  break;
               case SIC_Izhu:
                  d3gti4(si, psbr);    /* Get IZHU to S7 */
                  si = SRA(si, (FBwk-FBsi));
                  goto CkColorVarRange;
               case SIC_Hist: {
                  long sio;
                  d3gts2(sio, psbr);
                  ic = CI_HPP + (si < hts7) + ((sio < hts7)<<1); }
                  break;
               case SIC_Phase:         /* Color according to phase */
                  ic = CI_PHS +
                     ((int)(psi[LSmem] & PHASE_MASK) >> PTOC);
                  break;
               case SIC_QBar:          /* Replace si with */
               case SIC_SBar:          /*    some other variable */
                  d3gts2(si, psbr);
CkColorVarRange:  if (si < il->Ct.cpt) {
                     ic = CI_SEQ; break; }
                  /* Fall through to SIC_Si ... */
               case SIC_Si:
               case SIC_None:          /* JIC */
                  ic = ((si-(long)il->Ct.cpt)<<SEQN_EXP)/ccpt;
                  if (ic > SEQNUM-1) ic = SEQNUM-1;
                  ic += CI_SEQ;
                  break;
                  } /* End ksico switch */
               if (ic != jc) pencol(RP->colors[jc=ic]);

/*** N.B.  The KRP3B option is currently experimental and useful
*  only for the case where there is only one cell in the layer,
*  because d3go saves noise and nmod values only temporarily.
*  To be used for more cells, would need to add storage for
*  these items in the general or once-per-celltype data areas
*  and retrieve them here.
****/

               switch (ksiop) {
               case SIS_None:
               case SIS_Circle:
                  circle(xc,yc,r,kfill);
                  break;
               case SIS_Square:
                  rect(xc-r,yc-r,d,d,kfill);
                  break;
               /* N.B.  Draw si bar first using color set above */
               case SIS_3Bar:
                  rect(xc-1.5*bdx,yc-bdy,bdx,d,kfill);
                  pencol("GREEN");
                  dy = bdy * (float)CDAT.noise/(dS27/2.0);
                  rect(xc-0.5*bdx,yc-bdy,bdx,dy,kfill);
                  pencol("X073");
                  /* Revised code, 06/09/09, 3rd bar now plots A(1)
                  *  if it exists instead of noise modulation.  */
#if 1
                  dy = bdy * (float)*CDAT.psumaij/(dS27/2.0);
                  if (dy < 0.0)
                     rect(xc+0.5*bdx,yc-bdy+dy,bdx,-dy,kfill);
                  else
                     rect(xc+0.5*bdx,yc-bdy,bdx,dy,kfill);
#else
                  dy = bdy * (float)CDAT.nmodval/dS23;
                  rect(xc+0.5*bdx,yc-bdy,bdx,dy,kfill);
#endif
                  jc = -1;
                  break;
               case SIS_2Bar:
                  rect(xc-1.5*bdx,yc-bdy,bdx,d,kfill);
                  pencol("GREEN");
                  dy = bdy * (float)CDAT.noise/(dS27/2.0);
                  rect(xc-0.5*bdx,yc-bdy,bdx,dy,kfill);
                  jc = -1;
                  break;
               case SIS_DownTR:
                  dy = r*COS30;
                  trx[0] = xc, try[0] = yc - r;
                  trx[1] = xc - dy, trx[2] = xc + dy;
                  try[1] = try[2] = yc + r*SIN30;
                  goto PlotPolyLn;
               case SIS_LeftTR:
                  dy = r*COS30;
                  trx[0] = xc - r, try[0] = yc;
                  trx[1] = trx[2] = xc + r*SIN30;
                  try[1] = yc + dy, try[2] = yc - dy;
                  goto PlotPolyLn;
               case SIS_UpTR:
                  dy = r*COS30;
                  trx[0] = xc, try[0] = yc + r;
                  trx[1] = xc + dy, trx[2] = xc - dy;
                  try[1] = try[2] = yc - r*SIN30;
                  goto PlotPolyLn;
               case SIS_RightTR:
                  dy = r*COS30;
                  trx[0] = xc + r, try[0] = yc;
                  trx[1] = trx[2] = xc - r*SIN30;
                  try[1] = yc - dy, try[2] = yc + dy;
PlotPolyLn:       polyln(kfill, NTRV, trx, try);
                  break;
                  } /* End ksiop switch */
               } /* End si > threshold */

            /* Plot little blue 'V' if cell was probed */
            if (ilstnow(&prbit) == iac) {
               ilstiter(&prbit);
               pencol("BLUE"); jc = -2;
               line(xc,yc,xc+pszsin,yc+pszcos);
               line(xc,yc,xc+pszcos,yc+pszsin);
               } /* End if probed */

            /* Plot little red '+' mark if cell selected for DPRNT */
            if (dockdlplus & detlisted) {
               pencol("RED"); jc = -2;
               line(xc-r,yc,xc+r,yc);
               line(xc,yc-r,xc,yc+r);
               } /* End if detlisted */

            } /* End BUBBLE or SQUARE plot */

         } /* End cell loop */

      stattmr(OP_POP_TMR, GRPH_TMR);
      } /* End if KRPNZ and mcells != 0 */

#endif
   } /* End d3lplt() */
