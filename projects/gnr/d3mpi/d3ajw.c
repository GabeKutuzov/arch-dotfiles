/* (c) Copyright 1990-2016, The Rockefeller University *21115* */
/* $Id: d3ajw.c 76 2017-12-12 21:17:50Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                               d3ajw.c                                *
*                                                                      *
*     Interpret ARM, EFFECTOR, GRIP, JOINT, UNIVERSAL, and WINDOW      *
*                            control cards                             *
*                                                                      *
*  N.B.  As of V8H, all these cards have common arguments to specify   *
*  motor arbors (formerly arb1, arbw, arbs, etc.).  Rather than use a  *
*  separate control card for these, each card calls g1arb() when one   *
*  of these is encountered.                                            *
*                                                                      *
*  N.B.  The ARM card always creates VCELL blocks for touch and joint  *
*  kinesthesia, and the WINDOW card always creates a VCELL block for   *
*  window kinesthesia.  These get removed later if not referenced.     *
*  This decision was made so that numbered references to VJ,VT,VW      *
*  inputs on CONNTYPE cards are never required to create VCELL blocks  *
*  out of order (because they already exist).  It further provides a   *
*  place to store the deprecated parameters like ahku that can still   *
*  be read from ARM or WINDOW cards.  The downside is that SENSE card  *
*  processing has to be modified to match up with these guys.          *
*                                                                      *
*  N.B.  The VCELL block needs to point to the parent ARMDEF or WDWDEF *
*  block so the virtual sense routines can find the current position   *
*  information that they need.  Main darwin3 will now scan VCELL list  *
*  and switch to call individual routine according to vtype. Ptrs back *
*  from ARMDEFs and WDWDEFs to VCELLs are avoided because those blocks *
*  really belong to env.  Similarly, EFFARB blocks point to the parent *
*  JNTDEF, WDWDEF, or EFFECTOR so d3snsa can store the offset to the   *
*  motor neuron output where the effector can get at it.  Ptrs back to *
*  the EFFARBs are avoided because some parents really belong to env.  *
*                                                                      *
*  N.B.  Tests for validity of nvx,nvy moved to d3schk so we can       *
*  check for errors introduced on either ARM or SENSE cards.           *
************************************************************************
*  Rev, 04/19/90, JWT - Add z coord for joints                         *
*  V8A, 05/15/95, GNR - Broken out from d3grp1.c, create VCELL blocks  *
*                       for VJ,VT,VW                                   *
*  Rev, 02/15/05, GNR - Force all color names to upper case            *
*  V8D, 04/15/07, GNR - Add d3fix1                                     *
*  ==>, 01/06/08, GNR - Last mod before committing to svn repository   *
*  V8H, 12/04/10, GNR - Move code for EFFECTOR card here, add g1arb    *
*  Rev, 01/30/11, GNR - Add wdw_narb and wdw_nvwr                      *
*  Rev, 12/07/12, GNR - Convert ns[xy] to ui16, nst to ui32            *
*  R64, 12/09/15, GNR - Add EFFECTOR 'N' and 'ENTC' options            *
*  Rev, 12/19/15, GNR - Change 'arbs' to 'arbsep[NEXIN]'               *
*  R66, 03/11/16, GNR - Rename nvx->nsxl, nvy->nsya so != ix->nv[xy]   *
*                       Add nta,ntl,tll,tla to ARM, JOINT cards        *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "d3global.h"
#include "jntdef.h"
#include "armdef.h"
#include "wdwdef.h"
#include "rocks.h"
#include "rkxtra.h"
#include "bapkg.h"

/* Persistent data shared with d3g1ajw routines */
extern struct AJWsetup {
   struct ARMDEF **pla;       /* Ptr to end of ARM linked list */
   struct WDWDEF **plw;       /* Ptr to end of WDW linked list */
   struct EFFARB **pplea;     /* Ptr to end of EFFARB linked list */
   struct ARMDEF *pca;        /* Ptr to current ARM block */
   struct WDWDEF *pdw;        /* Ptr to most recent driven window */
   struct EFFARB *pcea;       /* Ptr to current arm's first EFFARB */
   int ijnt;                  /* Current joint number (from 0) */
   } AJW;

