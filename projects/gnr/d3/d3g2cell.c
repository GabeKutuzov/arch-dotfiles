/* (c) Copyright 1996-2018, The Rockefeller University *11116* */
/* $Id: d3g2cell.c 77 2018-03-15 21:08:14Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                              d3g2cell                                *
*  Read group II input cards relating to repertoires and cell types.   *
*                                                                      *
*  When d3grp2 encounters a control card bearing repertoire or cell    *
*  type parameters, it allocates and initializes a new parameter       *
*  block of the appropriate type, then calls one of the routines       *
*  in this file to process the control card.                           *
*                                                                      *
************************************************************************
*  V8A, 10/05/96, GNR - Broken out from d3tree                         *
*  Rev, 03/10/97, GNR - Add KRPEP code                                 *
*  Rev, 09/25/97, GNR - Replace CELLTYPE REPLAY with new SVITMS codes  *
*  Rev, 10/02/97, GNR - Independent dynamic allocations per conntype,  *
*                       allow AMPLIF card to follow CONNTYPE, SFHMATCH *
*  Rev, 12/23/97, GNR - Add ADECAY, PPF                                *
*  Rev, 02/18/98, GNR - Add resting potential, afterhyperpolarization  *
*  V8B, 01/27/01, GNR - Add RTIMER                                     *
*  V8C, 02/27/03, GNR - Cell s(i) in mV, add conductances, kill INOISE *
*  V8D, 11/11/06, GNR - Make D,P,T,Q,K options same in rep,cell blocks *
*  Rev, 10/21/07, GNR - Add TANH, tiered threshold defaults            *
*  ==>, 01/09/08, GNR - Last mod before committing to svn repository   *
*  Rev, 03/26/08, GNR - Add CELLTYPE SHAPE                             *
*  Rev, 05/20/08, GNR - Add getthr() and calls to it                   *
*  V8E, 01/14/09, GNR - Add g2izhi(), getsicol(), remove OPTMV         *
*  Rev, 02/12/09, GNR - Restructure g2phase to use PHASEDEF blocks     *
*  Rev, 06/13/09, GNR - Bug fix: g2decay gave PUNCERR if only omega1   *
*  V8G, 08/12/10, GNR - Add g2autsc                                    *
*  V8H, 11/23/10, GNR - Remove COMPAT=C neg test for mtj, mnsj, etc.   *
*  Rev, 04/21/11, GNR = Add phifn control                              *
*  Rev, 06/07/12, GNR - Add adefer per conntype, 06/30/12 add qpull    *
*  Rev, 08/29/12, GNR - Add upspsi, zetapsi to g2decay input           *
*  Rev, 04/23/13, GNR - Add mxsi to AMPLIF, PARAMS                     *
*  Rev, 05/08/13, GNR - Remove error on AUTOSCALE TI options together  *
*  Rev, 05/16/13, GNR - Add parameters for AUTOSCALE 'H' option        *
*  Rev, 06/25/13, GNR - Add V~ test on stanh                           *
*  Rev, 08/22/14, GNR - Si only EXP decay, remove DECAYDEF from DCYDFLT*
*  R63, 11/03/15, GNR - Add vdopt codes                                *
*  R65, 01/02/16, GNR - n[gq][xyg] to ui16, ngrp to ui32, ncpg to ubig *
*  R66, 02/17/16, GNR - Change long random seeds to typedef si32 orns  *
*  R72, 02/22/17, GNR - Add SVCLIST                                    *
*  R72, 03/09/17, GNR - Remove krp KRPRR,KRPSP, add Rp.xgap,ygap       *
*  R72, 03/29/17, GNR - Remove CELLTYPE BITS, add TPCLIST              *
*  R74, 08/05/17, GNR - Add COLSRC, explicit color, no more SOLID|OPEN *
*  R74, 09/13/17, GNR - Add CPCAP                                      *
*  R77, 02/13/18, GNR - Enhanced d3lplt bars options, LPCLIST          *
*  R78, 07/11/18, GNR - Add ibias                                      *
***********************************************************************/

#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "d3global.h"
#include "clblk.h"
#include "d3opkeys.h"
#include "rocks.h"
#include "rkxtra.h"
#include "bapkg.h"

/* Followig option names must be in same order as phifn codes in
*  CNDFLT structure.  Second set is 5-letter names for d3echo().  */
char *phifnnm[2][FFN_N] = { { "DEFAULT", "1", "ABSCIJ" },
                            { "DFLT",    "1", "|CIJ|"  } };

/*---------------------------------------------------------------------*
*                              getrspfn                                *
*  This little routine interprets the RF option on the MASTER CELLTYPE,*
*  REGION, and CELLTYPE control cards after kwscan option match.       *
*---------------------------------------------------------------------*/

int getrspfn(void) {

   /* Following option names must be in same order as rspmeth codes in
   *  enum RespFunc.  A few synonyms are offered for IZHIKEVICH 2007 */
#define RF_NOPT 9             /* Number of enterable RespFunc names */
   static char *rfnms[] =
      { "KNEE", "STEP", "SPIKE", "TANH", "AEIF",
        "IZHI2003", "IZHI2007", "IZHI", "IZHIKEVICH" };
   int km = match(RK_EQCHK, RK_MREQF, ~RK_COMMA, 0, rfnms, RF_NOPT);
   if (km > RF_IZH7) km = RF_IZH7;
   return km;

   } /* End getrspfn() */


/*---------------------------------------------------------------------*
*                              getshape                                *
*  This little routine interprets the SHAPE option on the MASTER       *
*  CELLTYPE, CELLTYPE, and CHANGE control cards.                       *
*---------------------------------------------------------------------*/

