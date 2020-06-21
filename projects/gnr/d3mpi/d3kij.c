/* (c) Copyright 1991-2018, The Rockefeller University *11115* */
/* $Id: d3kij.c 75 2017-10-13 19:38:52Z  $ */
/***********************************************************************
*                             CNS Program                              *
*            d3kij - Connection Generation Setup Routines              *
*                                                                      *
*  Note:  These routines perform setup for the d3cij, d3dij, d3lij,    *
*  d3sj family of routines, which are designed to make Lij, Cij, etc.  *
*  accessible at any point in CNS without regard to whether or not     *
*  OPTIMIZE STORAGE is in effect.  This should make CNS easier to      *
*  understand and maintain, at a cost of some inner loop calls and     *
*  redundant bits of arithmetic.                                       *
*                                                                      *
*  None of these routines should be called on a PAR0 node--this file   *
*  should not even be compiled on a PAR0 node, hence no #ifdefs.       *
*                                                                      *
*----------------------------------------------------------------------*
*                                                                      *
*  void d3kij(struct CELLTYPE *il, int kcall)                          *
*                                                                      *
*  This routine must be called for each celltype whenever the mode     *
*     of connection generation (kcall argument) changes.  It is        *
*     called once per cell type because in the case of regeneration,   *
*     not all cell types are regenerated.  This call is in addition    *
*     to the once-only setup in d3genr().                              *
*                                                                      *
*  Argument kcall is one of the following modes (defined in lijarg.h): *
*     KMEM_GO      0    Running, pick up or generate per OPTMEM        *
*     KMEM_GENR    1    First call, generate all Lij etc. values       *
*     KMEM_REGEN   2    Regenerating, compute unless external          *
*  The value of kcall is stored in Lija, where it may be accessed by   *
*  other routines, so it must always be kept up-to-date.  This is      *
*  assured by resetting to KMEM_GO mode in d3nset() at start of each   *
*  trial series and again after processing a cell type in d3regenr().  *
*                                                                      *
*  This call leaves appropriate function pointers in ix->lijbr[123],   *
*     ix->cijbr, ix->dijbr, and ix->sjbr so connection generating/     *
*     lookup routines can be called as needed later.  The lijbr1       *
*     routine generates the first connection to every cell and the     *
*     lijbr2 routine generates all other connections.  If subarbors    *
*     are being generated, lijbr2 points to the subarbor routine and   *
*     lijbr3 points to the secondary generating routine.               *
*  These function pointers get rid of inner-loop switches.  Because    *
*     these pointers are destroyed by memory broadcasts, d3kij()       *
*     is called from d3nset to reset them after each such broadcast.   *
*                                                                      *
*----------------------------------------------------------------------*
*                                                                      *
*  void d3kiji(struct CELLTYPE *il, ui32 icell)                        *
*                                                                      *
*  This routine must be called when a new cell is entered in order     *
*     to set Lij, Cij, etc. generation for a particular cell type      *
*     and cell number.                                                 *
*  Arguments:                                                          *
*     il          Ptr to CELLTYPE block for current cell type          *
*     icell       Cell number                                          *
*  Prerequisite is that d3kij() must have been called once at the      *
*     start of initial generation, network simulation, or regener-     *
*     ation to indicate the type of generation currently required.     *
*                                                                      *
*----------------------------------------------------------------------*
*                                                                      *
*  void d3kijf(struct CELLTYPE *il, struct CONNTYPE *ix, ui32 icell)   *
*                                                                      *
*  This is a rump version of d3kiji that is used in d3ijpl (and maybe  *
*  elsewhere) to set up everything that is needed just to call the     *
*  lijbr1 routine to find the first Lij for a cell and connection      *
*  type where needed for a CMBOX plot with kgen=B,K, or Q.             *
*                                                                      *
*  Arguments:                                                          *
*     il          Ptr to CELLTYPE block for current cell type          *
*     ix          Ptr to CONNTYPE block for conn type being plotted    *
*     icell       Cell number                                          *
*  Prerequisite is that d3kij() must have been called once at the      *
*     start of initial generation, network simulation, or regener-     *
*     ation to indicate the type of generation currently required.     *
*                                                                      *
*----------------------------------------------------------------------*
*                                                                      *
*  void d3kijx(struct CONNTYPE *ix)                                    *
*                                                                      *
*  This routine must be called when a new connection type is entered   *
*     and before any of d3lijx(), d3cijx(), etc.  It sets up counters  *
*     that are used to be sure the routine for each variable is used   *
*     only once per connection.                                        *
*  Argument:                                                           *
*     ix          Ptr to CONNTYPE block for current connection type    *
*  Prerequisites are that d3kij() and d3kiji() must already have been  *
*     called.                                                          *
*                                                                      *
************************************************************************
*  V8D, 09/28/06, GNR - Broken out from d3lij, Cij,Dij,Sj setup added  *
*  ==>, 12/28/07, GNR - Last mod before committing to svn repository   *
*  Rev, 06/03/08, GNR - Add color opponency, subarbor options L and 3  *
*  V8F, 02/15/10, GNR - Remove conditionals on TV inputs, add KGNE2    *
*  V8H, 10/20/10, GNR - Add xn (once per conntype) variable allocation *
*  Rev, 04/29/11, GNR - Add KGNYO                                      *
*  R65, 01/02/16, GNR - n[gq][xyg] to ui16, ngrp to ui32, ncpg to ubig *
*  R67, 11/20/16, GNR - Bug Fix:  cwrkoff carries for big icell (PARn) *
*  R72, 02/07/17, GNR - Add KGNKN, d3kijf, remove KGNDG                *
*  R73, 04/18/17, GNR - Convert KGNAN to circular or ellipsoidal areas *
*  R74, 06/19/17, GNR - Add routines for 48-bit color, steerable filts *
*  Rev, 07/14/17, GNR - Add KGNLM LOPTW Cij generating method          *
*  Rev, 08/17/17, GNR - Revise KGNYO setup to do nuy incr on new row   *
*  Rev, 08/29/17, GNR - Add KGNG1 rule (first cell in same group)      *
*  R75, 09/24/17, GNR - Add NOPVP Sj method (pix from highest angle)   *
*  R78, 03/15/18, GNR - Allow any KGEN 1st conn code with KGNKN kernel *
*  R78, 04/23/18, GNR - Image & preproc gray-from-color now in d3imgt  *
*                       remove C16 access and C8 except from IA        *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "d3global.h"
#include "lijarg.h"
#include "wdwdef.h"
#include "rkarith.h"

/*** #define MTXDBG ***/

