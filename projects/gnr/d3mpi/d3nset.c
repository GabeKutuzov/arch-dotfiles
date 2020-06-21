/* (c) Copyright 1990-2018, The Rockefeller University *21115* */
/* $Id: d3nset.c 77 2018-03-15 21:08:14Z  $ */
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
************************************************************************
*  Note for future work:  Here is the place to check whether memory    *
*  allocations under 32-bit compilations that are multiples of nel,    *
*  nelt etc. will exceed 32-bit even though nel,nelt etc. are within   *
*  the documented 31-bit limits.  These would include e.g. AUTSSL      *
*  lists and other Group III-variable storage.                         *
************************************************************************
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
*  Rev, 06/19/12, GNR - Remove pglway                                  *
*  Rev, 09/30/12, GNR - Use ndal to compute decay storage allocations  *
*  V8I, 11/26/12, GNR - New overflow count mechanism, povec, pdecay    *
*  Rev, 02/10/13, GNR - Permanent/changeable persistence storage       *
*  Rev, 05/11/13, GNR - Remove gconn falloff                           *
*  Rev, 05/16/13, GNR - Add alloc for AUTOSCALE OPT=H AUTSSL structs   *
*  Rev, 08/22/14, GNR - Si only EXP decay, remove DECAYDEF from DCYDFLT*
*  R65, 12/27/15, GNR - Merge serial into parallel gconn code          *
*  R66, 02/02/16, GNR - Allocate for GCONN round-surround tables, move *
*                       GCONN allocations from ltot to lindv.          *
*  R67, 04/27/16, GNR - Remove Darwin 1 support                        *
*  R67, 10/07/16, GNR - Add DETAIL PRINT=T to CONNDATA, add AVGCDATA   *
*  R72, 02/07/17, GNR - Add allocation for KGNKN kernels               *
*  R74, 06/08/17, GNR - Add psws space for raw imgs (if being reduced) *
*  R75, 09/22/17, GNR - Add cijfrac                                    *
*  R76, 10/13/17, GNR - Add space for volt-dep-modified Aij distribs   *
*  R77, 02/22/18, GNR - Add il->hicell, used in many places            *
*  R78, 03/28/18, GNR - Bug fix, alloc (nel/2+1) for cijfrac tables    *
*  R78, 05/12/18, GNR - Update allocs for KSTGP,KSTMG,KSTHR,[hl]dist   *
*                       Bug fix: pnxts increment when not first call   *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "rkarith.h"
#include "d3global.h"
#include "clblk.h"
#include "simdata.h"
#include "celldata.h"
#include "lijarg.h"
#include "rocks.h"
#include "tvdef.h"
#include "plotdefs.h"

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
*  work areas, change-ok decay persistence, autoscaling, and most      *
*  statistics is allocated using high-water-mark algorithms.  These    *
*  may run out of space late in a long simulation.  Accordingly, the   *
*  Group I OPTIMIZE card has options to set aside reserves for these   *
*  allocations and this space is grabbed in the initial call.          *
*     On a parallel computer, only pgpd, pps, psws, passl, pconnd,     *
*  povec, and stats are allocated on host node.                        *
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
   s_type   *pnxts2;       /* Ptr to next new-s(i) table (ps2) */
   char     *pnxti;        /* Ptr to next individual work space */
   si32     *pncijfr;      /* Ptr to next cijfrac table */
   char     *pnkern;       /* Ptr to next Lij gen kernel space */
   char     *pnnorm;       /* Ptr to next norm or rroff table */
#ifndef PAR0
   si64     *pnbands;      /* Ptr to GCONN bandsum space */
   rd_type  *pnxtr;        /* Ptr to next repertoire data */
   rd_type  *pnxtsyn0;     /* Ptr to rep data for next conntype */
   si32     *pnxtveff[NDCM];  /* Ptrs to two kinds of persistence */
#endif

   /* All these lengths are understood to be in bytes */
   ui64 lindv;             /* Total changeable allocation required */
   ui64 ltot  = jeul(0);   /* Total fixed allocation required */
   ui64 lgall = jeul(0);   /* Length of all group data on this node */
   ui64 lrall = jeul(0);   /* Length of all rep data on this node */
   ui64 lsall = jeul(0);   /* Length of all s(i) on this node */
   ui64 nslu  = jeul(0);   /* Length of full set of s(i) arrays */
   ui64 nslup = jeul(0);   /* Length of si lookup table pointers */
#ifndef PAR0
   size_t nslu2 = 0;       /* Length of update s(i) (ps2) arrays */
   size_t lavgcd;          /* Length of AVGCDATA */
   size_t lpsums;          /* Length of CDAT.praffs */
   size_t lveff[NDCM] = {0,0}; /* Lengths of 2 kinds of persistence */
#endif
   ulng lconnd;            /* Length of CONNDATA */
   ulng lsws,msws;         /* Length of shared work space */
   ulng lassl = 0;         /* Length of autoscale sort list area */
   size_t lcijfr = 0;      /* Length of all cijfrac tables */
#ifdef PAR
   ui32 lhlsi;             /* Length of all HILOSI blocks */
#endif
   ui32 lovec;             /* Length of povec area */
#ifndef PAR0
   ui32 npsums;            /* Number of si64s in CDAT.praffs */
   ui32 omarkh = 0;        /* Offset to next history marking array */
   ui32 omarks = 0;        /* Offset to next stim marking bit array */
#endif
#ifndef PARn
   ui32 ogstat = 0;        /* Offset to next GPSTAT block */
