/* (c) Copyright 1994-2010, Neurosciences Research Foundation, Inc. */
/* $Id: d3regenr.c 40 2010-11-03 16:02:24Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                              d3regenr                                *
*                                                                      *
*  Regenerate repertoires.  This routine is called at the end of each  *
*     trial series.  For all celltypes having OPT=R, and for each cell *
*     that did not respond to any stimuli during the preceding trial   *
*     series, it regenerates a new set of cell and synaptic variables, *
*     thus effectively replacing the cell with a new one based on the  *
*     same generating options.  (Connection data read from an external *
*     file are not affected.)  This code is very similar to that in    *
*     d3genr(), and should be maintained appropriately in parallel.    *
*                                                                      *
*  N.B.  Just because a cell is regenerated, we do not clear out the   *
*     modality markings en route in axonal delay, mostly because there *
*     is just one set for all cells, and most cells are not changed.   *
*                                                                      *
*  Written by G. N. Reeke                                              *
*  V7B, 06/22/94, GNR - Initial version, based on d3genr()             *
*  V8A, 05/26/95, GNR - Clear modality markers when s(i) is cleared,   *
*                       fix bug: was zeroing s(i) always for cell 0,   *
*                       use RGH for response test rather than XRM,     *
*                       remove "LTP", add DST                          *
*  Rev, 04/26/97, GNR - Set to always condense connection data         *
*  Rev, 09/28/97, GNR - Independent dynamic allocations per conntype   *
*  Rev, 02/24/98, GNR - Add resting potential, init noise and phase    *
*  V8C, 03/22/03, GNR - Cell responses in millivolts                   *
*  V8D, 08/18/05, GNR - Add conductances and ions                      *
*  Rev, 07/01/06, GNR - Move Lij, Cij, Dij generation to new routines  *
*  ==>, 12/29/07, GNR - Last mod before committing to svn repository   *
*  V8E, 01/20/09, GNR - Add Izhikevich variables, PHASEDEF block,      *
*                       remove addition of vrest to initial s(i)       *
*  Rev, 09/07/09, GNR - Add Brette-Gerstner (aEIF) neurons             *
*  V8F, 07/24/10, GNR - Make initsi a cell-at-a-time routine           *
*  V8H, 10/20/10, GNR - Add xn (once per conntype) variable allocation *
***********************************************************************/

#define LIJTYPE struct LIJARG

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "rkarith.h"
#include "d3global.h"
#ifndef PARn
#include "rocks.h"
#endif

extern struct LIJARG Lija; /* Arguments for Lij routines */

