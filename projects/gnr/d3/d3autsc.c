/* (c) Copyright 2010-2018, The Rockefeller University *11116* */
/* $Id: d3autsc.c 77 2018-03-15 21:08:14Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                               d3autsc                                *
*            Calculations relating to response autoscaling             *
*                                                                      *
*  Mod, 05/20/13, GNR - Previous versions did not rescale self inputs  *
*  (except autapses).  This was changed (in d3go and d3inht) on basis  *
*  that self inputs are from previous cycle, so should change to give  *
*  their corrected contribution to the cell response in the new cycle. *
*                                                                      *
*  Mod, 07/13/13, GNR - Rewrite W mode to recompute s(i) immediately   *
*  at end of cycle based on rescaled inhibitory inputs--no more trying *
*  to estimate scales for next cycle by chaining.                      *
************************************************************************
*  V8G, 08/13/10, GNR - New routine                                    *
*  ==>, 08/28/10, GNR - Last mod before committing to svn repository   *
*  V8H, 05/09/12, GNR - Add KAUT_ET option                             *
*  Rev, 06/05/12, GNR - Do not update scale if target response is < 0  *
*  Rev, 04/23/13, GNR - Implement KAUT_DNO option--scale down, not up  *
*  Rev, 05/20/13, GNR - Implement KAUT_HYP option--scale inhibition    *
*  Rev, 06/14/13, GNR - Move kauts/nauts initialization from d3news    *
*  Rev, 06/27/13, GNR - Move per-cycle initialization from d3go, add   *
*                       kasop mechanism to control afference scaling,  *
*                       chain scales without damping after cycle 1     *
*  Rev, 07/13/13, GNR - Rework W mode to apply AS_HYPB scale at once   *
*  Rev, 10/14/13, GNR - Add gsimax/min, rspmax/min mechanism           *
*  Rev, 08/22/14, GNR - Si only EXP decay, remove DECAYDEF from DCYDFLT*
*  R70, 01/31/17, GNR - Replace serial data gather with new mpgather   *
*  R77, 02/22/18, GNR - Bug Fix: Make PAR nsd updates deterministic    *
*  R78, 07/19/18, GNR - Modify treatment of noise per new 'V' option   *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "sysdef.h"
#include "rocks.h"
#include "rkarith.h"
#include "d3global.h"
#include "collect.h"
#include "swap.h"
#include "celldata.h"

extern struct CELLDATA CDAT;

#ifndef PARn
/*=====================================================================*
*                               d3asck                                 *
*                                                                      *
*  This routine performs autoscale initializations that can change     *
*  only at start of a new trial group.  il->nauts is set regardless    *
*  of whether or not updating scales.  il->kautu KAUT_UOK is set to    *
*  reflect global conditions for updating autoscale multipliers.       *
*  This is on when KAUT_NU is off so kautu==0 remains an easy test     *
*  for no autoscale.  Input tests assure >= 1 of A,S,G,M,I bits are    *
*  present.  If update is OK, also initializes AUTSCL astta field.     *
*  Earlier setup code was moved here from d3news() because this needs  *
*  to be called (like d3lafs()) before d3nset() both at the start of   *
*  the run and after any new Group III input.                          *
*=====================================================================*/

void d3asck(void) {

   struct CELLTYPE *il;

   for (il=RP->pfct; il; il=il->pnct) {
      ui16 tkaut = 0; byte tnaut = 0;
      il->ctf &= ~CTFAS1T;
      if (il->pauts) {
         struct AUTSCL *paut = il->pauts;
         tkaut = paut->kaut ^ KAUT_NU;
         /* Number of scales is stored whether or not updating */
         tnaut = (tkaut & KAUT_WTA) ? 2 : 1;
         /* Now set up controls for updating.  By design, KNUP_AS
         *  option does not affect WTA or T mode updates.  */
         if (paut->navg == 0 || (RP->knoup & KNUP_AS &&
            !(tkaut & (KAUT_ET|KAUT_WTA)))) tkaut &= ~KAUT_UOK;
         if (tkaut & KAUT_UOK) {
            /* Back-calculate from target response level the level
            *  of afference that would be needed to reach that level
            *  (ignoring spikes, which should be above the range of
            *  interest here)--avoids forward calc in d3autsc.  */
            paut->astta = paut->astt << (FBwk - FBsi);
            /* Adjust for noise applied after response fn.  */
            if (il->No.noikadd & NFL_ADDR)
               paut->astta -= jm64sl(il->No.nmn, il->No.nfr, -FBsc);
            if (il->Ct.rspmeth == RF_TANH) {
               /* Calc afference level that would give requested
               *  astt response.  By input test, stanh != 0 */
               float atth = (float)paut->astta/(float)il->Ct.stanh;
               if (fabsf(atth) >= 1.0) {
                  cryout(RK_E1, "0***AUTOSCALE TARGET OUT OF TANH "
                     "RANGE FOR ", RK_LN2, fmturlnm(il), RK_CCL, NULL);
                  paut->astta = atth >= 0 ? SI32_MAX : -SI32_MAX;
                  }
               else {
                  atth = atanhf(atth);
                  if (fabsf(atth) >= 16.0)
                     paut->astta = atth >= 0 ? SI32_MAX : -SI32_MAX;
                  else
                     paut->astta = (si32)(dS27*atth);
                  }
               } /* End adjustment for TANH response function */
            /* Adjust for average noise if applied to Vm */
            if (il->No.noikadd & NFL_ADDV)
               paut->astta -= jm64sl(il->No.nmn, il->No.nfr, -FBsc);
            /* Adjust for pt, nt if knee response */
            if (!(il->ctf & CTFDR)) paut->astta -=
               (paut->astta >= 0) ? il->Ct.pt : il->Ct.nt;
            }
         /* If not doing scaling in every cycle, there is no sense
         *  to change the target cell numbers in each cycle... */
         if (RP->ntc == 1 || paut->astf1 == paut->astfl)
            il->ctf |= CTFAS1T;
         else if (!(tkaut & (KAUT_CYC|KAUT_WTA))) {
            cryout(RK_P1, "0-->WARNING:  astfl set equal to astf1 "
               "(OPT=I|W not present) for ", RK_LN2, fmtlrlnm(il),
               RK_CCL, NULL);
            paut->astfl = paut->astf1;
            }
         } /* End if (il->pauts) */
      il->kautu = tkaut, il->nauts = tnaut;
      } /* End celltype loop */

   } /* End d3asck() */
