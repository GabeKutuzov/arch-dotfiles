/* (c) Copyright 1991-2010, Neurosciences Research Foundation, Inc. */
/* $Id: d3stat.c 50 2012-05-17 19:36:30Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                               d3stat                                 *
*                                                                      *
*  Print statistics for all D1 and D3 repertoires.  In the parallel    *
*  version, d3gsta gathers all statistics that have been accumulated   *
*  in the repertoire data structures.  However, cross-response stats   *
*  are gathered by d3stat itself, interleaved with printing, so that   *
*  accumulation structs can be reused for all GRPNO categorizations.   *
*                                                                      *
*  Note:  Counts of trials and cycles previously calculated in this    *
*  program are now obtained by actual counting in d3go().  Although    *
*  slightly slower, this method assures correct statistics following   *
*  operator intervention and improves maintainability because complex  *
*  contingencies for performing amplification do not affect d3stat().  *
************************************************************************
*                                                                      *
*  The following letters are used with 'TRA' for intra-class,          *
*        'TER' for inter-class, 'RAT' for intra/inter ratio            *
*  F     Found flag for class                                          *
*  G     Groups involved in cross-responses                            *
*  H     Hits                                                          *
*  K     Responses by current group to stim in current/other class     *
*  M     Max pairs                                                     *
*  N     Pairs found                                                   *
*  X     Expected pairs                                                *
*  A     Placed before a name makes it floating point or ratio         *
*                                                                      *
*  Note: LHF temporary scaling of SDIST is now permanent               *
*                                                                      *
*  V4A, 05/12/89, JWT - Translated into C                              *
*  Rev, 04/25/90, GNR - Separate table of CSKIP, calc from CDIST       *
*  V5C, 12/26/91, GNR - Add D1 stats                                   *
*  V6A, 03/29/93, GNR - Add modulation stats                           *
*  V6C, 09/23/93, GNR - Make DAS stats double, correct divrnd bug      *
*  V7B, 06/28/94, GNR - Give CUME 4 decimals, make Cij dist a fraction,*
*                       add regeneration count, improve centering      *
*  V7C, 11/12/94, GNR - Divide ngr into ncs,nds, add C,F dist stats    *
*  Rev, 03/26/95, GNR - Fix MPP to agree with writeup, fix gtra,gter   *
*  V8A, 05/06/95, GNR - Add MODALITY & correl. matrix, H,X,Y,Z,C,F,U   *
*                       stats controlled at CELLTYPE level,consolidate *
*                       messages for H,X,Y,Z stats, remove fhits[pn],  *
*                       nhit[pn] stats & centering, add GCONN/MOD pers *
*                       and spike count                                *
*  Rev, 11/28/96, GNR - Remove support for non-hybrid version          *
*  Rev, 01/13/97, GNR - Add fmtict routine, bypass is KAMBY, not KGNBP *
*  Rev, 02/18/97, GNR - Allow for ntrs,ntcs in amplification stats     *
*  Rev, 01/02/98, GNR - Add ADECAY, reorganize all distribution stats  *
*  Rev, 10/28/00, GNR - Remove XRH, tabulate KRPHR over all cycles     *
*  V8B, 12/16/00, GNR - Move to new memory management routines         *
*  V8C, 08/04/03, GNR - Cell responses in millivolts, add conductances *
*  V8D, 07/24/07, GNR - Add Mij stats to DAS, add KAMDR dist.          *
*  ==>, 12/29/07, GNR - Last mod before committing to svn repository   *
*  V8E, 04/12/09, GNR - More digits for 1st-row stats, detstats, etc.  *
*  Rev, 11/07/09, GNR - Move nntr to CLSTAT.nstcap (may differ per il) *
*  V8F, 05/17/10, GNR - Add KRPGP statistics                           *
*  V8G, 08/31/10, GNR - Add group stats to KRPCL stats                 *
*  Rev, 03/30/12, GNR - Add distribution of Sj statistic               *
*  Rev, 04/02/12, GNR - Expand DAS stats to include delta Cij by case  *
***********************************************************************/

#ifdef D1
#define D1TYPE  struct D1BLK
#endif
#define GSTYPE  struct GSDEF

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "sysdef.h"
#include "d3global.h"
#include "celldata.h"
#include "statdef.h"
#include "collect.h"
#ifdef D1
#include "d1def.h"
#endif
#ifndef PARn
#include "rocks.h"
#include "rkxtra.h"
#include "rkarith.h"
#endif
#include "bapkg.h"

extern struct CELLDATA CDAT;

#define MXCOL       26  /* Number of cols in cross-response matrix */
#define DS8 0.00390625  /* = 1/S8 for scaling S8 numbers */

/* Form averages from fixed-point sums with scaling and zero check.
*  Note, 09/24/93, GNR - The divrnd macro used in earlier versions
*  of d3stat was wrong for two reasons: (1) because negative numbers
*  were rounded towards zero, and (2) because rounding essentially
*  occurred twice, once in the macro and once in convrt, whereas all
*  rounding should be done only during conversion for printing.  */
#define divscl(x,y,s)  (((y) == 0) ? 0.0 : \
   ((double)(x)/((double)(1L<<(s))*(double)(y))))
#define divchk(x,y)    (((y) == 0) ? 0.0 : \
   ((double)(x)/(double)(y)))
#define wdivchk(x,y)   (((y) == 0) ? 0 : jdswq((x),(y)))

#ifdef PAR
/* Define COPDEF structure for collecting cross-response stats */
static struct COPDEF XRMCop = { ADD+LONG, 0 };
#endif

/* Structure for collecting cross-response statistics.  It should
*  contain only longs, as it is collected to host node by use of
*  xstpack(),xstunpk().  XSTASIZ is number of longs in struct.  */
struct GST {
   long nhit;        /* No. of cells responding at least once */
   long gtra;        /* Groups involved in intra-class pairs */
   long gter;        /* Groups involved in inter-class pairs */
   long htra;        /* Intra-class hits */
   long hter;        /* Inter-class hits */
   long mtra;        /* Maximum possible intra-class pairs */
   long mter;        /* Maximum possible inter-class pairs */
   long ntra;        /* Number of intra-class pairs observed */
   long nter;        /* Number of inter-class pairs observed */
   } ;
#define XSTASIZ (sizeof(struct GST)/sizeof(long))

/***********************************************************************
*      Local routines used to collect cross-response statistics        *
***********************************************************************/

/* Define global symbols needed for packing and collecting cross-
*  response statistics */
#ifdef PAR
static long *buff;         /* Base location of buffer */
static long *bptr;         /* Current location in buffer */
static long *btop;         /* Top of buffer */
static long xstodo;        /* Items remaining */
#endif

#ifdef PARn
/*---------------------------------------------------------------------*
*                               xstpack                                *
*                                                                      *
*  Routine to buffer cross-response statistics and accumulate totals   *
*  across nodes.  Caller must precompute the total number of items     *
*  and store this value in xstodo before the first call, in order      *
*  that the XRMCop struct can be filled in with the correct count.     *
*  Caller must also allocate initial buffer before first use and       *
*  call with NULL argument to send final (partial) buffer.             *
*---------------------------------------------------------------------*/

static void xstpack(long *item, int n) {

   /* Transfer n longs to output */
   while (n--) {
      /* If not sufficient space in buffer, perform global collection
      *  and allocate another.  */
      if (bptr >= btop) {
         int newsize = min(xstodo*sizeof(long), MAX_MSG_LENGTH);
         XRMCop.u.count = bptr - buff;
         vecollect((char *)buff, &XRMCop, 1, XSTATS_MSG,
            (char *)buff - (char *)bptr);
         if (!newsize) return;
         buff = bptr = (long *)angetpv(newsize, "XR stat buff");
         btop = buff + newsize/sizeof(long);
         } /* End buffer-full action */
      /* Copy item to buffer, update pointers and count */
      *bptr++ = *item++;
      --xstodo;
      } /* End while n */
   } /* End xstpack() */
#endif /* PARn */

#ifdef PAR0
/*---------------------------------------------------------------------*
*                               xstunpk                                *
*                                                                      *
*  Routine used to unpack cross-response statistics on host.           *
*  Caller must precompute the number of items and store this value     *
*  in xstodo before the first call, in order that the XRMCop struct    *
*  can be filled in with the correct count.  Caller must also set      *
*  buff = btop = 0 before first use and call anrelp(buff) to clean up. *
*---------------------------------------------------------------------*/

static void xstunpk(long *item, int n) {

   /* Transfer n longs to item array */
   while (n--) {
      /* If not sufficient data in current buffer,
      *  collect a batch from comp nodes.  */
      if (bptr >= btop) {
         int newsize = min(xstodo*sizeof(long), MAX_MSG_LENGTH);
         /* If buffer not yet allocated, get one now */
         if (!btop) {
            buff = bptr = (long *)angetpv(newsize, "XR stat buff");
            btop = buff + newsize/sizeof(long); }
         /* Clear and collect */
         memset((char *)buff, 0, newsize);
         XRMCop.u.count = newsize/sizeof(long);
         vecollect((char *)buff, &XRMCop, 1, XSTATS_MSG,
            -newsize);
         } /* End buffer-empty action */
      /* Copy item from buffer, update pointers and count */
      *item++ = *bptr++;
      --xstodo;
      } /* End while n */
   } /* End xstunpk() */
#endif

#ifndef PARn
/*---------------------------------------------------------------------*
*                               avghist                                *
*                                                                      *
*  Local routine used to compute average values in a histogram.        *
*  Arguments:                                                          *
*     jxv   Pointer to array containing histogram values               *
*     jls   Pointer to array containing s(i) distribution              *
*     stp   Pointer to result stat array (has same scale as jxv)       *
*---------------------------------------------------------------------*/

static void avghist(si64 *jxv, long *jls, long *stp) {

   long *jle = jls + LDSTAT;
   for ( ; jls < jle; jls++,jxv++,stp++)
      *stp = *jls ? jdswq(*jxv,*jls) : 0;
   } /* End avghist() */

/*---------------------------------------------------------------------*
*                              normhist                                *
*                                                                      *
*  Local routine used to compute normalized values in an S8 histogram  *
*  Arguments:                                                          *
*     jxv   Pointer to array containing histogram values               *
*     stp   Pointer to result stat array                               *
*  Returns TRUE if there are any nonzero data, otherwise FALSE         *
*---------------------------------------------------------------------*/

static int normhist(long *jxv, double *stp) {

   register double *st1,*st2 = stp + LDSTAT;
   double hsum = 0.0;
   for (st1=stp; st1<st2; st1++,jxv++)
      hsum += (*st1 = (double)(*jxv));
   if (hsum) for (st1=stp; st1<st2; st1++) *st1 /= hsum;
   return (hsum ? TRUE : FALSE);
   } /* End normhist() */

