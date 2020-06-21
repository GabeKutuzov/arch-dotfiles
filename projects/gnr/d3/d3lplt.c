/* (c) Copyright 1994-2018, The Rockefeller University *11116* */
/* $Id: d3lplt.c 78 2018-08-02 18:36:58Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                              d3lplt.c                                *
*                     Cell layer plotting routine                      *
*                                                                      *
*   ---This function should be called in parallel from all nodes---    *
*      (It expects not to be called if (kctp & CTPHP) is set and       *
*               it may or may not do anything on host)                 *
*                                                                      *
*  void d3lplt(struct CELLTYPE *il, s_type *psi0, int icyc,            *
*     float xla, float yla)                                            *
*                                                                      *
*  il:      Ptr to layer to be plotted                                 *
*             (d3rplt must be called first for setup)                  *
*  psi0:    Ptr to array containing s(i) starting at my low cell       *
*  icyc:    Cycle number.  0 to plot rep name, -1 to omit altogether.  *
*  xla,yla: Adjustment to X,Y coords of lower left corner for KRPVT.   *
*                                                                      *
*  This routine has been separated from d3rplt so that each cell       *
*  layer can be plotted individually, just after the new s(i) have     *
*  been calculated, but before they are exchanged out.  This assures   *
*  that the actual s(j) used in the calculation are still available,   *
*  so that "DRIVE" plots show the correct connections.  Furthermore,   *
*  this arrangement speeds up parallel execution by keeping the plot   *
*  pipeline more evenly filled.                                        *
*                                                                      *
*  EXECUTIVE DECISION, 02/16/18:  The GMxBars option is supposed to    *
*  plot a listed cell only if it is the cell with highest s(i) in its  *
*  group.  This information is not available at the time d3lplt() is   *
*  called from d3go() if the group is split across comp nodes, indeed, *
*  max s(i) may not be available until an UPWITH is executed after     *
*  other layers have been processed.  Decision is to plot all listed   *
*  cells in split groups that have s(i) above other cells on same node.*
*                                                                      *
*  Note that if a LPCLIST list is used, neuroanatomy plots will only   *
*  be made for cells on that list even if TPCLIST has other cells.     *
************************************************************************
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
*  Rev, 07/09/13, GNR - Plot asmul values if KRPAS is on               *
*  R65, 01/02/16, GNR - n[gq][xyg] to ui16, ngrp to ui32, ncpg to ubig *
*  R66, 03/11/16, GNR - Rename nvx->nsxl, nvy->nsya so != ix->nv[xy]   *
*  R72, 03/14/17, GNR - Move layer and neuroanat plot setup to d3cpli  *
*  R72, 04/01/17, GNR - Add plot shapes SIS_PhNdl and SIS_OrNdl        *
*  R73, 04/25/17, GNR - Add NAPC_EXIN                                  *
*  R74, 08/08/17, GNR - Add scaled excit,inhib colors ala ijplots,     *
*                       remove ic!=jc color tests, SPEX may differ     *
*  Rev, 09/02/17, GNR - Add GRPAVG and GRPMAX color for 1 symbol/group *
*  Rev, 09/13/17, GNR - Add CPCAP                                      *
*  Rev, 01/25/18, GNR - Bug fix in size calc--CTFPG uses min(gdx,gdy)  *
*  R77, 02/13/18, GNR - Enhanced d3lplt bars options, LPCLIST          *
***********************************************************************/

#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "sysdef.h"
#include "rkarith.h"
#include "d3global.h"
#include "celldata.h"
#include "lijarg.h"
#include "tvdef.h"
#include "clblk.h"
#include "rksubs.h"
#include "plots.h"

/* Define DBG_PLLIJ to plot Lij at each vector */
/***  DEBUG  #define DBG_PLLIJ  ***/

extern long allciter(iter *);
extern struct CELLDATA CDAT;
extern struct LIJARG Lija;
extern const double binsin[40];

/* Phase to color shift = log2(PHASE_RES/SEQNUM) */
#define PTOC (PHASE_EXP - SEQN_EXP)

/* Ratio of size of anatomy plot arrow barb to size of cell */
#define ARBRAT 0.5
#define ARBANG 30.0
/* Ratio of size of probe "V" to size of largest cell */
#define PSZRAT 1.20
/* Fraction of width of one bar in a bar plot that is gap */
#define BGAPFR 0.10
/* And a few general constants */
#define NTRV   3                 /* No. of vertices of a triangle */
#define COS30  0.8660254
#define SIN30  0.5

/* Table of hex output characters */
#ifndef PAR0
static char hextab[16] = "0123456789ABCDEF";
#endif