void getshape(struct CTDFLT *pctd) {

   int ibar,jbar, ksh;
   char lea[SCANLEN1];
   enum SiShapes kshe;
   /* Following shape names must be in same order as SiShapes enum
   *  in celltype.h  */
   static char *sishp[] = { "CIRCLE", "SQUARE", "DOWN-TRIANGLE",
      "LEFT-TRIANGLE", "UP-TRIANGLE", "RIGHT-TRIANGLE",
      "PHASE-NEEDLE", "ORIENT-NEEDLE", "BARS", "GMXBARS" };
   /* Letter codes to match bar options in enum SiBarId */
   static char sisletts[] = "0SVWDNU";

   ksh = match(RK_EQCHK, RK_MREQF, ~(RK_COMMA|RK_LFTPAREN),
      0, sishp, sizeof(sishp)/sizeof(char *));
   pctd->ksiple = (byte)SIS_None; pctd->ksbarn = 0;
   if (ksh <= 0) return;
   kshe = (enum SiShapes)ksh;
   if (kshe == SIS_Bars || kshe == SIS_GMxBars) {
      if ((enum SiColSrc)pctd->ksipce != SIC_None) {
         if (RP0->ccgrp == CCGRP3) { cryout(RK_P1, "0-->WARNING: "
            "Incompatible COLSRC option inactivated.", RK_LN2, NULL);
            pctd->ksipce == SIC_None; }
         else { cryout(RK_E1, "0***SHAPE=[GMX]BARS is incompatible "
            "with COLSRC.", RK_LN2, NULL); return; }
         }
      /* Bars -- get codes for types of bars */
      if (!(RK.scancode == RK_LFTPAREN)) { ermark(RK_PNRQERR); return; }
      scan(lea, RK_REQFLD);
      if (RK.scancode & ~(RK_INPARENS|RK_RTPAREN|RK_COMMA))
         { ermark(RK_PUNCERR); return; }
      if (RK.length >= MxKSBARS) { ermark(RK_LENGERR); return; }
      for (ibar=0; ibar<=RK.length; ++ibar) {
         char tlet = lea[ibar];
         for (jbar=0; jbar<(int)SIB_Aff; ++jbar) {
            if (tlet == sisletts[jbar]) {
               pctd->kksbar[ibar] = (byte)jbar; goto NextIbar; }
            }
         /* '0' means skip this bar, '1' to '9' is ict number */
         if (tlet < '0' || tlet > '9') { ermark(RK_IDERR); return; }
         pctd->kksbar[ibar] = (byte)SIB_Aff | (tlet - '0') << 4;
NextIbar:   ;
         }
      pctd->ksbarn = ibar;
      }  /* End special handling for BARS shape */
   /* Now that we have no errors, store the requested option */
   pctd->ksiple = (byte)ksh;
   } /* End getshape() */


/*---------------------------------------------------------------------*
*                              getsicol                                *
*  This little routine interprets the COLOR option on the MASTER       *
*  CELLTYPE, CELLTYPE, and CHANGE control cards.  As of R74, color     *
*  source is now a separate COLSRC option.  All that is left is:       *
*  COLOR={Colorspec|SEQUENCE|(exccolor,inhcolor)}                      *
*---------------------------------------------------------------------*/

void getsicol(struct CTDFLT *pctd) {

   if (RK.scancode == RK_EQUALS) {
      char lea[COLMAXCH+1];
      int oldlen = scanlen(COLMAXCH);
      scanck(lea, RK_REQFLD,
         ~(RK_COMMA|RK_INPARENS|RK_LFTPAREN|RK_RTPAREN));
      if (RK.scancode & RK_INPARENS) {
         scanagn();
         inform("(SV(2VA" qCMC "V))", pctd->sicolor, NULL);
         }
      else {
         /* This could be SEQUENCE or an actual color */
         strncpy(pctd->sicolor[EXCIT], lea, COLMAXCH);
         strncpy(pctd->sicolor[INHIB], lea, COLMAXCH);
         }
      scanlen(oldlen);
      }
   else {                     /* Punctuation error, no '=' */
      ermark(RK_EQRQERR);
      }

   } /* End getsicol() */


/*---------------------------------------------------------------------*
*                              getscsrc                                *
*  This little routine interprets the COLSRC option on the MASTER      *
*  CELLTYPE, CELLTYPE, and CHANGE control cards.  As of R74, this      *
*  is a separate option and no longer interacts with kctp options.     *
*  Specifically, this option no longer affects SOLID vs OPEN coloring. *
*---------------------------------------------------------------------*/

void getscsrc(struct CTDFLT *pctd) {

   enum SiShapes kshe = (enum SiShapes)pctd->ksiple;
   /* The following color source names must be in the same order as
   *  SiColSrc enum in celltype.h */
   static char *sicol[] = { "SI", "SBAR", "QBAR", "PHASE", "HISTORY",
      "IZHU", "GRPAVG", "GRPMAX" };

   if (kshe == SIS_Bars || kshe == SIS_GMxBars) {
      if (RP0->ccgrp == CCGRP3) { cryout(RK_P1, "0-->WARNING: Incom"
         "patible SHAPE=[GMX]BARS option inactivated.", RK_LN2, NULL);
         pctd->ksiple == SIS_Square; }
      else { cryout(RK_E1, "0***COLSRC is incompatible with "
         "SHAPE=[GMX]BARS.", RK_LN2, NULL); return; }
      }

   pctd->ksipce = match(RK_EQCHK, RK_MREQF, ~RK_COMMA, 0, sicol,
      sizeof(sicol)/sizeof(char *));

   } /* End getscsrc() */


/*---------------------------------------------------------------------*
*                              getvrest                                *
*  Read a value for Vrest and use it to store an adjuster named "Vr".  *
*  Check that the value is not negative if in COMPAT=C mode.  Store    *
*  it for d3echo.                                                      *
*---------------------------------------------------------------------*/

void getvrest(struct CTDFLT *pctd, int ierr) {

   double tvr;

   eqscan(&tvr, "Q$mV", ierr);
   if (tvr < 0.0 && RP->compat & OLDRANGE) {
      cryout(RK_E1, "0***NEGATIVE Vrest NOT ALLOWED WITH "
         "COMPAT=C.", RK_LN2, NULL);
      tvr = 0.0; }
   svvadj(tvr, "Vr");
   pctd->vrest = (si32)(tvr * (double)(1L << (RP->bsdc + FBwk)));

   } /* End getvrest() */


/*---------------------------------------------------------------------*
*                Process 'REGION' or 'REPERTOIRE' card                 *
*                                                                      *
*  Synopsis:  void g2rep(struct REPBLOCK *ir)                          *
*                                                                      *
*  Caution: if krp codes changed, must change also in d3chng.          *
*---------------------------------------------------------------------*/

