/* (c) Copyright 1991-2012 Neurosciences Research Foundation, Inc. */
/* $Id: d3nset.c 52 2012-06-01 19:54:05Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                               d3nset                                 *
*                                                                      *
*  Routine to set (or reset) system values and pointers which are lost *
*  via broadcast of tree data.  This routine is executed each time a   *
*  new CYCLE card is read, so items that can change at Group III time  *
*  can be handled here.  Reallocation of items that do not change is   *
*  redundant in the serial case.  In any event, care must be taken to  *
*  avoid reallocating s(i) ptr array (to preserve permutation) or any- *
*  thing else that must be preserved when there is no series reset.    *
*  Items that must be saved across broadcasts and that do not depend   *
*  on the cell type are stored in CELLDATA.                            *
*                                                                      *
*  Memory for decaying synaptic inputs that is subject to reallocation *
*  is zeroed for consistency (and this is the only place it is zeroed  *
*  prior to execution of the first trial).  If it becomes desirable to *
*  preserve these data across CYCLE cards, then each array must be in- *
*  dependently realloc()'d rather than globally malloc()'d and cleared.*
*  It will then take elaborate measures to avoid memory fragmentation. *
*                                                                      *
*  Calculation of individual cell Vrest for RF_IZH3 cells with izrb    *
*  is done here just because there is already a convenient loop over   *
*  cell types and the timing is right.                                 *
*                                                                      *
*  V4A, 03/26/90, JWT - Newly written                                  *
*  V5C, 11/18/91, GNR - Reorganize so d3genr calls for initial alloc   *
*  Rev, 02/21/92, GNR - Add call to d3lij, needed for OPT=STORAGE      *
*  V5E, 07/12/92, GNR - Revise to reenable GCONN's in serial version   *
*  Rev, 12/09/92, ABP - Add HYB code.                                  *
*  Rev, 02/27/93, GNR - Add KRPNZ bit                                  *
*  V6C, 08/15/93, GNR - Remove 2**n dependency, ready for load optim.  *
*  Rev, 08/18/93, GNR - Initialize repertoire data to 0--fix Mij bug   *
*  V6D, 02/01/94, GNR - Add DELAY, remove boxsum alloc for PAR         *
*  V7A, 04/24/94, GNR - Restore length calcs now changed in d3allo     *
*  V7B, 07/15/94, GNR - Bug fix: avoid bashing il->llt on node 0       *
*  Rev, 08/25/94, GNR - Allocate space for GCONN veff, falloff         *
*  V8A, 05/06/95, GNR - Add MODALITY and stat cntrl at celltype level, *
*                       nsetindv for AffData and GCONN decay,falloff   *
*  Rev, 11/22/96, GNR - Bug fix: preserve serial ps2 after first call  *
*  Rev, 02/16/97, GNR - Remove all random seed updating to d3go        *
*  Rev, 03/01/97, GNR - Allocate pps on host same as all other nodes   *
*  Rev, 09/28/97, GNR - Add psyn0 for each conntype                    *
*  Rev, 12/27/97, GNR - Add ADECAY, changeable allocation of stats     *
*  Rev, 03/27/98, GNR - Add memset(0) for aeff, i[ei]veff, mvaleff     *
*  Rev, 10/28/00, GNR - Add allocation for history statistics          *
*  V8B, 12/27/00, GNR - Move to new memory management routines         *
*  Rev, 09/10/01, GNR - Add setup for probes with cell lists and pptr  *
*  V8C, 02/27/03, GNR - Cell responses in millivolts, add conductances *
*  V8D, 01/28/06, GNR - New memory allocation scheme for conductances  *
*  ==>, 12/29/07, GNR - Last mod before committing to svn repository   *
*  Rev, 04/21/08, GNR - Add ssck (sum sign check) mechanism            *
*  V8E, 01/29/09, GNR - Add Izhekevich cells, reorganize stat offsets  *
*  V8F, 05/17/10, GNR - Add allocation for KRPGP statistics            *
*  Rev, 07/01/10, GNR - Add allocation of shared work area psws        *
*  V8G, 08/13/10, GNR - Add allocation of autoscale work area          *
*  Rev, 03/30/12, GNR - Add distribution of Sj statistic               *
*  Rev, 05/26/12, GNR - Set to store raw specific affsums as si64      *
***********************************************************************/

