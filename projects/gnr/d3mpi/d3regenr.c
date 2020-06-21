/* (c) Copyright 1994-2018, The Rockefeller University *11115* */
/* $Id: d3regenr.c 77 2018-03-15 21:08:14Z  $ */
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
*     modality markings, mostly because there is just one set for all  *
*     cells, and most cells are not changed.                           *
*  N.B.  It is OK here to use il->iovec overflow counting, as we are   *
*     called at the end of a trial series but before d3oflw is called. *
*                                                                      *
************************************************************************
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
*  V8H, 10/20/10, GNR - Add xn (once per conntype) variable allocation *
*  Rev, 12/24/12, GNR - New overflow checking                          *
*  R66, 02/17/16, GNR - Change long random seeds to typedef si32 orns  *
*  R78, 07/11/18, GNR - Add ibias, add noise only if NFL_ADDR          *                                     *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "rkarith.h"
#include "d3global.h"
#include "lijarg.h"
#ifndef PARn
#include "rocks.h"
#endif

extern struct LIJARG Lija; /* Arguments for Lij routines */

void d3regenr(void) {

   struct CELLTYPE *il;
   struct CONNTYPE *ix;
   struct PHASEDEF *ppd;
   si64 nskip;             /* Number of seeds to skip over */
   si32 bias;              /* Constant addition (S7/14) */
   si32 lzseed;            /* Local copy of izhi ABCD seed */
   si32 nmnS7;             /* Noise mean (S7) */
   si32 nsgS11;            /* Noise sigma (S11) */
   int  Noisy;             /* TRUE if noise must be generated */
   int  Phsupd;            /* Phase setting switch */

#ifndef PAR0
   struct REPBLOCK *ir;
   struct IONTYPE  *pion;
   rd_type *plsd;          /* Ptr to "ls" data for current cell */
   rd_type *pnuk;          /* Ptr to place to store NUK */
   void (*resetu)(struct CELLTYPE *il, ui32 icell);

   size_t llel;            /* Length of data for one cell */
   size_t lmvi;            /* Length of ion conc copy */

   int  icell;             /* Cell counter */
   int  icellsp;           /* = icell * lsp */
   orns  wpsd;             /* Working phase seed */
   orns  wnsd;             /* Working noise seed */

   si32  zero = 0;         /* Where zero needed as a lvalue */
   int   idly;             /* Delay counter */
   int   iki;              /* Kind of ion block */
   int   kold = RP->compat & OLDRANGE;

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

/* Set switches to control noise and phase for individual cells */

      e64dec(EAct(il->iovec+CTO_RGEN));
#ifndef PAR0
      ir = il->pback;
#endif
      bias = il->ibias;
      nmnS7 = SRA(il->No.nmn,FBwk-FBsi);
      nsgS11 = il->No.nsg >> (FBwk-FBsi);
      Noisy = il->No.noikadd & NFL_ADDR && !(il->Ct.ctopt & OPTNIN);
      if (il->phshft) {
         ppd = il->pctpd;
         Phsupd = (ppd->phimeth == PHI_CONST) ? (int)ppd->fixpha : -1;
         }
      else
         Phsupd = 0;

/* Update Izhikevich randomization seed, cseed, dseed, and lseed for a
*  new cell lifetime on all nodes (so will survive later broadcasts).
*  Note that cseed and lseed are updated more than needed in certain
*  cases, but this is simpler and OK as long as it is deterministic.
*  Do not test CTFRANOS here -- rebuild Izhi ABCD +/- OPTMEM.  izseed
*  is not changed during loops--always skips to specific cell.  */
      if (il->Ct.rspmeth >= RF_IZH3 && ctexists(il, IZRA)) {
         struct IZHICOM *piz = (struct IZHICOM *)il->prfi;
         lzseed = piz->izseed;
         udevwskp(&piz->izseed, jmsw(il->nizvar,il->nelt));
         }
      for (ix=il->pct1; ix; ix=ix->pct) {
         nskip = jmsw(il->nelt,ix->nc);
         if (ix->kgen & (KGNRC|KGNLM))
            udevwskp(&ix->cseed, nskip);
         if (ix->Cn.kdelay > DLY_CONST)
            udevwskp(&ix->Cn.dseed, nskip);
         udevwskp(&ix->lseed, jmsw(il->nelt,ix->uplij));
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
      llel = uw2zd(il->lel);
      /* Which resetu routine to use */
      resetu = (il->Ct.rspmeth == RF_IZH7) ? resetu7 : resetu3;
      for (icell=il->locell; icell<il->hicell; ++icell,plsd+=llel) {

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

/* Generate s(i) data.  We cannot call initsi() because it would
*  reset all cells in the layer, not just those being regenerated.
*  This code revised, 02/24/98,10/16/05,01/03/07 to perform same
*  actions as initsi (except order of using updated nsd,phiseed
*  is different).  */

         icellsp = spsize(icell, il->phshft);
         nskip = jmsw(icell, il->mxsdelay1);
         if (Noisy) {         /* Noise seed for this cell */
            wnsd = il->No.nsd;
            udevwskp(&wnsd, jasw(nskip,nskip)); }
         if (Phsupd < 0) {    /* Phase seed for this cell */
            wpsd = ppd->phiseed;
            udevwskp(&wpsd, nskip); }
         for (idly=0; idly<(int)il->mxsdelay1; idly++) {
            register s_type *pcell = il->pps[idly] + icellsp;
            si32 wksi = bias;
            if (Noisy) {                  /* Cell has noise */
               wksi += d3noise(nmnS7,nsgS11,il->No.nfr,wnsd);
               if (kold) {
                  if (wksi > S14) wksi = S14;
                  else if (wksi < 0) wksi = 0;
                  }
               } /* End generating cell with noise */
            d3ptjl2(wksi, pcell);
            if (il->phshft) {
               if (Phsupd >= 0)           /* Fixed phase */
                  pcell[LSmem] = (s_type)Phsupd;
               else
                  pcell[LSmem] = (s_type)(udev(&wpsd) & PHASE_MASK);
               } /* End phase assignment */
            } /* End delay row loop */

/* Clear all cell-level data (dst, depression, sbar, etc.) */

         memset((char *)plsd, 0, il->ls);

/* Generate Brette-Gerstner variables:  32-bit v and w only.  */

         if (il->Ct.rspmeth == RF_BREG) resetw(il, icell);

/* Generate Izhikevich variables:  u and a,b,c,d randomizations.
*  As per Izhikevich (2003), the equilibrium value of u is b*v.
*  N.B.  We apply randomizations as positive or negative offsets
*  to mean values of a,b,c,d--published code always adds them.  */

         else if (il->Ct.rspmeth >= RF_IZH3) {
            struct IZHICOM *piz = (struct IZHICOM *)il->prfi;
            if (ctexists(il, IZRA)) {
               rd_type *plzd = plsd + il->ctoff[IZRA];
               si32 tt;
               udevwskp(&lzseed, jmsw(il->nizvar,icell));
               if (piz->izra) {
                  tt = udev(&lzseed) << 1;
                  tt = mrssl(tt, (si32)piz->izra, FBrf);
                  d3ptjl2a(tt, plzd); plzd += HSIZE; }
               if (piz->izrb) {
                  tt = udev(&lzseed) << 1;
                  tt = mrssl(tt, (si32)piz->izrb, FBrf);
                  d3ptjl2a(tt, plzd); plzd += HSIZE; }
               if (piz->izrc) {
                  tt = udev(&lzseed) << 1;
                  tt = mrssl(tt, abs32(tt), FBrf);
                  tt = mrssld(tt, (si32)piz->izrc, -FBrf);
                  d3ptjl2a(tt, plzd); plzd += HSIZE; }
               if (piz->izrd) {
                  tt = udev(&lzseed) << 1;
                  tt = mrssl(tt, abs32(tt), FBrf);
                  tt = mrssld(tt, (si32)piz->izrd, -FBrf);
                  d3ptjl2a(tt, plzd); }
               }
            resetu(il, icell);
            } /* End rspmeth >= RF_IZH3 */

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

            for (Lija.isyn=0 ; Lija.isyn<(long)ix->nc; ++Lija.isyn) {

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
                  d3ptjl1(dijval, Lija.psyn+offDij);
                  } /* End if delay */

/* If MIJ, PPF, PPFT exist, clear them */

               if (offMij  >= DVREQ) d3ptjl2(zero, Lija.psyn+offMij);
               if (offPPF  >= DVREQ) d3ptjl2(zero, Lija.psyn+offPPF);
               if (offPPFT >= DVREQ) d3ptjl2(zero, Lija.psyn+offPPFT);

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

      nskip = jmsw(il->nelt, il->mxsdelay1);
      if (Noisy) udevwskp(&il->No.nsd, jasw(nskip,nskip));
      if (Phsupd < 0) udevwskp(&ppd->phiseed, nskip);

/* Since Lij may have changed, allow detail print to print them */

      il->dpitem &= ~DPDIDL;

      }  /* End of loop over cell types */

   }   /* End of d3regenr() */