void g2rep(struct REPBLOCK *ir) {

   struct REPBLOCK *jr;
   ui32  ic;                  /* kwscan field recognition codes */
   short tgrid = -1;          /* Signal no GRID read */

/* Read data from REGION or REPERTOIRE card */

   inform("(S=VI,VI)", &ir->nqx, &ir->nqy, NULL);
   ir->ngrp = (ui32)ir->ngx*(ui32)ir->ngy;
   kwscan(&ic,
      "KP%KH" okrpkrp,        &ir->Rp.krp,
      "KRP%KH" okrpkrp,       &ir->Rp.krp,
      "NQX%VI",               &ir->nqx,
      "NQY%VI",               &ir->nqy,
      "GRIDS%V>IH",           &tgrid,
      "XGRID%V>IH",           &ir->Rp.ngridx,
      "YGRID%V>IH",           &ir->Rp.ngridy,
      "X%VF",                 &ir->aplx,
      "Y%VF",                 &ir->aply,
      "W%VF",                 &ir->aplw,
      "H%VF",                 &ir->aplh,
      "VXO%F",                &ir->Rp.vxo,
      "VYO%F",                &ir->Rp.vyo,
      "XGAP%VF",              &ir->Rp.xgap,
      "YGAP%VF",              &ir->Rp.ygap,
      "BPTIMER%N" KWS_TIMNW,  &ir->Rp.jbptim,
      NULL);

   if (tgrid > 0) ir->Rp.ngridx = ir->Rp.ngridy = tgrid;

/* Error if repertoire sname duplicates another repertoire name.
*  It is now OK to duplicate a sense name--can be distinguished on
*  CONNTYPE etc. cards by putting repname in parens.
*  Note:  bltsnm test was erroneously applied to il->lname until
*  V7A.  -GNR  */

   for (jr=RP->tree; jr!=ir; jr=jr->pnrp) {
      if (strncmp(ir->sname, jr->sname, LSNAME) == 0) {
         dupmsg("SHORT REGION", ir->sname, LSNAME);
         break; }
      } /* End jr loop */

   } /* End g2rep() */


/*---------------------------------------------------------------------*
*                       Process 'CELLTYPE' card                        *
*                                                                      *
*  Synopsis:  void g2cell(struct REPBLOCK *ir, struct CELLTYPE *il)    *
*                                                                      *
*  N.B.  Printing, plotting, and statistical options may now (V8A,V8D) *
*     be controlled at the MASTER, REPERTOIRE, or CELLTYPE level--     *
*     relevant bits are in the CTDFLT struct and can be combined with  *
*     the '+' or '-' codes in mcodes().  KRPNS (omit stats) bit takes  *
*     precedence over all bits that request statistics.  Options C,Z,  *
*     Y,H,X,R,M,S,1 can't be entered on the CHANGE card unless memory  *
*     was preallocated by entry at Group II time.  For compatibility   *
*     with old input files, KP will be allowed here as a synonym for   *
*     KCTP, but undocumented--some options are now different.          *
* V8E, 02/11/09, GNR - ctopt=P is now phase, not persistence limit,    *
*                      ctopt=M is now optimize memory, not phase max   *
* R74, 08/06/17, GNR - Separate kctp vs stats, add COLSRC, expand      *
*                      COLOR to EXCCOLOR, INHCOLOR (sim to ijpl stuff) *
*---------------------------------------------------------------------*/