#endif
   int  dostat;            /* Local copy of cell type stats switch */
   int  kcall;             /* Keeps track of whether first time */
   int  odist;             /* Offset to next count stat */
   int  oddst;             /* Offset to next ddist stat */
#ifdef PARn
   int  relnode;           /* Node relative to first node used */
#endif

/* Start alloc counts with items already known.  lindv must be
*  aligned up every time anything is allocated that is not for
*  sure a multiple of sizeof(si64) mostly to make it unnecessary
*  to assign pieces from lindv data in decreasing order of size.
*  Repertoire data are aligned up at every cell for performance.  */

   kcall = myprd ? KMEM_GO : KMEM_GENR;
   /* AffD32 and AffD64 space needed on Node 0 for afference sums
   *  and dprnt messages */
   lindv = jeul(RP->lAffD64 + RP->lAffD32);
   /* Space for CONNDATA and AVGCDATA for amplification, etc.  This
   *  is not a lot of space, so we do not test precisely how much
   *  is needed (e.g. not needed on Node 0 if no DPRINT=Y or T).  */
   lconnd = RP->mxnce * ALIGN_UP(sizeof(struct CONNDATA));
   lindv = jaulo(lindv, lconnd);

   /* Items calculated here rather than d3allo so may change at
   *  Group III time (must be lindv space).  */
#ifdef PAR
   RP->nhlpt = 0;
#endif

   /* Find max shared work space for: */
   lsws = NCnCijStat*sizeof(long);           /* Cij distrib stats */
   if (MAX_MSG_LENGTH > lsws) lsws = MAX_MSG_LENGTH;
#ifndef PAR0
   npsums = 0;                               /* Conntype Aij sums */
   lavgcd = RP->mxsaa * ALIGN_UP(sizeof(struct AVGCDATA));
   lindv = jaulo(lindv, lavgcd);
   /* This is tricky.  d3allo() stores in il->liboxes the boxsum and
   *  [hv]strip space needed on the comp node with the biggest total,
   *  in RP->lishare the largest total across celltypes of liboxes +
   *  bandsums, and in RP0->li0bxs the liboxes values for Node 0, i.e.
   *  the space for the celltype with largest ngx*ngy that has GCONNs.
   *  This value is added to lindv only if edge plots are requested,
   *  but d3allo() always computes it because plotting is not yet set
   *  when d3allo() runs.  Tables for normalization and round surround
   *  are now movable.  RP->lixxxxx are aligned up in d3allo().  */
   lindv = jauw(lindv, RP->lishare);
   lindv = jaulo(lindv, RP->liindiv);
   msws = lrsettbl();                        /* Reset tables */
   if (msws > lsws) lsws = msws;
#else
   if (RP->kpl & KPLEDG) {
      lindv = jauw(lindv, RP0->li0bxs);
      lindv = jaulo(lindv, RP->liindiv);
      }
