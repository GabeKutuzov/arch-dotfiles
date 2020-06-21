/* (c) Copyright 1989-2017, The Rockefeller University *21115* */
/* $Id: d3dprt.c 77 2018-03-15 21:08:14Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                               d3dprt                                 *
*                                                                      *
*       Perform detail print/plot for one cell (serial version)        *
*   The cell type and cell number are contained in the CDAT global     *
*                                                                      *
************************************************************************
*                                                                      *
*  Usage: void d3dprt(void)                                            *
*                                                                      *
*  1. The first line always contains the cell number, coordinates,     *
*     s(i), u(i) and phase (if present).                               *
*  2. Subsequent lines are packed with 12-column fields containing     *
*     D,N,P,SBAR,QBAR,PSI,W,DST if present, then Izhikevich a,b,c,d    *
*     if requested.                                                    *
*  3. Then come individual "raw" (scaled only), persistence, and       *
*     "cooked" (scaled, capped, decayed, phased, voltage multiplied,   *
*     etc.) terms, first excitatory, then inhibitory, for (a) normal,  *
*     (b) geometric, and (c) modulatory connections, then for (d)      *
*     conductances.  (a) are phase distributions if phasing is in      *
*     effect, otherwise single terms.  Persistence has ndal terms,     *
*     others are always single terms.                                  *
*     This section is omitted if OMIT-TYPE-DETAIL is in effect.        *
*  4. Then come totals by connection class [ACGMSVT][EHQS]. These are  *
*     also either single terms or phase distributions, as required.    *
*  5. The selected connection-level data then follow.                  *
*                                                                      *
*  Data are normally formatted in lines of 132 columns, but this is    *
*  reduced to 80 columns with PRINT=N (Narrow) option, plus one for    *
*  a carriage control character in either case.                        *
************************************************************************
*  Written by George N. Reeke                                          *
*  V4A, 02/17/89  Translated to 'C' from version V3A                   *
*  Rev, 12/08/89, JMS - Break up Lij, Cij, Mij printout by conntype    *
*  Rev, 02/23/90, GNR - Fix for parallel gconn data structures         *
*  Rev, 10/23/91, GNR - Merge in changes from d3pdpz.c, handle phase   *
*  Rev, 12/24/91, GNR - Fix handling of depression, refractory period  *
*  Rev, 07/14/92, ABP - Fix reference to packed memory, to use d3gtxx  *
*  Rev, 08/01/92, GNR - Add baseline Cij, rm SPOUT, fix phase overprnt *
*  Rev, 08/04/92, GNR - Remove access to Lij when not defined          *
*  Rev, 08/26/92, GNR - Print colored Sj as three octal digits         *
*  V6D, 02/08/94, GNR - Add Dij and DPALL options, V7B add DPDIDL      *
*  V7C, 12/21/94, GNR - Print raw a(k) if != a(k)                      *
*  V8A, 04/30/96, GNR - New scheme for storing sums and phase dists    *
*  Rev, 05/07/97, GNR - Handle connection skips with optimize storage  *
*  Rev, 12/27/97, GNR - Add ADECAY, PPF, PPFT                          *
*  V8B, 01/06/01, GNR - Implement detailed print via ilst package      *
*  V8C, 02/27/03, GNR - Cell responses in millivolts, add conductances *
*  V8D, 05/28/06, GNR - New scheme for accessing individual type data  *
*  Rev, 07/01/06, GNR - Move Lij, Cij, Sj generation to new routines   *
*  Rev, 12/19/06, GNR - Add delta(Cij) output                          *
*  Rev, 09/30/07, GNR - Add Rij                                        *
*  Rev, 12/26/07, GNR - Add QBAR                                       *
*  ==>, 12/29/07, GNR - Last mod before committing to svn repository   *
*  V8E, 01/31/09, GNR - Add Izhikevich u and a,b,c,d                   *
*  V8G, 08/18/10, GNR - Add autoscale print                            *
*  V8H, 02/28/11, GNR - Add vmvt print for KAM=C                       *
*  Rev, 05/29/12, GNR - Add AffD64 data                                *
*  Rev, 09/04/12, GNR - Add Psi data                                   *
*  Rev, 07/20/13, GNR - New kaff scheme encodes self/nonself conntypes *
*  Rev, 08/21/13, GNR - Revise type-dep detail print to all 32-bit     *
*  Rev, 08/22/14, GNR - Si only EXP decay, remove DECAYDEF from DCYDFLT*
*  Rev, 10/20/15, GNR - Revise indiv comp print for ALPHA,DBLE decay   *
*  R67, 10/11/16, GNR - Add DPTERM output, handle DPOOB better         *
*  R74, 06/20/17, GNR - Handle all BM_xxx image modes in Sjraw output  *
*  R76, 11/07/17, GNR - Remove NORM_SUMS, add VdSq, VdSh, VSSq, VSSh   *
*  Rev, 12/11/17, GNR - Add Vm to first line print                     *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "d3global.h"
#include "lijarg.h"
#include "celldata.h"
#include "rocks.h"
#include "rkxtra.h"
#include "rkarith.h"
#include "bapkg.h"
#include "plots.h"

extern struct CELLDATA CDAT;
extern struct LIJARG   Lija;

/* Following definitions do not include carriage-control character.
*  N.B.  Widths may also be encoded literally in format strings.  */
#define DPITEMWIDTH   12   /* Width of first set of printed items */
#define MAXNORMCOLS  120   /* Maximum normal column width */
#define MAXNARRCOLS   80   /* Maximum narrow column width */
#if MAXNORMCOLS + 1 > LNSIZE || MAXNARRCOLS + 1 > LNSIZE
#error D3dprt: MAXNORMCOLS or MAXNARRCOLS exceeds OS limit
#endif

