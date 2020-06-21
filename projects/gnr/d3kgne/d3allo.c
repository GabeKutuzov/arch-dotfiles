/* (c) Copyright 1991-2012, Neurosciences Research Foundation, Inc. */
/* $Id: d3allo.c 44 2011-03-04 22:36:14Z  $ */
/***********************************************************************
*                             CNS Program                              *
*            d3allo - Allocate storage for repertoire data             *
*                                                                      *
*     For all celltypes in all repertoires:                            *
*        Set ctoff/xnoff/cnoff to alloc vars needed by rqst'd options. *
*        Determine dynamic offsets for all these variables.            *
*        Calculate lengths of cells, groups, repertoires, etc.         *
*        Estimate lengths of "deferred" decay, falloff, stats, etc.    *
*           for printing by d3echo (actual allocations in d3nset).     *
*                                                                      *
*     PREREQUISITES:  d3tchk, d3cchk, and d3mchk must be called first. *
*                                                                      *
*     To add a new dynamic variable to the system:                     *
*     1) Add a named constant in d3global.h to define the new variable.*
*           This constant gives the location in the gpoff (for group-  *
*           level variables), ctoff (for cell-level variables), xnoff  *
*           (for one-per-connection-type variables), or cnoff (for     *
*           connection-level variables) array where the offset of      *
*           the new variable will be stored, and the offset in the     *
*           gplength, ctlength, xnlength, or cnlength tables where the *
*           length of the new variable will be stored.  Assign the     *
*           next unused value in the appropriate section.  Increase    *
*           GPNDV, CTNDV, or CNNDV if needed to the next even number   *
*           that is greater than the highest variable number.          *
*     2) Add the code number of the new variable at an appropriate     *
*           place in the 'order' lists in allo_tbl.h.  The order lists *
*           determine the order in which the various items will be     *
*           assigned within the data for each group, cell, or synapse. *
*           There are separate order lists for group-level, cell-      *
*           level, conntype-level, and connection-level variables, and *
*           within each list, ordering should be by decreasing length. *
*     3) Put the length of the new variable in the gplength, ctlength, *
*           xnlength, or cnlength entry in allo_tbl.h corresponding to *
*           its code number. If a special length calculation is req'd, *
*           assign a new negative code number for this length calcula- *
*           tion, expand the cvlength array to hold the new length,    *
*           add code to d3allo and d3resetn to calculate this length.  *
*     4) Add code to d3allo to set the gpoff, ctoff, xnoff, or cnoff   *
*           element to DVREQ when options exist that need the new      *
*           variable.                                                  *
*     5) The variable will now be allocated automatically and its      *
*           offset will be appear in the gpoff, ctoff, xnoff, or cnoff *
*           entry corresponding to its code number.  Write the code    *
*           that uses the new variable, e.g. in d3go(), using the off- *
*           set table entry to locate the variable in the prd (cell)   *
*           or psyn0 (connection) data.  Add statistics as needed.     *
*     6) Add code to d3reset[zn] to reset the variable when needed.    *
*     7) Add code to d3save/d3rstr to save the variable to SAVENET     *
*           files and restore it when needed.                          *
*     8) If needed, add a code to the DETAIL PRINT card to print the   *
*           new item and add code to d3dprt/d3pdpz/d3pdpn to perform   *
*           the printing when requested.                               *
*     9) If needed, add code to the SAVE, REGION, CELLTYPE, and CONN-  *
*           TYPE card SVITMS options to request saving the new item in *
*           SIMDATA files, add the item to the variable descriptors in *
*           d3gfhd, add code to d3gfsh/d3gfsv to turn off the request  *
*           bit if the item does not exist, and then to put the item   *
*           in the file header and in the data file.                   *
************************************************************************
*  REVISION HISTORY                                                    *
*  Written by G. N. Reeke, 10/17/85                                    *
*  V4A, 09/08/88, Translated to C from version V3F                     *
*  Rev, 02/06/90, JMS - Correct ps2 array size in RP->lsall calc       *
*  Rev, 02/23/90, GNR - Change allocation for PAR gconns               *
*  Rev, 02/28/90, GNR - Remove node0/noden ifs--used only on node 0    *
*  Rev, 05/15/90, JWT - Remove Wallace, features/relations             *
*  Rev, 01/28/91,  MC - Double RP->lsall for phased celltypes          *
*  V5C, 11/18/91, GNR - Add D1, move most input checking to d3tchk     *
*  V5E, 07/12/92, GNR - Revise to reenable GCONNs in serial version    *
*  Rev, 07/26/92, GNR - Add Cij0, go to named offsets                  *
*  V6A, 03/26/93, GNR - Add KAMDQ, KAMDR                               *
*  V6C, 08/14/93, GNR - Remove 2**n dependency                         *
*  V6D, 01/31/94, GNR - Add DELAY, remove boxsum alloc for PAR         *
*  V7A, 04/24/94, GNR - Estimate lrall, ld1xxx memory for d3echo       *
*  V7B, 06/22/94, GNR - Set calc_lij, eliminate msk0, set nuklen       *
*  V7C, 08/25/94, GNR - Calc. space for GCONN persistence, move allocs *
*                       for stats here from d3tree, add CFstats        *
*  V8A, 05/06/95, GNR - Add MODALITY and stat cntrl at celltype level, *
*                       omit MOD/GCONN stat alloc if nostats, allocate *
*                       RGH for regen, OPTIMIZE DEFER-ALLOC, cumstat   *
*  Rev, 03/01/97, GNR - Allocate pps on host same as all other nodes   *
*  Rev, 04/26/97, GNR - Set to always condense connection data         *
*  Rev, 08/27/97, GNR - Add AK, PHD variables for GRAFDATA output      *
*                       (with fullword alignment), delete obsolete re- *
*                       play permutation table setup                   *
*  Rev, 09/28/97, GNR - Independent dynamic allocations per conntype   *
*  Rev, 12/27/97, GNR - Add ADCY, defer most statistics to d3nset      *
*  Rev, 10/28/00, GNR - Add allocation for history statistics          *
*  V8C, 02/27/03, GNR - Cell responses in millivolts, add conductances *
*  V8D, 08/20/06, GNR - Always store NUK at start of connection data   *
*  ==>, 12/29/07, GNR - Last mod before committing to svn repository   *
*  Rev, 04/23/08, GNR - Add ssck (sum sign check) mechanism            *
*  V8E, 01/15/09, GNR - Add allocations for Izhikevich variables       *
*  Rev, 02/10/09, GNR - Individual memory optimization per celltype    *
*  Rev, 09/03/09, GNR - Add Brette-Gerstner (aEIF) neurons             *
*  Rev, 05/15/10, GNR - Add KRPGP statistics, KGNE2                    *
*  V8H, 10/20/10, GNR - Add xn (once per conntype) variable allocation *
*  Rev, 04/02/12, GNR - Use defined constants in ldas calc             *
***********************************************************************/