#endif
#ifndef PARn
   lsws = max(lsws,2*LNSIZE+4);              /* Det print, d3gfsh */
   msws = lcmvsums();                        /* IJplot avg sums */
   if (msws > lsws) lsws = msws;
   /* Space for raw image space averaging, pixel size reduction,
   *  or 2-D image interpolation.  We don't know until an image
   *  is read whether or not TV_IFIT method will be used, so make
   *  space for it every time.  */
   {  struct TVDEF *cam;
      for (cam=RP->pftv; cam; cam=cam->pnxtv) {
         struct UTVDEF *putv = &cam->utv;
         if (!(putv->tvsflgs & TV_ON)) continue;
         msws = cam->tviflgs & (TV_Reduc|TV_SpAvg) ?
            putv->ltvrxy : 0;
         if (putv->tvuflgs & TV_IFIT) {
            ulng mfit = (ulng)putv->tvopxsz *
               (ulng)putv->tviy * (ulng)putv->tvx;
            if (mfit > msws) msws = mfit;
            }
         if (cam->utv.tvsflgs & TV_UPGM) msws +=
            (ulng)cam->utv.tvrlen;
         if (msws > lsws) lsws = msws;
         } /* End camera loop */
      } /* End cam local scope */
   /* Space for tvplot to fill empty plots, make colored preproc
   *  outputs, make gutters.  */
   {  struct PREPROC *pip;
      for (pip=RP->pfip; pip; pip=pip->pnip) {
         ulng opxsz;          /* Plot pixel size */
         if (!(pip->upr.ipuflgs & PP_PLOT)) continue;
         opxsz = (pip->ipsflgs & (PP_UPGM|PP_NEGS) ||
            qColored(pip->upr.ipkcol)) ? HSIZE*NColorDims : HSIZE;
         if (opxsz > HSIZE || pip->upr.nker % pip->dcols != 0) {
            msws = opxsz*(ulng)pip->upr.lppi3;
            if (msws > lsws) lsws = msws; }
         if (pip->dcoff > 0) {
            ulng nsep = (ulng)pip->dcoff;
            ulng lrow = (ulng)pip->dcols *
               ((ulng)pip->upr.nppx + nsep) - nsep;
            ulng lcol = (ulng)pip->kpc *
               ((ulng)pip->upr.nppy + nsep) - nsep;
            msws = opxsz * max(lrow,lcol);
            if (msws > lsws) lsws = msws; }
         }
      }
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
      ui32 irngx = (ui32)ir->ngx;

      lgall = jaulo(lgall, ir->lgt);

      for (il=ir->play1; il; il=il->play) {

#ifdef PAR0
         /* Parallel host node--handles no cells, so there is
         *  no contribution to lrall.  However, total length of
         *  repertoire data set by d3allo (il->llt) must be
         *  left untouched for use by d3save and d3rstr.  */
         il->locell = il->mcells = 0;
         il->hicell = il->nelt;
         il->mspt = 0;
#endif
#ifdef PARn
         /* Parallel node n--allocate my cells.  N.B.  Because group
         *  sizes can range from very small to very large, we do not
         *  force locell on each node to be first cell in some group.
         *  This has consequences for group max,avg plots in d3lplt. */
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
         il->hicell = il->locell + il->mcells;
         il->mspt = spsize(il->mcells, il->phshft);
         il->llt = jmuwj(il->lel, il->mcells);
         lrall = jauw(lrall, il->llt);
         nslu2 += il->mspt;
#endif
#ifndef PAR
         /* Serial--all cells on host.  Length of repertoire
         *  data (il->llt) has been set correctly by d3allo.  */
         il->locell = 0;
         il->mcells = il->hicell = il->nelt;
         lrall = jauw(lrall, il->llt);
         nslu2 += (il->mspt = il->lspt);
#endif
         il->logrp = (ui32)(il->locell/il->nel);
         il->higrp = (ui32)((il->hicell + il->nel - 1)/il->nel);
         il->logrpy = (ui16)(il->logrp/irngx);
         il->higrpy = (ui16)((il->higrp + irngx - 1)/irngx);
         /* Accumulate space needed for s(i)'s for this layer */
         nslu  = jauw(nslu, jmuwj(il->wspt, il->mxsdelay1));
         nslup = jaul(nslup, il->mxsdelay1*sizeof(s_type *));
#ifndef PAR0
         /* Set KRPNZ if rep has at least one cell on this node */
         if (il->mcells > 0) ir->Rp.krp |= KRPNZ;
#endif

/* Items not related to memory allocation but repeated from d3news()
*  because needed in d3genr() or d3asck() before d3news() is called. */

         il->ctf &= ~CTFDR;
         if (il->Ct.rspmeth == RF_STEP || il->Ct.ctopt & OPTDR)
            il->ctf |= CTFDR;
         il->No.noikadd = ((il->No.nmn | il->No.nsg) && il->No.nfr) ?
            il->No.noiflgs : 0;

/* Accumulate space needed for items (currently decay, statistics,
*  delta Cij's, and normalization) whose length is allowed to change
*  at Group III time.  */

         odist = oddst = 0;

#ifndef PAR0
         /* Space for marking which stimuli are seen--may be needed
         *  even if this CELLTYPE has stats off, to feed CELLTYPEs
         *  that do have stats.  */
         if (il->ctf & CTFXD && !(RP->CP.runflags & RP_NOSTAT)) {
            il->iMarks = omarks;
            omarks += il->lmm;
            } /* End marking space allocation */
#endif

         /* CELLTYPE statistics.  Optional stats that use fixed storage
         *  (XRM) are not allowed to be turned on at d3chng time if
         *  initially off.  No need to test RP_NOSTAT here, as KSTNS is
         *  always set in this case.  */
         dostat = !(il->Ct.kstat & KSTNS);
         if (dostat) {
#ifdef PAR
            /* These are for the overall stats, one per celltype */
            il->iHLsi = RP->nhlpt, RP->nhlpt += 1;
#endif
            if (il->Ct.kstat & KSTCL)
               il->oCdist = odist, odist+=il->nCdist;
            if (il->Ct.kstat & (KSTGP|KSTMG)) {
#ifndef PARn
               il->oGstat = ogstat; ogstat += il->nCdist;
#endif
#ifdef PAR
               RP->nhlpt += il->nCdist;
#endif
               }
#ifndef PAR0
            if (il->Ct.kstat & KSTHR) {
               il->iMarkh = omarkh;
               omarkh += il->lmm;
               }
#endif
            if (il->phshft)   il->oFdist = odist, odist+=2;
            ++oddst;       /* udist for IZHI|BREG, else Vm dist */
            /* If there is integration, must always allocate for
            *  pers dist, as nct and therefore omegau may change
            *  on a new trial series after this d3nset call.  */
            if (RP->kinteg & KIEUL ||
               il->Dc.omega || il->Dc.epsilon)       ++oddst;
            if (il->ctf & CTFDN)                      ++oddst;
            if (il->prfrc)                            ++oddst;
            }

         /* Autoscale sorted response lists */
         {  struct AUTSCL *paut = il->pauts;
            if (paut && !(paut->kaut & KAUT_NU)) {
               ulng autsz = paut->kaut & (KAUT_HPN|KAUT_WTA) ?
                  sizeof(struct AUTSSL) : sizeof(struct AUTSSLs);
               ulng sassl;
#ifdef PARn
               /* Work area for mpgather */
               msws = autsz * il->nelt;
               if (msws > lsws) lsws = msws;
               sassl = autsz * il->mcells;
#else
               /* Allow one extra block to absorb overflow at end
               *  due to added pointer at beginning.  */
               sassl = autsz * (il->nelt+1);
#endif
               if (sassl > lassl) lassl = sassl;
               }
            }

         /* SIMDATA file data mpgather work space.
         *  Note:  First time here, d3gfrl has not been called yet,
         *  suitms GFSI bit is still off and this code is skipped.  */
#ifdef PAR
         if (!(RP->ksv & (KSDNE|KSDNOK|KSDOFF)) && il->suitms & GFSI) {
            int ncells;
            if (il->suitms & GFGLBL) {
               /* Use all cells, no cell list */
               il->nsdcan = ncells = il->nelt;
#ifdef PARn
               /* Comp nodes also need space for their own data */
               il->nsdcpn = il->locell;
               il->nsdctn = il->mcells;
#endif
               }
            else {
               /* Use a cell list */
               ilst *pil = il->psdclb->pclil;
               /* All nodes need space for full cell list */
               il->nsdcan = ncells = (int)ilstitct(pil, il->nelt);
#ifdef PARn
               /* Comp nodes also need space for their own data */
               il->nsdcpn = (int)ilstitct(pil, il->locell);
               il->nsdctn = (int)ilstitct(pil, il->hicell) - il->nsdcpn;
#endif
               }
            /* Use larger of space for seg 0 or later segment */
            ncells += il->nsdctn;
            msws = (ulng)ncells * (ulng)(max(il->lsd1c, il->lsd1c0));
            if (msws > lsws) lsws = msws;
            } /* End if GFSI */
#else /* Serial -- just need space for one (missing) connection */
         if (!(RP->ksv & (KSDNE|KSDNOK|KSDOFF)) && il->suitms & GFSI) {
            msws = (ulng)(max(il->lsd1c, il->lsd1c0));
            if (msws > lsws) lsws = msws;
            }
#endif

         /* CONNTYPE items:  decay, regular and detailed amp stats,
         *  space for pre-volt-dep/mxax stats, cijfrac tables */
         for (ix=il->pct1; ix; ix=ix->pct) {
            int kdecay = qdecay(&ix->Cn.ADCY);
#ifndef PAR0
            /* Allocate space for decay.  Use ndal si32s per cell
            *  each for excitatory or inhibitory sums.  */
            if (kdecay) lveff[ix->Cn.ADCY.dcyopt & DCY_CHGOK] +=
               jmuzje((size_t)ix->Cn.ADCY.ndpsp, il->mcells,
                  EAfl(OVF_DCY));

            /* The following is really once-only code, but it is
            *  placed here rather than in d3genr() because some
            *  pointers are set that would not survive CONNTYPE
            *  broadcasts.  Trick:  BytesInXBits = 4-bit entries
            *  in a 32-bit word (divide by eight, round up) */
            if (ix->kgen & KGNMC) {
               struct FDHEADER *pfdhx = &RP->pfdh[ix->ucij.m.mno-1];
               int mtxsz = BytesInXBits(pfdhx->mrx*pfdhx->mry);
               ix->ucij.m.fdptr  = pfdhx->pfdm;
               ix->ucij.m.fdincr = mtxsz*ix->ucij.m.nks;
               ix->ucij.m.fdtop  = ix->ucij.m.fdptr +
                  mtxsz*ix->ucij.m.nkt;
               } /* End matrix Cij setup */

            /* Allocate space for keeping track of subarbor borders
            *  for CTPSB plots.  */
            if (il->Ct.kctp & CTPSB) {
               msws = (ulng)ix->nsarb * sizeof(struct NAPSLIM);
               if (msws > lsws) lsws = msws;
               }
#endif

            /* Allocate space for cijfrac tables used with NOPVP.
            *  (These tables depend on both il->nel and tcwid, so
            *  finding unique combinations would be a lot of code
            *  to save very little--just make tables for all
            *  conntypes that will use them.) */
            if (ix->Cn.cnopt & NOPVP) lcijfr += JSIZE*(il->nel/2 + 1);

            /* Allocate offsets for statistics */
            ix->qaffmods =
               ix->Cn.vdopt & VDANY || ix->Cn.mxax != SI32_MAX ||
               ix->phopt & PHASCNV;
            if (dostat) {
               ix->ocnddst = oddst;
               oddst += NCNDDST + kdecay + ix->qaffmods +
                  cnexists(ix, RBAR);
               ix->cnflgs &= ~DOFDIS;
               if ((il->Ct.kstat & KSTFD) && (ix->kgen & KGNMC)) {
                  ix->cnflgs |= DOFDIS;
                  ix->ucij.m.oFDMdist = odist;
                  odist += ix->ucij.m.nkt; }
               /* ix->ldas is 0 if no DAS, no need to test */
               lindv = jaul(lindv, (ui32)ix->ldas);
               }

            } /* End specific conntype loop */

         /* INHIBBLK items:  decay, stats */
         for (ib=il->pib1; ib; ib=ib->pib) {
            /* Count excit,inhib bands, set PSP bits */
            int kdecay = qdecay(&ib->GDCY);
#ifndef PAR0
            /* Allocate exact space needed for decay, viz., ndal si32s
            *  per psp sign (excit or inhib) per cell if self-avoidance
            *  set, else per group.  Use  as temp to hold this
            *  count for alloc loop below.  */
            if (kdecay) lveff[ib->GDCY.dcyopt & DCY_CHGOK] +=
               ib->ndcya = jmuzje((size_t)ib->GDCY.ndpsp,
                  NDecayCells(il, ib, il->mcells), EAfl(OVF_DCY));
#endif
            /* Allocate offsets for statistics */
            if (dostat) {
               ib->oidist = oddst;
               oddst += ib->nib + kdecay; }
            } /* End INHIBBLK scan */

         /* MODULATE items:  Allocate decay space and statistics */
         for (im=il->pmby1; im; im=im->pmby) {
            int kdecay = qdecay(&im->MDCY);
#ifndef PAR0
            if (kdecay) lveff[im->MDCY.dcyopt & DCY_CHGOK] +=
               (size_t)im->MDCY.ndpsp;
#endif
            if (dostat) {
               im->omdist = oddst;
               oddst += 1 + kdecay; }
            } /* End MODBY loop */

         /* Space for praffs which now includes data for raw and final
         *  afference stats, RBAR sums, and [rs]ax[pn] work areas.
         *  N.B.  Both sets of [rs]ax[pn] are now allocated even if
         *  only one set is needed for all conntypes (rare anyway),
         *  but having both makes code in d3go a little simpler.  */
#ifndef PAR0
         {  int mpsums = NNCTS*(ui32)il->nct + NEXIN*(ui32)RP->mxlax;
            if (mpsums > npsums) npsums = mpsums;
            } /* End mpsums local scope */
#endif

         /* CELLTYPE statistics, now including above dists.  All
         *  components have si64/ui64, so ALIGN_UP is not needed.  */
         if (dostat) {
            ui32 tlindv = sizeof(struct CLSTAT) +
               (ui32)il->nauts * sizeof(struct ASSTAT) +
               sizeof(ddist) * (il->nctddst = oddst) +
               sizeof(dist)  * (il->nctdist = odist);
            if (il->Ct.kstat & KSTHR)
               tlindv += 2*sizeof(hist)*il->nmdlts;
            lindv = jaul(lindv, tlindv);
            }

         } /* End loop over cell layers */

      } /* End loop over repertoires */