typedef int (*lijfn)(struct CONNTYPE *ix);
typedef int (*sjfn)(struct CONNTYPE *ix);
struct LIJARG Lija;

/*---------------------------------------------------------------------*
*               Prototypes for Lij generating routines                 *
*---------------------------------------------------------------------*/

/* Partitioned connection types */
int lb1fp(struct CONNTYPE *ix);
int lb1fpia(struct CONNTYPE *ix);
int lb1fpvg(struct CONNTYPE *ix);
int lb1gp(struct CONNTYPE *ix);
int lb1hp(struct CONNTYPE *ix);
int lb1jp(struct CONNTYPE *ix);
int lb1op(struct CONNTYPE *ix);
int lb1up(struct CONNTYPE *ix);
int lb1upia(struct CONNTYPE *ix);
int lb1upvg(struct CONNTYPE *ix);

/* Normal connection types--first connection */
int lb1f(struct CONNTYPE *ix);
int lb1fia(struct CONNTYPE *ix);
int lb1fvg(struct CONNTYPE *ix);
int lb1g(struct CONNTYPE *ix);
int lb1g1(struct CONNTYPE *ix);
int lb1h(struct CONNTYPE *ix);
int lb1j(struct CONNTYPE *ix);
int lb1n(struct CONNTYPE *ix);
int lb1o(struct CONNTYPE *ix);
int lb1t(struct CONNTYPE *ix);
int lb1tia(struct CONNTYPE *ix);
int lb1tvg(struct CONNTYPE *ix);
int lb1u(struct CONNTYPE *ix);
int lb1uia(struct CONNTYPE *ix);
int lb1uvg(struct CONNTYPE *ix);
int lb1x(struct CONNTYPE *ix);
int lb1xia(struct CONNTYPE *ix);

/* Normal connection types--subsequent connections */
int lb2a(struct CONNTYPE *ix);
int lb2aia(struct CONNTYPE *ix);
int lb2b(struct CONNTYPE *ix);
int lb2bia(struct CONNTYPE *ix);
int lbqb(struct CONNTYPE *ix);
int lb2c(struct CONNTYPE *ix);
int lb2cia(struct CONNTYPE *ix);
int lb2cvg(struct CONNTYPE *ix);
int lb2k(struct CONNTYPE *ix);
int lb2kia(struct CONNTYPE *ix);
int lb2kvg(struct CONNTYPE *ix);
int lb2q(struct CONNTYPE *ix);
int lb2qia(struct CONNTYPE *ix);
int lb2qvg(struct CONNTYPE *ix);
int lbsa(struct CONNTYPE *ix);
int lbsaia(struct CONNTYPE *ix);
int lbsai(struct CONNTYPE *ix);
int lbsal(struct CONNTYPE *ix);

/* Special cases */
int d3elij(struct CONNTYPE *ix);
int firstLij(struct CONNTYPE *ix);
int getlij(struct CONNTYPE *ix);
int lijomit(struct CONNTYPE *ix);
int otherLij(struct CONNTYPE *ix);

/*---------------------------------------------------------------------*
*               Prototypes for Cij generating routines                 *
*---------------------------------------------------------------------*/

int gcijg(struct CONNTYPE *ix);  /* Get from storage */
int gcije(struct CONNTYPE *ix);  /* Get from an external file */
int gcijl(struct CONNTYPE *ix);  /* Lambda gradient */
int gcijlw(struct CONNTYPE *ix); /* Lambda with OPT=W */
int gcijm(struct CONNTYPE *ix);  /* FD Matrix */
int gcijml(struct CONNTYPE *ix); /* FD Matrix with OPT=L */
int gcijr(struct CONNTYPE *ix);  /* Random */

/*---------------------------------------------------------------------*
*               Prototypes for Dij generating routines                 *
*---------------------------------------------------------------------*/

ui32 gdijg(struct CONNTYPE *ix); /* Get from storage */
ui32 gdijc(struct CONNTYPE *ix); /* Constant */
ui32 gdijr(struct CONNTYPE *ix); /* Uniform random */
ui32 gdijn(struct CONNTYPE *ix); /* Normal random */
ui32 gdiju(struct CONNTYPE *ix); /* User-function */

/*---------------------------------------------------------------------*
*                  Prototypes for Sj lookup routines                   *
*---------------------------------------------------------------------*/

int gsjiaggs(struct CONNTYPE *ix);  /* Input array -- Gray from GS */
int gsjiag08(struct CONNTYPE *ix);  /* Input array -- Gray from C8 */
int gsjiac08(struct CONNTYPE *ix);  /* Input array -- Color from C8 */
int gsjiao08(struct CONNTYPE *ix);  /* Input array -- Color opponent */
int gsjsfg16(struct CONNTYPE *ix);  /* PP_STGH -- Gray from GS16,C48 */
int gsjsfc48(struct CONNTYPE *ix);  /* PP_STGH -- Color from C48 */
int gsjsfo48(struct CONNTYPE *ix);  /* PP_STGH -- OpCol from C48 */
int gsjvpg16(struct CONNTYPE *ix);  /* PP_GETP -- NOPVP from GS16 */
int gsjppg16(struct CONNTYPE *ix);  /* PREPROC -- Gray from GS16,C48 */
int gsjppc48(struct CONNTYPE *ix);  /* PREPROC -- Color from C48 */
int gsjppo48(struct CONNTYPE *ix);  /* PREPROC -- OpCol from C48 */
int gsjtvggs(struct CONNTYPE *ix);  /* Cam-08  -- Gray from GS,C24 */
int gsjtvc24(struct CONNTYPE *ix);  /* Cam-08  -- Color from C24 */
int gsjtvo24(struct CONNTYPE *ix);  /* Cam-08  -- OpCol from C24 */
int gsjtvgg16(struct CONNTYPE *ix); /* Cam-16  -- Gray from GS16,C48 */
int gsjtvc48(struct CONNTYPE *ix);  /* Cam-16  -- Color from C48 */
int gsjtvo48(struct CONNTYPE *ix);  /* Cam-16  -- OpCol from C48 */
int gsjhv(struct CONNTYPE *ix);     /* Hand vision */
int gsjrep(struct CONNTYPE *ix);    /* Repertoire */
int gsjup(struct CONNTYPE *ix);     /* Call to user program */
int gsjval(struct CONNTYPE *ix);    /* Value */
int gsjvg(struct CONNTYPE *ix);     /* Virtual groups */
int gsjvgr4(struct CONNTYPE *ix);   /* Float virtual groups */

