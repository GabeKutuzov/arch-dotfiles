/* (c) Copyright 1992-2010, Neurosciences Research Foundation, Inc. */
/* $Id: d3ijpl.c 51 2012-05-24 20:53:36Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                       d3ijpl - Serial version                        *
*                                                                      *
*  Plot Cij, Cij0, Mij, PPF, Rj, Sj, or ESj                            *
*                                                                      *
*  Caution:  For speed, high byte of stored variables is accessed      *
*  without using the standard macros provided in d3memacc for this     *
*  purpose.                                                            *
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
***********************************************************************/

#define CMTYPE  struct IJPLDEF
#define LIJTYPE struct LIJARG

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "sysdef.h"
#include "d3global.h"
#include "ijpldef.h"
#include "clblk.h"
#include "celldata.h"
#include "rocks.h"
#include "rkxtra.h"
#include "plots.h"

extern struct CELLDATA CDAT;
extern struct LIJARG Lija;

/* Common block for passing constant parameters */

static struct {
   struct CONNTYPE *ix;          /* Ptr to target conn type */
   float scdx,scdy;              /* Source cell delta x,y */
   float sgdx,sgdy;              /* Source group delta x,y */
   float szsx,szsy;              /* Size of symbol along x,y */
   float xcl,ych;                /* X cell low, Y cell high */
   long  lcmamn;                 /* Min abs value of plot variable */
   long  lcmmin;                 /* Min value of plot variable */
   long  lsnel;                  /* Source cells/group */
   long  lsncx;                  /* Source cells along x */
   long  lsngx;                  /* Source groups along x */
   long  vard;                   /* Variable difference/color */
   int   doseq;                  /* TRUE if doing color sequence
                                 *  or tvcsel if doing CSELPLOT */
   int   icolor,jcolor;          /* Previous color, current color */
   ui16  lopt;                   /* Local copy of plot options */
   } CM;

/*---------------------------------------------------------------------*
*                              cmpltvar                                *
*                                                                      *
*  This routine plots a symbol for either averaged or unaveraged plots *
*---------------------------------------------------------------------*/