#endif


/*=====================================================================*
*                               d3ascc                                 *
*                                                                      *
*  Set flags to control autoscaling.  il->paut->kaut is copied to      *
*  il->kautu (except for KAUT_UOK bit), then modified to reflect       *
*  actions in this cycle.  HTN bit is set to cause a new '>ht' scale   *
*  to be calculated now and HPN bit a new 'H' scale (always with       *
*  OPT=C, in first cycle of trial with OPT=T, otherwise in last cycle  *
*  of trial).  In special KAUT_WTA mode, HTN scale is done in first    *
*  cycle of a trial, HPN in later cycles.  UPD and APP bits are set    *
*  according to calc mode.  CDAT.kasop is set to govern which of EXCIT *
*  vs INHIB, self vs nonself inputs are scaled by which asmul entry.   *
*  CDAT.qasov USG_QIR bit is set only when scales are to be applied.   *
*  WTA ignores (overrides) C bit.  T is incompatible with WTA, implies *
*  I in cycle 1.  Per doc, CYCLE card NOUP=A will not affect first-    *
*  cycle-new-trial calcs.                                              *
*                                                                      *
*  This code runs on all nodes, all cycles.                            *
*=====================================================================*/

void d3ascc(struct CELLTYPE *il) {

   ui32 ccyc1 = (ui32)CDAT.curr_cyc1;           /* Current cycle */
   /* Codes that will tell S,G,M,A input scaling which asmul multi-
   *  pliers to use depending on mode and cycle.  The 4 (=NASOP)
   *  codes are for EXCIT, EXCIT-self, INHIB, INHIB-self, bzw.  */
   enum asmode { AmNone = 0, AmHTN, AmHYP, AmTn } kasc;
   static byte kasop0[][NASOP] = {
      { AS_NOP,  AS_NOP,  AS_NOP,  AS_NOP  },   /* No autoscale */
      { AS_GHTB, AS_GHTB, AS_GHTB, AS_GHTB },   /* Normal HTN */
      { AS_NOP,  AS_NOP,  AS_HYPB, AS_HYPB },   /* Normal HYP */
      { AS_GHTB, AS_NOP,  AS_GHTB, AS_NOP  } }; /* T cycle >=2 */

   if (il->pauts) {
      struct AUTSCL *paut = il->pauts;
      ui16 lkaut = paut->kaut & ~KAUT_NU | il->kautu & KAUT_UOK;
      /* wtnewt = test for OPT=T or W and new trial */
      ui16 wtnewt = ccyc1 == 1 ? (KAUT_ET|KAUT_WTA) : 0;
      /* Establish scaling mode even if not updating scales */
      if (lkaut & KAUT_WTA)
         kasc = (ccyc1 == 1) ? AmNone : AmTn;
      else if (lkaut & KAUT_ET)
         kasc = (ccyc1 == 1 || lkaut & KAUT_CYC) ? AmNone : AmTn;
      else if (lkaut & KAUT_HYP) kasc = AmHYP;
      else if (lkaut & (KAUT_ANY|KAUT_IMM)) kasc = AmHTN;
      else kasc = AmNone;
      memcpy(CDAT.kasop, kasop0[kasc], NASOP);
      /* Now deal with updating controls */
      if (lkaut & KAUT_UOK) {
         /* Bins are OK and it is not celltype no-update */
         if (lkaut & KAUT_WTA) {
            /* Combo mode -- HTN in cycle 1, then HPN */
            lkaut |= (ccyc1 == 1) ? (KAUT_HTN|KAUT_UPD|KAUT_APP) :
               (KAUT_HPN|KAUT_APP);
            }
         else if (lkaut & KAUT_ET) {
            /* KAUT_ET mode -- new scale on every trial */
            if (lkaut & (wtnewt|KAUT_FS|KAUT_CYC))
               lkaut |= (lkaut & KAUT_HYP) ?
                  (KAUT_HPN|KAUT_UPD|KAUT_APP) :
                  (KAUT_HTN|KAUT_UPD|KAUT_APP);
            }
         else {
            /* Normal mode -- new scale on last cycle of trial.
            *  Rev, 05/31/13, GNR - Do not test KAUT_FS here,
            *  i.e. wait for last cycle if not W,T,C modes */
            if (!(RP->knoup & KNUP_AS) &&
                  (lkaut & KAUT_CYC || il->ctwk.go.ntca == 0))
               lkaut |= (lkaut & KAUT_HYP) ?
                  (KAUT_HPN|KAUT_UPD) : (KAUT_HTN|KAUT_UPD);
            }
         }
      /* Allow fast start only if actually doing a scale calc.
      *  Omit updating running average scale if new start */
      if (il->ctf & CTFASFS && lkaut & (KAUT_HPN|KAUT_HTN))
         il->ctf &= ~CTFASFS; else lkaut &= ~KAUT_FS;
      if (lkaut & (KAUT_FS|wtnewt) || paut->adamp == S15)
         lkaut &= ~KAUT_UPD;
      /* If in immediate-update mode or a fresh start cycle,
      *  set to apply scale in same cycle and omit updating
      *  individual conntype scales.  HPN update will use
      *  update tables based on S,G,M,A bits in paut->kaut to
      *  determine which conntypes to enter into scaling.  */
      if (lkaut & KAUT_WTA) {
         if (ccyc1 == 1) lkaut &= ~(KAUT_S|KAUT_G|KAUT_M|KAUT_A);
         lkaut |= KAUT_APP; }
      else if (lkaut & (KAUT_IMM|KAUT_FS|KAUT_APP|wtnewt)) {
         lkaut &= ~(KAUT_S|KAUT_G|KAUT_M|KAUT_A);
         lkaut |= KAUT_APP; }
      /* If old scale is not being used, set it to 1.0 for
      *  neater printing in d3dprt.  */
      if (lkaut & (KAUT_FS|wtnewt))
         paut->asmul[AS_GHTB] = paut->asmul[AS_HYPB] = S24;
      /* Store cycle-customized option bits in il->kautu--
      *  originals remain in paut->kaut for next time.  */
      il->kautu = lkaut;
      /* Initialize response list for autoscaling */
      CDAT.passl = (byte *)CDAT.passl0;
      /* In KAUT_IMM mode, a second pass is made through all the
      *  cells to update s(i), sbar, and qbar.  A separate copy
      *  of rsd is used to maintain determinism if parallel.  */
      il->pauts->asrsd = il->rsd;
      }
   CDAT.qasov = il->kautu & KAUT_ANY ? USG_QIR : 0;
   /* Set response max/min for tests in d3go() */
   if (il->kautu & KAUT_APP)
      CDAT.rspmax = SHRT_MAX << (FBwk-FBsi), CDAT.rspmin = -CDAT.rspmax;
   else
      CDAT.rspmax = RP->gsimax, CDAT.rspmin = RP->gsimin;
   } /* End d3ascc() */


