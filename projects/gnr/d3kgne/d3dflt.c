/* (c) Copyright 1991-2012, Neurosciences Research Foundation, Inc. */
/* $Id: d3dflt.c 46 2011-05-20 20:14:09Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                              d3dflt.c                                *
*  Includes routines cmdflt, cndflt, cndflt2, ctdflt, d1dflt,          *
*     d3dflt, d3dflt2, effdflt, envdflt, ibdflt, itpdflt, mddflt,      *
*     mdltdflt, opdflt, prbdflt, prepdflt, rpdflt, tvdflt, vbdflt,     *
*     vcdflt                                                           *
*                                                                      *
*  Establish defaults as defined in the writeup                        *
*     (Note: There is a separate entry for each major type of control  *
*  block.  Defaults that occur in this source file are not defined     *
*  with #define--because they occur in just this one place and this    *
*  is the natural place to look for them, there is no point to refer   *
*  the reader to a separate header file.  Defaults for arms, joints,   *
*  and windows are in d3ajw.c  -GNR)                                   *
*                                                                      *
*  Written by G. N. Reeke, 8/22/85                                     *
*  V4A, 07/20/88, Translated to 'C' from version V3E                   *
*  Rev, 06/19/89, EPT set to 32 (0.125 in FS8)                         *
*  Rev, 11/16/89, JMS - Default RP->snfr to 1                          *
*  Rev, 01/15/91,  MC - Add default for phase colors                   *
*  Rev, 04/25/91, GNR - Set null changes and sensors, add chdflt       *
*  Rev, 09/24/91, GNR - Zero RFdef pointers                            *
*  V5C, 11/11/91, GNR - Add Darwin 1 defaults                          *
*  Rev, 04/09/92, ABP - Add Video Disk defaults                        *
*  V5E, 06/12/92, GNR - Add D1 nsmxh (surround max hits) and nsprd     *
*  Rev, 07/25/92, GNR - Move mtj to LCnD (celltype defaults) struct    *
*  Rev, 08/25/92, GNR - Default cpr,cpt in CELLTYPE, D1BLK set from RP *
*  Rev, 07/07/93, GNR - Add vcdflt()                                   *
*  V6D, 02/10/94, GNR - Remove restriction on location of inhbnd,      *
*                       add defaults for spikes, correct siet,siscl    *
*  V7A, 04/23/94, GNR - Add RP0->cumdown, rewrite vcdflt               *
*  Rev, 05/16/94, GNR - Add opdflt()                                   *
*  V7B, 07/19/94, GNR - Add OUTCTL defaults                            *
*  V8A, 04/15/95, GNR - Add vcdflts for VJ,VT,VW, ident with getsnsid, *
*                       add mdltdflt(), tvdflt(), vbdflt(), DFLTDCY,   *
*                       pht, ht to S8, omega1 default to 1.0, remove   *
*                       "LTP"                                          *
*  Rev, 11/27/96, GNR - Remove support for OVERLAY and NCUBE graphics  *
*  Rev, 01/30/97, GNR - Add cedflt, cndflt2, pfdflt, sndflt routines   *
*  Rev, 09/28/97, GNR - Independent dynamic allocations per conntype   *
*  Rev, 12/10/97, GNR - Remove kux,kuy, make default snoise zero       *
*  Rev, 04/02/98, GNR - Change eilo default to 0                       *
*  Rev, 09/21/98, GNR - Automatic determination of ONLINE status       *
*  V8B, 12/10/00, GNR - Move to new memory management routines         *
*  Rev, 09/01/01, GNR - Add prbdflt()                                  *
*  V8C, 02/22/03, GNR - Cell responses in millivolts                   *
*  V8D, 05/04/05, GNR - Conductances and ions, new reset options       *
*  Rev, 03/22/07, GNR - Add itpdflt()                                  *
*  Rev, 10/20/07, GNR - Add TANH response function                     *
*  ==>, 02/28/08, GNR - Last mod before committing to svn repository   *
*  Rev, 03/22/08, GNR - Add MASTER versions of conntype defaults       *
*  Rev, 04/21/08, GNR - Add ssck (sum sign check) mechanism, d3dflt2   *
*  Rev, 12/31/08, GNR - Correct default upsd to S27 (S20 if COMPAT=C)  *
*  V8E, 01/13/09, GNR - Add defaults for Izhikevich neurons            *
*  Rev, 01/17/09, GNR - Reorganize colors into a single array          *
*  Rev, 05/04/09, GNR - Change et,itt,mt COMPAT=C dflt to 0            *
*  Rev, 09/02/09, GNR - Add Brette-Gerstner (aEIF) defaults            *
*  V8F, 02/13/10, GNR - Revise TV defaults for added UPGM type         *
*  Rev, 02/20/10, GNR - Remove cedflt, pfdflt, sndflt, vdrdflt         *
*  Rev, 04/19/10, GNR - Add prepdflt                                   *
*  V8G, 08/12/10, GNR - Add AUTOSCALE                                  *
*  V8H, 10/20/10, GNR - Add xn (once per conntype) variable allocation *
*  Rev, 01/02/11, GNR - Remove chdflt()                                *
*  Rev, 05/25/11, GNR - Add kract defaults, modify rlvl defaults       *
***********************************************************************/