static void cmpltvar(long lij, long var) {

   float xc,yc;                  /* Location of plotted symbol */
   float xs,ys;                  /* X,Y size of symbol */
   int   ivar;                   /* Var reduced to positive range */

/* Confine variable to range or skip if out of range */

   if (labs(var) < CM.lcmamn) return;
   if ((ivar = (int)(var - CM.lcmmin)) < 0) {
      if (CM.lopt & CMNOX) return;
      ivar = 0;
      }
   else if (ivar >= CM.vard) {
      if (CM.lopt & CMNOX && ivar > CM.vard) return;
      ivar = CM.vard-1;    /* So color stays in SEQNUM range */
      }

/* Calculate location of symbol */

   if (CM.lopt & CMSMAP) {                /* Linear source map */
      long iy = lij/CM.lsncx;
      long ix = lij - iy*CM.lsncx;
      xc = CM.xcl + (float)ix*CM.scdx;
      yc = CM.ych + (float)iy*CM.scdy;
      }
   else if (CM.ix->cnsrctyp == REPSRC) {  /* Source is a repertoire */
      long grp  = lij/CM.lsnel;
      long cell = lij - grp*CM.lsnel;
      long igy  = grp/CM.lsngx;
      long igx  = grp - igy*CM.lsngx;
      long icy  = cell/CM.lsncx;
      long icx  = cell - icy*CM.lsncx;
      xc = CM.xcl + (float)igx*CM.sgdx + (float)icx*CM.scdx;
      yc = CM.ych + (float)igy*CM.sgdy + (float)icy*CM.scdy;
      }
   else if (CM.ix->cnsrctyp == IA_SRC) {  /* Source is input array */
      if ((lij += CM.ix->osrc) & RP->xymaskc) return;
      xc = CM.xcl + (float)(lij >> RP->ky1)*CM.scdx;
      yc = CM.ych + (float)(lij & RP->ymask)*CM.scdy;
      }
   else {                                 /* Source is virtual area */
      long iy = lij/CM.lsngx;
      long ix = lij - iy*CM.lsngx;
      xc = CM.xcl + (float)ix*CM.scdx;
      yc = CM.ych + (float)iy*CM.scdy;
      }

/* Choose the color of the symbol */

   /* (Note that PPF is (S4), others are (S6) or (S8).
   *  This is accounted for in setting up the vard constant.)  */

   if (CM.doseq)
#if SEQNUM == 8
      CM.jcolor = CI_SEQ + (ivar<<3)/CM.vard;
#else
      CM.jcolor = CI_SEQ + (ivar*SEQNUM)/CM.vard;
#endif
   else
      CM.jcolor = var < 0 ? CI_INH : CI_EXC;

   if (CM.jcolor != CM.icolor)
      pencol(RP->colors[CM.icolor=CM.jcolor]);

/* Choose the size of the symbol */

   if (CM.lopt & CMVARY) {
      xs = (float)ivar*CM.szsx;
      ys = (float)ivar*CM.szsy;
      }
   else {
      xs = CM.szsx;
      ys = CM.szsy;
      }

/* Now draw a symbol of the requested shape, size, and color */

   if (CM.lopt & CMSIGN) {    /* Draw plus or minus sign */
      /* Start with minus sign in either case */
      line(xc-xs, yc, xc+xs, yc);
      /* If nonnegative, add vertical bar to make plus sign */
      if (var >= 0)
         line(xc, yc-ys, xc, yc+ys);
      } /* End plus/minus */

   else if (CM.lopt & CMCOL)  /* Draw filled box */
      rect(xc-xs, yc-ys, 2.0*xs, 2.0*ys, FILLED);

   else                       /* Draw outline box */
      rect(xc-xs, yc-ys, 2.0*xs, 2.0*ys, THIN);

   } /* End cmpltvar() */


/*---------------------------------------------------------------------*
*                              cmpltcsel                               *
*                                                                      *
*  This routine plots color selectivity for either averaged or         *
*  unaveraged plots on every third call.                               *
*---------------------------------------------------------------------*/