/*---------------------------------------------------------------------*
*                        wdw_narb and wdw_nvwr                         *
*                                                                      *
*  These routines encapsulate in one place the specification of the    *
*  numbers of motor arbors (wdw_narb) and numbers of kinesthetic rows  *
*  (wdw_nvwr) supported by a particular window as a function of its    *
*  kchng parameter (function argument).                                *
*                                                                      *
*  The ndegs array contains the number of degrees of freedom (dofs)    *
*  for each kchng (kind of change) in window record, viz:              *
*    kchng = 0 ==> no change                                           *
*    kchng = 1 ==> translate only ==> 2 dofs                           *
*    kchng = 2 ==> translate, rotate ==> 4 dofs                        *
*    kchng = 3 ==> translate, rotate, dilate ==> 5 dofs                *
*    kchng = 4 ==> lock to prev.window ==> 0 dofs                      *
*    kchng = 5 ==> piggyback on prev. xlated window ==> 2 dofs         *
*    kchng = 6 ==> piggy on prev. xlated, rotated window ==> 4 dofs    *
*    kchgn = 7 ==> piggy on prev. xlat, rotat,dilat ==> 5 dofs         *
*---------------------------------------------------------------------*/

static char ndegs[] = {0,2,4,5,0,2,4,5};

int wdw_narb(WINDOW *pw) {

   if (pw->kchng > WDWPTRD) abexit(161);     /* JIC */
   if (pw->kchng < 0) return 0;
   else return (int)ndegs[pw->kchng];

   } /* End wdw_narb() */

int wdw_nvwr(WINDOW *pw) {

   if (pw->kchng > WDWPTRD) abexit(161);     /* JIC */
   if (pw->kchng < 0) return 2;
   else return (int)ndegs[pw->kchng];

   } /* End wdw_nvwr() */


/*---------------------------------------------------------------------*
*                                g1arb                                 *
*                                                                      *
*  Process arguments relating to generic effector arbors.              *
*  This is intended to be a kwjreg routine called as default processor *
*  when an unrecognized argument is encountered on a specific effector *
*  control card.  Errors are signalled via RK.iexit.                   *
*                                                                      *
*  The first argument is a letter followed by a number.  The letter is *
*     'E', 'J', or 'W' according to whether control card is EFFECTOR   *
*     (BBD), JOINT, or WINDOW.  The number is '1' or '3' according to  *
*     whether the call is at Group I or Group III time.                *
*---------------------------------------------------------------------*/