#define D1TYPE  struct D1BLK

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "d3global.h"
#include "allo_tbl.h"
#ifdef D1
#include "d1def.h"
#endif
#include "rocks.h"
#include "rkarith.h"
#include "bapkg.h"

static int allocate(char *ord, short *offset, short *length,
   struct CELLTYPE *il, int *mxalign);

void d3allo(void) {

   struct REPBLOCK *ir;
   struct CELLTYPE *il;
   struct CONNTYPE *ix;
   struct INHIBBLK *ib;
   struct MODBY    *im;
   struct MODALITY *pmdlt;
#ifdef D1
   struct D1BLK    *pd1;
#endif

   long maxcpn;            /* Max cells on any one node,
                           *  = il->nelt if serial */
   long cumcnt;            /* Total connection,cell,group length */
   long sumlct;            /* Sum of lct for a cell type */
   int  dosvcij0;          /* Need to save all CIJ0 */
   int  kdecay;            /* =1 if conntype has decay, else 0 */
   int  kspeed;            /* Optimize speed or regenerating */
   int  mxalign;           /* Cumulative largest alignment mask */
   int  ndists;            /* Number of stat counts allocated */
   int  nddsts;            /* Number of stat ddists allocated */
#ifdef PAR
   ldiv_t qrm;             /* Modulo work area */
   int  nodes_used = NC.total - NC.headnode; /* Nodes used */
#endif
   int  sumnds;            /* Total nds of modalities sensed */
   int  talmsk;            /* Temp alignment mask */
   int  tnib;              /* Total number of inhibitory bands */
   enum SiColors esipc;    /* Local copy of ksipce & SIC_MASK */

/*---------------------------------------------------------------------*
*                       d3allo executable code                         *
*---------------------------------------------------------------------*/

#ifdef D1
/* Count up total space needed for Darwin 1 repertoires and scores.
*  Initialize cpn,crn,cut variables used elsewhere for allocation. */

   for (pd1=RP->pd1b1; pd1; pd1=pd1->pnd1b) {
      long nd1e = pd1->nd1r*pd1->nepr; /* Elements this block */
      long neal = ALIGN_UP(nd1e);      /* Number elements aligned */
#ifdef PAR
      /* Cells per node, remainder, and cutoff values */
      qrm = ldiv(nd1e, nodes_used);
      pd1->cpn = qrm.quot;
      pd1->crn = qrm.rem;
      pd1->cut = (pd1->cpn + 1)*pd1->crn;
      /* Set nd1e to max usage on any one node for ld1rall calc */
      nd1e = pd1->cpn + (pd1->crn > 0);
#endif
      /* Total length of repertoire data unique on each node */
      RP0->ld1rall += ALIGN_UP(nd1e*pd1->nbyt);
      /* Total length of D1 s(i) data replicated on all nodes */
      RP0->ld1sall += neal;
      /* Keep track of the largest si array for ps0 allocation */
      RP0->xtrasize = max(RP0->xtrasize,neal);
      } /* End loop over D1 blocks */
#endif

/* Set dosvcij0 flag to force all connection types to save CIJ0 if
*  RESPONSE FUNCTION SAVECIJ0 or any reset action KRACIJ is set.
*  Note:  There is currently no way KRACIJ can be set before Group
*  III, but we test in case that changes in future versions.  */

   dosvcij0 = (RP0->n0flags & N0F_SVCIJ0) |
      ((RP->kra[RLtent].kract | RP->kra[RLtmr].kract |
        RP->kra[RLevnt].kract | RP->kra[RLval].kract |
        RP->kra[RLser].kract  | RP->kra[RLnow].kract) & KRACIJ);

/* Start cumulative storage count for conductance table data */

   RP0->cumcnd = d3ctbl(0);

/* Loop over repertoires */

   /* Everybody needs these */
   RP0->lrall = RP->lAffData + RP->lAffD64;

   for (ir=RP->tree; ir; ir=ir->pnrp) {

/* Set alloc requests and lengths for group-level options
*     (currently the only one is IONG). */

      if (ir->niong) {
         cvlength[LIONG] = ESIZE * ir->niong;
         gprqst(ir,IONG); }

/* Loop over cell layers:
*     Prepare constants needed to allocate cells to nodes.
*     Estimate current space for decay, falloff, and statistics.
*     Set alloc requests for cell-level options.  */

      for (il=ir->play1; il; il=il->play) {

#ifdef PAR
/* Prepare constants needed to allocate cells to nodes.  In a future
*  version of CNS, a load optimizer function will be used to determine
*  which repertoires can be evaluated in parallel.  The variables il->
*  node1 and il->nodes will then be set accordingly and all further
*  allocation will follow based on those variables.  At present, all
*  cells are distributed over all nodes.  */

         il->node1 = NC.headnode;
         il->nodes = nodes_used;
         qrm = ldiv(il->nelt, il->nodes);
         il->cpn = qrm.quot;
         il->cut = (il->cpn + 1)*il->crn;
         /* Calculate max cells on any one node */
         maxcpn = (il->nelt+il->nodes-1)/il->nodes;
#else
         maxcpn = il->nelt;
#endif

/* Flag if optimize speed or regeneration is requested */

         kspeed = (il->Ct.ctopt & (OPTMEM|OPTRG)) != OPTMEM;

/* Request allocation of variables for Brette-Gerstner neurons.
*  The same IZVU variables will be used as for Izhikevich.  */

         if (il->Ct.rspmeth == RF_BREG) {
            ctrqst(il,IZVU);
            il->ctf |= CTFALIGN;
            }

/* Request allocation of variables for Izhikevich neurons.
*  Note:  Even if regenerating, we can honor OPTIMIZE STORAGE
*     for individual a,b,c,d,i3vr not for u.  */

         if (il->Ct.rspmeth >= RF_IZH3) {
            struct IZHICOM *piz = (struct IZHICOM *)il->prfi;
            il->nizvab = (piz->izra != 0) + (piz->izrb != 0);
            il->nizvcd = (piz->izrc != 0) + (piz->izrd != 0);
            il->nizvar = il->nizvab + il->nizvcd;
            cvlength[LIZVU] = 2*I32SIZE;
            if (piz->izrb != 0 && il->Ct.rspmeth == RF_IZH3) {
               if (il->Ct.ctopt & OPTMEM)
                  il->ctf |= CTFI3IVC;
               else {
                  cvlength[LIZVU] += I32SIZE;
                  il->ctf |= CTFI3IVR; }
               }
            ctrqst(il,IZVU);
            il->ctf |= CTFALIGN;
            if (il->nizvar) {
               if (il->Ct.ctopt & OPTMEM)
                  il->ctf |= CTFRANOS;
               else {
                  cvlength[LIZR] = SHSIZE * il->nizvar;
                  ctrqst(il,IZRA);
                  }
               }
            }

/* Calculate number of bytes needed to store per-cell ion data */

         if (il->nionc) {
            cvlength[LIONC] = ESIZE * il->nionc;
            ctrqst(il,IONC);
            }

/* Calculate number of bytes/cell needed to store conductance data.
*  If refractory, add 2 bytes for time since last hit.  If gDCY and
*  gated, add 2 bytes each for number hits and time since last hit.
*  Store estimated space for decay tables and conductance history
*  for d3echo()--updated per node in d3nset() (depends on maxcpn,
*  which is not available at d3cchk() time).  */

         if (il->ncnd) {         /* Any kind of conductance here */
            struct CONDUCT *pcnd;
            ui16 trdo = 0;       /* Temp for prd data offset */

            /* Calculate max and total cycles to decay, assign data
            *  offsets and calc length of CNDG data.  */
            for (pcnd=il->pcnd1; pcnd; pcnd=pcnd->pncd) {
               pcnd->ogrd = trdo;
               if (pcnd->gDCY.omega || pcnd->refrac) {
                  trdo += LgHNT;
                  pcnd->cdflgs |= CDFL_HITNT; }
               else
                  trdo += Lg;
               }

            ctrqst(il,CNDG);     /* Allocate CNDG variable */
            cvlength[LCNDG] = il->lcndg = trdo;

            } /* End conductance setup */

/* Calculate number of bytes needed to store cross-reaction data */

         sumnds = 0;
         if (il->ctf & CTFXD) {
            unsigned char *prmdlt = (unsigned char *)&il->rmdlts;
            for (pmdlt=RP->pmdlt1; pmdlt; pmdlt=pmdlt->pnmdlt) {
               if (bittst(prmdlt,(long)pmdlt->mdltno))
                  sumnds += pmdlt->nds;
               }
            cvlength[LXRM] = il->lmm = BytesInXBits(sumnds);
            if (il->lmm > RP->mxlmm) RP->mxlmm = il->lmm;
            }

/* Set allocation requests for conntype-level options.  Note
*  that for convenience, ndists and nddsts are counted without
*  regard to whether or not statistics are globally enabled,
*  but the total is only passed to RP0->ldstat if enabled.  */

         ndists = nddsts = 0;
         for (ix=il->pct1; ix; ix=ix->pct) {
            long mx;          /* Temp for length calcs */
            ndists += 1;      /* Count c distribution stats */
            nddsts += 1;      /* Count v distribution stats */
            if (ix->kgen & KGNLM) { /* L option specified */
               /* Insert default ucij.l2.flags for cells that drive
               *  effectors.  This code revised 04/29/91 for general
               *  effectors.  It previously inserted default flag
               *  values only if a search revealed that the cells
               *  drove a universal joint or a window.  This action
               *  is now controlled by d3schk, which turns on il->ctf
               *  CTFAE or CTFAI flags as appropriate for the various
               *  kinds of changes which it knows about.  The code is
               *  placed here to avoid an extra ix loop in d3tchk().
               */
               if (ix->ucij.l2.flags == 0) {
                  if (il->ctf & CTFAE) ix->ucij.l2.flags |=
                     (LOPNC|LOPPH|LOPRL|LOPSC);
                  if (il->ctf & CTFAI) ix->ucij.l2.flags |=
                     (LOPNC|LOPPH|LOPRL|LOPMI|LOPSC);
                  }
               } /* End setup for KGEN=L */
            else if (ix->kgen & KGNMC && il->Ct.kctp & KRPFD)
               ndists += ix->ucij.m.nkt;

/* Deal with variables that occur once per cell per conntype.
*  NUK is always saved, AK if requested for SIMDATA, IFD if
*  input from a CONNLIST file that contains feature detector
*  information for statistics.  */

            xnrqst(ix,NUK);
            if (ix->cnflgs & NSVAK) xnrqst(ix,AK);

/* Determine nuklen = 1+log2(nc)/8 = length needed to save number
*  of used connections (NUK) in xnoff data and skipped connection
*  information at end of Cij field. */

            ix->nuklen = 0; mx = ix->nc;
            do ix->nuklen++; while (mx>>=8); /*Assignment intended*/

/* Determine and store length of Lij = 1 + log2(srcnelt)/8.
*  Create lijlim as test value for skipped connections.
*  (Do this even if Lij not stored, as lijlim may be used
*     for end testing in detail print, Cij plots, etc.)  */

            mx = ix->srcnelt;
            if (ix->kgfl & KGNIA) mx <<= 1;
            ix->lijlen = 0;
            do ix->lijlen++; while (mx>>=8); /* Assignment intended */

            /* This ugly goo does: shift all-FF's right by the
            * diff of max(lijlen) & lijlen, multiplying by 8
            * to get a shift by the right number of bytes - jms */
            ix->lijlim = ((unsigned long)~0L) >>
                         ((sizeof(long)-ix->lijlen) << 3);

/* Determine conditions for Lij generation.  Lij are not generated at
*  all if input is from hand vision (KGNHV).  Lij are generated when
*  needed and not stored in memory when OPTIMIZE STORAGE is on.  This
*  is indicated by setting ix->Cn.cnflgs(CLIJ) TRUE.  However, storage
*  of Lij in memory is forced with Lij input from a file (KGEN=E) or
*  when there is regeneration (OPT=R).  Lij are always calculated
*  in d3genr, whether or not stored in memory.  They are used to
*  condense skipped connections out of the remaining connection
*  variables (Cij, Cij0, etc.), to construct motor map (KGNLM)
*  connection strengths, and to build a list of skipped connections
*  for sub-arbor bookkeeping in d3go().  (This list is kept in the
*  CIJ field to avoid need for calls to Lij generation just to get
*  the skips when Lij are not stored.  The skip list may expand the
*  width of the Cij field.  Because of the two-pass amplification
*  scheme expected to be introduced someday, the prd list must be
*  condensed even when Lij are not stored.)  */

            if (!(ix->kgen & KGNHV)) {
               if (kspeed | ix->kgen & KGNEX)
                  cnrqst(ix,LIJ);
               else
                  ix->cnflgs |= CLIJ;
               } /* End if not KGNHV */

/* Determine conditions for Cij generation.  Always store Cij unless
*  OPTIMIZE STORAGE and KAMMF (matrix freeze) options are on and
*  regeneration is off.  In KAMMF case, Mij and Cij0 are irrelevant
*  and omitted and d3news() kills amplification.  (Skip list is always
*  saved if Cij is saved, so the actual lc allocation is the maximum
*  of lc,nuklen.)  */

            if (!(ix->Cn.kam & KAMMF)) {
               cnrqst(ix,CIJ);
               /* Request Mij if slow amplification */
               if (ix->Cn.kam & (KAMSA|KAMTS|KAMUE)) cnrqst(ix,MIJ);
               /* Request Cij0 if DECAY KIND=B or RESPONSE FUNCTION
               *  SAVECIJ0 or any reset level requests KRACIJ.  */
               if (dosvcij0 || ix->Cn.kdecay & KINDB) cnrqst(ix,CIJ0);
               }
            if (kspeed)
               cnrqst(ix,CIJ);
            if (!cnexists(ix,CIJ)) {
               if ((ix->kgen & (KGNLM|KGNMC) | ix->Cn.saopt & SAOPTA))
                  ix->cnflgs |= NEEDLIJ;
               }

/* Determine conditions for Dij generation.  If connection delay is
*  requested, it is calculated each time needed if OPTIMIZE STORAGE,
*  otherwise calculated once in d3genr() or d3regenr() and stored in
*  repertoire (prd) data.  */

            if (ix->Cn.kdelay > DLY_CONST && kspeed)
               cnrqst(ix,DIJ);

/* Request allocation for paired-pulse facilitation */

            if (ix->PP) { cnrqst(ix,PPF); cnrqst(ix,PPFT); }

/* Request allocation for mean Sj KAMDR */

            if (ix->Cn.kam & KAMDR || ix->Cn.cnopt & NOPSJR) {
               cnrqst(ix,RBAR);
               nddsts += 1; }

/* Estimate space needed for Ak decay.  This space will be allocated
*  by d3nset() according to the exact amounts needed after any Group
*  III changes.  Allow one long each for excitatory and inhibitory
*  sum per cell as determined by d3lafs().  (AK could be merged with
*  this space if decayed Ak were acceptable in SIMDATA files...)  */

            kdecay = (ix->Cn.ADCY.omega != 0);
            if (kdecay) {
               RP0->ldecay += LSIZE * maxcpn * npsps((int)ix->cssck);
               nddsts += 1; }

/* Compute space needed for detailed amp stats
*  if stats enabled (CTPNS is set if RP_NOSTAT).  */

            if (!(il->Ct.kctp & CTPNS) && (ix->Cn.kam & KAMDS)) {
               ix->ldas = sizeof(*(ix->pdas))*LDR*(NdasRows +
                  ((ix->Cn.kam & (KAMVM|KAMCG|KAMUE)) != 0) +
                  ((ix->Cn.kam & (KAMSA|KAMTS|KAMUE)) != 0));
               RP0->ldstat += ix->ldas;
               }

            }  /* End CONNTYPE loop */

/* Calculate space needed for evaluating GCONNs.
*  RP->lishare accumulates total for space that can be shared by
*     all INHIBBLKs in the model (serial version only).
*  RP->lifixed accumulates total for INHIBBLK space that must be
*     in a fixed location but is not shared (serial only).
*  RP0->liall accumulates total for all space used by INHIBBLKs.
*  (Note: ngx,ngy are assumed same for source and target reps--
*     to relax this condition, whole scheme needs revision.)  */

         il->mxnr = 0;        /* max(number of rings - 1) */
         tnib = 0;            /* Total number of inhib bands */
         for (ib=il->pib1; ib; ib=ib->pib) {
#ifndef PAR
            long mxngxy = max(ib->l1x,ib->l1y); /* max(ngx,ngy) */
#endif
            /* max(rings-1) */
            il->mxnr = max(il->mxnr,(ui16)ib->l1n1);
            tnib += ib->nib;

/* Estimate space needed for GCONN decay.  Decaying GCONNs persist
*  from cycle to cycle and therefore space cannot be shared with other
*  INHIBBLKs.  Decay requires one long for positive betas, one long
*  for negative betas per group (per cell if self-avoidance bit is
*  set).  This space will be allocated by d3nset() according to the
*  exact amounts needed after any Group III changes.  */

            kdecay = (ib->GDCY.omega != 0);
            nddsts += ib->nib + kdecay;
            if (kdecay) RP0->ldecay += LSIZE *
               NDecayCells(il, ib, maxcpn) * npsps((int)ib->gssck);

/* Calculate space for falloff tables */

            RP0->liall += lfofftbl(ib, TRUE);

#ifndef PAR
/* Calculate space for norm tables */

            if (ib->kbc == BC_NORM)
               RP->lifixed += mxngxy*sizeof(struct XYNORMDEF);
#endif

            }  /* End INHIBBLK loop */

#ifndef PAR
/* End of inhibblk scan for this cell type.  Now, if serial, allocate
*  for boxsum, vstrip, hstrip, and bandsums.  This storage is shared
*  by all INHIBBLKs in the run.  See note above about assumption that
*  source ngx,ngy == target ngx,ngy.  ir->ngx,ngy used here because
*  this code must be generic to all INHIBBLK's for current il.  */

         if (il->pib1) {
            long ibtmp = sizeof(long)*(((tnib+3)*ir->ngx*ir->ngy) +
               (((ir->ngx+ir->ngy+il->mxnr)*il->mxnr)<<2));
            RP->lishare = max(RP->lishare,ibtmp);
            }
#endif

/* Calculate space needed for modulation and modulation statistics */

         for (im=il->pmby1; im; im=im->pmby) {
            nddsts += 1 + (im->MDCY.omega != 0);
            } /* End MODBY loop */

/* Count cell-level distribution arrays and request XRM variables
*  as required for X,Y,Z,C,H,G statistics (turned off in g2cell if
*  RP_NOSTAT).  (Note:  One C dist and one GPSTAT is allocated per
*  distinct stimulus. This allows d3go to index C,G stats using isn
*  and avoids having to construct global tables mapping isn to ign
*  for all GRPNO cards.  Such tables are currently not constructed
*  until d3stat() time.  nCdist must be stored whether or not stats
*  are turned on to allow for turn-on at Group III time.  */

         il->nCdist = (ui16)sumnds;
         if (il->Ct.kctp & (KRPXR|KRPYM|KRPZC|KRPHR))
            ctrqst(il,XRM);   /* Request xrm */

/* Calculate space needed for distribution statistics */

         if (!(il->Ct.kctp & CTPNS)) {
            if (il->Ct.kctp & KRPCL)         ndists += sumnds;
            if (il->ctf  & CTFDN)            nddsts += 1;
            if (il->phshft)                  ndists += 2;
            if (il->Ct.rspmeth >= RF_BREG)   nddsts += 1;
            else if (il->Dc.CDCY.omega)      nddsts += 1;
            if (il->prfrc)                   nddsts += 1;
            RP0->ldstat += sizeof(struct CLSTAT) +
               sizeof(dist)*ndists + sizeof(ddist)*nddsts;
            if (il->Ct.kctp & KRPHR)
               RP0->ldstat += sizeof(hist) * il->nmdlts;
            if (il->Ct.kctp & (KRPGP|KRPMG))
               RP0->ldstat += sizeof(struct GPSTAT) * sumnds;
            }

/* Set allocation bits for remaining cell-level options, including
*  any that may depend on the three connection class codes above.
*  N.B.  If on some machine, float alignment is different from
*  long alignment, alignment code must change.  */

#if ALIGN_TYPE > 0 && ESIZE != LSIZE
#error d3allo:  Code for cell data alignment not valid on this system
#endif
         esipc = (enum SiColors)(il->Ct.ksipce & SIC_MASK);
         if (il->orkam & (KAMSA|KAMZS|KAMTNU) || esipc == SIC_SBar ||
             il->ctwk.tree.orkdecay & KINDI)  ctrqst(il,SBAR);
         if (il->orkam & KAMDQ || esipc == SIC_QBar)
                                    ctrqst(il,QBAR);
         if (il->prfrc)             ctrqst(il,DST);
         if (il->ctf & CTFDN)       ctrqst(il,DEPR);
         if (il->Ct.ctopt & OPTRG)  ctrqst(il,RGH);
         if (il->ctf & CTFPD) /* CTFPD never set if no phasing */
            { il->ctf |= CTFALIGN;  ctrqst(il,PHD); }

/* Loop over all connection types
*     Allocate the requested variables.  If Cij is
*        stored, be sure lc is large enough to hold skip list.
*     Store the length of the synapse data for each type.
*     Accumulate the total length in sumlct to set cell length.
*
*     Alignment is not forced for individual synapse data, but
*        the first synapse of each cell is aligned to a multiple
*        of the largest variable in a synapse to maximize
*        chances for gratuitous alignment.
*/
         sumlct = 0;
         mxalign = 0;
         for (ix=il->pct1; ix; ix=ix->pct) {

            cvlength[LCIJ0] = ix->cijlen;
            cvlength[LLIJ]  = ix->lijlen;
            cvlength[LCIJ]  = ix->cijlen;
            cvlength[LNUK]  = ix->nuklen;
            ix->lc = allocate(ordcn, ix->cnoff,
               cnlength, NULL, &mxalign);
            if (cnexists(ix, CIJ)) {   /* Storing Cij in rd_data */
               /* Make sure skip list will fit in permanent data */
               if (ix->nuklen > ix->lc) ix->lc = ix->nuklen;
               }
            ix->lxn = allocate(ordxn, ix->xnoff,
               xnlength, NULL, &mxalign);
            /* Align so all connections start on same boundary */
            ix->lct = (ix->lc * ix->nc + ix->lxn + mxalign) & ~mxalign;
            sumlct += ix->lct;

            } /* End ix loop */

/* Allocate variables at cell (layer) level and pad length
*  to assure alignment of first connection  */

         talmsk = mxalign;    /* Save connection alignment mask */
         cumcnt = allocate(ordct,il->ctoff,ctlength,il,&mxalign);
         il->ls = cumcnt = (cumcnt+talmsk) & ~talmsk;

/* Complete calculation of length of all data for one cell and pad
*  to assure alignment of subsequent cells.  (Minimum alignment on
*  BYTE_ALIGN is traditional but unnecessary.)   Accumulate totals
*  for all cells in a layer and in a repertoire.  */

         cumcnt += sumlct;
         talmsk = BYTE_ALIGN - 1;
         if (talmsk > mxalign) mxalign = talmsk;
         cumcnt = (cumcnt+mxalign) & ~mxalign;
         il->lel = cumcnt;
         il->llt = il->lel * il->nelt;
         ir->lrt += il->llt;

/* Calculate memory needed for s(i) vectors and modality marker
*  arrays on various nodes.  Include space for phase if required.
*  Whole array is replicated for each needed unit time delay.
*  Calculation of RP0->lrall and RP0->lsall is temporary for use
*  in d3echo and reflects the largest amount of space any one node
*  will require.  Later, d3nset performs exact calculation on each
*  individual node.  */

         /* Calculate tl = space for one s(i) table for each delay
         *  needed, including pointers to them, plus one extra full
         *  table for holding new s(i) and isn markers.  */
         {  long tl = il->mxsdelay1*(sizeof(s_type *) +
               il->lspt + il->lmm);
#ifdef PAR
            RP0->lrall += maxcpn * il->lel;
            /* For comp nodes, lsall = length of ps arrays and ptrs
            *  to them, plus largest ps2 array needed on any one node.
            *  Also includes space for modality marker arrays if
            *  celltype has H,X,Y,Z, or C stats or feeds one that
            *  does.  All pointers are allocated from the bottom,
            *  so no additional alignment is required.  */
            RP0->lsall += tl + spsize(maxcpn,il->phshft) + il->lmm;
            /* PAR host does not need ps2 array */
            RP0->ls0all += tl;
#else
            RP0->lrall += il->llt;
            RP0->lsall += tl + il->lspt + il->lmm;
#endif
            } /* End tl local scope */

         } /* End il loop */

/* Allocate variables at group level and compute total length.
*  These will be exchanged across nodes after each cycle because
*  we don't want to force an integral number of groups per node.
*  Therefore, they are in a separate pool, not included in lrall.
*  Code elsewhere assumes IONG data are aligned on floating-point
*  word boundaries, so be sure that is true here... */

      cumcnt = allocate(ordgp,ir->gpoff,gplength,NULL,&mxalign);
      if (cumcnt) {
         /* Group data must be aligned */
         cumcnt = (cumcnt+mxalign) & ~mxalign;
         ir->lg = cumcnt;  /* lg = length of data for one group */
         ir->lgt = ir->ngrp * ir->lg;
         /* Total rep length for d3echo = group data + cell data */
         ir->lrt += ir->lgt;
         RP0->lgall += ir->lgt;
         } /* End length accounting for group data */

      } /* End of for loop over ir */

#ifndef PAR
   RP0->liall += RP->lishare + RP->lifixed;
#endif

   }  /* End of d3allo */