/* Determine total space that must be allocated for all s(i) and
*  repertoire data on this node.  The nslu2 data do not exist on
*  a PAR0 node.  */
   e64dec(EAfl(OVF_GEN));
   lgall = ymulup(lgall, BYTE_ALIGN);
   lrall = ymulup(lrall, BYTE_ALIGN);
   nslu  = ymulup(nslu,  BYTE_ALIGN);
   lsall = sl2w(RP->xtrasize);
   lsall = jauw(lsall, nslu);
   lsall = jauw(lsall, nslup);   /* nslup = aligned ptrs */
   /* Space for all s(i) + modality marks + extras */
#ifndef PAR0
   nslu2 = zmulup(nslu2, BYTE_ALIGN);
   lsall = jauzd(lsall, nslu2);
#endif
   lsws = ALIGN_UP(lsws);
#ifndef PARn
   RP0->cumsws = lsws;
   CDAT.ngstat = ogstat;
   ogstat *= sizeof(struct GPSTAT); ogstat = ALIGN_UP(ogstat);
   lindv = jaul(lindv, ogstat);
#endif
#ifndef PAR0
   RP->lmarkh = omarkh = ALIGN_UP(omarkh);
   lindv = jaul(lindv, omarkh);
   RP->lmarks = omarks = ALIGN_UP(omarks);
   lindv = jaul(lindv, omarkh);
