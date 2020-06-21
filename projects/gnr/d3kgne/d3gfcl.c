/* (c) Copyright 1997-2011, Neurosciences Research Foundation, Inc. */
/* $Id: d3gfcl.c 46 2011-05-20 20:14:09Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                               d3gfcl                                 *
*                                                                      *
*  This routine sets up final cell and sense lists for SIMDATA file    *
*  output.  It is executed the first time after d3tchk and before      *
*  d3allo, so it can request CTFPD and NSVAK storage.  It is called    *
*  again after d3grp3 to update cell lists, but not storage.           *
*                                                                      *
*  Argument:                                                           *
*     kg1      TRUE if called at Group I time, otherwise FALSE         *
*                                                                      *
*  The following operations are performed:                             *
*  (1) Check limits on cell and value lists.                           *
*  (2) Match requests for sense data with corresponding VCELL blocks   *
*      and match requests for user camera info with TVDEF block.       *
*  (3) Call d3clck to associate CLBLKs with CELLTYPEs, make sure no    *
*      requested cells are out of range, and sort into seqvmo order.   *
*  (4) Set or clear the GFSI svitms bit in each cell type according    *
*      to whether it has or does not have an explicit cell list.       *
*  (5) Copy a pointer to the linked cell list blocks to RP->pgdcl for  *
*      bdcst.  Copy GFNTC bit into RP->ksv for access from comp nodes. *
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
***********************************************************************/

#define CLTYPE struct CLBLK
#define SDSTYPE struct SDSELECT
#define TVTYPE struct TVDEF

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
#include "bapkg.h"

void d3gfcl(int kg1) {

   struct CELLTYPE *il;    /* Ptr to current cell type */
   struct CLHDR    *pclh;  /* Ptr to header of cell list */
   struct CLBLK    *pclb;  /* Ptr to a cell list block */
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
                  d3exit(BBD_ERR, NULL, pgsns->hgsnm);
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

/* If cell lists exist, associate CLBLKs with CELLTYPEs, check max
*  cell numbers, sort lists into CELLTYPE order, and request that
*  cell list be moved to shared memory.  */

   if (pgd->clid != 0 && (pclh = findcl(pgd->clid, TRUE))) {
      d3clck(pclh, NULL, CLCK_SORT);
      pclh->clflgs |= CLFL_SHARED;
      pgd->pgdcl = pclh->pfclb;
      }
   else
      pgd->pgdcl = NULL;

/* Copy pointer to cell lists to RP for broadcast.
*  Set or clear the GFSI svitms bit in each cell type according
*  to whether it has or does not have an explicit cell list.  */

   pclb = RP->pgdcl = pgd->pgdcl;
   for (il=RP->pfct; il; il=il->pnct) {
      while (pclb && pclb->clseq < il->seqvmo) pclb = pclb->pnclb;
      if (pclb && pclb->clseq == il->seqvmo) il->svitms |= GFSI;
      else                                   il->svitms &= ~GFSI;
      }

/* If this is Group I time, then for each cell type that has a cell
*  list (explicit or implicit), and for which file storage of AK
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
               } /* End CONNTYPE loop */
            } /* End if doing s(i) output */
         } /* End CELLTYPE loop */

      } /* End Group I allocation requests */

   } /* End d3gfcl */
