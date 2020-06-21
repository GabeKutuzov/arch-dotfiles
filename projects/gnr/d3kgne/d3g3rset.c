/* (c) Copyright 2007-2011, Neurosciences Research Foundation, Inc. */
/* $Id: d3g3rset.c 43 2011-02-24 04:00:37Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                             d3g3rset.c                               *
*                                                                      *
*  This file contains the d3g3rset function, which is called by        *
*  d3grp3() to interpret the options on the Group III RESET card.      *
************************************************************************
*  V8D, 03/15/07, GNR - Broken out from d3grp3(), add EVENT options    *
*  Rev, 11/22/07, GNR - Add BBD events, NEWSERIES, NEWFULL             *
*  ==>, 01/06/08, GNR - Last mod before committing to svn repository   *
*  Rev, 05/17/08, GNR - Add LEVEL(VALUE)=ALLSERIES                     *
*  V8F, 02/20/10, GNR - Remove PLATFORM reset                          *
*  V8H, 05/25/11, GNR - Major rewrite, simpler LEVEL, new ACTION codes *
***********************************************************************/

#define NEWARB          /* Use new EFFARB mechanism */
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "d3global.h"
#include "d3opkeys.h"
#include "armdef.h"
#include "jntdef.h"
#include "wdwdef.h"
#ifdef BBDS
#include "bbd.h"
#endif
#include "rocks.h"
#include "rkxtra.h"
#include "bapkg.h"

/*=====================================================================*
*                              rsetlvls                                *
*                                                                      *
*  This little local routine analyzes a possible list of reset types   *
*  in parentheses for the LEVEL and ACTION keywords.  The allowed      *
*  punctuation is RK_INPARENS plus either just a comma or right        *
*  parens plus equals sign.                                            *
*=====================================================================*/

static int rsetlvls(void) {

   /* NRLACT = 6 = Number of enterable reset types = Number of LEVEL
   *  and ACTION suboptions.  The following keys must match the order
   *  of relevant items in the ReLops enum defined in d3global.h  */
   static char *rlvop[NRLACT] =
      { "TRIAL", "TIMER", "EVENT", "VALUE", "SERIES", "NOW" };

   int smret, lvkeys = 0;
   do {
      if ((smret = match(0, RK_MREQF, ~(RK_COMMA|RK_RTPAREN|
            RK_EQUALS), RK_INPARENS, rlvop, NRLACT-1)) > 0)
         lvkeys |= 1 << smret;
      } while (!(RK.scancode & RK_RTPAREN));
   if (RK.scancode != (RK_INPARENS|RK_RTPAREN|RK_EQUALS))
      ermark(RK_PUNCERR);
   return lvkeys;
   } /* End rsetlvls() */


/*=====================================================================*
*                              d3g3rset                                *
*=====================================================================*/

