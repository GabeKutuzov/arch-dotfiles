/* (c) Copyright 1997-2018, The Rockefeller University *11115* */
/* $Id: d3news.c 79 2018-08-21 19:26:36Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                               d3news                                 *
*                                                                      *
*  Carry out any final checking and initialization required on node 0  *
*  before each new trial series begins.  This routine contains code    *
*  previously performed unnecessarily in d3go() before each trial.     *
*  Main pgm must set RP->CP.outnow to OUT_ERROR_EXIT on iexit errors   *
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
*  Rev, 06/19/12, GNR - Revise ampbi, finbnow for KAMBCM               *
*  Rev, 06/29/12, GNR - Add qpull, KAMQNB                              *
*  Rev, 12/24/12, GNR - New overflow checking, 02/02/13 mfscyc setup   *
*  Rev, 02/16/13, GNR - Decay CHNGOK checking                          *
*  Rev, 05/16/13, GNR - Add alloc for AUTOSCALE OPT=H|W AUTSSL structs *
*  Rev, 08/22/14, GNR - Si only EXP decay, remove DECAYDEF from DCYDFLT*
*  R63, 11/04/15, GNR - Move volt-dep options to vdopt, add 'A' code   *
*  R64, 12/09/15, GNR - Move pjnwd setup here from d3schk,d3snsa       *
*  Rev, 12/19/15, GNR - Change 'arbs' to 'arbsep[NEXIN]'               *
*  R67, 11/06/16, GNR - Allow >1 probe/celltype                        *
*  Rev, 11/18/16, GNR - Move VCELL src loc calcs from d3lplt neuroanat *
*                       (so VCELL does not have to be broadcast)       *
*  R72, 03/09/17, GNR - Remove krp KRPRR,KRPSP region print options    *
*  R72, 03/14/17, GNR - Add d3cpli calls for layer and neuroanat plots *
*  R72, 04/01/17, GNR - Add PHASE-NEEDLE and ORIENT-NEEDLE shapes      *
*  R74, 09/03/17, GNR - Add GRPAVG and GRPMAX color sources            *
*  R75, 10/04/17, GNR - Compute scaled background noise for TERNARY    *
*  R76, 11/06/17, GNR - Remove vdopt=C, pctt, NORM_SUMS w/cons tables  *
*  R77, 01/30/18, GNR - Add vdmm w/VDOPT=N interaction                 *
*  R77, 02/13/18, GNR - Enhanced d3lplt bars options, sep SIC[EI]CSQ   *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "d3global.h"
#include "jntdef.h"
#include "wdwdef.h"
#include "clblk.h"
#include "rocks.h"
#include "rkxtra.h"
#include "rkarith.h"
#include "bapkg.h"

void d3epsi(void);

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
   struct CONNTYPE *ix;
   struct INHIBBLK *ib;
   struct MODBY    *imb;
   struct MODVAL   *imv;
   struct CONDUCT  *pcnd;
   struct VBDEF    *ivb;
   struct PPFDATA  *ppfd;
   struct RFRCDATA *prf;
   struct EFFARB   *pea;
   struct TRTIMER  *ptt;
   int    preamp_enable;         /* ON if amplif or das active */

   /* Traditional amp case codes and corresponding wayset values */
#define NTAC   5        /* Number of traditional (D3) amp cases */
   static ui32 amp_case_codes[NTAC] = {
      KAM2I, KAM2J, KAM3, KAM4, KAMHB };
   static char amp_case_scales[NTAC][8] = {
      {1,1,0,0,1,0,0,0},         /* Two-way on si */
      {1,0,1,0,1,0,0,0},         /* Two-way on Sj */
      {1,1,1,0,1,0,1,0},         /* Three-way     */
      {1,1,1,1,1,0,1,0},         /* Four-way      */
      {1,0,0,0,1,0,0,0} };       /* Pure Hebbian  */

/* Turn off SAVENET output if this is a replay */

   if (RP->CP.runflags & RP_REPLAY) RP->ksv &= ~KSVOK;

/* Setup of merged kctp options in krp must be done in a separate set
*  of loops before the main d3news code below so various plot defaults
*  get treated properly.  Set bubble plot if implied by kctp options.
*  (Notes:  A bad ksple may get turned off in code below.  krp option
*  KRPPL turns on the bubble plot if requested on the REGION card,
*  even if not requested here.  Setting of KRPNS from CTPNS removed,
*  08/05/17, allow user to have stats on some celltypes.
*/

   for (ir=RP->tree; ir; ir=ir->pnrp) {
      ir->andctopt = ir->andctkst = ~0;
      ir->Rp.krp &= ~(KRPBP|KRPNS|KRPRG);
      for (il=ir->play1; il; il=il->play) {
         if ((enum SiShapes)il->Ct.ksiple != SIS_None)
            ir->Rp.krp |= KRPBP;
         if (il->Ct.kctp & KRPBPL) ir->Rp.krp |= KRPBP;
         if (il->Ct.ctopt & OPTRG) ir->Rp.krp |= KRPRG;
         ir->andctopt &= il->Ct.ctopt;
         ir->andctkst &= il->Ct.kstat;
         }
      if (ir->andctkst & KSTNS) ir->Rp.krp |= KRPNS;
      }  /* End ir loop */

/* Combine plot options from Group I and Group III.
*  If needed, set default positions for superposition plots.  */

   RP->kplot = RP0->kplt1 | RP0->kplt2;
   if (RP->kplot & (PLSUP|PLVTS))
      d3epsi();

