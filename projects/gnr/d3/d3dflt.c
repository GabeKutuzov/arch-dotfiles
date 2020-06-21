/* (c) Copyright 1985-2018, The Rockefeller University *11115* */
/* $Id: d3dflt.c 78 2018-08-02 18:36:58Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                              d3dflt.c                                *
*                                                                      *
*  Includes routines armdflt, cmdflt, cndflt, cndflt2, ctdflt,         *
*     d3dflt, d3dflt2, effdflt, envdflt, ibdflt, itpdflt, jntdflt,     *
*     mddflt, mdltdflt, opdflt, prbdflt, prepdflt, rpdflt, tvdflt,     *
*     vbdflt, vcdflt, terdflt, wdwdflt                                 *
*                                                                      *
*  Establish defaults as defined in the writeup                        *
*     (Note: There is a separate entry for each major type of control  *
*  block.  Defaults that occur in this source file are not defined     *
*  with #define--because they occur in just this one place and this    *
*  is the natural place to look for them, there is no point to refer   *
*  the reader to a separate header file.  -GNR)                        *
************************************************************************
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
*  Rev, 06/30/12, GNR - Change wayset dflts to 1.0, add qpull = 0.5    *
*  Rev, 08/29/12, GNR - Add upspsi default                             *
*  V8I, 01/12/13, GNR - Make etlo, itlo, mtlo defaults -inf.           *
*  Rev, 05/14/13, GNR - Add defaults for AUTOSCALE KAUT=H              *
*  Rev, 07/11/13, GNR - Correct cpt overwrite in d3dflt2               *
*  R66, 02/27/16, GNR - Longs to si32 in PRBDEF                        *
*  R66, 03/11/16, GNR - Rename nvx->nsxl, nvy->nsya so != ix->nv[xy]   *
*                       Add armdflt, jntdflt, wdwdflt                  *
*  R67, 04/27/16, GNR - Remove Darwin 1 support                        *
*  R67, 12/08/16, GNR - smshrnk->smscale                               *
*  R72, 03/18/17, GNR - Remove KGENGD defaults                         *
*  R73, 04/18/17, GNR - Remove nhx,nhy defaults for old KGNAN, mnbprad *
*  R75, 10/04/17, GNR - Add terdflt                                    *
*  R77, 12/28/17, GNR - Redo DEFTHR to handle global->local properly   *
*  R77, 01/30/18, GNR - Add vdmm default (-inf ==> no test), emxsj     *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "d3global.h"
#include "d3fdef.h"
#include "armdef.h"
#include "jntdef.h"
#include "wdwdef.h"
#include "ijpldef.h"
#include "simdata.h"
#include "tvdef.h"
#include "rocks.h"
#include "rkxtra.h"


/***********************************************************************
*                                                                      *
*     armdflt - defaults for each new arm                              *
*        (Call arg is pointer to new ARMDEF)                           *
*                                                                      *
***********************************************************************/

