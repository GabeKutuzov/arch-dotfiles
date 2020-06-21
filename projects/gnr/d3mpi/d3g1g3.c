/* (c) Copyright 1994-2017, The Rockefeller University *11115* */
/* $Id: d3g1g3.c 77 2018-03-15 21:08:14Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                              d3g1g3.c                                *
*                                                                      *
*  This file contains various functions used to parse information      *
*  from control cards which may be entered either in Group I or III.   *
*  Common parsing functions assure consistency and save a little       *
*  memory.                                                             *
************************************************************************
*  V7A, 05/16/94, GNR - Newly written                                  *
*  V7B, 07/04/94, GNR - Add getway, gtoutctl, gtkregen                 *
*  V7C, 08/27/94, GNR - Add getdecay (float hesam 2/2/95)              *
*  V8A, 01/31/97, GNR - Add getcolor, getpcf, alphabetize routines     *
*  V8B, 12/09/00, GNR - New memory management routines, trial timers,  *
*                       remove gtoutctl routine, add SIMDATA stuff     *
*  Rev, 08/27/01, GNR - Add getijpl, getplab, getpline, getploc        *
*  Rev, 05/04/02, GNR - Add cell list indexing to CIJ plots            *
*  Rev, 02/15/05, GNR - Force all color names to upper case,           *
*                       add PLINE thickness parameter                  *
*  V8D, 03/23/07, GNR - Add getitp                                     *
*  Rev, 11/25/07, GNR - Add SAVE NEWSERIES, CREATE|ROTATE_FILENAMES    *
*  Rev, 01/31/08, GNR - Add SAVE SIMDATA SENSE                         *
*  ==>, 02/05/08, GNR - Last mod before committing to svn repository   *
*  Rev, 03/15/08, GNR - Move too-few-scales warning to d3tchk          *
*  Rev, 03/22/08, GNR - Add gtpscale, 06/16/08, add CSELPLOT           *
*  Rev, 01/17/09, GNR - Reorganize colors into a single array          *
*  V8E, 07/30/09, GNR - Remove SAVE REPLAY, IA, SVITMS=G interference  *
*  V8F, 06/01/10, GNR - Add UTVINFO to SAVE card                       *
*  V8H, 11/08/10, GNR - Add SNSID to SAVE card                         *
*  Rev, 04/24/11, GNR - Allow SIMDATA to start in Group III            *
*  V8I, 09/23/12, GNR - Add ALPHA and DOUBLE EXPONENTIAL decay types   *
*  R66, 03/11/16, GNR - Rename nvx->nsxl, nvy->nsya so != ix->nv[xy]   *
*  R67, 04/27/16, GNR - Remove Darwin 1 support                        *
*  R74, 08/06/17, GNR - New color options for IJPLOT varieties         *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "sysdef.h"
#include "rkarith.h"
#include "d3global.h"
#include "wdwdef.h"
#include "armdef.h"
#include "d3opkeys.h"
#include "d3fdef.h"
#include "clblk.h"
#include "ijpldef.h"
#include "plbdef.h"
#include "savblk.h"
#include "simdata.h"
#include "tvdef.h"
#include "rocks.h"
#include "rkxtra.h"
#include "bapkg.h"
#include "swap.h"

/***********************************************************************
*                              getcolor                                *
*                                                                      *
*  This routine processes the COLORS card.                             *
***********************************************************************/

void getcolor(void) {
   ui32 ic = 0;            /* kwscan field recognition codes */
   int  kwret;             /* kwscan return code */
   int  ncol = SEQNUM;     /* Counter for inform */
   char lea[SCANLEN1];     /* Space for scanning */

   cdscan(RK.last, 1, SCANLEN, RK_WDSKIP);
   while (kwret = kwscan(&ic, /* Assignment intended */
         "CIRCLES%X",
         "PHASES%X",
         "WINDOWS%X",
         "ARMS%X",
         "HISTORY%X",
         "EXCITATORY%VA"  qCMC, RP->colors[CI_EXC],
         "INHIBITORY%VA"  qCMC, RP->colors[CI_INH],
         "BOXES%VA"       qCMC, RP->colors[CI_BOX],
         "EXTERNAL%VA"    qCMC, RP->colors[CI_EXT],
         "REGIONS%VA"     qCMC, RP->colors[CI_REP],
         "REPERTOIRES%VA" qCMC, RP->colors[CI_REP],
         "LAYERS%VA"      qCMC, RP->colors[CI_LAY],
         "GRID%VA"       qECMC, RP0->ecolgrd,
         "STIMULI%VA"    qECMC, RP0->ecolstm,
         NULL )) {

      if (RK.scancode != RK_EQUALS) {
         ermark(RK_EQRQERR); continue; }
      scan(lea, 0);
      if (RK.scancode & ~(RK_COMMA|RK_BLANK|RK_INPARENS)) {
         ermark(RK_PUNCERR); continue; }
      switch (kwret) {
      case 1:                    /* Handle CIRCLES */
         if (!(RK.scancode & RK_INPARENS))
            /* Color is a single name */
            sinform(lea, "(VA" qCMC ")", RP->colors[CI_BUB], NULL);
         else {
            /* Got a color sequence */
            strcpy(RP->colors[CI_BUB], "SEQUENCE");
            scanagn();
            inform("(SV(RVA" qCMC "V))",
               &ncol, RP->colors[CI_SEQ], NULL);
            } /* End COLOR SEQ else */
         /* Make it the default for celltypes as well */
         memcpy(RP0->GCtD.sicolor[EXCIT], RP->colors[CI_BUB], COLMAXCH);
         memcpy(RP0->GCtD.sicolor[INHIB], RP->colors[CI_BUB], COLMAXCH);
         break;
      case 2:                    /* Handle PHASE sequence */
         /* Rev, 02/07/09, GNR - No more single color for phases */
         scanagn();
         inform("(SV(RVA" qCMC "V))",
            &ncol, RP->colors[CI_PHS], NULL);
         break;
      case 3: {                  /* Handle WINDOWS */
         WINDOW *pw;
         /* Modify default in case called at Group I time */
         sinform(lea, "(VA" qECMC ")", RP0->ecolwin, NULL);
         /* Modify existing windows in case called at Group III time */
         for (pw=RP0->pwdw1; pw; pw=pw->pnxwdw)
            sinform(lea, "(VA" qCMC ")", pw->wcolr, NULL);
         } /* End local scope */
         break;
      case 4: {                  /* Handle ARMS */
         ARM *pa;
         /* Modify default in case called at Group I time */
         sinform(lea, "(VA" qECMC ")", RP0->ecolarm, NULL);
         /* Modify existing arms in case called at Group III time */
         for (pa=RP0->parm1; pa; pa=pa->pnxarm)
            sinform(lea, "(VA" qCMC ")", pa->acolr, NULL);
         } /* End local scope */
         break;
      case 5:                    /* Handle HISTORY */
         scanagn();
         inform("(SV(4VA" qCMC "V))", RP->colors[CI_HPP], NULL);
         break;
         } /* End kwret switch */
      } /* End kwret while */
   } /* End getcolor() */


/***********************************************************************
*                              getdecay                                *
*                                                                      *
*  This routine processes decay parameters for all connection types,   *
*  input by the user in one of the forms:                              *
*     DECAY=omega       (omega = persistence = exp(-decay*timestep))   *
*     DECAY=(omega)                                                    *
*     DECAY=(omega, {EXPONENTIAL | LIMITING | ALPHA | SATURATING})     *
*     DECAY=(omega, halfeff, SATURATING)                               *
*     DECAY=(omega1, omega2, {DOUBLE-EXP | DBLE-EXP})                  *
*     DECAY=NODECAY  (or set omega == 0) to turn off previous decay    *
*  (Any of the forms in parens can also have the keyword CHNGOK at     *
*  Group II time, which indicates that changes that modify the amount  *
*  of stored persistence memory (e.g. decay type, thresholds that      *
*  modify whether excitation and/or persistence are possible) will be  *
*  allowed at Group III time.  In this case, when a new CYCLE card is  *
*  entered, stored persistence for this DECAY option is destroyed and  *
*  reset is effectively to infinite time.)                             *
*  (Any of the method keywords can as usual be abbreviated by any      *
*  unique leading substring and may appear in any position within the  *
*  parentheses.  If no method keyword is entered at the first setting  *
*  of this decay definition, it defaults to EXPONENTIAL, otherwise,    *
*  any previous method is retained.  With SATURATING, the halfeff pa-  *
*  rameter defaults to 128mV (1.0) if not entered.  With DOUBLE-EXP,   *
*  two persistence parameters must always be entered.  The argument    *
*  must point to a DECAYDEF structure, where the results are stored.   *
*  On entry, the control card has been read and is in RK.last and      *
*  DECAY is the next positional parameter or the DECAY keyword has     *
*  been identified.)                                                   *
*                                                                      *
*  Rev, 09/18/13, GNR - Move checking of kdcy changes to d3chng(), all *
*     changes are OK at Group I/II time (e.g. from MASTER to override) *
*  R63, 10/28/15, GNR - Make hlfeff default 128 mV for mV scale        *
***********************************************************************/

