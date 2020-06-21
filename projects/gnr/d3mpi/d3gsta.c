/* (c) Copyright 1991-2018, The Rockefeller University *21117* */
/* $Id: d3gsta.c 75 2017-10-13 19:38:52Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                   d3gsta - gather the statistics                     *
*                                                                      *
*             d3ghls - gather the high/low response stats              *
*                                                                      *
*  This routine is called at the end of each trial series.             *
*  It is used only in the parallel version, and is executed on all     *
*  nodes to gather statistics.  Cross-response statistics are gathered *
*  one cell layer at a time, interleaved with printing, by d3stat.     *
*                                                                      *
*  RESTRICTION:  Code currently assumes a homogeneous collection of    *
*  comp nodes, so nxdr reformatting is used only for sending to host.  *
************************************************************************
*  V5C, 12/26/91, GNR - Add D1 stats, remove d3stat call               *
*  V5E, 07/15/92, GNR - Use prompted messages throughout               *
*  Rev, 12/14/92, ABP - Comment out odd node conditional on host node, *
*                       add HYB code to return data to host            *
*  V6A, 03/29/93, GNR - Add mdist (distribution of modulation value)   *
*  V6C, 09/17/93, GNR - Improved ring algorithm, make DAS double       *
*  V7A, 04/20/94, GNR - Revise to work with one comp node              *
*  V7C, 11/19/94, GNR - Add C,F dist stats, DIDSTATS tests             *
*  V8A, 05/12/96, GNR - Make GCONN/MOD stats dynamically allocated     *
*  Rev, 11/26/96, GNR - Remove support for non-hybrid version          *
*  Rev, 09/28/97, GNR - Independent dynamic allocations per conntype   *
*  V8C, 06/08/03, GNR - Cell responses in millivolts, add conductances *
*  V8D, 05/20/06, GNR - Change hhcom() calls to nncom()                *
*  ==>, 12/28/07, GNR - Last mod before committing to svn repository   *
*  V8F, 05/17/10, GNR - Add KRPGP statistics                           *
*  R67, 04/27/16, GNR - Remove Darwin 1 support, remove RING support   *
*  R67, 06/24/16, GNR - Rewrite to use coppack()                       *
*  R70, 01/25/17, GNR - Add d3ghls code to gather high/low aff stats   *
*  R78, 05/19/18, GNR - Correct gathering of info for KSTGP|KSTMG      *
***********************************************************************/

/*
*  D3GSTA can handle amounts of statistics that exceed the size of a
*  message buffer--a merge is done each time the buffer gets full.
*  The repertoire tree itself is used to store the intermediate and
*  final results, where they are handy for printing out with d3stat().
*  Nodes other than host have garbage in their statistics at the end.
*  Rather than create a giant COPDEF to define the merging process, it
*  is done in pieces, one CELLTYPE or CONNBLK at a time.
*/

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "sysdef.h"
#include "d3global.h"
#include "celldata.h"
#include "bapkg.h"
#include "collect.h"

/* To activate debug output for high/low stats: */
/* #define DBGHLS */
/*  Also #define DBGHLSD for more detail */
/* #include "rksubs.h" */
/* #define DBGHLSD */

extern struct CELLDATA CDAT;

#ifdef PAR
/*=====================================================================*
*                               d3ghls                                 *
*=====================================================================*/