/* Set virtual and real MODBY blocks to allow mdefer */
   for (imv=RP->pmvv1; imv; imv=imv->pmdv)
      imv->Mdc.mdefer = UI16_MAX;
   for (il=RP->pfct; il; il=il->pnct) {
      for (imv=il->pmrv1; imv; imv=imv->pmdv)
         imv->Mdc.mdefer = UI16_MAX;
      }

/* Repertoire-level initialization */

   RP0->n0flags &= ~(N0F_ANYAMP|N0F_ANYKCG); /* No amp or KAMCG */
   RP->CP.runflags &= ~RP_REPPR; /* Track need for KCTP=E print */

   for (ir=RP->tree; ir; ir=ir->pnrp) {

/* Cell-type-level initialization */

      for (il=ir->play1; il; il=il->play) {

         int donanat = 0;

         /* Clear ctf flags that may be reset below */
         il->ctf &= ~(CTFDR|CTFCN|CTFNC|CTFPG);
         il->sicop = 0;

         /* Set d3go step mode if OPT=D or RF=STEP (allowing
         *  these options to be switched independently)  */
         if (il->Ct.rspmeth == RF_STEP || il->Ct.ctopt & OPTDR)
            il->ctf |= CTFDR;

         /* Indicate need to add noise to cell responses */
         il->No.noikadd = ((il->No.nmn | il->No.nsg) && il->No.nfr) ?
            il->No.noiflgs : 0;

         /* If there is AHP, and decay is not abrupt,
         *  calculate the growth coefficient */
         if (prf = il->prfrc) {     /* Assignment intended */
            prf->upsahp = (prf->refrac > 1 && !(il->Ct.ctopt & OPTAB)) ?
               t2ups(prf->refrac) : 0;
            }

         /* Indicate need for centroid calc, phase seed update */
         if (il->phshft) {
            struct PHASEDEF *pdd = il->pctpd;
            if (pdd->phimeth == PHI_CENTROID ||
                  pdd->simeth == SI_CENTROID) il->ctf |= CTFCN;
            if (pdd->phimeth != PHI_CONST)    il->ctf |= CTFNC;
            }

         /* Remove DPRINT references to CLIST cards.  It is essential
         *  to delete all ptrs to cell list stuff (some may have been
         *  deleted by CLIST cards), then put back just the ones that
         *  exist (see below) to avoid membcst ptr errors.  Leave
         *  DPRINT neg clnum's--they do not have DPRINT blocks.  */
         if (il->ctclloc[CTCL_DPRINT].clnum >= 0) {
            memset(il->ctclloc+CTCL_DPRINT, 0, sizeof(clloc));
            il->dpclb = NULL; }

         /* If a detail print timer was not specified, use default */
         if (il->jdptim <= 0) il->jdptim =
            (RP0->n0flags & N0F_DPCTRL) ? gettimno(PRTRT, YES) : TRTN1;

         /* Items relating to layer plots.  Set il->Lp according to
         *  final plot position -- d3lplt will account for KRPVT.
         *  We are called too early for RP->kpl to be set up.  */
         if (RP->CP.runflags & RP_OKPLOT && !(il->Ct.kctp & CTPHP)) {
            /* Set up for bubble plot whether or not actually done */
            if (RP->kplot & (PLSUP|PLVTS))
               d3cpli(&il->Lp, il, NULL, ir->aplx, ir->aply,
                  ir->aplw, ir->aplh, KP_LAYER);
            else  /* Ancient defaults for non-superposition plots */
               d3cpli(&il->Lp, il, NULL, 0.5,0.5,7.5,10.0, KP_LAYER);

            /* donanat is 1 if ix->ict must match il->jijpl,
            *  2 to do all conntypes */
            donanat = (il->jijpl < 0) + (0 != (il->Ct.kctp & KRPNPL));
            /* Find cell list if a LPCLIST or TPCLIST was entered */
            {  struct CLHDR *pclh;
               pclh = findcl(&il->ctclloc[CTCL_NANAT], FALSE);
               if (pclh) d3clck(pclh, il, CLCK_NANAT);
               else il->ptclb = NULL;
               pclh = findcl(&il->ctclloc[CTCL_LPLT], FALSE);
               if (pclh) d3clck(pclh, il, CLCK_LPLT);
               else il->plpclb = NULL;
               }

            /* Set s(i) plot shape used from shape entered.  Change
            *  PhNdl or OrNdl to Square if required variables do not
            *  exist, just inactivate bad bars.  Change Circle, but
            *  not other ops, to Square if requested by global PLSQR
            *  option.  Keeping ksiplu and ksiple separate allows
            *  options to be properly combined no matter what order
            *  they are entered across multiple CYCLE cards (except
            *  bar plots once turned off remain off--would need
            *  separate entered-used copies to fix this).
            *  R74, 08/05/17, GNR - Removed deprecated kctp KRP3B,
            *     KRPSQ.  */
            il->ksipcu = il->Ct.ksipce;
            il->ksiplu = il->Ct.ksiple;
            switch ((enum SiShapes)il->ksiplu) {
            case SIS_None:
               if (RP->kplot & PLSQR) il->ksiplu = (byte)SIS_Square;
               break;
            case SIS_Circle:
               if (RP->kplot & PLSQR) il->ksiplu = (byte)SIS_Square;
               break;
            case SIS_PhNdl:
               if (!il->phshft) il->ksiplu = (byte)SIS_Square;
               break;
            case SIS_OrNdl:
               if (il->nel < 2) il->ksiplu = (byte)SIS_Square;
               else il->ctf |= CTFPG;
               break;
            case SIS_GMxBars:
               il->ksipcu = SIC_GMax;
               il->ctf |= CTFPG;
               /* No break, drop into case SIS_Bars ... */
            case SIS_Bars: {
               /* Revised, 02/13/18, to check individual bars */
               int ibar;
               for (ibar=0; ibar<il->Ct.ksbarn; ++ibar) {
                  int jbar = (int)il->Ct.kksbar[ibar];
                  enum SiBarId kbar = (enum SiBarId)(jbar & 0x0f);
                  char chbar = '1' + (char)ibar;
                  if ((kbar == SIB_Depr && !(il->ctf & CTFDN)) ||
                      (kbar == SIB_Noise &&
                           !(il->No.noikadd & NFL_GENN)) ||
                      (kbar == SIB_Izhu && !ctexists(il, IZU)))
                     goto BarWarning;
                  /* AK and RAW are stored if bar plot requested at
                  *  Group I time, but the test for existence here is
                  *  needed in case plot is requested at Group III
                  *  time but were not saved.  */
                  if (kbar == SIB_Aff) {
                     int jct,aict = jbar >> 4;
                     if (aict > (int)il->nct) goto BarWarning;
                     ix = il->pct1, jct=aict;
                     while (--jct > 0) ix = ix->pct;
                     if (!xnexists(ix,AK) || !xnexists(ix,RAW))
                        goto BarWarning;
                     }
                  continue;
BarWarning:       cryout(RK_P1, " -->WARNING: Bar ", RK_LN1, chbar, 1,
                     " for ", 5, fmtlrlnm(il), RK_CCL, " turned off:",
                     RK_CCL, "       requested conntype or variable "
                     "does not exist or was not saved.", RK_CCL, NULL);
                  il->Ct.kksbar[ibar] = (byte)SIB_None;
                  }}  /* End ibar loop and SIS_Bars local scope */
               goto SkipColorTests;    /* Else ksipcu --> SIC_Si */
               } /* End ksiplu switch */

            /* Preparations for s(i) plot color choice.  For the
            *  default case of using colors proportional to the
            *  quantity plotted, set up the color components here.
            *  We call deccol whether or not actually using scaled
            *  color in order to get the error msg for bad colors.  */
            if (ssmatch(il->Ct.sicolor[EXCIT], "SEQUENCE", 3))
               il->sicop |= SICECSQ;
            else if (ssmatch(il->Ct.sicolor[EXCIT], "OMIT", 3))
               il->sicop |= SICECOM;
            else if (deccol(&il->bexbgr, il->Ct.sicolor[EXCIT]))
               cryout(RK_E1, "0***INVALID EXCITATORY COLOR FOR ",
                  RK_LN2, fmturlnm(il), RK_CCL, NULL);
            if (ssmatch(il->Ct.sicolor[INHIB], "SEQUENCE", 3))
               il->sicop |= SICICSQ;
            else if (ssmatch(il->Ct.sicolor[INHIB], "OMIT", 3))
               il->sicop |= SICICOM;
            else if (deccol(&il->binbgr, il->Ct.sicolor[INHIB]))
               cryout(RK_E1, "0***INVALID INHIBITORY COLOR FOR ",
                  RK_LN2, fmturlnm(il), RK_CCL, NULL);

            /* Set s(i) plot color source default if not entered.
            *  If a color source was requested that does not exist,
            *  issue a warning and switch to Si coloring.  (Older
            *  code in d3chng() generated an error in these cases.
            *  Before first series, d3allo() turns on SBAR, QBAR if
            *  needed for coloring plots, but in later series this
            *  is impossible.)  */
            {  int kbad = FALSE;
               enum SiColSrc ksip = il->ksipcu;
               /* Set flag for drawing one symbol per group */
               if (ksip == SIC_GAvg || ksip == SIC_GMax)
                  il->ctf |= CTFPG;
               else {
                  if (ksip == SIC_None) ksip = il->ksipcu =
                     il->phshft ? SIC_Phase : SIC_Si;
                  switch (ksip) {
                  case SIC_SBar:
                     if (!ctexists(il, SBAR)) kbad = TRUE; break;
                  case SIC_QBar:
                     if (!ctexists(il, QBAR)) kbad = TRUE; break;
                  case SIC_Phase:
                     if (!il->phshft)         kbad = TRUE; break;
                  case SIC_Hist:    /* Temp restriction */
                     if (!(RP->kplot&PLSUP))  kbad = TRUE; break;
                  case SIC_Izhu:
                     if (!ctexists(il, IZU))  kbad = TRUE; break;
                     } /* End ksipcu switch */
                  if (kbad) {
                     cryout(RK_P1, "0-->WARNING:  Cell plot coloring set "
                        "to use s(i) for ", RK_LN2, fmtlrlnm(il), RK_CCL,
                        ": specified variable does not exist.", RK_CCL,
                        NULL);
                     il->ksipcu = SIC_Si;
                     }
                  } /* End else not fixed */
               } /* End kbad local scope */
SkipColorTests: ;
            } /* End if OKPLOT */

/* Connection-type-level initialization */

         il->orkam = 0;
         il->orvdopt = 0;
         il->orphopt = 0;
         for (ix=il->pct1; ix; ix=ix->pct) {

            ui32 lkam;           /* Local copy of ix->Cn.kam */
            char emsg[48+2*LSNAME];    /* Just code it once */
            sconvrt(emsg, "(' FOR CONNTYPE ',J1IH4,3HTO J0A" qLXN ")",
               &ix->ict, fmturlnm(il), NULL);

            /* Volt-dep den != 0 check is done here (rather than, say,
            *  in d3tchk) so tests can change at Group III time.  */
            if (ix->Cn.vdopt & VDANY && ix->Cn.vdha == ix->Cn.vdt)
               cryout(RK_E1, "0***VDHA CANNOT EQUAL VDT", RK_LN2,
                  emsg, RK_CCL, NULL);

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

            /* Setup variables needed to locate real or virtual source
            *  cells for neuroanat plots--all or just one conntype. */
            if (donanat + ((si16)ix->ict == il->jijpl) == 2) {
               d3cpli(&ix->Np, il, ix, 0, 0, 0, 0, KP_NANAT);
               /* Encode line coloring method */
               ix->knapc0 = ix->Cn.napc;
               /* If asks for Mij and Mij not there, color it red */
               if (ix->Cn.napc == NAPC_MIJ && !cnexists(ix,MIJ))
                  ix->knapc0 = NAPC_SOLID;
               }

            /* Set KAMTNU, KAMVNU bits for easier tests in d3go */
            kamchk(ix);

            /* Maintain up-to-date orkam, orphopt, and orvdopt */
            il->orkam |= lkam = ix->Cn.kam;
            il->orvdopt |= ix->Cn.vdopt;
            il->orphopt |= ix->phopt;

            /* Make vdmm >= 0 if VDOPN (d3tchk test repeated here
            *  in case changed at Group III time)  */
            if (ix->Cn.vdopt & VDOPN && ix->Cn.vdmm < 0)
               ix->Cn.vdmm = 0;

            /* Set to do or omit decay and amplification */
            ix->dcybi = NO_DECAY;
            ix->ampbi = NO_AMP;
            ix->finbi = NO_FIN_AMP;

            /* Do no more if KAMMF bit is set */
            if (lkam & KAMMF) continue;

            /* Setup if there is any decaying to be done */
            if (ix->Cn.gamma && ix->Cn.kdecay&(KINDE|KINDB|KINDI)) {
               if      (ix->Cn.kdecay & KINDE) ix->dcybi = DECAYE;
               else if (ix->Cn.kdecay & KINDB) ix->dcybi = DECAYB;
               else if (ix->Cn.kdecay & KINDI) ix->dcybi = DECAYI;
               ix->finbi = NULLAMP;  /* Perform at least null amp */
               }

            /* Setup if there is any amplifying to be done */
            preamp_enable = OFF;
            if (lkam & ANYAMASK) {
               if (ix->Cn.mdelta) {       /* Is delta non-zero? */
                  preamp_enable = ON;
                  if (lkam & KAMBCM)      ix->finbi = BCMAFIN;
                  else if (lkam & KAMUE)  ix->finbi = ELJTFIN;
                  else                    ix->finbi = NORMAMP;
                  }
               if ((lkam & KAMDS) &&
                  !(RP->CP.runflags & (RP_NOSTAT|RP_NOCYST)) &&
                  !(il->Ct.kstat & KSTNS)) preamp_enable = ON;
               }

            if (preamp_enable) {
               if (lkam & KAMUE)
                  ix->ampbi = ELJTAMP;
               else if (lkam & KAMTS)
                  ix->ampbi = TICKAMP;
               else if (lkam & KAMSA)
                  ix->ampbi = SLOWAMP;
               else
                  ix->ampbi = HEBBAMP;

               /* Set scale for each sign combination for requested
               *  subclass of Darwin II-style amplification.  Expand
               *  the static scales to S10 wayset values (incr'd from
               *  S8, 06/23/12, for more precision--larger might re-
               *  quire recodes in d3go to prevent overflow.  Error if
               *  user entered explicit wayset scales in this case.  */
               if (lkam & (KAM2I|KAM2J|KAM3|KAM4|KAMHB)) {
                  int ik,i;
                  if (ix->Cn.nwayrd > 0) {
                     cryout(RK_E1, "0***EXPLICIT WAY SCALES CANNOT BE "
                        "USED WITH AMP RULES H,I,J,3,4", RK_LN2,
                        emsg, RK_CCL, NULL);
                     }
                  for (ik=0; ik<NTAC; ik++) {
                     if (lkam & amp_case_codes[ik]) {
                        for (i=0; i<MAX_WAYSET_CODES; ++i)
                           ix->Cn.wayset[i] =
                              (short)amp_case_scales[ik][i] << FBws;
                        break;
                        }
                     }
                  } /* End if traditional kam case */

               /* Otherwise, warning if any way scales were entered,
               *  but fewer than the number appropriate for the
               *  selected amplification rule.  Then kill the entered
               *  count to avoid an error if the amp rule is later
               *  changed to one of the above.  */

               else {
                  byte nnwayrd = ix->Cn.nwayrd & ~NWSCLE;
                  if (nnwayrd > 0 && nnwayrd < (lkam & KAMVNU ?
                        MAX_WAYSET_CODES : MAX_WAYSET_CODES/2))
                     cryout(RK_P1,"0-->WARNING: Missing amp scale",
                        RK_LN2,ix->Cn.nwayrd & NWSCLE ? "s" : " codes",
                        RK_CCL, " set to 0",RK_CCL, emsg,RK_CCL, NULL);
                  ix->Cn.nwayrd = 0;
                  } /* End else cases that allow wayset entry */

               /* Make table of mdelta*phi for amplifier if mdelta has
               *  changed--not rounding here, will round in d3go */
               if (ix->Cn.mdelta != ix->phitab[0]) {
                  register si32 c,c2;
                  register si32 *pwork = ix->phitab;
                  for (c2=c=0; c<=127; c+=4,c2=c*c) *pwork++ =
                     mrssle(S28+c2*c2-(c2<<15), ix->Cn.mdelta, 28,
                     EAfl(OVF_GEN));
                  } /* End if mdelta changed */

               /* Set ix->sms, which is the scale to convert
               *  (Sj-wkmtj)*(si-mti)*(v-mtv)*way-scale to S16.  */
               ix->sms = (ix->Cn.kam & KAMVNU) ? 30 : 22;

               RP0->n0flags |= N0F_ANYAMP;
               } /* End amplification setup */

            /* Check value index for value-modulated amplification--
            *  seems wise to check whether or not actually
            *  amplifying.  */
            if ((lkam & (KAMVM|KAMUE)) &&
               ((ix->Cn.vi > RP->nvblk) || (ix->Cn.vi < 1)))
                  d3exit(fmturlnm(il), VMAMPL_ERR, (int)ix->ict);
            if (cnexists(ix,CIJ0) &&
               ((ix->Cn.dvi > RP->nvblk) || (ix->Cn.dvi < 0)))
                  d3exit(fmturlnm(il), VMAMPL_ERR, (int)ix->ict);

            /* Determine number of times rseed should be updated
            *  for each synapse and each cell that is amplified
            *  (update may be performed in sequence II or III).
            *  Since nsa < nc and nc < 2^28, upcij cannot overflow.  */
            ix->upcij1s = preamp_enable ?
               ((cnexists(ix,CIJ0) && ix->Cn.rho) ? 2 : 1) : 0;
            {  ui32 nups = ix->Cn.saopt & DOSARB ?
               ix->nc + ix->nsa : ix->nc;
               ix->upcij = nups*(long)ix->upcij1s;
               }

            /* If there is both phase and ADECAY, turn off the
            *  ADECAY, as current peipv allocation and d3go code
            *  cannot handle this case.  */
            if (ix->phopt && qdecay(&ix->Cn.ADCY)) {
               convrt("(P1,'0-->WARNING:  ADECAY turned off "
                  "for ',A" qLXN ",', conntype ',J0IH4/"
                  "'    because incompatible with phase.')",
                  fmtlrlnm(il), &ix->ict, NULL);
               offdecay(&ix->Cn.ADCY);
               }

            /* Check that if a change in ndpsp occurred, it is a
            *  valid change */
            if (qdecay(&ix->Cn.ADCY)) {
               byte npsp = ix->Cn.ADCY.ndal * npsps((int)ix->sssck);
               if (npsp != ix->Cn.ADCY.ndpsp &&
                     !(ix->Cn.ADCY.dcyopt & DCY_CHGOK))
                  cryout(RK_E1, "0***S(j) DECAY CHANGED AND CHNGOK "
                     "WAS NOT SET", RK_LN2, emsg, RK_CCL, NULL);
               ix->Cn.ADCY.ndpsp = npsp;
               }

            /* If both excit and inhib AffD64 exist, allow NOPCX
            *  option, otherwise nullify it -- no warning needed
            *  Rev, 08/03/13, GNR - Bug fix, check npsps == 2  */
            ix->cnflgs &= ~DOCEI;
            if (ix->Cn.cnopt & NOPCX && npsps((int)ix->sssck) == 2)
               ix->cnflgs |= DOCEI;

            /* Compute scaled background noise for TERNARY */
            if (ix->pvpfb) {
               struct TERNARY *pvp = ix->pvpfb;
               e64dec(EAfl(OVF_TSCL));
               /* nmn (S27) + bckscl (FBsc) -> dennoi (FBtm) */
               pvp->dennoi = (pvp->bckgnd == SI32_SGN) ?
                  mrsrsld(il->No.nmn, pvp->bckscl,
                     RP->bsdc + (20 + FBsc - FBtm)) : pvp->bckgnd;
               }

            } /* End ix loop */

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

            /* Check that if a change in ndpsp occurred, it is a
            *  valid change */
            if (qdecay(&ib->GDCY)) {
               byte npsp = ib->GDCY.ndal * npsps((int)ib->gssck);
               if (npsp != ib->GDCY.ndpsp &&
                     !(ib->GDCY.dcyopt & DCY_CHGOK))
               cryout(RK_E1, "0***GCONN DECAY CHANGED AND CHNGOK WAS "
                  "NOT SET", RK_LN2, wmsg, RK_CCL, NULL);
               ib->GDCY.ndpsp = npsp;
               if (!(ib->ibopt & IBOPFS)) {
                  ib->gcdefer = 0;
                  ib->gfscyc = UI16_MAX;
                  }
               else {
                  ib->gcdefer = il->upseq <= ib->pisrc->upseq;
                  ib->gfscyc = ib->gcdefer + 1;
                  if (ib->gdefer < ib->gcdefer)
                     cryout(RK_P1, "0-->WARNING:  gdefer < 1 w/OPT=F",
                        RK_LN2, wmsg, RK_CCL,
                        ", GCONNs = 0 in cycle 1.", RK_CCL, NULL);
                  if (ib->gfscyc > RP->ntc1)
                     cryout(RK_E1, "0***ntc1 TOO FEW FOR OPT=F",
                        RK_LN2, wmsg, RK_CCL, NULL);
                  }
               } /* End if decay */
            else {            /* No decay */
               ib->gcdefer = ib->gdefer;
               ib->gfscyc = UI16_MAX;
               }

            if (jmsle(ib->gbcm, (si32)(RP->ntc1-ib->gdefer-1),
                  EAfl(OVF_GEOM)) > S24)
               cryout(RK_P1, "0-->WARNING:  gbcm goes negative",
                  RK_LN2, wmsg, RK_CCL, NULL);
            if (ib->gdefer >= RP->ntc1)
               cryout(RK_P1, "0-->WARNING:  gdefer >= ntc", RK_LN2,
                  wmsg, RK_CCL, ", GCONNs have no effect.", RK_CCL,
                  NULL);
            } /* End ib loop */

/* MODBY initialization.
*  N.B.  Defined action:  IBOPFS for MODUL, unlike GCONN, always can
*     be applied in cycle 1 because sums are done after Si calc in
*     d3go().  As with GCONN, gdefer refers to the absolute cycle
*     number, regardless of update sequence.
*  Set Mdc.mdefer to minimum(mdefer in attached MODBYs) but 0 if any
*  of these have decay.  Also test for mdefer values too large. */

         for (imb=il->pmby1; imb; imb=imb->pmby) {
            char wmsg[48+2*LSNAME];    /* Just code it once */
            sconvrt(wmsg, "(' for ',J0A" qLXN ",', MODUL type ',J0IH4)",
               fmturlnm(il), &imb->mct, NULL);
            imv = imb->pmvb;
            imb->mfscyc = imb->Mc.mopt & IBOPFS ? 1 : UI16_MAX;
            if (qdecay(&imb->MDCY)) {
               byte npsp = imb->MDCY.ndal * npsps((int)imb->mssck);
               if (npsp != imb->MDCY.ndpsp &&
                     !(imb->MDCY.dcyopt & DCY_CHGOK))
                  cryout(RK_E1, "0***MODUL DECAY CHANGED AND CHNGOK "
                     "WAS NOT SET ", RK_LN2, wmsg, RK_CCL, NULL);
               imb->MDCY.ndpsp = npsp;
               imv->Mdc.mdefer = 0;
               }
            else if (imb->Mc.mdefer < imv->Mdc.mdefer)
               imv->Mdc.mdefer = imb->Mc.mdefer;
            if (imb->Mc.mdefer >= RP->ntc1)
               cryout(RK_P1, "0-->WARNING:  mdefer >= ntc", RK_LN2,
                  wmsg, RK_CCL, ", MODUL has no effect.", RK_CCL, NULL);
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
         if (il->orkam & (KAMSA|KAMZS|KAMTNU) &&
               il->orkam & (KAMBCM|KAMDQ|KAMPA) &&
               il->Dc.sdamp <= il->Dc.qdamp)
            cryout(RK_P1, "0-->WARNING:  SDAMP <= QDAMP for slow "
               "amplif for ", RK_LN2, fmtlrlnm(il), RK_CCL, NULL);

         /* Error if mixed KAMBCM and KAMQNB (=KAMDQ|KAMPA) within a
         *  cell type.  This is because they share the QBAR variable
         *  but it is different.  To allow this mixture, need to add
         *  a new QBAR2 for KAMBCM and dprnt, simdata, decay, etc.  */
         if ((il->orkam & (KAMBCM|KAMQNB)) == (KAMBCM|KAMQNB))
            cryout(RK_E1, "0***CANNOT MIX AMP RULE B WITH P or Q "
               "FOR ", RK_LN2, fmturlnm(il), RK_CCL, NULL);
         } /* End il loop */

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
         if (pdpd->dpclloc.clnum > 0) {
            pclh = findcl(&pdpd->dpclloc, FALSE);
            /* If list is empty, no error--resume using
            *  this list when new cells are supplied.  */
            if (pclh) d3clck(pclh, pdpd, CLCK_DPRINT);
            }
         }
      } /* End pdpd, pclh local scope */