/*---------------------------------------------------------------------*
*                               fmtict                                 *
*                                                                      *
*  Local routine used to format a connection type number as '(i)' if   *
*  type number is less than 10, otherwise as 'iii'.                    *
*  Argument:                                                           *
*     ict   Connection type number to be converted                     *
*  Result string is in static storage, and remains valid until next    *
*     call to this routine.                                            *
*---------------------------------------------------------------------*/

static char cict[4];

char *fmtict(int ict) {

   if (ict < 10) {
      cict[0] = '(';
      cict[1] = '0' + ict;
      cict[2] = ')';
      }
   else
      ibcdwt(RK_IORF+RK_PAD0+2, cict, (long)ict);
   cict[3] = '\0';
   return cict;
   } /* End fmtict */

/*---------------------------------------------------------------------*
*                               fmtmwdw                                *
*                                                                      *
*  Local routine to format a window number for a sensory modality.     *
*  Argument:                                                           *
*     pmdlt    Ptr to modality block whose window number is to be      *
*              printed.                                                *
*  Result string is in static storage cimw, and remains valid until    *
*     next call to this routine.  Return value is RK_CCL if there is   *
*     a value to print, otherwise RK_SKIP.                             *
*---------------------------------------------------------------------*/

static char cimw[] = " (WINDOW xxx)";

unsigned int fmtmwdw(struct MODALITY *pmdlt) {

   if (pmdlt->mdltwdw <= 0)
      return RK_SKIP;
   else {
      sconvrt(cimw+9, "(J0M3,H))", &pmdlt->mdltwdw, NULL);
      return RK_CCL;
      }
   } /* End fmtmwdw */

#endif

/***********************************************************************
*                               d3stat                                 *
***********************************************************************/