void getdecay(struct DECAYDEF *pdcd) {

#define MXLDCKEY 18                 /* Max length of a mode key */
#define qMXLDCKEY "18"              /* Quoted MXLDCKEY */
#define MXLDCKEY2 20                /* Key length to allocate in lea */
#define qMXLDCKEY2 "20"             /* Quoted MXLDCKEY2 */
#define NDCYVARS 4                  /* Max items in the parens */
#define qNDCYVARS "4"               /* Quoted NDCYVARS */
#define NDCYKEYS 7                  /* Number of possible keys */

   char *pmky;                      /* Ptr to mode key */
   char *pnp1,*pnp2;                /* Ptrs to numeric parms */
   int  i,kdecay,oldkdcy;           /* Index, method keyword index */
   char lea[NDCYVARS][MXLDCKEY2];   /* Space for scanning */

   /* Options for DECAY method keyword and values stored for each */
   static char *gdcykeys[NDCYKEYS] = {"NODECAY", "EXPONENTIAL", "LIMITING",
      "SATURATING", "ALPHA", "DOUBLE-EXPONENTIAL", "DBLE-EXPONENTIAL" };
   static int gdcyvals[NDCYKEYS] = {   NODECAY,   DCYEXP,        DCYLIM,
       DCYSAT,       DCYALPH, DCYDBLE,              DCYDBLE   };

   oldkdcy = (int)pdcd->kdcy;
   /* Scan up to NDCYVARS fields */
   inform("(SNV(" qNDCYVARS "A" qMXLDCKEY2 "V))", lea, NULL);
   /* Locate mode keyword, if any, CHNGOK, and numeric parms,
   *  to simplify later logic */
   pmky = pnp1 = pnp2 = NULL;
   for (i=0; i<RK.numcnv; ++i) {
      char *pfld = lea[i];
      if (cntrln(pfld, MXLDCKEY)) {
         if (ssmatch(pfld, "CHNGOK", 1)) {
            if (RP0->ccgrp & (CCGRP1|CCGRP2)) pdcd->dcyopt |= DCY_CHGOK;
            else { ermark(RK_IDERR); goto TURN_DECAY_OFF; }
            }
         else if (pmky) goto ERROR_TOO_MANY;
         else pmky = pfld;
         }
      else if (!pnp1) pnp1 = pfld;
      else if (!pnp2) pnp2 = pfld;
      else goto ERROR_TOO_MANY;
      }
   /* Identify mode if specified, otherwise EXPONENTIAL is default */
   if (pmky) {
      kdecay = smatch(0, pmky, gdcykeys, NDCYKEYS);
      if (kdecay <= 0) goto TURN_DECAY_OFF;
      kdecay = gdcyvals[kdecay-1];
      if (kdecay == NODECAY) {
         if (pnp1) goto ERROR_TOO_MANY;
         goto TURN_DECAY_OFF; }
      }
   else if (RP0->ccgrp & CCGRP3) {
      kdecay = NODECAY; pdcd->dcyopt |= DCY_NOKDE; }
   else if (oldkdcy != NODECAY) kdecay = oldkdcy;
   else                         kdecay = DCYEXP;
   /* The first numeric field is always omega and is always
   *  required once the NODECAY case has been handled above.  */
   if (pnp1) wbcdin(pnp1, &pdcd->omega,
      FBod*RK_BSCL+RK_QPOS+RK_IORF+RK_NI32+SCANLEN-1);
   else { ermark(RK_REQFERR); goto TURN_DECAY_OFF; }
   /* If omega is 0, turn off decay, ignore everything else */
   if (pdcd->omega == 0) goto TURN_DECAY_OFF;
   /* Error if omega >= 1.0 */
   if (pdcd->omega >= Sod) {
      cryout(RK_E1, "0***omega MUST BE < 1.", RK_LN2, NULL);
      goto TURN_DECAY_OFF;
      }
   /* Interpretation of pnp2 depends on mode */
   switch (kdecay) {
   case NODECAY:
      /* This case can only arise in Group III and is an error if
      *  a number is entered, because the true kdcy is unknown.  */
      if (pnp2) {
         cryout(RK_E1, "0***DECAY METHOD NEEDED WITH 2ND NUMERIC ARG.",
            RK_LN2, NULL);
         goto TURN_DECAY_OFF; }
      break;
   case DCYEXP:
   case DCYLIM:
      if (pnp2) goto ERROR_TOO_MANY;
      break;
   case DCYSAT:
      if (pnp2) { pdcd->du1.dsa.hlfeff =
            bcdin(RK_IORF+RK_SNGL+RK_ZTST+RK_QPOS+SCANLEN-1, pnp2);
         if (pdcd->du1.dsa.hlfeff <= 0.0) goto TURN_DECAY_OFF;
         }
      /* Set default hlfeff here.  Note:  hesam will be the same on
      *  both scales because satdcynum is 2**7 larger on OLDRANGE.  */
      else pdcd->du1.dsa.hlfeff = (RP->compat & OLDRANGE) ? 1.0 : 128.0;
      pdcd->du1.dsa.hesam = RP0->satdcynum/pdcd->du1.dsa.hlfeff;
      break;
   case DCYALPH:
      if (pnp2) goto ERROR_TOO_MANY;
      pdcd->du1.dal.mlnw =
         (si32)(-dSFBlw*log((double)pdcd->omega/dSFBod));
      break;
   case DCYDBLE:
      if (pnp2) {
         si32 dw;
         wbcdin(pnp2, &pdcd->du1.ddb.omeg2,
            FBod*RK_BSCL+RK_QPOS+RK_ZTST+RK_IORF+RK_NI32+SCANLEN-1);
         if ((dw = (pdcd->omega - pdcd->du1.ddb.omeg2)) <= 0) {
            cryout(RK_E1, "0***omeg1 > omega2 REQUIRED.", RK_LN2, NULL);
            goto TURN_DECAY_OFF; }
         pdcd->du1.ddb.w1ovdw = dsrsjqe(pdcd->omega, FBsc, dw,
            EAfl(OVF_GEN));
         }
      else { ermark(RK_REQFERR); goto TURN_DECAY_OFF; }
      break;
      } /* End kdecay switch */
   /* Input complete, store iovec, kdcy, and ndal */
   if (!(pdcd->dcyopt & DCY_IOVAS))
      pdcd->iovec = RP->novec++, pdcd->dcyopt |= DCY_IOVAS;
   pdcd->kdcy = (byte)kdecay;
   pdcd->ndal = kdecay >= DCYALPH ? 2 : 1;
   return;
ERROR_TOO_MANY:
   ermark(RK_TOOMANY);
TURN_DECAY_OFF:
   /* N.B.  If this is a call from d3chng(), we are not turning
   *  decay off, just the indicators in the temp holding area.  */
   offdecay(pdcd);
   return;
   } /* End getdecay() */


/***********************************************************************
*                               getijpl                                *
*                                                                      *
*  Process the CIJPLOT, CIJ0PLOT, CSELPLOT, ESJPLOT, MIJPLOT, PPFPLOT, *
*  RJPLOT, and SJPLOT cards.                                           *
*  Note:  Absence of a cell list is handled by code in the plotting    *
*  routines that plots data for all cells.  No error or warning here.  *
*                                                                      *
*  Rev, 09/24/07, GNR - Add MABCIJ, etc. options                       *
*  Rev, 10/26/07, GNR - Add RJPLOT, SJPLOT, and ESJPLOT                *
*  Rev, 06/16/08, GNR - Add CSELPLOT, smshrink  (12/08/16, -> smscale) *
*  Rev, 08/06/17, GNR - Add OPT=F, EXCCOLOR, INHCOLOR                  *
***********************************************************************/

