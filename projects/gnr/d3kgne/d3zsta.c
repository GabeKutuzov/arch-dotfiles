/* (c) Copyright 1991-2010, Neurosciences Research Foundation, Inc. */
/* $Id: d3zsta.c 50 2012-05-17 19:36:30Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                               d3zsta                                 *
*                                                                      *
*     Zero all statistical accumulators for a series of trials.        *
*                                                                      *
*  V5C, 12/01/91, GNR - Broken out from darwin3.c, D1 stats added      *
*  V6A, 03/29/93, GNR - Add mdist                                      *
*  Rev, 06/03/93, ABP - Add header files with memset prototype         *
*  V7C, 11/19/94, GNR - Add C,F dist stats                             *
*  V8A, 06/10/95, GNR - Move XRM back into main loop, always clear RGH,*
*                       pgpno and mxisn now for all modalities         *
*  Rev, 12/30/97, GNR - Statistics now subject to deferred allocation  *
*  Rev, 10/28/00, GNR - Remove XRH, tabulate KRPHR over all cycles     *
*  ==>, 12/28/07, GNR - Last mod before committing to svn repository   *
*  V8E, 11/07/09, GNR - Move nntr to CLSTAT.nstcap (may differ per il) *
*  V8F, 05/17/10, GNR - Add KRPGP statistics                           *
*  V8H, 05/15/12, GNR - Add loascl, hiascl statistics                  *
***********************************************************************/

#ifdef D1
#define D1TYPE  struct D1BLK
#endif
#define GSTYPE  struct GSDEF

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "d3global.h"
#include "celldata.h"
#include "statdef.h"
#include "bapkg.h"
#ifdef D1
#include "d1def.h"
#endif

extern struct CELLDATA CDAT;

void d3zsta(void) {

   struct REPBLOCK  *ir;   /* The traditional pointers */
   struct CELLTYPE  *il;
   struct CONNTYPE  *ix;
   struct MODALITY  *pmdlt;
#ifdef D1
   D1TYPE *pd1;            /* Ptr to Darwin 1 data blocks */
#endif

   if (!(RP->CP.runflags & (RP_NOSTAT|RP_NOCYST))) {

#ifdef D1
/* Clear Darwin 1 statistics.
*  Note:  At present, all stats are in D1BLK itself and therefore
*  are always allocated.  It is not worth testing NOSTATS option
*  to skip memset here.  */

      for (pd1=RP->pd1b1; pd1; pd1=pd1->pnd1b) {
         memset((char *)&pd1->stats, 0, sizeof(struct D1STATS));
         pd1->stats.d1minsco = LONG_MAX;
         }
#endif

/* Clear Darwin 3 statistics */

      for (ir=RP->tree; ir; ir=ir->pnrp) {
         if (ir->Rp.krp & KRPNS) continue;
         for (il=ir->play1; il; il=il->play) {
            if (il->Ct.kctp & CTPNS) continue;
            memset((char *)il->CTST, 0, sizeof(struct CLSTAT));
            il->CTST->loscor = LONG_MAX;
            il->CTST->hiscor = -LONG_MAX;
            il->CTST->loascl = SI32_MAX;
            il->CTST->hiascl = -SI32_MAX;
            /* This area now includes distributions for all
            *  three classes of connection types.  */
            if (il->nctddst) memset((char *)il->pctddst, 0,
               sizeof(ddist) * il->nctddst);
            if (il->nctdist) memset((char *)il->pctdist, 0,
               sizeof(dist) * il->nctdist);
            /* (KRPHR is turned off in d3mchk if nmdlts == 0) */
            if (il->Ct.kctp & KRPHR)
               memset((char *)il->phrs, 0, sizeof(hist) * il->nmdlts);
#ifndef PAR0
            /* Clear XRM marks for all individual cells */
            if (ctexists(il,XRM)) {
               register rd_type    *g = il->prd + il->ctoff[XRM];
               register rd_type *gtop = il->prd + il->llt;
               register int   xrmsize = (int)il->lmm;
               for ( ; g < gtop; g += il->lel)
                  memset((char *)g, 0, xrmsize);
               }
#endif
            /* Clear pps area used to propagate modality responses */
            if (il->lmm) {
               int idly;
               for (idly=0; idly<(int)il->mxsdelay1; idly++)
                  memset((char *)il->pps[idly]+il->lspt, 0,
                     (size_t)il->lmm);
               }

            /* Clear conntype-level statistics */
            for (ix=il->pct1; ix; ix=ix->pct) {
               memset((char *)&ix->CNST, 0, sizeof(struct CNSTAT));
               if (ix->Cn.kam & KAMDS)
                  memset((char *)ix->pdas, 0, (size_t)ix->ldas);
               ix->nccat = 0;
               }

            }  /* End of for loop over il */
         }  /* End of for loop over ir */

/* Clear arrays needed for keeping track of stimulus groups */

      for (pmdlt=RP->pmdlt1; pmdlt; pmdlt=pmdlt->pnmdlt) {
         memset((char *)&pmdlt->um1.MS, 0, sizeof(struct MDLTSTAT));
         if (pmdlt->pxxxx)
            memset((char *)pmdlt->pxxxx, 0, pmdlt->ndsbyt);
#ifndef PARn
         /* Clear pgpno array only if it has been allocated, but
         *  not preloaded by GRPNO card(s) in d3grp3().  */
         if (pmdlt->ngpnt == 0) memset((char *)pmdlt->pgpno->grp,
            0, pmdlt->nds*sizeof(ui16));
#endif
         } /* End modality loop */

/* Clear arrays of KRPGP|KRPMG statistics */

      if (CDAT.ngstat > 0) {
         struct GPSTAT *pgp = CDAT.pgstat;
         struct GPSTAT *pgpe = CDAT.pgstat + CDAT.ngstat;
         size_t ltgstat = CDAT.ngstat * sizeof(struct GPSTAT);
         memset((char *)CDAT.pgstat, 0, ltgstat);
         for ( ; pgp<pgpe; ++pgp) {
            pgp->lohiresp = pgp->loresp = SI32_MAX;
            pgp->hiresp = -SI32_MAX;
            }
         }

      }  /* End if ! (RP->CP.runflags & RP_NOSTAT) */

/* Regeneration history (RGH) must be cleared for all individual
*  cells (if it exists) whether or not doing statistics */

#ifndef PAR0
   for (il=RP->pfct; il; il=il->pnct) {
      if (il->Ct.ctopt & OPTRG) {
         register rd_type    *g = il->prd + il->ctoff[RGH];
         register rd_type *gtop = il->prd + il->llt;
         for ( ; g < gtop; g += il->lel) *g = 0;
         } /* End if OPTRG */
      } /* End CELLTYPE loop */
#endif

   return;
   } /* End d3zsta */