/* Check CLISTs specified in PRBDEF blocks.  If the specified CLIST
*  is not found, omit processing for this series, but keep the PRBDEF,
*  because the CLIST might be added later (PRBDEFs with explicit cell
*  lists are removed in getprobe() if the cell list is removed).
*  R67, 11/11/16, GNR - Duplication of PRBDEFs removed, now have
*                       COPY option in getprobe().
*                       Better error checking.
*/

   {  struct PRBDEF   *pprb;
      struct CLHDR    *pclh;
      struct CLBLK    *qclb;
      for (pprb=RP->pfprb; pprb; pprb=pprb->pnprb) {
         struct CELLTYPE *jl = pprb->psct;
         pprb->pclb = NULL;   /* Avoid passing bad ptr to comp nodes */
         pclh = findcl(&pprb->prbclloc, FALSE);
         if (pclh) {
            char *pmid = ssprintf(NULL, "0***PROBE %d INDEX %d OF "
               "CELLTYPE (%" qLSN "s,%" qLSN "s)", pprb->ictp,
               pprb->prbclloc.clndx, jl->pback->sname, jl->lname);
            int jndx = (int)pprb->prbclloc.clndx;
            qclb = pclh->pfclb;
            if (jndx > 0) {      /* Must use specified match */
               if (jndx > pclh->nclb) {
                  cryout(RK_E1, pmid, RK_LN2, " REFERS TO A "
                     "NONEXISTENT CLIST INDEX.", RK_CCL, NULL);
                  continue;
                  }
               /* Step out to specified list index */
               while (--jndx > 0) qclb = qclb->pnclb;
               /* Got a name there, check it, anon is OK */
               if (qclb->clnm.rnm[0] && (strncmp(qclb->clnm.rnm,
                     jl->pback->sname, LSNAME) ||
                     strncmp(qclb->clnm.lnm, jl->lname, LSNAME))) {
                  cryout(RK_E1, pmid, RK_LN2, " REFERS TO A "
                     "CLIST FOR A DIFFERENT LAYER.", RK_CCL, NULL);
                  continue;
                  }
               }
            /* No index specified, but there is an rlnm--
            *  use first match or give error.  */
            else if (qclb->clnm.rnm[0]) {
               for ( ; qclb; qclb = qclb->pnclb) {
                  if (!strncmp(qclb->clnm.rnm,
                        jl->pback->sname, LSNAME) &&
                        !strncmp(qclb->clnm.lnm, jl->lname, LSNAME))
                     goto GotClblkMatch;
                  }
               cryout(RK_E1, pmid, RK_LN2,
                  " NO MATCHING CLIST FOUND.", RK_CCL, NULL);
               continue;
               }
GotClblkMatch:
            pprb->pclb = qclb;
            pprb->prbdis &= ~PRBDSYS;
            d3clck(pclh, pprb, CLCK_PROBE);
            if (pprb->cpp == 0) pprb->prbdis |= PRBDSYS;
            }
         else {
            /* CLIST does not exist.  Disable probe with a warning--
            *  CLIST may be added in a later trial series.  */
            char *pmid = ssprintf(NULL, "0-->Probe %d of Celltype "
               "(%*s,%*s)", pprb->ictp,
               (int)LSNAME, pprb->psct->pback->sname,
               (int)LSNAME, pprb->psct->lname);
            cryout(RK_P2, pmid, RK_LN3,
               " refers to a nonexistent clist.", RK_CCL,
               " Probe disabled but kept for later use.", RK_LN0, NULL);
            pprb->prbdis |= PRBDSYS; /* Temp disable probe */
            }
         } /* End loop over probes */
      } /* End pprb, pclh local scope */

