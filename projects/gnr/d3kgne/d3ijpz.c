/* (c) Copyright 1992-2010, Neurosciences Research Foundation, Inc. */
/* $Id: d3ijpz.c 30 2010-07-09 21:26:56Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                               d3ijpz                                 *
*                                                                      *
*  Plot Cij, Cij0, Mij, PPF, Rj, or Sj -- Host node version.  Gets     *
*  selected variable from comp nodes, performs averaging if requested  *
*  and plotting.  See notes in d3ijpn.c regarding choice to send all   *
*  data to host rather than do partial averaging or plotting on comp   *
*  nodes.  This choice could be changed if appropriate on particular   *
*  equipment.                                                          *
*                                                                      *
*  Note:  Because this routine is built only for parallel CNS,         *
*  explicit preprocessor tests for PAR are not included.               *
************************************************************************
*  V4A, 06/05/89, JWT - Translated into C                              *
*  Rev, 01/17/90, JWT - Parallelized                                   *
*  Rev, 03/28/90, JWT - Added standard plot letter height              *
*  Rev, 05/03/91, GNR - Implement CHANGES structures                   *
*  V5C, 03/22/92, GNR - Break out d3ijpi, use nnget, merge common code *
*  Rev, 05/09/92, GNR - Use new ROCKS plot calls for atomization       *
*  Rev, 05/20/92, ABP - Add stattmr() calls                            *
*  Rev, 12/09/92, ABP - Add byte swapping for hybrid host              *
*  V6C, 08/14/93, GNR - Remove s**n dependency                         *
*  Rev, 05/23/99, GNR - Use new swapping scheme                        *
*  V8B, 11/10/01, GNR - Cell lists, unaveraged plots, OPTZZ, labels    *
*  V8C, 07/08/03, GNR - Cell responses in millivolts, add conductances *
*  V8D, 07/29/06, GNR - Use new d3lij to fetch Lij even if not stored  *
*  Rev, 09/24/07, GNR - Add MABCIJ, etc. options, RJPLOT, [E]SJPLOT    *
*  ==>, 12/24/07, GNR - Last mod before committing to svn repository   *
*  Rev, 06/21/08, GNR - Add CSELPLOT (color selectivity)               *
*  Rev, 01/17/09, GNR - Reorganize colors into a single array          *
*  Rev, 06/28/10, GNR - Move xlo,yhi calc to d3ijpi, fix shrink bug    *
***********************************************************************/

#define CMTYPE struct IJPLDEF
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
*  This routine plots a symbol for either averaged or unaveraged plots.*
*  (It should be identical to the corresponding routine in d3ijpl.c.)  *
*---------------------------------------------------------------------*/