void d3stat(void) {

   struct CELLTYPE *il;       /* Ptr to current cell block */
   struct REPBLOCK *ir;       /* Ptr to current repertoire */
   struct MODALITY *pmdlt;    /* Ptr to current MODALITY block */
   hist *phr;                 /* Ptr to current history stats  */
   ui16 *pncl;                /* Ptr to no. classes in stim. group */
   long lndsb;                /* Local ndsbyt */
   long nst1;                 /* Largest isn in a modality */
   ui16 igpnt,mgpnt;          /* GRPNO card counter,count */
   unsigned short xopt;       /* Cross-response statistics mask */

#ifndef PAR0
   long lllt;                 /* Copy of prd length  */
   rd_type *g1, *g2, *G;      /* Ptrs to access rep data */
   long ginc;                 /* Increment for g1 */
   long oxrm,ioff1,ioff2;     /* Offsets to oxrm data */
#endif

#ifndef PARn
   struct CONNTYPE *ix;       /* Ptr to current connection block */
   byte *pgpmsk,*pgpmski;     /* Ptrs to grpmsks */
   char *prange;              /* Ptr to distribution ranges */
   double rawscl;             /* dS23 or dS30 if COMPAT=C */
   double stat[LDSTAT];       /* Statistical info arrays */
   long lstat[LDSTAT];
   char gfmt[136];            /* Output format buffer */
   int ict;                   /* Conntype counter */
   static char blanks[] = "                                          "
      "          ";
   static char underlines[] = " _____________________________________"
      "______________________________________________________________"
      "_________________________________";
   static char *ranges[2] = {
   /* NEW */   "+S(mV)< -32.0   -16.0    -8.0    -4.0    -2.0"
            "    -1.0    -0.5     0.0     0.5     1.0     2.0"
            "     4.0     8.0    16.0    32.0   256.0",
   /* OLD */   "+S(I) <  0.06    0.13    0.19    0.25    0.31"
            "    0.38    0.44    0.50    0.56    0.63    0.69"
            "    0.75    0.81    0.88    0.94    1.00" };

   if (RP->compat & OLDRANGE) {
      prange = ranges[1];
      rawscl = dS30;
      }
   else {
      prange = ranges[0];
      rawscl = dS23;
      }
   /* If requsted, SPOUT everything */
   if (RP->CP.runflags & RP_SSTAT) spout(SPTM_ALL);
#endif

/*---------------------------------------------------------------------*
*             Preparations for cross-response statistics               *
*  Data for X,Y,Z,H stats are collected with xstpack/xstunpk while     *
*  data for C and G stats are collected by regular d3gsta mechanism.   *
*---------------------------------------------------------------------*/

/* Initialize ncl and group mask array--this must be redone for
*     each series because the selection of stimuli may change the
*     number and identity of the stimuli in each class.  G stats
*     (and C stats if revised to actually work with classes)
*     require grpmsks on Node 0, but they need not be broadcast.
*  N.B.  The grpmsk for each group is a bit array containing a one
*     bit for every stimulus that is a member of that group. These
*     arrays are accessed via pgrpmsk to broadcast as a unit.  */

#ifndef PARn
      /* Locate and clear the ncl and grpmsk arrays.  N.B.  Code in
      *  igpnt loop below assumes these values have been cleared.  */
      memset((char *)STAT.pgrpblk, 0, STAT.lgrpblk);
      pncl = STAT.pgrpncl;
      pgpmsk = STAT.pgrpmsk;

      if (RP->kxr & (XSTATS|YSTATS|ZSTATS|HSTATS|GSTATS)) {
         for (pmdlt=RP->pmdlt1; pmdlt; pmdlt=pmdlt->pnmdlt) {
            struct GRPDEF *pgpn;       /* Ptr to GRPNO chain */
            short *pgpnl;              /* Ptr to GRPNO list  */
            int ncs_exceeded = FALSE;  /* Flag for ncs error */
            short igr;                 /* Current group num. */
            lndsb = pmdlt->ndsbyt;     /* Bytes in a grpmsk  */
            mgpnt = NStimGrps(pmdlt);  /* Number GRPNO cards */

            /* Make GRPMSKs from GRPNO tables on host.  Scan through
            *  all the stimuli that were used, and assign a new class
            *  each time a new class number is encountered.  Set this
            *  number negative each place where it occurs on a higher-
            *  numbered stimulus so it will not be taken as new when
            *  it repeats.  At the end, set them all positive again.
            *  If pmdlt->ncs count is exceeded, issue a warning and
            *  put all further stimuli in the last class.  This was
            *  not required before V7C, when ncs = nds = ngr.  */
            pgpn = pmdlt->pgpno;          /* Ptr to GRPNO chain */
            nst1 = pmdlt->um1.MS.mxisn;   /* Highest isn found  */

            for (igpnt=1; igpnt<=mgpnt; igpnt++, pncl++) {
               long i,j;                  /* Loop counters */
               pgpnl = pgpn->grp;         /* Locate GRPNO list */
               pgpmski = pgpmsk - lndsb;  /* Locate grpmsk     */

               for (i=0; i<nst1; i++) {
                  if (!bittst(pmdlt->pxxxx,i+1)) continue;
                  if ((igr=pgpnl[i]) > 0) {
                     if (*pncl >= pmdlt->ncs) ncs_exceeded = TRUE;
                     else {         /* Max classes not exceeded */
                        (*pncl)++;
                        pgpmski += lndsb;
                        for (j=i; j<nst1; j++) {
                           if ((pgpnl[j] != igr) ||
                              !bittst(pmdlt->pxxxx,j+1)) continue;
                           bitset(pgpmski,j+1);
                           pgpnl[j] = -igr;
                           } /* End for j */
                        } /* End else (max ncs not exceeded) */
                     } /* End action for new class */
                  pgpnl[i] = abs(igr);
                  } /* End for i */
               pgpn = pgpn->pnxgrp;
               pgpmsk += lndsb*pmdlt->ncs;
               } /* End for igpnt */

            if (ncs_exceeded) cryout(RK_P1,
               "0-->WARNING: One or more stimulus groupings had more"
               " than NCS classes, excess ignored.", RK_LN2, NULL);
            } /* End modality loop */
         } /* End if X,Y,Z,H,G stats */
#endif

/* Back to all nodes, set xopt to accept KRPHR, but mask out KRPXR,
*  KRPYM, KRPZC bits according to whether more than one stimulus was
*  found for at least one modality.  */

   xopt = KRPHR;
   if (RP->kxr & (XSTATS|YSTATS|ZSTATS|HSTATS)) {

#ifdef PAR
      /* Local conversion table for the GRPMSK_MSG broadcast */
      long nxtabl[4];
#endif

      for (pmdlt=RP->pmdlt1; pmdlt; pmdlt=pmdlt->pnmdlt) {
         /* If two or more different stimuli were presented,
         *  set xopt flag to activate statistics later.  */
         lndsb = pmdlt->ndsbyt;
         if (bitcnt(pmdlt->pxxxx, lndsb) >= 2)
            xopt |= (KRPXR | KRPYM | KRPZC);
         } /* End modality loop */

/* Broadcast the ncl and grpmsk data collected in grpblk */
#ifdef PAR
      /* Create a "hand made" nxdr conversion table.  See nxdr doc for
      *  details.  Table describes ngpntt shorts (ncl), followed by sum
      *  over all modalities of (ngpnt*ncs*ndsbyt) bytes (grpmsks). */
      nxtabl[0] = STAT.lgrpblk;
      nxtabl[1] = (sizeof(short)<<8) | 's';
      nxtabl[2] = STAT.ngpntt<<8 | 'h' ;
      nxtabl[3] = (STAT.lgrpblk - STAT.ngpntt*sizeof(short))<<8 | 'C' ;
      blkbcst(STAT.pgrpblk, nxtabl, 1, GRPMSK_MSG);
#endif
      } /* End if X,Y,Z,H stats */

#ifdef D1
#ifndef PARn
/*---------------------------------------------------------------------*
*                         Print D1 statistics                          *
*---------------------------------------------------------------------*/

   if (RP->pd1b1) {
      struct D1BLK *pd1;      /* Ptr to current D1 block */
      cryout(RK_P2, "0   ", RK_NFSUBTTL+RK_LN2, NULL);
      cryout(RK_P2, blanks, RK_LN1+9,
         "---STATISTICS FOR DARWIN I REPERTOIRES---", RK_CCL, NULL);
      cryout(RK_P2,
         "0", RK_NFSUBTTL+RK_LN2+1, underlines, RK_OMITLN+RK_LN1+58,
         "+ REPERTOIRE  MEAN S      HIGHEST      LOWEST    CELLS>AMPT",
         RK_LN0, NULL);
      for (pd1=RP->pd1b1,ict=1; pd1; pd1=pd1->pnd1b,ict++) {
         register double dnsum;
         if (pd1->kd1p & KD1_NOST) continue;
         dnsum = (double)pd1->stats.d1ncyc*(double)pd1->nd1r;
         stat[1] = divchk(pd1->stats.d1numgtt,dnsum);
         dnsum *= pd1->nepr;
         stat[0] = divscl(pd1->stats.d1sumsco,dnsum,8);
         convrt("(P2,I8,Q12.3,2*B8IL12.3,Q12.3)",&ict,stat,
            &pd1->stats.d1maxsco,&pd1->stats.d1minsco,stat+1,NULL);
         } /* End D1 loop */
      } /* End if D1 */
#endif /* not PARn */
#endif /* D1 */

/*---------------------------------------------------------------------*
*                   Print D3 repertoire statistics                     *
*---------------------------------------------------------------------*/

   for (ir=RP->tree; ir; ir=ir->pnrp) {
#ifndef PARn
      int mxlrn,mxlrn2;
      int gassw, nassw, gotas;      /* AUTOSCALE format switches */
      ui16 orkaut = 0;
      char *ashd1[3] = {"", "   AUTOSCALE", "    AUTOSCALE MULTIPLIER"};
      char *ashd2[3] = {"", "  MULTIPLIER", "      MIN   LAST  MAX   "};
      char ascl[24];
#endif

      if (ir->Rp.krp & KRPNS) continue;

#ifndef PARn

/*---------------------------------------------------------------------*
* Print positive and negative response statistics for all cell types   *
*---------------------------------------------------------------------*/

      tlines(12);

      /* Since a subtitle follows, main heading must be normal print...
      *  (if also a subtitle, would not trigger)  */
      cryout(RK_P2, "0   ", RK_NFSUBTTL+RK_LN2, NULL);
      mxlrn = min(ir->lrn, 68);
      mxlrn2 = mxlrn>>1;
      cryout(RK_P2, blanks, RK_LN1+35-mxlrn2,
         "---SUMMARY STATISTICS FOR ", RK_CCL, getrktxt(ir->hrname),
         mxlrn, " REGION---", RK_CCL, NULL);
      cryout(RK_P2,"0            HIGHEST    MEAN POS   CELLS POS   "
         "A ABOVE PT     LOWEST    MEAN NEG   CELLS NEG   A BELOW NT",
         RK_NFSUBTTL+RK_LN2, underlines, RK_OMITLN+RK_LN1+105,
                   "+CELL TYPE     A(I)       A(I)     PER TRIAL   "
         " PER TRIAL      A(I)       A(I)     PER TRIAL    PER TRIAL",
         RK_LN0, NULL);

      /* Loop over all cell blocks (layers).  Note:  DIDSTATS is
      *  maintained to save double test of il->CTST->nstcap.  */
      for (il=ir->play1; il; il=il->play)
         if (il->ctf & DIDSTATS) {
            struct CLSTAT *pcst = il->CTST;
            double dntr = (double)pcst->nstcap;
            if (pcst->numpos == 0) pcst->hiscor = 0;
            if (pcst->numneg == 0) pcst->loscor = 0;
            lstat[0] = pcst->hiscor;
            lstat[1] = wdivchk(pcst->sumpos,pcst->numpos);
             stat[2] = (double)pcst->numpos/dntr;
             stat[3] = (double)pcst->hitpos/dntr;
            lstat[4] = pcst->loscor;
            lstat[5] = wdivchk(pcst->sumneg,pcst->numneg);
             stat[6] = (double)pcst->numneg/dntr;
             stat[7] = (double)pcst->hitneg/dntr;
            convrt("(P2,3X,A" qLSN ",2B20/27IL12.<4,2Q12.<4,"
               "2B20/27IL12.<4,2Q12.<4)", il->lname, lstat,
               stat+2, lstat+4, stat+6, NULL);
            orkaut |= il->kautu;
            } /* End for-if */

/*---------------------------------------------------------------------*
*  Second group of summary stats: mean response, sharpness, overflows  *
*---------------------------------------------------------------------*/

      if (orkaut & KAUT_ANY) {
         gotas = orkaut & KAUT_ET ? 2 : 1;
         gassw = 12*gotas;
         nassw = 93 + gassw;
         }
      else
         gotas = 0, gassw = RK_SKIP, nassw = 93;
      cryout(RK_P2,"0              MEAN     SHARPNESS     PERCENT    "
         " PERCENT      S(I)       GCONN     CELLS RE-",
         RK_NFSUBTTL+RK_LN2,
         ashd1[gotas], gassw, underlines, RK_OMITLN+RK_LN1+nassw,
                   "+CELL TYPE   RESPONSE   STATISTIC      REFRAC    "
         " SPIKING   TRUNCATED   TRUNCATED   GENERATED", RK_LN0,
         ashd2[gotas], gassw, NULL);

      /* Loop over all cell blocks (again) */
      for (il=ir->play1; il; il=il->play)
         if (il->ctf & DIDSTATS) {
            struct CLSTAT *pcst = il->CTST;
            long nstot = pcst->numpos + pcst->numneg;
            lstat[0] = wdivchk(pcst->sumrsp,nstot);
            if (pcst->nsharp > 0)
               lstat[1] = jdswq(pcst->sharp,pcst->nsharp) -
                  wdivchk(jrsw(pcst->sumrsp,pcst->sharp),
                     nstot - pcst->nsharp);
            else
               lstat[1] = 0;
            lstat[2] = ds64nq(pcst->nrfrc,0,-IBwk,nstot);
            lstat[3] = ds64nq(pcst->nspike,0,-IBwk,nstot);
            lstat[4] = il->CTST->sover;
            lstat[5] = il->CTST->gmover;
            lstat[6] = il->CTST->nregen;
            convrt("(P2,3X,A" qLSN ",#7,2B20/27IL12.<4,2B20IL12.<4,"
               "3IL12)", il->lname, lstat, NULL);
            switch (gotas) {
            case 2:
               if (il->kautu & KAUT_ET)   sconvrt(ascl,
                  "(O,2B24IJ12.<5)", &il->CTST->loascl, NULL);
               else                       sconvrt(ascl,
                  "(B24IJ18.<5)", RP->psclmul+il->oautsc, NULL);
               cryout(RK_P2, ascl, RK_CCL+24, NULL);
               break;
            case 1:                       sconvrt(ascl,
                  "(B24IJ12.<5)", RP->psclmul+il->oautsc, NULL);
               cryout(RK_P2, ascl, RK_CCL+12, NULL);
               } /* End gotas switch */
            } /* End for-if */
#endif

/*---------------------------------------------------------------------*
*                Distributions and response components                 *
*---------------------------------------------------------------------*/

      for (il=ir->play1; il; il=il->play) if (il->ctf & DIDSTATS) {
#ifndef PARn
         struct INHIBBLK *ib;    /* Ptr to current inhibition block */
         struct MODBY *imb;      /* Ptr to current modby block */
         ddist  *pdd;            /* Ptr to double stats */
         double dnctr;           /* Cells times cycles|trials */
         long nnel = il->nelt;   /* # elements in layer */
         long nctr = il->CTST->nstcap*nnel;
#endif
         ui32 lctp = il->Ct.kctp;/* Copy of option bits */

#ifndef PARn
         dnctr = (double)nctr;

         /* Print title for response components */
         tlines(12);
         cryout(RK_P2, "0   ", RK_NFSUBTTL+RK_LN2, NULL);
         mxlrn = min(ir->lrn, 78);
         mxlrn2 = mxlrn>>1;
         cryout(RK_P2, blanks, RK_LN1+40-mxlrn2,
            "---DETAILED STATISTICS FOR ", RK_CCL,
            getrktxt(ir->hrname), mxlrn, " REGION, ", RK_CCL,
            il->lname, LSNAME, " CELL TYPE---", RK_CCL, NULL);
         cryout(RK_P2, "0DISTRIBUTIONS AND RESPONSE COMPONENTS",
            RK_NFSUBTTL+RK_LN2, underlines, RK_OMITLN+RK_LN1+133,
            prange, RK_LN0, NULL);

         /* Loop over connection types, printing A distributions.
         *  Distributions are accumulated from the unscaled rsumaij
         *  in d3go() and scaled here so the scaling is done only
         *  once per bin rather than once per connection.  Autoscale
         *  cannot be taken into account with this scheme, but most
         *  likely this is what user wants anyway and should be so
         *  documented.
         *  Rev, 12/15/07, GNR - Revert to floating point to avoid
         *     overflow problem when result actually does not fit
         *     in B20/27 format of original--may give E format now.
         */
         for (ix=il->pct1; ix; ix=ix->pct) {
            char *fctn = fmtict((int)ix->ict);  /* Conntype number */
            /* Here is a little special version of avghist() to do
            *  the scaling.  Avg V = Total voltage/num of cells w/si
            *  in this bin.  If conntype input were turned off by an
            *  ntcs option, this statistic will reflect that fact,
            *  as it should.  To count only cycles with active input
            *  would need an extra sdist array for each conntype.  */
            double tscl = (double)ix->Cn.scl/dS24;
            int iad;
            setbscompat(ix->cnsrctyp);
            pdd = il->pctddst + ix->ocnddst + OaVd;
            for (iad=0; iad<LDSTAT; ++iad) {
               long niad = il->CTST->sdist[iad];
               if (niad) {
                  double tsum = swdbl(pdd->d[iad])/rawscl;
                  stat[iad] = tscl*tsum/(double)niad; }
               else
                  stat[iad] = 0.0;
               }
            convrt("(P2,2H A,A3,16Q8.<4)", fctn, stat, NULL);
            if (ix->Cn.ADCY.kdcy) {
               ++pdd;
               avghist(pdd->d, il->CTST->sdist, lstat);
               convrt("(P2,2H W,A3,16B20/27IL8.<4)", fctn, lstat, NULL);
               } /* End persistence distribution */
            if (cnexists(ix, RBAR)) {
               ++pdd;
               avghist(pdd->d, il->CTST->sdist, lstat);
               convrt("(P2,2H R,A3,16B7|14IL8.<4)", fctn, lstat, NULL);
               } /* End rbar distribution */
            } /* End loop over connection types */

         /* Loop over inhibition blocks, printing I distributions */
         for (ib=il->pib1; ib; ib=ib->pib) {
            char *fctn = fmtict((int)ib->igt);     /* GCONN number */
            int ibcnt;                             /* Band counter */
            pdd = il->pctddst + ib->oidist;
            for (ibcnt=0; ibcnt<ib->nib; ibcnt++,pdd++) {
               avghist(pdd->d, il->CTST->sdist, lstat);
               if (ibcnt == 0)
                  convrt("(P2,2H I,A3,16B20/27IL8.<4)",
                     fctn, lstat, NULL);
               else
                  convrt("(P2,5X,16B20/27IL8.<4)", lstat, NULL);
               } /* End for ibcnt */
            if (ib->GDCY.kdcy) {
               avghist(pdd->d, il->CTST->sdist, lstat);
               convrt("(P2,2H W,A3,16B20/27IL8.<4)", fctn, lstat, NULL);
               } /* End GCONN persistence distribution */
            } /* End loop over GCONNs */

         /* Loop over modulation blocks, printing M distributions */
         for (ict=1, imb=il->pmby1; imb; ict++, imb=imb->pmby) {
            char *fctn = fmtict((int)ict);
            pdd = il->pctddst + imb->omdist;
            avghist(pdd[OmVd].d, il->CTST->sdist, lstat);
            convrt("(P2,2H M,A3,16B20/27IL8.<4)", fctn, lstat, NULL);
            if (imb->MDCY.kdcy) {
               avghist(pdd[OmWd].d, il->CTST->sdist, lstat);
               convrt("(P2,2H W,A3,16B20/27IL8.<4)", fctn, lstat, NULL);
               } /* End MOD persistence distribution */
            } /* End loop over modby blocks */

         /* Get ready to print stats for cell-level variables */
         pdd = il->pctddst;
         /* Print Izhikevich 'u' = Brette-Gerster 'w' histogram */
         if (il->Ct.rspmeth >= RF_BREG) {
            avghist(pdd->d, il->CTST->sdist, lstat);
            convrt("(P2,' RFUW',16B7/14IL8.<4)", lstat, NULL);
            ++pdd;
            }
         /* Print persistence histogram (if not IZHI) */
         else if (il->Dc.CDCY.kdcy) {
            avghist(pdd->d, il->CTST->sdist, lstat);
            convrt("(P2,' PERS',16B20/27IL8.<4)", lstat, NULL);
            ++pdd;
            }

         /* Print depression histogram */
         if (il->ctf & CTFDN) {
            avghist(pdd->d, il->CTST->sdist, lstat);
            convrt("(P2,' DEPR',16B8IL8.<4)", lstat, NULL);
            ++pdd;
            }

         /* Print spike-threshold histogram */
         if (il->prfrc) {
            avghist(pdd->d, il->CTST->sdist, lstat);
            convrt("(P2,' DSpT',16B20/27IL8.<4)", lstat, NULL);
            ++pdd;
            }

         /* Print s(i) distribution */
         {  register long *jsv=il->CTST->sdist, *jle=jsv+LDSTAT;
            register double *stp=stat;
            for ( ; jsv < jle; jsv++) *stp++ = divchk(*jsv,dnctr);
            convrt("(P2,' DIST',16Q8.5)", stat, NULL);
            }

         /* Generate and print cumulative distribution */
         {  register long *jge=il->CTST->sdist, *jsv=jge+LDSTAT-1;
            register double nstot=0.0, *stp=stat+LDSTAT-1;
            for ( ; jsv >=jge ; jsv--) {
               nstot += (double)(*jsv);
               *stp-- = divchk(nstot,dnctr); }
            convrt("(P2,' CUME',16Q8.5)", stat, NULL);
            }

#ifndef PAR
         /* The high statistic cannot be gathered in parallel
         *  without more trouble than it's worth */
         convrt("(P2,' HIGH',16IL8)",il->CTST->hdist,NULL);
#endif

         /* Print C distribution (response vs. stimulus class).
         *  The data are gathered in d3go as a function of stimulus
         *  isn, which is related here to stimulus class.  Inasmuch
         *  as pxxxx array may not exist, must test Cdist itself to
         *  determine which stimuli were actually presented.  */
         if (lctp & KRPCL) {
            dist *pda,*pda1,*pda0 = il->pctdist + il->oCdist;
            pncl = STAT.pgrpncl;    /* Ptr to no. classes per group */
            for (pmdlt=RP->pmdlt1; pmdlt; pmdlt=pmdlt->pnmdlt) {
               struct GSDEF *pgs;   /* Ptr to current group data */
               struct GRPDEF *pgpn; /* Ptr to GRPNO chain */
               int ist, itrig;
               /* Only consider modalities sensed by this celltype */
               mgpnt = NStimGrps(pmdlt);
               if (!bittst((unsigned char *)&il->rmdlts,
                  (long)pmdlt->mdltno)) goto SkipModalityForC;
               pda1 = pda0;         /* Save ptr to 1st dist in mdlt */
               itrig = TRUE;
               /* Loop over stimuli presented via this modality */
               for (ist=1; ist<=pmdlt->nds; ist++,pda0++) {
                  if (normhist(pda0->d, stat)) {
                     if (itrig) {      /* Label the modality */
                        cryout(RK_P2, "0S(I) DISTRIBUTION FOR ", RK_LN2+
                           RK_NFSUBTTL, getrktxt(pmdlt->hmdltname),
                           LTEXTF, " MODALITY", RK_CCL, cimw,
                           fmtmwdw(pmdlt), " BY STIMULUS ID", RK_CCL,
                           NULL);
                        itrig = FALSE;
                        }
                     convrt("(P2H I3H 16Q8.5)", &ist, stat, NULL);
                     }
                  } /* End stimulus loop */

               /* Loop over GRPNO sets.  As w/KRPXR stats, must incr.
               *  pgs over ncs GSDEFs per GRPNO, even though there
               *  may not be that many groups in any one GRPNO set.  */
               pgs = pmdlt->pgsd;
               pgpn = pmdlt->pgpno;          /* Ptr to GRPNO chain */
               lndsb = pmdlt->ndsbyt;        /* Bytes in a grpmsk  */
               for (igpnt=1; igpnt<=mgpnt; ++igpnt,pgs+=pmdlt->ncs) {
                  struct GSDEF *qgs = pgs;
                  short *pgpnl = pgpn->grp;  /* Ptr to GRPNO list */
                  dist tCdst;                /* Aggregate Cdist */
                  int ncl = pncl[igpnt-1];
                  int gpid = 0;              /* Print group number */
                  /* Loop over the stimulus groups in the GRPNO set */
                  itrig = TRUE;
                  for (ist=0; ist<ncl; ++ist,++qgs) {
                     long i;
                     pda = pda1;          /* Ptr to first dist */
                     /* Clear the aggregate stats */
                     memset((char *)&tCdst, 0, sizeof(dist));
                     /* Loop over stimuli, sum dists for this set */
                     for (i=1; i<=(long)pmdlt->nds; ++i,++pda)
                           if (bittst(qgs->grpmsk, i)) {
                        int j;
                        for (j=0; j<LDSTAT; ++j)
                           tCdst.d[j] += pda->d[j];
                        gpid = (int)pgpnl[i-1];
                        }
                     /* Print aggregate statistics */
                     if (normhist(tCdst.d, stat)) {
                        if (itrig) {      /* Label the GRPNO */
                           char asetno[6];
                           wbcdwt(&igpnt, asetno,
                              RK_NUNS+RK_LFTJ+RK_IORF+RK_NHALF+5);
                           cryout(RK_P2, "0S(I) DISTRIBUTION BY STIMU"
                              "LUS GROUP FOR ", RK_NFSUBTTL+RK_LN4,
                              getrktxt(pmdlt->hmdltname), LTEXTF,
                              " MODALITY, GRPNO SET ", RK_CCL, asetno,
                               RK.length+1, NULL);
                           itrig = FALSE;
                           }
                        convrt("(P2H I3H 16Q8.5)", &gpid, stat, NULL);
#if 1    /*** DEBUG ***/
   {  double tot = 0.0, totn = 0.0, totp = 0.0;
      int j;
      for (j=0; j<LDSTAT; ++j) {
         tot += stat[j] * ((double)j + 0.5);
         }
      for (j=4; j<LDSTAT; ++j) {
         totn += stat[j];
         totp += stat[j] * ((double)j + 0.5);
         }
      tot /= 16.0, totp /= (16.0*totn);
      convrt("(P2,' MEAN RESPONSES ',2*Q10.6)", &tot, &totp, NULL);
      }
#endif
                        }
                     } /* End ist loop over stimulus groups */
                  pgpn = pgpn->pnxgrp;
                  } /* End NStimGrps loop */
               /* Be careful to increment pncl even for skipped modalities */
SkipModalityForC:
               pncl += mgpnt;
               } /* End MODALITY loop */
            } /* End KRPCL output */

         if (lctp & KRPFD) {
            /* Print s(i) dist. vs. feature-detector matrix number */
            dist *pda;              /* Ptr to distribution array */
            for (ix=il->pct1; ix; ix=ix->pct) {
               int i, iinc, itop, itrig;
               if (!(ix->cnflgs & DOFDIS)) continue;
               iinc = ix->ucij.m.nks;
               itop = iinc * ix->ucij.m.nkt;
               itrig = TRUE;
               pda = il->pctdist + ix->ucij.m.oFdist;
               /* Loop over nkt matrices */
               for (i=1; i<=itop; i+=iinc,pda++) {
                  if (normhist(pda->d, stat)) {
                     if (itrig) {      /* Label the FDM */
                        convrt("(P2,'0S(I) DISTRIBUTION BY FEATURE "
                           "DETECTING MATRIX, CONNTYPE ',JIH4)",
                           &ix->ict, NULL);
                        itrig = FALSE;
                        }
                     convrt("(P2H I3H 16Q8.5)", &i, stat, NULL);
                     }
                  } /* End loop over FDMs */
               } /* End connection type loop */
            } /* End KRPFD output */

         if (il->phshft) {
            register dist *pFdist = il->pctdist + il->oFdist;
            cryout(RK_P2, "0DISTRIBUTION OF PHASES", RK_NFSUBTTL+RK_LN2,
               underlines, RK_OMITLN+RK_LN1+133,
                  "+PHI <   22.5    45.0    67.5    90.0   112.5"
               "   135.0   157.5   180.0   202.5   225.0   247.5"
               "   270.0   292.5   315.0   337.5   360.0",
               RK_LN0, NULL);
            convrt("(P2,' DIST',16IL8)",pFdist[OcFd].d,NULL);
            cryout(RK_P2, "0DISTRIBUTION OF PHASE DIFFERENCES",
               RK_NFSUBTTL+RK_LN2, underlines, RK_OMITLN+RK_LN1+133,
                  "+|DPHI|<=11.3    22.5    33.8    45.0    56.3"
               "    67.5    78.8    90.0   101.3   112.5   123.8"
               "   135.0   146.3   157.5   168.8   180.0",
               RK_LN0, NULL);
            convrt("(P2,' DIST',16IL8)",pFdist[OcDFd].d,NULL);
            }

         /* Print distributions of Cij's--these moved to
         *  separate loop in V1K so can have meaningful titles.  */
         cryout(RK_P2,"0DISTRIBUTION OF CONNECTION STRENGTHS",
            RK_NFSUBTTL+RK_LN2, underlines, RK_OMITLN+RK_LN1+133,
               "+ CIJ   -0.94   -0.81   -0.69   -0.56   -0.44"
            "   -0.31   -0.19   -0.06    0.06    0.19    0.31"
            "    0.44    0.56    0.69    0.81    0.94", RK_LN0,  NULL);

         for (ix=il->pct1; ix; ix=ix->pct) {
            /* Use doubles here to avoid overflow in big runs */
            double dntrca = (double)((ix->cnflgs & DIMDLT) ?
               ix->psmdlt->um1.MS.ntract : il->CTST->nstcap) *
               (double)nnel;
            double dcskip = dntrca * (double)ix->nc;
            if (dcskip > 0.0) {
               if (!(ix->Cn.kam & KAMBY)) {
                  register double *stp = stat;
                  register si64 *jls = il->pctddst[ix->ocnddst+OaCd].d;
                  si64 *jle = jls + LDSTAT;
                  double dnum,dden = 1.0/dcskip;
#if 0
/*** DEBUG ***/   convrt("(P1,#32,4H SUM|4(3X,Z12,X,Z12)/)", jls, NULL);
#endif
                  for ( ; jls < jle; jls++) {
                     dnum = swdbl(jls[0]);
                     dcskip -= dnum;
                     *stp++ = dnum * dden; }
                  convrt("(P2,2H C,A3,8Q8.5,8Q8.5)",
                     fmtict((int)ix->ict), stat+8, stat, NULL);
                  } /* End not bypassing */
               ix->cnwk.stat.fcskip = dcskip/dntrca;
               ix->cnwk.stat.fnppfh = (double)ix->CNST.nppfh/dntrca;
               } /* End dcskip if */
            else
               ix->cnwk.stat.fcskip = ix->cnwk.stat.fnppfh = 0.0F;
            } /* End ix for */

         /* Print distributions of Sj's */
         cryout(RK_P2,"0DISTRIBUTION OF INPUT Sj",
            RK_NFSUBTTL+RK_LN3, underlines, RK_OMITLN+RK_LN1+133,
               "  mV  <  -112     -96     -80     -64     -48"
            "     -32     -16       0      16      32      48"
            "      64      80      96     112     256", RK_LN0,
               " OldS < -0.88   -0.75   -0.62   -0.50   -0.38"
            "   -0.25   -0.12       0    0.12    0.25    0.38"
            "    0.50    0.62    0.75    0.88    2.00", RK_LN0,  NULL);

         for (ix=il->pct1; ix; ix=ix->pct) {
            si64 *pnsj = il->pctddst[ix->ocnddst + OaSd].d;
            si64 totsj = jesl(0);
            double dtot;
            int  isj;
            for (isj=0; isj<LDSTAT; ++isj)
               totsj = jasw(totsj, pnsj[isj]);
            if (qsw(totsj) == 0) continue;
            /* Use doubles--easier than vdivl for 64-bit division */
            dtot = swdbl(totsj);
            for (isj=0; isj<LDSTAT; ++isj)
               stat[isj] = swdbl(pnsj[isj]) / dtot;
            convrt("(P2,2H A,A3,16Q8.5)",
               fmtict((int)ix->ict), stat, NULL);
            } /* End ix for */

/*---------------------------------------------------------------------*
*  Print other connection-type statistics that are independent         *
*     of amplification:  number of connections skipped and number      *
*     of PPF hits.                                                     *
*  These are printed even if conns bypassed, as a sort of reminder.    *
*---------------------------------------------------------------------*/

         cryout(RK_P2, "0OTHER CONNECTION DATA (PER CELL PER TRIAL)",
            RK_NFSUBTTL+RK_LN2, underlines, RK_OMITLN+RK_LN1+46,
            "+CONNTYPE    NUMBER SKIPPED    NUMBER PPF HITS",
            RK_LN0, NULL);
         for (ix=il->pct1; ix; ix=ix->pct) {
            if (ix->cnwk.stat.fcskip > 0.0F ||
                  ix->cnwk.stat.fnppfh > 0.0F) {
               convrt("(P2,4H   C,A3,F16.5,F18.5)",
                  fmtict((int)ix->ict), &ix->cnwk.stat.fcskip,
                  &ix->cnwk.stat.fnppfh, NULL);
               }
            } /* End ix for */

/*---------------------------------------------------------------------*
*             Amplification statistics by connection type              *
*---------------------------------------------------------------------*/

/* These statistics were revised in V8D, 07/25/07, in an attempt to
*  make them more useful for tuning.  Mean delta Cij and counts of
*  Mij over mxmij and Mijdecr over mxmp were added.  Delta Cij stats
*  are now recorded before any Cij averaging and do not include any
*  contributions due to decay, truncation, or random rounding.  Amp
*  stats are accumulated for all cycles when amplification is active,
*  so are normalized per cycle and per cell.  */

         if (!(RP->CP.runflags & RP_NOCHNG)) {
            float aden,avgp,nump,avgn,numn,avgo,avgc,nmov[2];
            long nm;
            int hlen, rpct;
            hlen = (il->orkam & (KAMSA|KAMTS|KAMUE)) ? 126 : 93;
            cryout(RK_P2, "0SYNAPTIC CHANGE STATISTICS "
               "(PER CELL PER CYCLE)", RK_NFSUBTTL+RK_LN2,
               underlines, RK_OMITLN+RK_LN1+hlen,
               "+TYPE   MEAN POS DELCij       N     MEAN NEG DELCij"
               "       N      Cij TRUNCATED   SIGN CHANGES  "
               "  Mij TRUNCATED   MXMP EXCEEDED", RK_LN0+hlen, NULL);

            for (ix=il->pct1; ix; ix=ix->pct) {
               aden = (float)ix->CNST.nampc * (float)nnel;
               if (aden > 0.0F) {
                  rpct = (ix->Cn.kam & (KAMSA|KAMTS|KAMUE)) ? 2 : 0;
                  aden = 1.0F/aden;
                  nm = ix->CNST.nmod[EXCIT];
                  avgp = (nm > 0) ? (float)jdswq(
                     ix->CNST.sdcij[EXCIT],nm)/dS16 : 0.0;
                  nump = (float)nm*aden;
                  nm = ix->CNST.nmod[INHIB];
                  avgn = (nm > 0) ? (float)jdswq(
                     ix->CNST.sdcij[INHIB],nm)/dS16 : 0.0;
                  numn = (float)nm*aden;
                  avgo = (float)ix->CNST.cover*aden;
                  avgc = (float)ix->CNST.cschg*aden;
                  if (rpct) {
                     nmov[0] = (float)ix->CNST.mover*aden;
                     nmov[1] = (float)ix->CNST.mpover*aden; }
                  convrt("(P2,2H C,A3,5X,VF10.<5,VF12.<5,6X,VF10.<5,"
                     "VF12.<5,2*VF16.<5,RVF16.<5)",
                     fmtict((int)ix->ict), &avgp, &nump, &avgn,
                     &numn, &avgo, &avgc, &rpct, &nmov, NULL);
                  } /* End if */
               } /* End ix for */
            } /* End NOCHNG if */

/*---------------------------------------------------------------------*
*  Detailed amplification statistics                                   *
*                                                                      *
*  DAS entries are as follows:                                         *
*     (1) Counts for the nine cases                                    *
*     (2) Total (si-mti) for the nine cases (S7 mV/S14)                *
*     (3) Total (sj-mtj) for the nine cases (S7 mv/S14)                *
*     (4) Total DeltaCij for the nine cases (S16)                      *
*     (5) Only if value being used (KAMVM|KAMCG|KAMUE):                *
*         Total (v-mtv) for the nine cases   (S8)                      *
*     (5|6) Only if Mij being calculated (KAMSA|KAMTS|KAMUE):          *
*         Total Mij for the nine cases      (S14)                      *
*                                                                      *
*  Rev, 06/10/03, GNR - DAS stats changed from double to si64 for      *
*     possible better speed during accumulation and consistency        *
*     across different float reps.                                     *
*  Rev, 07/24/07, GNR - Add Mij stats, make Value and Mij conditional  *
*  Rev, 04/02/12, GNR - Add DeltaCij, use defined constant offsets,    *
*     allow > 32 bits in counts if using 64-bit longs                  *
*  Note:  It would be nice to omit the -xx cases if no KAMVM, but a    *
*     celltype can have mixed conntypes with and without value, which  *
*     makes it difficult to set the subtitle for both.                 *
*---------------------------------------------------------------------*/

         if (il->orkam & KAMDS) {
            cryout(RK_P2, "0MEAN AMPLIFICATION TERMS BY WAYSET CASE",
               RK_NFSUBTTL+RK_LN2, underlines, RK_OMITLN+RK_LN1+95,
               "+CASE       +++       ++-       +-+       +--"
                    "       -++       -+-       --+       ---"
                    "     INELG", RK_LN0, NULL);

            for (ix=il->pct1; ix; ix=ix->pct)
                  if ((ix->Cn.kam & (KAMBY|KAMDS)) == KAMDS) {
               si64 *pd = ix->pdas;
               char *fctn = fmtict((int)ix->ict);
               long mdas[6*LDR], ndas, *pld, *pld2 = mdas + LDR;
               int  idr, ndr = ix->ldas/sizeof(*(ix->pdas));
               memset((char *)mdas, 0, sizeof(mdas));
               setbscompat(ix->cnsrctyp);
               for (pld=mdas; pld<pld2; pd++,pld++) {
#if LSIZE == 8
                  ndas = (long)pd[0];
#else
                  ndas = swlo(pd[0]);
#endif
                  if (ndas <= 0) continue;
                  pld[0] = ndas;
                  for (idr=LDR; idr<ndr; idr+=LDR)
#if LSIZE == 8
                     pld[idr] = (long)pd[idr] / ndas;
#else
                     pld[idr] = jdswq(pd[idr], ndas);
#endif
                  }
               if (ix->Cn.kam & (KAMVM|KAMCG|KAMUE)) convrt("(P2,"
                  "2H V,A3,9B8IL10.<4)", fctn, mdas+OdasVM, NULL);
               if (ix->Cn.kam & (KAMSA|KAMTS|KAMUE)) convrt("(P2,"
                  "2H M,A3,9B7|14IL10.<4)", fctn, mdas+ndr-LDR, NULL);
               convrt("(P2,2H I,A3,9B7/14IL10.<4/"
                          "2H J,A3,9B7|14IL10.<4/"
                          "5H dCij,9B16IL10.<4/5H DIST,9IL10)",
                  fctn,mdas+OdasSi, fctn,mdas+OdasSj,
                  mdas+OdasDC, mdas, NULL);
               } /* End if das stats exist and not bypassed */
            } /* End detailed amp stats */

/*---------------------------------------------------------------------*
*       Correct categorization statistics for KAMCG modalities         *
*   (Below this point, 3 levels of indenting have been suppressed)     *
*---------------------------------------------------------------------*/

if (il->orkam & KAMCG) {
   cryout(RK_P2, "0CORRECT CATEGORIZATIONS BY MODALITY",
      RK_NFSUBTTL+RK_LN2, underlines, RK_OMITLN+RK_LN1+43,
      "+MODALITY          NUM CORRECT  PCT CORRECT", RK_LN0, NULL);
   for (ix=il->pct1; ix; ix=ix->pct) if (ix->Cn.kam & KAMCG) {
      float fccat = (float)ix->nccat/(float)il->CTST->nstcap;
      convrt("(P2X,A16,IJ13,F13.5)", getrktxt(ix->psmdlt->hmdltname),
         &ix->nccat, &fccat, NULL);
      }
   } /* End KAMCG categorization statistics */

/*---------------------------------------------------------------------*
*           KRPGP and KRPMG ("Additional group statistics")            *
*---------------------------------------------------------------------*/

/* Print M statistics (max/min response vs. stimulus id).  The GPSTAT
*  data are gathered in d3go as a function of stimulus isn.  Aggregate
*  and print by stimulus group.  (The loop over stimuli is repeated
*  for each GRPNO set to avoid having to allocate an array of GPSTATs.
*  Typically there will be only a few sets, so this is not so bad.  */

if (lctp & (KRPGP|KRPMG)) {
   struct GPSTAT *pgp,*pgp0 = CDAT.pgstat + il->oGstat;
   pncl = STAT.pgrpncl;    /* Ptr to number classes per group */
   for (pmdlt=RP->pmdlt1; pmdlt; pmdlt=pmdlt->pnmdlt) {
      struct GSDEF *pgs;   /* Ptr to current group data */
      struct GRPDEF *pgpn; /* Ptr to GRPNO chain */
      double hpt;          /* Hits per trial */
      int ist, itrig;      /* Stimulus counter, title trigger */
      /* Only consider modalities sensed by this celltype */
      mgpnt = NStimGrps(pmdlt);
      if (!bittst((unsigned char *)&il->rmdlts,
         (long)pmdlt->mdltno)) goto SkipModalityForG;
      if (lctp & KRPGP)        goto SkipModalityForM;
      pgp = pgp0;
      itrig = TRUE;
      /* Loop over stimuli presented via this modality */
      for (ist=1; ist<=pmdlt->nds; ist++,pgp++) {
         if (pgp->nstim > 0) {
            if (itrig) {      /* Label the modality */
               cryout(RK_P2, "0MAX/MIN RESPONSES BY STIMULUS ID FOR ",
                  RK_NFSUBTTL+RK_LN3, getrktxt(pmdlt->hmdltname),
                  LTEXTF, " MODALITY", RK_CCL, cimw, fmtmwdw(pmdlt),
                  underlines, RK_OMITLN+RK_LN0+62,
                  "+ STIM ID  MAX S(i)  LowMAXSi  MIN S(i)  "
                  "PRESENTED  HITS/TRIAL", RK_LN0, NULL);
               itrig = FALSE;
               }
            hpt = (double)pgp->nhits/(double)pgp->nstim;
            convrt("(P2H I8,3B20/27IJ10.<4,IJ11,Q12.<4)", &ist,
               &pgp->hiresp, &pgp->nstim, &hpt, NULL);
            }
         } /* End stimulus loop */

      /* Loop over GRPNO sets.  As w/KRPXR stats, must increment
      *  pgs over ncs GSDEFs per GRPNO, even though there may not
      *  be that many groups in any one GRPNO set.  */
SkipModalityForM:
      pgs = pmdlt->pgsd;
      pgpn = pmdlt->pgpno;          /* Ptr to GRPNO chain */
      lndsb = pmdlt->ndsbyt;        /* Bytes in a grpmsk  */
      for (igpnt=1; igpnt<=mgpnt; ++igpnt,pgs+=pmdlt->ncs) {
         struct GSDEF *qgs = pgs;
         short *pgpnl = pgpn->grp;  /* Ptr to GRPNO list */
         struct GPSTAT tgps;        /* Temp GPSTAT for aggregating */
         si32 hihiresp;             /* Added HighMAXSi stat */
         int ncl = pncl[igpnt-1];
         int gpid = 0;              /* Group number for printing */
         /* Loop over the stimulus groups in the GRPNO set */
         itrig = TRUE;
         for (ist=0; ist<ncl; ++ist,++qgs) {
            long i;
            pgp = pgp0;          /* Ptr to first GPSTAT in modality */
            /* Clear the aggregate stats */
            tgps.nstim = tgps.nhits = 0;
            hihiresp = tgps.hiresp = -SI32_MAX;
            tgps.lohiresp = tgps.loresp = SI32_MAX;
            /* Loop over all stimuli, gather stats for this set */
            for (i=1; i<=(long)pmdlt->nds; ++i,++pgp)
                  if (bittst(qgs->grpmsk, i)) {
               tgps.nstim += pgp->nstim;
               tgps.nhits += pgp->nhits;
               if (pgp->hiresp > tgps.hiresp)
                  tgps.hiresp = pgp->hiresp;
               if (pgp->lohiresp < tgps.lohiresp)
                  tgps.lohiresp = pgp->lohiresp;
               if (pgp->lohiresp > hihiresp)
                  hihiresp = pgp->lohiresp;
               if (pgp->loresp < tgps.loresp)
                  tgps.loresp = pgp->loresp;
               gpid = (int)pgpnl[i-1];
               }
            /* Print aggregate statistics */
            if (tgps.nstim > 0) {
               if (itrig) {      /* Label the GRPNO */
                  char asetno[6];
                  wbcdwt(&igpnt, asetno,
                     RK_NUNS+RK_LFTJ+RK_IORF+RK_NHALF+5);
                  cryout(RK_P2, "0MAX/MIN RESPONSES BY STIMULUS GROUP"
                     " FOR ", RK_NFSUBTTL+RK_LN4,
                     getrktxt(pmdlt->hmdltname), LTEXTF, " MODALITY, "
                     "GRPNO SET ", RK_CCL, asetno, RK.length+1,
                     "                    Max(Stim) Min(Stim)",
                     RK_LN0+39, underlines, RK_OMITLN+RK_LN0+72,
                     "+ GRP NUM  MAX S(i)  LowMAXSi  LowMAXSi  MIN "
                     "S(i)  PRESENTED  HITS/TRIAL", RK_LN0, NULL);
                  itrig = FALSE;
                  }
               hpt = (double)tgps.nhits/(double)tgps.nstim;
               convrt("(P2H I8,B20/27IJ10.<4,B20/27IJ10.<4,2B20/27"
                  "IJ10.<4,IJ11,Q12.<4)", &gpid, &tgps.hiresp,
                  &hihiresp, &tgps.lohiresp, &tgps.nstim, &hpt, NULL);
               }
            } /* End ist loop over stimulus groups */
         pgpn = pgpn->pnxgrp;
         } /* End NStimGrps loop */
      /* Be careful to increment pncl even for skipped modalities */
SkipModalityForG:
      pncl += mgpnt;
      } /* End MODALITY loop */
   } /* End KRPGP output */

#endif /* !def PARn */

/*=====================================================================*
*           Response history and cross-response statistics             *
*=====================================================================*/

#ifdef PAR
/* Calculate number of items that need to be collected */
xstodo = 0;

if (lctp & KRPHR)
   xstodo += LHSTAT * il->nmdlts;
#endif

if (lctp & (KRPXR|KRPYM|KRPZC) & xopt) {
#ifndef PAR0
   /* Make looping constants for examining repertoire data */
   lllt = il->llt;         /* Copy of prd length  */
   g1 = il->prd;           /* Locate repertoire data */
   g2 = g1 + lllt;         /* End of repertoire data */
   ginc = il->lel;         /* Length of data for one cell */
   oxrm = il->ctoff[XRM];  /* Offset to XRM data */
   /* Offset to first bit of current modality in XRM data */
   ioff1 = 1L;
#endif

/*---------------------------------------------------------------------*
* Give cross-response statistics for all relevant sensory modalities   *
*---------------------------------------------------------------------*/

#ifdef PAR
/* If parallel, make a preliminary scan through all modalities this
*  celltype responds to and calculate the number of items that will
*  be gathered by xstpack() and xstunpk().  This allows xrstats for
*  all modalities to be buffered together and may reduce the number
*  of vecollects needed.   There does not seem to be any reasonable
*  way to make xstunpk() anticipate the number of items it will get.
*  Since nst1 may differ in each trial series, there is no point to
*  trying to save the xstodo count across series.  */

   pncl = STAT.pgrpncl;
   for (pmdlt=RP->pmdlt1; pmdlt; pmdlt=pmdlt->pnmdlt, pncl+=mgpnt) {
      mgpnt = NStimGrps(pmdlt);     /* Number GRPNO cards */
      /* Omit modalities that this celltype does not respond to */
      if (!bittst((unsigned char *)&il->rmdlts, (long)pmdlt->mdltno))
         continue;
      /* Omit modalities with fewer than two stimuli applied */
      if (bitcnt(pmdlt->pxxxx, (long)pmdlt->ndsbyt) < 2)
         continue;
      nst1 = pmdlt->um1.MS.mxisn;   /* Highest isn encountered */
      /* Cross-response statistics are generated if any of
      *  the 'X', 'Y', or 'Z' options have been requested.
      *  ('Z' is dependent on pnr array made by 'X')  */
      xstodo += mgpnt*XSTASIZ + nst1;
      for (igpnt=0; igpnt<mgpnt; igpnt++) xstodo += pncl[igpnt];
      /* Space for triangular cross-response matrices */
      if (lctp & (KRPYM|KRPZC)) xstodo += nst1*(nst1+1)>>1;
      } /* End modality loop */
#endif
   } /* End calculating looping constants and xstodo */

#ifdef PAR
/* Initialize allocation of collection buffers */
if (xstodo) {
#ifdef PARn
   /* On comp nodes, allocate the first buffer */
   long ll = min(xstodo*sizeof(long), MAX_MSG_LENGTH);
   bptr = buff = (long *)angetpv(ll, "XR stat buff");
   btop = buff + ll/sizeof(long);
#else
   /* On host, set xstunpk to allocate */
   buff = btop = 0;
#endif
   }
#endif   /* PAR */

if (lctp & xopt) {
   pncl = STAT.pgrpncl;
   phr = il->phrs;
   for (pmdlt=RP->pmdlt1; pmdlt; pmdlt=pmdlt->pnmdlt, pncl+=mgpnt) {

      struct GSDEF *pgs;         /* Ptr to current group data */
      long *knr;                 /* Ptr to response-by-stim counts */
#ifndef PARn
      long *knr2;                /* Limit for knr loops */
      long nstimu;               /* No. of unique stimuli presented */
#endif
#ifndef PAR0
      long oxrm1,oxrm2;          /* Byte offsets to response data */
      long joff1,joff2;          /* Zero-base offsets into oxrm */
      int  lxrml,lxrmr;          /* Left,right shifts to align oxrm */
      byte mrm1,mrm2;            /* Modality response masks */
#endif

      mgpnt = NStimGrps(pmdlt);  /* Number GRPNO cards */
      /* Omit modalities that this celltype does not respond to */
      if (!bittst((unsigned char *)&il->rmdlts, (long)pmdlt->mdltno))
         continue;

      /* Skip to response history if no cross-response stuff */
      if (!(lctp & (KRPXR|KRPYM|KRPZC) & xopt))
         goto SkipModality;

      /* Omit modalities with fewer than two stimuli applied */
#ifdef PARn
      if (bitcnt(pmdlt->pxxxx, (long)pmdlt->ndsbyt) < 2)
         goto SkipModality;
#else
      if ((nstimu = bitcnt(pmdlt->pxxxx, (long)pmdlt->ndsbyt)) < 2) {
         cryout(RK_P2,"0Skipping ", RK_LN2, getrktxt(pmdlt->hmdltname),
            LTEXTF, " modality--fewer than 2 stimuli presented.",
            RK_CCL, NULL);
         goto SkipModality;
         }
#endif

      lndsb = pmdlt->ndsbyt;        /* Bytes to hold nds bits */
      nst1 = pmdlt->um1.MS.mxisn;   /* Highest isn encountered */
#ifndef PAR0
#if BITSPERBYTE != 8
#error Rewrite code for odd byte size
#endif
      /* Parameters needed to isolate bits for this modality */
      ioff2 = ioff1 + nst1 - 1;  /* Last bit to be tested */
      joff1 = ioff1 - 1;
      joff2 = ioff2 - 1;
      lxrml = (int)(joff1 & 7);
      lxrmr = (int)(8 - lxrml);
      oxrm1 = oxrm + (joff1 >> 3);
      oxrm2 = oxrm + (joff2 >> 3);
      mrm1 = (byte)(0x00ffL >> lxrml);
      mrm2 = (byte)(0x7f80L >> (int)(joff2 & 7));
#endif

/*---------------------------------------------------------------------*
*               Numbers of responses by stimulus number                *
*---------------------------------------------------------------------*/

/* Accumulate pnr response counts first because result is used for
*  KRPXR, KRPYM, and KRPZC stats.  Totals are the same for all GRPNO
*  groupings, so no igpnt loop is needed.  A preliminary check is
*  done to omit counting when no hits.  */

#ifndef PARn
      knr2 = pmdlt->pnr + nst1;
#endif
      memset((char *)pmdlt->pnr, 0, nst1*sizeof(long));
#ifndef PAR0
      for (G=g1; G<g2; G+=ginc) {
         long i;
         byte hitck = 0, txrm;
         for (i=oxrm1; i<=oxrm2; i++) {
            txrm = G[i];
            if (i == oxrm1) txrm &= mrm1;
            if (i == oxrm2) txrm &= mrm2;
            hitck |= txrm; }
         if (hitck) {
            /* Count responses to individual stimuli */
            rd_type *Go = G + oxrm;
            long ib;
            for (knr=pmdlt->pnr,ib=ioff1; ib<=ioff2; knr++,ib++)
               if (bittst(Go,ib)) (*knr)++;
            } /* End if hitcnt */
         } /* End G for */
#endif
#ifdef PARn
      /* Comp nodes send to host */
      xstpack(pmdlt->pnr, (int)nst1);
#endif
#ifdef PAR0
      /* Node 0 receives from comp nodes */
      xstunpk(pmdlt->pnr, (int)nst1);
#endif

/*---------------------------------------------------------------------*
*               Cross-response and correlation matrices                *
*                                                                      *
*  The cross-response matrix gives the raw numbers of cells responding *
*  to each pair of stimuli.  The correlation matrix calculated from    *
*  the same raw data gives the correlation coefficient between binary  *
*  cell response vectors for each pair of stimuli.  If correlations of *
*  actual s(i) are wanted, write to SIMDATA file and analyze outside   *
*  CNS, as the memory requirements are too large for present parallel  *
*  computers.  Correlation coeffs are multipled by 1000 and printed as *
*  integers so both matrices can be printed in strips of 26 columns    *
*  (5 characters/stimulus).  Allocation of STAT.pxrmtx on PAR host     *
*  was added in V8A so matrix can be stored if printed both ways.      *
*---------------------------------------------------------------------*/

      if (lctp & (KRPYM|KRPZC)) {

#ifndef PARn
         long *pmtx, *pmtx0 = STAT.pxrmtx;
         long idiag, *pdiag;
         /* nn = size of lower triangular matrix */
         long nn = nst1*(nst1+1)>>1;
#endif
         long i, j;

/* Collect cross-response matrix in preallocated pxrmtx array */

#ifndef PAR    /*** Serial version ***/

/* In the serial version, the outer loop is over repertoire data
*  and the inner loop is over stimuli, thus minimizing virtual
*  memory faults or cache thrashing. */

         /* Clear the matrix */
         memset((char *)pmtx0, 0, nn*sizeof(long));

         /* Loop over all cells, count responses for stim pairs */
         for (G=g1+oxrm; G<g2; G+=ginc) {
            /* Loop over stimuli */
            for (pmtx=pmtx0, i=ioff1; i<=ioff2; i++) {
               if (!bittst(G,i)) /* No response to ith stim...
                                 *  advance to next */
                  pmtx += (i - ioff1 + 1);
               else              /* Had response to ith stim...
                                 *  count all pairs */
                  for (j=ioff1; j<=i; j++, pmtx++)
                     if (bittst(G,j)) (*pmtx)++;
               } /* End loop over stimuli */
            } /* End loop over cells */

#else          /*** Parallel version ***/

/* The cross-response matrix is collected using the xstpack(),
*  xstunpk() routines, which may perform one or more vecollect's
*  as needed.  Accordingly, the outer loop is over matrix elements
*  and the inner loop is over cells.  Problems with virtual memory
*  are unlikely, as most target parallel machines don't have it.  */

#ifdef PARn
/* Comp node code: Collect matrix in message buffer space */

         for (i=ioff1; i<=ioff2; i++) {
            for (j=ioff1; j<=i; j++) {
               long mtxij = 0;   /* Zero current matrix element */
               for (G=g1+oxrm; G<g2; G+=ginc)
                  if (bittst(G,i) && bittst(G,j)) ++mtxij;
               xstpack(&mtxij, 1);
               }} /* End stim loops */
#endif

#ifdef PAR0
/* Node 0 code: Receive the matrix */
         xstunpk(pmtx0, (int)nn);
#endif

#endif         /*** End parallel matrix collection ***/

#ifndef PARn
/* Print the cross-response matrix */

         if (lctp & KRPYM)
               for (pdiag=pmtx0, idiag=1; idiag<=nst1; idiag+=MXCOL) {
            char tcols[28];
            long cnt = idiag+MXCOL-1; if (cnt > nst1) cnt = nst1;
            sconvrt(tcols, "(' (COLS ',J1IL8,2H- J0IL8,2H):)",
               &idiag, &cnt, NULL);
            cryout(RK_P2, "0PAIR RESPONSE MATRIX FOR ",
               RK_LN2|RK_NFSUBTTL,
               getrktxt(pmdlt->hmdltname), LTEXTF,
               cimw, fmtmwdw(pmdlt), tcols, sizeof(tcols)-1, NULL);
            for (pmtx=pdiag,i=idiag; i<=nst1; pmtx+=i,i++) {
               int cnt = i-idiag+1; if (cnt > MXCOL) cnt = MXCOL;
               convrt("(P2XRIL5)",&cnt,pmtx,NULL);
               }

            /* Calculate amount of offset to next diagonal point,
            *  viz. F(S+MXCOL)-F(S), where F(S)=S*(S-1)/2, S=IDIAG */
            pdiag += MXCOL*idiag + MXCOL*(MXCOL-1)/2;
            } /* End cross-response matrix */

/* Calculate and print the correlation matrix.  Be careful to avoid
*  overflows which might occur with integer arithmetic if a layer
*  has more than 1<<15 cells.  Note that because the input data are
*  binary, sum(x*x) = sum(x), etc.  */

         if (lctp & KRPZC) {
            double xsum,ysum,xnorm,ynorm,corr;
            double dnnel = nnel;
            knr = pmdlt->pnr;
            for (pmtx=pmtx0,i=0; i<nst1; i++) {
               if (!knr[i]) { pmtx += (i+1); continue; }
               xsum = knr[i], xnorm = xsum*(dnnel - xsum);
               for (j=0; j<=i; pmtx++,j++) {
                  if (!knr[j]) continue;
                  ysum = knr[j], ynorm = ysum*(dnnel - ysum);
                  if ((corr = xnorm*ynorm) > 0) corr =
                     (dnnel*(double)(*pmtx) - xsum*ysum) / sqrt(corr);
                  *pmtx = 1000.0 * corr + (corr >= 0 ? 0.5 : -0.5);
                  }} /* End calculating correlation matrix */

            for (pdiag=pmtx0, idiag=1; idiag<=nst1; idiag+=MXCOL) {
               char tcols[28];
               long cnt = idiag+MXCOL-1; if (cnt > nst1) cnt = nst1;
               sconvrt(tcols, "(' (COLS ',J1IL8,2H- J0IL8,2H):)",
                  &idiag, &cnt, NULL);
               /* This sequence should put tcols on a separate line
               *  if the modality name is very long.  */
               cryout(RK_P2, "0PAIR CORRELATION MATRIX (r*1000) FOR ",
                  RK_LN2|RK_NFSUBTTL,
                  getrktxt(pmdlt->hmdltname), LTEXTF,
                  cimw, fmtmwdw(pmdlt), tcols, sizeof(tcols)-1, NULL);
               for (pmtx=pdiag,i=idiag; i<=nst1; pmtx+=i,i++) {
                  int cnt = i-idiag+1; if (cnt > MXCOL) cnt = MXCOL;
                  convrt("(P2XRIL5)",&cnt,pmtx,NULL);
                  }

               /* Calculate amount of offset to next diagonal point,
               *  = F(S+MXCOL)-F(S), where F(S)=S*(S-1)/2, S=IDIAG */
               pdiag += MXCOL*idiag + MXCOL*(MXCOL-1)/2;
               } /* End printing correlation matrix */
            } /* End correlation matrix */

#endif   /* !def PARn */

         } /* End cross-response and correlation matrices */

/*---------------------------------------------------------------------*
*                      Cross-response statistics                       *
*---------------------------------------------------------------------*/

/* The strategy for parallelization is as follows:
*     All nodes accumulate in parallel their contributions to the
*     nr sums (one per stimulus), to the nhit,...nter stats (one
*     set per GRPNO card) and to the ng sums (one per class per
*     GRPNO) using their local copy of the STAT structure and its
*     appendages.  These are then packed into message buffer space
*     and vecollected onto host.  This scheme minimizes communi-
*     cation, yet permits total statistics larger than one message
*     buffer while preserving the data structure of the serial pgm.
*  The whole thing is repeated for each modality.  */

      if (lctp & KRPXR) {
#ifndef PARn
         cryout(RK_P2, "0CLASSIFICATION STATISTICS FOR ",
            RK_NFSUBTTL|RK_LN2,
            getrktxt(pmdlt->hmdltname), LTEXTF,
            cimw, fmtmwdw(pmdlt), NULL);
#endif

/* Prepare GSDEF and GST blocks for cross-response accumulation.
*  N.B.  Must skip pgs over ncs GSDEFs per GRPNO, even though
*  only ncl of these have actually been used.  */

         pgs = pmdlt->pgsd;         /* Locate GSDEF structures */
         for (igpnt=1; igpnt<=mgpnt; igpnt++,pgs+=pmdlt->ncs) {

            int ncl = pncl[igpnt-1];
            struct GSDEF *kgs,*kgs2=pgs+ncl;
#ifndef PAR0
            struct GSDEF *kgsj;
#endif
            struct GST gst;         /* Scalar cross-response sums */

#ifndef PARn
            /* Accumulators for output on host */
            double atnel;
            double antra,anter;
            double amtra,amter;
            double axtra,axter;
            float aaxtra,aaxter;
            float amrat,axrat,anrat,aamrat,aaxrat;
            float ahit,ahits;
            float agtra,ahtra;
            float agter,ahter;
            float agrat,ahrat;
            long nhits;
#endif
            for (kgs=pgs; kgs<kgs2; kgs++) {
               kgs->nstj = bitcnt(kgs->grpmsk,lndsb);
               kgs->ng = 0;
               }
            memset((char *)&gst, 0, sizeof(struct GST));

/* Accumulate contributions from a single node */

#ifndef PAR0
            for (G=g1; G<g2; G+=ginc) {
               long i, j, hitcnt = 0;
               for (i=oxrm1; i<=oxrm2; i++) {
                  byte txrm = G[i];
                  if (i == oxrm1) txrm &= mrm1;
                  if (i == oxrm2) txrm &= mrm2;
                  hitcnt += bitcnt(&txrm,1); }
               if (hitcnt) {
                  char ftra = FALSE, fter = FALSE;
                  gst.nhit++;
                  /* Count cross-responses by classes.  Various
                  *  algorithms could have been used here to get AND
                  *  of two misaligned bit strings.  This one loops
                  *  on bytes rather than bits and needs no temp
                  *  array or external bit-oriented functions.
                  *  Segmentation violations are avoided.  */
                  for (kgs=pgs; kgs<kgs2; kgs++) {
                     long ktra = 0,kter;
                     for (i=oxrm1,j=0; j<lndsb; j++) {
                        byte txrm = G[i++];
                        if (lxrml) {
                           txrm <<= lxrml;
                           if (i < lllt) txrm |= (G[i] >> lxrmr); }
                        txrm &= kgs->grpmsk[j];
                        ktra += bitcnt(&txrm,1);
                        }
                     kgs->nhtj = ktra;
                     if (ktra > 0) {
                        kgs->ng += ktra;
                        if (ktra >= 2) {
                           ftra = TRUE;
                           gst.htra += ktra;
                           gst.mtra += kgs->nstj*(kgs->nstj-1);
                           gst.ntra += ktra*(ktra-1);
                           }
                        if (hitcnt - ktra > 0) {
                           fter = TRUE;
                           gst.hter += ktra;
                           for (kgsj=pgs; kgsj<kgs; kgsj++) {
                              if ((kter = kgsj->nhtj) > 0) {
                                 gst.mter += kgs->nstj*kgsj->nstj;
                                 gst.nter += ktra*kter;
                                 }
                              } /* End loop over inter-class pairs */
                           } /* End if any inter-class pairs */
                        } /* End if ktra */
                     } /* End kgs loop */
                  if (ftra) gst.gtra++;
                  if (fter) gst.gter++;
                  } /* End if hitcnt */
               } /* End G for */
#endif

/* Pack and collect.
*  Note that although the comp node and host codes are placed
*  together here to facilitate understanding, in fact host can
*  not begin printing the statistics until a full buffer load
*  has been received, which will normally not be until comp nodes
*  have completed the pmdlt loop. */

#ifdef PARn
            /* Comp nodes send to host */
            xstpack((long *)&gst, XSTASIZ);
            for (kgs=pgs; kgs<kgs2; kgs++)
               xstpack(&kgs->ng, 1);
#endif
#ifdef PAR0
            /* Node 0 receives from comp nodes */
            xstunpk((long *)&gst, XSTASIZ);
            for (kgs=pgs; kgs<kgs2; kgs++)
               xstunpk(&kgs->ng, 1);
#endif

/* Proceed with printing.
*  Note:  Calculation of axtra (expected intra-class pairs) is
*  Sum(groups)[Sum(j in group, k!=j in group) nhitsj * nhitsk]/(2*nel)
*  = Sum(groups)[Sum(j) nhitsj * (ng(group) - nhitsj)]/(2*nel)
*  = Sum(groups)[Sum(j) nhitsj * ng - nhitsj * nhitsj)]/(2*nel)
*  = (Sum(groups)[ng * ng] - Sum(stim)[nhitsj * nhitsj])/(2*nel)  */

#ifndef PARn
            gst.mtra >>= 1;
            gst.ntra >>= 1;
            nhits = 0;
            for (knr=pmdlt->pnr; knr<knr2; knr++)
               nhits += *knr;

            /* Expected intra- and inter-class cross-responses */
            atnel = 2.0*nnel;
            antra = gst.ntra; anter = gst.nter;
            amtra = gst.mtra; amter = gst.mter;
            axtra = axter = 0.0;
            ahit  = gst.nhit; ahits = nhits;
            agtra = gst.gtra; ahtra = gst.htra;
            agter = gst.gter; ahter = gst.hter;

            for (kgs=pgs; kgs<kgs2; kgs++) {
               double ang = kgs->ng;
               axtra += ang*ang;
               axter += ang*(nhits-ang); }

            for (knr=pmdlt->pnr; knr<knr2; knr++) {
               double lnr = *knr;
               axtra -= lnr*lnr; }

            axtra /= atnel;
            axter /= atnel;
            if (gst.nhit) { agtra /= ahit; agter /= ahit; }
            if (nhits) { ahtra /= ahits; ahter /= ahits; }
            amrat = amter ? amtra/amter : 0.0;
            axrat = axter ? axtra/axter : 0.0;
            if (amtra) amtra = antra/amtra;
            if (amter) amter = anter/amter;
            aaxtra = axtra ? antra/axtra : 0.0;
            aaxter = axter ? anter/axter : 0.0;
            agrat = agter ? agtra/agter : 0.0;
            ahrat = ahter ? ahtra/ahter : 0.0;
            anrat = anter ? antra/anter : 0.0;
            aamrat = amter ? amtra/amter : 0.0;
            aaxrat = aaxter ? aaxtra/aaxter : 0.0;
            convrt("(P2,22H CROSS RESPONSE (GRPNOUH3,11H):  STIMULIIL5,"
               "8H CLASSESI4,4X10HCELLS RESPIL9,4X10HTOTAL HITSIL10)",
               &igpnt,&nstimu,&ncl,&gst.nhit,&nhits,NULL);
            strcpy(gfmt,"(P2,4X11HINTRA-CLASS4X4HCRMSIL6,6H  C/CR"
               "F6.3,6H  HITSIL6,6H  H/THF6.3,5H  PRSIL7,5H  MPP"
               "IL7,6H  P/MPQ6.3,5H  XPPQ7.3,6H  P/XPF6.3)");

            if (gst.gtra > 0) {
               convrt(gfmt,&gst.gtra,&agtra,&gst.htra,&ahtra,
                  &gst.ntra,&gst.mtra,&amtra,&axtra,&aaxtra,NULL);
               strcpy(gfmt,"(P2,4X11HINTER-CLASSIL14F12.3IL12F12.3,"
                  "2*IL12Q12.3Q12.3F12.3)");
               }
            else
               memcpy(gfmt+11, "TER", 3);

            if (gst.gter > 0) {
               convrt(gfmt,&gst.gter,&agter,&gst.hter,&ahter,
                  &gst.nter,&gst.mter,&amter,&axter,&aaxter,NULL);
               }

            if (agrat) {
               convrt("(P2,4X11HINTRA/INTERF14.3,8*F12.3)",
                  &agrat,&agrat,&ahrat,&ahrat,&anrat,&amrat,&aamrat,
                  &axrat,&aaxrat,NULL);
               }
#endif
            } /* End pgs for */
         } /* End KRPXR statistics */

SkipModality: ;
#ifndef PAR0
      ioff1 += pmdlt->nds;    /* Not incremented if modality skipped */
#endif

/*---------------------------------------------------------------------*
*                     Response history statistics                      *
*---------------------------------------------------------------------*/

/* Note:  Prior to V8A, this code was located before the KRPXR code,
*  and statistics for just the last two cycles were collected by
*  looping over XRM fields in the repertoire data.  Now the counts
*  are collected in d3go during all cycles.  It is unnecessary to
*  count cycles that do and do not have new stimuli--this can be
*  determined from the counts in the phrs arrays.  */

      if (lctp & KRPHR) {
#ifndef PARn
#if LHSTAT > LDSTAT
#error Increase dimension of stat array in d3stat.c
#endif
         double dns,dnd;         /* Floating case counts */
         long nsame, ndiff;      /* Counts for two cases */
         int i;                  /* Normalization loop ctr */
#endif

         /* Collect response history data down to host */
#ifdef PARn
         xstpack((long *)phr, LHSTAT);
#endif
#ifdef PAR0
         xstunpk((long *)phr, LHSTAT);
#endif

#ifndef PARn
         /* Calculate number of cycles with same, different stim */
         nsame = phr->h[0] + phr->h[1] + phr->h[2] + phr->h[3];
         ndiff = phr->h[4] + phr->h[5] + phr->h[6] + phr->h[7];
         /* Normalize the totals to response fractions */
         dns = nsame > 0 ? 1.0/(double)nsame : 1.0;
         dnd = ndiff > 0 ? 1.0/(double)ndiff : 1.0;
         for (i=0; i<4; ++i) {
            stat[ i ] = dns * (double)phr->h[ i ];
            stat[i+4] = dnd * (double)phr->h[i+4];
            }
         nsame /= nnel;
         ndiff /= nnel;

         /* Print the response history  for the current modality */
         cryout(RK_P2, "0RESPONSE HISTORY STATISTICS FOR ",
            RK_NFSUBTTL|RK_LN2,
            getrktxt(pmdlt->hmdltname), LTEXTF,
            cimw, fmtmwdw(pmdlt), NULL);
         convrt("(P2,"
            "IL11#4,' SAME STIM:  ++',Q8.5,4X,'+-',Q8.5,4X,'-+',"
            "Q8.5,4X,'--',Q8.5/IL11#4,' DIFF STIM:  ++',"
            "Q8.5,4X,'+-',Q8.5,4X,'-+',Q8.5,4X,'--',Q8.5)",
            &nsame, stat, &ndiff, stat+4, NULL);
#endif
         ++phr;
         } /* End if KRPHR */

      } /* End loop over modalities */

/* Clean up cross-response collection */
#ifdef PARn
   xstpack(NULL, 0);
#endif
#ifdef PAR0
   anrelp(buff);
#endif
   } /* End cross-response stats */

/*---------------------------------------------------------------------*
*                       Resume normal indenting                        *
*---------------------------------------------------------------------*/

         } /* End loop over cell types */
      } /* End loop over repertoires */
#ifndef PARn
   /* Clear any remnant subtitles */
   cryout(RK_P2, blanks, RK_NTSUBTTL+RK_LN1+4, NULL);
   /* Flush last print buffer so on-line user can see stats */
   cryout(RK_P1, "\0", RK_LN0+RK_FLUSH+1, NULL);
   spout(SPTM_NONE);          /* Turn off "SPOUT ALL" */
#endif
   } /* End d3stat() */