/* Allow phase dists to exceed MAXNORMCOLS up to LNSIZE so we can
*  fit 16 on a line, which looks better than 12+12+6 on 3 lines */
#define DPDISTWIDTH    8   /* Width of item in phase dist */
#define DPHEADWIDTH    4   /* Width of phase dist label */
#define MAXNORMDITM   16   /* Maximum distribution items/normal line */
#define MAXNARRDITM    8   /* Maximum distribution items/narrow line */
#if MAXNORMDITM*DPDISTWIDTH + DPHEADWIDTH > (LNSIZE-1) || \
    MAXNARRDITM*DPDISTWIDTH + DPHEADWIDTH > MAXNARRCOLS
#error D3dprt: MAXNORMDITM or MAXNARRDITM exceeds line width
#endif

#define DPCONNWIDTH    8   /* Width of item in connection data */
#define DPPHIWIDTH     4   /* Width of phij in connection data */
#define DPCOLWIDTH    12   /* Width of color info in conn data */

/* Globals for format control */
static int lpha;           /* Items in line of phase dist */
static int jcol;           /* Column or offset of next item */
static int mcol;           /* Maximum cols per line + 2 */
static char *text;         /* Space for sconvrt() */


/*=====================================================================*
*                              testput()                               *
*  This routine outputs a label and a value to the current line.       *
*  When the line is full, it is printed and a new line is started.     *
*  Each item is assumed to have printed width DPITEMWIDTH.             *
*  Arguments:                                                          *
*     fmt      Format string for call to sconvrt (begin with 'T')      *
*     val      Pointer to item to be printed                           *
*=====================================================================*/

static void testput(char *fmt, void *val) {

   if (jcol + DPITEMWIDTH > mcol) {
      cryout(RK_P4, text, (RK_LN1-1) + jcol, NULL); jcol = 2; }

   sconvrt(text, fmt, &jcol, val, NULL);
   jcol += DPITEMWIDTH;

   } /* End testput() */


/*=====================================================================*
*                              affput()                                *
*  This routine outputs a label and an afference item, which can be a  *
*  single value (if the item has phase) or a full phase distribution   *
*  (if the item does have phase). When the line is full, it is printed *
*  and a new line is started.                                          *
*  Arguments:                                                          *
*     id       Four characters indicating (1) 'R' for "raw", 'W' for   *
*              "persistence", 'G' and 'H' for type ALPHA or DBLE decay,*
*              'B' for "bandsum", 'F' for "final", (2) 'A', 'G', 'M',  *
*              or 'C' for specific, geometric, modul, or conductance   *
*              afferent, (3) a blank space where the ict is inserted,  *
*              and (4) 'E' for excitatory or 'I' for inhibitory.       *
*     pdist    Pointer to item or phase distribution                   *
*     index    Index of current connection type                        *
*     kpha     +APph (1) if item has phase, +AP64 (2) if item is si64  *
*=====================================================================*/

static void affput(char *id, void *pdist, int index, int kpha) {

#define Lbfmt 26              /* Length of base format */
#define Lffmt 32              /* Length of phase format */
#define Lvfmt  4              /* Length of variable part of format */
#define APph   1              /* Flag for items with phase */
#define AP64   2              /* Flag for 64-bit items */

   char fmt[Lffmt];
   static char bfmt[Lbfmt] = "  (T,4HRAIEB20/27IL7.<4X)";
   static char ffmt[] = "(P4X,4HRAIE#" qPHR "|R(B20/27IL8.<4))";

   if (kpha & APph) memcpy(fmt, ffmt, Lffmt);
   else             memcpy(fmt, bfmt, Lbfmt);
   memcpy(fmt+7, id, Lvfmt);
   if (index >= 10) {
      if (index >= 100)       /* (Unusual case) */
         wbcdwt(&index, fmt+7, RK_IORF+RK_NINT+2);
      else
         wbcdwt(&index, fmt+8, RK_IORF+RK_NINT+1);
      }
   else
      fmt[9] = '0' + index;

   if (kpha & APph) {         /* Got phase */
      if (jcol > 2)
         cryout(RK_P4, text, (RK_LN1-1) + jcol, NULL);
      jcol = 2;
      fmt[24] = kpha & AP64 ? 'W' : 'J';
      convrt(fmt, &lpha, pdist, NULL);
      } /* End phase dist print */
   else {                     /* Single item */
      fmt[18] = kpha & AP64 ? 'W' : 'J';
      testput(fmt+2, pdist);
      }

   } /* End affput() */


