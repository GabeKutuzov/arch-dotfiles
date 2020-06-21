/* (c) Copyright 1997-2012, Neurosciences Research Foundation, Inc. */
/* $Id: d3news.c 52 2012-06-01 19:54:05Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                               d3news                                 *
*                                                                      *
*  Carry out any final checking and initialization required on node 0  *
*  before each new trial series begins.  This routine contains code    *
*  previously performed unnecessarily in d3go() before each trial.     *
*  Main pgm must set RP->CP.outnow to OUT_USER_QUIT on iexit errors    *
*  for proper network saving, graphics termination, etc. if run is     *
*  already in progress.  Checks on variables that can be read from     *
*  Group II or CHANGE cards can go here to avoid repetitive code.      *
************************************************************************
*  V8A, 02/20/97, GNR - New routine                                    *
*  Rev, 09/28/97, GNR - Independent dynamic allocations per conntype   *
*  Rev, 01/05/98, GNR - Add PPF.  2/18/98 - Add upsahp                 *
*  V8B, 07/21/01, GNR - Postprocess DETAIL PRINT for numbered CLISTS   *
*  Rev, 08/26/01, GNR - Group 3 changeable graph inits from d3tchk()   *
*  Rev, 03/02/02, GNR - New PROBE semantics--allow cell list sharing   *
*  V8C, 06/28/03, GNR - Cell responses in millivolts                   *
*  V8D, 03/27/05, GNR - Add conductances and ions                      *
*  Rev, 03/16/07, GNR - Add value event checking                       *
*  ==>, 01/08/08, GNR - Last mod before committing to svn repository   *
*  Rev, 03/15/08, GNR - Avoid divide by 0 error when ix->Cn.scl == 0   *
*  Rev, 12/31/08, GNR - Replace jm64sh with mssle                      *
*  V8E, 01/19/09, GNR - Set flags for d3chng-able Izhikevich options,  *
*                       warning, not error, on plot color etc. probs.  *
*  Rev, 09/06/09, GNR - Add Brette-Gerstner (aEIF) neurons             *
*  V8G, 08/13/10, GNR - Add AUTOSCALE, move wsmnax calc to d3go()      *
*  V8H, 12/27/10, GNR - EFFECTOR->EFFARB, recheck nelt overflow        *
*  Rev, 04/01/11, GNR - Add warnings for GDEFER, MDEFER too large      *
*  Rev, 04/09/12, GNR - Set MODVAL mdefer to min(MODBY) but 0 if decay *
*  Rev, 05/09/12, GNR - Add AUTOSCALE OPT=T                            *
*  Rev, 05/25/12, GNR - Add extended gconn warning from d3inhs()       *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "d3global.h"
#include "clblk.h"
#ifdef BBDS
#include "bbd.h"
#endif
#include "rocks.h"
#include "rkxtra.h"
#include "rkarith.h"
#include "bapkg.h"

/*=====================================================================*
*                                t2ups                                 *
*  Given the time for a process (or the half-time on scale S1),        *
*  calculate upsilon, the multiplier for exponential growth (S28).     *
*  upsilon = (ln(2)/halftime) = (2ln(2)/time) to (S28)                 *
*=====================================================================*/

#define LN2S29 3.721305590E8  /* Ln(2) on scale S29 */

static si32 t2ups(unsigned int t) {

   if (t == 0)
      return 0;
   else if (t == 1)
      return S28;
   else
      return (si32)(LN2S29/(double)t + 0.5);
   }

/*=====================================================================*
*                               d3news                                 *
*=====================================================================*/

