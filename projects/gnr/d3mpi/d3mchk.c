/* (c) Copyright 1995-2016, The Rockefeller University *11115* */
/* $Id: d3mchk.c 74 2017-09-18 22:18:33Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                              d3mchk.c                                *
*                                                                      *
*                  Postprocessing of MODALITY blocks                   *
*                        (must follow d3tchk)                          *
*                                                                      *
*  This routine performs the following actions:                        *
*  (1) Scan all cell types and locate or create MODALITY blocks for    *
*      connection types with implicit modalities.                      *
*  (2) Fill in modality request (rmdlts) and CTFXD (X,Y,Z,H,C,G,M      *
*      stats driven) bits in all cell types, propagating iteratively   *
*      to all connected target cell types.                             *
*  (3) Mark as USED all MODALITY blocks that are requested inputs      *
*      to cell types that have X,Y,Z,H,C, or G stats.  Mark as KAMC    *
*      all MODALITY blocks that are used to send stim group info to    *
*      d3go for categorization value.  Mark as CTFS all MODALITY       *
*      blocks that send new-stim-group infor to d3go for ctopt=C.      *
*  (4) Scan through all MODALITY blocks.  Set default ncs,nds for      *
*      blocks that need them.  (This information may come from un-     *
*      windowed parents of windowed modalities.)  Determine mxnds      *
*      over all modalities, as it may be used relative to any of them. *
*  (5) Accumulate total nds for all used modalities and be sure it     *
*      does not exceed SHRT_MAX such that celltype nCdist and all      *
*      mask offsets can be stored in shorts (seems generous).          *
*  (6) Scan through all CONNTYPE blocks of all cell types.  For those  *
*      that receive stimulus marking directly from the sensory broad-  *
*      cast, calculate the bit offset to this modality in XRM data.    *
*      But if somehow no modalities got marked for the celltype, turn  *
*      off all the statistics that depend on modality, e.g. to avoid   *
*      clearing non-existent arrays in d3zsta.                         *
*                                                                      *
*  The general philosophy for assigning modalities to conntypes is:    *
*  If 'MODALITY=NONE' was specified, no modality is ever assigned.     *
*  If the conntype is explicitly assigned a modality by the user,      *
*  that modality is used.  If the conntype receives input implicitly   *
*  from a sense or TV that had the XRSTATS option, that modality is    *
*  used.  If the conntype receives input directly from the input       *
*  array (possibly via a window), the built-in modality name VISION    *
*  is used.  If the conntype receives input from another repertoire,   *
*  the modalities that feed that repertoire are used.  Otherwise,      *
*  the connection type has no modality.                                *
*                                                                      *
*  Original version removed unused modalities.  This has been changed  *
*  to permit PRINT,PLOT and other cards to reference modalities even   *
*  if not used for cross-response statistics.                          *
************************************************************************
*  V8A, 05/15/95, GNR - Initial version                                *
*  Rev, 07/16/95, GNR - Delete code that removed unused modalities     *
*  Rev, 10/28/00, GNR - Remove XRH, tabulate KRPHR over all cycles     *
*  ==>, 12/27/07, GNR - Last mod before committing to svn repository   *
*  V8F, 05/15/10, GNR - Add marking for KRPGP statistics, correct bug  *
*                       that RQST bit incorrectly turned on for USED.  *
*  V8H, 11/06/10, GNR - Make fatal error if KRPXRM stats requested     *
*                       but no modality information is available.      *
*  Rev, 02/24/11, GNR - Add KAM=C for categorization models            *
*  Rev, 04/28/11, GNR - Add CTOPT=C for new-stim-group fast start--    *
*                       implies need for separate amdlts,rmdlts.       *
*  Rev, 07/28/12, GNR - Eliminate bit offsets and bitiors              *
*  Rev, 08/16/12, GNR - Use mdltbid to eliminate mdltno bit tests      *
*  R67, 04/27/16, GNR - Remove Darwin 1 support                        *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "sysdef.h"
#include "d3global.h"
#include "tvdef.h"
#include "rocks.h"
#include "rkxtra.h"
#include "bapkg.h"

/*---------------------------------------------------------------------*
*                               d3mchk                                 *
*---------------------------------------------------------------------*/

