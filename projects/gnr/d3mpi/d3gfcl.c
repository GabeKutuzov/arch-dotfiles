/* (c) Copyright 1997-2018, The Rockefeller University *11115* */
/* $Id: d3gfcl.c 77 2018-03-15 21:08:14Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                               d3gfcl                                 *
*                                                                      *
*  This routine sets up final cell and sense lists for SIMDATA file    *
*  output.  It is executed the first time after d3tchk and before      *
*  d3allo, so it can request CTFPD, NSVAK, and NSVRAW storage.  It is  *
*  called again after d3grp3 to update cell lists, but not storage.    *
*                                                                      *
*  Argument:                                                           *
*     kg1      TRUE if called at Group I time, otherwise FALSE         *
*                                                                      *
*  The following operations are performed:                             *
*  (1) Check limits on object and value lists.                         *
*  (2) Match requests for sense data with corresponding VCELL blocks   *
*      and match requests for user camera info with TVDEF block.       *
*  (3) For each celltype, get a pointer to the correct cell list, if   *
*      any, in il->psdclb according to priority (i) CELLTYPE CLIST,    *
*      (ii) CELLTYPE SVITMS=G, (iii) SAVE RESPONSES, (iv) SAVE CLIST   *
*      that names this cell type.                                      *
*  (4) Call d3clck to check that this CLBLK has no cells out-of-range. *
*  (5) Copy GFNTC bit into RP->ksv for access from comp nodes.         *
*  (6) Set request bits in CELLTYPEs for storage of PHD and in         *
*      CONNTYPEs for storage of AK.                                    *
*                                                                      *
************************************************************************
*  V8A, 08/27/97, GNR - New routine                                    *
*  V8B, 03/02/01, GNR - Add checking of values and objects lists       *
*  Rev, 08/30/01, GNR - Remove RP0->mxobj updating                     *
*  V8D, 01/31/08, GNR - Add sense data checking                        *
*  ==>, 02/05/08, GNR - Last mod before committing to svn repository   *
*  V8H, 11/08/10, GNR - Add SNSID checking                             *
*  Rev, 05/06/11, GNR - Use 'R' for arm, 'J' for Sj                    *
*  R72, 02/23/17, GNR - Now allowing individual CELLTYPE SDCLIST       *
*  R77, 02/08/18, GNR - Add NSVRAW setting                             *
***********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include "sysdef.h"
#include "d3global.h"
#include "d3fdef.h"
#include "clblk.h"
#include "tvdef.h"
#include "simdata.h"
#include "rocks.h"
#include "rkxtra.h"
#include "bapkg.h"

void d3gfcl(int kg1) {

   struct CELLTYPE *il;    /* Ptr to current cell type */
   struct CLHDR    *pclh;  /* Ptr to header of cell list */
   struct SDSELECT *pgd;   /* Ptr to selection block being checked */
   ui32            lksv;   /* Local copy of SAVE item request bits */

   if (!RP0->pgds) return; /* No SIMDATA, nothing to do */

   pgd = RP0->pgds;
   lksv = pgd->svitms;

/* If objects or values save lists have been entered, call ilstchk()
*  to complete option processing, check or update largest items.
*  Checked again in d3gfsh() to catch Group III changes.  */

   if (kg1 && pgd->pilobj)
      ilstchk(pgd->pilobj, IL_MASK, "SIMDATA object list");
   if (kg1 && pgd->pilval)
      ilstchk(pgd->pilval, (long)RP->nvblk, "SIMDATA value list");