#define CLTYPE  struct CLBLK
#define LIJTYPE struct LIJARG

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "rkarith.h"
#include "d3global.h"
#include "clblk.h"
#include "celldata.h"
#include "rocks.h"

extern struct CELLDATA CDAT;
extern struct LIJARG Lija;
extern void d3i3avr(struct CELLTYPE *il);
extern long lrsettbl(void);
extern long lcmvsums(void);

/*=====================================================================*
*                              d3nsetion                               *
*                                                                      *
*  Called separately for REPBLOCK and CELLTYPE IONTYPE chains to set   *
*  z * Faraday constant and pointers for locating cell-external ion    *
*  concentrations.  (Internal and cell-specific external ion concs     *
*  are located via il->prd + il->ctoff[IONC] + oCi [+4].  Since one    *
*  REGION-level IONTYPE block serves multiple CELLTYPEs, there is no   *
*  one value that can go here for cell-specific ion concentrations.)   *
*=====================================================================*/

static void d3nsetion(struct IONTYPE *pion, float **ppgion) {

   for ( ; pion; pion=pion->pnion) {

      pion->zF = Faraday * (float)pion->z;

      if (pion->ionflgs & (CDOP_IONEXT|CDOP_IONREV)) {   /* Variable */
         if (pion->ionopts & ION_REGION)
            pion->pCext = &pion->Cext;             /* Per region */
         else if (pion->ionopts & ION_GROUP)
            pion->pCext = (*ppgion)++;             /* Per group */
         else
            pion->pCext = NULL;                    /* Per cell */
         }
      else                                               /* Constant */
         pion->pCext = &pion->C0ext;

      } /* End IONTYPE loop */

   } /* End d3nsetion() */

/*=====================================================================*
*                               d3nset                                 *
*                                                                      *
*     Calculates node-specific values (locell, mcells, etc.) on        *
*  computational nodes.  The argument and also the value returned      *
*  is a pointer to the storage space allocated for D3 data on this     *
*  node. (This must be kept outside the RP tree to avoid destruction   *
*  at tree broadcast time.)  d3nset() should be called with a NULL     *
*  argument the first time, causing the space to be calloc()'d and     *
*  allocated to the various repertoires.                               *
*     Space for various kinds of afferent sums (see d3oafs.c), gconn   *
*  work areas, connection decay, autoscaling, and most statistics is   *
*  allocated using high-water-mark algorithms.  These may run out of   *
*  space late in a long simulation.  Accordingly, the Group I OPTIMIZE *
*  card has options to set aside reserves for these allocations.  To   *
*  grab the reserve, the first event list allocation is done here when *
*  myprd == 0 rather than when first needed.                           *
*     On a parallel computer, only pgpd, pps, psws, prgtht, and stats  *
*  are allocated on host node.                                         *
*=====================================================================*/