void g2cell(struct REPBLOCK *ir, struct CELLTYPE *il) {

   struct SFHMATCH *phm = getsfhm(SFHMCELL, il);
   struct CELLTYPE *jl;       /* Used to search for duplicate names */
   ui32 ic = 0;               /* kwscan field recognition codes */
   int  kwret;                /* kwscan return code */

/* Read data from CELLTYPE card.  If the number of parameters on
*  this card becomes too large, the planned solution is to move
*  pt, nt, st, etc. to a new CELLTYPE-level THRESHOLDS card.  */

   inform("(SW1A" qLSN "VI=" qNCTT "B20/27IJ$mV)",
      il->lname, &il->nel, &il->Ct.pt, NULL);
   /* Establish defaults for RESTORE files and UPWITH */
   phm->fno = 0;
   setrlnm(&phm->rrennm, ir->sname, il->lname);
   setrlnm(&il->ctwk.tree.usrcnm, ir->sname, il->lname);
   /* Scan keyword options.  N.B.  'ic' bits for commented items
   *  must match KRM_ defs in celltype.h--currently only bits 1-8 --
   *  bits not commented here may be used for other purposes.  */
   while (kwret = kwscan(&ic,    /* Assignment intended */
         "RF%X",                                   /* ic bit 1 */
         "SHAPE%X",
         "COLOR%X",
         "COLSRC%X",
         "VREST%X",
         "REST%X",                                 /* For compat */
         "SPIKE%VB20/27IJ$mV",   &il->Ct.sspike,   /* ic bit 7 */
         "THAMP%V~B20/27IJ$mV",  &il->Ct.stanh,    /* ic bit 8 */
         "PT%B20/27IJ$mV",       &il->Ct.pt,
         "ST%B20/27IJ$mV",       &il->Ct.st,
         "NT%B20/27IJ$mV",       &il->Ct.nt,
         "HT%B20/27IJ$mV",       &il->Ct.ht,
         "CM%V>8B23UJ$pF",       &il->Ct.Cm,
         "CPCAP%B7/14IH$mV",     &il->Ct.cpcap,
         "CPR%VF",               &il->Ct.cpr,
         "CPT%B7/14IH$mV",       &il->Ct.cpt,
         "UPWITH%N" KWS_RLNM,    &il->ctwk.tree.usrcnm,
         "RNAME%N" KWS_RLNM,     &phm->rrennm,
         "SIHA%B20/27IJ$mV",     &il->Ct.siha,
         "SLOPE%V>B" qqv(FBsc) "IJ", &il->Ct.sslope,
         "TAUM%V>B20UJ$ms",      &il->Ct.taum,
         "PLOTCT%IH",            &il->jijpl,
         "SDCLIST%X",
         "TPCLIST%X",
         "LPCLIST%X",
         "IBIAS%B7/14IH$mV",     &il->ibias,
         "MNCIJ%B8IH",           &il->mncij,
         "MXCIJ%B8IH",           &il->mxcij,
         "AUSAGE%N" KWS_PSPOV,   &il->apspov,
         "OPT%KH" okctopt,       &il->Ct.ctopt,
         "CTOPT%KH" okctopt,     &il->Ct.ctopt,
         "CNOPT%KH" okcnopt,     &RP0->LCnD.cnopt,
         "VDOPT%KC" okvdopt,     &RP0->LCnD.vdopt,
         "SVITMS%KJ" oksimct,    &il->svitms,
         "RFILE%VIH",            &phm->fno,
         "KCTP%KH" okctkctp,     &il->Ct.kctp,
         "KP%KP" okctkctp,       &il->Ct.kctp,
         "KETIMER%N" KWS_TIMNW,  &il->Ct.jketim,
         "STATS%KH" okctsta,     &il->Ct.kstat,
         NULL))
      switch (kwret) {
case 1:  /* RF (response function) option */
         il->Ct.rspmeth = getrspfn();
         break;
case 2:  /* SHAPE option */
         getshape(&il->Ct);
         break;
case 3:  /* COLOR option */
         getsicol(&il->Ct);
         break;
case 4:  /* COLSRC option */
         getscsrc(&il->Ct);
         break;
case 5:  /* VREST option */
case 6:  /* REST (compat) option */
         getvrest(&il->Ct, RK_EQCHK);
         break;
case 7:  /* SDCLIST option.  Read new cell list num.  If OFF key was
         *  entered, clear out the ctclloc and global options will take
         *  precedence.  */
         if (getclid(il->ctclloc+CTCL_SIMDAT))
            memset(il->ctclloc+CTCL_SIMDAT, 0, sizeof(clloc));
         break;
case 8:  /* TPCLIST option.  Read new cell list num.  If OFF key was
         *  entered, clear out the ctclloc and detail print cell list
         *  will take precedence.  */
         if (getclid(il->ctclloc+CTCL_NANAT))
            memset(il->ctclloc+CTCL_NANAT, 0, sizeof(clloc));
         break;
case 9:  /* LPCLIST option.  Read new cell list num.  If OFF key was
         *  entered, clear out the ctclloc (and plot all cells) */
         if (getclid(il->ctclloc+CTCL_LPLT))
            memset(il->ctclloc+CTCL_LPLT, 0, sizeof(clloc));
         break;
         }
   /* Save flags for SPIKE and THAMP options, which d3tchk() may
   *  use to set default rspmeth (left byte of ic either endian).  */
   il->ctwk.tree.krmop = *((char *)&ic) &
      (KRM_RFENT|KRM_SPENT|KRM_THENT);

/* Error checking for CELLTYPE card */

   for (jl=ir->play1; jl!=il; jl=jl->play) {
      if (strncmp(il->lname, jl->lname, LSNAME) == 0) {
         dupmsg("LNAME", il->lname, LSNAME);
         break; }
      } /* End jl loop */

   if ((il->Ct.ctopt & (OPTSQ|OPTSH)) == (OPTSQ|OPTSH))
      cryout(RK_E1, "0***INCOMPATIBLE INHIB OPTIONS.", RK_LN2, NULL);

   /* Test for violation of limitation on maximum number of cells.
   *  Note that in 32-bit systems an absolute restriction is set by
   *  the cell list storage scheme, which requires two high-order
   *  bits of a long for encoding ranges and strides.  */
   {  si64 tnelt, wnelt = jmsw(il->nel, ir->ngrp);
#if LSIZE == 8
      tnelt = jrsl(wnelt, SI32_MAX);
#else
      tnelt = jrsl(wnelt, (SI32_MAX>>2));
#endif
      if (qsw(tnelt) > 0)
         cryout(RK_E1, "0***NEL*NGRP EXCEEDS PGM LIMIT.", RK_LN2, NULL);
      il->nelt = swlo(wnelt);
      }

   /* If rspmeth is AEIF or IZHIKEVICH (2003 or 2007), create child
   *  block to hold parameters and move defaults there (may be
   *  overridden by later AEIF or IZHIKEVICH card).  */
   switch (il->Ct.rspmeth) {
   case RF_BREG:
      il->prfi = allocpcv(Static, 1,
         IXstr_BREGDEF, "Bret-Ger parameters");
      *(struct BREGDEF *)il->prfi = RP0->GBGD;
      ((struct BREGDEF *)il->prfi)->iovec = RP->novec++;
      break;
   case RF_IZH3:
      il->prfi = allocpcv(Static, 1,
         IXstr_IZ03DEF, "Izhi2003 parameters");
      *(struct IZ03DEF *)il->prfi = RP0->GIz3D;
      ((struct IZ03DEF *)il->prfi)->Z.iovec = RP->novec++;
      break;
   case RF_IZH7:
      il->prfi = allocpcv(Static, 1,
         IXstr_IZ07DEF, "Izhi2007 parameters");
      *(struct IZ07DEF *)il->prfi = RP0->GIz7D;
      ((struct IZ07DEF *)il->prfi)->Z.iovec = RP->novec++;
      break;
      } /* End rspmeth switch */

   /* Kill all stats for duration of run if RP_NOSTAT is set in
   *  runflags.  For compat, transfer CTPNS to kstat KSTNS and
   *  KRPXR to kstat KSTXR.  */
   if (RP->CP.runflags & RP_NOSTAT) il->Ct.kctp |= CTPNS;
   if (il->Ct.kctp & CTPNS) il->Ct.kstat = KSTNS;
   if (il->Ct.kctp & KRPXR) il->Ct.kstat |= KSTXR;

   /* Set global switch if OPT=R */
   if (il->Ct.ctopt & OPTRG) RP->CP.runflags |= RP_REGENR;

   /* Stash away any restore/rename information */
   initsfhm();    /* Copy defaults to three connection classes */
   if (!(il->Ct.ctopt & OPTOR)) linksfhm(phm, SFHMCELL);

/* Update quantities derived from quantities on CELLTYPE card */

   if (ir->nlyr == UI16_MAX)
      d3exit("layers", LIMITS_ERR, BYTE_MAX);
   ir->nlyr++;
#if LSIZE < 8
   {  si64 tncpg = jasl(jesl(ir->ncpg), (si32)il->nel);
      if (qsw(jrsl(tncpg, SBIG_MAX)) > 0)
         cryout(RK_E1, "0***Cells/group EXCEEDS PGM LIMIT.",
            RK_LN2, NULL);
      }
#endif
   ir->ncpg += (ubig)il->nel;
   } /* End g2cell() */