void getijpl(enum KIJPlot kijpl) {

   struct IJPLDEF *pc,**ppc;
   struct CLHDR   *pclh;      /* Ptr to associated CLHDR  */
   char           *pcmtype;   /* Ptr to type for err msgs */
   ui32   ic = 0;             /* kwscan() field rec codes */
   int    kwret;              /* kwscan() return code     */
   int    nclopt = 0;         /* Number of cell list opts */
   int    tplsq = 0;          /* Temp for plot sequence # */
   short  oldhighrc;          /* Saved value of RK.highrc */
   short  tjijpl = 1;         /* Temp for conntype number */
   rlnm   trlnm;              /* Temp region-layer name   */

   /* Order of following strings must match KIJPlot enum:
   *  Note:  Min,Max,Mnab values are stored on same scale as plot
   *  variables have in memory and are rescaled in plot routines
   *  to test against high-order byte only.  This is to avoid my
   *  confusion and need to change this code if plot routines are
   *  changed, and because there is no simple way to enter these
   *  on scale (S-1) as would be needed for L2S7 variables.  */
   static char *cmmnopts[] = { "MNCIJ%B14IH", "MNCIJ0%B14IH",
      "MNMIJ%B7/14IH$?mV", "MNPPF%B12IH", "MNRJ%B7/14IH$?mV",
      "MNSJ%B7/14IH$?mV", "MNESJ%B7/14IH$?mV", NULL };
   static char *cmmxopts[] = { "MXCIJ%B14IH", "MXCIJ0%B14IH",
      "MXMIJ%B7/14IH$?mV", "MXPPF%B12IH", "MXRJ%B7/14IH$?mV",
      "MXSJ%B7/14IH$?mV", "EMXSJ%B7/14IH$?mV", NULL };
   static char *cmabopts[] = { "MABCIJ%B14IH", "MABCIJ0%B14IH",
      "MABMIJ%B7/14IH$?mV", "MABPPF%B12IH", "MABRJ%B7/14IH$?mV",
      "MABSJ%B7/14IH$?mV", "MABESJ%B7/14IH$?mV", NULL };
   static char *cmtypes[] =
      { "CIJ", "CIJ0", "MIJ", "PPF", "RJ", "SJ", "ESJ", "CSEL" };
   static char kcmdisabl = KCM_DISABL;

/* Save RK.highrc and set it to zero so we can detect syntax
*  errors found by calls to ilstread() from d3clst().  */

   oldhighrc = RK.highrc;
   RK.highrc = 0;

/* Read the required region/layer name and optional conntype
*  and plot sequence numbers.  If these and the kijpl argument
*  match an existing IJPLDEF, use it.  Otherwise, create a new
*  one and initialize it with default values.  */

   pcmtype = cmtypes[kijpl];
   /* inform() checks for missing fields, bad lengths, and
   *  bad punctuation.  */
   inform("(SW1,2A" qLSN "=V>IH)", trlnm.rnm, &tjijpl, NULL);
   /* Check for a plot sequence number in parens */
   if (RK.scancode & RK_LFTPAREN)
      inform("(SV(V>IV))", &tplsq, NULL);
   /* Find any existing IJPLDEF block for this cell type.
   *  Note:  This is OK even in Group I--the CELLTYPE block
   *  doesn't need to be there yet, just the name is checked.  */
   for (ppc=&RP0->pfijpl; *ppc; ppc=&(*ppc)->pnijpl) {
      pc = *ppc;
      if (!strncmp(pc->cmtnm.rnm, trlnm.rnm, LSNAME) &&
          !strncmp(pc->cmtnm.lnm, trlnm.lnm, LSNAME) &&
          pc->cmtarg == tjijpl && pc->kcmplt == (byte)kijpl &&
          --tplsq == 0) goto GotIJPLDEF;
      } /* End match loop */
   /* Error if tplsq entered but not matched */
   if (tplsq > 0) {
      cryout(RK_E1, "0***", RK_LN2, pcmtype, RK_CCL,
         "PLOT WITH SPECIFIED SEQUENCE NUMBER NOT FOUND.",
         RK_CCL, NULL);
      skip2end();
      goto GetCijNextCard; }
   /* Make a new IJPLDEF block and initialize it.  If no
   *  cell list is entered, pc->cmclloc will remain zero.  */
   pc = *ppc = allocpcv(Host, 1, sizeof(struct IJPLDEF),
      "Connection plot data");
   ++RP->nijpldefs;
   cmdflt(pc);
   pc->cmtnm  = trlnm;
   pc->cmtarg = tjijpl;
   pc->kcmplt = (byte)kijpl;

/* Now we have a IJPLDEF.  Read positional parameters until
*  a nonnumeric value or equals sign is encountered, then
*  read keyword parameters.  */

GotIJPLDEF:
   if (!(RP->compat & OLDRANGE)) bscomset(RK_BSUNITS);
   inform("(S=+4*F)", &pc->cmpltx, &pc->cmplty,
      &pc->cmpltw, &pc->cmplth, NULL);
   while (kwret = kwscan(&ic, /* Assignment intended */
         "CELLS%X",              /* 'X' exit 1 */
         "CLIST%X",              /* 'X' exit 2 */
         "OFF%X",                /* 'X' exit 3 */
         "OPT%X",                /* 'X' exit 4 */
         "COLOR%X",              /* 'X' exit 5 */
         "X%VF",                 &pc->cmpltx,
         "Y%VF",                 &pc->cmplty,
         "W%V>F",                &pc->cmpltw,
         "H%V>F",                &pc->cmplth,
         /* Limits V>=0 removed, 06/18/10, GNR for special effects */
         "SMSCALE%V>F",          &pc->smscale,
         "SRCNGX%UJ",            &pc->srcncx,   /* For compatibility */
         "SRCNCX%UJ",            &pc->srcncx,
         "TGTNCX%UJ"  ,          &pc->tgtncx,
         "LABELTEXT%TH" qLTF,    &pc->cmltxt,
         "LTEXT%TH" qLTF,        &pc->cmltxt,
         "LABELCOLOR%VA" qCMC,   pc->cmltc,
         "LCOLOR%VA" qCMC,       pc->cmltc,
         "DISABLE%OC",           &pc->kcmsmap, &kcmdisabl,
         "ENABLE%OC~",           &pc->kcmsmap, &kcmdisabl,
         /* Next arg is NULL if CSELPLOT, stops kwscan() scan */
         cmmnopts[kijpl],        &pc->cmmin,
         cmmxopts[kijpl],        &pc->cmmax,
         cmabopts[kijpl],        &pc->cmamn,
         NULL)) {

      if (kwret <= 3 && nclopt++ > 0) cryout(RK_P1, "0-->WARNING:  "
         "Only last cell list option will be used.", RK_LN2, NULL);
      switch (kwret) {

      case 1:              /* CELLS option */
         if (RK.scancode != RK_EQUALS) {
            ermark(RK_PUNCERR);
            skip2end();
            goto GetCijNextCard; }
         /* Create a unique list id unless there is already one associ-
         *  ated with this block (possibly switching from CLIST=).  */
         if (pc->cmclloc.clnum >= 0) {
            pc->cmclloc.clnum = --RP0->nextclid;
            pc->cmclloc.clndx = 0; }
         /* Read new cell list and save rlnm in the new CLBLK.  NULL
         *  return indicates list is switched off, which here means
         *  leave the IJPLDEF and by default plot all cells.  */
         if ((pclh = d3clst(pc->cmclloc.clnum, CLKM_SINGLE)) != NULL)
            pclh->pfclb->clnm = trlnm;
         else {
            /* If there was no explicit OFF option, and an errmsg
            *  was not yet generated, then RK.highrc is 0 here. This
            *  means an unidentified field was passed into d3clst()
            *  but not processed, so now an errmsg is needed.  */
            if (RK.highrc == 0) ilstreq();
            pc->cmclloc.clnum = 0;
            }
         break;
      case 2:              /* CLIST option */
         /* Delete any existing unique cell list */
         if (pc->cmclloc.clnum < 0) d3clrm(pc->cmclloc.clnum);
         /* Read new cell list num.  If OFF key was entered, clear
         *  out the cmclloc and all cells will be plotted.  */
         if (getclid(&pc->cmclloc))
            memset(&pc->cmclloc, 0, sizeof(clloc));
         break;
      case 3:              /* OFF option */
         /* This is used to kill the entire IJPLDEF--one can use OFF
         *  with CELLS or CLIST to leave the plot but remove the list
         *  and use all cells.  */
         *ppc = pc->pnijpl;
         if (pc->cmclloc.clnum < 0) d3clrm(pc->cmclloc.clnum);
         freev(pc, "Connection plot data");
         --RP->nijpldefs;
         thatsall();
         goto GetCijNextCard;
      case 4:              /* OPT option */
         /* I am currently splitting the 17 option codes into 16 in
         *  pc->cmopt and the 'F' code in pc->doseq.  Old codes
         *  DCS_EFIX and DCS_IFIX are both set, but now redundant--
         *  kept here to make easier to separate later if needed.
         *  (If more options get added, plan is to have separate
         *  plotting shape key for V,P,K,2,G,N.  Method used here
         *  will handle +/- codes in Group III.)  */
         if (RK.scancode == RK_EQUALS) {
            char lea[SCANLEN1];
            ui32 topt = (ui32)pc->cmopt;
            if (pc->doseq & (DCS_EFIX|DCS_IFIX)) topt |= CMFOPT;
            scanck(lea, RK_REQFLD, ~RK_COMMA);
            mcodes(lea, okcmopt, &topt);
            pc->cmopt = (ui16)topt;
            pc->doseq &= ~(DCS_EFIX|DCS_IFIX);
            if (topt & CMFOPT) pc->doseq |= DCS_EFIX|DCS_IFIX;
            }
         else {            /* Punctuation error, no '=' */
            ermark(RK_EQRQERR);
            }
         break;
      case 5:              /* COLOR option */
         if (RK.scancode == RK_EQUALS) {
            char lea[COLMAXCH+1];
            int oldlen = scanlen(COLMAXCH);
            scanck(lea, RK_REQFLD,
               ~(RK_COMMA|RK_INPARENS|RK_LFTPAREN|RK_RTPAREN));
            if (RK.scancode & RK_INPARENS) {
               scanagn();
               inform("(SV(2VA" qCMC "V))", pc->cmeico, NULL);
               }
            else {
               memcpy(pc->cmeico[EXCIT], lea, COLMAXCH);
               memcpy(pc->cmeico[INHIB], lea, COLMAXCH);
               }
            scanlen(oldlen);
            }
         else {                     /* Punctuation error, no '=' */
            ermark(RK_EQRQERR);
            }
         break;
         } /* End kwret switch */

      } /* End kwret while */
   bscomclr(RK_BSUNITS);

   /* Check for valid Cij range */
   if (pc->cmmin > pc->cmmax)
      cryout(RK_E1, "0***INVALID ", RK_LN2, pcmtype, RK_CCL,
         "PLOT VARIABLE RANGE.", RK_CCL, NULL);

   /* KCIJSEL plots with averaging require CMSMAP */
   if (pc->kcmplt == KCIJSEL &&
         (pc->cmopt & (CMTAVG|CMSMAP)) == CMTAVG) {
      cryout(RK_P1, "0-->WARNING: Required option 'S' was turned on "
         "for CSELPLOT.", RK_LN2, NULL);
      pc->cmopt = pc->cmopt & ~(CMISIL|CMBOX) | CMSNOM;
      }

   /* Check for incompatible cell mapping options */
   tjijpl = pc->cmopt & (CMTAVG|CMTMAP|CMITIL);
   if (bitcnt((byte *)&tjijpl, sizeof(tjijpl)) > 1)
      cryout(RK_E1, "0***USE ONLY ONE OF A,J,T OPTIONS.",
         RK_LN2, NULL);
   tjijpl = pc->cmopt & (CMSMAP|CMISIL|CMBOX);
   if (bitcnt((byte *)&tjijpl, sizeof(tjijpl)) > 1)
      cryout(RK_E1, "0***USE ONLY ONE OF B,I,S OPTIONS.",
         RK_LN2, NULL);
   if (pc->cmopt & CMTAVG && pc->tgtncx > 1)
      cryout(RK_E1, "0***INVALID TGTNCX > 1 WITH TARGET AVERAGING.",
         RK_LN2, NULL);

GetCijNextCard:
   if (oldhighrc > RK.highrc) RK.highrc = oldhighrc;
   return;

   } /* End getijpl() */