#endif
#ifdef PAR
   lhlsi = RP->nhlssv * RP->nhlpt;
   RP->nhlsi = lhlsi;
   lhlsi *= sizeof(struct HILOSI); lhlsi = ALIGN_UP(lhlsi);
   lindv = jaul(lindv, lhlsi);
#endif
   lindv = jaulo(lindv, lsws);
   lovec = sizeof(long)*RP->novec; lovec = ALIGN_UP(lovec);
   lindv = jaul(lindv, lovec);
   lassl = ALIGN_UP(lassl);
   lindv = jaulo(lindv, lassl);

#ifdef PAR0
   /* Node 0 stores kernels only if needed for printing */
   ltot = jaul(ltot, RP0->lknprt);
#else
   {  size_t tlv0 = jmuzjd(lveff[0], JSIZE);
      size_t tlv1 = jmuzjd(lveff[1], JSIZE);
      lveff[0] = ALIGN_UP(tlv0);
      lveff[1] = ALIGN_UP(tlv1);
      } /* End local scope */
   ltot = jauzd(ltot, lveff[0]);
   ltot = jauld(ltot, RP->lkall);
   lindv = jauzd(lindv, lveff[1]);
   lpsums = WSIZE*npsums;
   lindv = jauzd(lindv, lpsums);
#endif
   ltot = jauw(ltot, lgall);
   ltot = jauw(ltot, lrall);
   ltot = jauw(ltot, lsall);
   lcijfr = ALIGN_UP(lcijfr);
   ltot = jauzd(ltot, lcijfr);

/*---------------------------------------------------------------------*
*  If the space was not already allocated, i.e., this is the first     *
*  call, allocate storage now for all repertoires, decay information,  *
*  and anything else in CELLDATA that must be preserved across Group   *
*  III broadcasts.  Include reserve in lindv space.  Changed 08/18/93  *
*  to zero the allocated storage so Mij, modality markers, and any     *
*  future variables not set in d3genr will be correctly initialized.   *
*  Rev, 09/28/07, GNR - Add space in praffs for rbar sums [moved]      *
*  Rev, 02/16/13, GNR - Separate fixed, movable persistence storage    *
*  R76, 11/18/17, GNR - Merge [rp]sumaij, include space in lindv       *
*---------------------------------------------------------------------*/

   if (kcall == KMEM_GENR) {
      myprd = (rd_type *)callocv(1, uw2zd(ltot), "Region data");
      RP->ldrsrv = ALIGN_UP(RP->ldrsrv);
      lindv = jaulo(lindv, RP->ldrsrv);

#ifndef PAR0
      /* Initialize CDAT.pdecay to access two decay routines */
      CDAT.pdecay[0] = d3decay;     /* Normal decay */
      CDAT.pdecay[1] = fsdecay;     /* Fast-start decay */
#endif
      }