/*---------------------------------------------------------------------*
*                    Process 'AMPLIFICATION' card                      *
*                                                                      *
*  Synopsis:  void g2amplif(struct CNDFLT *pdd)                        *
*---------------------------------------------------------------------*/

void g2amplif(struct CNDFLT *pdd) {

   ui32 ic;                   /* kwscan field recognition codes */
   int  oldbssel = RK.bssel;
   int  hpcfn,kwret,mrc;
   char rule[SCANLEN1];       /* Because inform has no key input */

   hpcfn = 0;
   rule[0] = '\0';
   if (!(RP->compat & OLDRANGE)) bscomset(RK_BSUNITS);
   inform("(S=A" qDML "B28VIJ,J" KWJ_THR "CR,J" KWJ_THR "XD,"
      "B" qqv(FBvl) "IH,J" KWJ_THR "CR,J" KWJ_THR "XC,B16IJ)",
      rule, &pdd->mdelta, &pdd->mti, &pdd->mtj, &pdd->mtv,
      &pdd->mnsi, &pdd->mnsj, &pdd->upsm, NULL);
   while (kwret = kwscan(&ic,    /* Assignment intended */
         "RULE%A" qDML,          rule,
         "AMPF%T" qLTF,          &hpcfn,
         "WAY%X",
         "PHIFN%X",
         "ADEFER%UH",            &pdd->adefer,
         "DELTA%VB28IJ",         &pdd->mdelta,
         "MTI%J" KWJ_THR "CR",   &pdd->mti,
         "MTJ%J" KWJ_THR "XD",   &pdd->mtj,
         "MTV%B" qqv(FBvl) "IH", &pdd->mtv,
         "MNSI%J" KWJ_THR "CR",  &pdd->mnsi,
         "MNSJ%J" KWJ_THR "XC",  &pdd->mnsj,
         "MXMIJ%VB14IH$?7mV",    &pdd->mxmij,
         "MXSI%J" KWJ_THR "CR",  &pdd->mxsi,
         "QPULL%V<=1B15UH",      &pdd->qpull,
         "RDAMP%V<=1B15UH",      &pdd->rdamp,
         "UPSM%B16IJ",           &pdd->upsm,
         "VT%B" qqv(FBvl) "IH",  &pdd->mtv,
         "VI%VIH",               &pdd->vi,
         NULL))
      switch (kwret) {

case 1:     /* WAY */
         getway(pdd);
         break;
case 2:     /* PHIFN */
         mrc = match(RK_EQCHK, RK_MREQF, ~RK_COMMA, 0, phifnnm[0], 3);
         pdd->phifn = (byte)(mrc > 0 ? mrc - 1 : 0);
         break;

         } /* End while and kwret switch */
   bscompat(oldbssel);

/* If amplif rule entered, apply to conntype defaults and
*  check for consistency errors.  */

   if (rule[0]) {
      mcodes(rule, okcnkam, &pdd->kam);
      ckkam(pdd->kam);
      } /* End if rule entered */

/* Find pcf table in linked list of pcf's */

   if (hpcfn) pdd->pamppcf = findpcf(hpcfn);

   } /* End g2amplif() */


/*---------------------------------------------------------------------*
*                         Process 'AEIF' card                          *
*                                                                      *
*  Synopsis:  void g2breg(struct BREGDEF *pbg)                         *
*---------------------------------------------------------------------*/

void g2breg(struct BREGDEF *pbg) {

   ui32 ic;                   /* kwscan field recognition codes */
   kwscan(&ic,
      "A%B24IJ$nS",           &pbg->bga,
      "B%B20/27IJ$nA",        &pbg->bgb,
      "DELT%V>B20/27IJ$mV",   &pbg->delT,
      "GL%B20UJ$nS",          &pbg->gL,
      "TAUW%V>B20UJ$ms",      &pbg->tauW,
      "VPEAK%B20/27IJ$mV",    &pbg->vPeak,
      "VRESET%B20/27IJ$mV",   &pbg->vReset,
      "VT%B20/27IJ$mV",       &pbg->vT,
      NULL);

   } /* End g2breg() */


/*---------------------------------------------------------------------*
*                      Process 'AUTOSCALE' card                        *
*                                                                      *
*  Synopsis:  void g2autsc(struct AUTSCL *paut)                        *
*---------------------------------------------------------------------*/

void g2autsc(struct AUTSCL *paut) {

   ui32 ic;                   /* kwscan field recognition codes */

   inform("(S=B7/14IH$mV,2*B15UH,IH,V>0<=1B15UH)",
      &paut->astt, &paut->astf1, &paut->astfl, &paut->navg,
      &paut->adamp, NULL);
   kwscan(&ic,
      "ASTF1%B15UH",          &paut->astf1,
      "ASTFL%B15UH",          &paut->astfl,
      "ASMXD%V>0<1B24IJ",     &paut->asmxd,
      "ASMXIMM%V>0B24IJ",     &paut->asmximm,
      "ASMNIMM%V>0<1B24IJ",   &paut->asmnimm,
      "ASTT%B7/14IH$mV",      &paut->astt,
      "ADAMP%V>0<=1B15UH",    &paut->adamp,
      "OPT%KH" okautsc,       &paut->kaut,
      "NAVG%IH",              &paut->navg,
      NULL);
   if (!(paut->kaut & (KAUT_ANY|KAUT_IMM))) cryout(RK_E1,
      "0***AUTOSCALE REQUIRES >=1 OF A,G,I,M,S OPTs.", RK_LN2, NULL);
   if ((paut->kaut & (KAUT_ET|KAUT_WTA)) == (KAUT_ET|KAUT_WTA))
      ermark(RK_EXCLERR);

   } /* End g2autsc() */