void armdflt(struct ARMDEF *pa) {

   pa->njnts = 4;
   pa->jatt = 0;
   pa->jpos = 6;
   strcpy(pa->acolr,"DEFAULT");
   /* The following are just defaults for joint values, not used by
   *  arm code but collected here so in one place in d3dflt.c  */
   pa->jmn0 = 12.0;
   pa->jmx0 = 180.0;
   pa->ja00 = 20.0;
   pa->jl00 =  8.0;
   pa->scla0 = 7.5;
   pa->tll = 8;            /* These are effective joint defaults */
   pa->tla = -1;           /* Default comes from joint length */
   pa->ntl = pa->nta = 8;

   } /* End armdflt */


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
   pc->smscale = 1.00;
   pc->cmtarg = 1;
   memcpy(pc->cmeico[EXCIT], RP->colors[CI_EXC], COLMAXCH);
   memcpy(pc->cmeico[INHIB], RP->colors[CI_INH], COLMAXCH);

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
      (GFLIJ|GFCIJ|GFCIJ0|GFDIJ|GFSJ|GFMIJ|GFPPF|GFAFF|GFNUK|GFRAW);
   /* Create a PPFDATA block if a MASTER PPF card or CELLTYPE-level
   *  PPF card was read and initialize it with those defaults.  */
   if (RP0->n0flags & (N0F_MPPFE|N0F_CTPPFE) && !ix->PP) {
      ix->PP = (struct PPFDATA *)allocpmv(Static,
         IXstr_PPFDATA, "PPFDATA structure");
      *(ix->PP) = RP0->LPpfD; }

   ix->stride = 1;            /* stride = 1 (for kgen=A,B,K only) */

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
   if (kode & (KGNFU|KGNTP|KGNST))
      ix->xoff = ix->yoff = SI32_SGN;

   /* Set default radius and width */
   if (kode & (KGNHV|KGNKN))  /* Radius used with HV and Kernel */
      ix->hradius = ULFIXP(3.0,16);
   if (kode & KGNBV)          /* loff used only with BBD vision */
      ix->loff = 1;

   /* Set default cell advance for KGNYO (others in d3tchk) */
   if (ix->kgen & KGNYO) ix->lsg = 2;        /* An S1 variable */

   /* Set default Cij values for code 'L' (when this is called,
   *  KGNTW and KGNLM have not yet been distinguished)  */
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
   RP0->LDfD  = RP0->GDfD;
   il->Ct = RP0->GCtD;
   il->Dc = RP0->GDcD;
   il->Dp = RP0->GDpD;
   il->No = RP0->GNoD;

   il->jijpl  = 1;
   il->mncij  = SFIXP( -1.0  ,  8);
   il->mxcij  = SFIXP(  1.0  ,  8);
   /* Note GFGLBL option is not default here -- see d3gfcl rules */
   il->svitms = RP0->pgds ? RP0->pgds->svitms & (GFVMP|GFSBAR|GFQBAR|
      GFPHI|GFDIS|GFLIJ|GFCIJ|GFCIJ0|GFDIJ|GFSJ|GFMIJ|GFPPF|GFAFF|
      GFNUK|GFIFUW|GFASM|GFSI|GFABCD|GFRAW) : 0;
   il->mxsdelay1 = 1;

   } /* End ctdflt() */


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
   /* Default colors for categories in order as defined in rpdef.h
   *  (exactly COLORNUM = 14 entries here).  */
   static char *rpcolors[COLORNUM+2*SEQNUM] = { "CYAN" /* CI_BOX */,
      "SEQUENCE" /* CI_BUB */,  "YELLOW" /* CI_EXT */,
      "GREEN"    /* CI_EXC */,  "RED"    /* CI_INH */,
      "CYAN"     /* CI_REP */,  "YELLOW" /* CI_LAY */,
      "GREEN"    /* CI_SPEX */, "RED"    /* CI_SPIN */,
      "GREEN"    /* CI_HPP */,  "YELLOW" /* CI_HPM */,
      "CYAN"     /* CI_H            MP */,  "RED"    /* CI_HMM */,
      "BLACK"    /* SPARE */,
   /* Default colors for bubble plot sequences */
      "X006", "RED", "ORANGE", "YELLOW",
      "X990", "XF80", "CYAN", "BLACK",
   /* Default colors for phases */
      "XF00", "X906", "X40C", "X02E",
      "X088", "X0E2", "X4C0", "X960" };

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

/* Run and reset options */

   RP->CP.runflags = (RP_OKPLOT | RP_OKPRIN | RP_NOINPT);
   if (qonlin()) RP->CP.runflags |= RP_ONLINE;
   RP->CP.rlnow = RL_ISKIP;
   RP->ksv = KSVOK|KSDNE;
   RP->ntc1 = 1;              /* For SIMDATA SEGMENT 0 NITS */
   RP0->evhosts = 1;
   RP0->rlvl[lvtent] = RL_DEFER;
   RP0->rlvl[lvtimr] = RL_SERIES;
   RP0->rlvl[lvevnt] = RL_NEWSERIES;
   RP0->rlvl[lvser]  = RL_SERIES;
   RP0->rlvl[lvnow]  = RL_ISKIP;
   RP0->rlvl[lvarm]  = RL_SERIES;
   RP0->rlvl[lvwin]  = RL_SERIES;
   RP0->rlvl[lvval]  = RL_SERIES;

/* Print, plot, movie, simdata save, and reset timers */

   RP->jdcptim = RP->jeptim = RP->jsptim = RP->jspmftim =
      RP->jvcptim = gettimno(PLTRT, YES);
   RP->jrsettm = TRTN0;
   RP->jmvtim  = RP->jsdsvtm = TRTN1;
   RP0->GRpD.jbptim = gettimno(PLTRT, YES);