#define PPTYPE  struct PREPROC
#define SDSTYPE struct SDSELECT
#define TVTYPE  struct TVDEF

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "d3global.h"
#include "d3fdef.h"
#include "ijpldef.h"
#include "simdata.h"
#ifdef BBDS
#include "bbd.h"
#endif
#ifdef D1
#include "d1def.h"
#endif
#include "tvdef.h"
#include "rocks.h"
#include "rkxtra.h"


/***********************************************************************
*                                                                      *
*     cmdflt - CIJPLOT/CIJ0PLOT/MIJPLOT/PPFPLOT etc. plot defaults     *
*        (Call arg is pointer to new IJPLDEF)                          *
*                                                                      *
***********************************************************************/

void cmdflt(struct IJPLDEF *pc) {

   pc->cmmin = pc->cmmax = CMLIMDFLT;
   pc->cmpltx = 8.00;
   pc->cmplty = 8.00;
   pc->cmpltw = 1.00;
   pc->cmplth = 1.00;
   pc->smshrink = SHRNKDFLT;  /* Request default after ncells known */
   pc->cmtarg = 1;

   } /* End cmdflt() */


/***********************************************************************
*                                                                      *
*     cndflt - initial conn type defaults                              *
*        (Call arguments are pointers to celltype and conntype blocks) *
*                                                                      *
***********************************************************************/

void cndflt(struct CELLTYPE *il, struct CONNTYPE *ix) {

   int idv;                   /* Dynamic variable index */

   ix->Cn     = RP0->LCnD;    /* Defaults from CELLTYPE level */
   ix->svitms = il->svitms &
      (GFLIJ|GFCIJ|GFCIJ0|GFDIJ|GFSJ|GFMIJ|GFPPF|GFAFF|GFNUK);
   /* Create a PPFDATA block if a MASTER PPF card or CELLTYPE-level
   *  PPF card was read and initialize it with those defaults.  */
   if (RP0->n0flags & (N0F_MPPFE|N0F_CTPPFE) && !ix->PP) {
      ix->PP = (struct PPFDATA *)allocpmv(Static,
         IXstr_PPFDATA, "PPFDATA structure");
      *(ix->PP) = RP0->LPpfD; }

   ix->pradi = ix->pradj = -1;/* No print amplif details */
   ix->stride = 1;            /* stride = 1 (for kgen=A, B only) */
   ix->nhx = ix->nvy = 1;     /* (for kgen=D) */
   ix->nhy = ix->nvx = 0;

   /* Initialize all dynamic variables to "not requested" */
   for (idv=0; idv<CNNDV; idv++) ix->cnoff[idv] = (DVREQ - 1);
   for (idv=0; idv<XNNDV; idv++) ix->xnoff[idv] = (DVREQ - 1);

   } /* End cndflt() */


/***********************************************************************
*                                                                      *
*     cndflt2 - connection type defaults that depend on KGEN code      *
*        (Call arguments are pointers to celltype and conntype blocks) *
*                                                                      *
***********************************************************************/