int gpjc(struct CONNTYPE *ix);      /* Constant phase */
int gpjj(struct CONNTYPE *ix);      /* Input phase */
int gpjr(struct CONNTYPE *ix);      /* Random phase */
int gpju(struct CONNTYPE *ix);      /* Uniform phase */


/*=====================================================================*
*                                d3kij                                 *
*                                                                      *
*  Prepare generation for a particular cell type and generation mode.  *
*  (d3kiji() below must then be called to set for a particular cell.)  *
*=====================================================================*/

void d3kij(struct CELLTYPE *il, int kcall) {

/* Tables of Lij functions selected by lbsrc */

   static lijfn Lfp[4] =
      { (lijfn)lb1fp, (lijfn)lb1fpia, NULL,          (lijfn)lb1fpvg };
   static lijfn Lup[4] =
      { (lijfn)lb1up, (lijfn)lb1upia, NULL,          (lijfn)lb1upvg };
   static lijfn Lf[4]  =
      { (lijfn)lb1f,  (lijfn)lb1fia,  NULL,          (lijfn)lb1fvg  };
   static lijfn Lt[4]  =
      { (lijfn)lb1t,  (lijfn)lb1tia,  (lijfn)lb1tia, (lijfn)lb1tvg  };
   static lijfn Lu[4]  =
      { (lijfn)lb1u,  (lijfn)lb1uia,  NULL,          (lijfn)lb1uvg  };
   static lijfn Lx[4]  =
      { (lijfn)lb1x,  (lijfn)lb1xia,  NULL,          (lijfn)lb1x    };
   static lijfn La[4]  =
      { (lijfn)lb2a,  (lijfn)lb2aia,  NULL,          (lijfn)lb2a    };
   static lijfn Lb[4]  =
      { (lijfn)lb2b,  (lijfn)lb2bia,  (lijfn)lb2bia, (lijfn)lb2b    };
   static lijfn Lbq[4] =
      { (lijfn)lbqb,  (lijfn)lb2bia,  (lijfn)lb2bia, (lijfn)lbqb    };
   static lijfn Lc[4]  =
      { (lijfn)lb2c,  (lijfn)lb2cia,  (lijfn)lb2cia, (lijfn)lb2cvg  };
   static lijfn Lk[4]  =
      { (lijfn)lb2k,  (lijfn)lb2kia,  (lijfn)lb2kia, (lijfn)lb2kvg  };
   static lijfn Lq[4]  =
      { (lijfn)lb2q,  (lijfn)lb2qia,  (lijfn)lb2qia, (lijfn)lb2qvg  };
   static lijfn Ls[4]  =
      { (lijfn)lbsa,  (lijfn)lbsaia,  (lijfn)lbsaia, (lijfn)lbsa    };

/* Table giving type of pixel packing vs BM_xxx tvmode:
*  0 => 8 bits/color, 1 => 16 bits/color, 2 => 233 packed,
*  3 => 555 packed, 4 => Not supported.  */
   static byte tvsm[8] = { 4, 0, 1, 1, 4, 2, 3, 0 };
/* Tables of Sj functions selected by operation for each source type */
   static sjfn STai[4] = {          /* Input array */
      (sjfn)gsjiaggs, (sjfn)gsjiag08, (sjfn)gsjiac08, (sjfn)gsjiao08 };
   static sjfn STsf[4] = {          /* Steerable filter */
      (sjfn)gsjsfg16, (sjfn)gsjsfg16, (sjfn)gsjsfc48, (sjfn)gsjsfo48 };
   static sjfn STpp[4] = {          /* Other preprocessor */
      (sjfn)gsjppg16, (sjfn)gsjppg16, (sjfn)gsjppc48, (sjfn)gsjppo48 };
   static sjfn ST08[4] = {          /* 8-bit/color camera */
      (sjfn)gsjtvggs, (sjfn)gsjtvggs, (sjfn)gsjtvc24, (sjfn)gsjtvo24 };
   static sjfn ST16[4] = {          /* 16-bit/color camera */
      (sjfn)gsjtvgg16,(sjfn)gsjtvgg16,(sjfn)gsjtvc48, (sjfn)gsjtvo48 };

   struct CONNTYPE *ix;

/* Save generating mode in Lija.kcall.  This may be accessed by
*  generating routines for Lij, Cij, etc.  Although independent
*  of cell type, it is set here on each call so caller doesn't
*  need to think about when to set it.  Also initialize values
*  needed to detect whether generating routines are being called
*  for a random cell or just the next cell in sequence.  */

   Lija.kcall = kcall;
   Lija.kl = NULL;
   Lija.kcell = -1;

/* Loop over all connection types of this cell type */

   for (ix=il->pct1; ix; ix=ix->pct) {

      int lbsrc;                             /* Source type */
      int mode = ix->cnxc.tvmode & BM_MASK;  /* Source color mode */
      enum ColorSens etvc = (enum ColorSens)(ix->cnxc.tvcsel & EXMASK);
      enum gsj_e { GfmG, GfmC, CfmC, OfmC } egsj;

/*---------------------------------------------------------------------*
*  Determine whether Cij generation is needed at this time,            *
*  and if so, what rule is to be applied.                              *
*---------------------------------------------------------------------*/

      if (Lija.kcall == KMEM_GO && cnexists(ix, CIJ))
         ix->cijbr = gcijg;         /* Fetch Cij from memory */
      else if (ix->kgen & KGNRC)
         ix->cijbr = gcijr;         /* R option specified */
      else if (ix->kgen & KGNTW)
         ix->cijbr = gcijlw;        /* L option with OPT=W */
      else if (ix->kgen & KGNLM)
         ix->cijbr = gcijl;         /* L option (original) */
      else if (ix->kgen & KGNE2)    /* 2 option specified */
         ix->cijbr = gcije;
      else if (ix->Cn.saopt & SAOPTL)
         ix->cijbr = gcijml;        /* Not R, L, or 2, must be M */
      else
         ix->cijbr = gcijm;         /* Plain M */

/*---------------------------------------------------------------------*
*  Determine whether Dij generation is needed at this time,            *
*  and if so, what rule is to be applied.                              *
*---------------------------------------------------------------------*/

      if (ix->Cn.kdelay <= DLY_CONST)
         ix->dijbr = gdijc;         /* Delay is constant or zero */
      else if (Lija.kcall == KMEM_GO && cnexists(ix, DIJ))
         ix->dijbr = gdijg;         /* Fetch Dij from memory */
      else if (ix->Cn.kdelay == DLY_UNIF)
         ix->dijbr = gdijr;         /* Uniform option specified */
      else if (ix->Cn.kdelay == DLY_NORM)
         ix->dijbr = gdijn;         /* Normal option specified */
      else
         ix->dijbr = gdiju;         /* User option specified */

/*---------------------------------------------------------------------*
*  Set Sj lookup according to type of source.  [This code assumes that *
*  illegal combinations have already been detected and errors given.]  *
*  Note:  The gsjiaxxx routines may involve moving windows on IA, so   *
*  must 1st check Lij in bounds on IA, then add ix->osrc, check again. *
*  At this time, only GS16 input is coded for NOPVP connections--add   *
*  others later if needed (d3tchk gives errors for other cases).       *
*---------------------------------------------------------------------*/

      if (ix->sjup)                       /* Call a user program */
         ix->sjbr = gsjup;
      else if (ix->cnsrctyp == REPSRC)    /* Input from a repertoire, */
         ix->sjbr = gsjrep;
      else if (ix->cnsrctyp == VALSRC)    /* From a value scheme, */
         ix->sjbr = gsjval;
      else if (ix->kgen & KGNHV)          /* From hand vision, */
         ix->sjbr = gsjhv;
      else if (ix->kgfl & KGNVG)          /* From virtual groups */
         ix->sjbr = (ix->cnflgs & CNVKR4) ? gsjvgr4 : gsjvg;
      else {   /* These depend on color selection option */
         if (qGray(mode)) egsj = GfmG;          /* Gray from gray */
         else if (etvc == BandW) egsj = GfmC;   /* Gray from color */
         else if (etvc < Opp) egsj = CfmC;      /* Color from color */
         else egsj = OfmC;                      /* Opponent colors */
         if (ix->kgfl & KGNIA)            /* Input from input array, */
            ix->sjbr = STai[egsj];
         else if (ix->Cn.cnopt & NOPVP)   /* Input by VP method */
            ix->sjbr = gsjvpg16;
         else if (ix->kgfl & KGNSF)       /* From steerable filters, */
            ix->sjbr = STsf[egsj];
         else if (ix->cnsrctyp == PP_SRC) /* Other preprocessor, */
            ix->sjbr = STpp[egsj];
         /* All the cases left are camera/tv inputs */
         else switch (tvsm[mode]) {
         case 0:                          /* 8-bit/color camera, */
            ix->sjbr = ST08[egsj]; break;
         case 1:                          /* 16-bit/color camera, */
            ix->sjbr = ST16[egsj]; break;
         case 4:                          /* Now includes cases 2,3 */
            d3exit("Unsupported bitmap", COLOR_ERR, mode);
            } /* End pixel storage mode switch */
         }  /* End modes that depend on color selection option */

      if (ix->phopt & PHASC)           /* with constant phase, */
         ix->pjbr = gpjc;
      else if (ix->phopt & PHASJ)      /* with input cell phase, */
         ix->pjbr = gpjj;
      else if (ix->phopt & PHASR)      /* with random phase, */
         ix->pjbr = gpjr;
      else                             /* uniform or no phase */
         ix->pjbr = gpju;

/*---------------------------------------------------------------------*
*  Determine whether Lij generation is needed at this time and set up  *
*---------------------------------------------------------------------*/

      /* Must do KGNHV test first, for all kcall */
      if (ix->kgen & KGNHV) {
         ix->lijbr1 = ix->lijbr2 = lijomit;
         ix->cnflgs |= GETLIJ;
         continue;
         }
      if (kcall == KMEM_GO && !(ix->cnflgs & CLIJ)) {
         ix->lijbr1 = ix->lijbr2 = getlij;
         ix->cnflgs |= GETLIJ;
         continue;
         }
      ix->cnflgs &= ~GETLIJ;

/* Categorize source cells feeding this connection type:
*  This code sets value of 'lbsrc' to
*        0     if source is a repertoire,
*        1     if source is IA (known in bounds),
*        2     if source is IA (kgen=S and window movement
*                 possibly out of bounds),
*        3     if source is virtual groups, value, or TV
*
*     Note: At present, no distinction is made between IA and
*     scanned IA except for some cases that are disallowed and
*     call d3exit.  The essential reason for this is that an
*     offset may be added in d3go to Lij's for full IA in order
*     to compensate for nux<nsx, nuy<nsy (osrc).  This implies
*     that boundary checking must be done in d3go whether or not
*     there is a scanned window.  */

      if (ix->kgfl & KGNXR) {
         /* Not a repertoire, check for virtual groups */
         if (ix->kgfl & (KGNVG|KGNTV)) lbsrc = 3;
         else if (ix->kgfl & KGNPB) lbsrc = 2;
         else lbsrc = 1;  /* IA (known in bounds) */
         }
      /* Is a repertoire */
      else lbsrc = 0;

/*---------------------------------------------------------------------*
*           Set branch addresses for first Lij generation.             *
*       (Assumes all invalid cases already rejected by g2conn.)        *
*---------------------------------------------------------------------*/

/* (Note: There is no reason why types U,UP,and X could not be used
*  with moving windows except there is currently no way to combine
*  these with 'S' bit that hooks it to a window. These cases there-
*  fore call d3exit at present.  Types F and J, on the other hand,
*  may generate out-of-bounds first connections, so these could
*  never be used with a window, as addition of the offset might
*  give an offset greater than nsx,nsy which could not be detected
*  as out-of-bounds by d3go.)  */

      if (ix->kgen & KGNPT) {  /* Partitioned? */

/*---------------------------------------------------------------------*
*                             Partitioned                              *
*---------------------------------------------------------------------*/

         if (ix->kgen & KGNFU)
            ix->lijbr1 = Lfp[lbsrc];      /* 'F' (Floating) */
         else if (ix->kgen & KGNUD)
            ix->lijbr1 = Lup[lbsrc];      /* 'U' (Uniform) */
         else if (ix->kgen & KGNGP)
            ix->lijbr1 = lb1gp;           /* 'G' (Group) */
         else if (ix->kgen & KGNHG)
            ix->lijbr1 = lb1hp;           /* 'H' (Hyper) */
         else if (ix->kgen & KGNJN)
            ix->lijbr1 = lb1jp;           /* 'J' (Joint) */
         else if (ix->kgen & KGNOG)
            ix->lijbr1 = lb1op;           /* 'O' (Other) */

         /* If no routine assigned, generate error */
         if (!ix->lijbr1)
            d3exit(fmturlnm(il), LIJGEN_ERR, (int)ix->ict);
         /* For partitioned, second = first */
         ix->lijbr2 = ix->lijbr1;
         continue;  /* Go on to next connection type */
         }

      else {

/*---------------------------------------------------------------------*
*                           Not partitioned                            *
*---------------------------------------------------------------------*/

         if      (ix->kgen & KGNEX)
            ix->lijbr1 = d3elij;          /* 'E' (External) */
         else if (ix->kgen & KGNFU)
            ix->lijbr1 = Lf[lbsrc];       /* 'F' (Floating) */
         else if (ix->kgen & KGNGP)
            ix->lijbr1 = lb1g;            /* 'G' (Group) */
         else if (ix->kgen & KGNG1)
            ix->lijbr1 = lb1g1;           /* '1' (Group cell 0) */
         else if (ix->kgen & KGNHG)
            ix->lijbr1 = lb1h;            /* 'H' (Hyper) */
         else if (ix->kgen & KGNJN)
            ix->lijbr1 = lb1j;            /* 'J' (Joint) */
         else if (ix->kgen & KGNND)
            ix->lijbr1 = lb1n;            /* 'N' (Normal) */
         else if (ix->kgen & KGNOG)
            ix->lijbr1 = lb1o;            /* 'O' (Other) */
         else if (ix->kgen & KGNTP)
            ix->lijbr1 = Lt[lbsrc];       /* 'T' (Topographic) */
         else if (ix->kgen & KGNUD)
            ix->lijbr1 = Lu[lbsrc];       /* 'U' (Uniform) */
         else if (ix->kgen & (KGNXO|KGNYO))
            ix->lijbr1 = Lx[lbsrc];       /* 'X' (Systematic) */

         /* Add future options here */

         /* If no routine assigned, generate error */
         if (!ix->lijbr1)
            d3exit(fmturlnm(il), LIJGEN_ERR, (int)ix->ict);

         }   /* End partition if-else */

/*---------------------------------------------------------------------*
*          Set branch address for subsequent Lij generation            *
*---------------------------------------------------------------------*/

/*    Note that the B, C, and Q codes permit the first connection
*  to be out of bounds.  To determine this information, they require
*  that the x,y coords and remainder (cell number relative to group)
*  be stored in fstx, fsty, and fstr, even if connection is skipped.
*
*     Historically, there were two sets of B, C, and D (now Q)
*  routines--the ordinary lb2 ones, used with lb1 routines that could
*  not generate out of bounds (and did not store the information), and
*  the lbq ones, used with F, S, and T, which could generate coords
*  out of bounds (and did store the information).  Currently, all lb1
*  routines store the information, and the lb2c-lbqc routines have
*  been consolidated.  The lb2b-lbqb routines have been kept separate
*  for complete compatibility with Darwin III.  However, code C may
*  now generate connections which would have been skipped with
*  earlier versions.  These are not thought to have been used with D3.
*
*     Since there is no reasonable course of action for the A routine
*  to follow if the first connection is out of bounds, (at least until
*  generalized boundary conditions are defined), this code is forbidden
*  to be used with F, S, T, or current prelim impl. of V.
*
*     The B, C, K, and Q routines for IA input are the same w/ or w/o
*  scanning, as boundary checking is now done against iawxsz,iawysz,
*  which take size of window into account.  */

      if (ix->kgen & KGNIN)
         ix->lijbr2 = ix->lijbr1;         /* Independent */
      else if (ix->kgen & KGNAJ)
         ix->lijbr2 = La[lbsrc];          /* 'A' (Adjacent) */
      else if (ix->kgen & KGNBL) {        /* 'B' (Box) */
         if (!(ix->kgen & (KGNST + KGNFU + KGNND + KGNTP)))
            ix->lijbr2 = Lb[lbsrc];
         else
            ix->lijbr2 = Lbq[lbsrc];
         }
      else if (ix->kgen & KGNCF)
         ix->lijbr2 = Lc[lbsrc];          /* 'C' (Crow's foot) */
      else if (ix->kgen & KGNKN)
         ix->lijbr2 = Lk[lbsrc];          /* 'K' (Kernel) */
      else if (ix->kgen & KGNAN)
         ix->lijbr2 = Lq[lbsrc];          /* 'Q' (Annulus) */
      /* If no routine assigned, generate error */
      if (!ix->lijbr2)
         d3exit(fmturlnm(il), LIJGEN_ERR, (int)ix->ict);

/* If inputs are divided into subarbors, store the lijbr2 pointer
*  in lijbr3 and put the address of the subarbor routine in lijbr2.  */

      if (ix->Cn.saopt & DOSARB) {
         ix->lijbr3 = ix->lijbr2;
         if (ix->Cn.saopt & SAOPTL)
            ix->lijbr2 = lbsal;
         else if (ix->Cn.saopt & SAOPTI)
            ix->lijbr2 = lbsai;
         else
            ix->lijbr2 = Ls[lbsrc];
         } /* End subarbor setup */

      } /* End loop over ix */

   } /* End d3kij() */