/***********************************************************************
*                               getitp                                 *
*                                                                      *
*  Processes the IMGTOUCH card, which creates a touch pad that         *
*  accesses pixels in an image at a position controlled by a BBD.      *
*  (An IMGTPAD block has a VCELL block at the top so it can be linked  *
*  transparently into the same list with the VCELLs.)                  *
***********************************************************************/

void getitp(void) {

   struct IMGTPAD *pitp;
   ui32 ic;                   /* kwscan field recognition codes */
   int  iitp;                 /* Index of specified IMGTPAD */
   int  isgrp1;               /* TRUE if in Group I */
   byte tops;                 /* Temp for ops */
   char lea[SCANLEN1];        /* Scan return field */

/* Check for label index and search for specified IMGTPAD.  In
*  this case, positional parameters are not allowed but keywords
*  are allowed for changing scale and plotting parameters.
*  This is allowed even in Group I--can't hurt anything.  */

   isgrp1 = (RP0->ccgrp == CCGRP1);
   cdscan(RK.last, 1, SCANLEN, RK_WDSKIP);
   if (scan(lea, RK_REQFLD) & RK_ENDCARD) return;
   if (RK.scancode == (RK_INPARENS|RK_RTPAREN)) {
      wbcdin(lea, &iitp, LEA2INT|RK_ZTST);
      for (pitp=(struct IMGTPAD *)RP0->pvc1; pitp && iitp;
            pitp=(struct IMGTPAD *)pitp->vc.pnvc) {
         if (pitp->vc.vtype == ITPSRC && --iitp == 0) break; }
      if (!pitp) {
         ermark(RK_MARKFLD);
         cryout(RK_E1, "0***INDEX > NUM IMAGE TOUCHPADS",
            RK_LN2, NULL);
         skip2end();
         return;
         }
      }

/* No index.  This is a new one.  Error if in Group III.  */

   else {
      if (!isgrp1) {
         cryout(RK_E1, "0***THIS CARD MUST BE ENTERED IN GROUP I.",
            RK_LN2, NULL);
         skip2end();
         return;
         }
      pitp = (struct IMGTPAD *)allocpcv(
         Host, 1, sizeof(struct IMGTPAD), "image touch info");
      *RP0->plvc = (struct VCELL *)pitp;
      RP0->plvc = &pitp->vc.pnvc;
      pitp->vc.vtype = ITPSRC;
      RP->nitps += 1;            /* Count for gettpnum() */
      itpdflt(pitp);             /* Set defaults */

      /* Read required positional parameters */
      wbcdin(lea, &pitp->vc.nsya, RK_ZTST|LEA2INT);
      inform("(S,V>IH,TH" qLTF ",A" qLSN ")",
         &pitp->vc.nsxl, &pitp->hcamnm, pitp->vc.vcname, NULL);
      if (getsnsid(pitp->vc.vcname) == USRSRC) {
         pitp->itop |= ITOP_USR;
         inform("(S,TH" qLTF "=3IH)", &pitp->vc.hvname,
            &pitp->ovsx, NULL);
         pitp->vc.lvrn = strnlen(getrktxt(pitp->vc.hvname), LTEXTF);
         }
      else
         inform("(S,IH=3IH)", &pitp->vc.hvname, &pitp->ovsx, NULL);
      }

/* Read keyword parameters common to Group I and Group III */

   tops = pitp->itop & (ITOP_INT|ITOP_PRES);
   kwscan(&ic,
         "CHANGE%V>=0<=3IC",  &pitp->itchng,
         "ITOP%KCPI",         &tops,
         "OFFX%F",            &pitp->offx,
         "OFFY%F",            &pitp->offy,
         "OFFA%F",            &pitp->offa,
         "SCLX%F",            &pitp->sclx,
         "SCLY%F",            &pitp->scly,
         "SCLA%F",            &pitp->scla,
         "TLA%RV>F", isgrp1,  &pitp->tla,    /* See CAUTION */
         "TLL%RV>F", isgrp1,  &pitp->tll,    /* in vcdef.h  */
         "COLOR%N" KWS_CSEL,  &pitp->vc.vcxc,
         "X%VF",              &pitp->vc.vcplx,
         "Y%VF",              &pitp->vc.vcply,
         "W%V>F",             &pitp->vc.vcplw,
         "H%V>F",             &pitp->vc.vcplh,
         "KP%KHNIOKQ2GB",     &pitp->vc.kvp,
         "CPT%B14IH",         &pitp->vc.vcpt,
         NULL);

   /* Store any new option bits into the itop byte */
   pitp->itop = pitp->itop & ~(ITOP_INT|ITOP_PRES) | tops;
   /* Set default tla,tll if not entered */
   if (pitp->tll <= 0.0) pitp->tll = (float)pitp->vc.nsxl;
   if (pitp->tla <= 0.0) pitp->tla = (float)pitp->vc.nsya;
   /* Use vcmin,vcmax to store edges of touchpad in image units */
   pitp->vc.vcmin = pitp->tll/(float)pitp->vc.nsxl;
   pitp->vc.vcmax = pitp->tla/(float)pitp->vc.nsya;
   } /* End getitp() */