void cndflt2(struct CELLTYPE *il, struct CONNTYPE *ix) {

   unsigned long kode = ix->kgen;

   /* Set default offsets for maps--all other cases xoff = yoff = 0 */
   if (kode & (KGNFU+KGNTP+KGNST))
      ix->xoff = ix->yoff = MINUS0;

   /* Set defaults relating to "hand vision" */
   if (kode & KGNHV)    /* Radius used with all forms of HV */
      ix->radius = ULFIXP(3.0,16);
   if (kode & KGNBV)    /* loff used only with BBD vision */
      ix->loff = 1;

   /* Set default hole size for annulus */
   if (kode & KGNAN) ix->nhx = ix->nhy = 1;

   /* Set default cell advance for systematic */
   if (kode & KGNYO) ix->lsg = 2;   /* An S1 variable */

   /* Set default Cij values for code 'L' */
   if (kode & KGNLM) {
      ix->ucij.l.c00 = ix->ucij.l.c01 =
         ix->ucij.l.c10 = ix->ucij.l.c11 = JFIXP(0.5, 24);
      ix->ucij.l.csg = JFIXP(0.167, 28);
      }
   /* Set default mno,nks for code 'M' */
   else if (kode & KGNMC) {
      ix->ucij.m.mno = 1;
      ix->ucij.m.nks = 1;
      }
   /* Set default cmn,csg,pp for code 'R' */
   else {
      ix->ucij.r.cmn = JFIXP( 0.5  , 24);
      ix->ucij.r.csg = JFIXP( 0.167, 28);
      ix->ucij.r.pp  = (kode & KGNHV) ? S16 : S15;
      }

   } /* End cndflt2() */


/***********************************************************************
*                                                                      *
*     ctdflt - cell type defaults                                      *
*        (Call argument is pointer to new celltype block)              *
*                                                                      *
*  Note: Many defaults are loaded from Group I or MASTER card values   *
*                                                                      *
***********************************************************************/

void ctdflt(struct CELLTYPE *il) {

   int idv;                            /* Dynamic variable index */

   /* Initialize all dynamic variables to "not requested" */
   for (idv=0; idv<CTNDV; idv++) il->ctoff[idv] = (DVREQ - 1);

   /* Copy the variables that are defaulted from MASTER cards */
   RP0->LCnD  = RP0->GCnD;
   RP0->LPpfD = RP0->GPpfD;
   il->Ct = RP0->GCtD;
   il->Dc = RP0->GDcD;
   il->Dp = RP0->GDpD;
   il->No = RP0->GNoD;

   il->jijpl  = 1;
   il->mncij  = SFIXP( -1.0  ,  8);
   il->mxcij  = SFIXP(  1.0  ,  8);
   il->svitms = RP0->pgds ? RP0->pgds->svitms & (GFABCD|GFSBAR|GFQBAR|
      GFPHI|GFDIS|GFLIJ|GFCIJ|GFCIJ0|GFDIJ|GFSJ|GFMIJ|GFPPF|GFAFF|
      GFNUK|GFIFUW|GFASM|GFSI) : 0;
   il->mxsdelay1 = 1;

   } /* End ctdflt() */


#ifdef D1
/***********************************************************************
*                                                                      *
*     d1dflt - Darwin 1 defaults                                       *
*        (Call argument is pointer to new D1BLK)                       *
*                                                                      *
***********************************************************************/

void d1dflt(struct D1BLK *pd1) {

   static long gd1seed = 0;

   pd1->hspacing = 0.40;
   pd1->vspacing = 0.20;
   pd1->cpr = RP0->GCtD.cpr;
   pd1->nbpe = BITSPERUI32;
   pd1->nsmxh = -1;        /* Request default in d3grp1 */
   pd1->nsprd = -1;        /* Request default in d3grp1 */
   pd1->d1seed = (gd1seed += 761348L);
   pd1->cpt  = -1;         /* Request default in d3grp1 */
   pd1->rept = -1;
   pd1->jfile = 1;

   } /* End d1dflt() */
#endif


/***********************************************************************
*                                                                      *
*     d3dflt - initial CNS global defaults                             *
*                                                                      *
*  N.B.  Defaults that depend on RP->compat must not be set here--     *
*  Group I control cards have not yet been read.                       *
***********************************************************************/

