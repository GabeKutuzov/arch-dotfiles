/* (c) Copyright 2001-2008, The Rockefeller University *11116* */
/* $Id: d3g3timr.c 70 2017-01-16 19:27:55Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                              d3g3timr                                *
*           Interpret Group 3 PRINT, PLOT, and TIMER cards             *
*                                                                      *
*  The syntax of the control card interpreted by this routine is       *
*     {PRINT|PLOT|TIMER let} [DETAIL], [(stimulus list)],              *
*        [MAXCYC=mxc], [{CYCLES|TRIALS}=(trial list)]                  *
*                                                                      *
*  After the card has been read and indentified, call as               *
*     d3g3timr(wmdl, plet), where                                      *
*        wmdl = bit mask of modalities to which stim list refers       *
*        plet = ptr to character string containing timer ID letter     *
*                                                                      *
*  Note that compatibility with old PRINT, PLOT cards is partial:      *
*  The stimulus list must now be enclosed in parens if other than      *
*  "ALL" or "NONE", the MAXCYC option is ignored if not in a list,     *
*  and "FIRST", "LAST", "OF EVERY" must be in the trial list parens.   *
************************************************************************
*  V8B, 01/20/01, GNR - New routine based on obsolete d3g3pp           *
*  V8D, 12/16/07, GNR - Add gettimnw wrapper for kwsreg registration   *
*  ==>, 01/06/08, GNR - Last mod before committing to svn repository   *
*  Rev, 03/27/08, GNR - Add EVTRIAL, EVRELTN options                   *
***********************************************************************/

#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "d3global.h"
#include "rocks.h"
#include "rkxtra.h"
#include "bapkg.h"

#define STR_TO_INT  (RK_CTST + RK_QPOS + RK_IORF + SCANLEN - 1)

/*=====================================================================*
*                              gettimnw                                *
*                                                                      *
*  This is a wrapper that calls gettimno with arguments NULL and TRUE  *
*  and stores the value returned into its one argument.                *
*=====================================================================*/

void gettimnw(int *ntim) {

   *ntim = gettimno(NULL, TRUE);

   } /* End gettimnw() */


/*=====================================================================*
*                              gettimno                                *
*                                                                      *
*  Get the number corresponding to a given timer ID letter.            *
*  Timer "0" is number 1, timer "1" is number 2, "A" - "Z" follow.     *
*                                                                      *
*  Arguments:                                                          *
*     timlet   Ptr to a string containing the letter to be interpreted.*
*              If this is a NULL ptr, get the string by calling scan().*
*     k01      TRUE to allow "0" or "1" as well as a letter.           *
*                                                                      *
*  Returns 0 if input is not a valid timer ID.                         *
*                                                                      *
*  N.B.  The gaps in the EBCDIC alphabet are currently handled by      *
*  allowing a large enough timer array that they can be ignored.       *
*  This would be the place to insert a more sophisticated treatment.   *
*=====================================================================*/

int gettimno(char *timlet, int k01) {

   char lea[SCANLEN1];
   int ulet;

   if (!timlet) {
      if (RK.scancode == RK_EQUALS) {
         scanck(lea, RK_REQFLD, ~(RK_COMMA|RK_LFTPAREN));
         timlet = lea; }
      else {
         ermark(RK_EQRQERR);
         return 0; }
      }

   ulet = toupper(timlet[0]);
   if (k01) {
      if (!strcmp(timlet, "0")) return TRTN0;
      if (!strcmp(timlet, "1")) return TRTN1;
      }
   if (strlen(timlet) == 1 && ulet >= 'A' && ulet <= 'Z')
      return (ulet - ('A' - TRTNA));

   ermark(RK_IDERR);
   return 0;

   } /* End gettimno() */


/*=====================================================================*
*                              d3g3timr                                *
*=====================================================================*/