/***********************************************************************
*                              gtkregen                                *
*                                                                      *
*  This routine processes the REGENERATION control card.  On entry,    *
*  the control card has been read and is in RK.last.  The numerical    *
*  values assigned to RP->kregen are used arithmetically in the main   *
*  pgm to control calls to d3regenr, therefore no #defines are used    *
*  and order of keys in regenkeys declaration below is NOT arbitrary.  *
*  Furthermore, no default is assigned in d3dflt, so 0 (ON) is it.     *
***********************************************************************/

void gtkregen(void) {

#define NRGNKEYS 4
   static char *regenkeys[NRGNKEYS] =
      { "ON", "UNFROZEN", "FROZEN", "OFF" };

   cdscan(RK.last, 1, SCANLEN, RK_WDSKIP);
   RP->kregen =
      match(0, RK_MREQF, ~RK_COMMA, 0, regenkeys, NRGNKEYS) - 1;
   thatsall();
   } /* End gtkregen() */


/***********************************************************************
*                               getpcf                                 *
*                                                                      *
*  This routine is called to process the PCF and PAF cards.  New       *
*  options to set up products of von Mises, wrapped normal, and        *
*  wrapped exponential functions have been implemented, along with     *
*  new NORM and VERIFY options.  Caller must initialize cdscan()       *
*  and process the name field, allocating (d3grp1) or locating         *
*  (d3grp3) the appropriate named PCF structure.                       *
*                                                                      *
*  N.B.  Currently, the PCF is required to contain all positive values *
*  and this requirement is enforced.  If it is desired to allow negs,  *
*  logic in d3go() must be examined to be sure sums go into correct    *
*  excitatory or inhibitory accumulation bins.                         *
*                                                                      *
*  Arguments:                                                          *
*  ppcf        Ptr to PCF structure to be initialized                  *
*  kpcf        Default normalization (PCFN_NONE, PCFN_MAX, PCF_MEAN)   *
***********************************************************************/

void getpcf(struct PCF *ppcf, int kpcf) {

#define WRAPCUT  1.0E-6       /* Cutoff criterion for wrapped dists */
#define PCFNNO 4     /* Number of normalization options */
   static char *ncn[] = { "NONE", "MAX", "MEAN", "SUM" };

   float tab[PHASE_RES];   /* Temporary storage for table */
   float tn;               /* Normalizing constant */
   float cutoff = 0.0F;    /* Cutoff value */
   ui32 ic;                /* kwscan field recognition codes */
   int kvfy = 0;           /* Verify switch */
   int kwret;              /* kwscan return code */
   int p;                  /* Table entry index  */
   char lea[SCANLEN1];     /* Space for scanning */

/* If next field is a number, read the whole table,
*  otherwise initialize table to all 1.0 values.  */

   scanck(lea, RK_REQFLD, ~(RK_COMMA|RK_EQUALS));
   if (RK.scancode & RK_ENDCARD) return;
   if (cntrl(lea)) {       /* Nonnumeric field */
      scanagn();              /* Push back */
      for (p=0; p<PHASE_RES; p++) tab[p] = 1.0F;
      }
   else {                  /* Numeric field */
      scanagn();              /* Push back first number */
      inform("(S," qPHR "VF)", tab, NULL);
      }

/* Scan keyword options until end of card */

   while (kwret = kwscan(&ic,    /* Assignment intended */
         "VONMISES%X",
         "MISES%X",
         "WNORMAL%X",
         "WEXPONENTIAL%X",
         "NORMALIZATION%X",
         "CUTOFF%VF", &cutoff,
         "VERIFY%S1", &kvfy, NULL)) {
      if (RK.scancode != RK_EQUALS)
         ermark(RK_EQRQERR);
      switch (kwret) {

      case 1:              /* VON MISES */
      case 2: {            /* MISES (an abbreviation) */
         double angle,kappa;
         inform("(SV(QVQV))", &angle, &kappa, NULL);
         /* Generate the circular Gaussian function */
         angle *= (-DEGS_TO_RADS);
         tn = 1.0F/((float)TWOPI*bessi0((float)kappa));
         for (p=0; p<PHASE_RES; angle+=BINS_TO_RADS,p++)
            tab[p] *= tn*(float)exp(kappa*cos(angle));
         break;
         } /* End MISES local scope */

      case 3: {            /* WNORMAL */
         double angle,sigma,wangle,esig,term,sum;
         inform("(SV(QV>QV))", &angle, &sigma, NULL);
         /* Generate wrapped normal function */
         angle *= (-DEGS_TO_RADS);
         sigma *= DEGS_TO_RADS;
         tn = (float)RSQRT2PI/(float)sigma;
         esig = -0.5/(sigma*sigma);
         for (p=0; p<PHASE_RES; angle+=BINS_TO_RADS,p++) {
            sum = exp(esig*angle*angle);
            for (wangle=angle; ; ) {
               wangle += TWOPI;
               term = exp(esig*wangle*wangle);
               if (term < WRAPCUT) break;
               sum += term;
               }
            for (wangle=angle; ; ) {
               wangle -= TWOPI;
               term = exp(esig*wangle*wangle);
               if (term < WRAPCUT) break;
               sum += term;
               }
            tab[p] *= tn*(float)sum;
            } /* End angle loop */
         break;
         } /* End WNORMAL local scope */

      case 4: {            /* WEXPONENTIAL */
         double angle,tau,wangle,etau,term,sum;
         inform("(SV(QV>QV))", &angle, &tau, NULL);
         /* Generate wrapped exponential function */
         angle *= (-DEGS_TO_RADS);
         tau   *= DEGS_TO_RADS;
         etau = -1.0/tau;
         for (p=0; p<PHASE_RES; angle+=BINS_TO_RADS,p++) {
            sum = (angle >= 0.0) ? exp(etau*angle) : 0.0;
            for (wangle=angle; ; ) {
               wangle += TWOPI;
               term = exp(etau*wangle);
               if (term < WRAPCUT) break;
               sum += term;
               }
            tab[p] *= (float)sum;
            } /* End angle loop */
         break;
         } /* End WEXPONENTIAL local scope */

      case 5:              /* NORMALIZATION */
         kpcf = match(0, RK_MREQF, ~(RK_COMMA|RK_BLANK), 0, ncn,
            PCFNNO) - 1;
         if (kpcf < 0) kpcf = 0;    /* Safety */

      }} /* End kwret while and switch */

/* Determine normalizing constant according to kpcf option */

   tn = 0.0;
   switch (kpcf) {
      case PCFN_NONE:      /* NO NORMALIZATION */
         tn = 1.0;
         break;
      case PCFN_MAX:       /* NORMALIZE LARGEST ENTRY TO 1.0 */
         for (p=0; p<PHASE_RES; p++)
            if (tab[p] > tn) tn = tab[p];
         break;
      case PCFN_MEAN:      /* NORMALIZE MEAN ENTRY TO 1.0 */
         for (p=0; p<PHASE_RES; p++)
            tn += tab[p];
         tn /= (float)PHASE_RES;
         break;
      case PCFN_SUM:       /* NORMALIZE SUM OF ENTRIES TO 1.0 */
         for (p=0; p<PHASE_RES; p++)
            tn += tab[p];
         break;
      } /* End kpcf switch */

/* Normalize and store table values.  Don't forget to check
*  for overflow.  The test used is conservative but safe.  */

   if (tn <= 0.0)
      cryout(RK_E1, "0***CANNOT NORMALIZE PCF TABLE", RK_LN2, NULL);
   else {
      int novfl = 0;
      tn = 1.0/tn;
      for (p=0; p<PHASE_RES; p++) {
         float tnt = tn * tab[p];
         if (fabs(tnt) < cutoff) tnt = 0.0F;
         if (tnt >= 7.9999) ++novfl;
         ppcf->bins[p] = (si32)(tnt*dS28 + 0.5);
         }
      if (novfl > 0)
         cryout(RK_E1, "0***OVERFLOW SAVING PCF TABLE", RK_LN2, NULL);
      }

/* Print verification */

   if (kvfy) {
      convrt("(P4,'0VERIFY: '|#" qPHR ",(8B28IJ9.6))",
         ppcf->bins, NULL);
      cryout(RK_P4, " ", RK_LN1, NULL);
      }

   } /* End getpcf() */


