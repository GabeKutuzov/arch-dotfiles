/* (c) Copyright 1991-2010 Neurosciences Research Foundation, Inc. */
/* $Id: d3resetn.c 47 2011-05-24 21:15:38Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                              d3resetn                                *
*                                                                      *
*               Reset cells to resting (inactive) state                *
*              (Call from all nodes, serial or parallel)               *
*                                                                      *
*  TO CAUSE A DYNAMIC VARIABLE TO BE RESET, add its allocation code    *
*     and reset type (variable to reset from or -1 to set to zero)     *
*     to the appropriate reset table below.  If a variable is being    *
*     reset from another variable (e.g. Cij from Cij0), the program    *
*     assumes their lengths are the same.  New code may need to be     *
*     added where indicated to handle new variable length codes        *
*     (negative lengths in allo_tbl).  It is good to keep entries      *
*     in same order as in ordering tables in allo_tbl.h to allow       *
*     adjacent variables to be consolidated.                           *
*                                                                      *
*  NOTES:                                                              *
*     Function d3rset was broken into separate routines to handle      *
*     repertoire data (d3resetn) and everything else (d3resetz).       *
*     This is necessary so that the following sequence of events       *
*     can happen when the a reset is initiated unpredictably by        *
*     a BBD device:                                                    *
*        (1) bbdsgetv returns value that initiates reset.              *
*        (2) Non-repertoire stuff is reset by d3resetz().              *
*        (3) Sensory cells are updated (may depend on                  *
*            updated arm/window/value data).                           *
*        (4) This stuff, including updated RP array, is                *
*            broadcast to other nodes, which find out for              *
*            first time that they are supposed to reset.               *
*        (5) Repertoire stuff is reset by d3resetn().                  *
*     Value information is reset on all nodes (by both pgms).          *
*     In the serial version, both routines are called in turn.         *
*                                                                      *
*     Translation to C introduces tables of dynamic variables that     *
*     need resetting.  This replaces the self-modifying code/macro     *
*     mechanism in the IBM BAL version.  Before looping through the    *
*     cell data, d3resetn() sets up tables containing the lengths      *
*     and offsets of the variables that need to be reset.  When it     *
*     steps through the repertoire data, it can use these tables       *
*     to reset the variables quickly.  Furthermore, CONNTYPE blocks    *
*     are specially threaded to allow skipping those with no resets.   *
*                                                                      *
*     In V8A, each conntype has individual dynamic allocation.  To     *
*     handle this in a reasonably efficient manner without allocat-    *
*     ing space permanently for reset tables, the tables are created   *
*     before and released after each reset.  This has the further      *
*     advantage that items that don't exist can be omitted from the    *
*     tables, so no tests are needed in the inner loop to skip over    *
*     them.  The load_reset_tbl() function was absorbed back into      *
*     the d3resetn code to give it access to conntype variables.       *
*                                                                      *
*     It could be argued that SBAR should be decayed whenever s(i)     *
*     is decayed (i.e. even if OPTJS is set), but given its use in     *
*     amplification, it seems better to keep the original treatment    *
*     (no decay if OPTJS).  OPTJS is of doubtful value anyway.         *
*                                                                      *
*  REVISION HISTORY:                                                   *
*  New, 08/06/91, GNR - Separate d3rset into d3resetz and d3resetn     *
*  Rev, 03/19/92, GNR - New call to initsi                             *
*  Rev, 07/14/92, ABP - Call to d3ptln cannot use a constant           *
*  Rev, 11/24/92, GNR - Implement OPTJS option to reset s(i) only      *
*  V6D, 02/08/94, GNR - Revise initsi call for delays, add Cij reset   *
*  V8A, 05/26/95, GNR - Clear modality markers en route axonal delay,  *
*                       clear mod/gconn decay, remove "LTP", add DST   *
*  Rev, 08/14/96, GNR - Call d3zval()                                  *
*  Rev, 09/28/97, GNR - Independent dynamic allocations per conntype   *
*  Rev, 12/27/97, GNR - Add ADECAY, PPF, consolidate adjacent entries  *
*  V8D, 08/17/05, GNR - Add conductances and ions, new reset options   *
*  ==>, 01/08/08, GNR - Last mod before committing to svn repository   *
*  Rev, 12/31/08, GNR - Replace jm64sh with msrsle                     *
*  V8E, 01/26/09, GNR - Move node 0 repertoire updates to d3resetn to  *
*                       simplify & correct bug when iti < mxsdelay1.   *
*  Rev, 03/28/09, GNR - Revise equilibration of Izhi u variable        *
*  Rev, 09/07/09, GNR - Add resetw routine for Brette-Gerstner reset,  *
*                       resetauw to reset u or w for all cells, move   *
*                       updating of effcyc here from darwin3 main.     *
*  Rev, 11/28/09, GNR - Separate resetu3 and resetu7                   *
*  V8H, 10/20/10, GNR - Add xn (once per conntype) variable allocation *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "sysdef.h"
#include "bapkg.h"
#include "d3global.h"
#include "celldata.h"
#include "allo_tbl.h"
#include "rksubs.h"
#include "rkarith.h"

#ifndef PAR0

extern struct CELLDATA CDAT;

/* Entry format for dynamic variable reset tables */
typedef struct {
   short alloc_code,    /* Variable's allocation code */
         kreset,        /* Kind of reset: code, later offset, of
                        *  variable to set from, -1 to set to 0.  */
         offset,        /* Offset into data area */
         len;           /* Length of variable data */
   } reset_tbl_entry;

/*---------------------------------------------------------------------*
*  Table of cell-level dynamic variables to reset.  Only first two     *
*  fields are initialized--others get set during execution.  If any    *
*  variable-length items are to be reset, length must be calculated    *
*  and inserted in cvlength table at point indicated in code below.    *
*---------------------------------------------------------------------*/

static reset_tbl_entry cell_var_reset_tbl[] =
         { {SBAR, -1}, {QBAR, -1}, {DST, -1}, {DEPR, -1}, {CNDG, -1} };

#define NCellVars  \
   (sizeof(cell_var_reset_tbl) / sizeof(reset_tbl_entry))

/*---------------------------------------------------------------------*
*  Table of connection-level variables to reset.                       *
*  Note:  Cij is coded specially to be reset to current Cij0 value.    *
*  No attempt is made to reset Cij0, because we have no reasonable     *
*  way of knowing the initial value to reset it to.                    *
*---------------------------------------------------------------------*/