void g1arb(char *type, struct EFFARB *pea) {

   int mret;                  /* match return */
   /* Note:  The more obvious keywords EXCELLS, INCELLS were not used
   *  because broke old input decks where EX, IN were used as abbrevs
   *  for EXCIT, INHIB.  */
#define NEAPS 14              /* Number of effarb parameters */
   static char *eopts[NEAPS] = { "EXCIT", "INHIB", "EFT", "SCALE",
      "EEFT", "ESCALE", "IEFT", "ISCALE", "ECELLS", "ICELLS", "CELLS",
      "SEPARATION", "ESEPARATION", "ISEPARATION" };
   /* BADOPS encodes Group III change rules:
   *     0, always OK to change,
   *     1, cannot change if KEOP_CELLS is set,
   *     2, cannot change if a cell list is being used,
   *     4, cannot change if a cell list is not being used,
   *     8, can never change.  */
   static const char BADOPS[NEAPS+1] =
      { 0, 8, 8, 0, 0, 0, 0, 0, 0, 5, 5, 5, 0, 0, 0 };
   enum EOPS { eoEXCIT=1, eoINHIB, eoEFT, eoSCALE,
      eoEEFT, eoESCALE, eoIEFT, eoISCALE, eoECELLS, eoICELLS, eoCELLS,
      eoSEP, eoESEP, eoISEP } ;        /* smatch() return code */

   /* Push back the last field and then try to match it */
   scanagn();
   mret = match(0, RK_MREQF, ~RK_EQUALS, 0, eopts, NEAPS);

   /* If field not matched, skip to next comma before return */
   if (mret <= 0) goto BAD_ARBOR_OPTION;

   /* Apply tests for illegal Group III options */
   if (type[1] == '3') {
      int ibad = (int)BADOPS[mret];
      if (ibad & 8)
         goto BAD_ARBOR_OPTION;
      if (ibad & 1 && pea->keop & KEOP_CELLS)
         goto BAD_ARBOR_OPTION;
      if (ibad & 2 && (pea->pefil[EXCIT] || pea->pefil[INHIB]))
         goto BAD_ARBOR_OPTION;
      if (ibad & 4 && (!pea->pefil[EXCIT] || !pea->pefil[INHIB]))
         goto BAD_ARBOR_OPTION;
      }

   /* Process one parameter */
   switch ((enum EOPS)mret) {
   case eoEXCIT:
      d3rlnm(pea->exinnm+EXCIT);
      break;
   case eoINHIB:
      d3rlnm(pea->exinnm+INHIB);
      break;
   case eoEFT:
      eqscan(pea->eft, "B7/14IH$mV", 0);
      pea->eft[INHIB] = pea->eft[EXCIT];
      break;
   case eoSCALE:
      eqscan(pea->escli, "F", 0);
      pea->escli[INHIB] = pea->escli[EXCIT];
      break;
   case eoEEFT:
      eqscan(pea->eft+EXCIT, "B7/14IH$mV", 0);
      break;
   case eoESCALE:
      eqscan(pea->escli+EXCIT, "F", 0);
      break;
   case eoIEFT:
      eqscan(pea->eft+INHIB, "B7/14IH$mV", 0);
      break;
   case eoISCALE:
      eqscan(pea->escli+INHIB, "F", 0);
      break;
   case eoECELLS:
      /* Set to save iteration lists on node 0 only */
      ilstsalf(callocv, reallocv, freev);
      RP0->ilseed += 103;  /* Any old seed will do */
      pea->pefil[EXCIT] = ilstread(pea->pefil[EXCIT],
         IL_NEW, 0, RP0->ilseed);
      break;
   case eoICELLS:
      /* Set to save iteration lists on node 0 only */
      ilstsalf(callocv, reallocv, freev);
      RP0->ilseed += 103;  /* Any old seed will do */
      pea->pefil[INHIB] = ilstread(pea->pefil[INHIB],
         IL_NEW, 0, RP0->ilseed);
      break;
   case eoCELLS:
      /* Set to save iteration lists on node 0 only */
      ilstsalf(callocv, reallocv, freev);
      RP0->ilseed += 103;  /* Any old seed will do */
      /* (Careful to avoid a mem leak if both replaced) */
      if (pea->pefil[EXCIT] == pea->pefil[INHIB])
         pea->pefil[EXCIT] = pea->pefil[INHIB] =
            ilstread(pea->pefil[EXCIT], IL_NEW, 0,
            RP0->ilseed);
      else {
         pea->pefil[EXCIT] = ilstread(pea->pefil[EXCIT],
            IL_NEW, 0, RP0->ilseed);
         pea->pefil[INHIB] = ilstread(pea->pefil[INHIB],
            IL_NEW, 0, RP0->ilseed);
         }
      break;
   case eoSEP:
      eqscan(&pea->arbsep, "VIJ", 0);
      pea->arbsep[INHIB] = pea->arbsep[EXCIT];
      break;
   case eoESEP:
      eqscan(&pea->arbsep+EXCIT, "VIJ", 0);
      break;
   case eoISEP:
      eqscan(&pea->arbsep+INHIB, "VIJ", 0);
      break;
      } /* End kmat switch */

   return;

/* If bad option found, skip to next comma before return */

BAD_ARBOR_OPTION:
   ermark(RK_IDERR);
   while (RK.scancode & ~(RK_COMMA|RK_ENDCARD)) scan(NULL, 0);
   pea->nchgs = 0;
   return;

   } /* End g1arb() */


