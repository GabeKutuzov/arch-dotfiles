/* (c) Copyright 1991-2018, The Rockefeller University *21119* */
/* $Id: d3zsta.c 79 2018-08-21 19:26:36Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                               d3zsta                                 *
*      Zero all statistical accumulators for a series of trials.       *
*                                                                      *
*                               d3zhls                                 *
*     Initialize the phlsi array for high/low response collection      *
*                                                                      *
************************************************************************
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
*  Rev, 12/25/12, GNR - Erase global overflow counts                   *
*  R67, 04/27/16, GNR - Remove Darwin 1 support                        *
*  R70, 01/25/17, GNR - Add d3zhls code to initialize phlsi            *
*  R78, 05/12/18, GNR - Add HILOSI mechanism to gather hiresp,loresp   *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "rkarith.h"
#include "d3global.h"
#include "celldata.h"
#include "statdef.h"
#include "bapkg.h"

extern struct CELLDATA CDAT;

#ifdef PAR
/*=====================================================================*
*                               d3zhls                                 *
*  (Called from d3zsta at start of trial series and from d3go after    *
*  every set of nhlssv trials.)                                        *
*=====================================================================*/

void d3zhls(void) {

   struct HILOSI *phls = CDAT.phlsi;
   struct HILOSI *phlse = phls + RP->nhlsi;
   for ( ; phls<phlse; ++phls) {
      phls->nstim = 0;
      phls->nhits = 0;
      phls->hiresp = -SI32_MAX;
      phls->loresp = SI32_MAX;
      }
   CDAT.ihlsi = 0;
   } /* End d3zhls() */
#endif


/*=====================================================================*
*                               d3zsta                                 *
*=====================================================================*/

void d3zsta(void) {

   struct REPBLOCK  *ir;   /* The traditional pointers */
   struct CELLTYPE  *il;
   struct MODALITY  *pmdlt;

   if (!(RP->CP.runflags & (RP_NOSTAT|RP_NOCYST))) {

      for (ir=RP->tree; ir; ir=ir->pnrp) {
         if (ir->Rp.krp & KRPNS) continue;
         for (il=ir->play1; il; il=il->play) {
            struct CONNTYPE  *ix;

            /* Omit performance statistics if switched off */
            if (il->Ct.kstat & KSTNS) continue;
            memset((char *)il->CTST, 0, sizeof(struct CLSTAT));
            il->CTST->loscor = LONG_MAX;
            il->CTST->hiscor = -LONG_MAX;
#ifndef PARn
            /* Autoscale stats are counted on PAR0, no gsta needed.
            *  If autoscales are not being updated, clear only first
            *  time through for d3stat() printing.  (Tests are left
            *  inside this infrequent loop to save code space.)  */
            if (il->nauts) {
               struct ASSTAT *psst = (struct ASSTAT *)(
                  (char *)il->CTST + sizeof(struct CLSTAT));
               struct ASSTAT *psste = psst + il->nauts;
               for ( ; psst<psste; ++psst) {
                  psst->ncaps = jesl(0);
                  if (il->kautu & KAUT_UOK)
                     psst->loascl = SI32_MAX, psst->hiascl = -SI32_MAX;
                  else if (RP->CP.effcyc == 0)
                     psst->loascl = psst->hiascl = S24;
                  }
               }
#endif
            /* This area now includes distributions for all
            *  three classes of connection types.  */
            if (il->nctddst) memset((char *)il->pctddst, 0,
               sizeof(ddist) * il->nctddst);
            if (il->nctdist) memset((char *)il->pctdist, 0,
               sizeof(dist) * il->nctdist);
            /* (KRPHR is turned off in d3mchk if nmdlts == 0) */
            if (il->Ct.kstat & KSTHR)
               memset((char *)il->phrs, 0, 2*sizeof(hist)*il->nmdlts);
#ifndef PAR0
            /* Clear XRM marks for all individual cells */
            if (ctexists(il,XRM)) {
               register rd_type    *g = il->prd + il->ctoff[XRM];
               register rd_type *gtop = il->prd + uw2zd(il->llt);
               size_t llel = uw2zd(il->lel);
               register int   xrmsize = (int)il->lmm;
               for ( ; g < gtop; g += llel)
                  memset((char *)g, 0, xrmsize);
               }
#endif

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

/* Clear arrays of KSTGP|KSTMG statistics */

#ifndef PARn
      if (CDAT.ngstat > 0) {
         struct GPSTAT *pgp = CDAT.pgstat;
         struct GPSTAT *pgpe = CDAT.pgstat + CDAT.ngstat;
         for ( ; pgp<pgpe; ++pgp) {
            pgp->nstim = pgp->nhits = 0;
            pgp->lohiresp = pgp->loresp = SI32_MAX;
            pgp->hiloresp = pgp->hiresp = -SI32_MAX;
            }
         }
#endif

      }  /* End if ! (RP->CP.runflags & RP_NOSTAT) */

/* Clear overflow counts whether or not doing statistics */

   memset((char *)CDAT.povec, 0, RP->novec*sizeof(*CDAT.povec));

/* Regeneration history (RGH) must be cleared for all individual
*  cells (if it exists) whether or not doing statistics */

#ifndef PAR0
   for (il=RP->pfct; il; il=il->pnct) {
      if (il->Ct.ctopt & OPTRG) {
         register rd_type    *g = il->prd + il->ctoff[RGH];
         register rd_type *gtop = il->prd + uw2zd(il->llt);
         size_t llel = uw2zd(il->lel);
         for ( ; g < gtop; g += llel) *g = 0;
         } /* End if OPTRG */
      } /* End CELLTYPE loop */
#endif

/* Initialize for collecting high/low responses on parallel machine */

#ifdef PAR
   d3zhls();
#endif

   return;
   } /* End d3zsta */