void d3regenr(void) {

   struct CELLTYPE *il;
   struct CONNTYPE *ix;
   long nskip;             /* Number of seeds to skip over */

#ifndef PAR0
   struct REPBLOCK *ir;
   struct IONTYPE  *pion;
   rd_type *plsd;          /* Ptr to "ls" data for current cell */
   rd_type *pnuk;          /* Ptr to place to store NUK */

   size_t lmvi;            /* Length of ion conc copy */

   long  celllim;          /* Cell counter limit */
   long  icell;            /* Cell counter */
   long  icellsp;          /* = icell * lsp */
   long  wnsd,wpsd;        /* Working noise and phase seeds */

   int   idly;             /* Delay counter */
   int   iki;              /* Kind of ion block */
   int   zero = 0;         /* Where zero needed as a lvalue */

   int   offCij,offCij0;   /* Offsets of Cij and Cij0 */
   int   offLij,offDij;    /* Offsets of Lij and Dij  */
   int   offMij;           /* Offset  of Mij          */
   int   offPPF,offPPFT;   /* Offsets of PPF and PPFT */
   int   offIONC;          /* Offset of ion conc in cell data */
   int   offRGH;           /* Offset of RGH in cell data */
#endif

/*---------------------------------------------------------------------*
*                      d3regenr executable code                        *
*---------------------------------------------------------------------*/

/* Loop over all celltypes (layers) */

   for (il=RP->pfct; il; il=il->pnct) {

/*---------------------------------------------------------------------*
*                        Initialize constants                          *
*---------------------------------------------------------------------*/

/* Skip over this cell type if doing a replay and KSVNR (no
*  generation) specified.  Also skip if no cell or connection
*  data exists, or if OPTRG (regeneration) was not specified.  */

      if (!(il->Ct.ctopt & OPTRG) || !il->lel || RP->ksv & KSVNR)
         continue;

#ifndef PAR0
      ir = il->pback;
#endif

      /* Set globals for initsi() */
      setinsi(il, 0, il->mxsdelay1);

      /* Save noise and phase seeds so can update later for all cells */
      wnsd = il->nsd;
      wpsd = il->pctpd->phiseed;

/* Update Izhikevich randomization seed, cseed, dseed, and lseed for a
*  new cell lifetime on all nodes (so will survive later broadcasts).
*  Note that cseed and lseed are updated more than needed in certain
*  cases, but this is simpler and OK as long as it is deterministic.
*/
      if (il->ctf & CTFRANOS) {
         struct IZHICOM *piz = (struct IZHICOM *)il->prfi;
         udevskip(&piz->izseed, il->nizvar*il->nelt);
         }
      for (ix=il->pct1; ix; ix=ix->pct) {
         nskip = il->nelt*ix->nc;
         if (ix->kgen & (KGNRC|KGNLM))
            udevskip(&ix->cseed, nskip);
         if (ix->Cn.kdelay > DLY_CONST)
            udevskip(&ix->Cn.dseed, nskip);
         udevskip(&ix->lseed, il->nelt*ix->uplij);
         } /* End conntype loop */

/*---------------------------------------------------------------------*
*                           Loop over cells                            *
*                Actually generate the repertoire data                 *
*----------------------------------------------------------------------*
*  Note:  Organization of code for regeneration raises challenging     *
*  problems.  Code prior to V8D for Cij and Lij generation assumed     *
*  cells would be generated in serial order once setup was complete.   *
*  With many, perhaps a majority, of cells being skipped during regen, *
*  one may choose (a) to reinitialize for every individual cell,       *
*  (b) to carry out most of the steps of generation but omit storing   *
*  the results for responsive (preserved) cells, or (c) redesign all   *
*  the generating routines to accept a cell number as argument and     *
*  remove the serial presumption. Option (c) has now been implemented. *
*---------------------------------------------------------------------*/

#ifndef PAR0
/*** BEGIN NODE ZERO EXCLUSION FROM LOOP OVER RESIDENT CELLS ***/

      d3kij(il, KMEM_REGEN);  /* Set for regen type of generation */

      offIONC = il->ctoff[IONC];
      offRGH = il->ctoff[RGH];
      plsd = il->prd;
      celllim = il->locell + il->mcells;  /* Set counter limit */
      for (icell=il->locell; icell<celllim; ++icell,plsd+=il->lel) {

/* If in a regeneration cycle, omit modifying cells that responded
*  to any stimulus in the previous trial series, as indicated by a
*  nonzero bit in the RGH statistical field, which must exist here.
*  (Prior to V8A, field XRM was tested.  RGH was introduced in V8A
*  to make regeneration independent of XRM statistics, which now
*  invoke all sorts of machinery in all celltypes to support propa-
*  gation of responses to modalities.  This fixes a bug in that d3go
*  does not mark XRM if stats are disabled, but now does mark RGH.)
*/

         if (*(plsd+offRGH)) continue;

/* Cell failed to respond, so regenerate it.
*  Initialize Lij and Cij generation for the current cell */

         d3kiji(il, icell);
         if (il->CTST) il->CTST->nregen++;   /* Count regens */

/* Clear all cell-level data (dst, depression, sbar, etc.)
*  (Must do before code which may write a,b,c,d,u,w vars */

         memset((char *)plsd, 0, il->ls);

/* Generate Izhikevich a,b,c,d randomizations.
*  N.B.  We apply randomizations as positive or negative offsets
*  to mean values of a,b,c,d--published code always adds them.  */

         if (il->Ct.rspmeth >= RF_IZH3) {
            struct IZHICOM *piz = (struct IZHICOM *)il->prfi;
            if (ctexists(il, IZRA)) {
               rd_type *plzd = plsd + il->ctoff[IZRA];
               si32 tt;
               si32 lzseed = piz->izseed;
               udevskip(&lzseed, il->nizvar*icell);
               if (piz->izra) {
                  tt = udev(&lzseed) << 1;
                  tt = mssle(tt, (si32)piz->izra, -31, OVF_IZHI);
                  d3ptl2(tt, plzd); plzd += SHSIZE; }
               if (piz->izrb) {
                  tt = udev(&lzseed) << 1;
                  tt = mssle(tt, (si32)piz->izrb, -31, OVF_IZHI);
                  d3ptl2(tt, plzd); plzd += SHSIZE; }
               if (piz->izrc) {
                  tt = udev(&lzseed) << 1;
                  tt = mssle(tt, abs32(tt), -31, OVF_IZHI);
                  tt = mssle(tt, (si32)piz->izrc, -31, OVF_IZHI);
                  d3ptl2(tt, plzd); plzd += SHSIZE; }
               if (piz->izrd) {
                  tt = udev(&lzseed) << 1;
                  tt = mssle(tt, abs32(tt), -31, OVF_IZHI);
                  tt = mssle(tt, (si32)piz->izrd, -31, OVF_IZHI);
                  d3ptl2(tt, plzd); }
               }
            } /* End rspmeth >= RF_IZH3 */

/* Generate s(i) data.  This code revised, 07/24/10, to call the new
*  initsi() which now can operate on a single cell.  This eliminates
*  a bunch of duplicated code and the danger of its getting out of
*  step with the version in d3genr().  */

         icellsp = spsize(icell, il->phshft);
         nskip = icell * il->mxsdelay1;
         if (Lija.Noisy) {       /* Noise seed for this cell */
            il->nsd = wnsd;
            udevskip(&il->nsd, nskip+nskip); }
         if (Lija.Phsupd < 0) {  /* Phase seed for this cell */
            il->pctpd->phiseed = wpsd;
            udevskip(&il->pctpd->phiseed, nskip); }
         initsi(il, icell);

/* Loop over connection types */

         for (ix=il->pct1; ix; ix=ix->pct) {

            /* Make ptrs to first, fence, skipped synapses */
            d3kijx(ix);
            d3lijx(ix);
            d3cijx(ix);
            d3dijx(ix);
            offCij = ix->cnoff[CIJ];
            offCij0= ix->cnoff[CIJ0];
            offLij = ix->cnoff[LIJ];
            offDij = ix->cnoff[DIJ];
            offMij = ix->cnoff[MIJ];
            offPPF = ix->cnoff[PPF];
            offPPFT= ix->cnoff[PPFT];

/*---------------------------------------------------------------------*
*                         Loop over synapses                           *
*---------------------------------------------------------------------*/

            for (Lija.isyn=0 ; Lija.isyn<ix->nc; ++Lija.isyn) {

/* Generate an Lij coefficient and store it.
*  Exit if premature end of list signalled.
*  (Note:  If we are running this regeneration code,
*  Lij, Cij are by definition always being stored.)  */

               if (!Lija.lijbr(ix)) break;
               d3ptln(Lija.lijval, Lija.psyn+offLij, ix->lijlen);

/* Generate a Cij coefficient.  You might think that it is not
*  necessary to regenerate type M coefficients, because they do
*  not change, but they might have been condensed due to Lij
*  out-of-bounds in previous incarnation of this cell.  */

               ix->cijbr(ix);
               d3pthn(Lija.cijsgn, Lija.psyn+offCij, ix->cijlen);
               if (cnexists(ix,CIJ0)) d3pthn(Lija.cijsgn,
                  Lija.psyn+offCij0, ix->cijlen);

/* Generate delay.  Constant delay is not stored in the data
*  structure, but is retrieved when needed from ix->Cn.mxcdelay */

               if (ix->Cn.kdelay > DLY_CONST) {
                  ui32 dijval = ix->dijbr(ix);
                  d3ptl1(dijval, Lija.psyn+offDij);
                  } /* End if delay */

/* If MIJ, PPF, PPFT exist, clear them */

               if (offMij  >= DVREQ) d3ptl2(zero, Lija.psyn+offMij);
               if (offPPF  >= DVREQ) d3ptl2(zero, Lija.psyn+offPPF);
               if (offPPFT >= DVREQ) d3ptl2(zero, Lija.psyn+offPPFT);

               Lija.psyn += ix->lc;
               } /* End of loop over synapses */

/* Here when all synapses of current type have been processed.
*  Save number of connections actually generated.  */

            pnuk = Lija.pxnd + ix->xnoff[NUK];
            d3ptln(Lija.isyn, pnuk, ix->nuklen);

            }  /* End of loop over connection types */

/* Initialize state of any ion concentrations specific to this cell
*  (internal conc. always, external only if not GROUP or REGION).
*  This is a subset of the code in inition that skips the latter.
*
*  First look at ions associated with the region, then ions
*  associated with the celltype in this ugly loop.
*
*  It is not necessary to check that the external concentration is
*  variable--if not, d3cchk will have set ionopts to ION_REGION.  */

         for (iki=0,pion=ir->pionr; iki<2; iki++,pion=il->pion1) {
            for ( ; pion; pion=pion->pnion) {
               lmvi = (pion->ionopts & ION_CELL) ? 2*ESIZE : ESIZE;
               memcpy((char *)(plsd+offIONC+pion->oCi),
                  (char *)&pion->C0int, lmvi);
               } /* End loop over chain of ion blocks */
            } /* End loop over two chains of ion blocks */

         } /* End of loop over cells */

      /* Restore Lij, Cij, etc. generation for d3go() */
      d3kij(il, KMEM_GO);

#endif                     /* END NODE 0 EXCLUSION FROM CELL LOOP */

/* Update noise and phase seeds on all nodes as if all cells regen'd */

      nskip = il->nelt * il->mxsdelay1;
      if (Lija.Noisy) {
         il->nsd = wnsd;
         udevskip(&il->nsd, nskip+nskip); }
      if (Lija.Phsupd < 0) {
         il->pctpd->phiseed = wpsd;
         udevskip(&il->pctpd->phiseed, nskip); }

/* Since Lij may have changed, allow detail print to print them */

      il->dpitem &= ~DPDIDL;

      }  /* End of loop over cell types */

   }   /* End of d3regenr() */