void d3dflt(void) {

   register int i;

   /* Note:  All color names must be upper case */
   /* Default colors for categories in order as defined in rpdef.h */
   static char *rpcolors[COLORNUM+2*SEQNUM] = { "CYAN", "SEQUENCE",
      "YELLOW", "GREEN", "RED", "CYAN", "YELLOW", "PHASES",
      "SEQUENCE", "GREEN", "YELLOW", "CYAN", "RED", "BLACK",
   /* Default colors for bubble plot sequences */
      "X006", "RED", "ORANGE", "YELLOW",
      "X990", "XF80", "CYAN", "BLACK",
   /* Default colors for phases */
      "XF00", "X906", "X40C", "X02E",
      "X088", "X0E2", "X4C0", "X960" };

   /* Default reset actions as a function of ReLops */
   const struct KRACT kra0[NRLACT] = {
/* RLtent */  { 0, 1, 0 },
/* RLtmr  */  { KRUSUAL, KRLSER, 0 },
/* RLevnt */  { KRUSUAL|KRANEWS, KRLSER, 0 },
/* RLval  */  { KRUSUAL|KRANEWS, KRLSER, 0 },
/* RLser  */  { KRUSUAL, KRLSER, 0 },
/* RLnow  */  { KRUSUAL|KRANEWS, KRLSER, 0 }, };

   /* Zero everything in RFdefs */
   memset((char *)d3file,0,FILETYPE_NUM*sizeof(void *));

/* Initializations that are not parameter defaults */

   /* Initialize linked lists rooted in RP,RP0 structures */
   RP0->plvc  = &RP0->pvc1;   /* Make empty sensors list */
   RP0->plplb = &RP0->pplb1;  /* Make empty plot labels list */
   RP0->plpln = &RP0->ppln1;  /* Make empty plot lines list */
   RP0->pltv  = &RP->pftv;    /* Make empty TV CAMERA list */
   for (i=0; i<SFHMNTYP; i++) /* Make empty restore lists */
      RP0->isfhm1[i] = -1;
   RP->mxsaa = 1;             /* Allocate at least one CONNDATA blk */

/* Run and reset options */

   RP->CP.runflags = (RP_OKPLOT | RP_OKPRIN | RP_NOINPT);
   if (qonlin()) RP->CP.runflags |= RP_ONLINE;
   RP->adefer = 1;            /* Amplification deferral */
   RP->ksv = KSVOK|KSDNE;
   RP->ntc1 = 1;              /* For SIMDATA SEGMENT 0 NITS */
   RP0->evhosts = 1;
   memcpy((char *)(RP->kra+RLtent), (char *)kra0, sizeof(kra0));

/* Print, plot, movie, simdata save, and reset timers */

   RP->jdcptim = RP->jeptim = RP->jsptim = RP->jspmftim =
      RP->jvcptim = gettimno(PLTRT, YES);
   RP->jrsettm = TRTN0;
   RP->jmvtim  = RP->jsdsvtm = TRTN1;
   RP0->GRpD.jrptim = gettimno(PRTRT, YES);
   RP0->GRpD.jbptim = gettimno(PLTRT, YES);

/* Other global parameters */

   RP0->satdcynum = DCYSFDLT;
   RP0->cdcycut = 0.01;

/* Variables relating to input array */

   RP->kx = RP->ky = 4;
   RP0->mpx = RP0->ncs = RP0->nds = 32;
   RP0->mxobj = 4;

/* Variables relating to stimulus noise */

   RP->snmn = RP->snsg = 0;
   RP->snfr = JFIXP(1.0, 24);
   RP->snseed = 9372546L;

/* Variables relating to 'CYCLE' card */

   RP->CP.rpirec = 1;
   RP->ifrz = -1;
   RP->iti  = 10;
   RP->timestep = 1000;

/* Plot scale and color defaults */

   RP0->GCtD.cpr = 0.9;
   RP0->GCtD.cpt = -1;
   RP0->grids = 1;
   RP0->pfac = 1.0;
   RP0->smshrnkdflt = 0.10;
   RP0->splx = 0.5;
   RP0->sply = 0.5;
   RP0->splw = 0.25;
   RP0->splh = 0.25*(SEQNUM + 2.0*PLCSP_BORDER*(1.0 - SEQNUM));
   RP0->vcpt = SFIXP(0.5, FBsi+Sr2mV);
   RP0->vplx[0] = 8.25, RP0->vplx[1] = 4.25;
   RP0->vply[0] = RP0->vply[1] = 0.50;
   RP0->vplw = 2.25;
   RP0->vplh = 0.25;
   RP->eplx = 3.0;
   RP->eply =-1.0;
   RP->eplw =-1.0;
   RP->eplh =-1.0;
   RP->stdlht = 0.22;

   strcpy(RP0->stitle, "0");
   RP0->lst = 1;
   for (i=0; i<(COLORNUM+2*SEQNUM); ++i)
      strncpy(RP->colors[i], rpcolors[i], COLMAXCH);

   } /* End d3dflt() */