/*---------------------------------------------------------------------*
*                        Process 'DECAY' card                          *
*                                                                      *
*  Synopsis:  void g2decay(struct CELLTYPE *il, struct DCYDFLT *pcd,   *
*     struct CNDFLT *pdd)                                              *
*  Note:  If RP0->notmaster is FALSE, this is a MASTER DECAY card and  *
*     SEED is not a valid parameter.                                   *
*---------------------------------------------------------------------*/

void g2decay(struct CELLTYPE *il, struct DCYDFLT *pcd,
      struct CNDFLT *pdd) {

   ui32 ic = 0;               /* kwscan field recognition codes */
   int  oldbssel = RK.bssel;

/* Read positional parameters from DECAY card,
*  pushing back any keywords for processing below.  */

   if (!(RP->compat & OLDRANGE)) bscomset(RK_BSUNITS);
   inform("(S=V<=1B15UH,V<=1B30UJ,V<=1B15UH)",
      &pcd->omega1, &pcd->omega, &pcd->sdamp, NULL);

/* Read keyword parameters.
*  Note:  mxmp does not need the KWJ_THR mechanism because
*  it is applied as an S20 number regardless of input type.  */

   kwscan(&ic, /* Assignment intended */
      "EPSILON%V<=1>=0B30IJ",&pcd->epsilon,
      "OMEGA1%V<=1B15UH",     &pcd->omega1,
      "OMEGA2%V<=1B30UJ",     &pcd->omega,
      "OMEGA%V<=1B30UJ",      &pcd->omega,
      "SIET%B20/27IJ$mV",     &pcd->siet,
      "SISCALE%B24IJ",        &pcd->siscl,
      "SEED%RVIJ", RP0->notmaster, &il->rsd,
      "UPSPSI%B22IJ",         &pcd->upspsi,
      "QDAMP%V<=1B15UH",      &pcd->qdamp,
      "SDAMP%V<=1B15UH",      &pcd->sdamp,
      "DAMPFACT%V<=1B15UH",   &pcd->sdamp,
      "ZETAPSI%V<1B15UH",     &pcd->zetapsi,
      "ADECAY%N" KWS_DECAY,   &pdd->ADCY,
      "DVI%VIH",              &pdd->dvi,
      "QPULL%V<=1B15UH",      &pdd->qpull,
      "RDAMP%V<=1B15UH",      &pdd->rdamp,
      "RHO%VB24IL",           &pdd->rho,
      "MXMP%VB20IJ$?7mV",     &pdd->mxmp,
      "MTICKS%V>IH",          &pdd->mticks,
      "ZETAM%VB16IJ",         &pdd->zetam,
      "KIND%KCEBI",           &pdd->kdecay,
      "GAMMA%V<=1B24UJ",      &pdd->gamma,
      "EQUIL%B16IJ",          &pdd->target,
      NULL);
   bscompat(oldbssel);

/* Error checking for DECAY card */

   if (bitcnt(&pdd->kdecay, 1) > 1)
      cryout(RK_E1, "0***CONFLICTING DECAY CODES.", RK_LN2, NULL);
   } /* End g2decay() */


/*---------------------------------------------------------------------*
*                        Process 'DELAY' card                          *
*                                                                      *
*  Synopsis:  void g2delay(struct CNDFLT *pdd)                         *
*                                                                      *
*  As with 'DECAY', loads RP0->LCnD default params if entered before   *
*  a CONNTYPE card, otherwise loads specific ix->Cn params for most    *
*  recent CONNTYPE only.  User-specified delay distribution is not     *
*  yet implemented.                                                    *
*---------------------------------------------------------------------*/

void g2delay(struct CNDFLT *pdd) {

   si32 tdelay;
   ui32 ic;                   /* kwscan field recognition codes */

/* Options for DELAY card ("DISTRIBUTION" not yet implemented) */
#define NDELAYOPTS 3          /* Number of delay options */
   static char *dlyopt[NDELAYOPTS] = {
      "CONSTANT", "UNIFORM", "NORMAL" };

/* Read data from DELAY card.
*  (Note that switch does nothing if kdelay match fails.)  */

   pdd->kdelay = match(0, RK_MREQF, ~RK_COMMA, 0, dlyopt, 3);

   switch(pdd->kdelay) {

case DLY_CONST:   /* Process CONSTANT delay, drops through ... */
case DLY_UNIF:    /* Process UNIFORM delay  */
      inform("(S,UC)", &pdd->mxcdelay, NULL);
      break;

case DLY_NORM:  /* Process NORMAL delay.
      *  Note nonstandard binary scales for ndev parameters
      *  to permit full range of delays out to max 255 to
      *  be generated.  */
      inform("(S,VB20IJ,VB24IJ)", &pdd->dmn, &pdd->dsg, NULL);
      /* Calculate max delay as mean + 3*sigma.  Allow for
      *  round-off constant added in d3genr.  */
      tdelay = (pdd->dmn + 3*(pdd->dsg>>4) + (S20>>1)) >> 20;
      pdd->mxcdelay =(byte)min(tdelay, 255);

      } /* End kind of delay switch */

   /* Check for seed as keyword parameter */
   kwscan(&ic, "SEED%RVIJ", RP0->notmaster, &pdd->dseed, NULL);
   } /* End g2delay() */


/*---------------------------------------------------------------------*
*                      Process 'DEPRESSION' card                       *
*                                                                      *
*  Synopsis:  void g2dprsn(struct DEPRDFLT *pdpr)                      *
*---------------------------------------------------------------------*/

void g2dprsn(struct DEPRDFLT *pdpr) {

   ui32 ic;                   /* kwscan field recognition codes */

/* Read data from 'DEPRESSION' card */

   inform("(S=VB27/20IL,V>=0<1B30IL)",
      &pdpr->upsd, &pdpr->omegad, NULL);
   kwscan(&ic,
      "UPSD%VB27/20IL",       &pdpr->upsd,
      "OMEGAD%V>=0<1B30IL",   &pdpr->omegad,
      NULL);
   } /* End g2dprsn() */