/*---------------------------------------------------------------------*
*                                g1arm                                 *
*                                                                      *
*                          Process ARM card                            *
*---------------------------------------------------------------------*/

void g1arm(void) {

#define ATTNUM 3              /* Number of arm attachment types */
   static char *atts[] = { "FRAME", "WINDOW", "UNIVERSAL" } ;

   struct ARMDEF *pa;         /* Ptr to current arm   */
   struct JNTDEF *pj;         /* Ptr to current joint */
   struct VCELL *pvj,*pvt;    /* Ptrs to kinesthesia, touch blocks */
   struct EFFARB tefa,*pea;   /* Temp for storing arbor defaults */
   ui32 ic;                   /* kwscan field recognition codes */
   int jjm,nj;
   int kwret;                 /* kwscan return val */

   /* Make new ARMDEF block and insert at end of list */
   pa = *AJW.pla = (struct ARMDEF *)
      mallocv(sizeof(struct ARMDEF), "arm");
   *(AJW.pla = &pa->pnxarm) = NULL;

   /* Increment arm count */
   if (RP->narms >= BYTE_MAX)
      d3exit("arm", LIMITS_ERR, BYTE_MAX);
   pa->armno = ++RP->narms;

   /* Locate (or create and initialize) VCELL blocks for
   *  VJ and VT senses on this arm */
   pvj = findvc("VJ", VJ_SRC, (int)RP->narms,
      " FOR JOINT KINESTHESIA ON CURRENT ARM");
   pvj->pvcsrc = pa;
   pvt = findvc("VT", VT_SRC, (int)RP->narms,
      " FOR TOUCH ON CURRENT ARM");
   pvt->pvcsrc = pa;

   /* Set default arm values */
   armdflt(pa);

   /* Note that defaults set by effdflt may be overridden
   *  by generic ARM defaults entered on this card.  */
   memset((char *)&tefa, 0, sizeof(struct EFFARB));
   effdflt(&tefa);

   /* Read ARM options */
   cdscan(RK.last, 1, SCANLEN, RK_WDSKIP);
   while (kwret = kwscan(&ic, /* Assignment intended */
         "ATTACH%X",
         "JOINTS%VIC",           &pa->njnts,
         "AOP%KHTBIGECPUW",      &pa->jatt,
         "POSITION%V>=0<=12IC",  &pa->jpos,
         "COLOR%P2VA" qCMC,      pa->acolr,
         "NKU%VIH",              &pvj->nsxl,
         "HWIDTH%VF",            &pvj->vhw,
         "NTA%VIH",              &pa->nta,
         "NTL%VIH",              &pa->ntl,
         "MNEXT%F",              &pa->jmn0,
         "MXEXT%F",              &pa->jmx0,
         "SCLA%P3F$-/mV",        &pa->scla0,
         "TLA%V>B" qqv(FBxy) "IJ", &pa->tla,
         "TLL%V>B" qqv(FBxy) "IJ", &pa->tll,
         "%%J" KWJ_GARB "J1",    &tefa,
         NULL)) {
      switch (kwret) {
         case 1:  /* ATTACH */
            jjm = match(RK_EQCHK, RK_MREQF, ~RK_COMMA, 0,
               atts, ATTNUM) - 1;
            pa->jatt |= (unsigned short)max(0,jjm);
            break;
         } /* End kwret switch */
      } /* End kwret while */

   /* Allocate joint info and arbor def arrays and set defaults.
   *  (nje = number joints effective, counting universal
   *           joint and grip blocks)
   *  See design notes (p. 30) for dynamic param alloc */
   nj = pa->njnts;
   if (!nj) {
      cryout(RK_E1, "0***ARM HAS NO JOINTS", RK_LN2, NULL);
      AJW.pcea = NULL;
      }
   else if (nj > MXNJNT) {
      /* This condition is so we can store joint-used bits in
      *  vcdef at jntinVT, a ui16  */
      cryout(RK_E1, "0***ARM HAS TOO MANY (>15) JOINTS", RK_LN2, NULL);
      AJW.pcea = NULL;
      }
   else {
      int j;
      int nje = ((pa->jatt&ARMAU) ?  nj+1 :  nj);
      int njg = ((pa->jatt&ARMGE) ? nje+1 : nje);
      pj = pa->pjnt = pvt->pjnt0 = (struct JNTDEF *)
         callocv(njg, sizeof(struct JNTDEF), "joints");
      /* Gripper uses value, not EFFARB to activate */
      AJW.pcea = pea = (struct EFFARB *)
         callocv(nje, sizeof(struct EFFARB), "joint arbors");

      for (j=1; j<=nje; pj++,pea++,j++) {

         /* Link current EFFARB into list */
         *AJW.pplea = pea;
         AJW.pplea = &pea->pneb;

         *pea = tefa;    /* Copy arbor defaults */
         pea->peff = pj;
         pea->heffnm = (txtid)pa->armno;
         pj->scla = pa->scla0;

         if (j>nj)  /* Initialize UNIVERSAL JOINT */  {
            jntdflt(pa, pj, (int)Universal);
            pea->nchgs        = 2;
            pea->keff         = KEF_UJ;
            } /* End UNIVERSAL JOINT */

         else /* Initialize REGULAR JOINT */ {
            jntdflt(pa, pj, (int)Normal);
            pa->jl00 = max(pa->jl00-1.0,4.0);
            pea->nchgs        = 1;
            pea->keff         = KEF_ARM;
            pea->heffnm |= j << BITSPERBYTE;
            } /* End REGULAR JOINT */

         } /* End joint loop */

      if (pa->jatt&ARMGE) {   /* Initialize GRIP JOINT */
         jntdflt(pa, pj, (int)Gripper);
         } /* End GRIP JOINT */

      } /* End nj else */

   /* Save items needed by following JOINT cards */
   AJW.pca  = pa;
   AJW.ijnt = 0;

   RP->CP.runflags &= ~RP_NOINPT;
   } /* End g1arm() */