/* Other global parameters */

   RP0->GCnD.cijmine = SI32_SGN;     /* Will depend on nbc later */
   RP0->GCnD.adefer = 1;               /* Amplification deferral */
   RP0->GCnD.nbc    = 4;                      /* Bits/connection */
   RP0->cdcycut = 0.01;
   RP0->satdcynum = DCYSDFLT;
   RP->nhlssv    = 100;      /* No. trials to save high/low affs */

/* Variables relating to input array */

   RP->kx = RP->ky = 4;
   RP0->mpx = RP0->ncs = RP0->nds = 32;
   RP0->mxobj = 4;

/* Variables relating to stimulus noise */

   RP->snmn = RP->snsg = 0;
   RP->snfr = JFIXP(1.0, 24);
   RP->snseed = 9372546;

/* Variables relating to 'CYCLE' card */

   RP->CP.rpirec = 1;
   RP->ifrz = 0;
   RP->iti  = 10;
   RP->timestep = 1000;

/* Plot scale and color defaults */

   RP0->GCtD.cpr = 0.9;
   RP0->GCtD.cpcap = S14;
   RP0->GCtD.cpt = -1;
   RP0->grids = 1;
   RP0->pfac = 1.0;
   RP0->splx = 0.5;
   RP0->sply = 0.5;
   RP0->splw = 0.25;
   RP0->splh = 0.25*(SEQNUM + 2.0*PLCSP_BORDER*(1.0 - SEQNUM));
   RP0->vcpt = SFIXP(0.5, FBsi+Sr2mV);
   RP0->vplx[0] = 8.25, RP0->vplx[1] = 4.25;
   RP0->vply[0] = RP0->vply[1] = 0.50;
   RP0->vplw = 2.25;
   RP0->vplh = 0.25;
   RP0->pltbufl = 4092;
   RP->eplx = 3.0;
   RP->eply =-1.0;
   RP->eplw =-1.0;
   RP->eplh =-1.0;
   RP->mnbpr = 0.01;
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
*  Rev, 09/27/12, GNR - Change Si decay omega (= omega2) default to 0  *
*  Rev, 10/22/15, GNR - Make nfr default 1S24 on new and old scales    *
***********************************************************************/