/*---------------------------------------------------------------------*
*                       Process 'IZHI2003' card                        *
*                                                                      *
*  Synopsis:  void g2iz03(struct IZ03DEF *piz3)                        *
*---------------------------------------------------------------------*/

void g2iz03(struct IZ03DEF *piz3) {

   ui32 ic;                   /* kwscan field recognition codes */

   inform("(S#4=B28IJ,B28IJ,B20/27IJ,B22/29IJ)", &piz3->Z.iza, NULL);
   kwscan(&ic,
      "A%B28IJ",              &piz3->Z.iza,
      "B%B28IJ",              &piz3->Z.izb,
      "C%B20/27IJ$mV",        &piz3->Z.izc,
      "D%B22/29IJ$nA",        &piz3->Z.izd,
      "VPEAK%B20/27IJ$mV",    &piz3->Z.vpeak,
      "VUR%B20/27IJ$mV",      &piz3->Z.vur,
      "CV2%V~B30/23IJ$-/mV",  &piz3->izcv2,
      "CV1%B24IJ",            &piz3->izcv1,
      "CV0%B20/27IJ$mV",      &piz3->izcv0,
      "SEED%RV>IJ", RP0->notmaster, &piz3->Z.izseed,
      "RDA%B14UH",            &piz3->Z.izra,
      "RDB%B14UH",            &piz3->Z.izrb,
      "RDC%B7/14UH",          &piz3->Z.izrc,
      "RDD%B7/14UH",          &piz3->Z.izrd,
      NULL);

   } /* End g2iz03() */


/*---------------------------------------------------------------------*
*               Process 'IZHIKEVICH' or 'IZHI2007' card                *
*                                                                      *
*  Synopsis:  void g2iz07(struct IZ07DEF *piz7)                        *
*---------------------------------------------------------------------*/

void g2iz07(struct IZ07DEF *piz7) {

   ui32 ic;                   /* kwscan field recognition codes */

   inform("(S=B28IJ,B23IJ,B20/27IJ,B19/26IJ)", &piz7->Z.iza,
      &piz7->iizb, &piz7->Z.izc, &piz7->iizd, NULL);
   kwscan(&ic,
      "A%B28IJ",              &piz7->Z.iza,
      "B%B23IJ",              &piz7->iizb,
      "C%B20/27IJ$mV",        &piz7->Z.izc,
      "D%B19/26IJ$nA",        &piz7->iizd,
      "VPEAK%B20/27IJ$mV",    &piz7->Z.vpeak,
      "VUR%B20/27IJ$mV",      &piz7->Z.vur,
      "K%VB24/17IJ",          &piz7->izk,
      "VT%B20/27IJ$mV",       &piz7->izvt,
      "BVLO%B23IJ",           &piz7->bvlo,
      "UMAX%B19/26IJ$nA",     &piz7->umax,
      "UMC%B28IJ",            &piz7->umc,
      "UMVP%B28IJ",           &piz7->umvp,
      "UV3%B30IJ",            &piz7->uv3,
      "UV3LO%B30IJ",          &piz7->uv3lo,
      "SEED%RV>IJ", RP0->notmaster, &piz7->Z.izseed,
      "RDA%B14UH",            &piz7->Z.izra,
      "RDB%B10UH",            &piz7->iizrb,
      "RDC%B7/14UH",          &piz7->Z.izrc,
      "RDD%B7/14UH",          &piz7->iizrd,
      NULL);

   } /* End g2iz07() */


/*---------------------------------------------------------------------*
*                        Process 'NOISE' card                          *
*                                                                      *
*  Synopsis:  void g2noise(struct NDFLT *pn)                           *
*                                                                      *
*  Rev, 04/07/07, GNR - Remove obsolete TYPE=M, use NMOD card          *
*  R78, 07/06/18, GNR - Add noiflgs, moved nsd to NDFLT struct so can  *
*                       make NDFLT the call arg so can call from       *
*                       MASTER NOISE card (but disallow nsd).          *
*---------------------------------------------------------------------*/

void g2noise(struct NDFLT *pn) {

   si64 tnfr = jesl(-1);         /* Temp nfr to hold int or frac */
   orns tnsd = -1;               /* Temp seed */
   ui32 ic = 0;                  /* kwscan field recognition codes */

/* Read data from NOISE card.  Use seed only if positive.
*  Keyword values must always trump positional values.
*  Compatibility features: if nfr > 1, treat as seed.
*  TYPE=S sets nfr <-- nsg, nsg <-- 0.  */

   inform("(S=B20/27IJ$mV,VB24/31IJ$mV,VB24IW,V>IJ)",
      &pn->nmn, &pn->nsg, &tnfr, &tnsd, NULL);
   if (tnfr >= 0) {              /* Check seed only if entered */
      if (qcuw(jesl(S24), tnfr)) {  /* Third arg is seed */
         if (!RP0->notmaster) ermark(RK_NUMBERR);
         else if (tnsd >= 0) ermark(RK_TOOMANY);
         else pn->nsd = swlo(jsrsw(tnfr, FBsc));
         }
      else                       /* Third arg is frac (already */
         pn->nfr = swlo(tnfr);   /* tested >= 0 in inform call) */
      }
   kwscan(&ic,                   /* ic values are tested below */
      "MEAN%B20/27IJ$mV",     &pn->nmn,
      "SIGMA%VB24/31IJ$mV",   &pn->nsg,      /* ic = 2 */
      "FRAC%VB24IJ",          &pn->nfr,      /* ic = 3 */
      "SEED%V>IJ",            &tnsd,
      "TYPE%KC" oknoise,      &pn->noiflgs,  /* ic = 5 */
      NULL);
   /* Seed is always an error with MASTER NOISE */
   if (tnsd >= 0) {
      if (RP0->notmaster) pn->nsd = tnsd;
      else cryout(RK_E1, "0***Seed not allowed on MASTER NOISE.",
         RK_LN2, NULL);
      }
   if (bittst((byte *)&ic, 5)) {
   /* If both 'R' & 'V' were turned off (by 'S'), restore default 'R' */
      if (!(pn->noiflgs &  NFL_GENN)) pn->noiflgs |= NFL_ADDR;
   /* Handle TYPE=S (only if entered on this card and only meaningful
   *  with positional sigma--error if entered keyword SIGMA or FRAC.
   *  If nsg was not entered positionally, default (0) will be used.) */
      if (pn->noiflgs & NFL_SHOT) {
         if (bittst((byte *)&ic, 2) || bittst((byte *)&ic, 3))
            ermark(RK_EXCLERR);
         pn->nfr = (RP->compat & OLDRANGE) ? pn->nsg >> Sr2mV : pn->nsg;
         pn->nsg = 0;
         }
      }
   } /* End g2noise() */