/***********************************************************************
*                               getplab                                *
*                                                                      *
*  This routine processes the PLAB card                                *
***********************************************************************/

void getplab(void) {

   struct PLBDEF *pplb;
   ui32  ic;               /* kwscan field recognition codes */
   int   iplb;             /* Index of label, return codes */
   char  lea[SCANLEN1];    /* Scan return field */

   cdscan(RK.last, 1, SCANLEN, RK_WDSKIP);

/* Check for label index in parens and advance pplb.
*  In this case, positional parameters are optional and
*  keywords are allowed for changing subsets.
*  This is allowed even in Group I--can't hurt anything.  */

   if (scan(lea, RK_REQFLD) & RK_ENDCARD)
      return;
   if (RK.scancode == (RK_INPARENS|RK_RTPAREN)) {
      wbcdin(lea, &iplb, RK_ZTST|LEA2INT); iplb -= 1;
      for (pplb=RP0->pplb1; pplb && iplb; --iplb,pplb=pplb->pnxplb) ;
      if (!pplb) {
         ermark(RK_MARKFLD);
         cryout(RK_P1, "0***INDEX > NUM PLOT LABELS", RK_LN2, NULL);
         skip2end();
         return;
         }
      inform("(S=2*F,V>F,F,VA" qCMC ",TH" qLTF ")", &pplb->lbx,
         &pplb->lby, &pplb->lht, &pplb->ang,
         pplb->lbcolor, &pplb->hplabel, NULL);
      kwscan(&ic,
            "LX%F",          &pplb->lbx,
            "LY%F",          &pplb->lby,
            "HT%V>F",        &pplb->lht,
            "ANGLE%F",       &pplb->ang,
            "COLOR%VA" qCMC, &pplb->lbcolor,
            "TEXT%TH" qLTF,  &pplb->hplabel,
            "DISABLE%S0",    &pplb->kplbenb,
            "ENABLE%S1",     &pplb->kplbenb,
            NULL);
      }

/* If it's not a label index, push it back and add a new block
*  at the end of the existing list.  In this case, the positional
*  parameters are mandatory and the block is always enabled.  */

   else {
      scanagn();
      pplb = *RP0->plplb = (struct PLBDEF *)
         allocpmv(Host, sizeof(struct PLBDEF), "Plot label");
      RP0->plplb = &pplb->pnxplb;
      pplb->pnxplb = NULL;
      inform("(S,2*F,V>F,F,VA" qCMC ",TH" qLTF ")", &pplb->lbx,
         &pplb->lby, &pplb->lht, &pplb->ang,
         pplb->lbcolor, &pplb->hplabel, NULL);
      pplb->kplbenb = TRUE;
      thatsall();
      }

   pplb->lplabel = strlen(getrktxt((int)pplb->hplabel));
   } /* End getplab() */


/***********************************************************************
*                              getpline                                *
*                                                                      *
*  This routine processes the PLINE card.  Note parallel to getplab(). *
**********************e************************************************/

void getpline(void) {

   struct PLNDEF *ppln;
   ui32  ic;               /* kwscan field recognition codes */
   int   ipln;             /* Index of line, return codes */
   char  lea[SCANLEN1];    /* Scan return field */

   cdscan(RK.last, 1, SCANLEN, RK_WDSKIP);

/* Check for line index in parens and advance ppln.
*  In this case, positional parameters are optional and
*  keywords are allowed for changing subsets.
*  This is allowed even in Group I--can't hurt anything.  */

   if (scan(lea, RK_REQFLD) & RK_ENDCARD)
      return;
   if (RK.scancode == (RK_INPARENS|RK_RTPAREN)) {
      wbcdin(lea, &ipln, RK_ZTST|LEA2INT); ipln -= 1;
      for (ppln=RP0->ppln1; ppln && ipln; --ipln,ppln=ppln->pnxpln) ;
      if (!ppln) {
         ermark(RK_MARKFLD);
         cryout(RK_P1, "0***INDEX > NUM PLOT LINES", RK_LN2, NULL);
         skip2end();
         return;
         }
      inform("(S=4*F,VA" qCMC ",VIC)", &ppln->x1, &ppln->y1,
         &ppln->x2, &ppln->y2, ppln->lncolor, &ppln->kthick, NULL);
      kwscan(&ic,
            "X1%F",          &ppln->x1,
            "Y1%F",          &ppln->y1,
            "X2%F",          &ppln->x2,
            "Y2%F",          &ppln->y2,
            "COLOR%VA" qCMC, &ppln->lncolor,
            "RETRACE%VIC",   &ppln->kthick,
            "DISABLE%S0C",   &ppln->kplnenb,
            "ENABLE%S1C",    &ppln->kplnenb,
            NULL);
      }

/* If it's not a label index, push it back and add a new block
*  at the end of the existing list.  In this case, the positional
*  parameters are mandatory and the block is always enabled.  */

   else {
      scanagn();
      ppln = *RP0->plpln = (struct PLNDEF *)
         allocpmv(Host, sizeof(struct PLNDEF), "Plot line");
      RP0->plpln = &ppln->pnxpln;
      ppln->pnxpln = NULL;
      ppln->kthick = -1;
      inform("(S,4*F,VA" qCMC ",NVIC)", &ppln->x1, &ppln->y1,
         &ppln->x2, &ppln->y2, ppln->lncolor, &ppln->kthick, NULL);
      ppln->kplnenb = TRUE;
      thatsall();
      }

   } /* End getpline() */


/***********************************************************************
*                               getploc                                *
*                                                                      *
*  This routine processes the PLOC card.                               *
***********************************************************************/

void getploc(void) {

   ui32 ic;                   /* kwscan field recognition codes */
   inform("(SW1=+4*VF+4*VF)",
         &RP->eplx, &RP->eply, &RP->eplw,  &RP->eplh,
         RP0->vplx, RP0->vply, &RP0->vplw, &RP0->vplh, NULL);
   kwscan(&ic,
      "EX%VF",    &RP->eplx,
      "EY%VF",    &RP->eply,
      "EW%V>F",   &RP->eplw,
      "EH%V>F",   &RP->eplh,
      "VX%VF",    RP0->vplx,
      "VY%VF",    RP0->vply,
      "VW%V>F",   &RP0->vplw,
      "VH%V>F",   &RP0->vplh,
      "VX2%VF",   RP0->vplx+1,
      "VY2%VF",   RP0->vply+1,
      NULL);

   } /* End getploc() */


/***********************************************************************
*                              gtpscale                                *
*                                                                      *
*  This routine processes the PSCALE card.                             *
***********************************************************************/

void gtpscale(void) {

   ui32 ic;                /* kwscan field recognition codes */
   inform("(SW1=2*VF,2*V>F)", &RP0->splx, &RP0->sply,
      &RP0->splw, &RP0->splh, NULL);
   kwscan(&ic,
      "SPLX%VF", &RP0->splx,
      "SPLY%VF", &RP0->sply,
      "SPLW%V>F", &RP0->splw,
      "SPLH%V>F", &RP0->splh, NULL);
   RP0->kplt1 |= PLCSP;

   } /* End gtpscale() */