void d3dflt2(void) {

/* Celltype default parameters */

   RP0->GCtD.st = JFIXP(  0.875, FBwk+FBsi); /* = 112mV new style */
   RP0->GCtD.ht = JFIXP(  0.875, FBwk+FBsi);
   RP0->GCtD.siha = JFIXP(0.5,   FBwk+FBsi);
   RP0->GNoD.nfr = JFIXP( 1.0,   24);
   RP0->GNoD.noiflgs = NFL_ADDR;
   if (RP->compat & OLDRANGE) {
      RP->gsimax = 1 << (FBwk+FBsi);
      if (RP0->GCtD.cpt == -1)
         RP0->GCtD.cpt = SFIXP( 0.5 , FBsi+Sr2mV);
      RP0->GCtD.pt  = JFIXP( 0.5 , FBwk+FBsi);
      RP0->GCtD.nt  = JFIXP(-0.5 , FBwk+FBsi);
      RP0->GNoD.nmn = JFIXP(0.025, FBwk+FBsi);
      RP0->GNoD.nsg = JFIXP(0.05 , 31);
      RP0->GDpD.upsd= LFIXP(0.05 , FBwk);
      RP0->GPdD.pht = JFIXP(0.1  , FBwk+FBsi);
      }
   else {
      RP->gsimax = SHRT_MAX << (FBwk-FBsi);
      RP->gsimin = -RP->gsimax;
      if (RP0->GCtD.cpt == -1)
         RP0->GCtD.cpt = SFIXP(4.0  , FBsi);
      RP0->GDpD.upsd= LFIXP(0.05 , FBwk+Sr2mV);
      }
   if (RP->CP.runflags & RP_OPTZZ) RP0->GCtD.ctopt |= OPTMEM;
   RP0->GCtD.Cm     = 100 << FBCm;
   RP0->GCtD.sslope = S24;
   RP0->GCtD.sspike = RP0->GCtD.stanh = JFIXP(1.0, FBwk+FBsi);
   RP0->GCtD.taum   = 100 << FBwk;
   RP0->GCtD.jketim = TRTN1;
   memcpy(RP0->GCtD.sicolor[EXCIT], RP->colors[CI_BUB], COLMAXCH);
   memcpy(RP0->GCtD.sicolor[INHIB], RP->colors[CI_BUB], COLMAXCH);

   RP0->GDcD.siet   = -1;              /* Set in d3tchk according */
   RP0->GDcD.siscl  = -1;              /*    to phasing options.  */
   RP0->GDcD.upspsi = UJFIXP( 0.5 , 22);
   RP0->GDcD.omega1 = USFIXP( 1.0 , FBdf);
   RP0->GDcD.sdamp  = USFIXP( 0.2 , FBdf);
   RP0->GDcD.qdamp  = USFIXP( 0.1 , FBdf);

   RP0->GDpD.omegad = LFIXP( 0.90 , FBod);

   RP0->GPdD.phimeth = PHI_CENTROID;         /* CENTROID */
   RP0->GPdD.simeth  = SI_CENTROID;          /* CENTROID */

/* Defaults for autoscaling */

   RP0->GASD.asmul[0] = RP0->GASD.asmul[1] = S24;
   RP0->GASD.asmxd = JFIXP(0.2,FBsc);
   RP0->GASD.asmximm = JFIXP(1.50,FBsc);
   RP0->GASD.asmnimm = JFIXP(0.50,FBsc);
   RP0->GASD.astt  = SFIXP(0.875,FBsi+Sr2mV);
   RP0->GASD.astf1 = USFIXP(0.1, FBdf);
   RP0->GASD.astfl = USFIXP(0.1, FBdf);
   RP0->GASD.adamp = USFIXP(0.1, FBdf);
   RP0->GASD.kaut = KAUT_S|KAUT_G|KAUT_M;
   RP0->GASD.navg = -1;    /* Call for default */

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
   RP0->GIz7D.bvlo    = SI32_SGN;
   RP0->GIz7D.umax    = SI32_MAX;
   RP0->GIz7D.uv3lo   = SI32_SGN;

/* Connection type default parameters.
*  Design note:  getthr() currently puts everything in ix->Cn when
*  we have OLDRANGE, otherwise uses [LG]DfD system for items with
*  variable scales.  If we add many more of these, it will be better
*  to always use [LG]DfD with new or old range, but then getthr()
*  will have a more complicated format selection problem.  mxmij
*  and mxmp do not require this mechanism because scaling depends
*  only on whether or not 'mV' is in the input, not the input type. */

   if (RP->compat & OLDRANGE) {
      RP0->GCnD.etlo = -SHRT_MAX;
      RP0->GCnD.mti = SFIXP(0.5, 14);
      RP0->GCnD.mtj = SFIXP(0.5, 14);
      RP0->GCnD.mtv = SFIXP(0.5, FBvl);
      RP0->GCnD.emxsj = SHRT_MAX;
      RP0->GCnD.mximg = JFIXP(1.0, 27);
      RP0->GCnD.scl = SI32_SGN;        /* Default later */
      RP0->rssck    = SJSGN_POS;
      }
   else {
      RP0->GCnD.mximg  = JFIXP(1.0, 20);
      RP0->GDfD.umVt.t.etlo = -SHRT_MAX;
      RP0->GDfD.ufrt.t.etlo = -SHRT_MAX;
      RP0->GDfD.umVt.t.emxsj = SHRT_MAX;
      RP0->GDfD.ufrt.t.emxsj = SHRT_MAX;
      RP0->GDfD.umVt.t.ppft = -1;      /* Set from pt */
      RP0->GDfD.ufrt.t.ppft = -1;      /* Set from pt */
      RP0->GDfD.us.s.scl = SI64_SGN;   /* Default later */
      RP0->rssck    = (SJSGN_POS|SJSGN_NEG);
      }
   RP0->GCnD.mxax   = SI32_MAX;
   RP0->GCnD.tcwid  = 45.0;
   RP0->GCnD.vdha   = JFIXP(64 , FBwk); /* = 0.5 (S27) */
   RP0->GCnD.vdmm   = SI32_SGN;
   RP0->GCnD.upsm   = JFIXP( 1.0 , 16);
   RP0->GCnD.zetam  = JFIXP( 0.1 , 16);
   RP0->GCnD.mticks = 4;
   RP0->GCnD.mxmij  = SFIXP( 1.0 , FBsi+Sr2mV);
   RP0->GCnD.mxsi   = SHRT_MAX;
   RP0->GCnD.mxmp   = JFIXP( 2.0 , 20);
   RP0->GCnD.qpull  = USFIXP(0.5 , FBdf);
   RP0->GCnD.rdamp  = USFIXP(0.05, FBdf);
   RP0->GCnD.sclmul = S24;             /* Kludge */
   RP0->GCnD.vi     = 1;
   {  short *pwse, *pws = RP0->GCnD.wayset;
      pwse = pws + MAX_WAYSET_CODES;
      while (pws < pwse) *pws++ = S10;
      } /* End pws,pwse local scope */
   RP0->GPpfD.htup  = USFIXP( 30 ,  1);
   RP0->GPpfD.htdn  = USFIXP(300 ,  1);
   RP0->GPpfD.ppflim= SFIXP( 0.25, 12);

   } /* End d3dflt2() */