#ifndef PAR0
/*=====================================================================*
*                               d3assv                                 *
*                                                                      *
*  This new version of d3assv, passed a cell afference value (before   *
*  any truncation), just saves that afference and, if KAUT_HYP is set, *
*  the individual depolarizing and hyperpolarizing components, in the  *
*  passl list.  On a parallel computer, these will be collected on     *
*  node 0 and used to determine the scale factor by one or both of     *
*  the ">astt" or "HYP" methods.  Code assumes it is not called if     *
*  the list has not been allocated.  Code also assumes the AUTSSL and  *
*  AUTSSLs structs differ only in the missing tdepol,thypol items.     *
*=====================================================================*/

void d3assv(struct CELLTYPE *il, si32 totaff) {

   struct AUTSSL *pasl = (struct AUTSSL *)CDAT.passl;

   /* Save complement of afference to get descending sort */
   pasl->resp = totaff ^ SI32_MAX;

   if (il->kautu & KAUT_HPN) {
      /* If KAUT_HYP method is requested, use the ASDep and ASHyp
      *  tables prepared by d3pafs() to loop over all the types of
      *  connections present for this celltype and store their sums
      *  in the AUTSSL data.  These sums are 64-bit, but should not
      *  exceed 32-bits, so here we just check to be sure...  */
      short *past;               /* Ptr to table of afferents */
      si64 tsum;
      int jeca = (int)il->pauts->iovec;
      int nadd = 1 << il->pdshft;

      /* Loop over inputs that will not be rescaled */
      tsum = jesl(0);
      for (past=CDAT.ASDepTable; *past >= 0; ++past) {
         si64 *fa = CDAT.pAffD64 + *past;
         register int p;
         for (p=0; p<nadd; ++p)
            tsum = jasw(tsum, fa[p]);
         }
      /* Include noise term */
      if (il->No.noikadd & NFL_ADDV)
         tsum = jasl(tsum, CDAT.noise);
      swloem(pasl->tdepol, tsum, jeca);

      /* Loop over (hyperpolarizing) inputs that will be rescaled */
      tsum = jesl(0);
      for (past=CDAT.ASHypTable; *past >= 0; --past) {
         si64 *fa = CDAT.pAffD64 + *past;
         register int p;
         for (p=0; p<nadd; ++p)
            tsum = jasw(tsum, fa[p]);
         }
      swloem(pasl->thypol, tsum, jeca);

      CDAT.passl += sizeof(struct AUTSSL);
      }
   else
      CDAT.passl += sizeof(struct AUTSSLs);

   } /* End d3assv() */
