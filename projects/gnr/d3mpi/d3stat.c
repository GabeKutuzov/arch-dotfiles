/* (c) Copyright 1989-2018, The Rockefeller University *21115* */
/* $Id: d3stat.c 77 2018-03-15 21:08:14Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                               d3stat                                 *
*                                                                      *
*  Print statistics for all D3 repertoires.  In the parallel           *
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
************************************************************************
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
*  Rev, 01/13/97, GNR - Add fmtict routine, bypass is NOPBY, not KGNBP *
*  Rev, 02/18/97, GNR - Allow for ntrs,ntcs in amplification stats     *
*  Rev, 01/02/98, GNR - Add ADECAY, reorganize all distribution stats  *
*  Rev, 10/28/00, GNR - Remove XRH, tabulate KSTHR over all cycles     *
*  V8B, 12/16/00, GNR - Move to new memory management routines         *
*  V8C, 08/04/03, GNR - Cell responses in millivolts, add conductances *
*  V8D, 07/24/07, GNR - Add Mij stats to DAS, add KAMDR dist.          *
*  ==>, 12/29/07, GNR - Last mod before committing to svn repository   *
*  V8E, 04/12/09, GNR - More digits for 1st-row stats, detstats, etc.  *
*  Rev, 11/07/09, GNR - Move nntr to CLSTAT.nstcap (may differ per il) *
*  V8F, 05/17/10, GNR - Add KSTGP statistics                           *
*  V8G, 08/31/10, GNR - Add group stats to KSTCL stats                 *
*  Rev, 03/30/12, GNR - Add distribution of Sj statistic               *
*  Rev, 04/02/12, GNR - Expand DAS stats to include delta Cij by case  *
*  Rev, 06/09/12, GNR - Snapshot Cij distrib, not generated, skipped   *
*  Rev, 06/27/12, GNR - Make xrmpack,xstpack for 32,64 bit variable    *
*                       to avoid overflow collecting KSTXR stats       *
*  Rev, 08/04/12, GNR - Eliminate bit offsets and bitiors              *
*  Rev, 08/16/12, GNR - Add common cells stat, mdltbid to test mdltno  *
*  Rev, 08/30/12, GNR - Reformat synaptic change stats, add <Psi>      *
*  Rev, 12/25/12, GNR - Remove printing of overflow stats to d3oflw.c  *
*  Rev, 05/23/13, GNR - Reformat autoscale stats, d3go scales Aij sums *
*  Rev, 10/15/13, GNR - Add sunder stat for negative truncation        *
*  Rev, 08/22/14, GNR - Si only EXP decay, remove DECAYDEF from DCYDFLT*
*  R67, 04/27/16, GNR - Remove Darwin 1 support                        *
*  R67, 05/03/16, GNR - Reduce nel, cpn, nodes etc. to ints per doc    *
*  Rev, 09/12/16, GNR - Remove xrmpack,xstpack, use coppack            *
*  R70, 01/19/17, GNR - Report overflows but do not terminate          *
*  Rev, 01/25/17, GNR - Add ldist (low aff dist), [hl]dists in PAR     *
*  R76, 10/14/17, GNR - Add volt-dep modified afference distributions  *
*  R78, 05/20/18, GNR - Correct printing of info for KSTGP|KSTMG       *
*  R78, 06/27/18, GNR - Add vhist stat (distribution of Vm)            *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "sysdef.h"
#include "rkarith.h"
#include "d3global.h"
#include "celldata.h"
#include "statdef.h"
#include "collect.h"
#ifndef PARn
#include "rocks.h"
#include "rkxtra.h"
#endif
#ifndef PAR0
#include "lijarg.h"
extern struct LIJARG Lija;
#endif
#include "bapkg.h"

extern struct CELLDATA CDAT;

#define MXCOL       26  /* Number of cols in cross-response matrix */

/* Form averages from fixed-point sums with scaling and zero check.
*  Note, 09/24/93, GNR - The divrnd macro used in earlier versions
*  of d3stat was wrong for two reasons: (1) because negative numbers
*  were rounded towards zero, and (2) because rounding essentially
*  occurred twice, once in the macro and once in convrt, whereas all
*  rounding should be done only during conversion for printing.  */
#define divchk(x,y)    (((y) == 0) ? 0.0 : \
   ((double)(x)/(double)(y)))
#define wdivchk(x,y)   (((y) == 0) ? 0 : jdswq((x),(y)))

#ifdef PAR
/* Define COPDEF structures for collecting cross-response stats */
static struct COPDEF pnrCop = { ADD+UI32V, 0 };
static struct COPDEF HistCop = { ADD+LONG, LHSTAT };
static struct COPDEF StatCop = { ADD+LLONG, 0 };
#define NPsiCopOps 4
static struct COPDEF PSICop[NPsiCopOps] = { { ADD+LLONG, 1 },
   { MIN+SI32V, 1 }, { MAX+SI32V, 1 }, { ADD+UI32V, 1 } };
#endif

/* Structure for collecting psi (amplif. refractoriness) stats */
struct PSIST {
   si64 sumpsi;      /* Sum of psi over cells */
   si32 minpsi;      /* Lowest psi */
   si32 maxpsi;      /* Highest psi */
   ui32 npover;      /* Number psi >= 1.0 */
   } ;

/* Structure for collecting cross-response statistics.  It should
*  contain only xstt's, as it is collected to host node by use of
*  coppack().  XSTASIZ is number of items in struct.
*  This is rebuilt for each GRPNO within each modality.  */
struct GST {
   xstt nhit;        /* No. of cells responding at least once */
   xstt gtra;        /* Groups involved in intra-class pairs */
   xstt gter;        /* Groups involved in inter-class pairs */
   xstt htra;        /* Intra-class hits */
   xstt hter;        /* Inter-class hits */
   xstt mtra;        /* Maximum possible intra-class pairs */
   xstt mter;        /* Maximum possible inter-class pairs */
   xstt ntra;        /* Number of intra-class pairs observed */
   xstt nter;        /* Number of inter-class pairs observed */
   xstt ncc0;        /* Cells that responded to no stimuli */
   xstt ncc1;        /* Cells that responded to one class only */
   xstt nccc;        /* Number of common cells per class pair */
   } ;
#define XSTASIZ (sizeof(struct GST)/sizeof(xstt))

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
      wbcdwt(&ict, cict, RK_IORF|RK_PAD0|RK_NINT|2);
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

   struct REPBLOCK *ir;       /* Ptr to current repertoire */
   struct CELLTYPE *il;       /* Ptr to current cell block */
   struct CONNTYPE *ix;       /* Ptr to current connection block */
   struct MODALITY *pmdlt;    /* Ptr to current MODALITY block */
#ifdef PAR
   CPKD *pspk;                /* Work area for coppack */
#endif
   hist *phr;                 /* Ptr to current history stats  */
   ui16 *pncl;                /* Ptr to no. classes in stim. group */
   long lndsb;                /* Local ndsbyt */
   long nst1;                 /* Largest isn in a modality */
   long nst1b;                /* Number of bytes in nst1 bits */
   ui16 igpnt,mgpnt;          /* GRPNO card counter,count */
   unsigned short xopt;       /* Cross-response statistics mask */