/***********************************************************************
*                                                                      *
*     effdflt - effector arbor defaults                                *
*        (Call arg is pointer to new EFFARB block)                     *
*                                                                      *
*  R64, 12/15/15, GNR - Change eft defaults to -inf (no test)          *
*  Rev, 12/19/15, GNR - Change 'arbs' to 'arbsep[NEXIN]'               *
***********************************************************************/

void effdflt(struct EFFARB *pea) {

   pea->escli[EXCIT] = pea->escli[INHIB] = 1.0;
   pea->eft[EXCIT] = pea->eft[INHIB] = -SHRT_MAX;
   pea->arbsep[EXCIT] = pea->arbsep[INHIB] = -1;
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
   ib->ittlo = -SHRT_MAX;

/* Set nib values of mxib to largest available number */

   for (i=0; i<lnib; i++,pb++) pb->mxib = SI32_MAX;

   } /* End ibdflt() */


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
*     jntdflt - Joint defaults                                         *
*     Arguments:                                                       *
*        pa       Ptr to parent arm                                    *
*        pj       Ptr to joint to be initialized                       *
*        kj       Kind of joint: 0-->normal, 1-->universal, 2-->grip   *
*                                                                      *
***********************************************************************/

void jntdflt(struct ARMDEF *pa, struct JNTDEF *pj, int kj) {

   switch ((enum kjnt)kj) {
   case Normal:
      pj->jl            = pa->jl00;
      pj->u.jnt.ja      = pa->ja00;
      pj->u.jnt.jmn     = pa->jmn0;
      pj->u.jnt.jmx     = pa->jmx0;
      pj->u.jnt.tla     = pa->tla;
      pj->u.jnt.tll     = pa->tll;
      pj->u.jnt.tzi     = 0;
      pj->u.jnt.ntl     = pa->ntl;
      pj->u.jnt.nta     = pa->nta;
      break;
   case Universal:
      pj->jl            =  0.0;
      pj->u.ujnt.ujsclx =  4.0;
      pj->u.ujnt.deltax =  pj->u.ujnt.deltay = 0.0;
      pj->u.ujnt.ujvt   =  128;
      pj->u.ujnt.ujvi   =    1;
      pj->u.ujnt.ctime  =    4;
      pj->u.ujnt.ujct   =    0;
      pj->u.ujnt.ujdxn  =  0.0;
      pj->u.ujnt.ujdyn  = -1.0;
      break;
   case Gripper:
      pj->u.grip.gwidth = 2.0;
      pj->u.grip.gspeed = 1.0;
      pj->u.grip.grvt   = 128;
      pj->u.grip.grvi   =   1;
      pj->u.grip.gtime  =   4;
      pj->u.grip.gwidth = 2.0;
      pj->u.grip.grct   =   0;
      break;
      } /* End kjnt switch */

   } /* End jntdflt() */


/***********************************************************************
*                                                                      *
*     mddflt - modulation defaults                                     *
*        (Call argument is pointer to new modulation block)            *
*                                                                      *
***********************************************************************/