/*---------------------------------------------------------------------*
*          Process 'PAIRED PULSE FACILITATION' or 'PPF' card           *
*                                                                      *
*  Synopsis:  void g2ppf(struct CNDFLT *pdd, struct PPFDATA *ppfd)     *
*                                                                      *
*  Loads PPF default params if entered before a CONNTYPE card,         *
*  otherwise loads ix->PP params for most recent CONNTYPE only.        *
*  The caller sets the arguments according to which situation applies. *
*---------------------------------------------------------------------*/

void g2ppf(struct CNDFLT *pdd, struct PPFDATA *ppfd) {

   ui32 ic;                   /* kwscan field recognition codes */
   const byte kabrupt = PPFABR;
   int  iskip;
   char lea[SCANLEN1];

   /* Need to skip over one word if it is "PPF", otherwise three */
   scanck(lea, RK_REQFLD, ~RK_COMMA);
   if (!ssmatch(lea, "PPF", 3)) for (iskip=0; iskip<2; ++iskip) {
      if (scanck(lea, RK_REQFLD|RK_WDSKIP, ~(RK_COMMA|RK_COLON)) ==
         RK_COLON) break;
      }
   kwscan(&ic,
      "HTUP%V>B1IH",          &ppfd->htup,
      "HTDN%V>B1IH",          &ppfd->htdn,
      "PPFLIM%V>=-1B12IH",    &ppfd->ppflim,
      "PPFT%J" KWJ_THR "XF",  &ppfd->ppft,
      "ABRUPT%OC",            &ppfd->ppfopt, &kabrupt,
      NULL);

   } /* End g2ppf() */


/*---------------------------------------------------------------------*
*                        Process 'PHASE' card                          *
*                                                                      *
*  Synopsis:  void g2phase(struct PHASEDEF *ppd, struct DCYDFLT *pdd)  *
*                                                                      *
*  Rev, 02/11/09, GNR - Reconfigure args to allow MASTER PHASE card    *
*  Rev, 02/28/09, GNR - SIMETH SPIKE is now CONST, SPIKE for compat    *
*---------------------------------------------------------------------*/

void g2phase(struct PHASEDEF *ppd, struct DCYDFLT *pdd) {

   ui32 ic;                   /* kwscan field recognition codes */
   int  hpcfn,kwret,smret;

   static char *fiopt[5] = {
      "CENTROID", "RANDOM", "FIRST", "MODE", "CONST" };
   static char *siopt[6] = {
      "CENTROID", "HEIGHT", "CONST", "SUMPOS", "MEAN", "SPIKE" };

/* Read data from PHASE card.  As of V8D, names "ET" and "SCALE" have
*  been changed to agree with DECAY and CHANGE cards.  */

   hpcfn = 0;
   while (kwret=kwscan(&ic,   /* Assignment intended */
         "PHIMETH%X",
         "SIMETH%X",
         "OPT%KC" okphopt,    &ppd->phops,
         "PHT%B20/27IL$mV",   &ppd->pht,
         "SEED%RVIJ", RP0->notmaster, &ppd->phiseed,
         "SIET%B20/27IJ$mV",  &pdd->siet,
         "SISCALE%B24IJ",     &pdd->siscl,
         "SIPCF%T" qLTF,      &hpcfn,
         NULL))
      switch(kwret) {

case 1:     /* PHIMETH */
         smret=match(RK_EQCHK, RK_MSCAN, ~(RK_COMMA|RK_EQUALS),
            0, fiopt, 5);
         ppd->phimeth = smret;
         if (smret == PHI_CONST) {
            /* phimeth = constant phase */
            eqscan(&ppd->fixpha, "IC", RK_EQCHK);
            ppd->fixpha &= PHASE_MASK;
            }
         break;

case 2:     /* SIMETH  */
         smret=match(RK_EQCHK, RK_MSCAN, ~(RK_COMMA|RK_EQUALS),
            0, siopt, 6);
         if (smret == 6) smret = SI_CONST;   /* Compat */
         ppd->simeth = smret;
         if (smret == SI_CONST)
            eqscan(&ppd->fixsi, "B20/27IL", RK_EQCHK);
         break;

         } /* End kwret switch */

/* Compatibility modifications to ppd->phops */

   if (RP->compat & OLDRANGE)      ppd->phops |= PHOPC;

/* Find PCF in linked list of pcf's */

   if (hpcfn) ppd->pselfpcf = findpcf(hpcfn);
   } /* End g2phase() */


/*---------------------------------------------------------------------*
*                  Process 'REFRACTORY PERIOD' card                    *
*                                                                      *
*  Synopsis:  void g2refrac(struct CELLTYPE *il)                       *
*---------------------------------------------------------------------*/

void g2refrac(struct CELLTYPE *il) {

   struct RFRCDATA *prf = il->prfrc;
   ui32 ic;                   /* kwscan field recognition codes */
   int  kode = 0;

   inform("(SW2=V<512UH,B16IJ,V>=0<1B30IJ,B20/27IJ$mV)",
      &prf->refrac, &prf->upsdst, &prf->omegadst, &prf->psdst, NULL);
   kwscan(&ic,
      "REFRAC%V<512UH",       &prf->refrac,
      "UPSDST%B16IJ",         &prf->upsdst,
      "OMEGADST%V>=0<1B30IJ", &prf->omegadst,
      "PSDST%B20/27IJ$mV",    &prf->psdst,
      "AHP%B20/27IJ$mV",      &prf->ahp,
      "OPT%KIA",              &kode,
      NULL);
   if (kode) il->Ct.ctopt |= OPTAB;
   } /* End g2refrac() */