static void cmpltcsel(long lij, long var) {

   float xc,yc;                  /* Location of plotted symbol */
   float xs,ys;                  /* X,Y size of symbol */
   long  bb,gg,rr;               /* Colors modified for opponency */
   int   ic = Lija.jsyn % NColorDims;
   char  cspec[COLMAXCH];        /* Color spec */

   static long v[NColorDims];    /* Storage for 3 Cij per plot */
   /* Table of hex output characters */
   static char hextab[16] = "0123456789ABCDEF";

   /* Starting place in jrgb as fn of tvcsel for excit,inhib color */
   static byte jcpe[NColorSels] = { 0, 0, 2, 1, 1, 0, 2, 2, 1, 0 };
   static byte jcpi[NColorSels] = { 0, 0, 0, 0, 2, 1, 0, 1, 0, 2 };
   /* Permutations of colors for Cij selection below */
   static byte jrgb[2*NColorDims-1] = { 0, 2, 1, 0, 2 };

   v[ic] = var;

   if (ic < (NColorDims-1)) return;

/* Calculate location of symbol */

   if (CM.lopt & CMSMAP) {                /* Linear source map */
      long ll = lij/NColorDims;
      long iy = ll/CM.lsncx;
      long ix = ll - iy*CM.lsncx;
      xc = CM.xcl + (float)ix*CM.scdx;
      yc = CM.ych + (float)iy*CM.scdy;
      }
   else if (CM.ix->cnsrctyp == IA_SRC) {  /* Source is input array */
      if ((lij += CM.ix->osrc) & RP->xymaskc) return;
      xc = CM.xcl + (float)(lij >> RP->ky1)*CM.scdx;
      yc = CM.ych + (float)(lij & RP->ymask)*CM.scdy;
      }
   else {                                 /* Source is virtual area */
      long iy = lij/CM.lsngx;
      long ix = lij - iy*CM.lsngx;
      xc = CM.xcl + (float)ix*CM.scdx;
      yc = CM.ych + (float)iy*CM.scdy;
      }

/* Choose the color of the symbol.  For a CSELPLOT, there are no
*  limits tests and the Cij are passed to us on S8.  The hex con-
*  versions are done by brute force to get the right combination of
*  leading zeros (missing with ssprintf) and no overflow indication
*  (problem with sconvrt or ubcdwt).  Saves a few nsec too.  */

   if (CM.doseq >= RmG) {
      int jce = jcpe[CM.doseq], jci = jcpi[CM.doseq];
      bb = v[jrgb[ jce ]] - v[jrgb[ jci ]];
      gg = v[jrgb[jce+1]] - v[jrgb[jci+1]];
      rr = v[jrgb[jce+2]] - v[jrgb[jci+2]];
      }
   else if (CM.doseq >= Blue) {
      int jce = jcpe[CM.doseq];
      bb = v[jrgb[ jce ]];
      gg = v[jrgb[jce+1]];
      rr = v[jrgb[jce+2]];
      }
   else
      bb = gg = rr = 128;

   if (bb < 0) bb = 0;
   if (gg < 0) gg = 0;
   if (rr < 0) rr = 0;

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
      long w2 = bb*bb + gg*gg + rr*rr;
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

static void cmaddvar(long lij, long var) {

   struct CMVSUMS *pcmv = (struct CMVSUMS *)CDAT.psws;
   pcmv[lij].sumvar += var;
   pcmv[lij].numvar += 1;

   } /* End cmaddvar() */


/*---------------------------------------------------------------------*
*                               d3ijpl                                 *
*---------------------------------------------------------------------*/

void d3ijpl(void) {

   register struct IJPLDEF *pi;  /* Ptr to current IJPLDEF block */

/* Initialization independent of layer being plotted */

   CM.doseq = !strcmp(RP->colors[CI_BUB],"SEQUENCE");

/* Loop over linked list of IJPLDEF blocks */

   for (pi=RP0->pfijpl; pi; pi=pi->pnijpl) {

      void (*AddOrPlot)(long, long); /* Ptrs to sum or plot routine */
      void (*PlotFn)(long, long);
      struct CELLTYPE *il;       /* Ptr to target cell type */
      struct CONNTYPE *ix;       /* Ptr to target conn type */
      struct CMVSUMS  *pcmv;     /* Ptr to averaging sums */
      ilst            *pil;      /* Ptr to iteration list */
      rd_type         *pvar0;    /* Ptr to 1st plot var for cell 0 */
      rd_type         *pvar;     /* Ptr to current plot variable */

      ilst  cmilst;              /* Fake ilst to plot all cells */
      iter  cmiter;              /* Cell list iterator */

      float szmult;              /* Symbol size multipler */

      long  coff;                /* Offset to prd data for this cell */
      long  Lij;                 /* Lij */
      long  Mijtest;             /* Test for Mij a clock */
      long  icell,jcell;         /* Current cell number */
      long  llc,llel;            /* Length of conn, cell data */
      long  ltnel,ltncx,ltngx;   /* For locating target cell */
      long  ncells;              /* Number of cells in plot */
      long  nconns;              /* Number of conns in cell */
      long  nuk;                 /* Number of connections used */
      long  tl;                  /* Temp of various uses */
      long  var;                 /* Variable to be plotted */
      int   limshft;             /* Shift for cmmin, etc. */

/* Omit plot if disabled */

      if (pi->kcmpldis) continue;

/*---------------------------------------------------------------------*
*                             Plot setup                               *
*---------------------------------------------------------------------*/

      il = pi->pcmtl;            /* Ptr to target cell type */
      CM.ix = ix = pi->pcmtx;    /* Ptr to target conn type */
      CM.lopt = pi->cmopt;       /* Option controls */
      llc  = ix->lc;             /* Length of connection data  */
      llel = il->lel;            /* Length of one cell element */

      /* Set current color and retrace mode */
      CM.icolor = -1;
      retrace((CM.lopt & CMRTR | RP->kplot & PLRTR) ?
         RETRACE : NORETRACE);

      /* Generate constants needed to compute x,y from Lij  */
      CM.scdx = pi->scdx, CM.scdy = pi->scdy;
      CM.sgdx = pi->sgdx, CM.sgdy = pi->sgdy;
      CM.lsncx = pi->srcncx;
      if (!(CM.lopt & CMSMAP)) {
         CM.lsnel = ix->srcnel;
         CM.lsngx = ix->srcngx;
         }

      if (pi->kcmplt == KCIJSEL) {
         szmult = 0.375;
         CM.doseq = CM.ix->cnxc.tvcsel & ~EXTR3C;
         CM.szsx =  szmult*pi->scdx;
         CM.szsy = -szmult*pi->scdy;
         PlotFn = cmpltcsel;
         }
      else {
         /* Default limit shift for variables with 1-byte pickup */
         limshft = BITSPERBYTE;
         /* Pick up data locator (high-order byte or first byte if
         *  Mij, relative to first cell) for item being plotted.  */
         switch (pi->kcmplt) {

         case KCIJC:
            limshft = 6;
            break;
         case KCIJC0:
            pvar0 = ix->psyn0 + ix->lxn + ix->cnoff[CIJ0]
#if D3MEMACC_BYTE_ORDER < 0      /* Access high byte only */
                                 + ix->cijlen - 1
#endif
               ;
            break;
         case KCIJM:
            pvar0 = ix->psyn0 + ix->lxn + ix->cnoff[MIJ];
            Mijtest = ix->Cn.mticks - S15;
            limshft = CM_Mij2S8;
            break;
         case KCIJPPF:
            pvar0 = ix->psyn0 + ix->lxn + ix->cnoff[PPF]
#if D3MEMACC_BYTE_ORDER < 0      /* Access high byte only */
                                 + 1
#endif
               ;
            break;
         case KCIJR:
            pvar0 = ix->psyn0 + ix->lxn + ix->cnoff[RBAR]
#if D3MEMACC_BYTE_ORDER < 0      /* Access high byte only */
                                 + 1
#endif
            ;
            break;
         case KCIJES:
            pvar0 = ix->psyn0 + ix->lxn + ix->cnoff[RBAR];
            /* Fall into KCIJS case to set limshft, no break */
         case KCIJS:
            limshft = CM_Sj2S8;
            break;

            } /* End kcmplt switch */

         /* Variable range tests are saved in IJPLDEF on storage scale
         *  for each plot variable, now shift right because only high-
         *  order information is passed to cmpltvar/cmaddvar.  */
         CM.lcmmin = SRA(pi->cmmin, limshft);   /* Min var */
         CM.lcmamn = pi->cmamn >> limshft;      /* Min abs(var) */
         /* Divisor for color range */
         CM.vard = (pi->cmmax - pi->cmmin) >> limshft;

         /* Set parameters for determining symbol size, either con-
         *  stant or as a multiple of value of plot variable (multi-
         *  plier is scaled up to make average symbol bigger).  */
         szmult = (CM.lopt & CMVARY) ? 0.5/(float)CM.vard : 0.375;
         CM.szsx =  szmult*pi->scdx;
         CM.szsy = -szmult*pi->scdy;

         PlotFn = cmpltvar;
         } /* End else not KCIJSEL */

/* Locate user's cell list or create one to select all cells.
*  Initiate iteration over this list.  */

      if (pi->pcmcl) {           /* Have an explicit cell list */
         pil = pi->pcmcl->pclil;
         ncells = ilstitct(pil, il->nelt);
         }
      else {                     /* Plot all cells */
         pil = &cmilst;
         ncells = il->nelt;
         memset((char *)&cmilst, 0, sizeof(cmilst));
         cmilst.nusm1 = (ncells > 1);
         cmilst.pidl = (ilstitem *)&cmilst.frst;
         cmilst.last = IL_REND | (ncells - 1);
         }
      ilstset(&cmiter, pil, il->locell);

/* Set to do plots of single variables or averages over cells */

      if (CM.lopt & CMTAVG) {
         /* Clear arrays for accumulating average values of plot
         *  variable.  In principle, these should be si64, but with
         *  one-byte args longs can accumulate up to 2**24 terms--
         *  should be enough in practice.  */
         pcmv = (struct CMVSUMS  *)CDAT.psws;
         nconns = (CM.lopt & CMSMAP) ? ix->nc : ix->srcnelt;
         tl = nconns*sizeof(struct CMVSUMS);
         memset((char *)pcmv, 0, tl);
         AddOrPlot = cmaddvar;
         }
      else {
         /* Pick up variables needed to locate target cell */
         if (CM.lopt & CMTMAP)
            icell = 0;
         else {
            ltnel = il->nel;
            ltngx = il->pback->ngx;
            ltncx = pi->tgtncx;
            }
         AddOrPlot = PlotFn;
         }

/*---------------------------------------------------------------------*
*                           Loop over cells                            *
*        Either accumulate sums for averaging or plot directly         *
*---------------------------------------------------------------------*/

      while ((jcell = ilstiter(&cmiter)) >= 0) {

         d3kiji(il, jcell);      /* Set Lij retrieval for this */
         d3kijx(ix);             /*   cell and connection type */
         d3lijx(ix);
         coff = Lija.lcoff;
         nuk = Lija.nuk;

         /* Locate center of first plot symbol for this cell */
         if (!(CM.lopt & CMTAVG)) {
            if (CM.lopt & CMTMAP) {
               long iy = icell/pi->tgtncx;
               long ix = icell - iy*pi->tgtncx;
               CM.xcl = pi->xlo + (float)ix*pi->tcdx;
               CM.ych = pi->yhi + (float)iy*pi->tcdy;
               ++icell;
               }
            else {
               long grp  = jcell/ltnel;
               long cell = jcell - grp*ltnel;
               long igy = grp/ltngx;
               long igx = grp - igy*ltngx;
               long icy = cell/ltncx;
               long icx = cell - icy*ltncx;
               CM.xcl = pi->xlo +
                  (float)igx*pi->tgdx + (float)icx*pi->tcdx;
               CM.ych = pi->yhi +
                  (float)igy*pi->tgdy + (float)icy*pi->tcdy;
               }
            }

/* We use a separate connection loop for each type of variable in
*  order to avoid a switch inside the loop.  Even with CMSMAP, we
*  call Lija.lijbr in order to detect skipped connections.  */

         switch (pi->kcmplt) {

         case KCIJC:          /* Cij plot */
         case KCIJSEL:        /* Color sensitivity plot */
            d3cijx(ix);
            for (Lija.isyn=0; Lija.isyn<nuk; ++Lija.isyn) {
               if (!Lija.lijbr(ix)) break;
               Lij = (CM.lopt & CMSMAP) ? Lija.jsyn : Lija.lijval;
               if (ix->cijbr(ix))   /* Cij, S31 --> S8 */
                  AddOrPlot(Lij, SRA(Lija.cijval, CM_Cij2S8));
               Lija.psyn += llc;
               } /* End loop over connections */
            break;

         case KCIJM:          /* Mij plot */
            pvar = pvar0 + coff;
            for (Lija.isyn=0; Lija.isyn<nuk; ++Lija.isyn) {
               if (!Lija.lijbr(ix)) break;
               Lij = (CM.lopt & CMSMAP) ? Lija.jsyn : Lija.lijval;
               /* If Mij is acting as a timer, use mxmij */
               d3gts2(var, pvar);
               if (var <= Mijtest) var = (long)ix->Cn.mxmij;
               AddOrPlot(Lij, SRA(var, CM_Mij2S8));
               pvar += llc;
               Lija.psyn += llc;
               } /* End loop over connections */
            break;

         case KCIJS:          /* Sj plot */
            d3sjx(ix);
            for (Lija.isyn=0; Lija.isyn<nuk; ++Lija.isyn) {
               if (!Lija.lijbr(ix)) break;
               Lij = (CM.lopt & CMSMAP) ? Lija.jsyn : Lija.lijval;
               if (ix->sjbr(ix))
                  AddOrPlot(Lij, SRA(Lija.sjval, CM_Sj2S8));
               Lija.psyn += llc;
               } /* End loop over connections */
            break;

         case KCIJES:         /* Effective Sj plot */
            d3sjx(ix);
            pvar = pvar0 + coff;
            for (Lija.isyn=0; Lija.isyn<nuk; ++Lija.isyn) {
               if (!Lija.lijbr(ix)) break;
               Lij = (CM.lopt & CMSMAP) ? Lija.jsyn : Lija.lijval;
               if (ix->sjbr(ix)) {
                  d3gts2(var, pvar);
                  var = Lija.sjval - var;
                  var = SRA(var, CM_Sj2S8);
                  AddOrPlot(Lij, var);
                  }
               pvar += llc;
               Lija.psyn += llc;
               } /* End loop over connections */
            break;

         default:             /* Cij0, PPF, or Rj plot */
            pvar = pvar0 + coff;
            for (Lija.isyn=0; Lija.isyn<nuk; ++Lija.isyn) {
               if (!Lija.lijbr(ix)) break;
               Lij = (CM.lopt & CMSMAP) ? Lija.jsyn : Lija.lijval;

               /* Construct proper negs, allowing for Cij0 = -0 */
               tl = (long)*pvar;
               var = tl & 0x7fL;
               if (tl > 0x7fL) var -= 0x80L;
               AddOrPlot(Lij, var);

               pvar += llc;
               Lija.psyn += llc;
               } /* End loop over connections */
            } /* End kcmplt switch */

         } /* End loop over cells */

/* If averaging, loop over connections a second time, now plotting
*  the average value of the plot variable for each connection.  */

      if (CM.lopt & CMTAVG) {
         CM.xcl = pi->xlo;
         CM.ych = pi->yhi;
         for (Lija.jsyn=0; Lija.jsyn<nconns; ++Lija.jsyn) {
            if (pcmv[Lija.jsyn].numvar) {
               var = pcmv[Lija.jsyn].sumvar/pcmv[Lija.jsyn].numvar;
               PlotFn(Lija.jsyn, var);
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
      rect(pi->cmpltx, pi->cmplty, pi->cmpltw, pi->cmplth, THIN);

/* Add source grid lines if requested */

      if (CM.lopt & CMSGRD) {
         float xyg,xygi,xyge,xtop;
         retrace(NORETRACE);
         /* Draw x (vertical) grid lines */
         xtop = pi->cmpltx + pi->cmpltw;
         xygi = pi->tcdx;
         xyge = xtop - 0.9*xygi;
         for (xyg=pi->cmpltx+xygi; xyg<xyge; xyg+=xygi)
            line(xyg, pi->cmplty, xyg, pi->cmtopy);
         /* Draw y (horizontal) grid lines */
         xygi = pi->tcdy;
         xyge = pi->cmtopy + 0.9*xygi;
         for (xyg=pi->cmplty-xygi; xyg<xyge; xyg-=xygi)
            line(pi->cmpltx, xyg, xtop, xyg);
         }

/* Add a title unless suppressed */

      if (!(CM.lopt & CMNTTL)) {
         if (pi->cmltc[0]) pencol(pi->cmltc);
         symbol(pi->cmpltx, pi->cmplty-RP->stdlht, 0.7*RP->stdlht,
            getrktxt(pi->cmltxt), 0, pi->cmltln);
         }

      } /* End pi loop */

   } /* End d3ijpl() */