/*---------------------------------------------------------------------*
*                               g1grip                                 *
*                                                                      *
*                          Process GRIP card                           *
*---------------------------------------------------------------------*/

void g1grip(void) {

   struct ARMDEF *pa;         /* Ptr to current arm */
   struct JNTDEF *pgj;        /* Ptr to grip joint */
   ui32 ic;                   /* kwscan field recognition codes */

   /* Pick up location of current arm if one exists */
   if (RP->narms <= 0) {
      cryout(RK_E1, "0***GRIP REQUIRES ARM CARD", RK_LN2, NULL);
      skip2end();
      return; }
   pa = AJW.pca;

   /* Check that current arm is set for grip joint */
   if (!(pa->jatt & ARMGE)) {
      cryout(RK_E1, "0***GRIP REQUIRES ARM AOP=G", RK_LN2, NULL);
      skip2end();
      return; }

   /* Locate GRIP joint block */
   pgj = pa->pjnt + pa->njnts + ((pa->jatt & ARMAU) != 0);

   /* Read grip parameters */
   inform("(SW1=VIJ,B8IJ)",&pgj->u.grip.grvi,
      &pgj->u.grip.grvt,NULL);
   kwscan(&ic,
      "VI%VIJ", &pgj->u.grip.grvi,
      "VT%B8IJ", &pgj->u.grip.grvt,
      "GTIME%VIL", &pgj->u.grip.gtime,
      "GWIDTH%VF", &pgj->u.grip.gwidth,
      "GSPEED%F", &pgj->u.grip.gspeed,
      NULL);
   } /* End g1grip() */