/***********************************************************************
*                                                                      *
*     d3dflt2 - Celltype and connection type global defaults           *
*                                                                      *
*  N.B.  Some of these defaults depend on RP->compat mode, so they     *
*  are set after Group I control cards have been read.                 *
*  V8E, 02/10/09, GNR - Set OPTMEM from RP_OPTZZ, set up GPdD          *
*  Rev, 03/11/09, GNR - Add alternate defaults for IZHI2003            *
***********************************************************************/

void d3dflt2(void) {

/* Celltype default parameters */

   RP0->GCtD.st = LFIXP(  0.875, FBwk+FBsi); /* = 112mV new style */
   RP0->GCtD.ht = LFIXP(  0.875, FBwk+FBsi);
   if (RP->compat & OLDRANGE) {
      RP0->GCtD.cpt = SFIXP( 0.5 , FBsi+Sr2mV);
      RP0->GCtD.pt  = LFIXP( 0.5 , FBwk+FBsi);
      RP0->GCtD.nt  = LFIXP(-0.5 , FBwk+FBsi);
      RP0->GNoD.nmn = JFIXP(0.025, FBwk+FBsi);
      RP0->GNoD.nsg = JFIXP(0.05 , 31);
      RP0->GNoD.nfr = JFIXP(1.0  , 24);
      RP0->GDpD.upsd= LFIXP(0.05 , FBwk);
      RP0->GPdD.pht = JFIXP(0.1  , FBwk+FBsi);
      }
   else {
      RP0->GCtD.cpt = SFIXP(4.0  , FBsi);
      RP0->GDpD.upsd= LFIXP(0.05 , FBwk+Sr2mV);
      }
   if (RP->CP.runflags & RP_OPTZZ) RP0->GCtD.ctopt |= OPTMEM;
   RP0->GCtD.Cm     = 100 << FBCm;
   RP0->GCtD.sspike = RP0->GCtD.stanh = JFIXP(1.0, FBwk+FBsi);
   RP0->GCtD.taum   = 100 << FBwk;
   RP0->GCtD.jketim = TRTN1;
   strncpy(RP0->GCtD.sicolor, RP->colors[CI_BUB], COLMAXCH);

   RP0->GASD.ngtht = -1;               /* AUTOSCALE defaults */
   RP0->GASD.asmxd = JFIXP(0.05,FBsc);
   RP0->GASD.asmximm = JFIXP(1.50,FBsc);
   RP0->GASD.asmnimm = JFIXP(0.50,FBsc);
   RP0->GASD.adamp = USFIXP(0.1, FBdf);
   RP0->GASD.kaut = KAUT_S|KAUT_G|KAUT_M;
   RP0->GDcD.CDCY.omega = LFIXP(0.4, FBod);
   RP0->GDcD.omega1 = LFIXP(  1.0 , FBod);
   RP0->GDcD.siet   = -1;              /* Set in d3tchk according */
   RP0->GDcD.siscl  = -1;              /*    to phasing options.  */
   RP0->GDcD.sdamp  = USFIXP( 0.2 , FBdf);
   RP0->GDcD.qdamp  = USFIXP( 0.1 , FBdf);

   RP0->GDpD.omegad = LFIXP( 0.90 , FBod);

   RP0->GPdD.phimeth = PHI_CENTROID;         /* CENTROID */
   RP0->GPdD.simeth  = SI_CENTROID;          /* CENTROID */

/* Defaults for aEIF (Brette-Gerstner) response function */

   RP0->GBGD.gL     = UJFIXP( 30.0, 20);
   RP0->GBGD.vPeak  = JFIXP(  90.6, FBwk);
   RP0->GBGD.vT     = JFIXP(  20.2, FBwk);
   RP0->GBGD.delT   = JFIXP(   2.0, FBwk);
   RP0->GBGD.tauW   = UJFIXP(144.0, 20);
   RP0->GBGD.bga    = JFIXP(   4.0, 24);
   RP0->GBGD.bgb    = JFIXP(0.0805, 20);

/* Defaults for Izhikevich response function.  Note that defaults
*  for c,d,vpeak,izcv2,izcv0,izvt are not rescaled for COMPAT=C.
*  Effectively, the values of these defaults are 128 x less (more
*  for izcv2) in this case:  more reasonable, and avoids overflow.  */

   RP0->GIz3D.Z.iza   = JFIXP( 0.02, FBIab);
   RP0->GIz3D.Z.izb   = JFIXP(  0.2, FBIab);
   RP0->GIz3D.Z.izc   = JFIXP(-65.0, FBwk);
   RP0->GIz3D.Z.izd   = JFIXP(  2.0, FBIu);
   RP0->GIz3D.Z.vpeak = JFIXP( 30.0, FBwk);
   RP0->GIz3D.izcv2   = JFIXP( 0.04, FBod);
   RP0->GIz3D.izcv1   = JFIXP(  5.0, FBsc);
   RP0->GIz3D.izcv0   = JFIXP(140.0, FBwk);

   RP0->GIz7D.Z.iza   = JFIXP( 0.03, FBIab);
   RP0->GIz7D.iizb    = LFIXP( -2.0, 23);
   RP0->GIz7D.Z.izc   = JFIXP( 10.0, FBwk);
   RP0->GIz7D.iizd    = JFIXP(100.0, 19);
   RP0->GIz7D.Z.vpeak = JFIXP( 95.0, FBwk);
   RP0->GIz7D.izk     = JFIXP(  0.7, FBsc);
   RP0->GIz7D.izvt    = JFIXP( 20.0, FBwk);
   RP0->GIz7D.bvlo    = MINUS0;
   RP0->GIz7D.umax    = SI32_MAX;
   RP0->GIz7D.uv3lo   = MINUS0;

/* Connection type default parameters */

   if (RP->compat & OLDRANGE) {
      RP0->GCnD.mti = RP0->GCnD.mtj = SFIXP(0.5, 14);
      RP0->GCnD.mtv = SFIXP( 0.5 , FBsv);
      RP0->rssck    = SJSGN_POS;
      }
   else {
      RP0->GCnD.vdha = LFIXP(  64 , 20);
      RP0->rssck    = (SJSGN_POS|SJSGN_NEG);
      }
   RP0->GCnD.mxax   = SI32_MAX;
   RP0->GCnD.scl    = JFIXP( 1.0 , 24);
   RP0->GCnD.upsm   = LFIXP( 1.0 , 16);
   RP0->GCnD.zetam  = LFIXP( 0.1 , 16);
   RP0->GCnD.mticks = 4;
   RP0->GCnD.mxmij  = SFIXP( 1.0 , FBsi+Sr2mV);
   RP0->GCnD.mxmp   = LFIXP( 2.0 , 20);
   RP0->GCnD.nbc    = 4;
   RP0->GCnD.rdamp  = USFIXP(0.05, FBdf);
   RP0->GCnD.vi     = 1;
   RP0->GPpfD.htup  = USFIXP( 30 ,  1);
   RP0->GPpfD.htdn  = USFIXP(300 ,  1);
   RP0->GPpfD.ppflim= SFIXP( 0.25, 12);
   RP0->GPpfD.ppft  = -1;     /* Set from pt */

   } /* End d3dflt2() */