/*=====================================================================*
*                               lijwrap                                *
*                                                                      *
*  Increment and wrap a repertoire or input array coordinate into the  *
*  source array (used with KGNXO or KGNYO conntypes and now handles    *
*  arbitrarily large increments, but not negative increments).         *
*  N.B.  If KGNIA, xoff *= 2*nsy.  Otherwise, yoff *= srcnel*srcngx.   *
*  R67, 11/20/16, GNR - Corrected KGNIA case for documented wraparound *
*=====================================================================*/

static long lijwrap(struct CONNTYPE *ix, long Lij0,
      long xoff, long yoff) {

   if (ix->kgfl & KGNIA) {
      long Lx = (Lij0 + xoff) >> RP->ky1;
      long Ly = (Lij0 & RP->ymask) + yoff;
      do {
         ldiv_t xdiv = ldiv(Lx, (long)RP->nsx);
         ldiv_t ydiv = ldiv(Ly, (long)RP->nsy);
         Lx = xdiv.rem + ydiv.quot;
         Ly = ydiv.rem + xdiv.quot;
         } while (Lx >= RP->nsx || Ly >= RP->nsy);
      return (Lx << RP->ky1) + Ly;
      }
   else
      return (Lij0 + xoff + yoff) % ix->srcnelt;
   } /* End lijwrap() */