/*---------------------------------------------------------------------*
*                                g1jnt                                 *
*                                                                      *
*                         Process JOINT card                           *
*---------------------------------------------------------------------*/

void g1jnt(void) {

   struct ARMDEF *pa;         /* Ptr to current arm */
   struct JNTDEF *pj;         /* Ptr to current joint */
   struct EFFARB *pea;        /* Ptr to current effector arbor */
   ui32 ic;                   /* kwscan field recognition codes */
   si32 tjl;                  /* Joint length as FBxy value */

   /* Pick up location of current arm if one exists */
   if (RP->narms <= 0) {
      cryout(RK_E1, "0***JOINT REQUIRES ARM CARD", RK_LN2, NULL);
      skip2end();
      return; }
   pa = AJW.pca;

   /* Pick up location of current joint if not out-of-range */
   if (AJW.ijnt >= pa->njnts) {
      if (AJW.ijnt == pa->njnts) /* Just one message */
         cryout(RK_E1, "0***TOO MANY JOINTS", RK_LN2, NULL);
      skip2end();
      return; }
   pj = pa->pjnt + AJW.ijnt;
   pea = AJW.pcea + AJW.ijnt;

   /* Read joint parameters */
   inform("(SW1=2*VF)", &pj->jl, &pj->u.jnt.ja, NULL);
   kwscan(&ic,
         "MNEXT%F",              &pj->u.jnt.jmn,
         "MXEXT%F",              &pj->u.jnt.jmx,
         "NTA%VIH",              &pj->u.jnt.nta,
         "NTL%VIH",              &pj->u.jnt.ntl,
         "TLA%V>B" qqv(FBxy) "IJ", &pj->u.jnt.tla,
         "TLL%V>B" qqv(FBxy) "IJ", &pj->u.jnt.tll,
         "TZ%IJ",                &pj->u.jnt.tzi,
         "SCLA%P3F$-/mV",        &pj->scla,
         "%%J" KWJ_GARB "J1", pea,
         NULL);

   /* If tla not input here or on ARM card, set it from jl */
   tjl = (si32)(dSxy*pj->jl);
   if (pj->u.jnt.tla < 0) pj->u.jnt.tla = tjl;
   /* Otherwise, check that touch length not > overall length */
   else if (pj->u.jnt.tla > tjl)
      cryout(RK_E1, "0***INVALID TOUCH LENGTH", RK_LN2, NULL);

   /* If any of tla,tll,nta,ntl is zero, set tla to zero as flag */
   if (!pj->u.jnt.tll || !pj->u.jnt.nta || !pj->u.jnt.ntl)
      pj->u.jnt.tla = 0;

   /* Increment to next joint block */
   AJW.ijnt++;
   } /* End g1jnt() */


/*---------------------------------------------------------------------*
*                               g1ujnt                                 *
*                                                                      *
*                    Process UNIVERSAL JOINT card                      *
*---------------------------------------------------------------------*/

void g1ujnt(void) {

   struct ARMDEF *pa;         /* Ptr to current arm */
   struct JNTDEF *puj;        /* Ptr to universal joint */
   struct EFFARB *pea;        /* Ptr to current effector arbor */
   ui32 ic;                   /* kwscan field recognition codes */

   /* Pick up location of current arm if one exists */
   if (RP->narms <= 0) {
      cryout(RK_E1, "0***UJOINT REQUIRES ARM CARD", RK_LN2, NULL);
      skip2end();
      return; }
   pa = AJW.pca;

   /* Check that current arm is set for universal joint   */
   if (!(pa->jatt&ARMAU)) {
      cryout(RK_E1, "0***UJOINT REQUIRES ARM AOP=U", RK_LN2, NULL);
      skip2end();
      return; }

   /* Universal joint follows regular joints */
   puj = pa->pjnt + pa->njnts;
   pea = AJW.pcea + pa->njnts;

   /* Read universal joint parameters */
   cdscan(RK.last, 2, SCANLEN, RK_WDSKIP);
   kwscan(&ic,
         "SCLA%F$-/mV",       &puj->scla,
         "SCLX%F$-/mV",       &puj->u.ujnt.ujsclx,
         "VI%VIJ",            &puj->u.ujnt.ujvi,
         "VT%B8IJ",           &puj->u.ujnt.ujvt,
         "CTIME%P2VIL",       &puj->u.ujnt.ctime,
         "%%J" KWJ_GARB "J1",  pea,
         NULL);
   } /* End g1ujnt() */