void d3news(void) {

   struct REPBLOCK *ir;
   struct CELLTYPE *il;
   struct CONNTYPE *ix,*ixt,**pixn,**pixt;
   struct INHIBBLK *ib;
   struct MODBY    *imb;
   struct MODVAL   *imv;
   struct CONDUCT  *pcnd;
   struct VBDEF    *ivb;
   struct PPFDATA  *ppfd;
   struct RFRCDATA *prf;
   struct TRTIMER  *ptt;
   int    preamp_enable;         /* ON if amplif or das active */

   /* Amp case codes in same order as amp_case_scales in d3go() */
   static ui32 amp_case_codes[NTAC] = {
      KAM2I, KAM2J, KAM3, KAM4, KAMHB };

/* Turn off SAVENET output if this is a replay */

   if (RP->CP.runflags & RP_REPLAY) RP->ksv &= ~KSVOK;

/* Combine plot options from Group I and Group III */

   RP->kplot = RP0->kplt1 | RP0->kplt2;

/* Set virtual and real MODBY blocks to allow mdefer */
   for (imv=RP->pmvv1; imv; imv=imv->pmdv)
      imv->Mdc.mdefer = UI16_MAX;
   for (il=RP->pfct; il; il=il->pnct) {
      for (imv=il->pmrv1; imv; imv=imv->pmdv)
         imv->Mdc.mdefer = UI16_MAX;
      }

/* Repertoire-level initialization */

   RP0->n0flags &= ~(N0F_ANYAMP|N0F_ANYKCG); /* No amp or KAMCG */
   RP->CP.runflags &= ~RP_REPPR; /* Track need for rep print */
   RP->mxngtht = 0;              /* Space for autoscale */

   for (ir=RP->tree; ir; ir=ir->pnrp) {

      ui32 orkctp = 0;
      ui16 andopt = ~0;

/* Cell-type-level initialization */

      for (il=ir->play1; il; il=il->play) {

         /* Clear ctf flags that may be reset below */
         il->ctf &= ~(CTFDR|CTFCN|CTFNC|INCLPERS);

         /* Set d3go step mode if OPT=D or RF=STEP (allowing
         *  these options to be switched independently)  */
         if (il->Ct.rspmeth == RF_STEP || il->Ct.ctopt & OPTDR)
            il->ctf |= CTFDR;

         /* If there is autoscaling, accumulate space needed to find
         *  ngtht largest responses and copy kaut used codes.  */
         il->kautu = 0;
         if (il->pauts) {
            struct AUTSCL *paut = il->pauts;
            if (paut->ngtht > 0) {
               il->kautu = paut->kaut;
               if (paut->ngtht > RP->mxngtht)
                  RP->mxngtht = paut->ngtht;
               }
            }

         /* Update SBAR,QBAR as soon as response is known unless
         *  doing immediate rescaling.  */
         il->ksqbup = 0;
         if (ctexists(il,SBAR) || ctexists(il,QBAR)) {
            il->ksqbup = (il->kautu & (KAUT_IMM|KAUT_ET)) ?
               KSQUP_AS : KSQUP_GO;
            }

         /* If there is AHP, and decay is not abrupt,
         *  calculate the growth coefficient */
         if (prf = il->prfrc) {     /* Assignment intended */
            prf->upsahp = (prf->refrac > 1 && !(il->Ct.ctopt & OPTAB)) ?
               t2ups(prf->refrac) : 0;
            }

         /* Indicate need for persistence to be included in normal
         *  connection sums for voltage-dependent connection types */
         if (RP->compat & OLDVDC && !il->phshft && !il->Dc.siscl)
            il->ctf |= INCLPERS;

         /* Indicate need for centroid calc, phase seed update */
         if (il->phshft) {
            struct PHASEDEF *pdd = il->pctpd;
            if (pdd->phimeth == PHI_CENTROID ||
                  pdd->simeth == SI_CENTROID) il->ctf |= CTFCN;
            if (pdd->phimeth != PHI_CONST)    il->ctf |= CTFNC;
            }

         /* Remove DPRINT and PROBE references to CLIST cards.  It is
         *  essential to delete all ptrs to cell list stuff (some may
         *  have been deleted by CLIST cards), then put back just the
         *  ones that exist (see below) to avoid membcst ptr errors.
         *  Leave DPRINT neg clnum's--they do not have DPRINT blocks.
         */
         if (il->ctclid[CTCL_DPRINT] >= 0) {
            il->ctclid[CTCL_DPRINT] = 0;
            il->dpclb = NULL; }
         il->pctprb = NULL;
         il->ctclid[CTCL_PRBSL] = 0;

         /* If a detail print timer was not specified, use default */
         if (il->jdptim <= 0) il->jdptim =
            (RP0->n0flags & N0F_DPCTRL) ? gettimno(PRTRT, YES) : TRTN1;

         /* Set s(i) plot shape used from shape entered, but if no
         *  shape option was entered, use older kctp = Q,3 codes.
         *  Change 3Bar to 2Bar or just Square if required vari-
         *  ables do not exist.  Change Circle, but not other ops,
         *  to Square if requested by global PLSQR option.  Then,
         *  if a shape was specified, harmlessly turn on orkctp
         *  KRPSQ bit so code below will request bubble plot at
         *  repertoire level.  Keeping ksiplu and ksiple separate
         *  allows options to be properly combined no matter what
         *  order they are entered across multiple CYCLE cards.  */
         il->ksiplu = il->Ct.ksiple;
         if ((enum SiShapes)il->ksiplu == SIS_None) {
            if (il->Ct.kctp & KRPSQ) il->ksiplu = (byte)SIS_Square;
            if (il->Ct.kctp & KRP3B) il->ksiplu = (byte)SIS_3Bar; }
         if ((enum SiShapes)il->ksiplu == SIS_3Bar) {
            if (!(il->ctf & CTFHN)) il->ksiplu = (byte)SIS_Square;
         /* Revised code, 06/09/09, 3rd bar now plots A(1) if it
         *  exists instead of noise modulation.  */
#if 1
            else if (il->nct <= 0) il->ksiplu = (byte)SIS_2Bar;
#else
            else if (!(il->ctf & CTFMN)) il->ksiplu = (byte)SIS_2Bar;
#endif
            }
         else if ((enum SiShapes)il->ksiplu == SIS_2Bar &&
               !(il->ctf & CTFHN)) il->ksiplu = (byte)SIS_Square;
         if ((enum SiShapes)il->ksiplu != SIS_None) orkctp |= KRPSQ;
         /* PLIM SQUARE by itself does not turn on bubble plot */
         if ((enum SiShapes)il->ksiplu <= SIS_Circle &&
               RP->kplot & PLSQR) il->ksiplu = (byte)SIS_Square;

         /* Set s(i) plot color source from CELLTYPE COLOR option if
         *  any, otherwise use default */
         il->ksipcu = il->Ct.ksipce & SIC_MASK;
         if ((enum SiColors)il->ksipcu == SIC_None) {
            if (il->phshft)               il->ksipcu = SIC_Phase;
            else if (strncmp(il->Ct.sicolor, "SEQUENCE", COLMAXCH))
                                          il->ksipcu = SIC_Fixed;
            else                          il->ksipcu = SIC_Si;
            }

         /* If a color source was requested that does not exist,
         *  issue a warning and switch to Si coloring.  (Older
         *  code in d3chng() generated an error in these cases.
         *  Before first series, d3allo() turns on SBAR, QBAR if
         *  needed for coloring plots, but in later series this
         *  is impossible.)  */
         {  int kbad = FALSE;
            switch (il->ksipcu) {
            case SIC_SBar:
               if (!ctexists(il, SBAR)) kbad = TRUE; break;
            case SIC_QBar:
               if (!ctexists(il, QBAR)) kbad = TRUE; break;
            case SIC_Phase:
               if (!il->phshft)         kbad = TRUE; break;
            case SIC_Hist:    /* Temp restriction */
               if (!(RP->kplot&PLSUP))  kbad = TRUE; break;
            case SIC_Izhu:
               if (!ctexists(il, IZVU)) kbad = TRUE; break;
               } /* End ksipcu switch */
            if (kbad) {
               cryout(RK_P1, "0-->WARNING:  Cell plot coloring set to "
                  "use s(i) for ", RK_LN2, fmtlrlnm(il), RK_CCL, ": "
                  "specified variable does not exist.", RK_CCL, NULL);
               il->ksipcu = SIC_Si;
               }
            } /* End kbad local scope */

         /* Collect all kctp options */
         orkctp |= il->Ct.kctp;
         andopt &= il->Ct.ctopt;

/* Connection-type-level initialization */

         il->orkam = 0;
         pixn = &il->pctt1;      /* Start normal and          */
         pixt = &ixt;            /* voltage-dependent threads */
         for (ix=il->pct1; ix; ix=ix->pct) {

            /* Connection types are rethreaded so normals can be
            *  processed in d3go before voltage-dependent types.
            *  This is done here (rather than, say, in d3tchk) so
            *  in principle threading can change at Group III time.
            *  Ptrs are translated to node addresses by membcst.  */
            if (ix->Cn.cnopt & NOPVA)
               *pixt = ix, pixt = &ix->pctt;
            else
               *pixn = ix, pixn = &ix->pctt;

            /* Values used for threshold testing Sj */
            ix->effet = ix->Cn.et   ? ix->Cn.et   - 1 : 0;
            ix->effnt = ix->Cn.etlo ? ix->Cn.etlo + 1 : 0;
            if (ix->Cn.cnopt & NOPKR)
               ix->subet = ix->Cn.et, ix->subnt = ix->Cn.etlo;
            else
               ix->subet = ix->subnt = 0;

            /* Calculate PPF growth and decay coefficients */
            if (ppfd = ix->PP) {    /* Assignment intended */
               if (ppfd->ppft == -1)
                  ppfd->ppft = il->Ct.pt>>(FBwk-FBsi);
               ppfd->upsp = t2ups(ppfd->htup);
               ppfd->taup = t2ups(ppfd->htdn);
               }

            /* Set KAMTNU, KAMVNU bits for easier tests in d3go */
            kamchk(ix);

            /* Maintain up-to-date orkam */
            il->orkam |= ix->Cn.kam;

            /* Set to do or omit decay and amplification */
            ix->dcybi = NO_DECAY;
            ix->ampbi = NO_AMP;
            ix->finbi = NO_FIN_AMP;

            /* Do no more if KAMMF bit is set */
            if (ix->Cn.kam & KAMMF) continue;

            /* Setup if there is any decaying to be done */
            if (ix->Cn.gamma && ix->Cn.kdecay&(KINDE|KINDB|KINDI)) {
               if      (ix->Cn.kdecay & KINDE) ix->dcybi = DECAYE;
               else if (ix->Cn.kdecay & KINDB) ix->dcybi = DECAYB;
               else if (ix->Cn.kdecay & KINDI) ix->dcybi = DECAYI;
               ix->finbi = NULLAMP;  /* Perform at least null amp */
               }

            /* Setup if there is any amplifying to be done */
            preamp_enable = OFF;
            ix->amptt = 0;
            if (ix->Cn.kam & ANYAMASK) {
               if (ix->Cn.delta) {        /* Is delta non-zero? */
                  int i;
                  preamp_enable = ON;
                  ix->finbi = (ix->Cn.kam & KAMUE) ? ELJTFIN : NORMAMP;
                  for (i=0; i<NTAC; i++) {
                     if (ix->Cn.kam & amp_case_codes[i])
                        { ix->amptt = i; break; }
                     }
                  }
               if ((ix->Cn.kam & KAMDS) &&
                  !(RP->CP.runflags & (RP_NOSTAT|RP_NOCYST)) &&
                  !(il->Ct.kctp & CTPNS)) preamp_enable = ON;
               }

            if (preamp_enable) {
               if (!(ix->Cn.kam & DIIAMASK))
                  /* Error:  Unimplemented synaptic model implied */
                  d3exit(LEIFAMP_ERR, fmturlnm(il), (long)ix->ict);
               else if (ix->Cn.kam & KAMUE)
                  ix->ampbi = ELJTAMP;
               else if (ix->Cn.kam & KAMTS)
                  ix->ampbi = TICKAMP;
               else if (ix->Cn.kam & KAMSA)
                  ix->ampbi = SLOWAMP;
               else
                  ix->ampbi = HEBBAMP;

               /* Make table of delta*phi for amplifier if delta has
               *  changed--not rounding here, will round in d3go */
               if (ix->Cn.delta != ix->phitab[0]) {
                  register long c,c2;
                  register long *pwork = ix->phitab;
                  for (c2=c=0; c<=127; c+=4,c2=c*c) *pwork++ = mssle(
                     S28+c2*c2-(c2<<15), ix->Cn.delta, -28, OVF_GEN);
                  } /* End if delta changed */

               RP0->n0flags |= N0F_ANYAMP;
               } /* End amplification setup */

            /* Check value index for value-modulated amplification--
            *  seems wise to check whether or not actually
            *  amplifying.  */
            if ((ix->Cn.kam & (KAMVM|KAMUE)) &&
               ((ix->Cn.vi > RP->nvblk) || (ix->Cn.vi < 1)))
                  d3exit(VMAMPL_ERR, fmturlnm(il), (long)ix->ict);
            if (cnexists(ix,CIJ0) &&
               ((ix->Cn.dvi > RP->nvblk) || (ix->Cn.dvi < 0)))
                  d3exit(VMAMPL_ERR, fmturlnm(il), (long)ix->ict);

            /* Determine number of times rseed should be updated
            *  for each synapse and each cell that is amplified
            *  (update may be performed in sequence II or III).  */
            ix->upcij1s = preamp_enable ?
               ((cnexists(ix,CIJ0) && ix->Cn.rho) ? 2 : 1) : 0;
            ix->upcij = ix->nc*(long)ix->upcij1s;

            /* If there is both phase and ADECAY, turn off the
            *  ADECAY, as current aeff allocation and d3go code
            *  cannot handle this case.  */
            if (ix->phopt && ix->Cn.ADCY.kdcy) {
               convrt("(P1,'0-->WARNING:  ADECAY turned off "
                  "for ',A" qLXN ",', conntype ',J0IH4/"
                  "'    because incompatible with phase.')",
                  fmtlrlnm(il), &ix->ict, NULL);
               ix->Cn.ADCY.kdcy = NODECAY;
               }

            /* If both excit and inhib AffD64 exist, allow NOPCX
            *  option, otherwise nullify it -- no warning needed  */
            ix->cnflgs &= ~DOCEI;
            if (ix->Cn.cnopt & NOPCX && npsps((int)ix->cssck))
               ix->cnflgs |= DOCEI; 

            } /* End ix loop */

         /* Finish the rethreading by linking the volt-deps on
         *  the end of the normals.  */
         *pixt = NULL, *pixn = ixt;

/* GCONN initialization.
*  N.B.  Defined action:  IBOPFS for GCONN is applied in cycle 1 if
*     source comes sooner in update sequence, otherwise in cycle 2
*     (error if ntc == 1), but gdefer refers to the absolute cycle
*     number, regardless of update sequence.  We can set gfscyc
*     large if no GDCY to save some tests in d3inhb()/d3gcon().
*  If IBOPFS, warning if gdefer < 1 because can't apply GCONN before
*     it is computed.  Also test for gdefer values too large.
*  Also warning if gbcm * (ntc-1) >= 1S24 as scale would go negative.
*  Rev, 04/19/12, GNR - Code reorganized for easier understanding.  */

         for (ib=il->pib1; ib; ib=ib->pib) {
            char wmsg[48+2*LSNAME];    /* Just code it once */
            sconvrt(wmsg, "(' for GCONN type ',J1IH4,3Hto J0A" qLXN ")",
               &ib->igt, fmturlnm(il), NULL);
            if (ib->GDCY.omega == 0) {
               ib->gcdefer = ib->gdefer;
               ib->gfscyc = UI16_MAX;
               }
            else if (!(ib->ibopt & IBOPFS)) {
               ib->gcdefer = 0;
               ib->gfscyc = UI16_MAX;
               }
            else {
               ib->gcdefer = il->upseq <= ib->pisrc->upseq;
               ib->gfscyc = ib->gcdefer + 1;
               if (ib->gdefer < ib->gcdefer)
                  cryout(RK_P1, "0-->WARNING:  gdefer < 1 w/OPT=F",
                     RK_LN2, wmsg, RK_CCL, ", GCONNs = 0 in cycle 1.",
                     RK_CCL, NULL);
               if (ib->gfscyc > RP->ntc1)
                  cryout(RK_E1, "0***ntc1 TOO FEW FOR OPT=F", RK_LN2,
                     wmsg, RK_CCL, NULL);
               }
            if (jmsle(ib->gbcm, (si32)RP->ntc1-1, OVF_GEOM) > S24)
               cryout(RK_P1, "0-->WARNING:  gbcm goes negative",
                  RK_LN2, wmsg, RK_CCL, NULL);
            if (ib->gdefer >= RP->ntc1)
               cryout(RK_P1, "0-->WARNING:  gdefer >= ntc", RK_LN2,
                  wmsg, RK_CCL, ", GCONNs have no effect.", RK_CCL,
                  NULL);
            if (ib->ibopt & IBOPTX) {
               /* Area of outer band with OPT=X is size of source
               *  - size of inner square--be sure it is positive.  */
               struct CELLTYPE *jl = ib->pisrc;    /* Ptr to source */
               long edge = 2*ib->ngb*(ib->nib-2) + 1;
               if (jl->nelt - edge*edge*jl->nel <= 1)
                  cryout(RK_E1, "0***EXTENDED GCON EXCEEDS LAYER SIZE",
                     RK_LN2, wmsg, RK_CCL, NULL);
               }
            } /* End ib loop */

/* MODBY initialization.
*  N.B.  Defined action:  IBOPFS for MODUL, unlike GCONN, always can
*     be applied in cycle 1 because sums are done after Si calc in
*     d3go().  As with GCONN, gdefer refers to the absolute cycle
*     number, regardless of update sequence.
*  Set Mdc.mdefer to minimum(mdefer in attached MODBYs) but 0 if any
*  of these have decay.  Also test for mdefer values too large. */

         for (imb=il->pmby1; imb; imb=imb->pmby) {
            imv = imb->pmvb;
            if (imb->MDCY.omega) imv->Mdc.mdefer = 0;
            else if (imb->Mc.mdefer < imv->Mdc.mdefer)
               imv->Mdc.mdefer = imb->Mc.mdefer;
            if (imb->Mc.mdefer >= RP->ntc1)
               convrt("(P1,'0-->WARNING:  mdefer >= ntc for ',J0A" qLXN
                  ",', MODUL type ',J0IH4,', MODUL has no effect.')",
                  fmtlrlnm(il), &imb->mct, NULL);
            } /* End imb loop */

/* Conductance initialization.
*  (The tests and setups located here are those that operate on
*  values that are allowed to change at Group III time, therefore,
*  d3cchk() can not be guaranteed to catch any problems.)  */

         for (pcnd=il->pcnd1; pcnd; pcnd=pcnd->pncd) {

            /* If LINEAR, be sure vmax > vth */
            if (pcnd->kcond == CDTP_LINEAR) {
               if (pcnd->ugt.lin.vth >= pcnd->ugt.lin.vmax)
                  cryout(RK_E1, "0***vth >= vmax FOR CONDUCTANCE \"",
                     RK_LN2, getrktxt(pcnd->hcdnm), "\" ON ", RK_CCL,
                     fmturlnm(il), RK_CCL, NULL);
               else
                  pcnd->ugt.lin.vmmth =
                     (ui32)(pcnd->ugt.lin.vmax - pcnd->ugt.lin.vth);
               } /* End LINEAR test */

            } /* End conductance loop */

         /* Set flag indicating categorization stats needed */
         if (il->orkam & KAMCG) RP0->n0flags |= N0F_ANYKCG;

         /* Set flag indicating print options must be serviced */
         if (il->Ct.kctp & KRPEP) RP->CP.runflags |= RP_REPPR;

         /* Warning if slow, fast average Si are in wrong order */
         if (il->orkam & (KAMSA|KAMZS|KAMTNU) && il->orkam & KAMDQ &&
               il->Dc.sdamp <= il->Dc.qdamp)
            cryout(RK_P1, "0-->WARNING:  SDAMP <= QDAMP for slow "
               "amplif for ", RK_LN2, fmtlrlnm(il), RK_CCL, NULL);

         } /* End il loop */

      /* Set flag indicating print options must be serviced */
      if (ir->Rp.krp & KRPRR) RP->CP.runflags |= RP_REPPR;

      /* Transfer AND of ctopt OPTBR, etc. to repblock */
      ir->andctopt = andopt;

      /* Set bubble plot, no stats if implied by kctp options.
      *  (Note:  There is a separate KRPPL option that turns on
      *  the bubble plot if requested on the REGION card, even
      *  if not requested here.)  */
      ir->Rp.krp &= ~(KRPBP|KRPNS);
      if (orkctp & CTPNS)  ir->Rp.krp |= KRPNS;
      if (orkctp & KRPBPL) ir->Rp.krp |= KRPBP;

      } /* End ir loop */

/* Merge data from DPDATA blocks.  These are for refs to CLISTs,
*  not cell lists on the DETAIL PRINT card, which are processed
*  in d3grp3().  Deferring this from d3grp3() allows us to pick
*  up info for CLISTs that may have been entered after the
*  DETAIL PRINT card.  Of course no effort is made to optimize
*  out redundant changes.  */

   {  struct DPDATA *pdpd;
      struct CLHDR  *pclh;
      for (pdpd=RP0->pdpd1; pdpd; pdpd=pdpd->pndpd) {
         if (pdpd->clnum > 0) {
            pclh = findcl((int)pdpd->clnum, FALSE);
            /* If list is empty, no error--resume using
            *  this list when new cells are supplied.  */
            if (pclh)
               d3clck(pclh, pdpd, CLCK_DPRINT);
            }
         }
      } /* End pdpd, pclh local scope */

/* Link PRBDEF blocks to matching cell types.  If the specified CLIST
*  is not found, omit processing for this series, but keep the PRBDEF,
*  because the CLIST might be added later (PRBDEFs with explicit cell
*  lists are removed in getprobe() if the cell list is removed).
*  Expand PRBDEF's (unless indexed) to arrays with a separate copy for
*  each CLBLK in the CLIST. On entry to this code, all references from
*  cell types to probes have been cleared, so they can be rebuilt in
*  the appropriate priority order.  */

   {  struct PRBDEF   *pprb,**ppprb;
      struct CLHDR    *pclh;
      for (ppprb=&RP->pfprb;
            pprb=*ppprb;      /* Assignment intended */
            ppprb=&(*ppprb)->pnprb) {
         pprb->pclb = NULL;   /* Avoid passing bad ptr to comp nodes */
         pclh = findcl((int)pprb->clnum, FALSE);
         if (pclh && pprb->prbdis == FALSE && pprb->cpp > 0) {
            size_t nblks = pclh->nclb;
            if (pprb->clndx > 0) {
               if (pprb->clndx > nblks)
                  continue;
               nblks = 1;
               }
            else if (pprb->psct && (nblks > 1 ||
                  pprb->psct != findln(&pclh->pfclb->clnm))) {
               cryout(RK_E1, "0***PROBE OF ", RK_LN3,
                  fmturlnm(pprb->psct), LXRLNM,
                  " REFERS TO A NONMATCHING CLIST.", RK_CCL,
                  "    (A CLIST INDEX MIGHT RESOLVE THE MATCH.)",
                  RK_LN0, NULL);
               continue;
               }
            pprb = *ppprb = (struct PRBDEF *)allocprv(pprb,
               nblks, IXstr_PRBDEF, "Probe data");
            d3clck(pclh, pprb, CLCK_PROBE);
            }
         }
      } /* End pprb, ppprb, pclh local scope */

/* Set parameters for plotting value.  (Moved here from d3tchk()
*  now that PLOC card can be entered at d3grp3 time.)  */

   RP0->yvbxht = 0.7*RP0->vplh;
   RP0->yvstrt[0] = RP0->vply[0];
   RP0->yvstrt[1] = RP0->vply[1];
   for (ivb=RP->pvblk; ivb; ivb=ivb->pnxvb) {
      if (ivb->vbopt & VBVOPNP) continue;
      RP0->yvstrt[(ivb->vbopt & VBVOP2C) != 0] += RP0->vplh;
      }

/* Check that VALUE reset events refer to values that exist */

   {  struct VALEVENT *pvev;
      char vnum[8];
      for (pvev=RP0->pvev1; pvev; pvev=pvev->pnve)
            if (pvev->ivdt > RP->nvblk) {
         wbcdwt(&pvev->ivdt, vnum, RK_IORF|RK_LFTJ|RK_NHALF+7);
         cryout(RK_E1, "0***A RESET EVENT REFERS TO "
            "NONEXISTENT VALUE SCHEME ", RK_LN2,
            vnum, RK.length + 1, NULL);
         }
      } /* End pvev local scope */

/* Set scale factors for effector outputs.  Recheck that effector
*  arbor calculations in d3go will not overflow nelt because we
*  are now allowing pea->arbs to change at Group III time.  */

   {  struct EFFARB *pea;
      for (pea=RP0->pearb1; pea; pea=pea->pneb) {
         long tnel,thigh,tnchgs;
         int  iei;
         for (iei=EXCIT; iei<=INHIB; ++iei) {
            if (il = pea->pect[iei]) {    /* Assignment intended */
               if (pea->pefil[iei]) {     /* Using cell list */
                  tnel = ilstitct(pea->pefil[iei], il->nelt);
                  thigh = ilsthigh(pea->pefil[iei]) + 1; }
               else {
                  tnel = il->nelt;
                  thigh = pea->arbw; }
               pea->esclu[iei] = pea->escli[iei]/(float)tnel;
               tnchgs = (long)(max(pea->nchgs,1) - 1);
               if ((thigh + (pea->nchgs-1)*pea->arbs) > il->nelt)
                  cryout(RK_E1, "0***AN EFFECTOR ARBOR FOR ", RK_LN2,
                     fmteffnm(pea), RK_CCL,
                     "EXCEEDS SIZE OF CELL LAYER", RK_CCL, NULL);
               }
            else pea->esclu[iei] = 0;
            } /* End for iei */
         } /* End EFFARB loop */
      } /* End pea local scope */

/* If any timer looks at events or event trials are requested,
*  set needevnt flag accordingly */

   RP0->needevnt &= ~(NEV_TIMR|NEV_EVTRL);
   for (ptt=RP0->ptrt1; ptt; ptt=ptt->pntrt) {
      if (ptt->tmrop & (TRTEVON|TRTEVOFF)) {
         RP0->needevnt |= NEV_TIMR;
         break; }
      }
   if (RP->kevtr) RP0->needevnt |= NEV_EVTRL;

   } /* End d3news() */