#ifndef PAR0
   rd_type *g1, *g2, *G;      /* Ptrs to access rep data */
   size_t ginc;               /* Increment for g1 */
   long oxrmi;                /* Offset to oxrm, psi data */
#endif

#ifndef PARn
   byte *pgpmsk,*pgpmski;     /* Ptrs to grpmsks */
   char *prange;              /* Ptr to distribution ranges */
   double stat[LDSTAT];       /* Statistical info arrays */
   long lstat[LDSTAT];
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
      }
   else {
      prange = ranges[0];
      }
   /* If requsted, SPOUT everything */
   if (RP->CP.runflags & RP_SSTAT) spout(SPTM_ALL);
   /* Setup to count overflows */
   e64dec(EAfl(OVF_STAT));
   RP->ovflow = 0;      /* Should be redundant */

/*---------------------------------------------------------------------*
*             Preparations for cross-response statistics               *
*  Data for X,Y,Z,H stats are collected here with coppack() while      *
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

      /* Locate and clear the ncl and grpmsk arrays.  N.B.  Code in
      *  igpnt loop below assumes these values have been cleared.  */
      if (STAT.lgrpblk > 0)
         memset((char *)STAT.pgrpblk, 0, STAT.lgrpblk);
      pncl = STAT.pgrpncl;

      if (RP->kxr & (XSTATS|YSTATS|ZSTATS|HSTATS|GSTATS)) {
         pgpmsk = STAT.pgrpmsk;
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

/* Back to all nodes, set xopt to accept KSTHR, but mask out KSTXR,
*  KSTYM, KSTZC bits according to whether more than one stimulus was
*  found for at least one modality.  */

   xopt = KSTHR;
   if (RP->kxr & (XSTATS|YSTATS|ZSTATS|HSTATS)) {

#ifdef PAR
      /* Local conversion table for the GRPMSK_MSG broadcast */
      long nxtabl[4];
#endif

      for (pmdlt=RP->pmdlt1; pmdlt; pmdlt=pmdlt->pnmdlt) {
         /* If two or more different stimuli were presented,
         *  set xopt flag to activate statistics later.  */
         nst1b = BytesInXBits(pmdlt->um1.MS.mxisn);
         if (bitcnt(pmdlt->pxxxx, nst1b) >= 2)
            xopt |= (KSTXR | KSTYM | KSTZC);
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
      pspk = coppini(XSTATS_MSG, 0, 0);
#endif
      } /* End if X,Y,Z,H stats */

/*---------------------------------------------------------------------*
*                   Print D3 repertoire statistics                     *
*---------------------------------------------------------------------*/

   for (ir=RP->tree; ir; ir=ir->pnrp) {
#ifndef PARn
      int lundr;                    /* Length of underlines */
      int mxlrn,mxlrn2;
      int grgsw, gotrg;             /* Regeneration format switches */
      byte mxnauts = 0;             /* 1 or 2 kinds of autoscale */
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
            if (il->nauts > mxnauts) mxnauts = il->nauts;
            } /* End for-if */

/*---------------------------------------------------------------------*
*       Second group of summary stats: mean response, sharpness        *
*---------------------------------------------------------------------*/

      lundr = RK_OMITLN+RK_LN1+80;
      if (ir->Rp.krp & KRPRG)
         gotrg = 1, lundr += grgsw = 12;
      else
         gotrg = 0, grgsw = RK_SKIP;

      cryout(RK_P2,"0              MEAN     SHARPNESS     PERCENT    "
         " PERCENT       S(I) TRUNCATED", RK_NFSUBTTL+RK_LN2,
         "   CELLS RE-", grgsw, underlines, lundr,
                   "+CELL TYPE   RESPONSE   STATISTIC      REFRAC    "
         " SPIKING      DEPOL    HYPERPOL", RK_LN0,
         "   GENERATED", grgsw, NULL);

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
            lstat[5] = il->CTST->sunder;
            convrt("(P2,3X,A" qLSN ",#6,2B20/27IL12.<4,2B20IL12.<4,"
               "2IL12,RIL12)",
               il->lname, lstat, &gotrg, &il->CTST->nregen, NULL);
            } /* End for-if */

/*---------------------------------------------------------------------*
*                        Autoscale statistics                          *
*  R70, 01/19/17, GNR - Remove overflow count, redundant w/d3oflw      *
*---------------------------------------------------------------------*/

      if (mxnauts > 0) {
         int nttl = 33 + 36*mxnauts;
         cryout(RK_P2,"0              LAST      FIRST (OR ONLY) "
            "CYCLE MULTIPLIER    LATE CYCLE (HYPERPOL) MULTIPLIER",
            RK_NFSUBTTL+RK_LN2+nttl,
            underlines, RK_OMITLN+RK_LN1+nttl,
                      "+CELL TYPE  MULTIPLIER      MIN         M"
            "AX     NCAPPED         MIN         MAX     NCAPPED  ",
            RK_LN0+nttl, NULL);

         /* Loop over all cell blocks (again) */
         for (il=ir->play1; il; il=il->play)
               if (il->ctf & DIDSTATS && il->nauts > 0) {
            struct AUTSCL *paut = il->pauts;
            int na = 3*(int)il->nauts;
            convrt("(P2,3X,A" qLSN ",B24IJ13.<5,#,2(2B24IJ12.<5,"
               "UW11;X))", il->lname, paut->asmul, &na,
               (char *)il->CTST + sizeof(struct CLSTAT), NULL);
            } /* End for-if */
         } /* End if autoscale */
#endif

/*---------------------------------------------------------------------*
*                Distributions and response components                 *
*---------------------------------------------------------------------*/

      for (il=ir->play1; il; il=il->play) if (il->ctf & DIDSTATS) {
#ifndef PAR0
         size_t llel = uw2zd(il->lel);
         size_t lllt = uw2zd(il->llt);
#endif
#ifndef PARn
         struct INHIBBLK *ib;    /* Ptr to current inhibition block */
         struct MODBY *imb;      /* Ptr to current modby block */
         ddist  *pdd;            /* Ptr to double stats */
         double dnnel;           /* Total cells in celltype = nelt */
         double dnctr;           /* Cells times cycles|trials */
#endif
         ui32 lkst = il->Ct.kstat;  /* Copy of stat selector bits */

#ifndef PARn
         dnnel = (double)il->nelt;
         dnctr = (double)il->CTST->nstcap*dnnel;

         /* Print title for response components */
         tlines(12);
         cryout(RK_P2, "0   ", RK_NFSUBTTL+RK_LN2, NULL);
         mxlrn = min(ir->lrn, 78);
         mxlrn2 = mxlrn>>1;
         cryout(RK_P2, blanks, RK_LN1+40-mxlrn2,
            "---DETAILED STATISTICS FOR ", RK_CCL,
            getrktxt(ir->hrname), mxlrn, " REGION, ", RK_CCL,
            il->lname, LSNAME, " CELL TYPE---", RK_CCL, NULL);
         cryout(RK_P2, "0DISTRIBUTIONS, RESPONSE COMPONENTS, HIGH/LOW"
            " RESPONSES", RK_NFSUBTTL+RK_LN2, underlines, RK_OMITLN+
            RK_LN1+133, prange, RK_LN0, NULL);

         /* Loop over connection types, printing A distributions.
         *  Rev, 06/21/13, GNR - Aij sums stored by d3go() are now
         *  already scaled, reflecting possible different autoscales
         *  for excit vs inhib inputs--old idea that user might want
         *  distribution before autoscale cannot be implemented.
         *  But OTOH double-float scaling is no longer needed.  */
         for (ix=il->pct1; ix; ix=ix->pct) {
            char *fctn = fmtict((int)ix->ict);  /* Conntype number */
            /* Note:  Avg Aij = total voltage/num of cells w/S(i)
            *  in this bin.  If conntype input were turned off by an
            *  ntcs option, this statistic will reflect that fact,
            *  as it should.  To count only cycles with active input
            *  would need an extra sdist array for each conntype.  */
            setbscompat(ix->cnsrctyp);
            pdd = il->pctddst + ix->ocnddst + OaVd;
            if (ix->qaffmods) {
               avghist(pdd->d, il->CTST->sdist, lstat);
               convrt("(P2,2H D,A3,16B20/27IL8.<4)", fctn, lstat, NULL);
               ++pdd;
               } /* End vdep,mxax modified afference distribution */
            avghist(pdd->d, il->CTST->sdist, lstat);
            convrt("(P2,2H A,A3,16B20/27IL8.<4)", fctn, lstat, NULL);
            ++pdd;
            if (qdecay(&ix->Cn.ADCY)) {
               avghist(pdd->d, il->CTST->sdist, lstat);
               convrt("(P2,2H W,A3,16B20/27IL8.<4)", fctn, lstat, NULL);
               ++pdd;
               } /* End persistence distribution */
            if (cnexists(ix, RBAR)) {
               avghist(pdd->d, il->CTST->sdist, lstat);
               convrt("(P2,2H R,A3,16B7|14IL8.<4)", fctn, lstat, NULL);
               } /* End rbar distribution */
            } /* End loop over connection types */

         /* Loop over inhibition blocks, printing I distributions */
         for (ib=il->pib1; ib; ib=ib->pib) {
            char *fctn = fmtict((int)ib->igt);     /* GCONN number */
            ui32 ibcnt;                            /* Band counter */
            pdd = il->pctddst + ib->oidist;
            for (ibcnt=0; ibcnt<ib->nib; ibcnt++,pdd++) {
               avghist(pdd->d, il->CTST->sdist, lstat);
               if (ibcnt == 0)
                  convrt("(P2,2H I,A3,16B20/27IL8.<4)",
                     fctn, lstat, NULL);
               else
                  convrt("(P2,5X,16B20/27IL8.<4)", lstat, NULL);
               } /* End for ibcnt */
            if (qdecay(&ib->GDCY)) {
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
            if (qdecay(&imb->MDCY)) {
               avghist(pdd[OmWd].d, il->CTST->sdist, lstat);
               convrt("(P2,2H W,A3,16B20/27IL8.<4)", fctn, lstat, NULL);
               } /* End MOD persistence distribution */
            } /* End loop over modby blocks */

         /* Get ready to print stats for cell-level variables */
         pdd = il->pctddst;

         /* Print Izhikevich 'u' = Brette-Gerster 'w' histogram
         *  or Vm distribution, according to rspmeth  */
         avghist(pdd->d, il->CTST->sdist, lstat);
         if (il->Ct.rspmeth >= RF_BREG)
            convrt("(P2,' RFUW',16B20/27IL8.<4)", lstat, NULL);
         else
            convrt("(P2,' Vm  ',16B20/27IL8.<4)", lstat, NULL);
         ++pdd;

         /* Print persistence histogram */
         if (il->Dc.omegau) {
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
         /* R70:  Now have HIGH and LOW statistics, both are affer-
         *  ences, not responses, both available in PAR or !PAR.  */
         convrt("(P2,' HIGH',16IL8/'  LOW',16IL8)",
            il->CTST->hdist, il->CTST->ldist, NULL);

         /* Print C distribution (response vs. stimulus class).
         *  The data are gathered in d3go as a function of stimulus
         *  isn, which is related here to stimulus class.  Inasmuch
         *  as pxxxx array may not exist, must test Cdist itself to
         *  determine which stimuli were actually presented.  */
         if (lkst & KSTCL) {
            dist *pda,*pda1,*pda0 = il->pctdist + il->oCdist;
            pncl = STAT.pgrpncl;    /* Ptr to no. classes per group */
            for (pmdlt=RP->pmdlt1; pmdlt; pmdlt=pmdlt->pnmdlt) {
               struct GSDEF *pgs;   /* Ptr to current group data */
               struct GRPDEF *pgpn; /* Ptr to GRPNO chain */
               int ist, itrig;
               /* Only consider modalities sensed by this celltype */
               mgpnt = NStimGrps(pmdlt);
               if (!(il->rmdlts & pmdlt->mdltbid))
                  goto SkipModalityForC;
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

               /* Loop over GRPNO sets.  As w/KSTXR stats, must incr.
               *  pgs over ncs GSDEFs per GRPNO, even though there
               *  may not be that many groups in any one GRPNO set.  */
               pgs = pmdlt->pgsd;
               pgpn = pmdlt->pgpno;          /* Ptr to GRPNO chain */
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
            } /* End KSTCL output */

         if (lkst & KSTFD) {
            /* Print s(i) dist. vs. feature-detector matrix number */
            dist *pda;              /* Ptr to distribution array */
            for (ix=il->pct1; ix; ix=ix->pct) {
               int i, iinc, itop, itrig;
               if (!(ix->cnflgs & DOFDIS)) continue;
               iinc = ix->ucij.m.nks;
               itop = iinc * ix->ucij.m.nkt;
               itrig = TRUE;
               pda = il->pctdist + ix->ucij.m.oFDMdist;
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
            } /* End KSTFD output */

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

#endif

         /* Print distributions of Cij's--these moved to separate
         *  loop in V1K so can have meaningful titles.  Changed in
         *  V8H to compute an end-of-series snapshot here.  (Two
         *  levels of indenting suppressed.)  */
   {  long *pCdist = (long *)CDAT.psws;   /* Space for Cij dists */
#ifndef PAR0
      int icell;
      d3kij(il, KMEM_GO);           /* Set to access network data */
#endif
#ifndef PARn
      long nCtot;
      int  isj;
      cryout(RK_P2,"0DISTRIBUTION OF CONNECTION STRENGTHS",
         RK_NFSUBTTL+RK_LN2, underlines, RK_OMITLN+RK_LN1+133,
            "+ CIJ   -0.94   -0.81   -0.69   -0.56   -0.44"
         "   -0.31   -0.19   -0.06    0.06    0.19    0.31"
         "    0.44    0.56    0.69    0.81    0.94", RK_LN0,  NULL);
#endif
      /* Count Cij distribution.  Count missing Cij in extra entry
      *  at pCdist[OCnSkipTot]. This d3stat() looping incidentally
      *  saves counting Cij dist. on every trial and cycle in d3go
      *  while giving a more useful Cij dist. after amplif.  */
      for (ix=il->pct1; ix; ix=ix->pct) {
         ix->cnwk.stat.totcskip = 0;
         if (ix->Cn.cnopt & NOPBY) continue;
         memset((char *)pCdist, 0, NCnCijStat*sizeof(long));
#ifndef PAR0
         for (icell=il->locell; icell<il->hicell; ++icell) {
            d3kiji(il, icell);
            d3kijx(ix);
            d3lijx(ix);
            d3cijx(ix);
            pCdist[OCnSkipTot] += (long)ix->nc - Lija.nuk;
            for (Lija.isyn=0; Lija.isyn<Lija.nuk;
                  ++Lija.isyn,Lija.psyn+=ix->lc) {
               int cijbin = !ix->cijbr(ix) ? OCnSkipTot :
                  ((Lija.cijsgn == SI32_SGN) ? (LDSTAT-1) :
                  (ui32)Lija.cijsgn >> (BITSPERUI32-4));
               pCdist[cijbin] += 1;
               } /* End synapse loop */
            } /* End cell loop */
#endif
#ifdef PAR
         /* If parallel, gather distribution sums to Node 0 */
         StatCop.count = NCnCijStat;
         vecollect((char *)pCdist, &StatCop, 1, CIJ_DIST_MSG,
            NCnCijStat*sizeof(long));
#endif
#ifndef PARn
         ix->cnwk.stat.totcskip = pCdist[OCnSkipTot];
         nCtot = 0;
         for (isj=0; isj<LDSTAT; ++isj) nCtot += pCdist[isj];
         if (nCtot > 0) {
            double dnCtot = 1.0/(double)nCtot;
            for (isj=0; isj<LDSTAT; ++isj)
               stat[isj] = dnCtot*(double)pCdist[isj];
            convrt("(P2,2H C,A3,8Q8.5,8Q8.5)",
               fmtict((int)ix->ict), stat+8, stat, NULL);
            } /* End nCtot if */
         else
            convrt("(P2,2H C,A3,' ==>NO CIJ FOUND')",
               fmtict((int)ix->ict), NULL);
#endif
         } /* End ix for */

      } /* End Cij distribution local scope and unindent */

#ifndef PARn
         /* Print distributions of Sj's */
         cryout(RK_P2,"0DISTRIBUTION OF INPUT Sj",
            RK_NFSUBTTL+RK_LN4,
               "  mV  <  -112     -96     -80     -64     -48"
            "     -32     -16       0      16      32      48"
            "      64      80      96     112     256", RK_LN0,
            underlines, RK_OMITLN+RK_LN0+133,
               "+OldS < -0.88   -0.75   -0.62   -0.50   -0.38"
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
*     of amplification:  number of connections not generated,          *
*     number skipped due to no input, and number of PPF hits.          *
*  These are printed even if conns bypassed, as a sort of reminder.    *
*  Rev, 06/09/12, GNR - Two kinds of skip counts, correct cycle count  *
*     when DIMDLT is set.                                              *
*---------------------------------------------------------------------*/

         cryout(RK_P2, "0OTHER CONNECTION DATA (PER CELL PER TRIAL)",
            RK_NFSUBTTL+RK_LN2, underlines, RK_OMITLN+RK_LN1+54,
            "+CONNTYPE   SKIPPED-NOT GEN   SKIPPED-NO SJ   PPF HITS",
            RK_LN0, NULL);
         for (ix=il->pct1; ix; ix=ix->pct) if (ix->cnwk.stat.totcskip |
               ix->CNST.ncskip | ix->CNST.nppfh) {
            double dntrca,dnsng,dnsnsj,dnppfh;
            long nmodtr, nalltr = il->CTST->nstcap;
            if (ix->cnflgs & DIMDLT) {
               nmodtr = RP->ntc*ix->psmdlt->um1.MS.ntract;
               if (nmodtr < nalltr) nalltr = nmodtr; }
            dntrca = (double)nalltr*dnnel;
            dnsng = (double)ix->cnwk.stat.totcskip/dnnel;
            if (dntrca > 0) {
               dnsnsj = (double)ix->CNST.ncskip/dntrca;
               dnppfh = (double)ix->CNST.nppfh/dntrca; }
            else
               dnsnsj = dnppfh = 0;
            convrt("(P2,4H   C,A3,Q16.5,Q16.5,Q15.5)",
               fmtict((int)ix->ict), &dnsng, &dnsnsj, &dnppfh, NULL);
            } /* End ix for */
#endif

/*---------------------------------------------------------------------*
*               Amplification refractoriness statistics                *
*  These statistics are programmed separately here to make them easy   *
*  to remove if this option turns out not to be useful.                *
*---------------------------------------------------------------------*/

         if (!(RP->CP.runflags & RP_NOCHNG) && il->orkam & KAMLP) {
            struct PSIST psist;
#ifndef PARn
            si32 avgpsi;
#endif
            psist.sumpsi = jesl(0);
            psist.minpsi = SI32_MAX;
            psist.maxpsi = -SI32_MAX;
            psist.npover = 0;
#ifndef PAR0
            /* Make looping constants for examining repertoire data */
            g1 = il->prd;           /* Locate repertoire data */
            g2 = g1 + lllt;         /* End of repertoire data */
            ginc = llel;            /* Length of data for one cell */
            oxrmi = il->ctoff[PSI]; /* Offset to PSI data */

            /* Loop over cells to collect stats */
            for (G=g1+oxrmi; G<g2; G+=ginc) {
               si32 psi;
               d3gtjs2(psi, G);
               psist.sumpsi = jasl(psist.sumpsi, psi);
               if (psi < psist.minpsi) psist.minpsi = psi;
               if (psi > psist.maxpsi) psist.maxpsi = psi;
               if (psi == S15)         psist.npover += 1;
               }
#endif
#ifdef PAR
            /* If parallel, gather psist stats to Node 0 */
            vecollect((char *)&psist, PSICop, NPsiCopOps,
               PSI_STAT_MSG, sizeof(psist));
#endif
#ifndef PARn
            avgpsi = jdswq(psist.sumpsi, il->nelt);
            cryout(RK_P2, "0SYNAPTIC CHANGE REFRACTORINESS (PSI)",
               RK_NFSUBTTL+RK_LN2, NULL);
            convrt("(P2,'   MEAN ',J0B15IJ12.<5,', MIN ',J0B15IJ12.<5,"
               "', MAX ',J0B15IJ12.<5,', N==1 ',J0UJ12)", &avgpsi,
               &psist.minpsi, &psist.maxpsi, &psist.npover, NULL);
#endif
            } /* End PSI statistics */

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

#ifndef PARn
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
               aden = (float)ix->CNST.nampc * (float)il->nelt;
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
*     (1) Only if value being used (KAMVM|KAMCG|KAMUE):                *
*         Total (v-mtv) for the nine cases  (FBvl=8)                   *
*     (2) Only if Mij being calculated (KAMSA|KAMTS|KAMUE):            *
*         Total Mij for the nine cases      (S14)                      *
*     (3) Total (si-mti) for the nine cases (S7 mV/S14)                *
*     (4) Total (sj-mtj) for the nine cases (S7 mv/S14)                *
*     (5) Total DeltaCij for the nine cases (S16)                      *
*     (6) Counts for the nine cases                                    *
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
               if (!(ix->Cn.cnopt & NOPBY) && (ix->Cn.kam & KAMDS)) {
               si64 *pd = ix->pdas;
               char *fctn = fmtict((int)ix->ict);
               long mdas[6*LDR], ndas, *pld, *pld2 = mdas + LDR;
               int  idr, ndr = ix->ldas/sizeof(*(ix->pdas));
               memset((char *)mdas, 0, sizeof(mdas));
               setbscompat(ix->cnsrctyp);
               for (pld=mdas; pld<pld2; pd++,pld++) {
                  ndas = sw2ld(pd[0]);
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
                  "2H V,A3,9B" qqv(FBvl) "IL10.<4)",
                  fctn, mdas+OdasVM, NULL);
               /* mdas+ndr-LDR locates the last (Mij) stats vector,
               *  which may follow OdasVM or OdasVC if no OdasVM */
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
*           KSTGP and KSTMG ("Additional group statistics")            *
*---------------------------------------------------------------------*/

/* Print M statistics (max/min response vs. stimulus id).  The GPSTAT
*  data are gathered in d3go as a function of stimulus number and col-
*  lected by d3gsta in a parallel comptuer.  Aggregate and print by
*  stimulus group.  (The loop over stimuli is repeated for each GRPNO
*  set to avoid having to allocate an array of GPSTATs.  Typically
*  there will be only a few sets, so this is not so bad.)  */

if (lkst & (KSTGP|KSTMG)) {
   struct GPSTAT *pgp,*pgp0 = CDAT.pgstat + il->oGstat;
   pncl = STAT.pgrpncl;    /* Ptr to number classes per group */
   for (pmdlt=RP->pmdlt1; pmdlt; pmdlt=pmdlt->pnmdlt) {
      struct GSDEF *pgs;   /* Ptr to current group data */
      struct GRPDEF *pgpn; /* Ptr to GRPNO chain */
      double hpt;          /* Hits per trial */
      int ist, itrig;      /* Stimulus counter, title trigger */
      /* Only consider modalities sensed by this celltype */
      mgpnt = NStimGrps(pmdlt);
      if (!(il->rmdlts & pmdlt->mdltbid)) goto SkipModalityForBoth;
      if (!(lkst & KSTMG)) goto SkipModalityForM;
      pgp = pgp0;
      itrig = TRUE;
      /* Loop over stimuli presented via this modality */
      for (ist=1; ist<=pmdlt->nds; ist++,pgp++) {
         if (pgp->nstim > 0) {
            if (itrig) {      /* Label the modality */
               cryout(RK_P2, "0MAX/MIN RESPONSES BY STIMULUS ID FOR ",
                  RK_NFSUBTTL+RK_LN3, getrktxt(pmdlt->hmdltname),
                  LTEXTF, " MODALITY", RK_CCL, cimw, fmtmwdw(pmdlt),
                  underlines, RK_OMITLN+RK_LN0+72,
                  "+ STIM ID  Max S(i)  LowMaxSi HighMinSi  Min S(i) "
                  " PRESENTED  HITS/TRIAL", RK_LN0+72, NULL);
               itrig = FALSE;
               }
            hpt = (double)pgp->nhits/(double)pgp->nstim;
            convrt("(P2H I8,4B20/27IJ10.<4,UJ11,Q12.<4)", &ist,
               &pgp->hiresp, &pgp->nstim, &hpt, NULL);
            }
         } /* End stimulus loop */

      /* Loop over GRPNO sets.  As w/KSTXR stats, must increment
      *  pgs over ncs GSDEFs per GRPNO, even though there may not
      *  be that many groups in any one GRPNO set.  */
SkipModalityForM:
      if (!(lkst & KSTGP)) goto SkipModalityForG;
      pgs = pmdlt->pgsd;
      pgpn = pmdlt->pgpno;          /* Ptr to GRPNO chain */
      for (igpnt=1; igpnt<=mgpnt; ++igpnt,pgs+=pmdlt->ncs) {
         struct GSDEF *qgs = pgs;
         short *pgpnl = pgpn->grp;  /* Ptr to GRPNO list */
         struct GPSTAT tgps;        /* Temp GPSTAT for aggregating */
         int ncl = pncl[igpnt-1];
         int gpid = 0;              /* Group number for printing */
         /* Loop over the stimulus groups in the GRPNO set */
         itrig = TRUE;
         for (ist=0; ist<ncl; ++ist,++qgs) {
            long i;
            pgp = pgp0;          /* Ptr to first GPSTAT in modality */
            /* Clear the aggregate stats */
            tgps.nstim = tgps.nhits = 0;
            tgps.hiloresp = tgps.hiresp = -SI32_MAX;
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
               if (pgp->hiloresp > tgps.hiloresp)
                  tgps.hiloresp = pgp->hiloresp;
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
                     "                    Min(Stim) Max(Stim)",
                     RK_LN0+39, underlines, RK_OMITLN+RK_LN0+72,
                     "+ GRP NUM  Max S(i) HighMaxSi  LowMaxSi  Min "
                     "S(i)  PRESENTED  HITS/TRIAL", RK_LN0+72, NULL);
                  itrig = FALSE;
                  }
               hpt = (double)tgps.nhits/(double)tgps.nstim;
               convrt("(P2H I8,4B20/27IJ10.<4,UJ11,Q12.<4)", &gpid,
                  &tgps.hiresp, &tgps.nstim, &hpt, NULL);
               }
            } /* End ist loop over stimulus groups */
         pgpn = pgpn->pnxgrp;
         } /* End NStimGrps loop */
SkipModalityForG:
      pgp0 += pmdlt->nds;
      /* Be careful to increment pncl even for skipped modalities */
SkipModalityForBoth:
      pncl += mgpnt;
      } /* End MODALITY loop */
   } /* End KSTGP|KSTMG output */

#endif /* !def PARn */

/*=====================================================================*
*           Response history and cross-response statistics             *
*=====================================================================*/

#ifndef PAR0
if (lkst & (KSTXR|KSTYM|KSTZC) & xopt) {
   /* Make looping constants for examining repertoire data */
   g1 = il->prd;           /* Locate repertoire data */
   g2 = g1 + lllt;         /* End of repertoire data */
   ginc = llel;            /* Length of data for one cell */
   oxrmi = il->ctoff[XRM]; /* Offset to XRM data */
   } /* End calculating looping constants */
#endif

/*---------------------------------------------------------------------*
* Give cross-response statistics for all relevant sensory modalities   *
*---------------------------------------------------------------------*/


if (lkst & xopt) {
   pncl = STAT.pgrpncl;
   phr = il->phrs;
   for (pmdlt=RP->pmdlt1; pmdlt; pmdlt=pmdlt->pnmdlt, pncl+=mgpnt) {

      struct GSDEF *pgs;         /* Ptr to current group data */
      xrmt *knr,*knr2;           /* Ptrs to response-by-stim counts */
#ifndef PARn
      long nstimu;               /* No. of unique stimuli presented */
#endif

      mgpnt = NStimGrps(pmdlt);  /* Number GRPNO cards */
      /* Omit modalities that this celltype does not respond to */
      if (!(il->rmdlts & pmdlt->mdltbid)) continue;

      /* Skip to response history if no cross-response stuff */
      if (!(lkst & (KSTXR|KSTYM|KSTZC) & xopt))
         goto SkipModality;

      /* Omit modalities with fewer than two stimuli applied */
      nst1 = pmdlt->um1.MS.mxisn;   /* Highest isn encountered */
      nst1b = BytesInXBits(nst1);   /* Bytes to examine */
#ifdef PARn
      if (bitcnt(pmdlt->pxxxx, nst1b) < 2)
         goto SkipModality;
#else
      if ((nstimu = bitcnt(pmdlt->pxxxx, nst1b)) < 2) {
         cryout(RK_P2,"0Skipping ", RK_LN2, getrktxt(pmdlt->hmdltname),
            LTEXTF, " modality--fewer than 2 stimuli presented.",
            RK_CCL, NULL);
         goto SkipModality;
         }
#endif

      lndsb = pmdlt->ndsbyt;        /* Bytes to hold nds bits */
      if (nst1 > SHRT_MAX)
         d3exit("Number of distinct stimuli exceeds 32767", 422, nst1);

/*---------------------------------------------------------------------*
*               Numbers of responses by stimulus number                *
*---------------------------------------------------------------------*/

/* Accumulate pnr response counts first because result is used for
*  KSTXR, KSTYM, and KSTZC stats.  Totals are the same for all GRPNO
*  groupings, so no igpnt loop is needed.  Save a little time by
*  skipping bytes with no bits.  No need to mask unused bits in the
*  last byte, these will always be 0 in the XRM data.  */

      memset((char *)pmdlt->pnr, 0, nst1*sizeof(xrmt));
      knr2 = pmdlt->pnr + nst1;
#ifndef PAR0
      for (G=g1+oxrmi; G<g2; G+=ginc) {
         rd_type *Go;
         long i;
         for (Go=G,knr=pmdlt->pnr; knr<knr2; ++Go,knr+=BITSPERBYTE) {
            if (*Go) {
               for (i=0; i<BITSPERBYTE; ++i)
                  if (bittst(Go,i+1)) knr[i] += 1;
               }
            }
         } /* End G for */
#endif
#ifdef PAR
      /* Accumulate sum of pnr counts on PAR0 node */
      pnrCop.count = nst1;
      coppack(pspk, pmdlt->pnr, &pnrCop, 1);
      copcomb(pspk);
#endif

/*---------------------------------------------------------------------*
*               Cross-response and correlation matrices                *
*                                                                      *
*  The cross-response matrix gives the raw numbers of cells responding *
*  to each pair of stimuli.  The correlation matrix calculated from    *
*  the same raw data gives the correlation coefficient between binary  *
*  cell response vectors for each pair of stimuli.  If correlations of *
*  actual s(i) are wanted, write to SIMDATA file and analyze outside   *
*  CNS (or add new code here).  Correlation coeffs are multipled by    *
*  1000 and printed as integers so both matrices can be printed in     *
*  strips of 26 columns (5 characters/stimulus).  Allocation of STAT.  *
*  pxrmtx on PAR host was added in V8A and on PARn in R67 (bug fix).   *
*---------------------------------------------------------------------*/

      if (lkst & (KSTYM|KSTZC)) {

         ui32 *pmtx, *pmtx0 = STAT.pxrmtx;
#ifndef PARn
         ui32 *pdiag;
         long idiag;
#endif
         long i, j;

/* Collect cross-response matrix in preallocated pxrmtx array */

         /* Clear the matrix */
         memset((char *)pmtx0, 0, nst1*(nst1+1)*JSIZE/2);

#ifndef PAR    /*** Serial version ***/

/* In the serial version, the outer loop is over repertoire data
*  and the inner loop is over stimuli, thus minimizing virtual
*  memory faults or cache thrashing. */

         /* Loop over all cells, count responses for stim pairs */
         for (G=g1+oxrmi; G<g2; G+=ginc) {
            /* Loop over stimuli */
            for (pmtx=pmtx0, i=1; i<=nst1; i++) {
               if (!bittst(G,i)) /* No response to ith stim...
                                 *  advance to next */
                  pmtx += i;
               else              /* Had response to ith stim...
                                 *  count all pairs */
                  for (j=1; j<=i; j++, pmtx++)
                     if (bittst(G,j)) (*pmtx)++;
               } /* End loop over stimuli */
            } /* End loop over cells */

#else          /*** Parallel version ***/

/* The cross-response matrix is collected using the coppack()
*  routines, which may perform one or more buffer collections as
*  needed.  Accordingly, the outer loop is over matrix elements
*  and the inner loop is over cells.  Because the current COPDEF
*  only allows counts up to 2**16, a separate coppack call is
*  made for each row in the matrix.  */

#ifdef PARn
/* Comp node code: Collect matrix for cells on this node */

         for (pmtx=pmtx0,i=1; i<=nst1; i++) {
            ui32 *pmrow = pmtx;
            for (j=1; j<=i; pmtx++,j++) {
               for (G=g1+oxrmi; G<g2; G+=ginc)
                  if (bittst(G,i) && bittst(G,j)) (*pmtx)++;
               }
            pnrCop.count = i;
            coppack(pspk, pmrow, &pnrCop, 1);
            } /* End stim loops */
         copcomb(pspk);
#endif

#ifdef PAR0
/* Node 0 code: Receive the matrix */
         for (pmtx=pmtx0,i=1; i<=nst1; i++) {
            pnrCop.count = i;
            coppack(pspk, pmtx, &pnrCop, 1);
            pmtx += i;
            }
         copcomb(pspk);
#endif

#endif         /*** End parallel matrix collection ***/

#ifndef PARn
/* Print the cross-response matrix */

         if (lkst & KSTYM)
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
               convrt("(P2XRUJ5)",&cnt,pmtx,NULL);
               }

            /* Calculate amount of offset to next diagonal point,
            *  viz. F(S+MXCOL)-F(S), where F(S)=S*(S-1)/2, S=IDIAG */
            pdiag += MXCOL*idiag + MXCOL*(MXCOL-1)/2;
            } /* End cross-response matrix */

/* Calculate and print the correlation matrix.  Be careful to avoid
*  overflows which might occur with integer arithmetic if a layer
*  has more than 1<<15 cells.  Note that because the input data are
*  binary, sum(x*x) = sum(x), etc.  */

         if (lkst & KSTZC) {
            double xsum,ysum,xnorm,ynorm,corr;
            si32 *psmtx,*psdiag; /* Signed arrays for correlations */
            knr = pmdlt->pnr;
            for (psmtx=(si32 *)pmtx0,i=0; i<nst1; i++) {
               if (!knr[i]) { psmtx += (i+1); continue; }
               xsum = uwdbl(knr[i]), xnorm = xsum*(dnnel - xsum);
               for (j=0; j<=i; psmtx++,j++) {
                  if (!quw(knr[j])) continue;
                  ysum = uwdbl(knr[j]), ynorm = ysum*(dnnel - ysum);
                  if ((corr = xnorm*ynorm) > 0) corr =
                     (dnnel*(double)(*psmtx) - xsum*ysum) / sqrt(corr);
                  *psmtx = (si32)(round(1000.0 * corr));
                  }} /* End calculating correlation matrix */

            psdiag = (si32 *)pmtx0;
            for (idiag=1; idiag<=nst1; idiag+=MXCOL) {
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
               for (psmtx=psdiag,i=idiag; i<=nst1; psmtx+=i,i++) {
                  int cnt = i-idiag+1; if (cnt > MXCOL) cnt = MXCOL;
                  convrt("(P2XRIJ5)",&cnt,psmtx,NULL);
                  }

               /* Calculate amount of offset to next diagonal point,
               *  = F(S+MXCOL)-F(S), where F(S)=S*(S-1)/2, S=IDIAG */
               psdiag += MXCOL*idiag + MXCOL*(MXCOL-1)/2;
               } /* End printing correlation matrix */
            } /* End correlation matrix */

#endif   /* !def PARn */

         } /* End cross-response and correlation matrices */

/*---------------------------------------------------------------------*
*                      Cross-response statistics                       *
*---------------------------------------------------------------------*/

/* The strategy for parallelization is as follows:
*     All nodes accumulate in parallel their contributions to the nr
*     sums (one per stimulus, see above), to the nhit,...nccc stats
*     (one set per GRPNO card) and to the ng and ncas sums (one per
*     class per GRPNO) using their local copy of the STAT structure
*     and its appendages.  These are then packed into message buffer
*     space and vecollected onto host.  This scheme minimizes communi-
*     cation, yet permits total statistics larger than one message
*     buffer while preserving the data structure of the serial pgm.
*  The whole thing is repeated for each modality and each GRPNO.  */

      if (lkst & KSTXR) {
#ifndef PARn
         int mlines;
         cryout(RK_P2, "0CLASSIFICATION STATISTICS FOR ",
            RK_NFSUBTTL|RK_LN2, getrktxt(pmdlt->hmdltname),
            LTEXTF, cimw, fmtmwdw(pmdlt), NULL);
#endif

/* Prepare GSDEF and GST blocks for cross-response accumulation.
*  N.B.  Must skip pgs over ncs GSDEFs per GRPNO, even though
*  only ncl of these have actually been used.  */

         pgs = pmdlt->pgsd;         /* Locate GSDEF structures */
         for (igpnt=1; igpnt<=mgpnt; igpnt++,pgs+=pmdlt->ncs) {

            int ncl = pncl[igpnt-1];
            struct GSDEF *kgs,*kgs2=pgs+ncl;
            struct GSDEF *kgsj;
            struct GST gst;         /* Scalar cross-response sums */

#ifndef PARn
            /* Accumulators for output on host */
            double atnel;
            double agtra,agter;
            double ahtra,ahter;
            double amtra,amter;
            double antra,anter;
            double axtra,axter;
            double ahit,ahits;
            double aaxtra,aaxter;
            double amrat,axrat,anrat,aamrat,aaxrat;
            double agrat,ahrat;
            xstt   nhits;
#endif
            for (kgs=pgs; kgs<kgs2; kgs++) {
               kgs->nstj = bitcnt(kgs->grpmsk,lndsb);
               kgs->ng = jeul(0);
               kgs->ncas = 0;
               }
            memset((char *)&gst, 0, sizeof(struct GST));

/* Accumulate contributions from a single node (or serial) */

#ifndef PAR0
            for (G=g1+oxrmi; G<g2; G+=ginc) {
               long i, lkt, hitcnt = bitcnt(G, nst1b);
               if (hitcnt) {
                  char ftra = FALSE, fter = FALSE;
                  gst.nhit = jaul(gst.nhit,1);
                  /* Count cross-responses by classes.
                  *  Rev, 08/11/12, GNR - Now that marker data are
                  *  aligned, a simple bytewise AND will do.
                  *  Note, 08/14/18, GNR - This use of CDAT.pmarks is
                  *  unrelated to use in d3go--just a handy temp.  */
                  for (kgs=pgs; kgs<kgs2; kgs++) {
                     xstt ktra = jeul(0), kter;
                     for (i=0; i<lndsb; ++i)
                        CDAT.pmarks[i] = G[i] & kgs->grpmsk[i];
                     lkt = bitcnt(CDAT.pmarks, lndsb);
                     kgs->nhtj = ktra = jeul(lkt);
                     if (lkt > 0) {
                        kgs->ng = jauw(kgs->ng, kgs->nhtj);
                        kgs->ncas += 1;
                        if (lkt >= 2) {
                           ftra = TRUE;
                           gst.htra = jauw(gst.htra,ktra);
                           gst.mtra = jauw(gst.mtra,
                              jmuw(kgs->nstj,kgs->nstj-1));
                           gst.ntra = jauw(gst.ntra, jmuw(lkt,lkt-1));
                           }
                        if (hitcnt - lkt > 0) {
                           fter = TRUE;
                           gst.hter = jauw(gst.hter,ktra);
                           for (kgsj=pgs; kgsj<kgs; kgsj++) {
                              kter = kgsj->nhtj;
                              if (quw(kter) > 0) {
                                 gst.mter = jauw(gst.mter,
                                    jmuw(kgs->nstj,kgsj->nstj));
                                 gst.nter = amuwwd(gst.nter,
                                    ktra, kter);
                                 gst.nccc = jaul(gst.nccc,1);
                                 }
                              } /* End loop over inter-class pairs */
                           } /* End if any inter-class pairs */
                        } /* End if ktra */
                     } /* End kgs loop */
                  if (ftra) gst.gtra = jaul(gst.gtra,1);
                  if (fter) gst.gter = jaul(gst.gter,1);
                  else      gst.ncc1 = jaul(gst.ncc1,1);
                  } /* End if hitcnt */
               else /* Count cells that had no hits */
                  gst.ncc0 = jaul(gst.ncc0,1);
               } /* End G for */

#endif

/* Pack and collect */

#ifdef PAR
            /* Comp nodes send to host, Node 0 receives */
            StatCop.count = XSTASIZ;
            coppack(pspk, &gst, &StatCop, 1);
            StatCop.count = 1;
            for (kgs=pgs; kgs<kgs2; kgs++)
               coppack(pspk, &kgs->ng, &StatCop, 1);
            pnrCop.count = 1;
            for (kgs=pgs; kgs<kgs2; kgs++)
               coppack(pspk, &kgs->ncas, &pnrCop, 1);
            copcomb(pspk);
#endif

/* Proceed with printing.
*  Note:  Calculation of axtra (expected intra-class pairs) is
*  Sum(groups)[Sum(j in group, k!=j in group) nhitsj * nhitsk]/(2*nel)
*  = Sum(groups)[Sum(j) nhitsj * (ng(group) - nhitsj)]/(2*nel)
*  = Sum(groups)[Sum(j) nhitsj * ng - nhitsj * nhitsj)]/(2*nel)
*  = Sum(groups)[ng * ng] - Sum(stim)[nhitsj * nhitsj])/(2*nel)  */

#ifndef PARn
            gst.mtra = jsruw(gst.mtra,1);
            gst.ntra = jsruw(gst.ntra,1);
            nhits = jeul(0);
            for (knr=pmdlt->pnr; knr<knr2; knr++)
               nhits = jaul(nhits,*knr);

            /* Expected intra- and inter-class cross-responses */
            atnel = 2.0*dnnel;
            antra = uwdbl(gst.ntra); anter = uwdbl(gst.nter);
            amtra = uwdbl(gst.mtra); amter = uwdbl(gst.mter);
            axtra = axter = 0.0;
            ahit  = uwdbl(gst.nhit); ahits = uwdbl(nhits);
            agtra = uwdbl(gst.gtra); ahtra = uwdbl(gst.htra);
            agter = uwdbl(gst.gter); ahter = uwdbl(gst.hter);

            for (kgs=pgs; kgs<kgs2; kgs++) {
               double ang = uwdbl(kgs->ng);
               axtra += ang*ang;
               axter += ang*(ahits-ang); }

            for (knr=pmdlt->pnr; knr<knr2; knr++) {
               double lnr = *knr;
               axtra -= lnr*lnr; }

            axtra /= atnel;
            axter /= atnel;
            if (ahit)  { agtra /= ahit; agter /= ahit; }
            if (ahits) { ahtra /= ahits; ahter /= ahits; }
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
            mlines = (quw(gst.gtra) > 0) + (quw(gst.gter) > 0) +
               (agrat != 0.0);
            tlines(1 + (mlines > 0) + mlines);
            convrt("(P2,22H0CROSS RESPONSE (GRPNOUH3,11H):  STIMULIIL5,"
               "8H CLASSESI4,4X10HCELLS RESPUW9,4X10HTOTAL HITSUW10)",
               &igpnt,&nstimu,&ncl,&gst.nhit,&nhits,NULL);
            if (mlines > 0) {
               cryout(RK_P2, "                         CRMS        C/CR"
                      "        HITS        H/TH       PAIRS    MAX POSS"
                      "        P/MP    EXPECTED        P/XP",
                  RK_LN1+125, NULL);
               if (quw(gst.gtra) > 0) convrt("(P2,4X11HINTRA-CLASS"
                  "UW14,Q12.3,UW12,Q12.3,2*UW12,3*Q12.3)",
                  &gst.gtra, &agtra, &gst.htra, &ahtra, &gst.ntra,
                  &gst.mtra, &amtra, &axtra, &aaxtra, NULL);
               if (quw(gst.gter) > 0) convrt("(P2,4X11HINTER-CLASS"
                  "UW14,Q12.3,UW12,Q12.3,2*UW12,3*Q12.3)",
                  &gst.gter, &agter, &gst.hter, &ahter, &gst.nter,
                  &gst.mter, &amter, &axter, &aaxter, NULL);
               if (agrat) convrt("(P2,4X11HINTRA/INTERQ14.3,8*Q12.3)",
                  &agrat, &agrat, &ahrat, &ahrat, &anrat, &amrat,
                  &aamrat, &axrat, &aaxrat, NULL);
               }

            /* Statistics on class responses by cells */
            nhits = jeul(0);
            for (kgs=pgs+1; kgs<kgs2; kgs++)
               for (kgsj=pgs; kgsj<kgs; kgsj++)
                  nhits = jauw(nhits, jmuw(kgs->ncas, kgsj->ncas));
            agter = uwdbl(nhits)/dnnel;
            agrat = agter ? (double)gst.nccc/agter : 0.0;
            nhits = jeul(il->nelt) - gst.ncc0 - gst.ncc1;
            convrt("(P2,4X23HCELLS RESP TO: NO STIM J0UW12,"
               "10H, 1 CLASS J0UW12,11H, >1 CLASS J0UW12,8H, PAIRS "
               "J0UW12,11H, EXPECTED J0Q12.3,8H, NP/NX J0Q12.<5)",
               &gst.ncc0, &gst.ncc1, &nhits, &gst.nccc, &agter,
               &agrat, NULL);
#endif
            } /* End pgs for */
         } /* End KSTXR statistics */

#ifndef PAR0
      oxrmi += pmdlt->ndsbyt; /* Not incremented if modality skipped */
#endif
SkipModality: ;

/*---------------------------------------------------------------------*
*                     Response history statistics                      *
*---------------------------------------------------------------------*/

/* Note:  Prior to V8A, this code was located before the KSTXR code,
*  and statistics for just the last two cycles were collected by
*  looping over XRM fields in the repertoire data.  Now the counts
*  are collected in d3go during all cycles.  It is unnecessary to
*  count cycles that do and do not have new stimuli--this can be
*  determined from the counts in the phrs arrays.  */

      if (lkst & KSTHR) {
#ifndef PARn
#if LHSTAT > LDSTAT
#error Increase dimension of stat array in d3stat.c
#endif
         double dns,dnd;         /* Floating case counts */
         long nsame, ndiff;      /* Counts for two cases */
         int i;                  /* Normalization loop ctr */
#endif

#ifdef PAR
         /* Collect response history data down to host */
         coppack(pspk, phr, &HistCop, 1);
         copcomb(pspk);
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
         nsame /= (long)il->nelt;
         ndiff /= (long)il->nelt;

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
         } /* End if KSTHR */

      } /* End loop over modalities */

   } /* End cross-response stats */

/*---------------------------------------------------------------------*
*                       Resume normal indenting                        *
*---------------------------------------------------------------------*/

         } /* End loop over cell types */
      } /* End loop over repertoires */
/* Clean up cross-response collection */
#ifdef PAR
   if (RP->kxr & (XSTATS|YSTATS|ZSTATS|HSTATS))
      copfree(pspk);
#endif
#ifndef PARn
   /* Report any overflows but do not terminate the run.  */
   if (RP->ovflow) {
      cryout(RK_P1, "0-->WARNING: SOME OVERFLOWS OCCURRED DURING "
         "STATISTICS PROCESSING", RK_LN2);
      }
   /* Clear any remnant subtitles */
   cryout(RK_P2, blanks, RK_NTSUBTTL+RK_LN1+4, NULL);
   /* Flush last print buffer so on-line user can see stats */
   cryout(RK_P1, "\0", RK_LN0+RK_FLUSH+1, NULL);
   spout(SPTM_NONE);          /* Turn off "SPOUT ALL" */
#endif
   } /* End d3stat() */