void d3lplt(struct CELLTYPE *il, s_type *psi0, int icyc, float xla,
      float yla) {

   struct REPBLOCK *ir = il->pback; /* Ptr to current repertoire     */
#ifndef PAR0
   struct CELLTYPE *ils;         /* Ptr to source CELLTYPE block     */
   struct CONNTYPE *ix;          /* Ptr to conntype block            */
   struct CONNTYPE *ix1;         /* Ptr to first neuroanat conntype  */
   struct CLBLK    *pnaclb;      /* Neuroanatomy (KRPTP) cell list   */
   struct NAPSLIM  *pnapsl;      /* Ptr to subarbor box limits       */
   struct PRBDEF   *pprb;        /* Ptr to probe info                */
   struct PRBDEF   *pprb0;       /* Starter pprb and v-plot switch   */
   char    *pcolor;              /* Ptr to selected color */
   rd_type *psbr,*psbr0;         /* Ptrs to [SQ]bar, izhu, s(t-1)    */
   s_type  *psi;                 /* Ptr to s(i) for current cell     */

   long (*iterator)(iter *);     /* Ptr to iterator function         */
   iter  lplit;                  /* Layer plot cell list iterator    */
   iter  dplit;                  /* Neuroanat (or dprnt) iterator    */

   float barbsz;                 /* Anatomy plot arrow barb size     */
   float barx0,bwid;             /* x offset to 1st bar, bar width   */
   float bary0;                  /* y offset to bottom of a bar      */
   float bdx,bdy;                /* Width, height of 3Bar,2Bar bars  */
   float bsize;                  /* Largest bar height (inches/S14)  */
   float csize;                  /* Largest circle size (inches/S14) */
   float dy;                     /* Temp used in x,y calculations    */
   float orrang;                 /* Orientation angle in radians     */
   float pszcos,pszsin;          /* Probe size times sin or cos(30)  */
   float rcpt;                   /* Circle plot threshold            */
   float sxc,syc;                /* x,y coords of a source cell      */
   float trx[NTRV],try[NTRV];    /* Vertices of a triangle           */
   float xc,yc;                  /* x,y coords of a cell             */

   long Lij;                     /* Lij of current connection        */

   si32 Cij;                     /* Cij of current connection        */
   si32 hts7;                    /* Local copy of il->ht (S7)        */
   si32 iv;                      /* Value that sets conn color       */
   ui32 lctp,rkrp;               /* Plot switches for layer,region   */
   si32 si,abssi;                /* Value of s(i), abs(si)           */
   si32 srngmod;                 /* S24/cpcap for mod colors (S14)   */
   si32 srngseq;                 /* S24/(si range) for seq colors    */
   si32 sswp;                    /* SI32_SGN to implement CTSWP      */

   int iac,iacu;                 /* Current absolute cell number     */
   int ic;                       /* Current color selection          */
   int kfill;                    /* Draw filled objects switch       */
   int krt;                      /* Retrace switch                   */
   int llc;                      /* Local copy of lc                 */
   int lnel;                     /* Local copy of nel                */
   int locell,hicell;            /* Low, high cell on this node      */
   int oldgrp,newgrp;            /* Previous,current group number    */

   enum SiColSrc kvsrc;          /* Cell drawing value source switch */
   enum SiShapes ksiop;          /* Shape drawing switch             */

   short mxmmn;                  /* Max Cij minus min Cij            */
   ui16  nctu;                   /* Number of conntype used (jijpl)  */
   short offMIJ;                 /* Offset to MIJ                    */

   char detlisted;               /* TRUE if cell on detail list      */
   char dobars;                  /* TRUE if SIS_Bars|SIS_GMxBars     */
   char doGAvg;                  /* TRUE if group average needed     */
   char doGMax;                  /* TRUE if group max needed         */
   char eiseq;                   /* TRUE if EXC,INH in same SEQUENCE */
   char karrw;                   /* Layer arrow plot switch          */
   char kbp;                     /* Bubble or square plot            */
   char kdrve;                   /* Drive plot switch                */
   char klijp;                   /* Neuroanatomy plot on             */
   byte knapc;                   /* Neuroanatomy plot color switch   */
   byte ksblk;                   /* Subarbor block plot              */
   char needsj;                  /* TRUE if need to fetch sj         */
   char dockdlplus;              /* Do check detail list for plus    */
   char nockdlanat;              /* Don't check detail list for anat */
   char cspecex[COLMAXCH];       /* Color spec for NAPC_EXIN excit   */
   char cspecin[COLMAXCH];       /* Color spec for NAPC_EXIN inhib   */

/* Omit all calculations if no cells to plot */

   if (ir->Rp.krp & KRPNZ && il->mcells) {

/* Activate timing statistics */

      stattmr(OP_PUSH_TMR, GRPH_TMR);

/* Pick up information from cell and repertoire blocks */

      locell = il->locell, hicell = il->hicell;
      iacu = 0;               /* Eliminate unitialized warning */
      hts7 = SRA(il->Ct.ht,(FBwk-FBsi)); /* Hit test value (S7) */
      lnel = il->nel;
      lctp = il->Ct.kctp;
      rkrp = ir->Rp.krp;

/* Set option switches */

      kbp    = (rkrp & (KRPPL|KRPBP)) != 0;
      kfill  = lctp & KRPCO ? FILLED : THIN;
      klijp  = lctp & KRPNPL && il->jijpl != 0;
      krt    = (rkrp & KRPPR | RP->kplot & PLRTR) ?
                  RETRACE : NORETRACE;
      ksblk  = (lctp & CTPSB) != 0;
      kvsrc  = (enum SiColSrc)il->ksipcu;
      ksiop  = (enum SiShapes)il->ksiplu;
      dockdlplus = (lctp & CTPOM) == 0;
      nockdlanat = (lctp & KRPTP) == 0;
      doGAvg = doGMax = FALSE;
      sswp   = (lctp & CTSWP) ? SI32_SGN : 0;

      /* Override bubble and anatomy plot options if config plot */
      if (icyc < 0) {
         kbp = TRUE; ksiop = SIS_Square; klijp = FALSE; }

      /* If needles in group plot, adjust [xy]la to center */
      if (ksiop == SIS_OrNdl) {
         xla += 0.5*(il->Lp.gdx - il->Lp.cdx);
         yla += 0.5*(il->Lp.gdy - il->Lp.cdy);
         }

      /* Store plot cycle offsets for possible NANAT use */
      il->xlal = xla, il->ylal = yla;

      /* Determine whether probed cells need to be marked */
      pprb0 = NULL;     /* Switch for v-plot and PRBDEF starter */
      if (RP->kevtu & EVTPRB) {
         for (pprb=il->pctprb; pprb; pprb=pprb->pnctp) {
            if (pprb->prbopt & PRBMARK) {
               ilstset(&pprb->prbit, pprb->pclb->pclil, (long)locell);
               pprb0 = il->pctprb;
               }
            } /* End PRBDEF loop */
         } /* End EVTPRB check */

      /* Set retrace mode */
      retrace(krt);

      /* Calculate radius of a maximal bubble (inches).
      *  V8C: Took out max size, let user set it via cpr */
      csize = 0.5 * il->Ct.cpr * ((il->ctf & CTFPG) ?
         min(il->Lp.gdx, fabs(il->Lp.gdy)) :
         min(il->Lp.cdx, fabs(il->Lp.cdy)));
      /* Constants needed for placing bars on bar plots.
      *  Assignment intended in next line.  */
      if ((dobars = (ksiop == SIS_Bars | ksiop == SIS_GMxBars))) {
         if (ksiop == SIS_Bars) {
            bdx = il->Ct.cpr * il->Lp.cdx / (float)il->Ct.ksbarn;
            bdy = il->Ct.cpr * fabsf(il->Lp.cdy); }
         else if (ksiop == SIS_GMxBars) {
            bdx = il->Ct.cpr * il->Lp.gdx / (float)il->Ct.ksbarn;
            bdy = il->Ct.cpr * fabsf(il->Lp.gdy);
            doGMax = TRUE; }
         barx0 = bdx*(0.5*(float)il->Ct.ksbarn - BGAPFR);
         bwid = bdx*(1.0 - 2.0*BGAPFR);
         bary0 = 0.5*bdy;
         }  /* End if bars */

      barbsz = ARBRAT*csize;
      orrang = PI/(float)il->nel;
      if (pprb0) {
         pszsin = csize * (PSZRAT * SIN30);
         pszcos = csize * (PSZRAT * COS30); }
      csize /= dS14;                       /* Now it's radius scale */
      bsize = 2.0*csize;
      rcpt = max(csize*il->Ct.cpt, RP->mnbpr);
      /* Set multipliers for mapping values to colors.  Note:  If new
      *  variables are ever added to SiColSrc that can only be excit or
      *  only inhib, eiseq should be set FALSE for those regardless of
      *  the tests here.  */
      iv = (si32)il->Ct.cpcap;
      srngmod = S24/max(iv,1);
      eiseq = (il->sicop & (SICECSQ|SICICSQ)) == (SICECSQ|SICICSQ) &&
         !(RP->compat & OLDRANGE);
      if (eiseq) iv += iv; else iv -= il->Ct.cpt;
      srngseq = S24/max(iv,1);

      /* Set pointer used to locate Sbar, Qbar, izhu for cell color */
      psbr0 = il->prd;
      switch (kvsrc) {
      case SIC_SBar: psbr0 += il->ctoff[SBAR]; break;
      case SIC_QBar: psbr0 += il->ctoff[QBAR]; break;
      case SIC_Izhu: psbr0 += il->ctoff[IZU]; break;
      case SIC_GAvg: doGAvg = TRUE; break;
      case SIC_GMax: doGMax = TRUE; break;
         } /* End kvsrc switch */

      /* Set pointers and switches needed for neuroanatomy plots */
      if (klijp) {
         karrw = (lctp&CTPAR) != 0;          /* Plotting arrows?    */
         kdrve = (lctp&KRPDR) != 0;          /* Drive plot?         */
         mxmmn = il->mxcij - il->mncij;      /* Range of plotted Cij */
         strcpy(cspecex, "Z000000");         /* excit EXIN pencol   */
         strcpy(cspecin, "Z000000");         /* inhib EXIN pencol   */

         /* Preliminary conntype scan--set ix1,nctu to control
         *  loop over conntypes during plot.  */
         nctu = il->nct;
         for (ix=ix1=il->pct1; ix&&ix->ict<=nctu; ix=ix->pct) {

            /* If jijpl<0, do all CONNTYPEs, else just jijpl'th one */
            if ((si16)ix->ict  < il->jijpl) continue;
            if ((si16)ix->ict == il->jijpl) ix1 = ix, nctu = ix->ict;

            /* Get window offset for connections tied to windows */
            if (ix->cnsrctyp == VS_SRC)
               ix->osrc = d3woff(ix);

            } /* End conntype loop */

         } /* End if neuroanatomy plot */

/*---------------------------------------------------------------------*
*                                                                      *
*                           Main cell loop                             *
*                                                                      *
*  Note:  With SIC_GAvg and SIC_GMax, we are going to loop over the    *
*  cells in the group normally so neuroanatomy can be plotted (all to  *
*  the same group center), but skip cell plot until the end of group.  *
*---------------------------------------------------------------------*/

      /* Loop over cells on my node */
      if (il->plpclb) {
         iterator = ilstiter;
         ilstset(&lplit, il->plpclb->pclil, (ilstitem)locell);
         }
      else {
         iterator = allciter;
         lplit.inow = (ilstitem)locell;
         lplit.iend = (ilstitem)(hicell - 1);
         }
      oldgrp = -1;               /* Force new group on first cell */
      pnaclb = il->ptclb ? il->ptclb : il->dpclb;
      ilstset(&dplit, pnaclb ? pnaclb->pclil : NULL, (ilstitem)locell);

      iv = 0;                    /* Clear unreferenced bits */
      while ((iac = (int)iterator(&lplit)) >= 0) {
         size_t lcoff;
         int relcell = iac - locell;
         lcoff = uw2zd(jmuwj(il->lel, relcell));
         psi = psi0 + spsize(relcell, il->phshft);
         psbr = psbr0 + lcoff;

         /* Calculate location of the current cell.  (Earlier
         *  versions of d3rplt avoided modulo operations here
         *  at cost of much more complex looping structure.)
         *  CTFPG types are plotting at center of group.  */
         if (il->ctf & CTFPG) {
            newgrp = iac/lnel;      /* Group number */
            div_t qrmg = div(newgrp, (int)ir->ngx);
            xc = il->Lp.xlo + xla + qrmg.rem*il->Lp.gdx;
            yc = il->Lp.yhi + yla + qrmg.quot*il->Lp.gdy;
            }
         else {                     /* Others on cell loc */
            div_t qrmgrp = div(iac, lnel);  /* Group and cell */
            div_t qrmg = div(qrmgrp.quot, (int)ir->ngx);
            div_t qrmc = div(qrmgrp.rem,  ir->nqx);
            newgrp = qrmgrp.quot;
            xc = il->Lp.xlo + xla + qrmg.rem*il->Lp.gdx +
                                    qrmc.rem*il->Lp.cdx;
            yc = il->Lp.yhi + yla + qrmg.quot*il->Lp.gdy +
                                    qrmc.quot*il->Lp.cdy;
            }

         /* Get s(i) now, may be needed for KP=D option test
         *  as well as SIC_Si or SIS_Bar coloring */
         d3gtjs2(si, psi);        /* mV S7/14 */

         /* Set a flag if current cell is selected for detail print.
         *  (Now that we have separate clists for layer plot and detail
         *  print, must use ilsttest rather than ilstnow here.)  */
         detlisted = ilsttest(dplit.pilst, (long)iac);

/*---------------------------------------------------------------------*
*               Perform neuroanatomy plot if requested                 *
*                                                                      *
* The following tests are performed to screen connections for plotting:*
*     ALWAYS:  Lij in bounds, mncij <= Cij <= mxcij                    *
*     DRIVE:   si >= ht, sj-sjrev > et or sj-sjrev < etlo              *
*     KRPDP:   cell was selected for detail print                      *
*     MIJ:     Mij > 0 (Neg Mij is a tick count or just an             *
*              inconvenient negative value that would need             *
*              special color selection code to plot.                   *
*---------------------------------------------------------------------*/

         if (klijp && (nockdlanat | detlisted) &&
               (!kdrve || si >= hts7)) {

            d3kiji(il, iac);     /* Set to access current cell */

            /* Loop over all connection types or one type requested */
            for (ix=ix1; ix&&ix->ict<=nctu; ix=ix->pct) {
               if (ix->cnflgs & SKPNAP) continue;
               knapc = ix->knapc0;
               needsj = (knapc == NAPC_SJ) || kdrve;
               ils = (ix->cnsrctyp == REPSRC) ?
                  (struct CELLTYPE *)ix->psrc : NULL;
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
                  if (ix->kgen & KGNST &&
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
                  *     If using Cij or Mij, must pick it up.
                  *  Currently, for NAPC_EXIN, we are just using shades
                  *  of red and green to avoid all the fuss with deccol
                  *  etc. in the d3ijpl plots.  */
                  if (knapc == NAPC_EXIN) {
                     if (Cij >= 0) {         /* Excitatory */
                        si32 gg = SRA(BYTE_MAX * Cij, 8);
                        /* Avoid pure black-->white */
                        if (gg == 0) gg = 1;
                        cspecex[3] = hextab[gg >> 4];
                        cspecex[4] = hextab[gg & 15];
                        pencol(cspecex);
                        }
                     else {                  /* Inhibitory */
                        si32 rr = SRA(BYTE_MAX * abs32(Cij), 8);
                        if (rr == 0) rr = 1;
                        cspecin[5] = hextab[rr >> 4];
                        cspecin[6] = hextab[rr & 15];
                        pencol(cspecin);
                        }
                     }  /* End handling for NAPC_EXIN */
                  else {
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
                        d3gtjs2(iv, Lija.psyn+offMIJ);
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
                     pencol(RP->colors[CI_SEQ+ic]);
                     }

                  /* Calculate plotting location of source cell */
                  switch (ix->cnsrctyp) {

                  case REPSRC: {       /* Input from REPERTOIRE */
                     div_t qrmb,qrmg,qrmq;
                     qrmb = div((int)Lij, (int)ix->srcnel);
                     qrmg = div(qrmb.quot, (int)ix->srcngx);
                     qrmq = div(qrmb.rem , ils->lnqx);
                     sxc = ix->Np.xlo + ils->xlal +
                        (float)qrmg.rem*ix->Np.gdx +
                        (float)qrmq.rem*ix->Np.cdx;
                     syc = ix->Np.yhi + ils->ylal +
                        (float)qrmg.quot*ix->Np.gdy +
                        (float)qrmq.quot*ix->Np.cdy;
                     } /* End REPERTOIRE */
                     break;

                  case IA_SRC:         /* Input array */
                  case VS_SRC: {       /* Scan window */
                     div_t qrm = div((int)Lij, (int)RP->nsy);
                     sxc = ix->Np.xlo +
                        (float)qrm.quot*ix->Np.cdx;
                     syc = ix->Np.yhi +
                        (float)qrm.rem*ix->Np.cdy;
                     } /* End IA */
                     break;

                  case PP_SRC: {       /* Image preprocessor */
                     float fy;
                     div_t qrm = div((int)Lij, (int)ix->srcngx);
                     sxc = ix->Np.xlo + (float)qrm.rem*ix->Np.cdx;
                     if (ix->rppc) {
                        /* This case is complicated by multicolumn
                        *  plotting of PREPROCs in tvplot() */
                        qrm = div(qrm.quot, (int)ix->rppc);
                        sxc += (float)qrm.quot*ix->Np.cdx;
                        fy = (float)qrm.rem;
                        }
                     else
                        fy = (float)qrm.quot;
                     syc = ix->Np.yhi + fy*ix->Np.cdy;
                     } /* End image preprocessor */
                     break;

                  default: {           /* Sensors and Images */
                     div_t qrm = div((int)Lij, (int)ix->srcngx);
                     sxc = ix->Np.xlo + (float)qrm.rem*ix->Np.cdx;
                     syc = ix->Np.yhi + (float)qrm.quot*ix->Np.cdy;
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
#ifdef DBG_PLLIJ
                  {  char cLij[6];
                     memset(cLij, 0, 6);
                     wbcdwtPn(&Lij, cLij, RK_LFTJ|RK_IORF|RK_NLONG|5);
                     symbol(sxc, syc-0.05, 0.1, cLij, 0, 6);
                     }
#endif
                  } /* End isyn for */

               /* Do box plots around subarbors */
               if (ksblk) {
                  float x1,x2,y1,y2;
                  ui32 isub;
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
                     pencol(RP->colors[CI_SEQ+ic]);
                     rect(x1, y1, x2-x1, y2-y1, THIN);
                     line(xc, yc, 0.5*(x2+x1), 0.5*(y2+y1));
                     }
                  } /* End CTPSB plot */

               } /* End ict for */

            } /* End NEUROANATOMY plot */

/*---------------------------------------------------------------------*
*         Perform cell activity ("bubble") plot if requested           *
*----------------------------------------------------------------------*
*  Three levels of indent suppressed from here to end of bubble plot   *
*---------------------------------------------------------------------*/

if (kbp) {
   float r;

   /* Calculate group average or max if new group entered.  As docu-
   *  mented in "EXECUTIVE DECISION" above, must use partial group if
   *  locell on node is not locell in group or if hicell on node is
   *  not end of group.  This code is placed after neuroanatomy plot
   *  because doGAvg,doGMax do not affect what is plotted there.
   *  Design note:  It is possible in theory for the first group to
   *  be partial at bottom and top (more nodes than groups).  If
   *  piece of group on this node is not largest, or highest of
   *  equals, skip all cells on node.  */
   if (doGAvg | doGMax) {
      if (newgrp != oldgrp) {
         s_type *psg;            /* Ptr to cell in group */
         s_type *psg0,*psge;     /* 1st,last s(i) in this group */
         int igc0,igce;          /* Low,high cell in this group */
#ifdef PAR
         int jgrp = iac % lnel;  /* Location of cell in its grp */
         igce = (igc0 = iac - jgrp) + lnel;
         /* Following code is designed to do as much as possible
         *  in the relatively rare cases when if clause is TRUE.
         *  But we can't check for exit on locell overlap until
         *  length of current node piece is determined later.  */
         if (igc0 < locell) igc0 = locell;
         if (igce > hicell) {
            if (igce - hicell >= hicell - igc0) {
               /* Piece above is bigger, skip this group */
               iacu = -1; goto SetOldGroup; }
            igce = hicell; }
         if (igce - igc0 < jgrp) {
            /* Piece below is bigger, skip this group */
            iacu = -1; goto SetOldGroup; }
         {  int rgc0 = igc0 - locell;
            psg0 = psi0 + spsize(rgc0, il->phshft); }
         {  int rgce = igce - locell;
            psge = psi0 + spsize(rgce, il->phshft); }
# else   /** SERIAL ***/
         igce = (igc0 = iac) + lnel;
         psg0 = psi0 + spsize(igc0, il->phshft);
         psge = psi0 + spsize(igce, il->phshft);
#endif
         if (doGAvg) {
            si64 siGsum = jesl(0);
            for (psg=psg0; psg<psge; psg+=il->lsp) {
               d3gtjs2(si, psg);    /* mV S7/14 */
               siGsum = jasl(siGsum, si);
               }
            si = drswq(siGsum, igce-igc0);
            iacu = igce - 1;     /* Only plot at last cell in grp */
            }
         if (doGMax) {
            si32 siGmax = -SI32_MAX;
            int  igmx = igc0;
            for (psg=psg0; psg<psge; ++igmx,psg+=il->lsp) {
               d3gtjs2(si, psg);    /* mV S7/14 */
               if (si > siGmax) siGmax = si, iacu = igmx;
               }
            si = siGmax;
            }
#ifdef PAR
SetOldGroup:
#endif
         oldgrp = newgrp;
         }  /* End new group entered */
      if (iac != iacu) continue; /* Go to next cell */
      }  /* End if doGAvg|doGMax */

   abssi = abs32(si);
   if (abssi >= il->Ct.cpt) {
      if (dobars) {

/*=====================================================================*
*  Plot multiple bars.  To keep code as simple as possible, and bars   *
*  as big as possible, bar height is based on absolute value of plot   *
*  variable and color differs for positive vs negative values.  Fixed  *
*  colors are set here for different bar types, coded to make it easy  *
*  to add these to RP->colors and the COLORS card if so desired.       *
*  Variables that are not stored in the data structure (Pers, Noise)   *
*  are recomputed if needed.  Bars are killed in d3news for variables  *
*  that do not exist at all.                                           *
*=====================================================================*/

         rd_type *pbv;           /* Ptr to bar variable */
         float x = xc - barx0, y = yc - bary0;
         si32 bv,tbv;            /* Bar variables, temp */
         int ibar, ibare = (int)il->Ct.ksbarn;
         for (ibar=0; ibar<ibare; x+=bdx,++ibar) {
            switch ((enum SiBarId)(il->Ct.kksbar[ibar] & 0x0f)) {
            case SIB_None:       /* Skip this bar */
               continue;
            case SIB_Si:         /* Bar height from s(i) */
               bv = si;
               pcolor = RP->colors[(bv^sswp) >= 0 ? CI_EXC : CI_INH];
               break;
            case SIB_Vm:         /* Bar height from Vm */
               pbv = psbr + il->ctoff[VMP];
               d3gtjl4a(bv, pbv);
               bv = SRA(bv, FBwk-FBsi);
               pcolor = (bv^sswp) >= 0 ? "X04F" : "X0F4";
               break;
            case SIB_Pers:       /* Bar hgt from persistence */
               if (RP->compat & OLDPERS &&
                  il->Ct.rspmeth < RF_BREG) tbv = si;
               else {
                  pbv = psbr + il->ctoff[VMP];
                  d3gtjl4a(tbv, pbv);
                  tbv = SRA(tbv, FBwk-FBsi); }
               bv = mrsrsl(tbv, il->Dc.omegau, FBod);
               bv = SRA(bv, FBwk-FBsi);
               pcolor = (bv^sswp) >= 0 ? "X40E" : "XE04";
               break;
            case SIB_Depr:       /* Bar hgt from depression */
               pbv = psbr + il->ctoff[DEPR];
               d3gtjl1(bv, pbv); /* DEPR is S8 */
               bv <<= Sv2mV;
               pcolor = "X0EE";
               break;
            case SIB_Noise: {    /* Bar height from noise */
               orns wnsd = il->nsd0;
               udevwskp(&wnsd, jslsw(jesl(iac),1));
               bv = d3noise(il->No.nmn, il->No.nsg,
                  CDAT.nmodval, wnsd);
               bv = SRA(bv, FBwk-FBsi);
               pcolor = "XE40"; }
               break;
            case SIB_Izhu:       /* Bar hgt from Izhikevich 'u' */
               pbv = psbr + il->ctoff[IZU];
               d3gtjl4a(bv, pbv);
               bv = SRA(bv, FBIu-FBsi);
               pcolor = "XE0E";
               break;
            case SIB_Aff: {      /* Bar height from afference */
               float r2;
               si32 abak,bak = *(si32 *)(il->prdbak[ibar] + lcoff);
               si32 araw,raw = *(si32 *)(il->prdraw[ibar] + lcoff);
               si32 bv2;
               bak = SRA(bak, FBwk-FBsi); abak = abs32(bak);
               raw = SRA(raw, FBwk-FBsi); araw = abs32(raw);
               bv = min(abak,araw);
               if (abak == araw) {  /* Just one bar */
                  pcolor = RP->colors[(bv^sswp) >= 0 ? CI_EXC : CI_INH];
                  break; }
               if (abak > araw) {   /* AK is upper bar */
                  bv2 = abak;
                  pencol(RP->colors[(bak^sswp) >= 0 ? CI_EXC : CI_INH]);
                  pcolor = (raw^sswp) >= 0 ? "X66F" : "X6F6";
                  }
               else {               /* RAW is upper bar */
                  bv2 = araw;
                  pencol((raw^sswp) >= 0 ? "X66F" : "X6F6");
                  pcolor = RP->colors[(bak^sswp) >= 0 ? CI_EXC:CI_INH];
                  }
               /* Draw the upper bar */
               r = bsize*(float)bv;
               r2 = bsize*(float)bv2;
               if (bwid < RP->mnbpr)
                  line(x, y+r, x, y+r2);
               else
                  rect(x, y+r, bwid, r2, kfill);
               break;   /* Drop through to draw lower bar ... */
               }  /* End SIB_Aff local scope */
               }  /* End kksbar switch */

            /* Got single bar value in bv or doing lower bar of two
            *  for afference affected by vdep mult etc.  Use color
            *  selected in switch above according to item type.
            *  Draw rectangle, or just a line if dbx is very small.  */
            pencol(pcolor);
            r = bsize*(float)abs32(bv);
            if (bwid < RP->mnbpr)
               line(x, y, x, y+r);
            else
               rect(x, y, bwid, r, kfill);
            }  /* End ibar loop */
         }  /* End if dobars */

      else {

         /*=================*/
         /*  Set plot size  */
         /*=================*/
         r = csize*(float)abssi;
         if (r < RP->mnbpr) r = RP->mnbpr;

         /*==================*/
         /*  Set plot color  */
         /*==================*/
         switch (kvsrc) {
         case SIC_None:          /* (Changed to SIC_Si in d3news) */
         case SIC_Si:            /* Color from s(i) */
         case SIC_GAvg:          /* Color from average s(i) */
         case SIC_GMax:          /* Color from max s(i) in grp */
            break;
         case SIC_SBar:          /* Color from Sbar */
         case SIC_QBar:          /* Color from Qbar */
            d3gtjs2(si, psbr);
            break;
         case SIC_Phase:         /* Color according to phase */
            pcolor = RP->colors[CI_PHS +
               ((int)(psi[LSmem] & PHASE_MASK) >> PTOC)];
            goto UsePColor;
         case SIC_Hist: {        /* Color according to history */
            si32 sio;
            /* N.B. Here we are looking up a cell in bcst info,
            *  so I need absolute, not relative cell number */
            psbr = il->pps[0] + spsize(iac, il->phshft);
            d3gtjs2(sio, psbr);
            pcolor = RP->colors[CI_HPP +
               (si < hts7) + ((sio < hts7)<<1)]; }
            goto UsePColor;
         case SIC_Izhu:
            d3gtjl4a(si, psbr);  /* Get IZHU to S7 */
            si = SRA(si, (FBwk-FBsi));
            break;
            } /* End kvsrc switch */

         /* Derive color from variable set above */
         if (eiseq) {            /* s(i) range in one SEQUENCE */
            if (si >= il->Ct.cpcap)       ic = SEQNUM-1;
            else if (si <= -il->Ct.cpcap) ic = 0;
            else                          ic =
               mrssl(si+(si32)il->Ct.cpcap, srngseq, 24-SEQN_EXP);
            pcolor = RP->colors[CI_SEQ + ic];
            goto UsePColor;
            }  /* End exc,inh range in one SEQUENCE */
         /* Or use a fixed color */
         else if (lctp & CTPFC) {
            pcolor = il->Ct.sicolor[si < 0];
            goto UsePColor; }
         else {
            bgrv *peics;         /* Ptr to unscaled color */
            si32 bb,gg,rr;       /* Scaled colors */
            if (si >= 0) {       /* Excitatory */
               if (il->sicop & SICECOM) goto SkipThisSi;
               if (il->sicop & SICECSQ) goto GenSeqColor;
               peics = &il->bexbgr;
               pcolor = RP->colors[CI_SPEX];
               }
            else {               /* Inhibitory */
               if (il->sicop & SICICOM) goto SkipThisSi;
               si = abs32(si);
               if (il->sicop & SICICSQ) goto GenSeqColor;
               peics = &il->binbgr;
               pcolor = RP->colors[CI_SPIN];
               }
            /* deccol() returns S8, si is S7/14 */
            if (si > il->Ct.cpcap) si = il->Ct.cpcap;
            /* Scaled excit or inhib color.  (If COMPAT=C,
            *  negative case remains harmlessly unused.)  */
            bb = mrssl(si * peics->bb, srngmod, 24);
            gg = mrssl(si * peics->gg, srngmod, 24);
            rr = mrssl(si * peics->rr, srngmod, 24);
            /* Disable old black/white reversal convention */
            if ((bb|gg|rr) == 0)
               strcpy(pcolor, "Z010101");
            else {
               pcolor[0] = 'Z';
               pcolor[1] = hextab[bb >> 4];
               pcolor[2] = hextab[bb & 15];
               pcolor[3] = hextab[gg >> 4];
               pcolor[4] = hextab[gg & 15];
               pcolor[5] = hextab[rr >> 4];
               pcolor[6] = hextab[rr & 15];
               pcolor[7] = '\0';
               }
            goto UsePColor;
            } /* End else use scaled color */
GenSeqColor:
         /* One-sided excitatory sequence */
         if (si >= il->Ct.cpcap)    ic = SEQNUM-1;
         else if (si <= il->Ct.cpt) ic = 0;
         else                       ic =
            mrssl(si-(si32)il->Ct.cpt, srngseq, 24-SEQN_EXP);
         pcolor = RP->colors[CI_SEQ + ic];
UsePColor: pencol(pcolor);

         /*===================================*/
         /*  Set plot shape and perform plot  */
         /*===================================*/
         switch (ksiop) {
         case SIS_None:
         case SIS_Circle:
            circle(xc,yc,r,kfill);
            break;
         case SIS_Square: {
            float d = 2.0*r;
            rect(xc-r,yc-r,d,d,kfill); }
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
PlotPolyLn: polyln(kfill, NTRV, trx, try);
            break;
         case SIS_PhNdl: {
            /* d3news makes sure we have phase here */
            float xr,yr;
            int iang = (int)(psi[LSmem] & PHASE_MASK);
            xr = r*(float)binsin[iang+8];
            yr = r*(float)binsin[iang];
            line(xc,yc,xc+xr,yc+yr); }
            break;
         case SIS_OrNdl: {
            float sang = orrang*(float)(iac % il->nel);
            float xr = r*cosf(sang), yr = r*sinf(sang);
            line(xc-xr,yc-yr,xc+xr,yc+yr); }
            break;
            } /* End ksiop switch */
         } /* End else not dobars */
      } /* End si > threshold */
   else
      r = rcpt;      /* For DPRNT marking if no bubble */

   /* Plot little blue 'V' if cell was probed */
SkipThisSi:
   for (pprb=pprb0; pprb; pprb=pprb->pnctp) {
      if (pprb->prbopt & PRBMARK &&
            ilstnow(&pprb->prbit) == (long)iac) {
         ilstiter(&pprb->prbit);
         pencol("BLUE");
         line(xc,yc,xc+pszsin,yc+pszcos);
         line(xc,yc,xc+pszcos,yc+pszsin);
         } /* End if probed */
      } /* End PRBDEF loop */

   /* Plot little red '+' mark if cell selected for DPRNT */
   if (dockdlplus & detlisted) {
      pencol("RED");
      line(xc-r,yc,xc+r,yc);
      line(xc,yc-r,xc,yc+r);
      } /* End if detlisted */
   } /* End BUBBLE or SQUARE plot */

/*---------------------------------------------------------------------*
*                 End three levels indent suppression                  *
*---------------------------------------------------------------------*/

         } /* End cell loop */

      stattmr(OP_POP_TMR, GRPH_TMR);
      } /* End if KRPNZ and mcells != 0 */
#endif   /* !PAR0 */

#ifndef PARn
/* If KRPAS is on, plot asmul values for this cell type (if it will
*  fit in the available height--we are not going to worry about width
*  for what is basically a debug plot).  This plot has to be done in
*  d3lplt(), not the more logical d3rplt(), because the new scales are
*  not yet available at the time d3rplt() is called from d3go().  */

   if (il->Ct.kctp & KRPAS && il->pauts) {
      if (ir->Rp.krp & (KRPPL|KRPBP)) {
         si32 *pscl = il->pauts->asmul;
         float lettht = RP->stdlht;
         float xs,ys;
         int   iscl;
         if (RP->kplot & (PLSUP|PLVTS))
            xs = ir->aplx, ys = ir->aply;
         else
            xs = ys = 0.5;
         ys -= (float)(il->lnir+1)*lettht;
         if (ys >= 0) for (iscl=0; iscl<il->nauts; ++iscl) {
            number(xs, ys, 0.7*lettht, (double)pscl[iscl]/dS24, 0, 3);
            xs += 5.6*lettht;
            }
         }
      } /* End autoscale value plot */
#endif

   } /* End d3lplt() */