static reset_tbl_entry conn_var_reset_tbl[] =
         { {RBAR, -1}, {PPF, -1}, {PPFT, -1}, {MIJ, -1}, {CIJ, CIJ0} };

#define NConnVars  \
   (sizeof(conn_var_reset_tbl) / sizeof(reset_tbl_entry))

/*---------------------------------------------------------------------*
*  Routine to return the space needed for reset tables, called from    *
*  d3nset() only to put this space in the psws area.  This is not a    *
*  lot of space, and almost every run will have a level 3 reset, so    *
*  no attempt is made to check whether the space is really needed.     *
*---------------------------------------------------------------------*/

long lrsettbl(void) {
   return (NCellVars + RP->mxnct*NConnVars) *
      sizeof(reset_tbl_entry);
   }
#endif /* !PAR0 */


#ifndef PAR0
/*=====================================================================*
*                         resetu3 and resetu7                          *
*                                                                      *
*  As per Izhikevich (2003), the equilibrium value of u is b*v.        *
*  To get sufficient precision in Izhikevich calculations to replicate *
*  the published examples, the v and u variables need 32 bits.  For a  *
*  specified cell, these routines expand the normal 16-bit s variable  *
*  to 32 bits for v, and also calculate the Izhikevich u variable to   *
*  be in equilibrium with the current value of v (si) during initial   *
*  generation, regeneration, or reset, using u = b*(v-vur) or variant. *
*  These routines must only be called when the appropriate rspmeth is  *
*  in effect (and prfi exists).  Note that vur is allowed with the     *
*  2003 model, even though not in the publication.                     *
*=====================================================================*/

void resetu3(struct CELLTYPE *il, ui32 icell) {
   struct IZ03DEF *pi3 = (struct IZ03DEF *)il->prfi;
   rd_type *plrd = il->prd + (icell - il->locell)*il->lel;
   byte *prvu = (byte *)(plrd + il->ctoff[IZVU]);
   s_type  *psi = il->pps[0] + spsize(icell, il->phshft);
   si32 b,simvur,wbu,wksi;

   d3gts2(wksi, psi);         /* Pick up s(i) */
   wksi <<= (FBwk-FBsi);      /* Scale s(i) to S20/27 */

   /* We need b to calculate i3vr */
   b = pi3->Z.izb;
   if (pi3->Z.izrb) {     /* Cells have individual b values */
      int qzra = pi3->Z.izra != 0;
      if (il->Ct.ctopt & OPTMEM) {  /* Not in memory */
         /* N.B.  Must discard bits to bring precision to
         *  same number of bits saved when not OPTMEM.  */
         si32 lzseed = pi3->Z.izseed;
         udevskip(&lzseed, il->nizvar*icell + qzra);
         wbu = udev(&lzseed) << 1;
         wbu = mssle(wbu, (si32)pi3->Z.izrb, -31, OVF_IZHI);
         /* S31 + 14 - 31 + 14 ==> S28 */
         b += wbu << 14; }
      else {                        /* Indiv b in memory */
         rd_type *prb = plrd + il->ctoff[IZRA] + SHSIZE*qzra;
         d3gth2(wbu, prb);
         /* S14 + 16 (d3gth2) - 2 = S28 */
         b += SRA(wbu,2); }
      } /* End getting individual b */

   /* Adjust CNS s(i)(t-1) for 2003 model Vrest scale and
   *  for any difference in v and u rest potentials.
   *  N.B.  When resetu3 is called the first time, stored
   *  values of i3vr are not yet available, so always
   *  calculate as if not stored.  */
   simvur = wksi + d3i3vr(pi3, b) - pi3->Z.vur;

   /* Now have b, calculate u */
   wbu = msrsle(b, simvur, FBIu-FBIab-FBwk, OVF_IZHI);
   d3pti4(wksi, prvu);
   d3pti4(wbu, (prvu+LSIZE));
   } /* End resetu3() */


void resetu7(struct CELLTYPE *il, ui32 icell) {
   struct IZ07DEF *pi7 = (struct IZ07DEF *)il->prfi;
   rd_type *plrd = il->prd + (icell - il->locell)*il->lel;
   byte *prvu = (byte *)(plrd + il->ctoff[IZVU]);
   s_type  *psi = il->pps[0] + spsize(icell, il->phshft);
   si32 b,simvur,wbu,wksi,wuv3;

   d3gts2(wksi, psi);         /* Pick up s(i) */
   wksi <<= (FBwk-FBsi);      /* Scale s(i) to S20/27 */
   /* simvur = wksi adjusted for u rest pot'ls */
   simvur = wksi - pi7->Z.vur;

   if (simvur < 0) {
      /* Use b,uv3 for negative s(i) */
      b = pi7->ubvlooCm, wuv3 = pi7->uuv3looCm; }
   else {
      b = pi7->Z.izb, wuv3 = pi7->uuv3oCm;
      if (pi7->Z.izrb) {      /* Cells have individual b values */
         int qzra = pi7->Z.izra != 0;
         if (il->Ct.ctopt & OPTMEM) {  /* Not in memory */
            /* N.B.  Must discard bits to bring precision to
            *  same number of bits saved when not OPTMEM.  */
            si32 lzseed = pi7->Z.izseed;
            udevskip(&lzseed, il->nizvar*icell + qzra);
            wbu = udev(&lzseed) << 1;
            wbu = mssle(wbu, (si32)pi7->Z.izrb, -31, OVF_IZHI);
            /* S31 + 14 - 31 + 14 ==> S28 */
            b += wbu << 14; }
         else {                        /* Indiv b in memory */
            rd_type *prb = plrd + il->ctoff[IZRA] + SHSIZE*qzra;
            d3gth2(wbu, prb);
            /* S14 + 16 (d3gth2) - 2 = S28 */
            b += SRA(wbu,2); }
         } /* End getting individual b */
      }
   /* Now have b, calculate u */
   wbu = msrsle(b, simvur, FBIu-FBIab-FBwk, OVF_IZHI);
   if (wuv3) {                /* Need v-cubed term */
      si64 t64; si32 r3;
      int isqis = -2*(FBwk + RP->bsdc);
      t64 = jmsw(simvur, simvur);
      t64 = msswe(t64, simvur, isqis, OVF_IZHI);
      t64 = msrswe(t64, wuv3, FBIu-FBwk-36, OVF_IZHI);
      r3 = (si32)(swlo(t64));
      if (swhi(t64) != SRA(r3,(BITSPERUI32-1)))
         e64act("resetu", OVF_IZHI);
      addck(wbu, r3, OVF_IZHI);
      }
   d3pti4(wksi, prvu);
   d3pti4(wbu, (prvu+LSIZE));
   } /* End resetu7() */


