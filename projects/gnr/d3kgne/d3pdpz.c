/* (c) Copyright 1991-2012 Neurosciences Research Foundation, Inc. */
/* $Id: d3pdpz.c 52 2012-06-01 19:54:05Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                               d3pdpz                                 *
*                                                                      *
*     Perform detail print/plot for all cells (zero node version)      *
*                                                                      *
************************************************************************
*                                                                      *
*  Usage: d3pdpz(void)                                                 *
*                                                                      *
*  1. The first line always contains the cell number, coordinates,     *
*     s(i) and phase (if present).                                     *
*  2. Subsequent lines are packed with 12-column fields containing     *
*     D,N,P,SBAR,QBAR,W,DST if present.                                *
*  3. Then come individual "raw" (scaled only), persistence, and       *
*     "cooked" (scaled, capped, decayed, phased, voltage multiplied,   *
*     etc.) terms, first excitatory, then inhibitory, for (a) normal,  *
*     (b) geometric, and (c) modulatory connections, then for (d)      *
*     conductances.  (a) are phase distributions if phasing is in      *
*     effect, otherwise single terms.  Others are always single terms. *
*     This section is omitted if OMIT-TYPE-DETAIL is in effect.        *
*  4. Then come totals by connection class [ACGMSVT][EHQS]. These are  *
*     also either single terms or phase distributions, as required.    *
*  5. The selected connection-level data then follow.                  *
*                                                                      *
*  Data are normally formatted in lines of 132 columns, but this is    *
*  reduced to 80 columns with PRINT=N (Narrow) option, plus one for    *
*  a carriage control character in either case.                        *
*                                                                      *
*  For parallel version, nodes responsible for selected detail print   *
*  cells use d3pdpn() to send relevant data to node zero, which uses   *
*  d3pdpz() to format the data as in the serial version.               *
*                                                                      *
*  Written by George N. Reeke                                          *
*  V4A, 02/17/89  Translated to 'C' from version V3A                   *
*  Rev, 01/04/90, JMS - Sj print format changed                        *
*  Rev, 03/01/90, GNR - Correct gconn and modby bugs, restructure      *
*                       buffer testing, send conntypes one-by-one      *
*  Rev, 04/28/91,  MC - Added PHASED cells and connection data         *
*  V5C, 10/23/91, GNR - Include s(i) always, reformat, delete plotting *
*                       and 56 column limit, fix end-of-list code      *
*  Rev, 12/24/91, GNR - Fix handling of depression, refractory period  *
*  Rev, 03/18/92, GNR - Use nnget routines                             *
*  Rev, 08/01/92, GNR - Add baseline Cij, rm SPOUT, fix phase overprnt *
*  Rev, 08/26/92, GNR - Print colored Sj as three octal digits         *
*  Rev, 08/16/93, GNR - Use findcell() to prepare for load optim.      *
*  V6D, 02/08/94, GNR - Add Dij and DPALL options, V7B add DPDIDL      *
*  V7C, 12/21/94, GNR - Print raw a(k) if != a(k)                      *
*  V8A, 05/11/96, GNR - New scheme for storing sums and phase dists    *
*  Rev, 11/28/96, GNR - Remove support for non-hybrid version          *
*  Rev, 12/27/97, GNR - Add ADECAY, PPF, PPFT                          *
*  Rev, 05/30/99, GNR - Use new swapping scheme                        *
*  V8B, 01/06/01, GNR - Implement detailed print via ilst package      *
*  V8C, 02/27/03, GNR - Cell responses in millivolts, add conductances *
*  V8D, 05/28/06, GNR - New scheme for accessing individual type data  *
*  Rev, 12/19/06, GNR - Add delta(Cij) output                          *
*  Rev, 09/30/07, GNR - Add Rij                                        *
*  Rev, 12/26/07, GNR - Add QBAR                                       *
*  ==>, 12/29/07, GNR - Last mod before committing to svn repository   *
*  V8E, 01/31/09, GNR - Add Izhikevich u and a,b,c,d                   *
*  V8G, 08/18/10, GNR - Add autoscale print                            *
*  V8H, 02/28/11, GNR - Add vmvt print for KAM=C                       *
*  Rev, 05/29/12, GNR - Add AffD64 data                                *
***********************************************************************/