/*---------------------------------------------------------------------*
*                               obvList                                *
*                                                                      *
*  Creates a list of objxy, objid, or value options from a SAVE card.  *
*                                                                      *
*  Argument:                                                           *
*     ppil     Ptr to ptr to iteration list to be constructed.         *
*                                                                      *
*  Note:  The iteration list is a little ponderous for these simple    *
*  lists, but gives us for free the ability to modify existing lists   *
*  at Group III time.                                                  *
*---------------------------------------------------------------------*/

static void obvList(ilst **ppil) {

   ilstsalf(callocv, reallocv, freev);
   RP0->ilseed += 107;  /* Any old number will do */
   *ppil = ilstread(*ppil, IL_NEW|IL_ELOK|IL_ESOK, 1, RP0->ilseed);

   } /* End obvList() */


/***********************************************************************
*                               getsave                                *
*                                                                      *
*  This routine is called to process the SAVE card.  It is assumed     *
*  that the card has already been printed.                             *
*
*  Note:  Since this card can be encountered before or after a REPLAY  *
*  card, it is simplest to go through the full interpretation, then    *
*  turn off writing the SIMDATA file if subsequently it turns out a    *
*  replay is in progress.  Or, it might be desired sometime to modify  *
*  d3gfsv() calls to allow writing a new SIMDATA while doing a replay. *
*                                                                      *
*  Note 07/30/09:  To avoid option conflicts, FULL_IA and IA options   *
*  and SVITMS=G were removed and bits set to word options saved while  *
*  SVITMS changed.  svops enum added to minimize switch errors.        *
*                                                                      *
*  Note 04/24/11:  As of this date, it is now possible to initiate     *
*  SIMDATA saving at Group I or Group III time, although initiation    *
*  at Group III cannot cause a variable to be allocated and computed.  *
***********************************************************************/

void getsave(void) {

#define MXFLDLEN ((SCANLEN > MXKEYLEN) ? SCANLEN : MXKEYLEN)

   struct SDSELECT *pgd;   /* Pointer to SIMDATA selections  */
   char lea[MXFLDLEN+1];   /* Scan buffer */

   /* N.B.  Since d3clst checks for names "NEW", "ADD", "DEL", "OFF",
   *  and "MXCELLS", those option keys must be listed here and go to
   *  case SVO_NONE if they could otherwise be incorrectly recognized
   *  as abbreviations of one of the SAVE card keys.  */
   static char *svop[] = { "NETWORK", "NEWSERIES", "CREATE-FILENAMES",
      "ROTATE-FILENAMES", "NOREPLAY", "REPLAY",
      "RESPONSES", "SIMDATA", "ENABLE", "DISABLE", "SVITMS",
      "GRAPH", "TIMER", "CLIST", "NEW", "OBJECT", "OBJXY",
      "OBJID", "OBJALL", "VALUE", "SENSES", "SNSID", "UTVINFO" } ;
   enum svops { SVO_MENDC=-2, SVO_NONE=0, SVO_NETW, SVO_NEWS,
      SVO_CREAT, SVO_ROT, SVO_NOREP, SVO_REPL, SVO_RESP, SVO_SIMD,
      SVO_ENB, SVO_DIS, SVO_SVIT, SVO_GRAF, SVO_TIMR,
      SVO_CLIST, SVO_NEW, SVO_OBJECT, SVO_OBJXY, SVO_OBJID,
      SVO_OBJALL, SVO_VALUE, SVO_SENSE, SVO_SNSID, SVO_UTVINF } smret;

   cdscan(RK.last, 1, MXFLDLEN, RK_WDSKIP|RK_NEST);

   /* Look for keyword options */
   while ((smret = (enum svops)match(RK_NMERR, RK_MNEWK,
         ~(RK_BLANK|RK_COMMA|RK_LFTPAREN|RK_EQUALS|RK_INPARENS),
         0, svop, SVO_UTVINF)) != SVO_MENDC) {

      /* If in parens, this has to be a cell list.  Reset smret
      *  so that if a region name happens to be the same as one
      *  of the recognized keywords, we will treat it as a cell
      *  list and not as one of those keywords.  */
      if (RK.scancode & RK_INPARENS) smret = SVO_NONE;

      /* Make sure SIMDATA file exists and SDSELECT block is
      *  allocated when needed.  */
      if (!RP0->pgds && (smret == SVO_NONE || smret > SVO_ROT)) {
         RP0->pgds = (SDSTYPE *)allocpcv(Host, 1,
            sizeof(SDSTYPE), "SIMDATA Info block");
         RP0->pgds->gdfile = SIMDATA;
         RP->ksv &= ~KSDNE;   /* Saves pgds tests later */
         } /* End not NETWORK and no SDSELECT */

      pgd = RP0->pgds;
      switch (smret) {

         case SVO_NONE:
         case SVO_NEW:
            /* If no recognized keyword, assume this is a cell
            *  list--if not, d3clst will generate an error.  */
            scanagn();
            if (pgd->sdclloc.clnum >= 0)
               pgd->sdclloc.clnum = --RP0->nextclid;
            d3clst((int)pgd->sdclloc.clnum, CLKM_INDIV);
            break;

         case  SVO_NETW:      /* NETWORK  */
            RP->ksv |= (RP0->ccgrp & CCGRP1) ? KSVSR : KSVS3;
            break;

         case  SVO_NEWS:      /* NEWSERIES */
            RP->ksv |= KSVNS;
            break;

         case  SVO_CREAT:     /* CREATE-FILENAMES */
            RP->ksv &= ~KSVRFN;
            RP->ksv |= KSVCFN;
            break;

         case  SVO_ROT:       /* ROTATE-FILENAMES */
            RP->ksv &= ~KSVCFN;
            RP->ksv |= KSVRFN;
            break;

         case  SVO_NOREP:     /* NOREPLAY */
            pgd->svitms &= ~GFGLBL;
            break;

         case  SVO_REPL:      /* REPLAY   */
         case  SVO_RESP:      /* RESPONSES */
            pgd->svitms |= GFGLBL;
            break;

         case  SVO_SIMD:      /* SIMDATA  */
            /* Just needs RP0->pgds set--already done above */
            break;

         case  SVO_ENB:       /* ENABLE   */
            RP->ksv &= ~KSDOFF;
            break;

         case SVO_DIS:        /* DISABLE  */
            RP->ksv |= KSDOFF;
            break;

         case SVO_SVIT:       /* SVITMS   */
         case SVO_GRAF:       /* GRAPH (retained for compat) */
            /* NOTE:  Codes are defined in simdata.h.  Obsolete AHD
            *  options are no longer recognized or have new meanings.
            */
            if (RK.scancode != RK_EQUALS)
               ermark(RK_EQRQERR);
            else {
/* Define bits that must be preserved by SVITMS code changes */
#define SVSVI (GFSI|GFGLBL|GFVAL|GFOXY|GFOID|GFSNS|GFSID|GFUTV)
               ui32 svsvi = pgd->svitms & SVSVI;
               scanck(lea, RK_REQFLD|RK_PMEQPM, ~RK_COMMA);
               mcodes(lea, oksimgl, &pgd->svitms);
               pgd->svitms = pgd->svitms & ~SVSVI | svsvi;
               }
            break;

         case SVO_TIMR:       /* TIMER    */
            RP->jsdsvtm = gettimno(NULL, YES);
            break;

         case SVO_CLIST:      /* CLIST    */
            /* Remove any existing idiosyncratic cell list */
            if (pgd->sdclloc.clnum < 0) d3clrm(pgd->sdclloc.clnum);
            if (getclid(&pgd->sdclloc))
               memset(&pgd->sdclloc, 0, sizeof(clloc));
            break;

         case SVO_OBJECT:     /* OBJECT (retained for compat) */
         case SVO_OBJXY:      /* OBJXY    */
            obvList(&pgd->pilobj);
            if (pgd->pilobj) pgd->svitms |= GFOXY;
            else             pgd->svitms &= ~(GFOXY|GFOID);
            break;

         case SVO_OBJID:      /* OBJID    */
            obvList(&pgd->pilobj);
            if (pgd->pilobj) pgd->svitms |= GFOID;
            else             pgd->svitms &= ~(GFOXY|GFOID);
            break;

         case SVO_OBJALL:     /* OBJALL   */
            obvList(&pgd->pilobj);
            if (pgd->pilobj) pgd->svitms |= (GFOXY|GFOID);
            else             pgd->svitms &= ~(GFOXY|GFOID);
            break;

         case SVO_VALUE:      /* VALUE    */
            obvList(&pgd->pilval);
            if (pgd->pilval) pgd->svitms |= GFVAL;
            else             pgd->svitms &= ~GFVAL;
            break;

         case SVO_SENSE:      /* SENSE    */
         case SVO_SNSID:      /* SNSID    */
            if (RK.scancode != RK_EQUALS) {
               ermark(RK_PUNCERR);
               skip2end();
               return; }
            /* Note:  This code currently allows repeating or adding
            *  new senses to an existing list at Group III time, but
            *  not removing any senses--why bother, just ignore when
            *  reading via getgd3.  */
            do {
               struct GDSNS *pgsns, **pplgdsns;
               char tvcname[LSNAME];   /* Temps for GDSNS */
               txtid hvcnm,thgsnm;
               scanck(lea, 0,
                  ~(RK_COMMA|RK_INPARENS|RK_LFTPAREN|RK_RTPAREN));
               hvcnm = (txtid)savetxt(lea);
               if (RK.scancode & RK_LFTPAREN) {
                  /* Got left paren, is built-in name */
                  if (RK.length >= LSNAME) ermark(RK_LENGERR);
                  memcpy(tvcname, lea, LSNAME);
                  scanck(lea, RK_REQFLD, ~(RK_INPARENS|RK_RTPAREN));
                  if (!(RK.scancode & RK_RTPAREN)) {
                     ermark(RK_PUNCERR);
                     skip2end();
                     return; }
                  wbcdin(lea, &thgsnm, RK_IORF|RK_QPOS|RK_CTST|
                     RK_ZTST|RK_NHALF|RK.length);
                  }
               else {
                  /* No left paren, must be an external (BBD)
                  *  name, set tvcname = 0 as a flag of this.  */
                  tvcname[0] = 0;
#ifdef BBDS
                  thgsnm = hvcnm;
#else
                  ermark(RK_IDERR);
#endif
                  }
               /* If this name is already listed, ignore it
               *  (allowed in Group III, error in Group I).  */
               pplgdsns = &pgd->pgdsns[smret != SVO_SENSE];
               for ( ; pgsns=*pplgdsns; pplgdsns=&pgsns->pngsns) {
                  if (!strncmp(tvcname, pgsns->vcname, LSNAME) &&
                        thgsnm == pgsns->hgsnm) {
                     if (RP0->ccgrp == CCGRP1) {
                        ermark(RK_MARKFLD);
                        dupmsg("SENSE", NULL, hvcnm);
                        }
                     goto NextSenseName;
                     }
                  }
               /* Not found, add to end of list */
               *pplgdsns = pgsns = (struct GDSNS *)
                  allocpcv(Host, 1, sizeof(struct GDSNS),
                  "SIMDATA Sense List Block");
               memcpy(pgsns->vcname, tvcname, LSNAME);
               pgsns->hgsnm = thgsnm;
               pgd->svitms |= (smret == SVO_SENSE) ? GFSNS : GFSID;
NextSenseName: ;
               } while (curplev() > 0);
            break;

         case SVO_UTVINF:     /* User program TV info */
            eqscan(&pgd->hcamnm, "TH" qTV_MAXNM, RK_EQCHK);
            if (strcmp(getrktxt(pgd->hcamnm), "OFF"))
               pgd->svitms |= GFUTV;
            else {
               pgd->hcamnm = 0; pgd->svitms &= ~GFUTV; }
            break;

         default:  /* Probably a punctuation error */
            skip2end();
            return;

         } /* End MATCH switch */
      } /* End MATCH while */

   if (RP->ksv & KSVOK && RP->ksv & (KSVSR|KSVS3|KSVNS)) {
      if (!(RP0->psnf = d3file[SAVREP_OUT])) /* Assignment intended */
         cryout(RK_E1, "0***SAVENET FILE REQUIRED.", RK_LN2, NULL);
      else if (RP->ksv & KSVRFN && !d3file[SAVREP_OUT]->nf)
         cryout(RK_E1, "0***TWO OR MORE SAVENET FILES REQUIRED.",
            RK_LN2, NULL);
      }

   if (RP0->pgds && !d3file[SIMDATA])
      cryout(RK_E1, "0***SIMDATA FILE REQUIRED.", RK_LN2, NULL);

   } /* End getsave() */