/*=====================================================================*
*                              d3dprt()                                *
*=====================================================================*/

void d3dprt(void) {

   struct REPBLOCK *ir;    /* Ptr to current repertoire */
   struct CELLTYPE *il;    /* Ptr to current cell type block */
   struct CONNTYPE *ix;    /* Ptr to current conn block */
   struct INHIBBLK *ib;    /* Ptr to current inhib block*/
   struct MODBY    *im;    /* Ptr to current mod block  */
   struct CONDUCT  *ic;    /* Ptr to conductance block */

   si32 *pd32,*pd32inh;    /* Ptr to type-detail items */
   si32 pitm;              /* Item to be printed */
   int  jct;               /* Connection type index */
   int  kphar,kphav;       /* Phase switches */
   ui16 taitms;            /* Codes for connection items */
   char ppftoff[DPCONNWIDTH]; /* Text for PPFT timers */

   /* Table to control selection and labels of terms in final totals */
   struct TotCtrl {
      char Head[4];              /* Label identifier */
      byte Koff;                 /* Offset in kaff array */
      byte Mask;                 /* kaff bit selector */
      };
   static struct TotCtrl TotPrint[] = {
      { "SeEx", SGM_Self,       AFS_Excit  },
      { "SeHp", SGM_Self,       AFS_Hyper  },
      { "CdEx", SGM_Conduct,    AFF_Excit  },
      { "CdHp", SGM_Conduct,    AFF_Hyper  },
      { "SpEx", SGM_Specific,   AFF_Excit  },
      { "SpHp", SGM_Specific,   AFF_Hyper  },
      { "SpSq", SGM_Specific,   AFF_Square },
      { "SpSh", SGM_Specific,   AFF_Shunt  },
      { "SSEx", SGM_Specific,   AFS_Excit  },
      { "SSHp", SGM_Specific,   AFS_Hyper  },
      { "SSSq", SGM_Specific,   AFS_Square },
      { "SSSh", SGM_Specific,   AFS_Shunt  },
      { "VdEx", SGM_VoltDep,    AFF_Excit  },
      { "VdHp", SGM_VoltDep,    AFF_Hyper  },
      { "VdSq", SGM_VoltDep,    AFF_Square },
      { "VdSh", SGM_VoltDep,    AFF_Shunt  },
      { "VSEx", SGM_VoltDep,    AFS_Excit  },
      { "VSHp", SGM_VoltDep,    AFS_Hyper  },
      { "VSSq", SGM_VoltDep,    AFS_Square },
      { "VSSh", SGM_VoltDep,    AFS_Shunt  },
      { "GmEx", SGM_Geometric,  AFF_Excit  },
      { "GmHp", SGM_Geometric,  AFF_Hyper  },
      { "GmSq", SGM_Geometric,  AFF_Square },
      { "GmSh", SGM_Geometric,  AFF_Shunt  },
      { "GSEx", SGM_Geometric,  AFS_Excit  },
      { "GSHp", SGM_Geometric,  AFS_Hyper  },
      { "GSSq", SGM_Geometric,  AFS_Square },
      { "GSSh", SGM_Geometric,  AFS_Shunt  },
      { "MoEx", SGM_Modulatory, AFF_Excit  },
      { "MoHp", SGM_Modulatory, AFF_Hyper  },
      { "MoSq", SGM_Modulatory, AFF_Square },
      { "MoSh", SGM_Modulatory, AFF_Shunt  },
      { "MSEx", SGM_Modulatory, AFS_Excit  },
      { "MSHp", SGM_Modulatory, AFS_Hyper  },
      { "MSSq", SGM_Modulatory, AFS_Square },
      { "MSSh", SGM_Modulatory, AFS_Shunt  },
      { "TotE", SGM_Total,      AFF_Excit  },
      { "TotH", SGM_Total,      AFF_Hyper  },
      { "TotQ", SGM_Total,      AFF_Square },
      { "TotS", SGM_Total,      AFF_Shunt  },
      { "GTot", SGM_Total,    AFF_CellResp }, };

/* Pick up pointers to repertoire and cell blocks */

   il = CDAT.cdlayr;
   ir = il->pback;

/* Initialization for testput and affput routines */

   if (il->dpitem & DPC80)
      lpha = MAXNARRDITM, mcol = MAXNARRCOLS+2;
   else
      lpha = MAXNORMDITM, mcol = MAXNORMCOLS+2;
   /* Space for assembling up to two lines of text */
   text = (char *)CDAT.psws;
   text[0] = ' ';
   memset(ppftoff, ' ', DPCONNWIDTH-2);
   ppftoff[DPCONNWIDTH-2] = ppftoff[DPCONNWIDTH-1] = '-';
   /* Turn off constant items if not DPALL */
   taitms = il->dpitem;
   if ((taitms & (DPALL|DPDIDL)) == DPDIDL)
      taitms &= ~(DPLIJ|DPDIJ|DPABCD);

/*---------------------------------------------------------------------*
*  Cell number, position, state, and phase                             *
*---------------------------------------------------------------------*/

   convrt("(P4,4H0==>2*A" qLSN ",' CELL ',J2UJ10,'AT',UH4,', ',JUH4,"
      "5H Vm= J0B20/27IJ8.<4,6H, Si= J0B20/27IJ8.<4)", ir->sname,
      il->lname, &CDAT.cdcell, &CDAT.groupx, &CDAT.groupy,
      &CDAT.new_vm, &CDAT.new_si, NULL);
   /* These will continue the previous line because they do not
   *  begin with carriage control characters.  */
   if (il->Ct.rspmeth == RF_BREG)
      convrt("(P4,5H, W= J0B20/27IJ8.<4)", &CDAT.izhu, NULL);
   if (il->Ct.rspmeth >= RF_IZH3)
      convrt("(P4,5H, U= J0B22/29IJ8.<4)", &CDAT.izhu, NULL);
   if (il->ctf & (CTFI3IVC|CTFI3IVR))
      convrt("(P4,5H, Vr=J0B20/27IJ8.<4)", &CDAT.i3vr, NULL);
   if (il->phshft) convrt("(P4,6H, PHI=IC3)", &CDAT.new_phi, NULL);

/*---------------------------------------------------------------------*
*  Response function components                                        *
*---------------------------------------------------------------------*/

   cryout(RK_P4, " Response function components:",
      RK_LN1+RK_NFSUBTTL, NULL);
   jcol = 2;

   /* Izhikevich a,b,c,d (only if variable).
   *  We want to print the actual values used in d3go, so these are
   *  passed to us via CDAT.  If cell spiked, only c,d are computed
   *  and used, otherwise a,b.  */
   if (taitms & DPABCD && il->Ct.rspmeth >= RF_IZH3) {
      struct IZHICOM *piz = (struct IZHICOM *)il->prfi;
      if (CDAT.events & SPIKED) {
         if (piz->izrc) testput("(T,' C=',B20/27IJ8.<5X)", &CDAT.izhc);
         if (piz->izrd) testput("(T,' D=',B20/27IJ8.<5X)", &CDAT.izhd);
         }
      else {
         if (piz->izra) testput("(T,' A=',B28IJ8.<5X)", &CDAT.izha);
         if (piz->izrb) testput("(T,' B=',B28IJ8.<5X)", &CDAT.izhb);
         }
      }

   if (il->ctf & CTFDN)             /* Depression */
      testput("(T,' D=',B8IJ8.4X)", &CDAT.depr);

   if (CDAT.noise)                  /* Noise */
      testput("(T,' N=',B20/27IJ8.<4X)", &CDAT.noise);

   if (CDAT.nmodval != il->No.nfr)  /* Noise fraction */
      testput("(T,' Nf=',B24IJ7.5X)", &CDAT.nmodval);

   if (CDAT.events & PROBED)        /* Probe */
      testput("(T,' Pr=',B20/27IJ7.<4X)", &CDAT.probeval);

   if (ctexists(il,SBAR))           /* Sbar */
      testput("(T,' Sb=',B7/14IJ7.<4X)", &CDAT.sbar);

   if (ctexists(il,QBAR))           /* Qbar */
      testput("(T,' Qb=',B7/14IJ7.<4X)", &CDAT.qbar);

   if (il->orkam & KAMLP)           /* 1 - Psi */
      testput("(T,' Ar=',B15IJ7.<4X)", &CDAT.ompsi);

   if (CDAT.events & DECAYED) {     /* Persistence */
      testput("(T, ' W=',B20/27IJ8.<4X)", &CDAT.pers);
      }

   if (il->prfrc) {                 /* DST term */
      if (CDAT.dst < DST_CLK0S16) { /* DST encodes refractory time */
         pitm = DST_CLK0S16 - CDAT.dst;
         testput("(T,' RT=',B16IJ7X)", &pitm); }
      else                          /* DST is delta spike threshold */
         testput("(T,' Dt=',B20/27IJ7.<4X)", &CDAT.dst);
      }

/* Flush output line */

   if (jcol > 2) cryout(RK_P4, text, (RK_LN1-1)+jcol, NULL);

/*---------------------------------------------------------------------*
*  Optional raw, decay, and final single terms or phase distributions  *
*---------------------------------------------------------------------*/

   if (RP->CP.runflags & RP_OPTOTD) goto OmitTypeDetail;

   cryout(RK_P4, " Individual input components:",
      RK_LN1+RK_NFSUBTTL, NULL);
   jcol = 2;

   /* Print specific connection type data */
   for (ix=il->pct1; ix; ix=ix->pct) {
      if (ix->kgfl & KGNBP) continue;
      jct = ix->ict;
      kphar = (ix->phopt & (PHASJ|PHASR)) != 0;
      kphav =
         (ix->phopt & (PHASJ|PHASR|PHASCNV|PHASVDU)) != 0;
      pd32 = pd32inh = CDAT.pAffD32 + ix->odpafs + kphar;
      if (ix->sssck & PSP_POS) {                         /* Excit */
         affput("RA1E", pd32, jct, kphar);
         pd32 += ix->npafraw - kphar;
         if (qdecay(&ix->Cn.ADCY)) {
            if (ix->Cn.ADCY.kdcy >= DCYALPH) {
               affput("GA1E", pd32++, jct, 0);
               affput("HA1E", pd32++, jct, 0);
               }
            else
               affput("WA1E", pd32++, jct, 0);
            }
         affput("FA1E", pd32, jct, kphav);
         pd32inh += ix->npaftot;
         }
      if (ix->sssck & PSP_NEG) {                         /* Inhib */
         affput("RA1I", pd32inh, jct, kphar);
         pd32inh += ix->npafraw - kphar;
         if (qdecay(&ix->Cn.ADCY)) {
            if (ix->Cn.ADCY.kdcy >= DCYALPH) {
               affput("GA1I", pd32inh++, jct, 0);
               affput("HA1I", pd32inh++, jct, 0);
               }
            else
               affput("WA1I", pd32inh++, jct, 0);
            }
         affput("FA1I", pd32inh, jct, kphav);
         }
      if (ix->Cn.kam & KAMCG)
         testput("(T,' V=',B" qqv(FBvl) "IJ8.5X)",
            &CDAT.pconnd[ix->ocdat].vmvt);
      } /* End ix loop */

/* Loop over geometric types, printing contributions */

   for (ib=il->pib1; ib; ib=ib->pib) {
      ui32 jx;
      jct = ib->igt;
      for (jx=0; jx<ib->nib; jx++) {                     /* Bandsums */
         si64 *kx = &(ib->inhbnd[jx].bandsum[CDAT.groupn]);
         if (jx > 0) testput("(TB20/27IW11.<4X)", kx);
         else        affput("BG1 ", kx, jct, AP64);
         } /* End loop over bands */
      pd32 = CDAT.pAffD32 + ib->ogafs;
      if (ib->gssck & PSP_POS) {                         /* Excit */
         affput("RG1E", pd32++, jct, 0);
         if (qdecay(&ib->GDCY)) {
            if (ib->GDCY.kdcy >= DCYALPH) {
               affput("GG1E", pd32++, jct, 0);
               affput("HG1E", pd32++, jct, 0);
               }
            else
               affput("WG1E", pd32++, jct, 0);
            }
         affput("FG1E", pd32++, jct, 0); }
      if (ib->gssck & PSP_NEG) {                         /* Inhib */
         affput("RG1I", pd32++, jct, 0);
         if (qdecay(&ib->GDCY)) {
            if (ib->GDCY.kdcy >= DCYALPH) {
               affput("GG1I", pd32++, jct, 0);
               affput("HG1I", pd32++, jct, 0);
               }
            else
               affput("WG1I", pd32++, jct, 0);
            }
         affput("FG1I", pd32++, jct, 0); }
      } /* End inhibblk loop */

/* Loop over modulation types, printing contributions */

   for (im=il->pmby1; im; im=im->pmby) {
      if (!(im->mssck & (PSP_POS|PSP_NEG))) continue;
      jct = im->mct;
      pd32 = CDAT.pAffD32 + im->omafs;
      if (*pd32 >= 0) {                                  /* Excit */
         affput("RM1E", pd32++, jct, 0);
         if (qdecay(&im->MDCY)) {
            if (im->MDCY.kdcy >= DCYALPH) {
               affput("GM1E", pd32++, jct, 0);
               affput("HM1E", pd32++, jct, 0);
               }
            else
               affput("WM1E", pd32++, jct, 0);
            }
         affput("FM1E", pd32++, jct, 0); }
      else {                                             /* Inhib */
         affput("RM1I", pd32++, jct, 0);
         if (qdecay(&im->MDCY)) {
            if (im->MDCY.kdcy >= DCYALPH) {
               affput("GM1I", pd32++, jct, 0);
               affput("HM1I", pd32++, jct, 0);
               }
            else
               affput("WM1I", pd32++, jct, 0);
            }
         affput("FM1I", pd32, jct, 0); }
      } /* End im loop */

/* Loop over conductances, printing contributions */

   for (ic=il->pcnd1,jct=1; ic; ic=ic->pncd,++jct) {
      pd32 = CDAT.pAffD32 + ic->ocafs;
      if (*pd32 >= 0) {                                 /* Excit */
         affput("RC1E", pd32++, jct, 0);
         if (qdecay(&ic->gDCY)) {
            if (ic->gDCY.kdcy >= DCYALPH) {
               affput("GC1E", pd32++, jct, 0);
               affput("HC1E", pd32++, jct, 0);
               }
            else
               affput("WC1E", pd32++, jct, 0);
            }
         affput("FC1E", pd32++, jct, 0); }
      else {                                             /* Inhib */
         affput("RC1I", pd32++, jct, 0);
         if (qdecay(&ic->gDCY)) {
            if (ic->gDCY.kdcy >= DCYALPH) {
               affput("GC1I", pd32++, jct, 0);
               affput("HC1I", pd32++, jct, 0);
               }
            else
               affput("WC1I", pd32++, jct, 0);
            }
         affput("FC1I", pd32, jct, 0); }
      } /* End ic loop */

/* Flush output line */

   if (jcol > 2) cryout(RK_P4, text, (RK_LN1-1)+jcol, NULL);

OmitTypeDetail: ;

/*---------------------------------------------------------------------*
*  Totals by connection class                                          *
*---------------------------------------------------------------------*/

   cryout(RK_P4,
      " Total inputs by connection class", RK_LN1+RK_NFSUBTTL,
      " (Max bin normalized)", (il->pctpd && il->pctpd->phops &
      PHOPM) ? RK_CCL : RK_SKIP, ":", 1, NULL);
   jcol = 2;

/* Terms are printed in order and with labels given in tables above */

   for (jct=0; jct<sizeof(TotPrint)/sizeof(struct TotCtrl); jct++) {
      int kkoff = (int)TotPrint[jct].Koff;
      byte kmask = TotPrint[jct].Mask;
      if (il->kaff[kkoff] & kmask) {
         si64 *pd64 = CDAT.pAffD64 + getoas(il, kkoff, kmask);
         if (il->phshft) {
            convrt("(P4XA4#" qPHR "|R(B20/27IW8.<4))",
               TotPrint[jct].Head, &lpha, pd64, NULL);
            jcol = 2; }
         else {
            if (jcol + DPITEMWIDTH > mcol) {
               cryout(RK_P4, text, (RK_LN1-1) + jcol, NULL);
               jcol = 2; }
            sconvrt(text, "(TA4B20/27IW7.<4X)", &jcol,
               TotPrint[jct].Head, pd64, NULL);
            jcol += DPITEMWIDTH; }
         }
      } /* End loop over response function terms */

/* Flush output line */

   if (jcol > 2) cryout(RK_P4, text, (RK_LN1-1)+jcol, NULL);

/*---------------------------------------------------------------------*
*                     Autoscale adjustment factor                      *
*---------------------------------------------------------------------*/

   if (il->kautu) {
      int na = (int)il->nauts;
      convrt("(P4,' Autoscale multiplier:',#,2(1XJ0B24IJ8.<5))",
         &na, il->pauts->asmul, NULL);
      }

/*---------------------------------------------------------------------*
*  Print connection-level data under control of DPITEM mask.           *
*                                                                      *
*  Note:  The move to the d3lij() family of routines allows Lij (and   *
*  Sj) to be printed even in OPTIMIZE STORAGE mode, when Lij values    *
*  are not in memory.  Also, for user convenience, changed to printing *
*  all the values for one connection before going on to the next.      *
*---------------------------------------------------------------------*/

/* Note: Here, jcol is the offset into the line.  In the code above
*  and in the FORTRAN version it was the column number (= offset+1).
*  Note (R67):  With addition of DPTERM output, we have up to 132 cols
*  (15 8-col items + 12-col Sj) which just fits LNSIZE cols.  DPC80
*  code is ignored because multi-row output would be hard to read.  */

   if (il->pct1 && taitms &
      (DPLIJ|DPDIJ|DPCIJ|DPCIJ0|DPMIJ|DPRIJ|DPPPF|DPSJ|DPDCIJ|DPTERM)) {

      struct CONNDATA *pND;         /* Ptr to connection data */
      gvin_fn sjrget;               /* Function to get Sjraw colors */
      ui32 kbcdp,kbcds;             /* wbcdwt conversion codes */
      si32 Mijtest;                 /* Test for Mij a clock */
      int doSjraw;                  /* pix size to print raw colors */
      int llc;                      /* Local copy of lc */
      int nsjc;                     /* Number of columns for Sj */
      ui16 doLij,doDij,doCij,doCij0,   /* TRUE if printing the */
         doMij,doRij,doPPF,doPPFT,     /* named variable */
         doSj,CkSj,doDCij,doTerm;

      /* Lij (except KGNHV, when lijbr returns 0), Cij, and Sj always
      *  exist and can always be accessed and printed whether or not
      *  stored, therefore, their doXij flags can be set before the
      *  conntype loop.  */
      doLij = taitms & DPLIJ;
      doCij = taitms & DPCIJ;
      doSj  = taitms & DPSJ;

      /* Prepare for access to connection variables for this cell */
      d3kiji(il, CDAT.cdcell);

/* Loop over connection types */

      for (ix=il->pct1; ix; ix=ix->pct) {
         byte svtveoff;
         if (ix->kgfl & KGNBP) continue;

         pND = CDAT.pconnd + ix->ocdat;
         llc = (int)ix->lc;
         jct = (int)ix->ict;
         svtveoff = ix->cnxc.tveoff; /* Save tveoff JIC */

         /* wbcdwt format code for Sj, Mij, Rij */
         kbcds = (ix->sjbs<<RK_BS)|4<<RK_DS|RK_AUTO|RK_IORF|
            RK_NI32|DPCONNWIDTH-1;

         /* Set up the subtitle according to the items present */
         strcpy(text, "       J");
         jcol = DPCONNWIDTH;
         d3kijx(ix);
         /* Always need Lij even if just to get value of jsyn */
         d3lijx(ix);
         if (doLij) {
            memcpy(text+jcol, "     Lij", DPCONNWIDTH);
            doLij = jcol; jcol += DPCONNWIDTH;
            } /* Assignment intended in next line */
         if (doDij = taitms & DPDIJ && ix->Cn.kdelay) {
            d3dijx(ix);
            memcpy(text+jcol, "     Dij", DPCONNWIDTH);
            doDij = jcol; jcol += DPCONNWIDTH;
            }
         if (doCij) {
            d3cijx(ix);
            memcpy(text+jcol, "     Cij", DPCONNWIDTH);
            doCij = jcol; jcol += DPCONNWIDTH;
            }  /* Assignment intended in next line */
         if (doCij0 = taitms & DPCIJ0 && cnexists(ix,CIJ0)) {
            memcpy(text+jcol, "    Cij0", DPCONNWIDTH);
            doCij0 = jcol; jcol += DPCONNWIDTH;
            }  /* Assignment intended in next line */
         if (doMij = taitms & DPMIJ && cnexists(ix,MIJ)) {
            memcpy(text+jcol, "     Mij", DPCONNWIDTH);
            Mijtest = (si32)ix->Cn.mticks - S15;
            doMij = jcol; jcol += DPCONNWIDTH;
            }  /* Assignment intended in next line */
         if (doRij = taitms & DPRIJ && cnexists(ix, RBAR)) {
            memcpy(text+jcol, "    <Sj>", DPCONNWIDTH);
            doRij = jcol; jcol += DPCONNWIDTH;
            }  /* Assignment intended in next line */
         if (doDCij = taitms & (DPDCIJ|DPTERM) &&
               ix->finbnow != NO_FIN_AMP) {
            memcpy(text+jcol, "  DelCij", DPCONNWIDTH);
            doDCij = jcol; jcol += DPCONNWIDTH;
            }  /* Assignment intended in next line */
         if (doPPF = taitms & DPPPF && cnexists(ix,PPF)) {
            memcpy(text+jcol, "     PPF", DPCONNWIDTH);
            doPPF = jcol; jcol += DPCONNWIDTH;
            }  /* Assignment intended in next line */
         if (doPPFT = taitms & DPPPF && cnexists(ix,PPFT)) {
            memcpy(text+jcol, " PPFTime", DPCONNWIDTH);
            kbcdp = (ix->PP->ppfopt & PPFABR) ?
               (RK_BSCL+2*RK_D+RK_GFP+RK_IORF+DPCONNWIDTH-2) :
               (2*RK_D+RK_GFP+RK_IORF+DPCONNWIDTH-2);
            doPPFT = jcol; jcol += DPCONNWIDTH;
            }
         if (doSj) {
            int rmode = ix->cnxc.tvmode & BM_MASK;
            d3sjx(ix);
            doSjraw = rmode >= BM_C48 ? bmpxsz(rmode) : 0;
            if (ix->phopt & (PHASJ|PHASR|PHASC)) {
               nsjc = DPCONNWIDTH + DPPHIWIDTH;
               memcpy(text+jcol, "      Sj Phi", nsjc); }
            else if (doSjraw) {
               /* Note special source code DP_SRC here to get gvin_fn
               *  that picks a color, regardless of ix->cnxc.tvcsel */
               sjrget = d3gvin(DP_SRC, &ix->cnxc);
               nsjc = DPCONNWIDTH + DPCOLWIDTH;
               memcpy(text+jcol, "      Sj   R   G   B", nsjc); }
            else {
               nsjc = DPCONNWIDTH;
               memcpy(text+jcol, "      Sj", nsjc); }
            doSj = jcol; jcol += nsjc;
            }  /* Assignment intended in next line */
         if (doTerm = taitms & DPTERM && ix->finbnow != NO_FIN_AMP) {
            memcpy(text+jcol, "  Si-mti  Mj-mtj  Vk-mtv  Wayscl"
               " Product  Change", 6*DPCONNWIDTH);
            doTerm = jcol; jcol += 6*DPCONNWIDTH;
            }
         /* Just a little safety check */
         if (jcol > LNSIZE)
            d3exit(fmturlnm(il), DPLLEN_ERR, jct);
         sconvrt(text+jcol, "(' Connection data for cell ',J0UJ8,"
            "', conntype ',J0I4)", &CDAT.cdcell, &jct, NULL);
         cryout(RK_P4, text+jcol, RK_NFSUBTTL+RK_LN2,
            text, RK_LN0+jcol, NULL);

/* Loop over synapses */

         /* Caution:  pND is incremented through all connections in
         *  d3go, and so must be also in detail print, even when we
         *  are skipping printing out-of-bounds connections.  */
         for (Lija.isyn=0; Lija.isyn<Lija.nuk;
               ++Lija.isyn,Lija.psyn+=llc,++pND) {

            /* Keep everything in phase in case skipping Sj OOB */
            Lija.lijbr(ix);
            if (doDij) ix->dijbr(ix);
            if (doCij) ix->cijbr(ix);
            /* Omit the whole show if Sj out-of-bounds
            *  and option to omit (DPOOB) was selected.  */
            if (doSj && !(CkSj = ix->sjbr(ix)) &&
               taitms & DPOOB) continue;

            /* Print Lij */
            if (doLij) {
               wbcdwt(&Lija.lijval, text+doLij,
                  RK_IORF+RK_NLONG+DPCONNWIDTH-1);
               }

            /* Print Dij (delay) */
            if (doDij) {
               wbcdwt(&Lija.dijval, text+doDij,
                  RK_IORF+RK_NI32+DPCONNWIDTH-1);
               }

            /* Print Cij */
            if (doCij) {
               wbcdwt(&Lija.cijsgn, text+doCij, RK_MZERO+(31<<RK_BS)+
                  (4<<RK_DS)+RK_IORF+RK_NI32+DPCONNWIDTH-1);
               }

            /* Print baseline Cij (Cij0) */
            if (doCij0) {
               si32 cij0 = pND->new_cij0 & ix->cmsk;
               wbcdwt(&cij0, text+doCij0, RK_MZERO+(31<<RK_BS)+
                  (4<<RK_DS)+RK_IORF+RK_NI32+DPCONNWIDTH-1);
               }

            /* Print Mij */
            if (doMij) {
               d3gtjs2(pitm, Lija.psyn+ix->cnoff[MIJ]);
               if (pitm <= Mijtest) {
                  pitm += S15;
                  wbcdwt(&pitm, text+doMij,
                     RK_IORF+RK_NI32+DPCONNWIDTH-1);
                  text[doMij+(DPCONNWIDTH-2)-RK.length] = 'T'; }
               else
                  wbcdwt(&pitm, text+doMij, kbcds);
               }

            /* Print Rij */
            if (doRij) {
               d3gtjs2(pitm, Lija.psyn+ix->cnoff[RBAR]);
               wbcdwt(&pitm, text+doRij, kbcds);
               }

            /* Print delta(Cij) */
            if (doDCij) {
               wbcdwt(&pND->delCij, text+doDCij, (FBCij<<RK_BS)+
                  (4<<RK_DS)+RK_IORF+RK_NI32+DPCONNWIDTH-1);
               }

            /* Print PPF */
            if (doPPF) {
               d3gtjs2(pitm, Lija.psyn+ix->cnoff[PPF]);
               wbcdwt(&pitm, text+doPPF, 12*RK_BSCL+4*RK_D+RK_IORF+
                  RK_NI32+DPCONNWIDTH-1);
               }

            /* Print PPFT */
            if (doPPFT) {
               d3gtjs2(pitm, Lija.psyn+ix->cnoff[PPFT]);
               if (pitm == 0)
                  memcpy(text+doPPFT, ppftoff, DPCONNWIDTH);
               else {
                  si32 aitm = abs32(pitm);
                  wbcdwt(&aitm, text+doPPFT, kbcdp);
                  text[doPPFT+DPCONNWIDTH-1] = pitm > 0 ? 'D' : 'R';
                  }
               }

            /* Print Sj.  There are two optional components:
            *  phase or raw color information, but never both.  */
            if (doSj) {
               char *tSj = text + doSj;
               if (CkSj) {
                  /* Sj as used */
                  wbcdwt(&Lija.sjval, tSj, kbcds);
                  /* Optional phase */
                  if (ix->phopt & (PHASJ|PHASR|PHASC)) {
                     int phi = ix->pjbr(ix);
                     wbcdwt(&phi, tSj+DPCONNWIDTH,
                        RK_IORF+DPPHIWIDTH-1);
                     }
                  /* Optional raw color in R-G-B format */
                  else if (doSjraw) {
                     struct EXTRCOLR *pxc = &ix->cnxc;
                     byte *psj;        /* Ptr to Sj */
                     long Lij = Lija.lijval;
                     si32 rgb;
                     int ec;
                     if (ix->kgfl & (KGNIA|KGNPB))
                        Lij += ix->osrc;
                     else
                        Lij *= doSjraw;
                     psj = ix->srcorg + Lij;
                     tSj += DPCONNWIDTH;
                     for (ec=0; ec<=2; tSj+=4,++ec) {
                        pxc->tveoff = (byte)ec;
                        rgb = 100*sjrget(psj, pxc);
                        wbcdwt(&rgb, tSj, FBim<<RK_BS|RK_IORF|RK_NI32+3);
                        }
                     }
                  }
               else {
                  /* Otherwise, if sj bad, print dashes */
                  memcpy(tSj, "    ---- --- --- ---", nsjc);
                  }
               } /* End print Sj data */

            /* Print detailed amplification information */
            if (doTerm) {
               sconvrt(text+doTerm, "(2*B7/14IJ8.<4,B" qqv(FBvl)
                  "IJ8.<4,B10IJ8.<4,2*B16IJ8.<4)",
                  &pND->simmti, &pND->mjmmtj, &pND->vmvt,
                  &pND->wayscl, &pND->Cijprod, &pND->Cijterm, NULL);
               }

            /* Print connection number */
            wbcdwt(&Lija.jsyn, text, RK_IORF+RK_NLONG+DPCONNWIDTH-1);
            cryout(RK_P4, text, RK_LN1+jcol, NULL);
            } /* End loop over connections */
         ix->cnxc.tveoff = svtveoff;

         } /* End loop over connection types */

      /* Silently clear any subtitle left from above output */
      cryout(RK_P4, text, RK_LN1+RK_NTSUBTTL+1, NULL);

      } /* End connection-level data output */

   } /* End d3dprt() */