/***********************************************************************
*                                                                      *
*     effdflt - effector arbor defaults                                *
*        (Call arg is pointer to new EFFARB block)                     *
*                                                                      *
***********************************************************************/

void effdflt(struct EFFARB *pea) {

   pea->escli[EXCIT] = pea->escli[INHIB] = 1.0;
   pea->arbs = -1L;     /* Signal not entered */
   pea->nchgs = 1;

   } /* End effdflt() */


/***********************************************************************
*                                                                      *
*     envdflt - environment block defaults                             *
*     N.B.  ecolstm is not set here--color mode is not yet known       *
*                                                                      *
***********************************************************************/

void envdflt(void) {

   strcpy(RP0->ecolgrd,"CYAN");   /* Grid color */
   strcpy(RP0->ecolwin,"RED");    /* Window color */
   strcpy(RP0->ecolarm,"XB0F");   /* Arm color (pink) */
   RP0->eilo = 0;             /* Background pixel */
   RP0->eihi = 240;           /* Foreground pixel */
   RP0->epr = 0.7;            /* Pixel radius */
   RP0->ept = 32;             /* Pixel threshold */

   } /* End envdflt() */


/***********************************************************************
*                                                                      *
*     ibdflt - inhibition type defaults                                *
*        (Call arguments are:                                          *
*           1) pointer to new inhibition block                         *
*           2) 'nib' = number of beta's at end of block)               *
*                                                                      *
*  CAUTION:  This routine must not erase the pointer in ib->inhbnd!!   *
*                                                                      *
***********************************************************************/

