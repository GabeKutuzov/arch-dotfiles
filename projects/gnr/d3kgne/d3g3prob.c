/* (c) Copyright 2009, Neurosciences Research Foundation, Inc. */
/* $Id: d3g3prob.c 26 2009-12-31 21:09:33Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                             d3g3prob.c                               *
*                                                                      *
*           Read and interpret Group III PROBE control card            *
*                                                                      *
*  N.B.  This code enforces the d3go() restriction that no cell type   *
*  can be affected by more than one PROBE card.  But the CLIST=(list,  *
*  ndx) form allows one cell list to be linked to more than one cell   *
*  type.  It is OK for a PROBE card to refer to a CLIST that refers    *
*  to more than one cell type.  The PRBDEF block is converted into an  *
*  array in d3news so each cell type can have its own myipc1, etc.     *
*                                                                      *
*  N.B.  The PRBDEF chain is built in three sections to enforce the    *
*  documented priorities:  (1) Entries with psct != 0 (specific rlnm   *
*  was given), (2) Entries with clnum < 0 (explicit cell list given),  *
*  (3) Entries with clnum > 0 (reference to a numbered cell list).     *
*  All PRBDEFs are Dynamic, even if disabled (rare), because other-    *
*  wise all sorts of care would be needed with the linkage pointers.   *
*                                                                      *
*  N.B.  Key difference from DETAIL PRINT case is that PRBDEF blocks,  *
*  unlike DPDATA blocks, are built for both explicit lists and CLIST   *
*  refs and are stored in shared memory, because the stimulation parms *
*  stay there to save space in the CELLTYPE block.  PRBDEFs are not    *
*  matched up to cell types until d3news(), whether or not an rlnm was *
*  specified, just to keep all the matching code in one place.  New    *
*  PRBDEF's with clnum < 0 go at the start of the list, those with     *
*  clnum > 0 go at the end.  This permits d3news() to get by with one  *
*  pass through the entire list.                                       *
*                                                                      *
************************************************************************
*  V8B, 09/22/01, GNR - Complete rewrite to handle timers & cell lists.*
*  Rev, 03/02/02, GNR - Allow keyword CLIST with cell type index.      *
*  Rev, 03/10/02, GNR - Broken out from d3grp3.c                       *
*  ==>, 01/06/08, GNR - Last mod before committing to svn repository   *
*  V8E, 02/10/09, GNR - Add ramps, inform 'N(' code                    *
*  Rev, 07/01/09, GNR - Add FIRSTCYC=cyc1                              *
***********************************************************************/

#define CLTYPE  struct CLBLK

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "d3global.h"
#include "clblk.h"
#include "rocks.h"
#include "rkxtra.h"
#include "bapkg.h"

enum prbkeyt { unrec_opt = 0, sprb_opt, ramp_opt, pptr_opt, cpp_opt,
   cyc1_opt, enb_opt, dsb_opt, opt_opt, timer_opt, uprog_opt,
   clist_opt, cells_opt, word_for_d3clst } ;

/*=====================================================================*
*                           rdprbopt(pprb)                             *
*  Read probe options.                                                 *
*  Arguments:                                                          *
*     pprb     Ptr to PRBDEF block to be filled in.                    *
*  Return value:                                                       *
*     TRUE     An option was entered to turn off this probe.           *
*     FALSE    The probe has not been turned off.                      *
*              Any errors are reported in RK.iexit and/or RK.highrc.   *
*=====================================================================*/