/* Set parameters for plotting value.  (Moved here from d3tchk()
*  now that PLOC card can be entered at d3grp3 time.)  */

   RP0->yvbxht = 0.7*RP0->vplh;
   RP0->yvstrt[0] = RP0->vply[0];
   RP0->yvstrt[1] = RP0->vply[1];
   for (ivb=RP->pvblk; ivb; ivb=ivb->pnxvb) {
      if (ivb->vbopt & VBVOPNP) continue;
      RP0->yvstrt[(ivb->vbopt & VBVOP2C) != 0] += RP0->vplh;
      }
   RP->rlvlval = RP0->rlvl[lvval];  /* For d3resetn->d3zval */

/* Check that VALUE reset events refer to values that exist */

   {  struct VALEVENT *pvev;
      char vnum[8];
      for (pvev=RP0->pvev1; pvev; pvev=pvev->pnve)
            if (pvev->ivdt > RP->nvblk) {
         wbcdwt(&pvev->ivdt, vnum, RK_IORF|RK_LFTJ|RK_NHALF+7);
         cryout(RK_E1, "0***A RESET EVENT REFERS TO A "
            "NONEXISTENT VALUE SCHEME ", RK_LN2,
            vnum, RK.length + 1, NULL);
         }
      } /* End pvev local scope */