/* High-water mark allocation for individual data that may change at
*  Group III time.  realloc() is not needed because data do not need
*  to be copied.  If this changes, must copy each block individually.
*  (If kdcy changes, and thus ndal, there would seem to be no point
*  to preserving old h(t),g(t) decay info.)
*/

   if (qcuw(CDAT.lnsetindv, lindv)) {
      if (CDAT.pAffD64) freev(CDAT.pAffD64,
         "Variable-length connection data");
      CDAT.lnsetindv = lindv;
      CDAT.pAffD64 = (si64 *)mallocv(uw2zd(lindv),
         "Variable-length connection data");
      }

/*---------------------------------------------------------------------*
*  Make a second pass through all repertoires and celltypes.  Assign   *
*  starting addresses for each type of data area. These ptrs (other    *
*  than those derived from pnxti) point to data that may need to be    *
*  preserved across trials, so be sure they always point to the same   *
*  places each time this code runs.  Be sure not to shuffle the s(i)   *
*  delay ptr array (to preserve permutation).  (Each length is in      *
*  bytes, not number of variables, and is aligned up.)                 *
*  Also initialize Lij,Cij,Sj,Dij generation.                          *
*---------------------------------------------------------------------*/

   pnxti  = (char *)CDAT.pAffD64 + RP->lAffD64;
   CDAT.pAffD32 = (si32 *)pnxti; pnxti += RP->lAffD32;
   CDAT.pconnd = (struct CONNDATA *)pnxti; pnxti += lconnd;
   CDAT.passl0 = (struct AUTSSL *)pnxti; pnxti += lassl;
   CDAT.povec  = (long *)pnxti; pnxti += lovec;
#ifndef PARn
   CDAT.pgstat = (struct GPSTAT *)pnxti; pnxti += ogstat;
#endif
#ifdef PAR
   CDAT.phlsi = (struct HILOSI *)pnxti; pnxti += lhlsi;
#endif
#ifndef PAR0
   CDAT.pavgcd = (struct AVGCDATA *)pnxti; pnxti += lavgcd;
   CDAT.praffs = (si64 *)pnxti; pnxti += lpsums;
   RP->boxsum = (si64 *)pnxti; pnxti += uw2zd(RP->lishare);
   CDAT.pmarks = pnxti; pnxti += omarks;
   CDAT.pmarkh = pnxti; pnxti += omarkh;
#endif
   CDAT.psws   = pnxti; pnxti += lsws;
   CDAT.pmbits = RP->pbcst + RP->ombits;
   pgd = (char *)(pnxtgd = myprd); pgd += uw2zd(lgall);
   pnxtps = (s_type **)pgd;
   pnxts  = (s_type *)(pgd + uw2zd(nslup));
   pnxts2 = pnxts + uw2zd(nslu); /* Harmless if PAR0 */
   pgd += uw2zd(lsall);
#ifndef PAR0
   pnxtr  = (rd_type *)pgd; pgd += uw2zd(lrall);
   pnkern = pgd; pgd += RP->lkall;
   pnxtveff[0] = CDAT.pdpers[0] = (si32 *)pgd; pgd += lveff[0];
   pnxtveff[1] = CDAT.pdpers[1] = (si32 *)pnxti; pnxti += lveff[1];
   /* The following zeroing is for compatibility with current
   *  reset codes.  Remove once reset control is updated.  */
   memset((char *)pnxtveff[0], 0, lveff[0]);
   memset((char *)pnxtveff[1], 0, lveff[1]);
   pnnorm = pnxti, pnxti += RP->liindiv;
#else
   pnkern = pgd; pgd += RP0->lknprt;
   pnnorm = pnxti;               /* To kill uninitialized warning */
   if (RP->kpl & KPLEDG) {                /* PAR0 does edge plots */
      pnxti += RP->liindiv;
      RP->boxsum = (si64 *)pnxti, pnxti += RP0->li0bxs;
      }