void d3ghls(void) {

#ifdef PAR0
   struct CELLTYPE *il;
   struct HILOSI *phlse;
#endif
   struct HILOSI *phls;
   ui32 nhlrem;

   if (CDAT.ihlsi == 0) return;     /* JIC */

/* COPDEF for collecting HILOSI data.
*  Repeat count field is set dynamically */
#define NumHlsOps 5           /* Number of COPDEFs in phlsi stats */
   static struct COPDEF HlsCop[NumHlsOps] = {
      { REPEAT+RepCop(4), 1 },
      { MAX+UI32V, 1 },
      { ADD+UI32V, 1 },
      { MAX+SI32V, 1 },
      { MIN+SI32V, 1 } };

#ifdef DBGHLS
   int  trial = RP->CP.trial - CDAT.ihlsi;
   dbgprt(ssprintf(NULL, "In d3ghls, Trial %4d, ihlsi = %d",
      RP->CP.trial, CDAT.ihlsi ));
#endif

/* Collect phlsi arrays across all nodes (in batches if too many) */

   nhlrem = CDAT.ihlsi * RP->nhlpt;
   phls = CDAT.phlsi;
   while (nhlrem > 0) {
      ui32 nhldo = min(nhlrem, UI16_MAX);
      HlsCop[0].count = nhldo;
      vecollect((char *)phls, HlsCop, NumHlsOps, HLSSTAT_MSG,
         nhldo*sizeof(struct HILOSI));
      phls += nhldo; nhlrem -= nhldo;
      }

/* Continuing on host node, loop over trials in current batch,
*  binning the [hl]dist and GPSTAT values for each celltype */

#ifdef PAR0
   phls = CDAT.phlsi;
   phlse = CDAT.phlsi + CDAT.ihlsi * RP->nhlpt;
   while (phls < phlse) {
      for (il=RP->pfct; il; il=il->pnct) {
         si32 lhisi,llosi;
         int  ihab,ilab;
         if (il->Ct.kstat & KSTNS) continue;
         lhisi = phls->hiresp;
         llosi = phls->loresp; ++phls;
         if (RP->compat & OLDRANGE) {
            ihab = lhisi >> (FBwk+3);
            ilab = llosi >> (FBwk+3);
            }
         else {
            if (lhisi >= 0)
               ihab = (LDSTAT/2) + bitsz((lhisi>>(FBwk-1))+1);
            else
               ihab = (LDSTAT/2) - bitsz((abs32(lhisi)>>(FBwk-1))+1);
            if (llosi >= 0)
               ilab = (LDSTAT/2) + bitsz((llosi>>(FBwk-1))+1);
            else
               ilab = (LDSTAT/2) - bitsz((abs32(llosi)>>(FBwk-1))+1);
            }
         if (ihab > (LDSTAT-1)) ihab = LDSTAT-1;
         else if (ihab < 0) ihab = 0;
         if (ilab > (LDSTAT-1)) ilab = LDSTAT-1;
         else if (ilab < 0) ilab = 0;
#ifdef DBGHLSD
#define WCODE (RK_IORF|RK_NI32|7)
   {  char hiresp[10],loresp[10];
      if (RP->compat & OLDRANGE) {
         wbcdwtPn(&lhisi, hiresp, (27<<RK_BS)+(5<<RK_DS)+WCODE);
         wbcdwtPn(&llosi, loresp, (27<<RK_BS)+(5<<RK_DS)+WCODE); }
      else {
         wbcdwtPn(&lhisi, hiresp, (20<<RK_BS)+(4<<RK_DS)+WCODE);
         wbcdwtPn(&llosi, loresp, (20<<RK_BS)+(4<<RK_DS)+WCODE); }
      dbgprt(ssprintf(NULL,"Tr %4d, CT %4s, hisi %8s, ihab %4d "
         "losi %8s, ilab %4d", trial, il->lname,
         hiresp, ihab, loresp, ilab));
      }
#endif
         il->CTST->hdist[ihab] += 1;
         il->CTST->ldist[ilab] += 1;
#ifdef DBGHLS
   dbgprt(ssprintf(NULL,"Tr %4d, CT %4s, hdist[%4d] = %8d, "
      "ldist[%4d] = %8d", trial, il->lname,
      ihab, il->CTST->hdist[ihab], ilab, il->CTST->ldist[ilab]));
#endif
/* And now the GPSTAT data must be computed from the HILOSI data.  One
*  GPSTAT is allocated for each distinct stimulus in each modality the
*  celltype responds to.  The code here does not need to know which
*  modality or which stimulus is which--it just needs to check all the
*  HILOSI in turn and add into the GPSTATs.  (See serial version in
*  d3go.)
*  Rev, 08/14/18, GNR - I eliminated a loop over pmarks here that
*     allowed groups of 8 unencountered stimuli (a NULL pmark byte)
*     to be skipped.  This made it possible to get rid of a whole
*     machinery to collect these markings and pass (or compute)
*     them on the PAR0 node, which may have taken longer than all
*     the nhits checks that are zero.  */
         if (il->Ct.kstat & (KSTGP|KSTMG)) {
            struct GPSTAT *pgp = CDAT.pgstat + il->oGstat;
            struct HILOSI *qhlse = phls + il->nCdist;
#ifdef DBGHLSD
            struct HILOSI *phls0 = phls-1;
#endif
            for ( ; phls<qhlse; ++phls,++pgp) {
               if (!phls->nstim) continue;
               pgp->nstim += phls->nstim;
               pgp->nhits += phls->nhits;
               if (phls->hiresp > pgp->hiresp)
                  pgp->hiresp = phls->hiresp;
               if (phls->hiresp < pgp->lohiresp)
                  pgp->lohiresp = phls->hiresp;
               if (phls->loresp < pgp->loresp)
                  pgp->loresp = phls->loresp;
               if (phls->loresp > pgp->hiloresp)
                  pgp->hiloresp = phls->loresp;
#ifdef DBGHLSD
   {  char hiresp[10],loresp[10];
      if (RP->compat & OLDRANGE) {
         wbcdwtPn(&pgp->hiresp, hiresp, (27<<RK_BS)+(5<<RK_DS)+WCODE);
         wbcdwtPn(&pgp->loresp, loresp, (27<<RK_BS)+(5<<RK_DS)+WCODE); }
      else {
         wbcdwtPn(&pgp->hiresp, hiresp, (20<<RK_BS)+(4<<RK_DS)+WCODE);
         wbcdwtPn(&pgp->loresp, loresp, (20<<RK_BS)+(4<<RK_DS)+WCODE); }
      dbgprt(ssprintf(NULL,"Tr %4d, CT %4s, isn %d, nstim %d, nhits %d, "
         "hiresp %8s, loresp %8s", trial, il->lname, phls-phls0,
         pgp->nstim, pgp->nhits, hiresp, loresp));
      }
#endif
               } /* End loop over modalities and stimuli */
            } /* End recording GPSTAT stats */
         } /* End loop over cell types */
#ifdef DBGHLSD
   ++trial;
#endif
      } /* End loop over recorded trials/cycles */
#endif   /* PAR0 */
   } /* End d3ghls() */