/*---------------------------------------------------------------------*
*           Match EFFARB blocks with associated cell types             *
*             and allocate pjnwd space (first time only)               *
*                                                                      *
*  ljnwd = length of changes array (words).  It includes:              *
*     peff->pbdev->ldat elements for each effector +                   *
*     1 change for each joint (2 for universal joints) +               *
*     wdw_narb(pw) for each window.                                    *
*                                                                      *
*     Method of handling 'N' option:  Since bbdspute writes to client  *
*  only once per trial, we set pjnwd to hold data for all cycles just  *
*  for those effectors that request that option.  Client can unpack.   *
*     This code was moved here from d3schk and d3snsa because it now   *
*  must execute after d3grp3 so allocation with EOPT=N can access      *
*  ntc1.  When this option is active, ntc1 is not allowed to change.   *
*---------------------------------------------------------------------*/

   RP0->ljnwd = 0;
   e64dec(EAfl(OVF_ALLO));
   for (pea=RP0->pearb1; pea; pea=pea->pneb) {
      si64 tlock,tt;          /* Temp for length overflow check */
      long tjhigh;            /* High cell in output (long for ilst */
      long tnelt;             /* Total cells in celltype */
      si32 tljnwd;            /* Entries in output */
      int  nrows;             /* Number of rows of arbw outputs */
      si32 tcols = 0;         /* Total cols, EXCIT + INHIB */
      int  iei;               /* Index over excit,inhib */
      int  nexin = 0    ;     /* Number pjnwd entries this EFFARB */
      static const char *mexin[NEXIN] = { " EXCIT ", " INHIB " };

      /* Even if there is only inhibition, code below needs offset
      *  to first pjnwd to store in device block.  Note that we now
      *  allow an effector to draw excit and inhib cells from same
      *  celltype, but with different cell lists.  */
      pea->jjnwd[EXCIT] = RP0->ljnwd;

      /* JIC error check */
      if (!pea->peff) {
         cryout(RK_E1, "0***EFFECTOR ", RK_LN2,
            fmteffnm(pea), RK_CCL, "NOT SET UP", RK_CCL, NULL);
         continue;
         }

      /* Save checking this several places inside the iei loop */
      if (pea->nchgs == 0) goto EFFARB_NO_CHANGES;

      /* Pick up cell types for EXCIT,INHIB cells.
      *  If no name entered, hbcnm is 0 and this iei is skipped. */
      for (iei=EXCIT; iei<=INHIB; ++iei) if (pea->exinnm[iei].hbcnm) {
         tnelt = (long)pea->pect[iei]->nelt;  /* Total cells */
         if (pea->pefil[iei]) {  /* Cell list found */
            /* Check cell list against cells in celltype.  This
            *  could be two different ilsts operating on the same
            *  celltype, or the same ilst operating on different
            *  celltypes.  So just run ilstchk without the ifs.  */
            if (ilstchk(pea->pefil[iei], tnelt, fmteffnm(pea))) {
               RK.iexit |= 1; continue; }
            pea->arbw[iei] = (ui32)ilstitct(pea->pefil[iei], IL_MASK);
            tjhigh = ilsthigh(pea->pefil[iei]) + 1L;
            }
         else {                  /* No cell list found */
            pea->arbw[iei] = (ui32)tnelt/pea->nchgs;
            tjhigh = (long)pea->arbw[iei];
            } /* End synthetic arbor setting mechanism */
         tljnwd = (pea->keop & KEOP_CELLS) ? pea->arbw[iei] : 1;
         /* Check that d3go will not exceed the top of the layer
         *  in doing nchgs repeats of the cell list.  */
#if LSIZE == 8
         tlock = jaswd(tjhigh, (pea->nchgs-1)*pea->arbsep[iei]);
         if (tlock > tnelt)
#else
         tlock = jmsw(pea->nchgs-1, pea->arbsep[iei]);
         /* tjhigh - tnelt cannot underflow, OK to combine */
         tlock = jaslod(tlock, tjhigh - tnelt);
         if (qsw(tlock) > 0)
#endif
            cryout(RK_E1, "0***", RK_LN2+4, mexin[iei]+1, RK_CCL,
               "EFFECTOR ARBOR FOR ", RK_CCL, fmteffnm(pea), RK_CCL,
               "EXCEEDS SIZE OF CELL LAYER", RK_CCL, NULL);
         pea->jjnwd[iei] = RP0->ljnwd;    /* Redundant for excit */
         /* Compute total space and add to grand total.
         *  Default entc = ntc1 for first trial series, then const.  */
         nrows = pea->nchgs;
         if (pea->keop & KEOP_NITS) {
            if (pea->entc == 0) pea->entc = (ui32)RP->ntc1;
            nrows = jmsld(nrows, pea->entc);
            }
         else pea->entc = 1;
         tcols += tljnwd;
         tlock = jmsw(tljnwd, nrows);
         tt = jaswd(jesl(nexin), tlock);
         swlodm(nexin, tt);
         tt = jaswd(jesl(RP0->ljnwd), tlock);
         swlodm(RP0->ljnwd, tt);
         /* Compute working scale factor */
         pea->esclu[iei] = pea->escli[iei] /
            (float)(1 << (RP->bsdc + FBsi));
         } /* End iei for-if loop */
      if (nexin == 0) pea->nchgs = 0;
EFFARB_NO_CHANGES: ;

/* Store updated information back in effector block */

      switch (pea->keff) {
      case KEF_ARM:
      case KEF_UJ:
         ((JOINT *)pea->peff)->ojnwd = pea->nchgs ?
            pea->jjnwd[EXCIT] : INT_MAX;
         break;
      case KEF_WDW: {
         WINDOW *pw = (WINDOW *)pea->peff;
         if (pea->nchgs == 0) {
            pw->kchng = WDWZIP;
            pw->ojnwd = LONG_MAX; }
         else
            pw->ojnwd = pea->jjnwd[EXCIT];
         break;
         }
#ifdef BBDS
      case KEF_BBD: {
         BBDDev *pbbd = (BBDDev *)pea->peff;
         pbbd->bdflgs |= BBDUsed;
         /* We allow nexin == 0 here--if this is not allowed
         *  by the device, bbdschk will complain.  */
         pbbd->ldat = (si32)nexin;
         pbbd->UD.Eff.effx = tcols, pbbd->UD.Eff.effy = nrows;
         break;
         }
#endif
         } /* End keff switch */

      } /* End loop over EFFARB blocks */

/* Allocate changes array (RP0->pjnwd)--first trial, host only  */

   if (RP0->ljnwd > 0 && !RP->CP.effcyc)
      RP0->pjnwd = (float *)allocpcv(Host,
         RP0->ljnwd, sizeof(float), "Changes array");

#ifdef BBDS
/* Sadly, we had to loop over all effectors to get size of pjnwd,
*  now have to repeat that loop to store pjnwd into BBDDev blocks */

   for (pea=RP0->pearb1; pea; pea=pea->pneb) {
      if (pea->keff == KEF_BBD) ((BBDDev *)pea->peff)->UD.Eff.pmno =
         RP0->pjnwd + pea->jjnwd[EXCIT];
      }
#endif

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