void d3g3rset(void) {

   int  smret;
   char lea[SCANLEN1];

#define RSTNUM 9     /* Number of RESET options */
   static char *resetop[RSTNUM] = { "LEVEL", "ACTION", "ARM",
      "WINDOW", "ITI", "EVERY", "TIMER", "EVENT", "NOW" };
   /* Give names to the option cases (same order as resetop): */
   enum erops { opbad = 0, oplev, opact, oparm,
      opwin, opiti, opevry, optimr, opevnt, opnow };

#ifdef BBDS
#define REVTNUM 5    /* Number of reset event types */
   static char *revtyp[REVTNUM] = { "VALUE", "ENVIRONMENT",
      "NOENVIRONMENT", "BBD", "NOBBD" };
   /* Give names to the reset event types (same order as revtyp): */
   enum erevt { rtval = 1, rtenv, rtnoen, rtbbd, rtnobbd };
#else
#define REVTNUM 3    /* Number of reset event types */
   static char *revtyp[REVTNUM] = { "VALUE", "ENVIRONMENT",
      "NOENVIRONMENT" };
   /* Give names to the reset event types (same order as revtyp): */
   enum erevt { rtval = 1, rtenv, rtnoen };
#endif

#define RVALTP 3     /* Number of value reset tests */
   static char *rvaltyp[RVALTP] = { "NONE", "LE", "GE" };
   /* Give names to the value operations (same order as rvaltyp): */
   enum evalt { rvnone = 1, rvle, rvge };

   cdscan(RK.last, 1, SCANLEN, RK_WDSKIP);
   while ((smret = match(OFF, RK_MNEWK, ~(RK_COMMA|RK_EQUALS|
         RK_LFTPAREN), 0, resetop, RSTNUM)) != RK_MENDC)
      switch ((enum erops)smret) {

      case opbad:                   /* UNRECOGNIZED */
         break;

      case oplev: {                 /* LEVEL */
         /* V8H, 06/11/11, GNR - Now only numbers 0,1,2,3,4 are
         *  recognized as reset level for each ReLops type  */
         int lvndx, lvkeys = rsetlvls();
         if (lvkeys) {
            int nlev;
            eqscan(&nlev, "V<=" qKRLMX "I", 0);
            for (lvndx=0; lvkeys; ++lvndx,lvkeys>>=1) if (lvkeys & 1) {
               RP->kra[lvndx].rlev = (byte)nlev;
               RP->kra[lvndx].rlent = TRUE;
               }
            }
         } /* End LEVEL local scope */
         break;

      case opact: {                 /* ACTION */
         int lvndx, lvkeys = rsetlvls();
         if (lvkeys) {
            scanck(lea, RK_REQFLD, ~(RK_COMMA|RK_ENDCARD));
            mcodes(lea, okkract, NULL);
            for (lvndx=0; lvkeys; ++lvndx,lvkeys>>=1) if (lvkeys & 1) {
               mcodoph(&RP->kra[lvndx].kract);
               RP->kra[lvndx].rlent = TRUE;
               }
            }
         } /* End ACTION local scope */
         break;

      case oparm: {                 /* ARM */
         struct ARMDEF *pa;
         int ia,nj,lj;
         inform("(SV(V>IV))", &ia, NULL); /* Parens optional */
         if (ia > (int)RP->narms) {
            ermark(RK_MARKFLD);
            cryout(RK_P1, "0***REF TO NONEXISTENT ARM",
               RK_LN2, NULL); }
         else {
            /* Find the specified arm */
            pa = RP0->pparm[ia - 1];
            /* Signal d3resetz to override bypass next time */
            pa->jatt |= ARMOR;
            /* Joint values are optional */
            if (RK.scancode & RK_EQUALS) {
               nj = (int)pa->njnts;
               lj = sizeof(struct JNTDEF);
               inform("(SV(R^FV))", &nj, &lj,
                  &pa->pjnt->u.jnt.ja0, NULL);
               }
            }
         } /* End ARM local scope */
         break;

      case opwin: {                 /* WINDOW */
         struct WDWDEF *pw;
         int iw;
         inform("(SV(V>IV))", &iw, NULL); /* Parens optional */
         if (iw > (int)RP->nwdws) {
            ermark(RK_MARKFLD);
            cryout(RK_P1, "0***REF TO NONEXISTENT WINDOW",
               RK_LN2, NULL); }
         else {
            /* Find the specified window */
            pw = RP0->ppwdw[iw - 1];
            /* Signal d3resetz to override bypass next time */
            pw->flags |= WBWOPOR;
            /* Window parameters are optional */
            if (RK.scancode & RK_EQUALS) {
               inform("(SV(FFNV>FV>FV))", &pw->wx0, &pw->wy0,
                  &pw->wwd0, &pw->wht0, NULL);
               }
            }
         } /* End WINDOW local scope */
         break;

      case opiti:                   /* ITI */
         eqscan(&RP->iti, "V>IH", RK_EQCHK);
         break;

      case opevry:                  /* EVERY */
         inform("(SVIL)", &RP0->ntrsi, NULL);
         break;

      case optimr:                  /* TIMER */
         RP->jrsettm = gettimno(NULL, YES);
         break;

      case opevnt: {                /* EVENT */
         /* Bad punctuation here is probably hopeless */
         if (RK.scancode != RK_EQUALS) {
            ermark(RK_PUNCERR);
            skip2end(); break;
            }
         /* A do loop here allows single options to be entered
         *  without the parens.  */
         do {
            switch ((enum erevt)match(0, RK_MSCAN, ~(RK_COMMA|
               RK_INPARENS|RK_RTPAREN), 0, revtyp, REVTNUM)) {
            /* A mismatch here (smatch returns 0) matches no case
            *  and so the while just repeats... */
            case rtval: {              /* VALUE */
               si32 evtrv = 0;         /* Trigger value */
               int  ivn;               /* Value block number */
               int  iop;               /* Operation */
               struct VALEVENT **ppvev,*pvev;
               inform("(S,V(V>IV))", &ivn, NULL);
               /* Pick up the operation and value.  Note:  If iop is
               *  rvnone and there is a value, leave it unscanned and
               *  it will give an error on the next top-level scan.  */
               iop = match(0, RK_MREQF, ~(RK_COMMA|RK_RTPAREN),
                  RK_INPARENS, rvaltyp, RVALTP);
               if ((enum evalt)iop >= rvle) {
                  scanck(lea, RK_REQFLD,
                     ~(RK_COMMA|RK_INPARENS|RK_RTPAREN));
                  wbcdin(lea, &evtrv, (FBsv<<RK_BS)|RK_IORF|RK_CTST|
                     RK_NI32+SCANLEN-1);
                  }
               /* See whether there is already a test on this value */
               for (ppvev=&RP0->pvev1; pvev=*ppvev; ppvev=&pvev->pnve) {
                  if (ivn == pvev->ivdt) goto ModifyValEvnt;
                  }
               /* Nope.  Make a new one unless op is "NONE" */
               if ((enum evalt)iop <= rvnone)
                  cryout(RK_E1, "0***THERE IS NO VALUE TEST WITH "
                     "THIS INDEX TO TURN OFF", RK_LN2, NULL);
               else {
                  pvev = *ppvev = callocv(1, sizeof(struct VALEVENT),
                     "Value event");
                  pvev->evtrig = evtrv;
                  pvev->ivdt = (short)ivn;
                  pvev->veops = (enum evalt)iop - rvle;
                  }
               break;
ModifyValEvnt: if ((enum evalt)iop == rvnone) {
                  /* Delete existing value event */
                  *ppvev = pvev->pnve;
                  freev(pvev, "Value event"); }
               else {
                  pvev->evtrig = evtrv;
                  pvev->veops = (enum evalt)iop - rvle;
                  }
               } /* End rtval local scope */
               break;
            case rtenv:                /* ENVIRONMENT */
               RP0->krenvev = ENV_RESET;
               break;
            case rtnoen:               /* NOENVIRONMENT */
               RP0->krenvev = 0;
               break;
#ifdef BBDS
            case rtbbd:                /* BBD */
               RP0->needevnt |= NEV_RSET;
               break;
            case rtnobbd:              /* NOBBD */
               RP0->needevnt &= ~NEV_RSET;
               break;
#endif
               } /* End event type switch */
            } while ((RK.scancode & (RK_INPARENS|RK_RTPAREN)) ==
               RK_INPARENS);
         break;

      case opnow:                   /* NOW */
         RP->kra[RLnow].rlent = TRUE;
         break;

      } /* End smret while and switch */

   /* End of scan.  Set RP_TMRSET bit according to whether timed
   *  resets have been specified, but not if this is a replay.  */
   if ((RP0->ntrsi > 0 || RP->jrsettm != TRTN0) &&
         !(RP->CP.runflags & RP_REPLAY))
      RP->CP.runflags |= RP_TMRSET;
   else
      RP->CP.runflags &= ~RP_TMRSET;

   } /* End d3g3rset() */