void ibdflt(struct INHIBBLK *ib, int lnib) {

   struct INHIBBAND *pb = ib->inhbnd;
   register int i;

   ib->nib = lnib;
   ib->ngb = 1;

/* Set nib values of mxib to largest available number */

   for (i=0; i<lnib; i++,pb++) pb->mxib = LONG_MAX;

   } /* End ibdflt() */


/***********************************************************************
*                                                                      *
*     mddflt - modulation defaults                                     *
*        (Call argument is pointer to new modulation block)            *
*                                                                      *
***********************************************************************/

void mddflt(struct MODBY *im) {

   im->mscl  = LFIXP(-0.5, FBwk);
   im->mxmod = LONG_MAX;

   } /* End mddflt() */


/***********************************************************************
*                                                                      *
*     mdltdflt - modality defaults                                     *
*        (Call argument is pointer to new MODALITY block)              *
*                                                                      *
***********************************************************************/

void mdltdflt(struct MODALITY *pmdlt) {

   pmdlt->ntcs  = 0xFFFF;     /* Default--treated as -1 in d3echo    */
   pmdlt->ntrs  = 0xFFFF;     /* Default--treated as -1 in d3echo    */
   pmdlt->ngpnt = -1;

   } /* End mdltdflt() */


/***********************************************************************
*                                                                      *
*     prbdflt - probe defaults                                         *
*        (Call argument is pointer to new PRBDEF block)                *
*                                                                      *
***********************************************************************/

void prbdflt(struct PRBDEF *pprb) {

   pprb->pptr   = LONG_MAX;
   pprb->sprb   = JFIXP(128.0, FBwk);
   pprb->cpp    = SHRT_MAX;
   pprb->cyc1   = 1;
   pprb->prbopt = PRBMARK;
   pprb->jprbt  = TRTN1;

   } /* End prbdflt() */


/***********************************************************************
*                                                                      *
*     prepdflt - image preprocessing defaults                          *
*        (Call argument is pointer to new PREPROC block                *
*                                                                      *
***********************************************************************/

void prepdflt(struct PREPROC *pip) {

   pip->ipplx = 3.0;          /* Left x coord of displayed image */
   pip->ipply = 3.5;          /* Bottom y coord, in inches */
   pip->upr.oipx = UI16_MAX;  /* Call for default offsets */
   pip->upr.oipy = UI16_MAX;
   pip->upr.ipkcol = Col_GS;  /* Color is grayscale */
   } /* End prepdflt() */


/***********************************************************************
*                                                                      *
*     rpdflt - repertoire block defaults                               *
*        (Call argument is pointer to new repertoire block)            *
*                                                                      *
***********************************************************************/

void rpdflt(struct REPBLOCK *ir) {

   /* Copy the variables that have global defaults */
   ir->Rp = RP0->GRpD;

   ir->aplx = -1.0;
   ir->aply = -1.0;
   ir->aplw = -1.0;
   ir->aplh = -1.0;

   } /* End rpdflt() */