/***********************************************************************
*                               getstat                                *
*                                                                      *
*  This routine may be called at Group I or Group III time to modify   *
*  certain statistical options.  Note that if statistics are turned    *
*  off initially, certain options cannot be turned on later because    *
*  memory will not have been allocated.  Also, at present, there is    *
*  no provision to control LASTCYC vs ALLCYC or SPOUT vs NOSPOUT for   *
*  individual cell types.                                              *
***********************************************************************/

void getstat(void) {

   int smret;
#define STATNUM 5       /* Number of STATISTICS options */
   static char *statopt[STATNUM] = {
      "NONE", "ALLCYC", "LASTCYC", "SPOUT", "NOSPOUT" };
   enum statops
      { StO_None = 1, StO_All, StO_Last, StO_SS, StO_NOSS };

   cdscan(RK.last, 1, SCANLEN, RK_WDSKIP);
   while ((smret = match(OFF, RK_MNEWK, ~RK_COMMA, 0,
         statopt, STATNUM)) != RK_MENDC)
      switch ((enum statops)smret) {
      case StO_None:       /* NONE */
         RP->CP.runflags |= RP_NOSTAT;
         break;
      case StO_All:        /* ALLCYC */
         RP->CP.runflags |= RP_ALLCYC;
         break;
      case StO_Last:       /* LASTCYC */
         RP->CP.runflags &= ~RP_ALLCYC;
         break;
      case StO_SS:         /* SPOUT */
         RP->CP.runflags |= RP_SSTAT;
         break;
      case StO_NOSS:       /* NOSPOUT */
         RP->CP.runflags &= ~RP_SSTAT;
         break;
      } /* End while and switch */
   return;
   } /* End getstat() */


/***********************************************************************
*                               getway                                 *
*                                                                      *
*  This routine may be called from g2amplif() or d3chng() to read in   *
*  either a set of new-style amplification scales (8 numerical values  *
*  in parentheses) or an old-style amplification way code (a string of *
*  four or eight characters).  If the amplification rule is 'E', then  *
*  d3go uses the wayset values entered by the user, otherwise, they    *
*  are replaced with values appropriate for the given rule code.  Note *
*  that the character values read in the old-style wayset code are     *
*  supposed to be '0', '1', or '3', but by long tradition, only the    *
*  two low-order bits of each character are inspected to determine the *
*  intended scale.                                                     *
*                                                                      *
*  Note that the scales are stored as L2S10 values--when more precision*
*  is needed, the user should set the global scale with mdelta and use *
*  wayset scales just to modify the global scale.                      *
*                                                                      *
*  Re the warnings for missing values:  In general, there is no way    *
*  at the time this routine is run to know whether there should be 4   *
*  or 8 values, so the warning has been moved to d3nset.               *
*                                                                      *
*  Arguments:                                                          *
*  pdd      Ptr to CNDFLT struct, either global or conntype-specific   *
***********************************************************************/

void getway(struct CNDFLT *pdd) {

   int j;                  /* Character counter */
   char lea[SCANLEN1];     /* Space for scanning */

   /* First eliminate any previous scale settings */
   memset((char *)pdd->wayset, 0, MAX_WAYSET_CODES*sizeof(short));
   scanck(lea, RK_REQFLD,
      ~(RK_COMMA|RK_BLANK|RK_INPARENS|RK_RTPAREN));
   if (RK.scancode & RK_INPARENS) {
      /* Got new-style list of scales in parens */
      scanagn();
      j = MAX_WAYSET_CODES;
      inform("(SV(NRB10IHV))", &j, pdd->wayset, NULL);
      pdd->nwayrd = RK.numcnv | NWSCLE;
      } /* End reading parenthesized scale list */
   else {
      /* Got old-style string of codes */
      if (RK.length >= MAX_WAYSET_CODES) ermark(RK_LENGERR);
      else for (j=0; j<=RK.length; j++)
         pdd->wayset[j] = (lea[j] & 1) ? (lea[j] & 2 ? -S10 : S10) : 0;
      pdd->nwayrd = j;
      } /* End reading character-string codes */

   } /* End getway() */