/*=====================================================================*
*                               d3kiji                                 *
*                                                                      *
*  Prepare generation for a particular cell type and numbered cell.    *
*  (No longer assumes that generation always starts at cell 0.)        *
*                                                                      *
*  To avoid complicated tests because it is not always clear at new    *
*  cell time which connection-level variables will actually be needed, *
*  this routine prepares for Lij, Cij, Dij, etc. generation whether    *
*  it will be needed or not.                                           *
*=====================================================================*/

void d3kiji(struct CELLTYPE *il, ui32 icell) {

   struct REPBLOCK *ir;       /* Ptr to current region block */
   struct CONNTYPE *ix;       /* Ptr to a connection type block */
   div_t qrm;                 /* Quotient and remainder */

   ir = il->pback;

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*
*                                                                      *
*  Shortcut path taken if just advancing to the next sequential cell   *
*                                                                      *
*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

   if (il == Lija.kl && icell == ++Lija.kcell) {

      /* (Possible overflow would have been caught in d3nset) */
      Lija.lcoff += uw2zd(il->lel);
      Lija.new_group = icell >= Lija.grouptop;

      if (Lija.new_group) {      /* Increment group variables */
         ++Lija.groupn;
         Lija.grouptop += il->nel;
         if (++Lija.groupx >= ir->ngx) {
            Lija.groupx = 0;
            ++Lija.groupy;
            }
         }
      Lija.gcell = icell % (ui32)il->nel;

/* Initializations relating to connection types */

      for (ix=il->pct1; ix; ix=ix->pct) {

/*---------------------------------------------------------------------*
*                              Lij setup                               *
*                                                                      *
*  If neither generating, nor regenerating, nor recomputing Lij,       *
*  skip this connection type, but not just if it is bypassed,          *
*  because bypassing can be turned off later.                          *
*---------------------------------------------------------------------*/

         if (Lija.kcall == KMEM_GO && !(ix->cnflgs & CLIJ))
            goto SkipLijNextCell;

         /* This code is here, rather than in d3lijx(), to prevent
         *  double execution first time through on a new cell  */
         if (Lija.new_group) {      /* NEW GROUP */
            /* Items for ul1 codes */
            udevskip(&ix->clseed, ix->uplij);
            if (ix->kgen & (KGNJN|KGNOG|KGNGP|KGNG1|KGNND)) {
               ix->ul1.g.wgrpad = ix->ul1.g.wgrptp;
               ix->ul1.g.wgrptp += ix->ul1.g.srcnel1; }
            else if (ix->kgen & KGNHG) ix->ul1.g.wgrpad =
               (Lija.groupx/ir->Rp.ngridx)*(long)ir->Rp.ngridx*
               ix->srcnel + (Lija.groupy/ir->Rp.ngridy)*
               (long)ir->Rp.ngridy*ix->ul1.g.srcnel1;
            else if (ix->kgen & KGNTP)
               ix->ul1.s.fstnew = TRUE;
            else if (ix->kgen & KGNYO)
               ix->ul1.x.gwrkoff = ix->ul1.x.cwrkoff = Lija.groupx ?
                  lijwrap(ix, ix->ul1.x.gwrkoff, ix->ul1.x.grpincx, 0) :
                  lijwrap(ix, 0, ix->ul1.x.fstoffx,
                     Lija.groupy*ix->ul1.x.grpincy + ix->ul1.x.fstoffy);
            } /* End new-group */

         else {                     /* NOT NEW GROUP */
            /* Advance Lij seed except in special STM11 case */
            if (!(ix->cnflgs & STM11))
               udevskip(&ix->clseed, ix->uplij);
            } /* End not-a-new-group */

         /* Set for kernel depending on position in group */
         if (ix->kgen & KGNKN) ix->ul2.k.rkern =
            ix->ul2.k.ptori[Lija.gcell];

SkipLijNextCell:

/*---------------------------------------------------------------------*
*                              Cij setup                               *
*---------------------------------------------------------------------*/

         if (Lija.kcall == KMEM_GO && cnexists(ix, CIJ))
            goto SkipCijNextCell;

         if (ix->kgen & (KGNRC|KGNLM)) {              /* Type R or L */
            /* Advance working Cij seed to next cell */
            udevskip(&ix->wcseed,
               (ix->Cn.saopt & SAOPTC) ? ix->nsa : ix->nc);
            if (ix->kgen & KGNLM) {                   /* Type L */
               /* Advance initial angle of gradient */
               if (ix->ucij.l2.flags & LOPNC ||
                  (ix->ucij.l2.flags & LOPNG && Lija.new_group))
                  ix->ucij.l2.curang += ix->ucij.l2.clinc;
               } /* End type L */
            } /* End type R or L */
         else if (ix->kgen & KGNMC) {                 /* Type M */
#ifdef MTXDBG
         if (il->seqvmo == 2)
            dbgprt(ssprintf(NULL, "d3kiji incr for %4s, cell %d, ict "
               "%d, mtxel = %d", il->lname, icell, (int)ix->ict,
               (ix->ucij.m.fstmptr - ix->ucij.m.fdptr)));
#endif
            if ((ix->ucij.m.fstmptr += ix->ucij.m.fdincr) >=
                  ix->ucij.m.fdtop)
               ix->ucij.m.fstmptr = ix->ucij.m.fdptr;
#ifdef MTXDBG
         if (il->seqvmo == 2)
            dbgprt(ssprintf(NULL, "   mtxel set to %d",
               (ix->ucij.m.fstmptr - ix->ucij.m.fdptr)));
#endif
            }
         /* Add else {Type 2 setup} here if needed */
SkipCijNextCell:

/*---------------------------------------------------------------------*
*                              Dij setup                               *
*                                                                      *
*  (Unlike the case with Cij, where lwcseed is needed for cloning in   *
*  a new subarbor, here the generation of Dij leaves the wdseed ready  *
*  for the next cell, so no udevskip is needed.)                       *
*---------------------------------------------------------------------*/

         ;

         } /* End conntype loop */

      } /* End setting next sequential cell */

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*
*                                                                      *
*               Normal path for new cell type and cell                 *
*                                                                      *
*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

   else {

/* Prepare variables needed to keep track of current cell group */

      Lija.kl = il;
      Lija.kcell = icell;
      Lija.gcell = icell % (ui32)il->nel;
      Lija.lcoff = uw2zd(jmuwj(il->lel, (icell - il->locell)));
      Lija.groupn = icell/il->nel;
      Lija.grouptop = (Lija.groupn + 1)*il->nel;
      qrm = div((int)Lija.groupn, (int)ir->ngx);
      Lija.groupx = (ui16)qrm.rem;
      Lija.groupy = (ui16)qrm.quot;
      Lija.new_group = TRUE;

/* Initializations relating to connection types.
*  Note:  All results of these initializations must be stored
*     in CONNTYPE blocks, because user code will loop over all
*     connections for all connection types before calling for
*     a new cell.
*/

      for (ix=il->pct1; ix; ix=ix->pct) {

/*---------------------------------------------------------------------*
*                              Lij setup                               *
*                                                                      *
*  If neither generating, nor regenerating, nor recomputing Lij,       *
*  skip this connection type, but not just if it is bypassed,          *
*  because bypassing can be turned off later.                          *
*---------------------------------------------------------------------*/

         if (Lija.kcall == KMEM_GO && !(ix->cnflgs & CLIJ))
            goto SkipLijNewCell;

/* Advance random number-generating seeds as required */

         ix->clseed = ix->lseed;
         if (ix->cnflgs & STM11) {
            if (Lija.groupn > 0)
               udevwskp(&ix->clseed, jmsw(ix->uplij,Lija.groupn));
            }
         else if (icell > 0)
            udevwskp(&ix->clseed, jmsw(ix->uplij,icell));

/* Store addresses needed to locate connections.
*  This code replaced with identical copy of that in d3genr(),
*     07/31/94, GNR.  This corrects serial GJNOU bug.
*  This code moved to d3kiji(), 07/01/06, GNR.  This assures
*     that all uses will see this same code.  Better design.
*  R67, 11/20/16, GNR - Correctly handle KGNXO and KGNYO carries
*     from x->y and y->x for IA input.
*  R78, 03/17/18, GNR - Include x,y offsets in wgrpad for KGNG1.  */

         /* Variables used with Lij generating options G,1,J,N,O */
         if (ix->kgen & (KGNJN|KGNOG|KGNGP|KGNG1|KGNND)) {
            ix->ul1.g.wgrpad = Lija.groupn*ix->ul1.g.srcnel1;
            /* Adjust for (KGNG1|KGNJN) origin at ogoff */
            if (ix->kgen & (KGNG1|KGNJN))
               ix->ul1.g.wgrpad += ix->ul1.g.ogoff;
            ix->ul1.g.wgrptp = ix->ul1.g.wgrpad + ix->ul1.g.srcnel1;
            } /* End G,1,J,N,O */

         /* Variables used with Lij generating option H */
         else if (ix->kgen & KGNHG) ix->ul1.g.wgrpad =
            (Lija.groupx/ir->Rp.ngridx)*(long)ir->Rp.ngridx*
            ix->srcnel + (Lija.groupy/ir->Rp.ngridy)*
            (long)ir->Rp.ngridy*ix->ul1.g.srcnel1;

         /* Variable used with Lij generating options S,T */
         else if (ix->kgen & KGNTP)
            ix->ul1.s.fstnew = TRUE;

         /* Variable used with Lij generating option X */
         else if (ix->kgen & KGNXO) {
            long ncmul = (ix->kgen & KGNIN) ?
               (long)icell*(long)ix->nc : (long)icell;
            ix->ul1.x.cwrkoff = lijwrap(ix, 0,
               ncmul*ix->ul1.x.incoffx + ix->ul1.x.fstoffx,
               ncmul*ix->ul1.x.incoffy + ix->ul1.x.fstoffy);
            }
         /* Variables used with Lij generating option Y */
         else if (ix->kgen & KGNYO) {
            long ncmul = (long)(icell%il->nel);
            if (ix->kgen & KGNIN) ncmul *= (long)ix->nc;
            ix->ul1.x.gwrkoff = lijwrap(ix, 0,
               Lija.groupx*ix->ul1.x.grpincx + ix->ul1.x.fstoffx,
               Lija.groupy*ix->ul1.x.grpincy + ix->ul1.x.fstoffy);
            ix->ul1.x.cwrkoff = lijwrap(ix, ix->ul1.x.gwrkoff,
               ix->ul1.x.incoffx*ncmul, ix->ul1.x.incoffy*ncmul);
            }

         /* Set for kernel depending on position in group */
         if (ix->kgen & KGNKN) ix->ul2.k.rkern =
            ix->ul2.k.ptori[Lija.gcell];

SkipLijNewCell:

/*---------------------------------------------------------------------*
*                              Cij setup                               *
*---------------------------------------------------------------------*/

         if (Lija.kcall == KMEM_GO && cnexists(ix, CIJ))
            goto SkipCijNewCell;

         /* This could go in d3genr but we would then need a test
         *  here to leave the lw numbers undisturbed... */
         if (ix->kgen & (KGNRC|KGNLM)) {              /* Type R or L */
            /* Set working Cij seed for current cell */
            ix->wcseed = ix->cseed;
            if (icell) udevwskp(&ix->wcseed, jmsw(
               icell,(ix->Cn.saopt & SAOPTC) ? ix->nsa : ix->nc));
            if (ix->kgen & KGNLM) {                   /* Type L */
               /* Set initial angle of gradient.  The initial angle
               *  depends on icell, a fact that was not appreciated
               *  until this bug fix on 07/27/92.  */
               ix->ucij.l2.curang = (-ix->ucij.l2.flags) & 3;
               if ((ix->ucij.l2.flags & LOPMF) || (icell == 0)) ;
               else if (ix->ucij.l2.flags & LOPNG)
                  ix->ucij.l2.curang += Lija.groupn*ix->ucij.l2.clinc;
               else if (ix->ucij.l2.flags & LOPNC)
                  ix->ucij.l2.curang += icell*ix->ucij.l2.clinc;
               } /* End type L */
            } /* End type R or L */
         else if (ix->kgen & KGNMC) {                 /* Type M */
#ifdef MTXDBG
         if (il->seqvmo == 2)
            dbgprt(ssprintf(NULL, "d3 kiji new for %4s, cell %d, ict "
               "%d, mtxel = %d", il->lname, icell, (int)ix->ict,
               (ix->ucij.m.fstmptr - ix->ucij.m.fdptr)));
#endif
            /* Bug fix, 08/18/06:  Divide nkt by nks, so fstmptr
            *  cannot go outside the range of the fdptr array */
            ix->ucij.m.fstmptr = ix->ucij.m.fdptr + ix->ucij.m.fdincr*
               (Lija.kcell%(ix->ucij.m.nkt/ix->ucij.m.nks));
#ifdef MTXDBG
         if (il->seqvmo == 2)
            dbgprt(ssprintf(NULL, "   mtxel set to %d",
               (ix->ucij.m.fstmptr - ix->ucij.m.fdptr)));
#endif
            }
         /* Add else { Type 2 setup } here if needed */
SkipCijNewCell:

/*---------------------------------------------------------------------*
*                              Dij setup                               *
*---------------------------------------------------------------------*/


         if (Lija.kcall == KMEM_GO && cnexists(ix, DIJ))
            goto SkipDijNewCell;

         if (ix->Cn.kdelay > DLY_CONST) {
            /* Set delay seed for current cell */
            ix->wdseed = ix->Cn.dseed;
            if (icell) udevwskp(&ix->wdseed, jmsw(icell,ix->nc));
            } /* End delay setup */
SkipDijNewCell:

         ;

         } /* End conntype loop */

      } /* End setting new cell type and cell */

   } /* End d3kiji() */