#endif


/*=====================================================================*
*                               d3autsc                                *
*                                                                      *
*  Called from d3go at the end of a trial, this routine calculates     *
*  normalization scale factors by one of the two available methods     *
*  (HTN method:  adjust overall scale so a given fraction of the       *
*  cells respond above a specified level 'astt'.  HPN method:          *
*  Achieve same end by adjusting only selected inhibitory scales.)     *
*     Earlier code that used a response histogram to avoid saving all  *
*  the individual responses was not accurate enough.  This code sorts  *
*  a list of all the responses to obtain the desired cutoff points.    *
*     HPN uses the AUTSSL data structure.  HTN can share this or use   *
*  the smaller AUTSSLs struct if not in combo with HPN ('W' mode).     *
*  Either list is filled  by d3assv() calls.  On a parallel computer,  *
*  these must be collected on node 0 and the calculated scales must    *
*  be broadcast to the PARn nodes for use.                             *
*     If immediate scaling is requested, this routine also performs    *
*  the immediate scaling.  Scales are chained from cycle to cycle      *
*  according to the given damping factor or straight product.          *
*=====================================================================*/

void d3autsc(struct CELLTYPE *il) {

   struct AUTSCL *paut = il->pauts;
   si32 newscl;                  /* New scale */
#ifndef PARn
   si32 avgaffs;                 /* Average of mavg cells in range */
   ui32 ccyc1 = (ui32)CDAT.curr_cyc1;  /* Current cycle */
   si32 oldscl;                  /* Old working scale */
#endif
   int  iscl = 0;                /* Scale selector in asmul array */
   int  jeca = (int)paut->iovec; /* Code for counting overflows */
   ui16 lkaut = il->kautu;       /* Local copy of kaut flags */

/* Skip all the scaling calcs if just doing KAUT_APP scaling.
*  (This is indented to left margin to save one indent level.)  */
if (lkaut & (KAUT_HTN|KAUT_HPN)) {

#ifndef PARn
   struct AUTSSL *pasl;
   struct AUTSSL *psl1,*psl2;
   struct ASSTAT *psst, *psst0 = (struct ASSTAT *)(
      (char *)il->CTST + sizeof(struct CLSTAT));
   si64 sumaffs;                 /* Afference sum in range */
   ui32 insl,nslc,nsl1,nsl2;     /* Cell numbers for cycle +/- navg/2 */
   si32 mavg;                    /* Actual number cells in average */
   ui32 ncap = 0;                /* Count for psst->ncaps (which
                                 *  will not exist if CTPNS)  */
   int qdbg = RP->CP.dbgflags & DBG_AUTSC && RP->CP.trial % 100 < 4;
#endif
   int strsz = (lkaut & KAUT_HPN) ?
      sizeof(struct AUTSSL) : sizeof(struct AUTSSLs);

#ifndef PARn
   if (qdbg) {
      convrt("(P1,' For Series ',J0UJ10,', Trial ',J0UJ10,"
         "', Cycle ',J0UJ10,2H, J0A24)", &RP->CP.ser,
         &RP->CP.trial, &ccyc1, fmtlrlnm(il), NULL);
      }
#endif

#ifdef PAR
/*---------------------------------------------------------------------*
*                  Data merge for parallel computers                   *
*                                                                      *
*  The data stored on each node by d3assv() calls is sent to PAR0 for  *
*  collection, sorting, and the scaling calculation.  As of R70, data  *
*  are collected with the mpitools utility mpgather(), which operates  *
*  in time proportional to the log2 of the number of nodes, replacing  *
*  earlier versions that did a simple serial collection (which might   *
*  have deadlocked if lots of nodes. This should also be a lot faster  *
*  for runs with lots of nodes.                                        *
*                                                                      *
*  Note that because the list will be sorted on node 0 when all data   *
*  are received, there is no need to keep track of which bit came from *
*  which cells (although mpgather does this anyway).  The linked-list  *
*  pointers must be added here, as they are not contained in the data  *
*  returned by mpgather().                                             *
*                                                                      *
*  For now, assumes all nodes have same byte order--if not, we must    *
*  fix d3exch also.                                                    *
*---------------------------------------------------------------------*/


#ifdef PARn                /* I am a comp node */
   mpgather((char *)CDAT.psws, CDAT.passl0, il->locell, strsz,
      il->nelt, il->mcells, AUTSID_MSG);
#else                      /* I am host node */
   /* Leave space for first pointer at start of gather output */
   mpgather((char *)CDAT.passl0+sizeof(void *), NULL, strsz,
      (lkaut & KAUT_HPN) ?
      sizeof(struct AUTSSLn) : sizeof(struct AUTSSLsn),
      il->nelt, 0, AUTSID_MSG);
#endif /* !PARn */
#endif /* PAR */

#ifndef PARn
   /* Convert the data array into a linked list for sorting */
   {  char *pndat, *pgdat = (char *)CDAT.passl0;
      char *pgdate = pgdat + strsz*(il->nelt-1);
      for ( ; pgdat < pgdate; pgdat = pndat) {
         pndat = pgdat + strsz;
         ((struct AUTSSL *)pgdat)->pnsl = (struct AUTSSL *)pndat;
         }
      ((struct AUTSSL *)pgdat)->pnsl = NULL;
      } /* End pgdat local scope */

   pasl = (struct AUTSSL *)sort(CDAT.passl0, sizeof(void *),
      2*sizeof(si32), 0);

/*---------------------------------------------------------------------*
*          Preliminary calculations for HPN and HTN methods            *
*---------------------------------------------------------------------*/

   /* Compute the target cell fraction as function of cycle number */
   if (il->ctf & CTFAS1T)
      nslc = uwlo(jsruw(jmuw((ui32)il->nelt,(ui32)paut->astf1), FBdf));
   /* Rev, 07/02/13, GNR - If scales are not applied immediately,
   *  use target calculated for next (or last) cycle.  */
   else {
      ui32 ecyc, snum, sden = S15 * (ui32)(RP->ntc - 1);
      ecyc = (lkaut & KAUT_APP) ? ccyc1 : min(ccyc1+1, (ui32)RP->ntc);
      snum = (ui32)paut->astf1*(RP->ntc - ecyc) +
             (ui32)paut->astfl*(ecyc - 1);
      nslc = jduwq(jmuw((ui32)il->nelt, snum), sden);
      }
   /* Note if navg = 1, nsl1 == nsl2 by design.
   *  If navg = 0, this code should never be called.  */
   nsl1 = nslc - paut->navg/2;
   /* Beware:  nsl's are ui32, so negs appear very large */
   if (nsl1 >= il->nelt) nsl1 = 0;
   nsl2 = nslc + (paut->navg - 1)/2;
   if (nsl2 >= il->nelt) nsl2 = il->nelt - 1;
   mavg = nsl2 - nsl1 + 1;
   /* Locate the nsl1 response, compute average of mavg affs */
   sumaffs = jesl(0);
   for (psl1=pasl,insl=1; insl<nsl1; psl1=psl1->pnsl,++insl) ;
   for (psl2=psl1,insl=nsl1; insl<=nsl2; psl2=psl2->pnsl,++insl)
      sumaffs = jasl(sumaffs, psl2->resp ^ SI32_MAX);
   avgaffs = jdswq(sumaffs, mavg);

/*---------------------------------------------------------------------*
*                    Calculate scale by HTN method                     *
*                                                                      *
*  The original method that used just one selected cell to calculate   *
*  the desired scale has been replaced by a new method that uses the   *
*  average of navg cells.  To approximate tests run with the original  *
*  code, set navg = 1 and astt = il->ht (but now scaling is based on   *
*  afference rather than response, so it will never be exactly equal). *
*---------------------------------------------------------------------*/

   if (lkaut & KAUT_HTN) {

      oldscl = paut->asmul[iscl];

      /* If avg selected affs are < 0, omit scale calc altogether, as a
      *  larger scale just makes response more negative and eventually
      *  can cause overflow as scale tries to get very large.  Still
      *  must broadcast if parallel, as PARn nodes don't know this.  */
      if (avgaffs <= 0) ;
      /* If avg selected affs are <= target/S7, set new scale to high
      *  limit, because we could otherwise generate a negative scale
      *  multiplier or get a divide check (not good).  Still must
      *  broadcast to waiting PARn's.  User intervention will be
      *  needed to fix this celltype.  */
      else if (avgaffs <= SRA(paut->astta,7))
         paut->asmul[iscl] = paut->asmximm, ++ncap;
      /* Calculate ratio of astta/(avg selected affs) (S24), used
      *  both for immediate and later rescaling */
      else
         paut->asmul[iscl] = dsrsjqe(paut->astta, FBsc, avgaffs, jeca);

      /* Keep track of calculated scale before KAUT_DNO for stats */
      newscl = paut->asmul[iscl];

      /* Apply 'scale activity down only' option */
      if (lkaut & KAUT_DNO && paut->asmul[iscl] > S24)
         paut->asmul[iscl] = S24, ++ncap;

      /* If doing statistics, record largest and smallest raw newscl */
      if (!(il->Ct.kstat & KSTNS)) {
         psst = psst0 + iscl;
         if (newscl < psst->loascl) psst->loascl = newscl;
         if (newscl > psst->hiascl) psst->hiascl = newscl;
         psst->ncaps = jaul(psst->ncaps, ncap);
         }

      if (qdbg) {
         convrt("(P1,'   HTN scale = ',B24IJ8.5,', avgaffs = ',"
            "B20/27IJ10.5,', astta = ',B20/27IJ10.5,', nslc = ',UJ8,"
            "', mavg = ',IJ8)", paut->asmul+iscl, &avgaffs,
            &paut->astta, &nslc, &mavg, NULL);
         }
      } /* End computing HTN scale */

   else if (qdbg) {
      convrt("(P1,'   avgaffs = ',B20/27IJ10.5,', astta = ',B20/27IJ"
         "10.5,', nslc = ',UJ8,', mavg = ',IJ8)", &avgaffs,
         &paut->astta, &nslc, &mavg, NULL);
      }

/*---------------------------------------------------------------------*
*                    Calculate scale by HPN method                     *
*---------------------------------------------------------------------*/

   if (lkaut & KAUT_HPN) {

      si64 sumdep = jesl(0);  /* Sum of unscaled (depolarizing) terms */
      si64 sumhyp = jesl(0);  /* Sum of scaled (hyperpolarizing) data */
      ui32 ncap = 0;          /* Count for psst->ncaps (which
                              *  will not exist if CTPNS)  */

      /* Cannot use lkaut here--KAUT_IMM bit is cleared */
      iscl = (paut->kaut & KAUT_WTA) ? AS_HYPB : AS_GHTB;
      oldscl = paut->asmul[iscl];

      for (psl2=psl1,insl=nsl1; insl<=nsl2; psl2=psl2->pnsl,++insl) {
         sumdep = jasl(sumdep, psl2->tdepol);
         sumhyp = jasl(sumhyp, psl2->thypol);
         }

      /* If there are no data, just leave the scale unchanged, as
      *  with HTN.  If a negative scale is calculated, this implies
      *  changing hyperpolarizing inputs into depolarizing.  Possibly
      *  these should be set to 0, but current policy is to let them
      *  work, but count and report them, as user needs to increase
      *  scale of depolarizing inputs to correct the problem.  */
      if (qsw(sumhyp))
         paut->asmul[iscl] = dsrswwje(jrsw(jmsw(mavg, paut->astta),
            sumdep), sumhyp, FBsc, jeca);

      /* Keep track of calculated scale before KAUT_DNO for stats */
      newscl = paut->asmul[iscl];

      /* Apply 'scale activity down only' option */
      if (lkaut & KAUT_DNO && paut->asmul[iscl] < 0)
         paut->asmul[iscl] = 0, ++ncap;

      /* If doing statistics, record largest and smallest raw newscl */
      if (!(il->Ct.kstat & KSTNS)) {
         psst = psst0 + iscl;
         if (newscl < psst->loascl) psst->loascl = newscl;
         if (newscl > psst->hiascl) psst->hiascl = newscl;
         psst->ncaps = jaul(psst->ncaps, ncap);
         }

      if (qdbg) {
         sumdep = drswq(sumdep, mavg);
         sumhyp = drswq(sumhyp, mavg);
         convrt("(P1,'   HPN scale = ',B24IJ8.5,', avgdep = ',"
            "B20/27IW10.4,', avghyp = ',B20/27IW10.4,', N = ',IJ8)",
            paut->asmul+iscl, &sumdep, &sumhyp, &mavg, NULL);
         }

      } /* End computing HPN scale */

/* At this point, new scale is in paut->asmul [AS_GHTB or AS_HYPB].
*  Running average is performed only on the first scale, which
*  is either the only scale or the scale that will be applied
*  to depolarizing inputs when both scales are in use.
*  See design notes, p. 67--decision to use multiplicative vs.
*  additive scale updating depends on whether inputs to this
*  cycle were scaled or not, and we are trying the idea of no
*  damping, no asmxd test after cycle 1 of a trial.  */

   newscl = paut->asmul[iscl];

   /* Calculate running average new scale (unless KAUT_UPD bit is off
   *  because doing fast start, or this is first cycle of a trial in
   *  OPT=T mode, or after first cycle in OPT=W mode, or adamp is 1.0).
   *  Combine multiplicatively if inputs were scaled, else additively.
   */
   if (lkaut & KAUT_UPD) {
      if (ccyc1 == 1) {
         if (lkaut & KAUT_ANY) {
            si32 tt = S24 + mrssl(paut->adamp, newscl-S24, FBdf);
            newscl = mrssle(oldscl, tt, FBsc, jeca); }
         else newscl = oldscl +
            mrssl(paut->adamp, newscl-oldscl, FBdf);
         }
      else if (lkaut & KAUT_ANY)
         newscl = mrssle(newscl, oldscl, FBsc, jeca);

      /* Do not change by more than + or - asmxd */
      if (newscl > oldscl) {
         si32 tbig = mrssle(oldscl, S24 + paut->asmxd, FBsc, jeca);
         if (newscl > tbig) newscl = tbig;
         }
      else if (newscl < oldscl) {
         si32 tsma = mrssle(oldscl, S24 - paut->asmxd, FBsc, jeca);
         if (newscl < tsma) newscl = tsma;
         }
      } /* End calculating running average */

   /* Apply immediate rescaling limit here on Node 0 -- early
   *  experience suggested this was only useful in this case...  */
   if (lkaut & KAUT_APP) {
      ui32 ncap = 0;
      if      (newscl > paut->asmximm) newscl = paut->asmximm, ++ncap;
      else if (newscl < paut->asmnimm) newscl = paut->asmnimm, ++ncap;
      if (!(il->Ct.kstat & KSTNS))
         psst0->ncaps = jaul(psst0->ncaps, ncap);
      }

   paut->asmul[iscl] = newscl;

   if (qdbg) {
      convrt("(P1,'   After mangling, new scale = ',B24IJ8.5)",
         &newscl, NULL);
      }
#endif

/* If parallel, distribute to comp nodes.  This is currently the
*  only material broadcast possibly on every cycle, and we need
*  to apply the scale before input to any downstream layers, so
*  there is nothing to consolidate it with.  */
#ifdef PAR
#ifdef PARn
   iscl = ((lkaut & (KAUT_WTA|KAUT_HPN)) == (KAUT_WTA|KAUT_HPN)) ?
      AS_HYPB : AS_GHTB;
#endif
   blkbcst(paut->asmul, ttloc(IXasmul_type), 1, AUTSCL_MSG);
#endif

   /* On all nodes, if just one scale, copy it to asmul[AS_HYPB]
   *  to save testing later.  */
   if (iscl != AS_HYPB) paut->asmul[AS_HYPB] = paut->asmul[AS_GHTB];
   } /* End if KAUT_HTN or KAUT_HPN */

/* Now all nodes have the new scale.  If requested, apply it to
*  the waiting new responses in il->ps2 and update sbar and qbar.  */
#ifndef PAR0
   if (lkaut & KAUT_APP) {
      struct AUTSSL *pasl = CDAT.passl0;
      s_type *psi = il->ps2;
      rd_type *plsd = il->prd;
      si32 ntrunchi = 0, ntrunclo = 0;
      si32 rnd1 = paut->asrsd;
      int icell, celllim = il->mcells;
      int qwta = (lkaut & (KAUT_WTA|KAUT_HPN)) == (KAUT_WTA|KAUT_HPN);
      newscl = paut->asmul[iscl];
#ifdef PAR
      if (qwta && il->No.noikadd & NFL_GENN)
         /* Make nsd deterministic */
         udevwskp(&il->No.nsd, jslsw(jesl(il->locell),1));
#endif
      for (icell=0; icell<celllim; ++icell,psi+=il->lsp) {
         si32 wksi;
         if (qwta) {

/* Here is test code for WTA option.  If this is successful, the
*  entire set of response functions from d3go should be called
*  into play here.  For this test, just assume tanh is used.
*  The AUTSSL data are accessed in array order, not sorted order.  */

            wksi = pasl->tdepol +
               mrssle(pasl->thypol, newscl, FBsc, jeca);
            ++pasl;
            if (il->ctf & CTFDR) {
               /* STEP response */
               if (wksi < il->Ct.pt && wksi > il->Ct.nt) wksi = 0;
               }
            else {
               /* KNEE response */
               if (wksi >= il->Ct.pt) wksi -= il->Ct.pt;
               else if (wksi <= il->Ct.nt) wksi -= il->Ct.nt;
               else wksi = 0;
               }
            /* Noise added if added before response function */
            if (il->No.noikadd & NFL_ADDV) {
               si32 tsum;
               CDAT.noise = d3noise(
                  il->No.nmn, il->No.nsg, CDAT.nmodval, il->No.nsd);
               jasjem(tsum, wksi, CDAT.noise, jeca);
               wksi = tsum; }
            if ((enum RespFunc)il->rspmethu == RF_TANH)
               wksi = (si32)(il->ctwk.go.fstanh *
                  tanhf((float)wksi/dS27));
            /* Noise added if added after response function */
            if (il->No.noikadd & NFL_ADDR) {
               si32 tsum;
               CDAT.noise = d3noise(
                  il->No.nmn, il->No.nsg, CDAT.nmodval, il->No.nsd);
               jasjem(tsum, wksi, CDAT.noise, jeca);
               wksi = tsum; }
            if (il->Dc.omegau) {
               /* We have to assume CDAT.pers, based on old Vm, is on
               *  a scale not too different from the current scale,
               *  as there is no obvious way to correct it.  Probably
               *  best not to combine these options.  */
               si32 tsum;
               jasjem(tsum, wksi, CDAT.pers,
                  (int)(il->iovec + CTO_DCY));
               wksi = tsum;
               }
            }
         else {
            d3gtjs2(wksi, psi);
            wksi <<= (FBwk-FBsi);
            wksi = msrsle(wksi, newscl, -FBsc, jeca);
            }
         /* Force response into 16-bit range */

         if (wksi > RP->gsimax) {
            wksi = RP->gsimax, ntrunchi += 1;
            }
         else if (wksi < RP->gsimin) {
            wksi = RP->gsimin, ntrunclo += 1;
            }
         /* Round and shift to S7/14 */
         wksi = (wksi + (1 << (FBwk-FBsi-1))) >> (FBwk-FBsi);
         d3ptl2(wksi, psi);

/* Note:  To minimize subroutine call overhead, this sbar,qbar
*  update code was copied from d3go rather than being put in a
*  separate routine callable from both places.  */

         if (ctexists(il,SBAR)) {
            rd_type *psbr = plsd + il->ctoff[SBAR];
            long sbar1;
            if (il->ctf & CTFDOFS)
               sbar1 = wksi;
            else {
               d3gtjs2(CDAT.sbar, psbr);
               sbar1 = (long)il->Dc.sdamp*(wksi - CDAT.sbar) +
                  (rnd1 >> Ss2hi);
               sbar1 = SRA(sbar1, FBdf) + CDAT.sbar;
               }
            d3ptl2(sbar1, psbr);
            }
         if (ctexists(il,QBAR)) {
            rd_type *pqbr = plsd + il->ctoff[QBAR];
            long qbar1 = wksi;
            if (il->orkam & KAMBCM)
               qbar1 *= qbar1, qbar1 = SRA(qbar1,(FBsi+FBsi));
            if (!(il->ctf & CTFDOFS)) {
               d3gtjs2(CDAT.qbar, pqbr);
               qbar1 = (long)il->Dc.qdamp*(qbar1 - CDAT.qbar) +
                  (rnd1 & (S15-1));
               qbar1 = SRA(qbar1, FBdf) + CDAT.qbar;
               }
            d3ptl2(qbar1, pqbr);
            }
         plsd += uw2zd(il->lel);
         udevskip(&rnd1, 4);
         } /* End cell loop */
#ifdef PAR
      if (qwta && il->No.noikadd & NFL_GENN)
         /* Make nsd deterministic */
         udevwskp(&il->No.nsd, jslsw(jesl(il->nelt-il->hicell),1));
#endif
      /* Store truncation counts if doing statistics */
      if (il->ctf & DIDSTATS)
         il->CTST->sover += ntrunchi, il->CTST->sunder += ntrunclo;
      } /* End rescaling cell responses */
#endif

   } /* End d3autsc() */