static void cmpltvar(long lij, long var) {

   float xc,yc;                  /* Location of plotted symbol */
   float xs,ys;                  /* X,Y size of symbol */

/* Confine variable to range or skip if out of range */

   if (labs(var) < CM.lcmamn) return;
   if ((var -= CM.lcmmin) < 0) {
      if (CM.lopt & CMNOX) return;
      var = 0;
      }
   else if (var >= CM.vard) {
      if (CM.lopt & CMNOX && var > CM.vard) return;
      var = CM.vard-1;  /* So color stays in SEQNUM range */
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
      CM.jcolor = CI_SEQ + (var<<3)/CM.vard;
#else
      CM.jcolor = CI_SEQ + (var*SEQNUM)/CM.vard;
#endif
   else
      CM.jcolor = var < 0 ? CI_INH : CI_EXC;

   if (CM.jcolor != CM.icolor)
      pencol(RP->colors[CM.icolor=CM.jcolor]);

/* Choose the size of the symbol */

   if (CM.lopt & CMVARY) {
      xs = (float)var*CM.szsx;
      ys = (float)var*CM.szsy;
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
*  (It should be identical to the corresponding routine in d3ijpl.c.)  *
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
*                               d3ijpz                                 *
*---------------------------------------------------------------------*/

void d3ijpz(void) {

   struct NNSTR nnio;            /* Node I/O controls */
   register struct IJPLDEF *pi;  /* Ptr to current IJPLDEF block */
   long EndSignal = CM_END_DATA;

/* Initialization independent of layer being plotted */

   CM.doseq = !strcmp(RP->colors[CI_BUB],"SEQUENCE");
   stattmr(OP_PUSH_TMR, GRPH_TMR);

/* Loop over linked list of IJPLDEF blocks */

   for (pi=RP0->pfijpl; pi; pi=pi->pnijpl) {

      void (*AddOrPlot)(long, long); /* Ptrs to sum or plot routine */
      void (*PlotFn)(long, long);
      struct CELLTYPE *il;       /* Ptr to target cell type */
      struct CONNTYPE *ix;       /* Ptr to target conn type */
      struct CMVSUMS  *pcmv;     /* Ptr to averaging sums */
      ilst            *pil;      /* Ptr to iteration list */

      ilst  cmilst;              /* Fake ilst to plot all cells */
      iter  cmiter;              /* Cell list iterator */

      float szmult;              /* Symbol size multiplier */

      long  Lij;                 /* Lij */
      long  curr_node,next_node; /* Current, next sending node */
      long  icell,jcell;         /* Current cell, conn number */
      long  ltnel,ltncx,ltngx;   /* For locating target cell */
      long  ncells;              /* Number of cells in plot */
      long  nconns;              /* Number of conns in cell */
      long  nuk;                 /* Number of connections used */
      long  tl;                  /* Temp of various uses */
      long  var;                 /* Variable to be plotted */
      int   limshft;             /* Shift for cmmin, etc. */
      int   lkcmplt;             /* kcmplt jiggered for CSELPLOT */

/* Omit plot if disabled */

      if (pi->kcmpldis) continue;

/*---------------------------------------------------------------------*
*                             Plot setup                               *
*---------------------------------------------------------------------*/

      il = pi->pcmtl;            /* Ptr to target cell type */
      CM.ix = ix = pi->pcmtx;    /* Ptr to target conn type */
      CM.lopt = pi->cmopt;       /* Option controls */

      /* Set current color and retrace mode */
      CM.icolor = -1;
      retrace((CM.lopt & CMRTR | RP->kplot & PLRTR) ?
         RETRACE : NORETRACE);

      /* Generate constants needed to compute x,y from Lij */
      CM.scdx = pi->scdx, CM.scdy = pi->scdy;
      CM.sgdx = pi->sgdx, CM.sgdy = pi->sgdy;
      CM.lsncx = pi->srcncx;
      if (!(CM.lopt & CMSMAP)) {
         CM.lsnel = ix->srcnel;
         CM.lsngx = ix->srcngx;
         }

      lkcmplt = pi->kcmplt;
      if (pi->kcmplt == KCIJSEL) {
         if (CM.lopt & CMSMAP) lkcmplt = KCIJC;
         szmult = 0.375;
         CM.doseq = CM.ix->cnxc.tvcsel & ~EXTR3C;
         CM.szsx =  szmult*pi->scdx;
         CM.szsy = -szmult*pi->scdy;
         PlotFn = cmpltcsel;
         }
      else {
         /* Default limit shift for variables with 1-byte pickup */
         limshft = BITSPERBYTE;
         /* Variables with other scales */
         switch (pi->kcmplt) {
         case KCIJM:
            limshft = CM_Mij2S8;
            break;
         case KCIJES:
         case KCIJS:
            limshft = CM_Sj2S8;
         default:
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
*  Initiate iteration over this list.  Set current node to -1
*  to force data stream to open on first cell.  */

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
      ilstset(&cmiter, pil, 0);
      curr_node = -1;

/* Set to do plots of single variables or averages over cells */

      if (CM.lopt & CMTAVG) {
         /* Clear arrays for accumulating average values of plot
         *  variable.  In principle, these should be si64, but with
         *  one-byte args longs can accumulate up to 2**24 terms--
         *  should be enough in practice.  */
         pcmv = (struct CMVSUMS *)CDAT.psws;
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

/* Loop over cells.  When opening stream from a new node,
*  read cell number and check that lists are synchronized.
*  Then read number of connections actually used. */

      while ((jcell = ilstiter(&cmiter)) >= 0) {

         next_node = findcell(il, jcell);
         if (next_node != curr_node) {
            /* Open data stream from next node and
            *  make it the new current node.  */
            nngcr(&nnio, next_node, IJPL_DATA_MSG);
            tl = nngeti4(&nnio);
            if (tl != jcell)
               d3exit(IJPLEND_ERR, fmturlcn(il, tl), 0);
            curr_node = next_node;
            }
         nuk = nngeti4(&nnio);

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

/* Loop over connections until count is exhausted.  A separate loop is
*  needed for CSELPLOT w/o CMSMAP in order to receive both jsyn and
*  lijval from the comp nodes.  */

         if (lkcmplt == KCIJSEL) {     /* CSEL plot w/o CMSMAP */
            long isyn;
            for (isyn=0; isyn<nuk; ++isyn) {
               Lij = nngeti4(&nnio);
               if (Lij == ix->lijlim) break;
               Lija.jsyn = nngeti4(&nnio);
               var = nngeti4(&nnio);
               AddOrPlot(Lij, var);
               } /* End loop over connections */
            }
         else {                        /* All other plots */
            long isyn;
            for (isyn=0; isyn<nuk; ++isyn) {
               Lij = nngeti4(&nnio);
               if (Lij == ix->lijlim) break;
               var = nngeti4(&nnio);
               AddOrPlot(Lij, var);
               } /* End loop over connections */
            }

/* If this is the last cell from its node,
*  close data stream from that node.  */

         tl = ilstnow(&cmiter);
         if (tl < 0 || findcell(il, tl) != curr_node) {
            tl  = nngeti4(&nnio);
            if (tl != EndSignal)
               d3exit(IJPLEND_ERR, fmturlcn(il, tl), 0);
            nngcl(&nnio);
            }

         } /* End loop over cells */

/* If averaginig, loop over connections a second time, now plotting
*  the average value of the plot variable for each connection.  */

      if (CM.lopt & CMTAVG) {
         CM.xcl = pi->xlo;
         CM.ych = pi->yhi;
         for (Lija.jsyn=0; Lija.jsyn<nconns; ++Lija.jsyn) {
            if (pcmv[Lija.jsyn].numvar) {
               var = pcmv[Lija.jsyn].sumvar/pcmv[Lija.jsyn].numvar;
               cmpltvar(Lija.jsyn, var);
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

/* Add a title unless suppressed */

      if (!(CM.lopt & CMNTTL)) {
         if (pi->cmltc[0]) pencol(pi->cmltc);
         symbol(pi->cmpltx, pi->cmplty-RP->stdlht, 0.7*RP->stdlht,
            getrktxt(pi->cmltxt), 0, pi->cmltln);
         }

      } /* End pi loop */

   stattmr(OP_POP_TMR, GRPH_TMR);

   } /* End d3ijpz() */