/***********************************************************************
*                                                                      *
*  allocate -- Routine to assign offsets to dynamic group, cell,       *
*              celltype, and synapse variables in a CNS repertoire.    *
*                                                                      *
*  Synopsis:                                                           *
*        static int allocate(char *ord, long *offset,                  *
*                 long *length, struct CELLTYPE *il, int *mxalign);    *
*                                                                      *
*  Arguments:                                                          *
*        ord      is 'order table' to be used.                         *
*        offset   is offset array to be initialized.                   *
*        length   is length table to be consulted.                     *
*        il       points to the CELLTYPE--used only when setting       *
*                 cell-level offsets to deal with the special case     *
*                 of late alignment of PHD.                            *
*        mxalign  is set to one less than largest boundary size on     *
*                 which any item allocated by this call should be      *
*                 aligned.  Note that variable-length items are not    *
*                 aligned, even if length happens to fit a word.       *
*                                                                      *
*  Return value:  cumulative length of all variables allocated by      *
*                 this call for one instance of the block being        *
*                 allocated (e.g. cell-level or conection-level data). *
***********************************************************************/

static int allocate(char *ord, short *offset, short *length,
      struct CELLTYPE *il, int *mxalign) {

   long cumcnt = 0;           /* Cumulative size of allocation */
   long ilen;                 /* Length of current allocation  */
   int iord;                  /* Copy of ord[i] used to reduce
                              *  the number of array references */
   int ils;                   /* Alignment size for item */

/* Table of alignment masks vs item size */
#define LARGEST_ALIGNMSK_INDEX  8
   static char alignmsk[LARGEST_ALIGNMSK_INDEX + 1] =
      { 0, 0, 1, 0, 3, 0, 1, 0, 7 };

/* Loop over variables listed in order table
*  and allocate those that are requested. */

   while ((iord=(*ord++)) != END_OF_ALLOC_TABLE) {

      if (iord == SAVE_LS_COUNT) {
         /* Special case to store ls count for restoring from a file.
         *  This count omits PHD, statistics, and padding.
         *  If one of these long items is included & machine requires
         *  fullword alignment, cumcnt is rounded up to a boundary.
         *  See d3go and d3gfsv for usage of these items.  */
         if (!il) d3exit(LSRSTR_ERR, NULL, 0);
         il->lsrstr = (short)cumcnt;
#if ALIGN_TYPE > 0
         if (il->ctf & CTFALIGN) cumcnt = ALIGN_UP(cumcnt);
#endif
         }

      else if (offset[iord] == DVREQ) {
         /* Item has been requested--allocate it */
         offset[iord] = (ui16)cumcnt;
         if ((ilen = length[iord]) < 0)   /* Length from table */
            cumcnt += cvlength[abs(ilen)];
         else {                           /* Direct length */
            cumcnt += ilen;
            /* Anything larger than a double (e.g. PHD) is probably
            *  an array of words--align accordingly.  */
            ils = (ilen > LARGEST_ALIGNMSK_INDEX) ? 3 : alignmsk[ilen];
            if (ils > *mxalign) *mxalign = ils;
            }
         } /* End else if item requested */
      } /* End while loop over order table */

   return cumcnt;
   } /* End allocate() */