void mddflt(struct MODBY *im) {

   im->mscl  = LFIXP(-0.5, FBwk);
   im->mxmod = SI32_MAX;
   im->Mc.mtlo = -SHRT_MAX;

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

   pprb->pptr   = SI32_MAX;
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
   pip->dcols = 1;            /* Number of plot columns */
   pip->dcoff = 2;            /* Column and row separation */
   pip->upr.oipx = pip->upr.oipy = UI16_MAX; /* Call for defaults */
   pip->upr.ipkcol = TVC_PNE; /* Indicates cmode not entered */
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
*     tvdflt - TV camera setting defaults                              *
*        (Call argument is pointer to new TVDEF block)                 *
*                                                                      *
***********************************************************************/

void tvdflt(struct TVDEF *pcam) {

   pcam->tvplx = 3.0;         /* Top left x coord of displayed image */
   pcam->tvply = 5.5;         /* Top left y coord, in inches */
   pcam->rsseed = RS_SEED;    /* Random rounding seed */
   pcam->rspct = 90;          /* Target percentile for rescaling */
   /* rslvl[NColorDims] gives gray levels rspct histo levels are
   *  rescaled to.  Green, blue are copied from red at g1cam if colored
   *  images and only a single value is entered.  */
   pcam->rslvl[0] = USFIXP(0.9, FBim);
   pcam->utv.tvrxe = 512;     /* Entered x-size of raw TV image */
   pcam->utv.tvrye = 512;     /* Entered y-size of raw TV image */
   pcam->utv.tvx = 60;        /* x-size of TV image after averaging */
   pcam->utv.tvy = 60;        /* y-size of TV image after averaging */
   pcam->utv.tvsa = 1;        /* Space averaging factor */
   pcam->utv.tvta = 1;        /* Time averaging factor */
   pcam->utv.tvkicol = TVC_GS|TVC_B1;
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
   ivb->vscl   = (1<<FBsc);
   ivb->vdamp  = (1<<(FBdf-1));  /* Default is 0.5 (S15) */
   ivb->vbest  = 10;
   ivb->vdelay = 1;

   } /* End vbdflt() */


/***********************************************************************
*                                                                      *
*     vcdflt - Virtual cell (sensor) defaults                          *
*        (Call args are ptr to new vcell block and source id name)     *
*                                                                      *
*  N.B.  There are no defaults for vcplx, vcply because the default is *
*  that plotting is turned off.  Defaults for nsxl,nsya,tla,tll depend *
*  on info that may not be known at vcdflt() time--these are filled in *
*  at d3schk() time.  vcmin,vcmax are irrelevant for VJ,VT,VW but are  *
*  harmless.                                                           *
***********************************************************************/

void vcdflt(struct VCELL *pvc, char *vsrcrid) {

   /* Store vsrcrid name in VCELL block.  Long name handle
   *  for USRSRC will be stored later, when read in d3grp1().  */
   strncpy(pvc->vcname, vsrcrid, LSNAME);

   /* Store sensor type code */
   pvc->vtype = getsnsid(vsrcrid);

   /* Defaults for these variables are type-specific, see d3schk() */
   pvc->nsxl = pvc->nsya = -1;
   pvc->tla = pvc->tll = -1;

   /* Set remaining defaults */
   pvc->vcmin = 0.0;                /* Minimum value */
   pvc->vcmax = 1.0;                /* Maximum value */
   pvc->vcplw = 1.0;                /* Plot width    */
   pvc->vcplh = 0.25;               /* Plot height   */
   pvc->vhw  = 2.0;                 /* Half width    */
   pvc->vcpt = -1;                  /* Use global default later */

   } /* End vcdflt() */


/***********************************************************************
*                                                                      *
*     terdflt - defaults for VP FBset controls (test code)             *
*        (Call arg is pointer to new TERNARY block)                    *
*                                                                      *
***********************************************************************/

void terdflt(struct TERNARY *pvp) {

   pvp->modscl = JFIXP(1.0, FBsc);
   pvp->bckscl = JFIXP(1.0, FBsc);
   pvp->bckgnd = SI32_SGN;

   } /* End terdflt() */


/***********************************************************************
*                                                                      *
*     wdwdflt - defaults for new window                                *
*        (Call arg is pointer to new WDWDEF)                           *
*                                                                      *
***********************************************************************/

void wdwdflt(struct WDWDEF *pw) {

   pw->wx = 3.0;
   pw->wy = 14.0;
   pw->wwd = 10.0;
   pw->wht = 12.0;
   pw->kchng = WDWT;
   pw->sclx = 4.0;
   pw->sclr = 7.5;
   strcpy(pw->wcolr,"DEFAULT");

   } /* End wdwdflt() */