rd_type *d3nset(rd_type *myprd) {

   struct REPBLOCK *ir;
   struct CELLTYPE *il;
   struct CONNTYPE *ix;
   struct INHIBBLK *ib;
   struct MODBY    *im;
#ifndef PAR0
   struct MODVAL   *imv;
#endif

   char     *pgd;          /* Ptr to generic data */
   float    *pgion;        /* Ptr to next group-specific ion conc */
   rd_type  *pnxtgd;       /* Ptr to next group data array */
   s_type  **pnxtps;       /* Ptr to next s(i) pointer table */
   s_type   *pnxts;        /* Ptr to next s(i) array */
   char     *pnxti;        /* Ptr to next individual work space */
#ifndef PAR0
   s_type   *pnxts2;       /* Ptr to next new-s(i) table (ps2) */
   rd_type  *pnxtr;        /* Ptr to next repertoire data */
   rd_type  *pnxtsyn0;     /* Ptr to rep data for next conntype */
#ifndef PAR
   char     *pnxtc;        /* Ptr to shared GCONN work space */
   char     *pnbands;      /* Ptr to space for bandsums */
   char     *pnnorm;       /* Ptr to next normalization table */
#endif
#endif

   /* All these lengths are understood to be in bytes */
   long lgall = 0;         /* Length of all group data on this node */
   long lrall = 0;         /* Length of all rep data on this node */
   long lsall = 0;         /* Length of all s(i) on this node */
   long nslu  = 0;         /* Length of full set of s(i) arrays */
   long nslup = 0;         /* Length of si lookup table pointers */
#ifndef PAR0
   long ldelc = 0;         /* Length for saving delta(Cij) values */
   long nslu2 = 0;         /* Length of update s(i) (ps2) arrays */
#endif
   long lindv;             /* Total changeable allocation required */
   long lrgtht;            /* Length of prgtht area */
   long lsws,msws;         /* Length of shared work space */
   long ltot;              /* Total allocation required */
#ifdef PARn
   long relnode;           /* Node relative to first node used */
#endif
   int  dostat;            /* Local copy of cell type stats switch */
   int  kcall;             /* Keeps track of whether first time */
   int  odist;             /* Offset to next count stat */
   int  oddst;             /* Offset to next ddist stat */
   int  ogstat = 0;        /* Offset to next KRPGP stat */

/* Start alloc counts with items already known.  lindv must be
*  aligned up every time anything is allocated that is not for
*  sure a multiple of sizeof(si64) because some stats are si64's.
*  Repertoire data are aligned up at every cell for performance.  */

   kcall = myprd ? KMEM_GO : KMEM_GENR;
   /* AffData and AffD64 space needed on Node 0 for dprnt messages */
   lindv = RP->lAffD64 + RP->lAffData;

#ifdef PAR0
   lsws = 0;               /* Find max shared work space for: */
#else
   lsws = lrsettbl();                        /* Reset tables */
   msws = RP->mxsaa*sizeof(struct CONNDATA); /* Cij averaging */
   if (msws > lsws) lsws = msws;
#endif
#ifndef PARn
   lsws = max(lsws,2*LNSIZE+4);              /* Detailed print */
   msws = lcmvsums();                        /* IJplot avg sums */
   if (msws > lsws) lsws = msws;
#endif

#ifdef PAR
   ltot = 0;
#else
   ltot = ALIGN_UP(RP->lishare + RP->lifixed);
#endif

/*---------------------------------------------------------------------*
*  Scan through all repertoires and celltypes to determine space       *
*  needed on this node.  In parallel versions, this differs from       *
*  the calculation done in d3allo, which gives largest needed on       *
*  any one node for d3echo.  Total space for s(i)'s for this layer     *
*  is mxsdelay+1 pointers in pps array plus mxsdelay+1 full sets       *
*  of s(i),phase data and modality markers that pps ptrs point to,     *
*  plus one set of s(i),phase for mcells and modality markers in ps2.  *
*---------------------------------------------------------------------*/

   for (ir=RP->tree; ir; ir=ir->pnrp) {

      lgall += ir->lgt;

      for (il=ir->play1; il; il=il->play) {

#ifdef PAR0
         /* Parallel host node--handles no cells, so there is
         *  no contribution to lrall.  However, total length of
         *  repertoire data set by d3allo (il->llt) must be
         *  left untouched for use by d3save and d3rstr.  */
         il->locell = il->mcells = 0;
#endif
#ifdef PARn
         /* Parallel node n--allocate my cells */
         relnode = NC.node - il->node1;
         if (relnode >= 0 && relnode < il->nodes) {
            if (relnode < il->crn)
               /* Node is in low group with cpn+1 cells */
               il->locell = relnode * (il->mcells = il->cpn + 1);
            else
               /* Node is in high group with cpn cells */
               il->locell = relnode * (il->mcells = il->cpn) + il->crn;
            }
         else
            il->locell = il->mcells = 0;
         lrall += (il->llt = il->mcells*il->lel);
         nslu2 += spsize(il->mcells,il->phshft) + il->lmm;
#endif
#ifndef PAR
         /* Serial--all cells on host.  Length of repertoire
         *  data (il->llt) has been set correctly by d3allo.  */
         il->locell = 0;
         il->mcells = il->nelt;
         lrall += il->llt;
         nslu2 += il->lspt + il->lmm;
#endif
         il->logrp = il->locell/il->nel;
         /* Accumulate space needed for s(i)'s for this layer */
         nslu  += il->mxsdelay1*(il->lspt + il->lmm);
         nslup += il->mxsdelay1*sizeof(s_type *);
#ifndef PAR0
         /* Set KRPNZ if rep has at least one cell on this node */
         if (il->mcells > 0) ir->Rp.krp |= KRPNZ;
#endif

/* Setting noise flag here saves doing it in about 4 other places */

         il->ctf &= ~CTFHN;
         if ((il->No.nmn | il->No.nsg) && il->No.nfr)
            il->ctf |= CTFHN;

/* Accumulate space needed for items (currently decay, statistics,
*  delta Cij's, falloff, and normalization) whose length is allowed to
*  change at Group III time.  Stats and sums are on all nodes, others
*  are not on node 0.  No need to test RP_NOSTAT, as CTPNS is always
*  set in this case.  */

         dostat = !(il->Ct.kctp & CTPNS);
         odist = oddst = 0;

         /* CELLTYPE statistics */
         if (dostat) {
            if (il->Ct.kctp & KRPCL)
                              il->oCdist = odist, odist+=il->nCdist;
            if (il->Ct.kctp & (KRPGP|KRPMG))
                              il->oGstat = ogstat, ogstat+=il->nCdist;
            if (il->phshft)   il->oFdist = odist, odist+=2;
            if (il->Ct.rspmeth >= RF_BREG)   ++oddst;
            else if (il->Dc.CDCY.kdcy)       ++oddst;
            if (il->ctf & CTFDN)             ++oddst;
            if (il->prfrc)                   ++oddst;
            }

         /* CONNTYPE items:  decay, regular and detailed amp stats */
         for (ix=il->pct1; ix; ix=ix->pct) {
            int kdecay = (ix->Cn.ADCY.kdcy != NODECAY);
#ifndef PAR0
            /* Allocate space for decay.  Use one or two si32 per cell
            *  each for excitatory or inhibitory sums.  */
            lindv += ALIGN_UP(I32SIZE * il->mcells * ix->Cn.ADCY.ndal *
               npsps((int)ix->cssck));

            /* The following is really once-only code, but it is
            *  placed here rather than in d3genr() because some
            *  pointers are set that would not survive CONNTYPE
            *  broadcasts.  Trick:  BytesInXBits = 4-bit entries
            *  in a 32-bit word (divide by eight, round up) */
            if (ix->kgen & KGNMC) {
               struct FDHEADER *pfdhx = &RP->pfdh[ix->ucij.m.mno-1];
               long mtxsz = BytesInXBits(pfdhx->mrx*pfdhx->mry);
               ix->ucij.m.fdptr  = pfdhx->pfdm;
               ix->ucij.m.fdincr = mtxsz*ix->ucij.m.nks;
               ix->ucij.m.fdtop  = ix->ucij.m.fdptr +
                  mtxsz*ix->ucij.m.nkt;
               } /* End matrix Cij setup */

            /* Allocate space for keeping track of subarbor borders
            *  for CTPSB plots.  */
            if (il->Ct.kctp & CTPSB) {
               msws = (long)ix->nsarb * sizeof(struct NAPSLIM);
               if (msws > lsws) lsws = msws;
               }
#endif
            /* Allocate offsets for statistics */
            if (dostat) {
               ix->ocnddst = oddst;
               oddst += NCNDDST + kdecay + cnexists(ix, RBAR);
               ix->cnflgs &= ~DOFDIS;
               if ((il->Ct.kctp & KRPFD) && (ix->kgen & KGNMC)) {
                  ix->cnflgs |= DOFDIS;
                  ix->ucij.m.oFdist = odist;
                  odist += ix->ucij.m.nkt; }
               /* ix->ldas is 0 if no DAS, no need to test */
               lindv += ix->ldas;
               }

            } /* End specific conntype loop */

         /* INHIBBLK items:  decay, falloff, stats */
         for (ib=il->pib1; ib; ib=ib->pib) {
            /* Count excit,inhib bands, set PSP bits */
            int kdecay = (ib->GDCY.kdcy != NODECAY);
#ifdef PAR0
            /* Make sure psws array has enough temp space for the
            *  largest falloff table--this used on Node 0 just for
            *  printing these tables.  */
            if ((ib->ibopt & (IBOPFO|IBOPTT)) == (IBOPFO|IBOPTT)) {
               msws = lfofftbl(ib, FALSE);
               if (msws > lsws) lsws = msws;
               }
#else
            /* Allocate exact space needed for decay, viz., ndal
            *  decay arrays for all positive psp sums and another
            *  for all negative sums.  Need this space per cell if
            *  OPT=V, otherwise per group.  */
            if (kdecay)
               lindv += ALIGN_UP(I32SIZE * npsps((int)ib->gssck) *
                  ib->GDCY.ndal * NDecayCells(il, ib, il->mcells));
            /* Allocate space for falloff tables */
            if ((ib->ibopt & (IBOPFO|IBOPTT)) == (IBOPFO|IBOPTT))
               lindv += ALIGN_UP(lfofftbl(ib, FALSE));
#endif
            /* Allocate offsets for statistics */
            if (dostat) {
               ib->oidist = oddst;
               oddst += ib->nib + kdecay; }
            } /* End INHIBBLK scan */

         /* MODULATE items:  Only allocation item is statistics.
         *  Clear variables representing decaying pseudo-cells.
         *  The umve union is not needed now, but is there for
         *  when we want to preserve mvaleff across CYCLE cards.  */
         for (im=il->pmby1; im; im=im->pmby) {
            im->umve.mvaleff = 0;
            if (dostat) {
               im->omdist = oddst;
               oddst += 1 + (im->MDCY.kdcy != NODECAY); }
            } /* End MODBY loop */

         /* CELLTYPE statistics, now including above dists */
         if (dostat) lindv += ALIGN_UP(sizeof(struct CLSTAT)) +
               sizeof(ddist) * (il->nctddst = oddst) +
               sizeof(dist)  * (il->nctdist = odist) +
               (il->Ct.kctp & KRPHR ? sizeof(hist)*il->nmdlts : 0);

#ifndef PAR0
         /* Allocate space for delta Cij detail print.  Just
         *  allocate max(celltypes)(il->nce), hence no danger
         *  of an allocation failure as amp, decay, dprnt are
         *  turned on or off.  */
         if (il->nce > ldelc) ldelc = il->nce;
#endif

         } /* End loop over cell layers */

      } /* End loop over repertoires */

/* Determine total space that must be allocated for all s(i) and
*  repertoire data on this node.  The nslu2 data do not exist on
*  a PAR0 node.  */

   lgall = ALIGN_UP(lgall);
   lrall = ALIGN_UP(lrall);
   nslu  = ALIGN_UP(nslu);
   /* Space for all s(i) + modality marks + max D1's etc. */
#ifdef PAR
#ifdef PAR0
   lsall = nslu + nslup + RP0->xtrasize;
#else
   lsall = nslu + nslup + nslu2;
#endif
#else /* Serial */
   lsall = nslu + nslup + nslu2 + RP0->xtrasize;
#endif
   CDAT.ngstat = ogstat;
   ogstat *= sizeof(struct GPSTAT); ogstat = ALIGN_UP(ogstat);
   lrgtht = sizeof(si32)*RP->mxngtht; lrgtht = ALIGN_UP(lrgtht);
   lindv += ogstat + lrgtht;
#ifdef PARn
   msws = lrgtht << 1;
   if (msws > lsws) lsws = msws;
#endif
#ifndef PAR0
   ldelc *= SHSIZE; ldelc = ALIGN_UP(ldelc);
   lsws = ALIGN_UP(lsws);
   lindv += ldelc + lsws;
#endif
   lsall = ALIGN_UP(lsall);
   ltot += lgall + lrall + lsall;

/*---------------------------------------------------------------------*
*  If the space was not already allocated, i.e., this is the first     *
*  call, allocate storage now for all repertoires, initial event list, *
*  and anything else in CELLDATA that must be preserved across Group   *
*  III broadcasts.  Include reserve in lindv space.  Changed 08/18/93  *
*  to zero the allocated storage so Mij, modality markers, and any     *
*  future variables not set in d3genr will be correctly initialized.   *
*  Rev, 09/28/07, GNR - Add space in rsumaij for rbar sums             *
*  Rev, 02/25/11, GNR - Use max(lmm) instead of cumnds to alloc tmark0 *
*---------------------------------------------------------------------*/

   if (kcall == KMEM_GENR) {
      myprd = (rd_type *)callocv(1, ltot, "Region data");
      lindv += ALIGN_UP(RP->ldrsrv);
#ifndef PAR0
      /* Allocate space for Aij and Rbar totals by connection type
      *  and excitatory and inhibitory subarbor and raw Aij sums.
      *  Rbar space is allocated whether needed or not--is relatively
      *  small and avoids need for some ifs in d3go().  */
      {  size_t lsums = 2 * sizeof(si64) * (RP->mxnct + RP->mxlax);
         CDAT.rsumaij = (si64 *)mallocv(lsums, "Raw connection sums");
         } /* End lsums local scope */

      /* Allocate work space for modality marking */
      if (RP->kxr & MXSTATS) CDAT.tmark0 =
         (byte *)mallocv((size_t)RP->mxlmm, "Response markers");

      /* Allocate space related to connection types */
      if (RP->mxnct > 0) {
         CDAT.psumaij = (si32 *)mallocv(RP->mxnct*sizeof(si32),
            "processed connection sums");
         CDAT.pglway = (short *)mallocv(RP->mxnct*
            ((MAX_WAYSET_CODES+1)*sizeof(short)), "Amp scales");
         }
#endif
      }

/* High-water mark allocation for individual data that may change at
*  Group III time.  realloc() is not needed because data do not need
*  to be copied.  If this changes, must copy each block individually.
*/

   if (lindv > CDAT.lnsetindv) {
      if (CDAT.pAffD64) freev(CDAT.pAffD64,
         "Variable-length connection data");
      CDAT.pAffD64 = (si64 *)mallocv(CDAT.lnsetindv = lindv,
         "Variable-length connection data");
      }

/*---------------------------------------------------------------------*
*  Make a second pass through all repertoires and celltypes.  Assign   *
*  starting addresses for each type of data area. These ptrs (others   *
*  than those derived from CDAT.pAffD64) point to data that may need   *
*  to be preserved across trials, so be sure they always point to the  *
*  same places each time this code runs.  Be sure not to shuffle the   *
*  s(i) delay ptr array (to preserve permutation).  (Each length is    *
*  in bytes, not number of variables, and is aligned up.)              *
*  Also initialize Lij,Cij,Sj,Dij generation.                          *
*---------------------------------------------------------------------*/

   pnxti  = (char *)CDAT.pAffD64 + RP->lAffD64;
   CDAT.pAffData = (long *)pnxti; pnxti += RP->lAffData;
   CDAT.pgstat = (struct GPSTAT *)pnxti; pnxti += ogstat;
   CDAT.prgtht = (si32 *)pnxti; pnxti += lrgtht;
   pgd = (char *)(pnxtgd = myprd); pgd += lgall;
   pnxtps = (s_type **)pgd;
   pnxts  = (s_type *)(pgd + nslup);
#ifndef PAR0
   pnxts2 = pnxts + nslu;
   pgd += lsall;
   pnxtr  = (rd_type *)pgd; pgd += lrall;
   CDAT.pdelcij = (short *)pnxti; pnxti += ldelc;
   CDAT.psws = pnxti; pnxti += lsws;
#ifndef PAR
   pnxtc  = pgd; pgd += RP->lishare;
   pnnorm = pgd;
#endif
#endif

   for (ir=RP->tree; ir; ir=ir->pnrp) {

/* Initialization of group-specific variables */

      ir->pgpd = pnxtgd;
         pnxtgd += ir->lgt;

      /* Set Cext ptrs for ions belonging to this region */
      pgion = (float *)(ir->pgpd + ir->gpoff[IONG]);
      if (ir->pionr) d3nsetion(ir->pionr, &pgion);

/* Loop over layers (cell types)  */

      for (il=ir->play1; il; il=il->play) {
         long lpsrow = il->lspt + il->lmm;
         dostat = !(il->Ct.kctp & CTPNS);

/* Initialize pointers to repertoire data.
*  Note that pps points to an array of pointers indexed by time
*     delay, which are useful for accessing delayed s(i) values.
*  No access is provided to the set of s(i) arrays as a whole.
*  The modality markers are at the end of each row of s(i) data. */

         il->pps  = pnxtps;
         if (kcall != KMEM_GENR) {        /* Not first call */
            pnxtps += il->mxsdelay1;
            pnxts +=  il->mxsdelay1*lpsrow;
            }
         else {               /* First call */
            register int idly;
            for (idly=0; idly<(int)il->mxsdelay1; idly++)
               *pnxtps++ = pnxts, pnxts += lpsrow;
#ifndef PAR          /* Serial:  set ps2 first time only */
            il->ps2 = pnxts2, pnxts2 += lpsrow;
#endif
            }

#ifndef PAR0
#ifdef PAR           /* Parallel: set ps2 after every broadcast */
         il->ps2 = pnxts2;
            pnxts2 += spsize(il->mcells,il->phshft) + il->lmm;
#endif
         /* Allocate space for SBAR, QBAR, DEPR, DST, etc. */
         il->prd = pnxtr, pnxtr += il->llt;
         pnxtsyn0 = il->prd + il->ls;

         /* Initialize Lij, Cij, etc. generation according to kcall
         *  type.
         *
         *  N.B.  This routine establishes pointers that are over-
         *  written by membcst--therefore, must be called after
         *  each broadcast, not just set once-for-all in d3genr(). */
         d3kij(il, kcall);

         /* Set Cext ptrs for ions belonging to this cell type */
         if (il->pion1) d3nsetion(il->pion1, &pgion);

         /* Deal with variable Vrest for RF_IZH3 cells */
         if (il->ctf & CTFI3IVR) d3i3avr(il);
#endif

         /* Allocate space for cell type statistics (all nodes) */
         if (dostat) {
            il->pctddst = (ddist *)pnxti;
               pnxti += sizeof(ddist) * il->nctddst;
            il->pctdist = (dist *)pnxti;
               pnxti += sizeof(dist) * il->nctdist;
            il->CTST = (struct CLSTAT *)pnxti;
               pnxti += ALIGN_UP(sizeof(struct CLSTAT));
            if (il->Ct.kctp & KRPHR) {
               il->phrs = (hist *)pnxti;
               pnxti += sizeof(hist) * il->nmdlts; }
            }

/* Deal with specific connection types */

         for (ix=il->pct1; ix; ix=ix->pct) {
#ifndef PAR0
            /* Allocate space for Lij, Cij, Mij, etc. */
            ix->psyn0 = pnxtsyn0, pnxtsyn0 += ix->lct;

            /* Allocate space for EPSP/IPSP decay */
            if (ix->Cn.ADCY.kdcy) {
               size_t laeff = I32SIZE * ix->Cn.ADCY.ndal *
                  npsps((int)ix->cssck) * il->mcells;
               memset(pnxti, 0, laeff);
               ix->aeff = (long *)pnxti, pnxti += ALIGN_UP(laeff);
               }
#endif
            /* Allocate space for CONNTYPE statistics (all nodes) */
            if (dostat && (ix->Cn.kam & KAMDS)) {
               ix->pdas = (si64 *)pnxti;
                  pnxti += ix->ldas;
               } /* End CONNTYPE stats */
            } /* End CONNTYPE loop */

/* Deal with geometric connection types */

#ifndef PAR
/* In the serial case, bandsums will be allocated consecutively
*  for all inhibblks from an origin determined by the largest
*  boxsum array needed.  Calculate that origin now.  */

         pnbands = pnxtc + (((ir->ngx+ir->ngy+il->mxnr)*il->mxnr)<<4)
            + 12*ir->ngx*ir->ngy;
#endif

         for (ib=il->pib1; ib; ib=ib->pib) {
#ifndef PAR0
            /* Allocate space for effective gconn decay (ndal si32s per
            *  postsynaptic potential sign (positive or negative) per
            *  cell if self-avoidance set, else per group).
            *  We don't need to determine whether our node starts on
            *  an even group boundary, just allocate the standard amt.
            */
            if (ib->GDCY.kdcy) {
               size_t lgdcy = ALIGN_UP(I32SIZE * ib->GDCY.ndal *
                  NDecayCells(il, ib, il->mcells));
               if (ib->gssck & PSP_POS) {
                  memset(pnxti, 0, lgdcy);
                  ib->ieveff = (long *)pnxti, pnxti += lgdcy; }
               if (ib->gssck & PSP_NEG) {
                  memset(pnxti, 0, lgdcy);
                  ib->iiveff = (long *)pnxti, pnxti += lgdcy; }
               } /* End space calc for GCONN decay */

            /* Allocate space for falloff table if needed */
            if ((ib->ibopt & (IBOPFO|IBOPTT)) == (IBOPFO|IBOPTT)) {
               ib->falloff = (long *)pnxti;
                  pnxti += ALIGN_UP(lfofftbl(ib, FALSE));
               } /* End falloff allocation */
#endif
#ifndef PAR
            /* Allocate space for norm table if needed */
            if (ib->kbc == BC_NORM) {
               long mxngxy = max(ib->l1x, ib->l1y);
               ib->xynorm = (struct XYNORMDEF *)pnnorm;
                  pnnorm += mxngxy*sizeof(struct XYNORMDEF);
               }  /* End if BC=NORM */

            /* Allocate boxsum and hstrip, vstrip arrays */
            ib->boxsum = (long *)pnxtc;
            ib->hstrip = ib->boxsum + ib->l1xn2yn2;
            ib->vstrip = ib->hstrip + ib->l1yn2*ib->l1x;
            ib->boxsum00 = ib->boxsum + ib->l1n1xn2 + ib->l1n1;
            ib->boxsumbb = ib->boxsum + ib->l1n1xn2 + ib->l1yxn2;
            /* Allocate bandsum arrays */
            {  register int iband;
               for (iband=0; iband<(int)ib->nib; iband++) {
                  ib->inhbnd[iband].bandsum = (long *)pnbands;
                     pnbands += ib->l1xy*sizeof(long);
                  }
               } /* End iband local scope */
#endif
            }  /* End INHIBBLK loop */

#ifndef PAR0
/* Setup for probes.  The definition of pptr requires that each
*  node know when to begin processing any cells on the cell list
*  that are on that node.  This calculation is done here.  */

         if (il->pctprb) {
            struct PRBDEF *pprb = il->pctprb + il->ctclid[CTCL_PRBSL];
            CLTYPE *pcb = pprb->pclb;
            ilst *pil = pcb->pclil;
            long ncells;            /* Cells in a list below a bound */

            /* Number of cells in a full cycle of this list */
            ncells = ilstitct(pil, il->nelt);

            if (pprb->pptr > 0) {   /* Don't do all probes in each trial */
#ifdef PAR
               ldiv_t qrm;
#endif
               pprb->lprbcyc = (ncells+pprb->pptr-1)/pprb->pptr;
#ifdef PAR
               ncells        = ilstitct(pil, il->locell);
               qrm           = ldiv(ncells, pprb->lprbcyc);
               pprb->myipc1  = qrm.quot;
               pprb->myipt1  = pprb->pptr - qrm.rem;
#endif
               }
            else {                  /* Do all probes in each trial */
               pprb->lprbcyc = 1;
#ifdef PAR
               pprb->myipc1  = 0;
               pprb->myipt1  = ncells;
#endif
               }

            } /* End PRBDEF setup */

#endif

         } /* End LAYER loop   */
      } /* End REPERTOIRE loop */

#ifndef PAR0
/* Match MODVAL blocks with their corresponding sources.
*  Do not execute on PAR0--want NULL pointers for membcst().  */

   for (imv=RP->pmvv1; imv; imv=imv->pmdv) {

      if (imv->mdsrctyp == VALSRC)
         imv->amsrc = &RP->pvdat[imv->mdsrcndx-1].fullvalue;
      else if (imv->Mdc.mvxc.tvmode == Col_R4)
         imv->amsrc = (s_type *)(RP->paw + imv->omsrc);
      else
         imv->amsrc = (s_type *)RP->pbcst + imv->omsrc;
      imv->mdgetfn = d3gvin(imv->mdsrctyp, &imv->Mdc.mvxc);

      } /* End imv loop */
#endif

#ifndef PARn
#ifdef D1
/* Assign space on host for exchange of D1 states only */
   RP0->ps0 = pnxts;
#endif
#endif

   return myprd;
   } /* End d3nset() */

