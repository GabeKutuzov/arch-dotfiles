/* (c) Copyright 1995-2012, The Rockefeller University *11115* */
/* $Id: d3mset.c 70 2017-01-16 19:27:55Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                               d3mset                                 *
*                                                                      *
*  Routines used to allocate storage on all nodes for collecting       *
*  cross-response (XSTATS) and extra group (GSTATS) statistics.        *
*                                                                      *
*  mkgrpdef()  Allocate a GRPDEF structure and space for nds group     *
*              number entries.  Call only from host node.  This is     *
*              done once per modality at setup (d3msgl) time, and      *
*              again from d3grp3() when a new GRPNO card is read.      *
*              This space is never freed, but can be written over.     *
*  d3msgl()    Does global (GRPNO-independent) allocations.            *
*              Call once from every node before first CYCLE card.      *
*  d3msgr()    Does GRPNO-dependent allocations that may change        *
*              each time Group III cards have been read.  Call         *
*              once from every node each time a CYCLE card is read.    *
*                                                                      *
*  (These arrays are placed in private node storage because they do    *
*  not require broadcasting.  The basic idea is to allocate all the    *
*  space needed to handle the X,G STATS for one cell type.  The space  *
*  is reused in turn for each cell type.  This makes malloc() failure  *
*  unlikely just when printing statistics at the end of a long run.    *
************************************************************************
*  V8A, 06/25/95, GNR - Broken out from darwin3.c to handle modality   *
*  Rev, 10/28/00, GNR - Remove XRH, tabulate KRPHR over all cycles     *
*  ==>, 02/17/05, GNR - Last mod before committing to svn repository   *
*  V8F, 07/12/10, GNR - Modify to handle GSTATS                        *
*  Rev, 11/13/10, GNR - But fix:  Allocate all msgr stuff on node 0    *
*  V8H, 06/27/12, GNR - Change GST to ui64, use xrmt,xstt types        *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "d3global.h"
#include "statdef.h"
#include "rocks.h"

#ifndef PARn
/*---------------------------------------------------------------------*
*                             mkgrpdef()                               *
*                                                                      *
*  GRPDEF blocks are used to hold GRPNO data prior to setting up       *
*  GRPMSK arrays.  This code moved here from main pgm in V8A, so       *
*  that same routine can be used when GRPNO card read in d3grp3()      *
*  as in initial setup.  mallocdv() is now plain mallocv().            *
*---------------------------------------------------------------------*/

struct GRPDEF *mkgrpdef(int nds) {

   struct GRPDEF *pgd = (struct GRPDEF *)
      mallocv(sizeof(struct GRPDEF), "GRPDEF block");
   pgd->grp = (short *)mallocv(nds*sizeof(short), "grpnos");
   pgd->pnxgrp = NULL;
   return pgd;
   } /* End mkgrpdef */
#endif

/*---------------------------------------------------------------------*
*                              d3msgl()                                *
*                                                                      *
*  Clear STAT struct, allocate pxrmtx, pxcyc, pxxxx, pnr, pgpno arrays *
*---------------------------------------------------------------------*/

void d3msgl(void) {

   struct MODALITY *pmdlt;

   memset((char *)&STAT, 0, sizeof(struct STDEF));

/* Allocate cross-response/correlation matrix (host and serial) */

   if (RP->kxr & (YSTATS|ZSTATS)) {
      size_t zmxn;
      ui32 tmxn = (ui32)RP->mxnds;
      tmxn = tmxn*(tmxn+1)>>1;   /* Cannot overflow, mxnds is ui16 */
      zmxn = jmuzjd((size_t)tmxn, JSIZE);
      STAT.pxrmtx = (ui32 *)mallocv(zmxn, "Cross-response matrix");
      }

#ifndef PARn
/* All modalities get a pxcyc whether or not XSTATS are requested.
*  This area is also used for print/plot stimulus number controls.  */

   for (pmdlt=RP->pmdlt1; pmdlt; pmdlt=pmdlt->pnmdlt) {
      pmdlt->pxcyc = (stim_mask_t *)mallocv
         (pmdlt->ndsbyt*sizeof(stim_mask_t),"Cycle stim usage array.");
      } /* End modality loop */
#endif

   if (RP->kxr & MXSTATS) {

/* Allocate pxxxx and pnr.  These arrays are used to record which
*  stimuli have actually been presented during each trial series
*  and the numbers of cells responding to each.  There is one pnr
*  and one pxxxx array for each modality block.  Since this is a
*  once-only allocation, we don't bother consolidating the mallocs.
*/

      for (pmdlt=RP->pmdlt1; pmdlt; pmdlt=pmdlt->pnmdlt) {
         pmdlt->pxxxx = (stim_mask_t *)mallocv
            (pmdlt->ndsbyt*sizeof(stim_mask_t),"Stim usage arrays.");
         pmdlt->pnr = (xrmt *)mallocv
            (pmdlt->nds*sizeof(xrmt),"Stim response counts.");
#ifndef PARn
/* On host only, allocate GRPDEFS if MXSTATS */
         pmdlt->pgpno = mkgrpdef((int)pmdlt->nds);
         pmdlt->ngpnt = 0;
#endif
         } /* End modality loop */

      } /* End if MXSTATS */

   } /* End d3msgl() */