static int rdprbopt(struct PRBDEF *pprb) {

   int    nclopt = 0;         /* Number of cell list options entered */
   char   lea[SCANLEN1];      /* Scanning buffer */

#define PRBKNUM 12            /* Number of possible keys  */
   static char *prbkeys[] = { "SPRB", "RAMP", "PPTR", "CPP",
      "FIRSTCYC", "ENABLE", "DISABLE", "OPT", "TIMER", "UPROG",
      "CLIST", "CELLS" };
   enum prbkeyt smret;

/* Read keyword parameters */

   while (!(scanck(lea, RK_NEWKEY, ~(RK_COMMA|RK_EQUALS|
         RK_INPARENS|RK_LFTPAREN|RK_ENDCARD)) & RK_ENDCARD)) {
      smret = (enum prbkeyt)smatch(RK_NMERR, lea, prbkeys, PRBKNUM);

      /* If in parens, this has to be a cell list.  Reset smret
      *  so that if a region name happens to be the same as one
      *  of the recognized keywords, we will treat it as a cell
      *  list and not as one of those keywords.  Similarly, if
      *  keyword is not recognized, and this is format without
      *  positional layer name, let d3clst process for NEW, etc.
      */
      if ((smret == unrec_opt && !pprb->psct) ||
         RK.scancode & RK_INPARENS) smret = word_for_d3clst;

      switch (smret) {

      case unrec_opt:            /* Unrecognized, not in parens */
         ermark(RK_IDERR);
         while (RK.scancode & RK_EQUALS) scan(NULL, 0);
         continue;

      case sprb_opt:             /* SPRB (activation level) */
         eqscan(&pprb->sprb, "B20/27IL$mV", RK_EQCHK);
         break;

      case ramp_opt:             /* RAMP (activation level) */
         eqscan(&pprb->ramp, "B20/27IL$mV", RK_EQCHK);
         break;

      case pptr_opt:             /* PPTR (probes per trial) */
         eqscan(&pprb->pptr, "VIL", RK_EQCHK);
         break;

      case cpp_opt:              /* CPP (cycles per probe) */
         eqscan(&pprb->cpp, "V>UH", RK_EQCHK);
         break;

      case cyc1_opt:             /* FIRSTCYC */
         eqscan(&pprb->cyc1, "V>UH", RK_EQCHK);
         break;

      case enb_opt:
         pprb->prbdis = FALSE;
         break;

      case dsb_opt:
         pprb->prbdis = TRUE;
         break;

      case opt_opt:              /* OPT */
         eqscan(&pprb->prbopt, "KCV", RK_EQCHK);
         break;

      case timer_opt:            /* TIMER */
         pprb->jprbt = (byte)gettimno(NULL, YES);
         break;

      case uprog_opt:            /* UPROG */
         if (match(0, RK_MREQF, ~RK_COMMA, RK_INPARENS,
               prbkeys+(int)sprb_opt-1, 1) <= 0) {
            while (RK.scancode & RK_INPARENS) scan(NULL, 0);
            scanagn();
            continue;
            }
         pprb->pupsprb = (float (*)(long, long, long,
            long, long, float))d3uprg(TRUE);
         break;

      case clist_opt:            /* CLIST */
         /* Delete any existing unique cell list */
         if (pprb->clnum < 0) d3clrm(pprb->clnum);
         /* Read new cell list num, return code to delete
         *  PRBDEF if list is turned OFF.  */
         if (getclid(&pprb->clnum, &pprb->clndx)) {
            thatsall();
            return TRUE;
            }
         if (nclopt++ > 0) cryout(RK_P1, "0-->WARNING:  Only last "
            "cell list option will be used.", RK_LN2, NULL);
         break;

      case word_for_d3clst:      /* Possible explicit cell list */
         scanagn();
         goto cells_opt_no_punct_test;

      case cells_opt:            /* CELLS= and an explicit list */
         if (RK.scancode != RK_EQUALS) {
            ermark(RK_PUNCERR);
            break; }

      cells_opt_no_punct_test:   /* Here for list w/o CELLS= */
         /* Create a unique list number unless there is already
         *  one associated with this block--which can only happen
         *  with the positional cell type style.  */
         if (pprb->clnum >= 0) {
            pprb->clnum = --RP0->nextclid;
            pprb->clndx = 0; }
         /* Read new cell list, return code to delete PRBDEF if
         *  list is turned OFF.  The address of the CLHDR is not
         *  saved--makes later code more similar to CLIST case.  */
         if (!d3clst(pprb->clnum,
               pprb->psct ? CLKM_SINGLE : CLKM_MULTI)) {
            /* If there was no explicit OFF option, and an errmsg
            *  was not yet generated, then RK.highrc is 0 here. This
            *  means an unidentified field was passed into d3clst()
            *  but not processed, so now an errmsg is needed.  */
            if (RK.highrc == 0) {
               if (smret == cells_opt)
                  ilstreq();
               else
                  ermark(RK_IDERR);
               }
            thatsall();
            return TRUE;
            }
         if (nclopt++ > 0) cryout(RK_P1, "0-->WARNING:  Only last "
            "cell list option will be used.", RK_LN2, NULL);
         break;

      default:          /* Probably a punctuation error */
         continue;

         } /* End option switch */

      /* Flag entry of this option for copying below */
      pprb->prob1st |= 1L << (int)smret;

      } /* End field scan */

   return FALSE;

   } /* End rdprbopt() */