/* If Group III, additional senses may have been added to the
*  SIMDATA request lists already processed in d3schk for Group I.
*  Now is the time to match up these requests with corresponding
*  VCELL blocks.  Generate an error if the VCELL block has not
*  already been marked VKUSED--Group III is too late to allocate
*  space for and configure sensors.  Same applies for cameras
*  requested for UTVINFO output.  */

   if (!kg1) {
      int ilist;                 /* Control to check two lists */
      for (ilist=PGD_SENSE; ilist<=PGD_SNSID; ++ilist) {
         struct GDSNS *pgsns = pgd->pgdsns[ilist];
         for ( ; pgsns; pgsns=pgsns->pngsns) {
            if (!pgsns->pvsns) {
               if (pgsns->vcname[0] == 0) {  /* External sense */
#ifdef BBDS
                  pgsns->pvsns = findvc("BBD", USRSRC, pgsns->hgsnm,
                     "FOR SIMDATA OUTPUT");
#else
                  d3exit(NULL, BBD_ERR, pgsns->hgsnm);
#endif
                  }
               else {                        /* Built-in sense */
                  pgsns->pvsns = findvc(pgsns->vcname, REPSRC,
                     pgsns->hgsnm, "for SIMDATA output"); }
               }
            if (pgsns->pvsns) {
               if (!(pgsns->pvsns->vcflags & VKUSED))
                  cryout(RK_E1, "0***", RK_LN3+4, fmtvcn(pgsns->pvsns),
                     RK_CCL, " NOT IN USE--", RK_CCL,
                     "       MUST REQUEST SIMDATA SAVE IN GROUP I.",
                     RK_LN0, NULL);
               }
            else  /* Prevent seg fault in d3gfsh */
               pgd->svitms &= (ilist == PGD_SENSE) ? ~GFSNS : ~GFSID;
            }
         } /* End ilist loop */
      if (lksv & GFUTV) {
         pgd->pgdcam = findtv(pgd->hcamnm, "FOR SIMDATA INFO");
         if (pgd->pgdcam && !(pgd->pgdcam->utv.tvsflgs & TV_ON))
            cryout(RK_E1, "0***CAMERA \"", RK_LN3,
               getrktxt(pgd->hcamnm), RK_CCL, "\" NOT IN USE--",
               RK_CCL, "       MUST REQUEST SIMDATA SAVE IN GROUP I.",
               RK_LN0, NULL);
         }
      } /* End Group III checks */

/* Pass "internal cycles" flag to comp nodes */

   if (lksv & GFNTC) RP->ksv |= KSDNC;
   else              RP->ksv &= ~KSDNC;

/* Loop over celltypes.  Assign cell list according to priorities
*  documented above, then, if there is a cell list, call d3clck to
*  validate it.  Otherwise, GFGLBL will be set, or this cell type
*  will not be participating at all in SIMDATA.  Note that this
*  assignment is repeated for each new CYCLE card and an old cell
*  list may no longer be used, but is not removed in case used
*  elsewhere.  There is currently no way to build a negative clnum
*  on a CELLTYPE SDCLIST, so no need to check for removing those.  */

   for (il=RP->pfct; il; il=il->pnct) {
      il->psdclb = NULL;      /* Remove any previous cell list */
      il->svitms &= ~GFSI;    /* And turn off saving this cell type */
      pclh = NULL;            /* And indicate no new cell list */
      if (il->ctclloc[CTCL_SIMDAT].clnum)
         pclh = findcl(&il->ctclloc[CTCL_SIMDAT], TRUE);
      else if ((il->svitms | pgd->svitms) & GFGLBL)
         il->svitms |= GFSI;
      else if (pgd->sdclloc.clnum) {
         pclh = findcl(&pgd->sdclloc, TRUE);
         }
      if (pclh) {
         d3clck(pclh, il, CLCK_SIMDAT);
         }
      } /* End cell type loop */

/* If this is Group I time, then for each cell type that has a cell
*  list (explicit or implicit), and for which file storage of AK, RAW
*  (afference) or PHD (phi distribution) is requested, set bit to
*  request d3allo to allocate these items.  At Group III time, it
*  is too late to allocate, and d3gfsh will ignore these requests.  */

   if (kg1) {
      struct CONNTYPE *ix;
      ui32 lctsv, lcnsv;

      for (il=RP->pfct; il; il=il->pnct) {
         lctsv = lksv | il->svitms;
         if (lctsv & (GFGLBL|GFSI)) {
            if (lctsv & GFDIS && il->phshft) il->ctf |= CTFPD;
            for (ix=il->pct1; ix; ix=ix->pct) {
               lcnsv = lctsv | ix->svitms;
               if (lcnsv & GFAFF) ix->cnflgs |= NSVAK;
               if (lcnsv & GFRAW) ix->cnflgs |= NSVRAW;
               } /* End CONNTYPE loop */
            } /* End if doing s(i) output */
         } /* End CELLTYPE loop */

      } /* End Group I allocation requests */

   } /* End d3gfcl */