#endif

/*=====================================================================*
*                               d3gsta                                 *
*=====================================================================*/

void d3gsta(void) {

/*---------------------------------------------------------------------*
*                          COPDEF Structures                           *
*---------------------------------------------------------------------*/

/* The following declarations define the structure of the statistics
*  in CELLTYPEs, CONNTYPEs, INHIBBLKs, and MODBY blocks.  They must
*  be maintained in step with the definitions of those structures in
*  celltype.h, conntype.h, etc. */

/* Define statistics in CELLTYPE structure */
#define NumCellOps  5         /* Number of COPDEFs in cell stats */
   static struct COPDEF CellCop[NumCellOps] = {
      { ADD+LLONG, 4 },
      { MIN+LONG,  1 },
      { MAX+LONG,  2 },       /* Max(nstcap) OK, same all nodes */
      { ADD+LONG, 10 },
      { ADD+LONG, 2*LDSTAT }, /* d3ghls handles hdist,ldist */
      } ;

/* Define statistics in CONNTYPE structure */
#define NumConnOps  3         /* Number of COPDEFs in conn stats */
   static struct COPDEF ConnCop[NumConnOps] = {
      { ADD+LLONG, 2 },
      { NOOP+LONG, 1 },
      { ADD+LONG,  8 }
      } ;

#define NumDasOps   1         /* Number of COPDEFs in DAS stats  */
   static struct COPDEF DasCop[NumDasOps] = {
      { ADD+LLONG,  0 }       /* Size filled in dynamically */
      } ;

/* Define long and long long distribution statistics */
#define NumDistOps  2         /* Number of COPDEFs in a distribution */
   static struct COPDEF DdistCop[NumDistOps] = {
      { REPEAT+RepCop(1), 0 },
      { ADD+LLONG, LDSTAT }};
   static struct COPDEF DistCop[NumDistOps] = {
      { REPEAT+RepCop(1), 0 },
      { ADD+LONG,  LDSTAT }} ;

/*---------------------------------------------------------------------*
*                                                                      *
*                         Other Declarations                           *
*                                                                      *
*---------------------------------------------------------------------*/

   CPKD *pcpk;                /* Ptr to control array for coppack() */
   struct REPBLOCK *ir;       /* Scanning repertoire pointer */
   struct CELLTYPE *il;       /* Scanning celltype pointer */
   struct CONNTYPE *ix;       /* Scanning conntype pointer */

/*---------------------------------------------------------------------*
*                                                                      *
*                          Executable Code                             *
*                                                                      *
*---------------------------------------------------------------------*/

   pcpk = coppini(STATS_MSG, 0, 0);

   /* Loop over repertoires, then celltypes, then conntypes */
   for (ir=RP->tree; ir; ir=ir->pnrp) {
      /* Skip gather if statistics suppressed */
      if (ir->Rp.krp & KRPNS) continue;

      for (il=ir->play1; il; il=il->play) {
         /* Skip gather if statistics suppressed */
         if (!(il->ctf & DIDSTATS)) continue;
         /* Collect base celltype stats */
         coppack(pcpk, il->CTST, CellCop, NumCellOps);
         /* Collect LLONG and LONG distribution stats */
         DdistCop[0].count = (ui16)il->nctddst;
         coppack(pcpk, il->pctddst, DdistCop, NumDistOps);
         DistCop[0].count = (ui16)il->nctdist;
         coppack(pcpk, il->pctdist, DistCop, NumDistOps);

         /* Get connection-level stats */
         for (ix=il->pct1; ix; ix=ix->pct) {
            /* Gather detailed amplification statistics */
            if (ix->Cn.kam & KAMDS) {
               DasCop[0].count = ix->ldas/sizeof(si64);
               coppack(pcpk, ix->pdas, DasCop, NumDasOps);
               }
            /* Gather regular connection stats */
            coppack(pcpk, &ix->CNST, ConnCop, NumConnOps);
            } /* End loop over connection types */

         } /* End loop over cell blocks */

      } /* End loop over regions */

   /* Complete processing of any partial buffers */
   copcomb(pcpk);
   /* Free up the collection buffers
   *  (could do once per run for a tiny speed increase) */
   copfree(pcpk);

   /* Pick up any remnants not collected in d3go() */
   if (CDAT.ihlsi > 0) d3ghls();

   } /* End d3gsta() */