#define CLTYPE struct CLBLK

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "d3global.h"
#include "nccolors.h"
#include "celldata.h"
#include "clblk.h"
#include "rocks.h"
#include "rkxtra.h"
#include "rkarith.h"
#include "bapkg.h"
#include "swap.h"

extern struct CELLDATA CDAT;

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
*              "persistence", 'B' for "bandsum", or 'F' for "final",   *
*              (2) 'A', 'G', 'M', or 'C' for specific, geometric,      *
*              modulatory, or conductance afferent, (3) a blank space  *
*              where the ict is inserted, and (4) 'E' for excitatory   *
*              or 'I' for inhibitory.                                  *
*     pdist    Pointer to item or phase distribution                   *
*     index    Index of current connection type                        *
*     kpha     +1 if item has phase, +2 if item is si64                *
*=====================================================================*/

static void affput(char *id, void *pdist, int index, int kpha) {

#define Lbfmt 26              /* Length of base format */
#define Lffmt 32              /* Length of phase format */
#define Lvfmt  4              /* Length of variable part of format */

   char fmt[Lffmt];
   static char bfmt[Lbfmt] = "  (T,4HRAIEB20/27IL7.<4X)";
   static char ffmt[] = "(P4X,4HRAIE#" qPHR "|R(B20/27IL8.<4))";

   if (kpha & 1) memcpy(fmt, ffmt, Lffmt);
   else          memcpy(fmt, bfmt, Lbfmt);
   memcpy(fmt+7, id, Lvfmt);
   if (index >= 10) {
      if (index >= 100)       /* (Unusual case) */
         wbcdwt(&index, fmt+7, RK_IORF+RK_NINT+2);
      else
         wbcdwt(&index, fmt+8, RK_IORF+RK_NINT+1);
      }
   else
      fmt[9] = '0' + index;

   if (kpha & 1) {            /* Got phase */
      if (jcol > 2)
         cryout(RK_P4, text, (RK_LN1-1) + jcol, NULL);
      jcol = 2;
      fmt[24] = kpha & 2 ? 'W' : 'L';
      convrt(fmt, &lpha, pdist, NULL);
      } /* End phase dist print */
   else {                     /* Single item */
      fmt[18] = kpha & 2 ? 'W' : 'L';
      testput(fmt+2, pdist);
      }

   } /* End affput() */


/*=====================================================================*
*                              d3pdpz()                                *
*=====================================================================*/