/***********************************************************************
*                                                                      *
*     itpdflt - Image touch pad defaults                               *
*        (Call argument is pointer to new IMGTPAD block)               *
*                                                                      *
***********************************************************************/

void itpdflt(struct IMGTPAD *pitp) {

   pitp->scla = 1.0;
   pitp->ovsy = 1, pitp->ovso = 2;
   pitp->itchng = 3;
   pitp->itop = ITOP_INT;
   pitp->vc.vcpt = -1;           /* Use global default later */

   } /* End itpdflt() */


/***********************************************************************
*                                                                      *
*     tvdflt - TV camera setting defaults                              *
*        (Call argument is pointer to new TVDEF block)                 *
*                                                                      *
***********************************************************************/

void tvdflt(struct TVDEF *pcam) {

   pcam->tvplx = 3.0;         /* Top left x coord of displayed image */
   pcam->tvply = 5.5;         /* Top left y coord, in inches */
   pcam->rspct = 90;          /* Target percentile for rescaling */
   pcam->rslvl = 230;         /* Gray level rspct is rescaled to */
   pcam->utv.tvix = 512;      /* x-size of raw TV image */
   pcam->utv.tviy = 384;      /* y-size of raw TV image */
   pcam->utv.tvx = 60;        /* x-size of TV image after averaging */
   pcam->utv.tvy = 60;        /* y-size of TV image after averaging */
   pcam->utv.tvsa = 1;        /* Space averaging factor */
   pcam->utv.tvta = 1;        /* Time averaging factor */
   pcam->utv.tvkcol  = UTVCol_C24;
   pcam->utv.tvkfreq = UTVCFr_TRIAL;
   } /* End tvdflt() */


/***********************************************************************
*                                                                      *
*     vbdflt - value defaults                                          *
*        (Call argument is pointer to new VBDEF block)                 *
*                                                                      *
***********************************************************************/

void vbdflt(struct VBDEF *ivb) {

   /* Defaults for valnm are not used by all value types, but
   *  do insure that error message texts are always valid.  */
   setrlnm(&ivb->valnm, "01", "01");
   ivb->vscl   = (1L<<24);
   ivb->vdamp  = (1<<14);   /* Default is 0.5 (S15) */
   ivb->vbest  = 10;
   ivb->vdelay = 1;

   } /* End vbdflt() */


/***********************************************************************
*                                                                      *
*     vcdflt - Virtual cell (sensor) defaults                          *
*        (Call args are ptr to new vcell block and source id name)     *
*                                                                      *
*  N.B.  There are no defaults for vcplx, vcply because the default is *
*  that plotting is turned off.  Defaults for nvx,nvy depend on info   *
*  that is not known at vcdflt() time--these are set to (-1,-1), then  *
*  filled in from values in snsnvx0,snsvy0 tables at d3schk() time.    *
*  vcmin,vcmax are irrelevant for VJ,VT,VW but are harmless.           *
***********************************************************************/

/* Default values for numbers of sensors along x,y.  (These must
*  be in same order as XX_SRC constants defined in d3global.h,
*  with defaults for user-defined senses in the [0] position.) */
short snsnvx0[NRST] =
   { 12, 0, 0, 12, 12, 4, 0, 0, 0, 0, 0 } ;
short snsnvy0[NRST] =
   {  1, 0, 0, -1, -1, 8, 0, 0, 0, 0, 0 } ;

void vcdflt(struct VCELL *pvc, char *vsrcrid) {

   /* Store vsrcrid name in VCELL block.  Long name handle
   *  for USRSRC will be stored later, when read in d3grp1().  */
   strncpy(pvc->vcname, vsrcrid, LSNAME);

   /* Store sensor type code */
   pvc->vtype = getsnsid(vsrcrid);

   /* Temporarily set number of sensors along x,y */
   pvc->nvx = pvc->nvy = -1;

   /* Set remaining defaults */
   pvc->vcmin = 0.0;                /* Minimum value */
   pvc->vcmax = 1.0;                /* Maximum value */
   pvc->vcplw = 1.0;                /* Plot width    */
   pvc->vcplh = 0.25;               /* Plot height   */
   pvc->vhw  = 2.0;                 /* Half width    */
   pvc->vcpt = -1;                  /* Use global default later */

   } /* End vcdflt() */