/*=====================================================================*
*                               resetw                                 *
*                                                                      *
*  This is the Brette-Gerstner variant of resetu().  It does the same  *
*  calculation without the complications of bvlo and vur, i.e. expands *
*  s(i) to 32 bits and calculates equilibrium value of w for that s(i).*
*=====================================================================*/

void resetw(struct CELLTYPE *il, ui32 icell) {
   struct BREGDEF *pbg = (struct BREGDEF *)il->prfi;
   rd_type *plrd = il->prd + (icell - il->locell)*il->lel;
   byte *prvw = (byte *)(plrd + il->ctoff[IZVU]);
   s_type  *psi = il->pps[0] + spsize(icell, il->phshft);
   long wkw,wksi;          /* Working w(i),s(i) (mV S20) */

   d3gts2(wksi, psi);      /* Pick up s(i) */
   wksi <<= (FBwk-FBsi);   /* Scale s(i) to S20/27 */
   wkw = mssle(pbg->bga, wksi, -24, OVF_BREG);
   d3pti4(wksi, prvw);
   d3pti4(wkw, (prvw+LSIZE));
   } /* End resetw() */


/*=====================================================================*
*                               resetauw                               *
*                                                                      *
*  Resets Izhikevich u or Brette-Gerstner aEIF w for for all cells of  *
*  specified type on current node.                                     *
*=====================================================================*/

void resetauw(struct CELLTYPE *il) {
   long icell,hicell;
   hicell = il->locell + il->mcells;
   if (il->Ct.rspmeth == RF_IZH7) {
      for (icell=il->locell; icell<hicell; ++icell)
         resetu7(il, icell);
      }
   else if (il->Ct.rspmeth == RF_BREG) {
      for (icell=il->locell; icell<hicell; ++icell)
         resetw(il, icell);
      }
   else {
      for (icell=il->locell; icell<hicell; ++icell)
         resetu3(il, icell);
      }
   } /* End resetauw() */
#endif   /* !PAR0 */


/*=====================================================================*
*                              d3resetn                                *
*                                                                      *
*  Argument:  rl is the current reset level                            *
*     (Currently, level 1 reset is performed on-the-fly in d3go().)    *
*=====================================================================*/