#endif
   pncijfr = (si32 *)pgd; pgd += lcijfr;

   e64set(E64_COUNT, CDAT.povec);   /* Set overflow count location */
   e64dec(EAfl(OVF_ALLO));          /* But don't use it yet */
   for (ir=RP->tree; ir; ir=ir->pnrp) {
      ui32 irngx = (ui32)ir->ngx;
#ifndef PARn
      ui32 irngy = (ui32)ir->ngy;
#endif

/* Initialization of group-specific variables */

      ir->pgpd = pnxtgd;
         pnxtgd += ir->lgt;

      /* Set Cext ptrs for ions belonging to this region */
      pgion = (float *)(ir->pgpd + ir->gpoff[IONG]);
      if (ir->pionr) d3nsetion(ir->pionr, &pgion);

/* Loop over layers (cell types)  */

      for (il=ir->play1; il; il=il->play) {
#ifndef PAR0
         struct PRBDEF *pprb;
#endif
         ui32 ilmgy;       /* Rows of groups on this node */
         e64dec(EAfl(OVF_ALLO));    /* Changed by probe setup below */
         dostat = !(il->Ct.kstat & KSTNS);

         /* Info regarding groups on current node--currently
         *  only used for gconns, but is really more generic.  */
#ifdef PAR0
         ilmgy = (RP->kpl & KPLEDG) ? irngy : 0;
#elif !defined(PAR)
         ilmgy = irngy;
#else
         /* This setting of ilmgy should assure that boxsumbb
         *  never goes above the top of the source region.  */
         ilmgy = il->higrpy - il->logrpy;
#endif
         il->mgy = ilmgy;

/* Initialize pointers to repertoire data.
*  Note that pps points to an array of pointers indexed by time
*     delay, which are useful for accessing delayed s(i) values.
*  No access is provided to the set of s(i) arrays as a whole.  */

         il->pps  = pnxtps;
         if (kcall != KMEM_GENR) {        /* Not first call */
            pnxtps += il->mxsdelay1;
            pnxts += uw2zd(jmuwjd(il->wspt, (ui32)il->mxsdelay1));
            }
         else {               /* First call */
            register int idly;
            for (idly=0; idly<(int)il->mxsdelay1; idly++)
               *pnxtps++ = pnxts, pnxts += il->lspt;
#ifndef PAR          /* Serial:  set ps2 first time only */
            il->ps2 = pnxts2, pnxts2 += il->lspt;
#endif
            }

#ifndef PAR0
#ifdef PAR           /* Parallel: set ps2 after every broadcast */
         il->ps2 = pnxts2;
            pnxts2 += spsize(il->mcells,il->phshft);
#endif

         /* Allocate space for SBAR, QBAR, DEPR, DST, etc. */
         il->prd = pnxtr, pnxtr += uw2zd(il->llt);
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
               pnxti += sizeof(struct CLSTAT) +
                  il->nauts * sizeof(struct ASSTAT);
            if (il->Ct.kstat & KSTHR) {
               il->phrs = (hist *)pnxti;
               pnxti += sizeof(hist) * il->nmdlts; }
            }

/* Deal with specific connection types */

         for (ix=il->pct1; ix; ix=ix->pct) {
#ifndef PAR0
            /* Allocate space for Lij, Cij, Mij, etc. */
            ix->psyn0 = pnxtsyn0, pnxtsyn0 += ix->lct;
            /* If a bar plot uses this conntype, store ptrs to bar
            *  AK and RAW data for first cell on node.  This takes
            *  a few more bytes in CELLTYPE than copying xnoff's
            *  separately, but makes d3lplt a tad faster.  (Could
            *  check CTPHP here, no harm doing this anyway.)  */
            if (il->Ct.ksbarn > 0) {
               int ibar;
               for (ibar=0; ibar<il->Ct.ksbarn; ++ibar) {
                  ui16 aict = (ui16)il->Ct.kksbar[ibar] >> 4;
                  if (aict == ix->ict) {
                     il->prdbak[ibar] = ix->psyn0 + ix->xnoff[AK];
                     il->prdraw[ibar] = ix->psyn0 + ix->xnoff[RAW];
                     }
                  }
               }

            /* Allocate space for EPSP/IPSP decay */
            if (qdecay(&ix->Cn.ADCY)) {
               int idt = ix->Cn.ADCY.dcyopt & DCY_CHGOK;
               ix->Cn.ADCY.peipv = pnxtveff[idt];
               pnxtveff[idt] +=
                  jmuzjd((size_t)ix->Cn.ADCY.ndpsp, il->mcells);
               }
#endif
            /* Allocate space for Cij fraction tables */
            if (ix->Cn.cnopt & NOPVP)
               ix->cijfrac = pncijfr, pncijfr += il->nel/2 + 1;
            /* Allocate space for Lij gen kernels */
#ifdef PAR0
            if (ix->kgen & KGNKN && ix->knopt & KNO_PRINT &&
                  RP->CP.runflags & RP_OKPRIN)
#else
            if (ix->kgen & KGNKN)
#endif
               {
               ix->ul2.k.ptori = (kern **)pnkern;
               ix->ul2.k.pkern = (kern *)
                  (pnkern + il->nel*sizeof(kern *));
               pnkern += ix->ul2.k.lkern;
               }
            /* Allocate space for KGNAN (annulus) grid tables */
            if (ix->kgen & KGNAN) {
               ix->ul2.q.paxy = (xysh *)pnkern;
               pnkern += ALIGN_UP(sizeof(xysh)*ix->ul2.q.lannt);
               }
            /* Allocate space for CONNTYPE statistics (all nodes) */
            if (dostat && (ix->Cn.kam & KAMDS)) {
               ix->pdas = (si64 *)pnxti;
                  pnxti += ix->ldas;
               } /* End CONNTYPE stats */
            } /* End CONNTYPE loop */

/* Deal with geometric connection types.  The following code is very
*  similar to that in d3allo() except that it is now specific to each
*  computational node.  In particular, different nodes will have
*  different numbers of rows in the boxsums and node 0 will need
*  room for all boxes (not bands), but only if box plot is requested.
*  (But to minimize complexity, the same lishare + liindiv space is
*  allocated on all nodes--some will use a little less.)  */

         if ((ib = il->pib1)) {  /* Assignment intended */
            ui32 nrm1;           /* Number of rings - 1 */
            ui32 twonrm1;        /* Twice nrm1 */

/* Variables used in construction of boxsum, hstrip, vstrip etc.
*  d3inhb() constructs pointers from RP->boxsum on each call.
*  R65, 01/19/16, GNR - Move 'l1' variable setup code here from d3tchk.
*/

#ifndef PAR0
            pnbands = (si64 *)((char *)RP->boxsum + uw2zd(il->liboxes));
#endif
            for ( ; ib; ib=ib->pib) {
               /* max(rings-1), note which INHIBBLK has largest nr */
               nrm1 = (ui32)ib->l1n1;
               twonrm1 = nrm1 + nrm1;
               ib->gcarea = (si32)((twonrm1+1)*(twonrm1+1));
               ib->l1n1x = irngx*nrm1;        /* Cannot overflow */
               ib->l1xn2 = irngx + twonrm1;
               ib->l1n1xn2 = ib->l1xn2*nrm1;  /* Cannot overflow */
               ib->l1yn2 = ilmgy + twonrm1;
               ib->l1xyn2 = jmuld(irngx, ib->l1yn2);
               ib->l1yxn2 = jmuld(ilmgy, ib->l1xn2);
               ib->l1xn2yn2 = jmuld(ib->l1xn2, ib->l1yn2);

#ifndef PAR0
               /* Allocate space for effective value decay using count
               *  prestored in iu1.ndcy1.  We don't need to determine
               *  whether our node starts on an even group boundary,
               *  just allocate the standard amount.  */
               if (qdecay(&ib->GDCY)) {
                  int idt = ib->GDCY.dcyopt & DCY_CHGOK;
                  ib->GDCY.peipv = pnxtveff[idt];
                  pnxtveff[idt] += ib->ndcya; }

               /* Allocate bandsum arrays consecutively (required by
               *  zeroing code in d3inhb) for all INHIBBLKs in the
               *  current celltype from an origin determined by the
               *  largest boxsum array needed.  */
               {  size_t lband = il->higrp - il->logrp;
                  register int iband;
                  for (iband=0; iband<(int)ib->nib; iband++) {
                     ib->inhbnd[iband].bandsum = pnbands;
                     pnbands += lband;
                     }
                  } /* End iband local scope */
#endif
#ifdef PAR0
            if (RP->kpl & KPLEDG) {
#endif
               /* Allocate space for norm table if needed */
               if (ib->kbc == BC_NORM) {
                  size_t mxngxy = max(irngx, ilmgy);
                  ib->xynorm = (struct XYNORMDEF *)pnnorm;
                     pnnorm += mxngxy*sizeof(struct XYNORMDEF);
                  }  /* End if BC=NORM */

               /* Allocate space for round-region table if needed */
               if (ib->ibopt & IBOPRS) {
                  ib->prroff = (size_t *)pnnorm;
                     pnnorm += ib->nrro*sizeof(size_t);
                  }
#ifdef PAR0
               } /* End enclosing KPLEDGE test if PAR0 */
#endif
               }  /* End INHIBBLK loop */
            } /* End handling INHIBBLKs */

#ifndef PAR0
         /* MODULATE items:  Allocate decay space */
         for (im=il->pmby1; im; im=im->pmby) {
            if (qdecay(&im->MDCY)) {
               int idt = im->MDCY.dcyopt & DCY_CHGOK;
               im->MDCY.peipv = pnxtveff[idt];
               pnxtveff[idt] += (long)im->MDCY.ndpsp;
               }
            } /* End MODBY loop */

/* Setup for probes.  The definition of pptr requires that each
*  node know when to begin processing any cells on the cell list
*  that are on that node.  This calculation is done here.  */

         for (pprb=il->pctprb; pprb; pprb=pprb->pnctp) {
            struct CLBLK  *pcb = pprb->pclb;
            ilst *pil = pcb->pclil;
            long ncells,tpbc;       /* Cells in a list below a bound */
            si32 mcells;

            /* Number of cells in a full cycle of this list */
            e64dec(EAct(OVF_PROB));
            ncells = ilstitct(pil, (long)il->nelt);
#if LSIZE == 8
            swlodm(mcells, ncells);
#else
            mcells = (si32)ncells;
#endif

            if (pprb->pptr > 0) {   /* Don't do all probes in each trial */
#ifdef PAR
               div_t qrm;
#endif
               tpbc = (ncells + (long)(pprb->pptr-1))/pprb->pptr;
               swlodm(pprb->lprbcyc, tpbc);
#ifdef PAR
               ncells        = ilstitct(pil, (long)il->locell);
               swlodm(mcells, ncells);
               qrm           = div(mcells, pprb->pptr);
               pprb->myipc1  = qrm.quot;
               pprb->myipt1  = pprb->pptr - qrm.rem;
#endif
               }
            else {                  /* Do all probes in each trial */
               pprb->lprbcyc = 1;
#ifdef PAR
               pprb->myipc1  = 0;
               pprb->myipt1  = mcells;
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
      else if (imv->Mdc.mvxc.tvmode == BM_NONSTD)
         imv->amsrc = (s_type *)(RP->paw + imv->omsrc);
      else
         imv->amsrc = (s_type *)RP->pbcst + imv->omsrc;
      imv->mdgetfn = d3gvin(imv->mdsrctyp, &imv->Mdc.mvxc);

      } /* End imv loop */
#endif

#ifndef PARn
   /* Tell BBD and UPGM cameras where to store images */
   {  struct TVDEF *cam;
      for (cam=RP->pftv; cam; cam=cam->pnxtv) {
         if (!(cam->utv.tvsflgs & TV_ON)) continue;
#ifdef BBDS
         if (cam->utv.tvsflgs & TV_BBD) {
            BBDDev *pbbd = (BBDDev *)cam->utv.ptvsrc;
            pbbd->UD.Cam.pim = cam->tviflgs & (TV_Reduc|TV_SpAvg) ?
               (byte *)CDAT.psws : cam->pmyim;
            }
#endif
         if (cam->utv.tvsflgs & TV_UPGM) {
            cam->utv.p1row = (byte *)CDAT.psws;
            if (cam->tviflgs & (TV_Reduc|TV_SpAvg))
               cam->utv.p1row += cam->utv.ltvrxy;
            }
         } /* End camera loop */
      } /* End cam local scope */
#endif

   return myprd;
   } /* End d3nset() */