/*---------------------------------------------------------------------*
*                                g1wdw                                 *
*                                                                      *
*                         Process WINDOW card                          *
*---------------------------------------------------------------------*/

void g1wdw(void) {

   struct WDWDEF *pw;         /* Ptr to current window */
   struct EFFARB *pea;        /* Ptr to arbor information */
   struct VCELL *pvw;         /* Ptr to kinesthesia block */
   ui32 ic;                   /* kwscan field recognition codes */
   short lkchng;              /* Local copy of pw->kchng */

   /* Make new WDWDEF block and insert at end of list */
   pw = *AJW.plw = (struct WDWDEF *)allocpcv(
      Host, 1, sizeof(struct WDWDEF), "window");
   AJW.plw = &pw->pnxwdw;

   /* Increment window count */
   if (RP->nwdws >= BYTE_MAX)
      d3exit("window", LIMITS_ERR, BYTE_MAX);
   pw->wdwno = RP->nwdws++;

   /* Locate (or create and initialize) VCELL block
   *  for possible window kinesthesia */
   pvw = findvc("VW", VW_SRC, (int)RP->nwdws,
      " FOR KINESTHESIA ON CURRENT WINDOW");
   pvw->pvcsrc = pw;

   /* Allocate and initialize effector arbor block */
   pea = *AJW.pplea = (struct EFFARB *)allocpcv(
      Host, 1, sizeof(struct EFFARB), "Effector arbor");
   AJW.pplea = &pea->pneb;
   effdflt(pea);
   pea->peff = pw;
   pea->keff = KEF_WDW;
   pea->heffnm = (txtid)pw->wdwno + 1;

   /* Set default window values */
   wdwdflt(pw);

   /* Read window options */
   inform("(SW1=+4*VF)",
      &pw->wx, &pw->wy, &pw->wwd, &pw->wht, NULL);
   kwscan(&ic,
      "CHANGE%P2V<8IH",    &pw->kchng,
      "NKU%VIH",           &pvw->nsxl,
      "HWIDTH%VF",         &pvw->vhw,
      "COLOR%P2VA" qCMC,   pw->wcolr,
      "SCLX%F$-/mV",       &pw->sclx,
      "SCLR%F",            &pw->sclr,
      "WOP%KCB",           &pw->flags,
      "EOPT%P2KCF",        &pea->keop,
      "%%J" KWJ_GARB "W1", pea,
      NULL);

   lkchng = pw->kchng;
   pea->nchgs = wdw_narb(pw);

   /* Link locked window to most recent driven window */
   if (lkchng != WDWLCK) AJW.pdw = pw;
   else if (AJW.pdw)     pw->pdrive = AJW.pdw;

   /* Error check */
   if ((lkchng == WDWLCK || lkchng == WDWPIG) && !pw->wdwno)
      cryout(RK_E1, "0***NO PREV WINDOW TO LOCK/PIGGYBACK TO",
         RK_LN2, NULL);
   RP->CP.runflags &= ~RP_NOINPT;
   } /* End g1wdw() */


#ifdef BBDS
/*---------------------------------------------------------------------*
*                                g1eff                                 *
*                                                                      *
*                        Process EFFECTOR card                         *
*---------------------------------------------------------------------*/