void d3resetn(int rl) {

   struct REPBLOCK *ir;
   struct CELLTYPE *il;
#ifndef PAR0
   struct INHIBBLK *ib;
   struct MODBY    *imb;
   struct MODVAL   *imv;
   struct CONNTYPE *ix,*ix0,**iix;
   struct CONDUCT  *pcnd;
   double          ititime;      /* Minus intertrial time (msec) */
   float           efac,gfac;    /* exp(-t/tau), 1.0 - efac */
   long            llel;         /* Local copy of lel */
#endif
   int             liti;         /* Local copy of iti */

#ifdef PAR
/* Reset value schemes (comp nodes).
*  (Omit if serial because done by d3resetz also.)  */

   if (rl >= RP0->rlvl[lvval] && RP->nvblk > 0) d3zval(VBVOPBR);
#endif

   switch (rl) {

/*---------------------------------------------------------------------*
*  Reset level 2 (OMEGA1):  The traditional reset formerly in d3go().  *
*  (This didn't check OPTBR originally, so it doesn't do it now.)      *
*  Rev, 07/04/05, GNR - Change omega1 to S30, omit il->vrest.          *
*  V8E, 01/30/09, GNR - Add recalculation of Izhikevich u after reset  *
*---------------------------------------------------------------------*/

/* All s(i) values, including those in delay pipelines, are decayed
*  according to omega1.  At the same time, new noise is generated and
*  added.  Both actions are implicit in the idea that omega1
*  corresponds to an additional time delay applied between stimuli.
*
*  N.B.  There is no noise modulation here, so criterion for
*        adding noise is different than in cell evaluation loop.
*  N.B.  Storage for different delay times may not be contiguous.
*        It is necessary to loop over time and then over cells.
*/

case RL_OLDTRIAL:

   for (il=RP->pfct; il; il=il->pnct) {
      if (il->Dc.omega1 < S30) { /* Omit if omega1 >= 1.0 (S30) */
         register s_type *pcell;
         s_type *pce;            /* Cell loop control */
         long avgnoise;          /* Average noise (mV S7) */
         long nmnS7,nsgS11;      /* Scaled noise parameters */
         long wksi;              /* Working s(i) (mV S7) */
         int  knoise = (il->ctf & CTFHN) != 0;
         int  idly;              /* Delay loop control */
         if (knoise) {
            nmnS7    = SRA(il->No.nmn,FBwk-FBsi);
            nsgS11   = il->No.nsg >> (FBwk-FBsi);
            avgnoise = msrsle(nmnS7, il->No.nfr, -FBsc, OVF_RSET);
            }
         else
            avgnoise = 0;
         for (idly=0; idly<(int)il->mxsdelay1; idly++) {
            pcell = il->pps[idly];
            pce = pcell + il->lspt;
            for ( ; pcell<pce; pcell+=il->lsp) {
               d3gts2(wksi, pcell);
               wksi = msrsle(wksi - avgnoise, il->Dc.omega1,
                  -FBod, OVF_RSET);
               /* Generate the noise term */
               if (knoise) wksi +=
                  d3noise(nmnS7, nsgS11, il->No.nfr, il->nsd);
               d3ptl2(wksi, pcell);
               }  /* End cell loop */
            } /* End delay loop */
#ifndef PAR0
         /* Update Izhikevich 'u' or Brette-Gerstner 'w' */
         if (il->Ct.rspmeth >= RF_BREG) resetauw(il);
#endif
         } /* End of new-trial decay */
      } /* End celltype loop */
   break;

/*---------------------------------------------------------------------*
*  Reset level 3:  Decay everything as if iti cycles have passed.      *
*---------------------------------------------------------------------*/

case RL_TRIAL:

/* Loop over regions and reset ion concentrations.  Note:  Floating-
*  word alignment of region- and cell-level ion concentrations should
*  be assured by d3allo().  If that situation is changed, this code
*  must copy from data array into temporary floats, decay, then copy
*  back when ALIGN_TYPE == TRUE.  Note:  The inition routine could be
*  combined with this one, with code to force efac-->0 for inition.  */

   liti = RP->iti;
#ifndef PAR0               /* No ions on Node 0 */
   ititime = -1.0E-3*(double)liti*(double)RP->timestep;
   for (ir=RP->tree; ir; ir=ir->pnrp) {
      struct IONTYPE *pion;
      rd_type *plid,*plie;       /* Ptrs to layer ion data */
      rd_type *pri0,*prid,*prie; /* Ptrs to region ion data */
      long    llg = ir->lg;

/* If not bypassing at region level, reset region-level ions.
*  N.B.  Do not check CELLTYPE bypass for region-level ions.  */

      if (ir->andctopt & OPTBR) continue;
      pri0 = ir->pgpd + ir->gpoff[IONG];
      prie = ir->pgpd + ir->lgt;
      for (pion=ir->pionr; pion; pion=pion->pnion) {
         if (pion->ionflgs & (CDOP_IONEXT|CDOP_IONREV)) {
            /* Decay external concentration */
            efac = (float)exp(ititime/pion->tauext);
            gfac = 1.0 - efac;
            if (pion->ionopts & ION_REGION)           /* Regional */
               pion->Cext = efac*pion->Cext + gfac*pion->C0ext;
            else if (pion->ionopts & ION_GROUP) {     /* Per-group */
               for (prid=pri0; prid<prie; prid+=llg) {
                  register float *pc = (float *)prid;
                  *pc = efac*(*pc) + gfac*pion->C0ext; }
               pri0 += ESIZE; }
            else for (il=ir->play1; il; il=il->play) { /* Per-cell */
               plid = il->prd + il->ctoff[IONC] + pion->oCi + ESIZE;
               plie = il->prd + il->llt;
               llel = il->lel;
               for ( ; plid<plie; plid+=llel) {
                  register float *pc = (float *)plid;
                  *pc = efac*(*pc) + gfac*pion->C0ext; }
               } /* End loop over CELLTYPEs in this region */
            } /* End external conc */
         if (pion->ionflgs & (CDOP_IONINT|CDOP_IONREV)) {
            /* Decay internal concentration */
            efac = (float)exp(ititime/pion->tauint);
            gfac = 1.0 - efac;
            for (il=ir->play1; il; il=il->play) {
               plid = il->prd + il->ctoff[IONC] + pion->oCi;
               plie = il->prd + il->llt;
               llel = il->lel;
               for ( ; plid<plie; plid+=llel) {
                  register float *pc = (float *)plid;
                  *pc = efac*(*pc) + gfac*pion->C0int; }
               } /* End loop over CELLTYPEs in this region */
            } /* End internal conc */
         } /* End region-level IONTYPE loop */

/* If not bypassing at celltype level, reset celltype-level ions */

      for (il=ir->play1; il; il=il->play) {
         if (il->Ct.ctopt & OPTBR) continue;
         llel = il->lel;
         for (pion=il->pion1; pion; pion=pion->pnion) {
            if (pion->ionflgs & (CDOP_IONEXT|CDOP_IONREV)) {
               /* Decay external concentration */
               efac = (float)exp(ititime/pion->tauext);
               gfac = 1.0 - efac;
               if (pion->ionopts & ION_REGION)        /* Regional */
                  pion->Cext = efac*pion->Cext + gfac*pion->C0ext;
               else if (pion->ionopts & ION_GROUP) {  /* Per-group */
                  for (prid=pri0; prid<prie; prid+=llg) {
                     register float *pc = (float *)prid;
                     *pc = efac*(*pc) + gfac*pion->C0ext; }
                  pri0 += ESIZE; }
               else {                                 /* Per-cell */
                  plid = il->prd + il->ctoff[IONC] + pion->oCi + ESIZE;
                  plie = il->prd + il->llt;
                  for ( ; plid<plie; plid+=llel) {
                     register float *pc = (float *)plid;
                     *pc = efac*(*pc) + gfac*pion->C0ext; }
                  } /* End checking update mode */
               } /* End decaying external concentrations */
            if (pion->ionflgs & (CDOP_IONINT|CDOP_IONREV)) {
               /* Decay internal concentration */
               efac = (float)exp(ititime/pion->tauint);
               gfac = 1.0 - efac;
               plid = il->prd + il->ctoff[IONC] + pion->oCi;
               plie = il->prd + il->llt;
               for ( ; plid<plie; plid+=il->lel) {
                  register float *pc = (float *)plid;
                  *pc = efac*(*pc) + gfac*pion->C0int; }
               } /* End decaying internal concentrations */
            } /* End CELLTYPE-level IONTYPE loop */
         } /* End CELLTYPE loop */
      } /* End region loop */
#endif

/* Loop over cell types.  If bypass set in
*  layer options, don't reset this layer */

   for (il=RP->pfct; il; il=il->pnct) if (!(il->Ct.ctopt & OPTBR)) {

      s_type  *pc,*pc0,*pc1,*pce;         /* Cell loop controls */
      struct PHASEDEF *ppd = il->pctpd;   /* Ptr to phase parms */
#ifndef PAR0
      rd_type *pr,*pr0,*prc,*pre;
      rd_type *px,*px0,*px1,*pxe;
      struct RFRCDATA *prf;      /* Ptr to refractory parameters */
      struct PPFDATA  *ppfd;     /* Ptr to PPF parameters */
      long llc;                  /* Local copy of ix->lc */
      int  jc;                   /* Mij, PPF decay steps */
#endif
      long avgnoise;             /* Average noise (mV S7) */
      long nmnS7,nsgS11;         /* Scaled noise parameters */
      long wkomeg;               /* Working persistence (S30) */
      long wkv;                  /* Working variable */
      int  docyc  = liti;
      int  mxsd   = il->mxsdelay1 - 1;
      int  excyc  = docyc - mxsd;
      int  knoise = (il->ctf & CTFHN) != 0;
      int  komega = il->Dc.CDCY.omega < S30;

/* Deal with items that are saved over mxsdelay1 cycles:  At present,
*  these include s(i), phase, and modality markings.  s(i) data are
*  decayed by a factor of il->omega per cycle, phase is randomized
*  (unless constant), and marker data are cleared.  Here each node
*  computes values for all cells.  Alternatively, each node could
*  compute for just its own cells and then call d3exch(), but the
*  amount of computation is small enough that the exchange might
*  well take longer.  (Separate loops for each variable minimize
*  'if's inside the loops.)  */

      if (knoise) {
         nmnS7    = SRA(il->No.nmn,FBwk-FBsi);
         nsgS11   = il->No.nsg >> (FBwk-FBsi);
         avgnoise = msrsle(nmnS7, il->No.nfr, -FBsc, OVF_RSET);
         }
      else
         avgnoise = 0;

      /* If iti > mxsdelay, delay first by iti-mxsdelay cycles */
      if (excyc > 0) {
         pc0 = il->pps[0];
         pce = pc0 + il->lspt;
         if (komega | knoise) {
            wkomeg = ui32pow(il->Dc.CDCY.omega, excyc, FBod, OVF_DCY);
            for (pc=pc0; pc<pce; pc+=il->lsp) {
               d3gts2(wkv, pc);  /* wkv = s(i) */
               wkv -= avgnoise;
               if (komega) wkv = msrsle(wkv, wkomeg, -FBod, OVF_RSET);
               if (knoise) wkv +=
                  d3noise(nmnS7, nsgS11, il->No.nfr, il->nsd);
               d3ptl2(wkv, pc);
               }  /* End cell loop */
            } /* End s(i) decay */
         if (il->ctf & CTFNC) for (pc=pc0; pc<pce; pc+=il->lsp)
            pc[LSmem] = (s_type)(udev(&ppd->phiseed) & PHASE_MASK);
         if (il->lmm)
            memset((char *)pce, 0, (size_t)il->lmm);
         docyc -= excyc;
         } /* End excess cycle decay */

      /* Now perform min(iti,mxsdelay) cycles of decay */
      while (docyc--) {

         /* Move the s(i), phase, and marker bits down one step.
         *  Note that if mxsd is 0, docyc will now be 0 also,
         *  so there is no need to test for mxsd > 0 here.  */
         int isd = mxsd;
         pce = il->pps[isd];
         while (isd--) il->pps[isd+1] = il->pps[isd];
         pc0 = il->pps[0] = pce;
         pce = pc0 + il->lspt;

         if (komega | knoise) {
            wkomeg = il->Dc.CDCY.omega;
            pc1 = il->pps[1];
            for (pc=pc0; pc<pce; pc+=il->lsp,pc1+=il->lsp) {
               d3gts2(wkv, pc1); /* wkv = s(i) */
               wkv -= avgnoise;
               if (komega) wkv = msrsle(wkv, wkomeg, -FBod, OVF_RSET);
               if (knoise) wkv +=
                  d3noise(nmnS7, nsgS11, il->No.nfr, il->nsd);
               d3ptl2(wkv, pc);
               }  /* End cell loop */
            } /* End s(i) decay */
         if (il->ctf & CTFNC) for (pc=pc0; pc<pce; pc+=il->lsp)
            pc[LSmem] = (s_type)(udev(&ppd->phiseed) & PHASE_MASK);
         if (il->lmm)
            memset((char *)pce, 0, (size_t)il->lmm);

         } /* End decay cycle loop */

#ifndef PAR0
      /* Update Izhikevich 'u' or Brette-Gerstner 'w' */
      if (il->Ct.rspmeth >= RF_BREG) resetauw(il);

      /* Decay real modulation value blocks using omega for current
      *  cell type (not necessary to decay virtual MODVAL blocks, as
      *  they get recomputed on every cycle).  */
      if (komega) {
         wkomeg = ui32pow(il->Dc.CDCY.omega, liti, FBod, OVF_DCY);
         for (imv=il->pmrv1; imv; imv=imv->pmdv) imv->umds.mdsum =
            msrswe(imv->umds.mdsum, wkomeg, FBod, OVF_DCY);
         }

      /* If OPTJS bit is set, quit after decaying s(i) and modval */
      if (il->Ct.ctopt & OPTJS) continue;

      /* Decay variables representing decaying postsynaptic
      *  potentials in modulated-by blocks  */
      for (imb=il->pmby1; imb; imb=imb->pmby) if (imb->MDCY.omega) {
         wkomeg = ui32pow(imb->MDCY.omega, liti, FBod, OVF_DCY);
         imb->umve.mvaleff =
            msrsle(imb->umve.mvaleff, wkomeg, -FBod, OVF_RSET);
         }

      /* Decay variables representing decaying postsynaptic
      *  potentials in geometric connection types  */
      for (ib=il->pib1; ib; ib=ib->pib) if (ib->GDCY.omega) {
         long igd,lgdcy = NDecayCells(il, ib, il->mcells);
         wkomeg = ui32pow(ib->GDCY.omega, liti, FBod, OVF_DCY);
         if (ib->gssck & PSP_POS) for (igd=0; igd<lgdcy; igd++)
            ib->ieveff[igd] =
               msrsle(ib->ieveff[igd], wkomeg, -FBod, OVF_RSET);
         if (ib->gssck & PSP_NEG) for (igd=0; igd<lgdcy; igd++)
            ib->iiveff[igd] =
               msrsle(ib->iiveff[igd], wkomeg, -FBod, OVF_RSET);
         }

/* Decay other cell-level variables */

      llel = il->lel;
      pr0 = il->prd;
      pre = pr0 + il->llt;

      /* Decay average activity */
      if (ctexists(il, SBAR) && il->Dc.sdamp < S15) {
         wkomeg = ui32pow(S15-(ui32)il->Dc.sdamp, liti,
            FBdf, OVF_DCY);
         for (pr=pr0+il->ctoff[SBAR]; pr<pre; pr+=llel) {
            d3gts2(wkv, pr);
            wkv = msrsle(wkv, wkomeg, -FBod, OVF_RSET);
            d3ptl2(wkv, pr);
            }
         }
      if (ctexists(il, QBAR) && il->Dc.qdamp < S15) {
         wkomeg = ui32pow(S15-(ui32)il->Dc.qdamp, liti,
            FBdf, OVF_DCY);
         for (pr=pr0+il->ctoff[QBAR]; pr<pre; pr+=llel) {
            d3gts2(wkv, pr);
            wkv = msrsle(wkv, wkomeg, -FBod, OVF_RSET);
            d3ptl2(wkv, pr);
            }
         }

      /* Decay depression */
      if (il->ctf & CTFDN && il->Dp.omegad < S30) {
         wkomeg = ui32pow(il->Dp.omegad, liti, FBod, OVF_DCY);
         for (pr=pr0+il->ctoff[DEPR]; pr<pre; pr+=llel) {
            d3gtl1(wkv, pr);
            wkv = msrsle(wkv, wkomeg, -FBod, OVF_RSET);
            d3ptl1(wkv, pr);
            }
         }

      /* Decay refractory period control.
      *  A negative value of DST is a refrac timer.  */
      if (prf = il->prfrc) {     /* Assignment intended */
         /* Most common value will be full period */
         wkomeg = ui32pow(prf->omegadst, liti, FBod, OVF_DCY);
         for (pr=pr0+il->ctoff[DST]; pr<pre; pr+=llel) {
            d3gth2(wkv, pr);     /* S7+16=23 */
            if (wkv < 0) {       /* In a refrac period */
               wkv += liti << Ss2hi;
               if (wkv == 0)     /* Escaped refrac period */
                  wkv = prf->psdst << 3;
               else if (wkv > 0) {
                  long twkv = ui32pow(prf->omegadst,
                     wkv>>Ss2hi, FBod, OVF_DCY);
                  wkv = msrsle(prf->psdst<<3, twkv, -FBod, OVF_RSET); }
               }
            else
               wkv = msrsle(wkv, wkomeg, -FBod, OVF_RSET);
            d3pth2(wkv, pr);
            }
         }

/* Decay relevant ion-channel conduction variables */

      if (il->ctf & CTFgD) {
         /* Celltype has at least one conductance w/traditional gDCY.
         *  These can be gated or nongated. */
         ui32 g,jgnh,jgth;
         long teff;           /* Temp for time calcs */
         prc = pr0 + il->ctoff[CNDG];
         for (pcnd=il->pcnd1; pcnd; pcnd=pcnd->pncd) {
            if (pcnd->gDCY.omega) {
               /* Traditional decay method.  The residue in prd data
               *  decays exponentially.  If conductance is gated and
               *  an event is still active and passage of iti cycles
               *  would exceed nc2dc, add what's left into the decay
               *  part and kill the event.  If not gated, no events
               *  are recorded and refrac parameter is forbidden at
               *  input time.  */
               wkomeg = ui32pow(pcnd->gDCY.omega, liti, FBod, OVF_DCY);
               if (pcnd->cdflgs & CDFL_GgDCY) {
                  for (pr=prc; pr<pre; pr+=llel) {
                     d3gtl3(g, pr);
                     d3gtl2(jgnh, pr+ognh);
                     d3gtl2(jgth, pr+ogth);
                     g = msrsle(g, wkomeg, -FBod, OVF_RSET);
                     if (jgnh) {
                        teff = (RP->CP.effcyc & UI16_MAX) - (long)jgth;
                        if (teff < 0) teff += S16;
                        if ((teff += liti - (int)pcnd->nc2dc) >= 0) {
                           ui64 term = jmuw(jgnh, (ui32)RP->pgact[
                              pcnd->ogcat+pcnd->nc2dc-1]);
                           long omp = ui32pow(pcnd->gDCY.omega, teff+1,
                              FBod, OVF_DCY);
                           term = msuwe(term, omp,
                              -((2*FBod-FBsc)+pcnd->ugi.igi), OVF_DCY);
                           term = jaul(term, g);
                           for (teff=0; uwhi(term) || uwlo(term)>=S24;
                                 ++teff) term = jsruw(term, 1);
                           if (teff > 0) cndshft(il, pcnd, teff);
                           g = uwlo(term);
                           jgnh = 0;
                           d3ptl2(jgnh, pr+ognh);
                           }
                        } /* End dealing with active event */
                     d3ptl3(g, pr);
                     }
                  prc += LgHNT;
                  }
               else {      /* Has gDCY but not gated */
                  for (pr=prc; pr<pre; pr+=llel) {
                     d3gtl3(g, pr);
                     g = msrsle(g, wkomeg, -FBod, OVF_RSET);
                     d3ptl3(g, pr);
                     }
                  prc += Lg;
                  }
               } /* End gDCY.omega if */
            /* No traditional decay, may still have refrac period */
            else prc += (pcnd->refrac > 0) ? LgHNT : Lg;
            } /* End conductance loop */
         } /* End traditional decay style */

/* Decay relevant connection-level variables */

      for (ix=il->pct1; ix; ix=ix->pct) {
         rd_type *pnuk;
         long nuk;               /* Number connections used */
         px0 = ix->psyn0 + ix->lxn;
         llc = ix->lc;

         /* Decay modifying substance.  A value of Mij <= Mijtest is
         *  a slow amp timer.  Must do the decay steps explicitly in
         *  order to apply the max pumping rate.  */
         if (cnexists(ix, MIJ)) {
            long Mijtest = ix->Cn.mticks - S15;    /* Mij clock test  */
            long Mijdecr;                          /* Mij decrement   */
            /* Loop over cells */
            pnuk = ix->psyn0 + ix->xnoff[NUK];
            px1 = px0 + ix->cnoff[MIJ];
            for ( ; px1<pre; pnuk+=llel,px1+=llel) {
               d3gtln(nuk, pnuk, ix->nuklen);
               pxe = px1 + nuk*llc;
               for (px=px1; px<pxe; px+=llc) {     /* Loop over conns */
                  d3gts2(wkv, px);                 /* Pick up old Mij */
                  if (wkv <= Mijtest) {            /* Is Mij a timer? */
                     if ((wkv+=liti) > 0) wkv = 0; /* Incr Mij clock  */
                     }
                  else for (jc=0; jc<liti; ++jc) { /* Mij is a value  */
                     Mijdecr = wkv*ix->Cn.zetam;   /* S14 + S16 = S30 */
                     Mijdecr = SRA(Mijdecr, 10);   /* Mijdecr to S20  */
                     if (labs(Mijdecr) > ix->Cn.mxmp) Mijdecr =
                        (Mijdecr >= 0) ? ix->Cn.mxmp : -ix->Cn.mxmp;
                     wkv -= SRA(Mijdecr, 6);
                     } /* End decay cycle loop */
                  d3ptl2(wkv, px);
                  } /* End connection loop */
               } /* End cell loop */
            } /* End decaying Mij */

         /* Decay paired pulse facilitation.  Must do the decay steps
         *  explicitly in view of switch from rising to falling PPF.  */
         if (ppfd = ix->PP) {                  /* Assignment intended */
            rd_type *pwf,*pwt;                    /* Ptrs to PPF,PPFT */
            long wkf, wkt;                          /* PPF, PPF timer */
            int kabrupt = ppfd->ppfopt & PPFABR;    /* TRUE if abrupt */
            pnuk = ix->psyn0 + ix->xnoff[NUK];     /* Loop over cells */
            for (px1=px0; px1<pre; pnuk+=llel,px1+=llel) {
               d3gtln(nuk, pnuk, ix->nuklen);
               pxe = px1 + nuk*llc;
               for (px=px1; px<pxe; px+=llc) {     /* Loop over conns */
                  pwf = px + ix->cnoff[PPF];
                  pwt = px + ix->cnoff[PPFT];
                  d3gts2(wkf, pwf);                    /* Pick up PPF */
                  d3gts2(wkt, pwt);              /* Pick up PPF timer */
                  if (kabrupt) {                        /* ABRUPT PPF */
                     for (jc=0; jc<liti; ++jc) {  /* Count iti cycles */
                        if (wkt < 0) {           /* Rising from a hit */
                           if ((wkt += 2) >= 0) {  /* Rise time is up */
                              wkf = ppfd->ppflim;  /* Now begin decay */
                              wkt = ppfd->htdn + ppfd->htup;
                           }}
                        /* In decay phase, count down the decay clock.
                        *  When decay time is up, reset to idle state.*/
                        else if (wkt > 0 && (wkt -= 2) <= 0)
                           wkf = wkt = 0;
                        } /* End decay cycle loop */
                     } /* End ABRUPT PPF */
                  else {                                /* Normal PPF */
                     for (jc=0; jc<liti; ++jc) {  /* Count iti cycles */
                        if (wkt < 0) {           /* Rising from a hit */
                           if (++wkt >= 0) wkt = ppfd->htdn;
                           wkf += msrsle(ppfd->upsp, ppfd->ppflim-wkf,
                              -28, OVF_RSET);
                           }
                        else if (wkt > 0) { /* In the declining phase */
                           if (--wkt == 0) wkf = 0;
                           else wkf -= msrsle(ppfd->taup, wkf,
                              -28, OVF_RSET);
                           }
                        } /* End decay cycle loop */
                     } /* End normal PPF */
                  d3ptl2(wkf, pwf);               /* Store back PPF */
                  d3ptl2(wkt, pwt);              /* Store back PPFT */
                  } /* End connection loop */
               } /* End cell loop */
            } /* End decaying paired-pulse facilitation */

         } /* End connection type loop */
#endif /* !PAR0 */

      /* Check for overflow errors */
      if (RP->ovflow)
         d3exit(OVERFLOW_ERR, fmturlnm(il), RP->ovflow);
      } /* End CELLTYPE loop */

   /* Increment effective cycle counter by reset time interval */
   RP->CP.effcyc += RP->iti;
   break;

/*---------------------------------------------------------------------*
*  Reset level 4:  Decay everything as if infinite time has passed.    *
*  Reset level 5:  Same as 4 except also decay connection strengths.   *
*                                                                      *
*  Note that effcyc is not incremented in this case (to infinity?),    *
*  so anything dependent on it just continues on from where it was.    *
*---------------------------------------------------------------------*/

case RL_SERIES:
case RL_FULL:
case RL_NEWSERIES:
case RL_NEWFULL: {

#ifndef PAR0
   reset_tbl_entry *pcttbl,*pcntbl,*ptbl,*ptble,*pt,*ptp;
   long            coff;         /* Offset to data for current cell */
   long            lllt;         /* Local copy of llt */

   /* This switch must be set on every call, not just every series,
   *  because of the possibility of event-initiated resets.  */
   /* BUG FIX, 11/08/10, GNR - Test should only pass for the two
   *  cases RL_FULL and RL_NEWSERIES -- this will be much more
   *  logical when new RESET controls are implemented.  */
   int krscij = ((rl & RL_FULL) == RL_FULL) || (RP->compat & RESETCIJ);

/* Use psws space for reset tables */

   pcttbl = (reset_tbl_entry *)CDAT.psws;

/* Loop over regions and reset ion concentrations */

   for (ir=RP->tree; ir; ir=ir->pnrp)
      inition(ir, OPTBR);
#endif

/* Loop over all cell types */

   for (il=RP->pfct; il; il=il->pnct) {

      /* If bypass set in layer options, don't reset this layer */
      if (il->Ct.ctopt & OPTBR) continue;

/* Clear activity variables */

      /* Clear the s[i][t] and modality marker tables */
      initsi(il, 0, (int)il->mxsdelay1);
      if (il->lmm) {
         int idly;
         for (idly=0; idly<(int)il->mxsdelay1; idly++)
            memset((char *)il->pps[idly]+il->lspt, 0, (size_t)il->lmm);
         }

      /* Request fast starts on next cycle */
      if (il->orkam & KAMFS) il->ctf |= CTFDOFS;
      if (il->kautu & KAUT_FS) il->ctf |= CTFASFS;

#ifndef PAR0
      /* Update Izhikevich 'u' or Brette-Gerstner 'w' */
      if (il->Ct.rspmeth >= RF_BREG) resetauw(il);

      /* Reset real modulation value blocks (not necessary to reset
      *  virtual MODVAL blocks, they get recomputed every cycle). */
      for (imv=il->pmrv1; imv; imv=imv->pmdv)
         memset((char *)&imv->umds.mdsum, 0, sizeof(imv->umds.mdsum));

      /* If OPTJS bit is set, just reset s(i) and modval */
      if (il->Ct.ctopt & OPTJS) continue;

      /* Reset variables representing decaying postsynaptic
      *  potentials in modulated-by blocks  */
      for (imb=il->pmby1; imb; imb=imb->pmby)
         imb->umve.mvaleff = 0;

      /* Reset variables representing decaying postsynaptic
      *  potentials in geometric connection types  */
      for (ib=il->pib1; ib; ib=ib->pib) if (ib->GDCY.omega) {
         size_t lgdcy = sizeof(long) *
            NDecayCells(il, ib, il->mcells);
         if (ib->gssck & PSP_POS)
            memset((char *)ib->ieveff, 0, lgdcy);
         if (ib->gssck & PSP_NEG)
            memset((char *)ib->iiveff, 0, lgdcy);
         }

/* Prepare to reset the individual cell and connection state variables
*  requested in the static tables above.  Initialize the reset tables,
*  thread the CONNTYPE blocks that require resets, advance to the next
*  layer if no variables in this one need resetting.  */

      pt = pcttbl;      /* Next table entry */
      ptp = NULL;       /* Previous table entry */

      /* Store special lengths that may be referenced at cell level */
      cvlength[LCNDG] = il->lcndg;

      /* Load cell-level reset table */
      ptble = cell_var_reset_tbl + NCellVars;
      for (ptbl=cell_var_reset_tbl; ptbl<ptble; ptbl++) {
         /* If variable exists, make entry to reset it */
         if (ctexists(il,ptbl->alloc_code)) {
            short ioff = il->ctoff[ptbl->alloc_code];
            short ilen = ctlength[ptbl->alloc_code];
            if (ilen < 0) ilen = cvlength[abs(ilen)];
            /* If item is adjacent to previous item, and both
            *  are being reset to zero, consolidate them.  */
            if (ptp && ptp->offset + ptp->len == ioff &&
                  (ptp->kreset & ptbl->kreset) < 0)
               ptp->len += ilen;
            else {
               *pt = *ptbl;      /* Copy the table entry */
               if (pt->kreset >= 0)
                  pt->kreset = il->ctoff[pt->kreset];
               pt->offset = ioff;
               pt->len = ilen;
               ptp = pt++;
               }
            } /* End if item allocated */
         } /* End loop over cell_var_reset_tbl */

      /* Load connection-level reset tables */
      iix = &ix0;
      pcntbl = pt;
      ptble = conn_var_reset_tbl + NConnVars;
      for (ix=il->pct1; ix; ix=ix->pct) {

         /* Error if trying to reset Cij to Cij0
         *  and CIJ0 was not saved.  */
         if (krscij && cnexists(ix,CIJ) && !cnexists(ix,CIJ0))
            d3exit(RESETCIJ_ERR, fmturlnm(il), ix->ict);

         /* Reset variables representing decaying postsynaptic
         *  potentials in specific connection types */
         if (ix->Cn.ADCY.omega) memset((char *)ix->aeff, 0,
            LSIZE * il->mcells * npsps((int)ix->cssck));

         /* Request rbar fast start on next cycle */
         if (ix->Cn.kam & KAMFS) ix->cnflgs |= CNDOFS;

         ix->cnwk.reset.ncnvar = 0;
         ptp = NULL;
         /* Store special lengths for connection level */
         cvlength[LCIJ0] = ix->cijlen;
         cvlength[LLIJ]  = ix->lijlen;
         cvlength[LCIJ]  = ix->cijlen;
         for (ptbl=conn_var_reset_tbl; ptbl<ptble; ptbl++) {
            /* If variable exists, make entry to reset it */
            if (cnexists(ix,ptbl->alloc_code) &&
                  ((ptbl->alloc_code != CIJ) || krscij)) {
               short ioff = ix->cnoff[ptbl->alloc_code];
               short ilen = cnlength[ptbl->alloc_code];
               if (ilen < 0) ilen = cvlength[abs(ilen)];
               /* If item is adjacent to previous item, and both
               *  are being reset to zero, consolidate them.  */
               if (ptp && ptp->offset + ptp->len == ioff &&
                     (ptp->kreset & ptbl->kreset) < 0)
                  ptp->len += ilen;
               else {
                  *pt = *ptbl;   /* Copy the table entry */
                  if (pt->kreset >= 0)
                     pt->kreset = ix->cnoff[pt->kreset];
                  pt->offset = ioff;
                  pt->len = ilen;
                  ptp = pt++, ix->cnwk.reset.ncnvar++;
                  }
               } /* End if item allocated */
            } /* End loop over conn_var_reset_tbl */
         /* Thread just the CONNTYPEs that need resets */
         if (ix->cnwk.reset.ncnvar)
            *iix = ix, iix = &ix->cnwk.reset.pnxr;
         } /* End loop over connection types */

      if (pt == pcttbl) continue;   /* Skip if nothing to reset */
      *iix = NULL;                  /* Terminate CONNTYPE thread */

/* Step through repertoire data, zeroing variables that need it */

      llel = il->lel, lllt = il->llt;
      for (coff=0; coff<lllt; coff+=llel) {

         /* Reset cell-level variables--may be no-pass loop */
         register rd_type *pcd = il->prd + coff;
         for (ptbl=pcttbl; ptbl<pcntbl; ptbl++) {
            if (ptbl->kreset >= 0)
               memcpy((char *)pcd + ptbl->offset,
                  (char *)pcd + ptbl->kreset, ptbl->len);
            else
               memset((char *)pcd + ptbl->offset, 0, ptbl->len);
            }

         /* Loop over just those CONNTYPEs--if any--that have
         *  connection-level variables to reset and reset them.
         *  (Currently no xnoff-type variables require resetting.)
         *  (The order of the pxd and pt loops could be reversed
         *  since the latter will usually have fewer passes, but
         *  this was not done--could lead to more memory paging.) */
         for (ix=ix0; ix; ix=ix->cnwk.reset.pnxr) {
            rd_type *pxd = ix->psyn0 + coff, *pxde;
            rd_type *pnuk = pxd + ix->xnoff[NUK];
            long nuk;
            int llc = (int)ix->lc;
            d3gtln(nuk, pnuk, ix->nuklen);
            ptble = ptbl + ix->cnwk.reset.ncnvar;
            pxd += ix->lxn;
            pxde = pxd + nuk*llc;
            for ( ; pxd<pxde; pxd+=llc) {
               for (pt=ptbl; pt<ptble; pt++) {
                  if (pt->kreset >= 0)
                     memcpy((char *)pxd + pt->offset,
                        (char *)pxd + pt->kreset, pt->len);
                  else
                     memset((char *)pxd + pt->offset, 0, pt->len);
                  } /* End reset variables loop */
               } /* End connection loop */
            ptbl = ptble;
            } /* End connection type loop */
         } /* End cell loop */
      } /* End celltype loop */

#endif /* !PAR0 */

   } /* End RL_SERIES, NEWSERIES, FULL, and NEWFULL local scope */
   } /* End rl switch */

   } /* End d3resetn() */