void d3pdpz(void) {

   struct REPBLOCK *ir;    /* Ptr to current repertoire */
   struct CELLTYPE *il;    /* Ptr to current cell type block */
   struct CONNTYPE *ix;    /* Ptr to current conn block */
   struct INHIBBLK *ib;    /* Ptr to current inhib block*/
   struct MODBY    *im;    /* Ptr to current mod block  */
   struct CONDUCT  *ic;    /* Ptr to conductance block */
   struct NNSTR    dpio;   /* Struct to manage node data stream */
   iter            dpit;   /* Cell list iterator */
   si64 *pd64,*pe64;       /* Ptr to 64-bit type-detail items */
   long *pdist,*pe32;      /* Ptr to type-detail items */
   long cellno;            /* Number of current cell */
   long kbcdbs;            /* Binary scale for s values */
   long pitm;              /* Value to be printed */
   int  jct;               /* Connection type index */
   int  kphar,kphav;       /* Phase switches */
   int  node;              /* Node receiving data from */
   ui16 taitms;            /* Codes for 1 trial items */
   char ppftoff[DPCONNWIDTH]; /* Text for PPFT timers */

   /* Table to control selection and labels of terms in final totals */
   struct TotCtrl {
      ui32 Mask;                 /* kaff bit selector */
      char Head[4];              /* Label identifier */
      };
   static struct TotCtrl TotPrint[] = {
      { AFF_Excit  << SGM_Self,        "Self" },
      { AFF_Excit  << SGM_Specific,    "SpEx" },
      { AFF_Hyper  << SGM_Specific,    "SpHp" },
      { AFF_Square << SGM_Specific,    "SpSq" },
      { AFF_Shunt  << SGM_Specific,    "SpSh" },
      { NORM_SUMS  << SGM_VoltDep,     "Norm" },
      { AFF_Excit  << SGM_VoltDep,     "VdEx" },
      { AFF_Hyper  << SGM_VoltDep,     "VdHp" },
      { AFF_Excit  << SGM_Geometric,   "GmEx" },
      { AFF_Hyper  << SGM_Geometric,   "GmHp" },
      { AFF_Square << SGM_Geometric,   "GmSq" },
      { AFF_Shunt  << SGM_Geometric,   "GmSh" },
      { AFF_Excit  << SGM_Modulatory,  "MoEx" },
      { AFF_Hyper  << SGM_Modulatory,  "MoHp" },
      { AFF_Square << SGM_Modulatory,  "MoSq" },
      { AFF_Shunt  << SGM_Modulatory,  "MoSh" },
      { AFF_Excit  << SGM_Conduct,     "CdEx" },
      { AFF_Hyper  << SGM_Conduct,     "CdHp" },
      { AFF_Excit  << SGM_Total,       "TotE" },
      { AFF_Hyper  << SGM_Total,       "TotH" },
      { AFF_Square << SGM_Total,       "TotQ" },
      { AFF_Shunt  << SGM_Total,       "TotS" },
      { 1          << SGM_CellResp,    "GTot" },  };

   /* Table of shift and mask values to extract RGB components
   *  as function of tvmode.  */
   static const byte tvsv[Col_C24-Col_GS][4] = {
      { 0, 3, 6, 7 }, { 10, 5, 0, 0x1f }, { 16, 8, 0, 0xff } };

/* Adjust binary scales for compatibility option */

   kbcdbs = (FBsi + RP->bsdc) << RK_BS;

/* Pick up pointers to repertoire and cell blocks */

   il = CDAT.cdlayr;
   ir = il->pback;

/* Initialization for testput and distput routines */

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

/* Process all DP cells in current cell layer */

   ilstset(&dpit, il->dpclb->pclil, 0);
   while ((cellno = ilstiter(&dpit)) >= 0) {

/* Calculate node to which this cell was allocated
*  and initiate communications with this node.  */

      node = findcell(il, cellno);
      nngcr(&dpio, node, DP_DATA_MSG);

/* Receive and swap portion of CELLDATA structure that contains
*  data of interest, leaving our following host pointers intact.
*/

      {  char nxdata[lnndat];
         char *pd = nxdata;
         long *pl;
         nnget(&dpio, nxdata, lnndat);
         for (pl=&CDAT.cdcell; pl<=&CDAT.new_si; pl++,pd+=FMLSIZE)
            *pl = lemtoi4(pd);
         memcpy((char *)&CDAT.old_phi, pd,
            (unsigned)&CDAT.flags - (unsigned)&CDAT.old_phi + 1);
         } /* End local scope */

/*---------------------------------------------------------------------*
*  Cell number, position, state, and phase                             *
*---------------------------------------------------------------------*/

      convrt("(P4,4H0==>2*A" qLSN ",' CELL ',J2UL6,'AT',UL4,', ',JUL4,"
         "5H Si= J0B20/27IL8.<4)", ir->sname, il->lname,
         &CDAT.cdcell, &CDAT.groupx, &CDAT.groupy, &CDAT.new_si, NULL);
      /* These will continue the previous line because they do not
      *  begin with carriage control characters.  */
      if (il->Ct.rspmeth == RF_BREG)
         convrt("(P4,5H, W= J0B20/27IL8.<4)", &CDAT.izhu, NULL);
      if (il->Ct.rspmeth >= RF_IZH3)
         convrt("(P4,5H, U= J0B22/29IL8.<4)", &CDAT.izhu, NULL);
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
         if (piz->izrc) testput("(T,' C=',B20/27IL8.<5X)", &CDAT.izhc);
         if (piz->izrd) testput("(T,' D=',B20/27IL8.<5X)", &CDAT.izhd);
         }
      else {
         if (piz->izra) testput("(T,' A=',B28IL8.<5X)", &CDAT.izha);
         if (piz->izrb) testput("(T,' B=',B28IL8.<5X)", &CDAT.izhb);
         }
      }

      if (il->ctf & CTFDN)             /* Depression */
         testput("(T,' D=',B8IL8.4X)", &CDAT.depr);

      if (CDAT.noise)                  /* Noise */
         testput("(T,' N=',B20/27IL8.<4X)", &CDAT.noise);

      if (CDAT.nmodval != il->No.nfr)  /* Noise fraction */
         testput("(T,' Nf=',B24IJ7.5X)", &CDAT.nmodval);

      if (CDAT.events & PROBED)        /* Probe */
         testput("(T,' Pr=',B20/27IL7.<4X)", &CDAT.probeval);

      if (ctexists(il,SBAR))           /* Sbar */
         testput("(T,' Sb=',B7/14IL7.<4X)", &CDAT.sbar);

      if (ctexists(il,QBAR))           /* Qbar */
         testput("(T,' Qb=',B7/14IL7.<4X)", &CDAT.qbar);

      if (CDAT.events & DECAYED)       /* Persistence */
         testput("(T, ' W=',B20/27IL8.<4X)", &CDAT.pers);

      if (il->prfrc) {                 /* DST term */
         if (CDAT.dst < 0) {        /* DST encodes refractory time */
            long dstt = -CDAT.dst;
            testput("(T,' RT=',B16IL7X)", &dstt); }
         else                       /* DST is delta spike threshold */
            testput("(T,' Dt=',B20/27IL7.<4X)", &CDAT.dst);
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

      /* Collect all the data except the bandsums */
      pe64 = CDAT.pAffD64  + il->nAff64;
      for (pd64=CDAT.pAffD64; pd64<pe64; pd64++)
         *pd64 = nngeti8(&dpio);
      pe32 = CDAT.pAffData + il->nAff32;
      for (pdist=CDAT.pAffData; pdist<pe32; pdist++)
         *pdist = nngeti4(&dpio);

      /* Print specific connection type data */
      for (ix=il->pct1,jct=1; ix; ix=ix->pct,++jct) {
         if (ix->kgfl & KGNBP) continue;
         pd64 = CDAT.pAffD64 + ix->oex64;
         kphar = (ix->phopt & (PHASJ|PHASR)) != 0;
         kphav =
            (ix->phopt & (PHASJ|PHASR|PHASECNV|PHASDCNV|PHASVDU)) != 0;
         if (ix->cssck & PSP_POS) {
            pdist = CDAT.pAffData + ix->oexafs;          /* Excit */
            affput("RA1E", pd64, jct, kphar+2);
            pd64 += ix->lpax64;
            if (ix->Cn.ADCY.omega)
               affput("WA1E", pdist++, jct, 0);
            affput("FA1E", pdist, jct, kphav);
            }
         if (ix->cssk & PSP_NEG) {
            pdist = CDAT.pAffData + ix->oinafs;          /* Inhib */
            affput("RA1I", pdist, jct, kphar);
            pdist += kphar ? PHASE_RES : 1;
            if (ix->Cn.ADCY.omega)
               affput("WA1I", pdist++, jct, 0);
            affput("FA1I", pdist, jct, kphav);
            }
         if (ix->Cn.kam & KAMCG) {
            ui16 vmvt = nngeti2(&dpio);
            testput("(T,' V=',B8IH8.5X)", &vmvt);
            }
         } /* End ix loop */

      /* Receive and print geometric connection data */
      for (ib=il->pib1,jct=0; ib; ib=ib->pib) {
         long jx;             /* Inhib band counter */
         pdist = CDAT.pAffData + ib->ogafs;
         for (jx=0; jx<ib->nib; jx++) {                  /* Bandsums */
            pitm = nngeti4(&dpio);
            if (jx > 0) testput("(TB20/27IL11.<4X)", &pitm);
            else        affput("BG1 ", &pitm, jct, 0);
            } /* End loop over bands */
         if (ib->gssck & PSP_POS) {                      /* Excit */
            affput("RG1E", pdist++, jct, 0);
            if (ib->GDCY.omega)
               affput("WG1E", pdist++, jct, 0);
            affput("FG1E", pdist++, jct, 0); }
         if (ib->gssck & PSP_NEG) {                      /* Inhib */
            affput("RG1I", pdist++, jct, 0);
            if (ib->GDCY.omega)
               affput("WG1I", pdist++, jct, 0);
            affput("FG1I", pdist++, jct, 0); }
         } /* End inhibblk loop */

      /* Print modulation data */
      for (im=il->pmby1; im; im=im->pmby,++jct) {
         if (!(im->mssck & (PSP_POS|PSP_NEG))) continue;
         jct = im->mct;
         pdist = CDAT.pAffData + im->omafs;
         if (*pdist >= 0) {                              /* Excit */
            affput("RM1E", pdist++, jct, 0);
            if (im->MDCY.omega)
               affput("WM1E", pdist++, jct, 0);
            affput("FM1E", pdist++, jct, 0); }
         else {                                          /* Inhib */
            affput("RM1I", pdist++, jct, 0);
            if (im->MDCY.omega)
               affput("WM1I", pdist++, jct, 0);
            affput("FM1I", pdist++, jct, 0); }
         } /* End im loop */

      /* Print conductance data */
      for (ic=il->pcnd1,jct=1; ic; ic=ic->pncd,++jct) {
         pdist = CDAT.pAffData + ic->ocafs;
         if (*pdist >= 0) {                              /* Excit */
            affput("RC1E", pdist++, jct, 0);
            if (ic->gDCY.omega)
               affput("WC1E", pdist++, jct, 0);
            affput("FC1E", pdist++, jct, 0); }
         else {                                          /* Inhib */
            affput("RC1I", pdist++, jct, 0);
            if (ic->gDCY.omega)
               affput("WC1I", pdist++, jct, 0);
            affput("FC1I", pdist++, jct, 0); }
         } /* End ic loop */

/* Flush output line */

      if (jcol > 2) cryout(RK_P4, text, (RK_LN1-1)+jcol, NULL);

OmitTypeDetail: ;

/*---------------------------------------------------------------------*
*  Totals by connection class                                          *
*---------------------------------------------------------------------*/

      /* Read totals arrays from comp node and swap if necessary */
      {  long *ptot = CDAT.pTotSums;/* Ptr to item in totals array */
         long *ptote =              /* Ptr to end of totals array */
            (long *)((char *)ptot + CDAT.lSpecSums + CDAT.lGeomSums +
               CDAT.lModulSums + CDAT.lCondSums + CDAT.lTotSums);
         for ( ; ptot<ptote; ptot++)
            *ptot = nngeti4(&dpio);
         } /* End local scope and receiving totals */

      cryout(RK_P4,
         " Total inputs by connection class", RK_LN1+RK_NFSUBTTL,
         " (GT normalized)", (il->pctpd && il->pctpd->il->phops &
         PHOPM) ? RK_CCL : RK_SKIP, ":", 1, NULL);
      jcol = 2;

/* Terms are printed in order and with labels given in tables above */

      for (jct=0; jct<sizeof(TotPrint)/sizeof(struct TotCtrl); jct++) {
         ui32 mask = il->kaff & TotPrint[jct].Mask;
         if (mask) {
            pdist = CDAT.pAffData + getoas(il, mask);
            if (il->phshft) {
               convrt("(P4XA4#" qPHR "|R(B20/27IL8.<4))",
                  TotPrint[jct].Head, &lpha, pdist, NULL);
               jcol = 2; }
            else {
               if (jcol + DPITEMWIDTH > mcol) {
                  cryout(RK_P4, text, (RK_LN1-1) + jcol, NULL);
                  jcol = 2; }
               sconvrt(text, "(P4TA4B20/27IL7.<4X)", &jcol,
                  TotPrint[jct].Head, pdist, NULL);
               jcol += DPITEMWIDTH; }
            }
         } /* End loop over response function terms */

/* Flush output line */

      if (jcol > 2) cryout(RK_P4, text, (RK_LN1-1)+jcol, NULL);

/*---------------------------------------------------------------------*
*                     Autoscale adjustment factor                      *
*---------------------------------------------------------------------*/

   if (il->kautu) convrt("(P4,' Autoscale multiplier: ',J0B24IJ8.<5)",
      RP->psclmul + il->oautsc, NULL);

/*---------------------------------------------------------------------*
*  Print connection-level data under control of DPITEM mask.           *
*  There will be nothing transmitted if DPITEM is zero.                *
*                                                                      *
*  Note:  The move to the d3lij() family of routines allows Lij (and   *
*  Sj) to be printed even in OPTIMIZE STORAGE mode, when Lij values    *
*  are not in memory.  Also, for user convenience, changed to printing *
*  all the values for one connection before going on to the next.      *
*---------------------------------------------------------------------*/

/* Note: Here, jcol is the offset into the line.  In the code above
*  and in the FORTRAN version it was the column number (= offset+1). */

      if (il->pct1 && taitms &
            (DPLIJ|DPDIJ|DPCIJ|DPCIJ0|DPMIJ|DPRIJ|DPPPF|DPSJ|DPDCIJ)) {

         long isyn;              /* Synapse index (0 base) */
         long J;                 /* Connection number */
         long Mijtest;           /* Test for Mij a clock */
         long kbcdp,kbcds;       /* ibcdwt conversion codes */
         long nuk;               /* Number of connections used */
         long tvsm;              /* Color extraction mask */
         int nsjc;               /* No. of columns for sj,phasej */
         int tvsr,tvsg,tvsb;     /* Color extraction shifts */
         ui16 doLij,doDij,doCij,doCij0,   /* TRUE if printing the */
            doMij,doRij,doPPF,doPPFT,     /* named variable */
            doSj,doDCij;

         /* Lij (except KGNHV, when lijbr returns 0), Cij, and Sj
         *  always exist and can always be accessed and printed
         *  whether or not stored, therefore, their doXij flags
         *  can be set before the conntype loop.  */
         doLij = taitms & DPLIJ;
         doCij = taitms & DPCIJ;
         doSj  = taitms & DPSJ;

         /* ibcdwt format code for Sj, Mij, Rij */
         kbcds = kbcdbs|(4<<RK_DS|RK_GFP|RK_AUTO|RK_IORF|DPCONNWIDTH-1);

/* Loop over connection types */

         for (ix=il->pct1, jct=1; ix; ix=ix->pct, jct++) {
            if (ix->kgfl & KGNBP) continue;

            /* Set up the subtitle according to the items present */
            strcpy(text, "       J");
            jcol = DPCONNWIDTH;
            if (doLij) {
               memcpy(text+jcol, "     Lij", DPCONNWIDTH);
               jcol += DPCONNWIDTH;
               } /* Assignment intended in next line */
            if (doDij = taitms & DPDIJ && ix->Cn.kdelay) {
               memcpy(text+jcol, "     Dij", DPCONNWIDTH);
               jcol += DPCONNWIDTH;
               }
            if (doCij) {
               memcpy(text+jcol, "     Cij", DPCONNWIDTH);
               jcol += DPCONNWIDTH;
               }  /* Assignment intended in next line */
            if (doCij0 = taitms & DPCIJ0 && cnexists(ix,CIJ0)) {
               memcpy(text+jcol, "    Cij0", DPCONNWIDTH);
               jcol += DPCONNWIDTH;
               }  /* Assignment intended in next line */
            if (doMij = taitms & DPMIJ && cnexists(ix,MIJ)) {
               memcpy(text+jcol, "     Mij", DPCONNWIDTH);
               jcol += DPCONNWIDTH;
               Mijtest = ix->Cn.mticks - S15;
               }  /* Assignment intended in next line */
            if (doRij = taitms & DPRIJ && cnexists(ix,RBAR)) {
               memcpy(text+jcol, "    <Sj>", DPCONNWIDTH);
               jcol += DPCONNWIDTH;
               }
            if (doDCij = taitms & DPDCIJ &&
                  ix->finbnow != NO_FIN_AMP) {
               memcpy(text+jcol, "  DelCij", DPCONNWIDTH);
               jcol += DPCONNWIDTH;
               }  /* Assignment intended in next line */
            if (doPPF = taitms & DPPPF && cnexists(ix,PPF)) {
               memcpy(text+jcol, "     PPF", DPCONNWIDTH);
               jcol += DPCONNWIDTH;
               }  /* Assignment intended in next line */
            if (doPPFT = taitms & DPPPF && cnexists(ix,PPFT)) {
               memcpy(text+jcol, " PPFTime", DPCONNWIDTH);
               jcol += DPCONNWIDTH;
               kbcdp = (ix->PP->ppfopt & PPFABR) ?
                  (RK_BSCL+2*RK_D+RK_GFP+RK_IORF+DPCONNWIDTH-2) :
                  (2*RK_D+RK_GFP+RK_IORF+DPCONNWIDTH-2);
               }  /* Assignment intended in next line */
            if (doSj) {
               tvsm = 0;
               if (ix->phopt & (PHASJ|PHASR|PHASC)) {
                  nsjc = DPCONNWIDTH + DPPHIWIDTH;
                  memcpy(text+jcol, "      Sj Phi", nsjc); }
               else if ((enum ColorMode)ix->cnxc.tvmode > Col_GS &&
                        (enum ColorMode)ix->cnxc.tvmode < Col_R4) {
                  int icm = (enum ColorMode)ix->cnxc.tvmode - Col_C8;
                  tvsr = (int)tvsv[icm][0];
                  tvsg = (int)tvsv[icm][1];
                  tvsb = (int)tvsv[icm][2];
                  tvsm = (long)tvsv[icm][3];
                  nsjc = DPCONNWIDTH + DPCOLWIDTH;
                  memcpy(text+jcol, "      Sj   R   G   B", nsjc); }
               else {
                  nsjc = DPCONNWIDTH;
                  memcpy(text+jcol, "      Sj", nsjc); }
               jcol += nsjc;
               }
            /* Just a little safety check */
            if (jcol > LNSIZE)
               d3exit(DPLLEN_ERR, fmturlnm(il), jct);
            sconvrt(text+jcol, "(' Connection data for cell ',J0UL8,"
               "', conntype ',J0I4)", &CDAT.cdcell, &jct, NULL);
            cryout(RK_P4, text+jcol, RK_NFSUBTTL+RK_LN2,
               text, RK_LN0+jcol, NULL);

/* Loop over synapses.  Receive and print
*  J, Lij, Dij, Cij, Cij0, Mij, Rij, DCij, PPF, and Sj data */

            nuk = nngeti4(&dpio);
            for (isyn=0; isyn<nuk; isyn++) {

               /* Fetch connection number (J)--value corrected for
               *  skipped connections is passed from comp node.  */
               J = nngeti4(&dpio);
               jcol = DPCONNWIDTH;

               /* Get and print Lij */
               if (doLij) {
                  pitm = nngeti4(&dpio);
                  ibcdwt(RK_IORF+DPCONNWIDTH-1, text+jcol, pitm);
                  jcol += DPCONNWIDTH;
                  }

               /* Get and print Dij (delay) */
               if (doDij) {
                  pitm = nngeti4(&dpio);
                  ibcdwt(RK_IORF+DPCONNWIDTH-1, text+jcol, pitm);
                  jcol += DPCONNWIDTH;
                  }

               /* Get and print Cij */
               if (doCij) {
                  pitm = nngeti4(&dpio);
                  ibcdwt((31<<RK_BS)+(3<<RK_DS)+RK_GFP+RK_IORF+
                     DPCONNWIDTH-1, text+jcol, pitm);
                  jcol += DPCONNWIDTH;
                  }

               /* Get and print baseline Cij (Cij0) */
               if (doCij0) {
                  pitm = nngeti4(&dpio);
                  ibcdwt((31<<RK_BS)+(3<<RK_DS)+RK_GFP+RK_IORF+
                     DPCONNWIDTH-1, text+jcol, pitm);
                  jcol += DPCONNWIDTH;
                  }

               /* Get and print Mij */
               if (doMij) {
                  pitm = nngeti4(&dpio);
                  if (pitm <= Mijtest) {
                     ibcdwt(RK_IORF+DPCONNWIDTH-1, text+jcol,
                        pitm+S15);
                     text[jcol+(DPCONNWIDTH-2)-RK.length] = 'T'; }
                  else
                     ibcdwt(kbcds, text+jcol, pitm);
                  jcol += DPCONNWIDTH;
                  }

               /* Get and pritn Rij */
               if (doRij) {
                  pitm = nngeti4(&dpio);
                  ibcdwt(kbcds, text+jcol, pitm);
                  jcol += DPCONNWIDTH;
                  }

               /* Get and print delta(Cij).
               *  Note:  If ix->finbnow is zero, both d3go and this
               *  code and d3pdpn skip advancing pdelc, so we
               *  should keep in step.  */
               if (doDCij) {
                  pitm = nngeti4(&dpio);
                  ibcdwt((15<<RK_BS)+(4<<RK_DS)+RK_GFP+RK_IORF+
                     DPCONNWIDTH-1, text+jcol, pitm);
                  jcol += DPCONNWIDTH;
                  }

               /* Get and print PPF */
               if (doPPF) {
                  pitm = nngeti4(&dpio);
                  ibcdwt(12*RK_BSCL+4*RK_D+RK_GFP+RK_IORF+DPCONNWIDTH-1,
                     text+jcol, pitm);
                  jcol += DPCONNWIDTH;
                  }

               /* Get and print PPFT */
               if (doPPFT) {
                  pitm = nngeti4(&dpio);
                  if (pitm == 0)
                     memcpy(text+jcol, ppftoff, DPCONNWIDTH);
                  else {
                     ibcdwt(kbcdp, text+jcol, labs(pitm));
                     text[jcol+DPCONNWIDTH-1] = pitm > 0 ? 'D' : 'R'; }
                  jcol += DPCONNWIDTH;
                  }

               /* Get and print Sj */
               if (doSj) {
                  long sjval = nngeti4(&dpio);
                  if (sjval == -1) {
                     /* If scanned topo and out-of-bounds, omit */
                     if (taitms & DPOOB) continue;
                     /* Sj bad, print dashes--phase,color not sent */
                     memcpy(text+jcol, "    ---- --- --- ---", nsjc);
                     }
                  else {
                     /* Sj as used */
                     ibcdwt(kbcds, text+jcol, sjval);
                     /* Optional phase */
                     if (ix->phopt & (PHASJ|PHASR|PHASC)) {
                        long phi = nngeti4(&dpio);
                        ibcdwt(RK_IORF+DPPHIWIDTH-1,
                           text+jcol+DPCONNWIDTH, phi);
                        }
                     /* Optional raw color in R-G-B format */
                     else if (tvsm) {
                        long sjraw = nngeti4(&dpio);
                        ibcdwt(RK_IORF+3, text+jcol+DPCONNWIDTH,
                           sjraw >> tvsr & tvsm);
                        ibcdwt(RK_IORF+3, text+jcol+(DPCONNWIDTH+4),
                           sjraw >> tvsg & tvsm);
                        /* Note:  In Col_S8 mode, blue mask includes
                        *  an extra bit harmlessly--always 0 in ljraw */
                        ibcdwt(RK_IORF+3, text+jcol+(DPCONNWIDTH+8),
                           sjraw >> tvsb & tvsm);
                        }
                     }
                  jcol += nsjc;
                  }

               /* Print connection number */
               ibcdwt(RK_IORF+DPCONNWIDTH-1, text, J);

               cryout(RK_P4, text, RK_LN1+jcol, NULL);
               } /* End loop over connections */

            /* Check for synchronization with comp node */
            pitm = nngeti4(&dpio);
            if (pitm != END_OF_GROUP)
               d3exit(DPSYNC_ERR, fmturlnm(il), jct);

            } /* End loop over connection types */

         /* Silently clear any subtitle left from above output */
         cryout(RK_P4, text, RK_LN1+RK_NTSUBTTL+1, NULL);

         } /* End connection-level data output */

      nngcl(&dpio);           /* Close data stream */
      } /* End DP search in current cell layer */

   } /* End d3pdpz() */