void d3mchk(void) {

   struct CELLTYPE *il,*jl;   /* Ptr to current CELLTYPE block */
   struct CONNTYPE *ix;       /* Ptr to current CONNTYPE block */
   struct MODALITY *pmdlt;    /* Ptr to current MODALITY block */

   ui32 sumnds;               /* Total ncs+nds bytes for broadcast */
   int kiter;                 /* Iterate modality propagation  */
   mdlt_type allmdlts = 0;    /* Mask bits for used modalities */

/* We can skip stages 1,2,3 if nobody has X,Y,Z,C,H,G STATS */

   if (!(RP->kxr & (MXSTATS|ISKAMC|ISOPTC)))
      goto NoMarkPropagation;

/* Scan all connection types, locating modalities where missing.
*  This is done regardless of whether X,Y,Z,C,H,G statistics
*  are requested for this celltype because we may need to pass
*  our responses through to other celltypes that do have stats.  */

   for (il=RP->pfct; il; il=il->pnct) {

      /* Turn on CTFXD bit if any form of statistics that
      *  requires modality marking is present--this will
      *  cause allocation of marking space (for all modalities,
      *  not just those that are actually entered in stats).  */
      if (il->Ct.kstat & KSTXRM)
         il->ctf |= CTFXD;

      for (ix=il->pct1; ix; ix=ix->pct) {

         /* Ignore this input if MODALITY=NONE was specified */
         if (ix->cnflgs & NOMDLT) continue;

         /* If no explicit modality was assigned,
         *  assign a default according to the input type */
         if (!(ix->cnflgs & OVMDLT)) switch (ix->cnsrctyp) {

         case REPSRC:         /* Input from a repertoire */
            /* The following is unnecessary now, but may save
            *  an iteration of the propagation loop below.  */
            jl = (struct CELLTYPE *)ix->psrc;
            il->amdlts |= jl->amdlts;
            break;

         case IA_SRC:         /* Input from the input array */
            /* Use the default "VISION" modality, which has
            *  MDLTFLGS_RQST implicitly set.  */
            ix->psmdlt = findmdlt("VISION", 0);
            break;

         case VS_SRC:         /* Scan window */
            ix->psmdlt = findmdlt("VISION", ix->srcnm.hbcnm);
            break;

         case TV_SRC:         /* Input from television */
            /* If the TV has a modality already assigned, and
            *  XRSTATS were requested for that modality, use it,
            *  otherwise leave this conntype unassociated. */
            {  struct TVDEF *ptv = (struct TVDEF *)ix->psrc;
               if (pmdlt = ptv->ptvmdlt) { /* Assignment intended */
                  if (pmdlt->mdltflgs & MDLTFLGS_RQST)
                     ix->psmdlt = pmdlt;
                  }
               } /* End TV_SRC local scope */
            break;

         case PP_SRC:         /* Input from an image preprocessor */
            /* Treat the same as the TV to which the preprocessor
            *  is attached.  */
            {  struct PREPROC *pip = (struct PREPROC *)ix->psrc;
               struct TVDEF *ptv = pip->pipcam;
               if (pmdlt = ptv->ptvmdlt) { /* Assignment intended */
                  if (pmdlt->mdltflgs & MDLTFLGS_RQST)
                     ix->psmdlt = pmdlt;
                  }
               } /* End PP_SRC local scope */
            break;

         case VALSRC:         /* Input from a value scheme */
         case VJ_SRC:         /* Input from joint kinesthesia */
         case VH_SRC:         /* Input from hand vision */
         case VW_SRC:         /* Window kinesthesia */
            /* These input types cannot have a modality */
            break;

         default:
            /* All senses not explicitly listed above (e.g. BBDS) are
            *  assumed to support assignment of a modality.  If the
            *  source sense has a modality already assigned, and
            *  XRSTATS were requested for that modality, use it,
            *  otherwise leave this conntype unassociated.  */
            {  struct VCELL *pvc = (struct VCELL *)ix->psrc;
               if (pmdlt = pvc->pvcmdlt) { /* Assignment intended */
                  if (pmdlt->mdltflgs & MDLTFLGS_RQST)
                     ix->psmdlt = pmdlt;
                  }
               } /* End default local scope */
            } /* End cnsrctyp switch */

         /* If a modality was assigned, now is the time to set the
         *  corresponding request bit in the CELLTYPE.  If KAMCG
         *  amplification was requested, set the MDLTFLGS_KAMC bit.  */
         if (pmdlt = ix->psmdlt) {  /* Assignment intended */
            il->amdlts |= pmdlt->mdltbid;
            if (ix->Cn.kam & KAMCG) pmdlt->mdltflgs |= MDLTFLGS_KAMC;
            }

         }  /* End conntype loop */
      }  /* End cell type loop */

/* At this point, modalities of all external connection sources
*  have been established.  Now propagate this information to all
*  repertoires that receive modality-associated input indirectly
*  via other repertoires.  Iterate until no further additions.
*  At the same time, mark all celltypes that require isn marking
*  solely to provide modality information to other celltypes.  */

   do {
      kiter = FALSE;
      for (il=RP->pfct; il; il=il->pnct) {
         mdlt_type old_amdlts = il->amdlts;
         for (ix=il->pct1; ix; ix=ix->pct) {
            if (ix->kgfl & KGNXR) continue;
            if (ix->cnflgs & (NOMDLT|OVMDLT)) continue;
            /* Input is from another repertoire, so propagate
            *  its source modalities into our cell type.  */
            jl = (struct CELLTYPE *)ix->psrc;
            il->amdlts |= jl->amdlts;
            /* Set CTFXD bit in source celltype if ours is set */
            if (il->ctf & ~jl->ctf & CTFXD)
               jl->ctf |= CTFXD, kiter = TRUE;
            }  /* End conntype loop */
         if (il->amdlts != old_amdlts) kiter = TRUE;
         }  /* End cell type loop */
      } while (kiter);

/* Mark as USED all MODALITY blocks that are requested inputs to cell
*  types that have H,X,Y,Z,C,G, or M stats. These may have been recog-
*  nized only as a result of the iterative propagation just completed.
*  This will cause these modalities to be included in the stimulus
*  broadcast.  VALUE ENVVAL is only other place that can mark USED.
*
*  Rev, 05/15/10, GNR - Remove test: only done if RQST bit set.
*  Rev, 04/28/11, GNR - Also set MDLTFLGS_CTFS for OPTCFS celltypes.
*/
   for (il=RP->pfct; il; il=il->pnct) {
      if (il->Ct.ctopt & OPTCFS) il->ctwk.tree.krmop |= KRM_MODND;
      for (pmdlt=RP->pmdlt1; pmdlt; pmdlt=pmdlt->pnmdlt) {
         if (il->amdlts & pmdlt->mdltbid) {
            if (il->ctf & CTFXD) pmdlt->mdltflgs |= MDLTFLGS_USED;
            if (il->Ct.ctopt & OPTCFS) {
               pmdlt->mdltflgs |= MDLTFLGS_CTFS;
               il->ctwk.tree.krmop &= ~KRM_MODND;
               }
            } /* End if modality feeds this cell type */
         } /* End modality loop */
      } /* End cell type loop */

/* Scan the MODALITY list, setting default ncs,nds.  Modalities that
*  are viewed through a window inherit these values from the parent
*  that is not associated with a window (if one exists).  Determine
*  maximum nds values over all modalities.  Accumulate sum of nds for
*  modalities used for XYZHCG stats plus ncs for modalities used for
*  KAMCG amplification plus 1 for modalities used for OPTCFS.  These
*  will be used to allocate space for stim and group masks in sense
*  broadcast.  Check that sumnds does not exceed SHRT_MAX, assuring
*  that offsets to all stim masks may be stored in short integers.  */

NoMarkPropagation:
   sumnds = 0;
   for (pmdlt=RP->pmdlt1; pmdlt; pmdlt=pmdlt->pnmdlt) {
      struct MODALITY *parent;   /* Ptr to parent MODALITY block */
      if ((pmdlt->mdltwdw > 0) && ((parent = findmdlt(
            getrktxt(pmdlt->hmdltname), -1)) != NULL)) {
         if (pmdlt->ncs  == 0) pmdlt->ncs  = parent->ncs;
         if (pmdlt->nds  == 0) pmdlt->nds  = parent->nds;
         if (pmdlt->ntcs > SHRT_MAX) pmdlt->ntcs = parent->ntcs;
         if (pmdlt->ntrs > SHRT_MAX) pmdlt->ntrs = parent->ntrs;
         }
      /* Even the parent may not have specified ncs,nds */
      if (pmdlt->ncs == 0) pmdlt->ncs = RP0->ncs;
      if (pmdlt->nds == 0) pmdlt->nds = RP0->nds;
      if (pmdlt->nds > RP->mxnds) RP->mxnds = pmdlt->nds;
      /* Calculate number of bytes needed to store ncs,nds bits */
      pmdlt->ncsbyt = BytesInXBits(pmdlt->ncs);
      pmdlt->ndsbyt = BytesInXBits(pmdlt->nds);
      /* Calculate total space and offsets of this modality in
      *  sense data broadcast.  */
      if (pmdlt->mdltflgs & MDLTFLGS_USED) {
         pmdlt->mdltboff = sumnds;
         sumnds += pmdlt->ndsbyt;
         allmdlts |= pmdlt->mdltbid;
         }
      if (pmdlt->mdltflgs & MDLTFLGS_KAMC) {
         pmdlt->mdltgoff = sumnds;
         sumnds += pmdlt->ncsbyt;
         }
      if (pmdlt->mdltflgs & MDLTFLGS_CTFS) {
         pmdlt->pignc = (stim_mask_t *)callocv(2, pmdlt->ncsbyt,
            "CTOPT=C check array");
         pmdlt->mdltnoff = sumnds;
         sumnds += 1;
         }
      } /* End modality loop */

   if (sumnds > SHRT_MAX)
      d3exit("distinct stimuli", LIMITS_ERR, SHRT_MAX);
   RP->cumbcst += RP->cumnds = sumnds;

/* Scan all cell types once again.  Save amdlts for OPTCFS, etc.,
*  but remove from rmdlts bits referring to modalities that are not
*  used for stats.  For all connection types of each cell type that
*  receive stimulus information directly from the sensory broadcast,
*  calculate and store the offset where marking bits should be stored
*  in the celltype's XRM data for that conntype's modality.  This is
*  needed regardless of whether cell type has XYZHCGM stats.  Also,
*  set the DIMDLT bit in cnflgs to indicate this case.  */

   for (il=RP->pfct; il; il=il->pnct) {
      /* Clean out bits for modalities that are not used */
      il->rmdlts = il->amdlts & allmdlts;
      il->nmdlts = (short)bitcnt((unsigned char *)&il->rmdlts,
         (long)sizeof(mdlt_type));
      if (il->nmdlts == 0) {
         if (il->Ct.kstat & KSTXRM) {
            cryout(RK_E1, "0***C,G,H,M,X,Y, OR Z STATS REQUESTED FOR ",
               RK_LN3, fmturlnm(il), LXRLNM, "    BUT NO MODALITY "
               "INFORMATION WAS FOUND", RK_LN0, NULL);
            il->Ct.kstat &= ~KSTXRM;    /* Kill other checking */
            }
         }
      /* Loop over conntypes, check for those with direct input */
      else for (ix=il->pct1; ix; ix=ix->pct) {
         /* Ignore this input if MODALITY=NONE was specified or
         *  no modality was assigned (because globally not used) */
         if (ix->cnflgs & NOMDLT || !ix->psmdlt) continue;
         /* If not from a repertoire, find pmbits offset */
         if (ix->kgfl & KGNXR || ix->cnflgs & OVMDLT) {
            ui32 mboff = 0;
            byte mymdltno = ix->psmdlt->mdltno;
            ix->cnflgs |= DIMDLT;
            /* This loop is OK even if modality list is sorted to
            *  other than number order--not currently done. */
            for (pmdlt=RP->pmdlt1; pmdlt; pmdlt=pmdlt->pnmdlt)
               if ((il->rmdlts & pmdlt->mdltbid) &&
                  pmdlt->mdltno < mymdltno) mboff += pmdlt->ndsbyt;
            ix->mdltmoff = mboff;
            } /* End input not from a repertoire */
         }  /* End conntype loop */
      /* Perform separate error check for KAMGC amplification and
      *  no modality info was found (independent of il->nmdlts).
      *  (If ix->psmdlt exists, MDLTFLGS_KAMC was set above, so
      *  no need to check that condition here).  */
      for (ix=il->pct1; ix; ix=ix->pct) {
         if (ix->Cn.kam & KAMCG && !ix->psmdlt) {
            cryout(RK_E1, "0***AMP RULE C REQUESTED FOR ", RK_LN3,
               fmturlnm(il), LXRLNM, " BUT NO MODALITY "
               "INFORMATION WAS FOUND", RK_LN0, NULL);
            ix->Cn.kam &= ~KAMCG;
            }
         } /* End conntype loop */
      /* And last but not least, error check for OPTCFS */
      if (il->ctwk.tree.krmop & KRM_MODND) {
         cryout(RK_E1, "0***CTOPT=C REQUESTED FOR ", RK_LN3,
            fmturlnm(il), LXRLNM, " BUT NO MODALITY "
            "INFORMATION WAS FOUND", RK_LN0, NULL);
         il->Ct.ctopt &= ~OPTCFS;
         }
      }  /* End cell type loop */

   } /* End d3mchk */