void g1eff(void) {

   struct EFFARB *pea;     /* Ptr to EFFECTOR block */
   ui32   ic;              /* For kwscan() */
   txtid  heffh,hbbdnm;    /* Handles to host, device names */
   static char *pBBD = "BBD";

   /* At present, the only protocol allowed is BBD.
   *  Add others here as needed with switch to read rest.  */
   cdscan(RK.last, 1, SCANLEN, RK_WDSKIP);
   if (match(0, RK_MREQF, ~(RK_BLANK|RK_COMMA), 0, &pBBD, 1) != 1)
      goto BAD_EFFECTOR_PARAMETER;

   /* Read effector name and be sure it is unique */
   g1bbdn(&heffh, &hbbdnm);
   for (pea=RP0->pearb1; pea; pea=pea->pneb) {
      if (pea->heffnm == hbbdnm) {
         dupmsg("BBD DEVICE", NULL, hbbdnm);
         goto BAD_EFFECTOR_PARAMETER;
         }
      }

   /* Allocate and initialize effector arbor block */
   pea = *AJW.pplea = (struct EFFARB *)allocpcv(
      Host, 1, sizeof(struct EFFARB), "Effector arbor");
   AJW.pplea = &pea->pneb;
   effdflt(pea);
   pea->keff = KEF_BBD;
   pea->heffnm = hbbdnm;

   /* Use g1arb to handle arbor parameters -- Any parameters
   *  unique to EFFECTOR can be added here as needed */
   ic = 0;
   kwscan(&ic,
      "NCHGS%V>I",         &pea->nchgs,
      "ENTC%VUJ",          &pea->entc,
      "EOPT%P2KCNCF",      &pea->keop,
      "%%J" KWJ_GARB "E1", pea, NULL);

   /* Register with the BBD package.  Data pointer and size are
   *  set in d3schk when iteration list counts can be known.  */
   pea->peff = bbdsrege(getrktxt(hbbdnm),
      heffh ? getrktxt(heffh) : NULL, 0);
   /* Set a flag to simplify EVTRIAL controls */
   RP->kevtb |= EVTEFF;
   return;

BAD_EFFECTOR_PARAMETER:
   ermark(RK_IDERR);
   skip2end();
   return;

   } /* End g1eff() */
#endif


/*---------------------------------------------------------------------*
*                               d3fix1                                 *
*                                                                      *
*  Perform minor cleanups after d3grp1 formerly in the main program.   *
*  These include:                                                      *
*  (1) Calculate constants relating to the input array.                *
*  (2) Make indexes so arms and windows can be located quickly.        *
*---------------------------------------------------------------------*/

void d3fix1(void) {

   float ratio;

   RP->ky1 = RP->ky + 1;
   RP->nsx = 1<<RP->kx;
   RP->nsy = 1<<RP->ky;
   RP->xmask = (RP->nsx - 1) << RP->ky1;
   RP->ymask = RP->nsy - 1;
   RP->xymask = RP->xmask | RP->ymask;
   RP->xymaskc = ~RP->xymask;
   ratio = ((float)RP->nsy)/(float)RP->nsx;
   if (RP->nsx >= RP->nsy) {
      RP0->efplw = 10.0;
      RP0->efplh = 10.0*ratio;
      }
   else {
      RP0->efplh = 10.0;
      RP0->efplw = 10.0/ratio;
      }
   if (RP0->kux == 0) RP0->kux = min(10,RP->nsx);
   if (RP0->kuy == 0) RP0->kuy = min(12,RP->nsy);

   /* Make an arm index */
   if (RP->narms > 0) {
      struct ARMDEF *pa,**ppa = RP0->pparm = (struct ARMDEF **)
         mallocv(RP->narms*sizeof(struct ARMDEF *), "arm index");
      for (pa=RP0->parm1; pa; pa=pa->pnxarm)
         *ppa++ = pa;
      }

   /* Make a window index */
   if (RP->nwdws > 0) {
      struct WDWDEF *pw,**ppw = RP0->ppwdw = (struct WDWDEF **)
         mallocv(RP->nwdws*sizeof(struct WDWDEF *), "window index");
      for (pw=RP0->pwdw1; pw; pw=pw->pnxwdw)
         *ppw++ = pw;
      }

   } /* End d3fix1() */