/*=====================================================================*
*                        rmprobe(pprb, newprb)                         *
*  Remove a probe.                                                     *
*  Arguments:                                                          *
*     pprb     Ptr to PRBDEF block to be removed.                      *
*     newprb   TRUE if probe was just made--produce warning unless     *
*              RK.highrc indicates there was a syntax error.           *
*=====================================================================*/

static void rmprobe(struct PRBDEF *pprb, int newprb) {

   struct PRBDEF **ppprb = &RP->pfprb;

   /* If probe was just created, and no syntax error, give a warning */
   if (newprb && RK.highrc < 4)
       cryout(RK_P1, "0-->WARNING:  You are removing a probe "
         "that was just created.", RK_LN2, NULL);

   /* Delete the cell list if unique */
   if (pprb->clnum < 0) d3clrm(pprb->clnum);

   /* If it's in the PRBDEF list already, unlink it */
   for ( ; *ppprb; ppprb=&(*ppprb)->pnprb) {
      if (*ppprb == pprb) {
         *ppprb = pprb->pnprb;
         break;
         }
      }

   freepv(pprb, "Probe data");

   } /* End rmprobe() */

/*=====================================================================*
*                             getprobe()                               *
*  Process PROBE control card.                                         *
*=====================================================================*/

void getprobe(void) {

   struct CELLTYPE *il;
   struct CLHDR    *pclh;
   struct PRBDEF   *pprb,*ptprb,**ppprb;
   int    newprb = FALSE;     /* TRUE if it's a new probe */
   int    krmprb;             /* TRUE to delete this probe */
   short  oldhighrc;          /* Saved value of RK.highrc */
   rlnm   trlnm;              /* Region and layer name    */
   char   lea[SCANLEN1];      /* Scanning buffer */

/* Save RK.highrc and set it to zero so we can detect syntax
*  errors found on the current control card.  */

   oldhighrc = RK.highrc;
   RK.highrc = 0;

/* Determine which form of the PROBE card we have.
*  There are three top-level cases:
*     (1) A word followed by an equals sign:  This has to have
*         a CLIST option on it somewhere or it is an error.  We
*         no longer require that CLIST= be the first keyword, so
*         we have to set up a temporary PRBDEF and match it later
*         to the specified CLIST and from there the celltypes.
*     (2) Just the word OFF:  Remove all probes and PRBDEFs.
*     (3) A word followed by a blank or comma, not the last word
*         on the card:  This has to be a cell type specifier.
*         This is the only form that can have positional numeric
*         parameters.
*  Any unexpected punctuation is probably a serious error, so
*  this card is abandoned in that case.  */

   cdscan(RK.last, 1, SCANLEN, RK_WDSKIP);
   if (scanck(lea, RK_REQFLD, ~(RK_BLANK|RK_COMMA|RK_EQUALS)) &
      ~(RK_BLANK|RK_COMMA|RK_EQUALS)) goto NEXT_CARD;

/* Case 1.  Make a PRBDEF, fill it with defaults,
*  push the field back, and scan for keywords.  */

   if (RK.scancode & RK_EQUALS) {
      ptprb = allocpcv(Dynamic, 1, IXstr_PRBDEF, "Probe data");
      prbdflt(ptprb);
      scanagn();
      krmprb = rdprbopt(ptprb);

/* If we got a cell list number, try to match it up with that of
*  any existing PRBDEF (it doesn't matter whether the CLIST has
*  actually been entered or not at this point).  If found, either
*  delete new and old PRBDEFs or copy over all options that were
*  modified on the new PROBE card.  If not found, either delete
*  the new one (with a warning) or just move it to the PRBDEF
*  list.  Position it for LIFO processing, with all the negative
*  clnums first, then all the positive.  If there was no cell
*  list of any kind, drop through to warning below.  */

      if (ptprb->clnum) {
         /* Here is a case where structured programming is ugly */
         for (pprb=RP->pfprb; pprb; pprb=pprb->pnprb) {
            if (pprb->clnum == ptprb->clnum &&
                pprb->clndx == ptprb->clndx) break;
            }
         if (pprb) {                      /* CLNUM MATCHED */
            if (krmprb) {                    /* Got OFF option? */
               rmprobe(pprb, FALSE);         /* Remove this PRBDEF */
               freepv(ptprb, "Probe data");  /* Remove temp PRBDEF */
               goto NEXT_CARD;               /* Avoid clnum test */
               }
            else {                           /* Copy all overrides */
               if (ptprb->prob1st & 1L << (int)sprb_opt)
                  pprb->sprb = ptprb->sprb;
               if (ptprb->prob1st & 1L << (int)ramp_opt)
                  pprb->ramp = ptprb->ramp;
               if (ptprb->prob1st & 1L << (int)pptr_opt)
                  pprb->pptr = ptprb->pptr;
               if (ptprb->prob1st & 1L << (int)cpp_opt)
                  pprb->cpp = ptprb->cpp;
               if (ptprb->prob1st & 1L << (int)cyc1_opt)
                  pprb->cyc1 = ptprb->cyc1;
               if (ptprb->prob1st & (1L<<(int)enb_opt|1L<<(int)dsb_opt))
                  pprb->prbdis = ptprb->prbdis;
               /* Controls for OPT= are still in RK.mcbits, etc. */
               if (ptprb->prob1st & 1L << (int)opt_opt)
                  mcodopc(&pprb->prbopt);
               if (ptprb->prob1st & 1L << (int)timer_opt)
                  pprb->jprbt = ptprb->jprbt;
               if (ptprb->prob1st & 1L << (int)uprog_opt)
                  pprb->pupsprb = ptprb->pupsprb;
               freepv(ptprb, "Probe data");  /* Remove temp PRBDEF */
               }
            }
         else {                           /* CLNUM NOT MATCHED */
            short tval;                      /* Test clnum value */
            if (krmprb) {                    /* Got OFF option? */
               rmprobe(ptprb, TRUE);         /* Remove with warning */
               goto NEXT_CARD;               /* Avoid clnum test */
               }
            /* Establish test value that will place this PRBDEF before
            *  others of its kind (explicit or implicit CLIST). */
            tval = (ptprb->clnum > 0) ? 0 : -SHRT_MAX;
            for (ppprb=&RP->pfprb;           /* Find insertion point */
                  (pprb=*ppprb) &&           /* Assignment intended */
                  pprb->psct && pprb->clnum<tval;
                  ppprb=&(*ppprb)->pnprb) ;
            ptprb->pnprb = pprb;
            *ppprb = ptprb;
            pprb = ptprb;                    /* For clnum test below */
            } /* End clnum not matched */
         } /* End handling probe with cell list found */
      } /* End Case 1 */
   else {

/* Distinguish Case 2 from Case 3.  If first and only word on card
*  is "OFF", this is case 2.  Otherwise, save first two words as a
*  potential region and layer name and go on to Case 3.  */

      if (RK.length >= LSNAME) ermark(RK_LENGERR);
      memcpy(trlnm.rnm, lea, LSNAME);
      scanlen(LSNAME);
      scan(trlnm.lnm, RK_FENCE);
      scanlen(SCANLEN);
      if (ssmatch(lea, "OFF", 3) && RK.scancode & RK_ENDCARD) {

/* Case 2.  Delete all the PRBDEF blocks and
*  any explicit CLBLKs that they refer to.  */

         for (pprb=RP->pfprb; pprb; ) {
            struct PRBDEF *nextprb = pprb->pnprb;
            if (pprb->clnum < 0) d3clrm((int)pprb->clnum);
            freepv(pprb, "Probe data");
            pprb = nextprb;
            }
         RP->pfprb = NULL;
         } /* End Case 2 */
      else {

/* Case 3.  Positional region and layer names are now in trlnm.
*  If there is not already a PRBDEF block for this cell type,
*  make one now and position it for LIFO processing.  */

         if ((il = findln(&trlnm)) == NULL) {
            cryout(RK_E1, "0***", RK_LN2+4, fmtrlnm(&trlnm),
               LXRLNM, " NOT FOUND.", RK_CCL, NULL);
            goto NEXT_CARD; }
         if (!(pprb = il->pctprb)) {   /* Assignment intended */
            il->pctprb = pprb =
               allocpcv(Dynamic, 1, IXstr_PRBDEF, "Probe data");
            prbdflt(pprb);
            pprb->pnprb = RP->pfprb;
            RP->pfprb = pprb;
            pprb->psct = il;
            newprb = TRUE;
            }

/* Now we have a PRBDEF.  If the next and last keyword is "OFF",
*  remove all probing information for this cell type and set a
*  flag in the cell type to ignore any CLIST-style probes.  */

         scanck(lea, RK_NEWKEY,
            ~(RK_COMMA|RK_EQUALS|RK_INPARENS|RK_LFTPAREN));
         if (ssmatch(lea, "OFF", 3)) {
            if (scan(lea, 0) & RK_ENDCARD)
               rmprobe(pprb, newprb);
            else
               ermark(RK_TOOMANY);
            il->ctf |= CTFNP;
            goto NEXT_CARD;
            } /* End local "OFF" option processing */
         scanagn();

/* Read optional positional parameters until a nonnumeric value
*  or parentheses are encountered.  Then read keyword parameters.
*  A request to turn off a list is OK if it is a unique (clnum < 0)
*  list, but is an error if it is a general (clnum > 0) list.
*  However, in case the general list can be identified as a single
*  list referring to this cell type, we accept the removal option.
*/

         il->ctf &= ~CTFNP;         /* Remove previous prohibition */
         inform("(S=KN(,B20/27IL$mV,UL,UH,B20/27IL$mV)",
            &pprb->sprb, &pprb->pptr, &pprb->cpp, &pprb->ramp, NULL);
         krmprb = rdprbopt(pprb);
         pclh = findcl(pprb->clnum, FALSE);
         if (krmprb) {
            if (pprb->clnum > 0) {
               if (pclh && pclh->nclb == 1 &&
                     pclh->pfclb->pcll == il) {
                  d3clrm(pprb->clnum);
                  rmprobe(pprb, FALSE);
                  }
               else {
                  cryout(RK_E1, "0***A CLIST reference cannot be "
                     "removed from a single cell type.", RK_LN5,
                     "   Use PROBE ", RK_LN0, trlnm.rnm, LSNAME,
                     " ", 1, trlnm.lnm, LSNAME, " OFF to remove "
                     "all probes from this cell type.", RK_CCL,
                     "   Use PROBE CLIST=(n,OFF) to remove all "
                     "probes that use this CLIST.", RK_LN0,
                     "   Otherwise, edit the CLIST to remove the "
                     "cells of this cell type.", RK_LN0, NULL);
                  }
               }
            else
               rmprobe(pprb, newprb);
            goto NEXT_CARD;
            } /* End handling OFF request */
         if (pclh && pclh->pfclb && pclh->pfclb->clnm.rnm[0] == 0)
            pclh->pfclb->clnm = trlnm;
         } /* End Case 3 */
      } /* End Not Case 1 */

/* If no cell lists were found, print a warning and delete
*  the PRBDEF block.  Note:  This test cannot be done inside
*  rdprbopt(), because the card might be an update of an
*  existing PRBDEF.  So we just have to jump around this test
*  to NEXT_CARD every time a probe is removed.  */

   if (pprb->clnum == 0) {
      cryout(RK_P1, "0-->WARNING: "
         "No cell list found for this PROBE.", RK_LN3,
         "    PROBE has been deleted.", RK_LN0, NULL);
      rmprobe(pprb, FALSE);
      }

   if (oldhighrc > RK.highrc) RK.highrc = oldhighrc;
   return;

NEXT_CARD:
   skip2end();
   if (oldhighrc > RK.highrc) RK.highrc = oldhighrc;
   return;

   } /* End getprobe() */