/*=====================================================================*
*                               d3kijf                                 *
*                                                                      *
*  Prepare generation of just the first Lij for a particular cell      *
*  type, connection type with kgen=C,K, or Q, and numbered cell.       *
*                                                                      *
*  There is no "shortcut" path as in d3kiji because there is so little *
*  to do either way.  Also, this code will be used whether Lij are in  *
*  memory (where it is needed because first Lij may be out-of-bounds), *
*  or not (to save an 'if' inside the isyn loop to capture first Lij.  *
*  Also includes needed portions of d3kijx and d3lijx.                 *
*=====================================================================*/

void d3kijf(struct CELLTYPE *il, struct CONNTYPE *ix, ui32 icell) {

   struct REPBLOCK *ir;       /* Ptr to current region block */
   div_t qrm;                 /* Quotient and remainder */

   ir = il->pback;

/* Prepare variables needed to keep track of current cell group */

   Lija.kl = il;
   Lija.kcell = -2;           /* Force kiji to use long path */
   Lija.gcell = icell % (ui32)il->nel;
   Lija.groupn = icell/il->nel;
   Lija.grouptop = (Lija.groupn + 1)*il->nel;
   qrm = div((int)Lija.groupn, (int)ir->ngx);
   Lija.groupx = (ui16)qrm.rem;
   Lija.groupy = (ui16)qrm.quot;
   Lija.isas = Lija.jsyn = 0;
   Lija.satest = -1;

/* Advance random number-generating seeds as required */

   Lija.wlseed = ix->lseed;
   if (ix->cnflgs & STM11) {
      if (Lija.groupn > 0)
         udevwskp(&Lija.wlseed, jmsw(ix->uplij,Lija.groupn));
      }
   else if (icell > 0)
      udevwskp(&Lija.wlseed, jmsw(ix->uplij,icell));

/* Store addresses needed to locate connections */

   /* Variables used with Lij generating options G,1,J,N,O */
   if (ix->kgen & (KGNJN|KGNOG|KGNGP|KGNG1|KGNND)) {
      ix->ul1.g.wgrpad = Lija.groupn*ix->ul1.g.srcnel1;
      /* Adjust for (KGNG1|KGNJN) origin at ogoff */
      if (ix->kgen & (KGNG1|KGNJN))
         ix->ul1.g.wgrpad += ix->ul1.g.ogoff;
      } /* End G,1,J,N,O */

   /* Variables used with Lij generating option H */
   else if (ix->kgen & KGNHG) ix->ul1.g.wgrpad =
      (Lija.groupx/ir->Rp.ngridx)*(long)ir->Rp.ngridx*
      ix->srcnel + (Lija.groupy/ir->Rp.ngridy)*
      (long)ir->Rp.ngridy*ix->ul1.g.srcnel1;

   /* Variable used with Lij generating options S,T */
   else if (ix->kgen & KGNTP)
      ix->ul1.s.fstnew = TRUE;

   /* Variable used with Lij generating option X */
   else if (ix->kgen & KGNXO) {
      long ncmul = (ix->kgen & KGNIN) ?
         (long)icell*(long)ix->nc : (long)icell;
      ix->ul1.x.cwrkoff = lijwrap(ix, 0,
         ncmul*ix->ul1.x.incoffx + ix->ul1.x.fstoffx,
         ncmul*ix->ul1.x.incoffy + ix->ul1.x.fstoffy);
      }

   /* Variables used with Lij generating option Y */
   else if (ix->kgen & KGNYO) {
      long ncmul = (long)(icell%il->nel);
      if (ix->kgen & KGNIN) ncmul *= (long)ix->nc;
      ix->ul1.x.gwrkoff = lijwrap(ix, 0,
         Lija.groupx*ix->ul1.x.grpincx + ix->ul1.x.fstoffx,
         Lija.groupy*ix->ul1.x.grpincy + ix->ul1.x.fstoffy);
      ix->ul1.x.cwrkoff = lijwrap(ix, ix->ul1.x.gwrkoff,
         ix->ul1.x.incoffx*ncmul, ix->ul1.x.incoffy*ncmul);
      }

   /* Variables used with Lij generating option K */
   if (ix->kgen & KGNKN) {
      ix->ul2.k.rkern =
         ix->ul2.k.ptori[Lija.gcell];
      Lija.xcur = 0;
      }

   } /* End d3kijf() */


/*=====================================================================*
*                               d3kijx                                 *
*                                                                      *
*  Common preparation needed by all of d3lijx(), d3cijx(), etc.        *
*  Must be called before any of those routines.                        *
*=====================================================================*/

void d3kijx(struct CONNTYPE *ix) {

   Lija.pxnd  = ix->psyn0 + Lija.lcoff;
   Lija.psyn  = Lija.pxnd + ix->lxn;
   Lija.lijrc = 0;
   Lija.nLij  = -1;              /* Force call to d3lijx() */
   Lija.nDij  = -1;              /* Force call to d3dijx() */

   } /* End d3kijx() */