#ifndef PARn
/*=====================================================================*
*                               d3aprt                                 *
*                                                                      *
*  Print autoscale multipliers (currently DP=A on CYCLE card)          *
*=====================================================================*/

void d3aprt(void) {

   if (RP0->nautsc) {
      struct CELLTYPE *il;
      char line[28+2*LSNAME];
      int ias,nscl;
      /* One line for every four celltypes plus one for title */
      if (RP->kdprt & PRSPT) spout(((int)RP0->nautsc+7)>>2);
      cryout(RK_P2, "0Autoscale multipliers (at start of trial):",
         RK_LN2, NULL);
      for (il=RP->pfct,ias=0; il; il=il->pnct) {
         if (!il->kautu) continue;
         if ((ias++ & 3) == 0) cryout(RK_P2, " ", RK_LN1+1, NULL);
         nscl = (int)il->nauts;
         sconvrt(line, "(3XA" qLSN ",XA" qLSN ",2X,RJB24IJ9.<5)",
            il->pback->sname, il->lname, &nscl, il->pauts->asmul,
            NULL);
         cryout(RK_P2, line, RK_CCL, NULL);
         } /* End CELLTYPE loop */
      if (ias & 3) cryout(RK_P2, "\0", RK_LN0+RK_FLUSH+1, NULL);
      } /* End if nautsc */
   } /* End d3aprt() */
#endif