/*---------------------------------------------------------------------*
*                              d3msgr()                                *
*                                                                      *
*  Allocate arrays used for cross-response statistics that may need    *
*  to be updated if new GRPNO cards are read at d3grp3 time:           *
*  (1) pmdlt->pgsd:  Ptr to array of ncs*ngpnt GSDEF structs.  For     *
*                    each GRPNO card, up to ncs stimulus categories    *
*                    can be defined.  GSDEFS define these categories   *
*                    and count hits in response to each.  Code must    *
*                    be careful to increment by ncs when going to a    *
*                    new group, even if fewer than ncs classes found.  *
*  (2) pgsd->grpmsk: One bit array of nds bits for each GSDEF struct,  *
*                    used to keep track of which stimuli belong to     *
*                    each stimulus class.  This is a pointer into the  *
*                    grpblk info described in the next item.           *
*  (3) STAT.pgrpblk: Area for broadcasting GRPNO information to nodes, *
*                    consisting of ngpntt counters (pgrpncl) giving    *
*                    actual number of classes found on each GRPNO card *
*                    (may be <= ncs), followed by one grpmsk of length *
*                    ndsbyt bytes for each of ncs*ngpnt possible stim  *
*                    classes associated with each modality.            *
*                                                                      *
*  This stuff always goes on node 0, and on all nodes if X,Y,Z,H stats *
*  are present.  It is used only at d3stat time.                       *
*  Allocation follows the d3grp3 call because a user is permitted to   *
*  add more GRPNO cards during the course of a run.  However, alloc is *
*  done before starting the series to avoid allocation failure if done *
*  at d3stat time.  Note that reallocation is only necessary if the    *
*  number of tables increases.  See "statdef.h" for explanation of     *
*  cross-response data structures.                                     *
*---------------------------------------------------------------------*/

void d3msgr(void) {

   struct MODALITY *pmdlt;
   struct GSDEF *jgs;         /* Ptrs for distributing */
   byte *jgrpmsk;             /* allocations to modalities */

   short mgsblk;              /* Number of GSDEFs needed */
   long mgrpblk;              /* Size of grpblk needed */

#ifdef PARn
   if (!(RP->kxr & (XSTATS|YSTATS|ZSTATS|HSTATS))) return;
#endif

/* Count total number of group masks needed */

   STAT.ngpntt = 0;
   mgsblk = 0, mgrpblk = 0;
   for (pmdlt=RP->pmdlt1; pmdlt; pmdlt=pmdlt->pnmdlt) {
      int mgpnt = NStimGrps(pmdlt);     /* Number GRPNO cards */
      STAT.ngpntt += mgpnt;
      mgsblk  += mgpnt * pmdlt->ncs;
      mgrpblk += mgpnt * (sizeof(short) + pmdlt->ncs*pmdlt->ndsbyt);
      }

   /* Allocate (or release and reallocate) GRPBLK info */
   if (mgrpblk > STAT.lgrpblk) {
      if (STAT.lgrpblk > 0) freev(STAT.pgrpblk, "Group masks");
      STAT.pgrpblk = (byte *)mallocv(mgrpblk,"Group masks");
      STAT.lgrpblk = mgrpblk;
      }
   STAT.pgrpncl = (ui16 *)STAT.pgrpblk;
   STAT.pgrpmsk = STAT.pgrpblk + STAT.ngpntt*sizeof(ui16);

   /* Allocate (or release and reallocate) GSDEF structs */
   if (mgsblk > STAT.ngsblk) {
      if (STAT.ngsblk > 0) freev(STAT.pgsblk, "Stim Group Defs");
      STAT.pgsblk = (struct GSDEF *)mallocv(mgsblk*
         sizeof(struct GSDEF),"Stim Group Defs");
      STAT.ngsblk = mgsblk;
      }

   /* Distribute the storage that has been allocated among
   *  all the MODALITY and GSDEF structures.  */
   jgs = STAT.pgsblk;
   jgrpmsk = STAT.pgrpmsk;
   for (pmdlt=RP->pmdlt1; pmdlt; pmdlt=pmdlt->pnmdlt) {
      int i, j;
      int mgpnt = NStimGrps(pmdlt);    /* Number GRPNO cards */
      pmdlt->pgsd = jgs;
      for (i=0; i<mgpnt; i++) {
         for (j=0; j<pmdlt->ncs; j++,jgs++) {
            jgs->grpmsk = jgrpmsk;
            jgrpmsk += pmdlt->ndsbyt; }
         } /* End GRPNO loop */
      } /* End MODALITY loop */

   } /* End d3msgr() */