void d3g3timr(mdlt_type wmdl, char *timlet) {

#define OPTNUM   7   /* Number of options */
   static char *options[OPTNUM] = {
      "CYCLES", "TRIALS", "STIMULI", "DETAIL", "MAXCYC",
      "EVRELTN", "EVTRIAL" };
   enum trops { PPCYCS=1, PPTRLS, PPSTIM, PPDTL, PPMAXC,
      PPEVREL, PPEVTR };
#define EVTRON   3   /* Number of EVTRIAL suboptions */
   static char *evtrop[EVTRON] = { "ON", "OFF", "LIST" };
   enum evtops { EVTON=1, EVTOFF, EVTLIST };

   struct TRTIMER  *ptt, **pptt;
   iter   trti;                  /* Iterator for making stim masks */
   size_t szsm;                  /* Size of stimulus mask */
   long   isr;                   /* Stimulus ref number */
   int jlet = gettimno(timlet,0);/* Timer array bit offset */
   enum trops mret;              /* Match return code */

/* Error return if unidentified timer ID letter */

   if (jlet == 0) {
      skip2end();
      return; }

   /* Set to save iteration lists on node 0 only */
   ilstsalf(callocv, reallocv, freev);

/* Find matching TRTIMER block or create a new one */

   szsm = (size_t)BytesInXBits(RP->mxnds);
   pptt = &RP0->ptrt1;
   while (ptt = *pptt) {      /* Assignment intended */
      if (ptt->jtno == jlet) goto GotTimer;
      pptt = &ptt->pntrt;
      }
   ptt = *pptt = callocv(1, sizeof(struct TRTIMER)+szsm-1, "Timer");
   ptt->jtno = jlet;
GotTimer:
   ptt->smdlts = wmdl;

/* Loop over fields, matching options */

   while (1) {
      mret = (enum trops)match(RK_NMERR, RK_MNEWK,
         ~(RK_BLANK|RK_COMMA|RK_EQUALS|RK_LFTPAREN), 0,
         options, OPTNUM);
      if (RK.scancode & RK_ENDCARD) break;
      switch (mret) {

      case PPCYCS:      /* CYCLES (compatibility misnomer) */
      case PPTRLS:      /* TRIALS */
         if (RK.scancode != RK_EQUALS) ermark(RK_EQRQERR);
         RP0->ilseed += 103;  /* Any old number will do */
         ptt->ptil = ilstread(ptt->ptil, IL_NEW|IL_ELOK|IL_ESOK,
            1, RP0->ilseed);
         RK.iexit |= ilstchk(ptt->ptil, IL_MASK, "Trial list");
         break;

      case PPSTIM:      /* STIM */
         if (RK.scancode != RK_EQUALS) {
            ermark(RK_EQRQERR); break; }
ScanStimList:
         RP0->ilseed += 105;  /* Any old number will do */
         ptt->psil = ilstread(ptt->psil, IL_NEW|IL_ELOK|IL_ESOK,
            1, RP0->ilseed);
         RK.iexit |= ilstchk(ptt->psil, RP->mxnds, "Stimulus list");
         /* Now make a mask from this ilst.  The only reason the
         *  ilst itself is saved is so the user can modify it... */
         memset((char *)ptt->timsm, 0, szsm);
         ilstset(&trti, ptt->psil, 0);
         while ((isr = ilstiter(&trti)) > 0)
            bitset(ptt->timsm, isr);
         break;

      case PPDTL:       /* DETAIL */
         /* This obsolete option was allowed only on
         *  the PRINT card, now treated as TIMER O.  */
         if (RK.scancode & ~RK_COMMA) ermark(RK_PUNCERR);
         if (!strcmp(timlet, PRTRT)) RP0->n0flags |= N0F_DPCTRL;
         else ermark(RK_IDERR);
         break;

      case PPMAXC:      /* MAXCYC */
         ermark(RK_WARNING|RK_MARKFLD);
         if (RK.scancode != RK_EQUALS) ermark(RK_EQRQERR);
         else scanck(NULL, RK_REQFLD, ~RK_COMMA);
         cryout(RK_P1, "0-->WARNING: MAXCYC is now ignored "
            "when not in a parenthesized list.", RK_LN2, 0);
         break;

      case PPEVREL:     /* EVRELTN */
         if (RK.scancode & ~RK_COMMA) ermark(RK_PUNCERR);
         ptt->tmrop |= TRTREL;
         break;

      case PPEVTR:      /* EVTRIAL */
         mret = (enum trops)match(RK_EQCHK, RK_MREQF,
            ~RK_COMMA, 0, evtrop, EVTRON);
         switch (mret) {
         case EVTON:       /* ON */
            ptt->tmrop = ptt->tmrop & ~TRTEVOFF | TRTEVON;
            break;
         case EVTOFF:      /* OFF */
            ptt->tmrop = ptt->tmrop & ~TRTEVON | TRTEVOFF;
            break;
         case EVTLIST:     /* LIST */
            ptt->tmrop &= ~(TRTEVON|TRTEVOFF);
         default:
            break;
            }
         break;

      default:          /* Must be stimulus list (or error) */
         scanagn();
         goto ScanStimList;

         } /* End switch */
      } /* End while */

   } /* End d3g3timr() */
