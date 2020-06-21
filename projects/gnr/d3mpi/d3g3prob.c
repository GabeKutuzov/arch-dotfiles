/* (c) Copyright 2001-2016, The Rockefeller University *11116* */
/* $Id: d3g3prob.c 72 2017-04-14 19:11:21Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                             d3g3prob.c                               *
*           Read and interpret Group III PROBE control card            *
*                                                                      *
*  N.B. The format of the PROBE card has been heavily modified for R67.*
*  The format that involved providing a CLIST index and no rlnm on the *
*  PROBE card has been eliminated because the code (and documentation) *
*  was a nightmare to maintain. The COPY option has been added to give *
*  a way to use the same parameters for more than one cell type. It is *
*  now possible to have more than one probe on the same cell type. All *
*  PRBDEFs are Dynamic, even if disabled (rare), because otherwise all *
*  sorts of care would be needed with the linkage pointers.            *
*                                                                      *
*  N.B.  PRBDEFs are now matched up here to cell types, but are not    *
*  matched up to cell lists until d3news(), because cell lists can be  *
*  changed on control cards following this one.                        *
*                                                                      *
************************************************************************
*  V8B, 09/22/01, GNR - Complete rewrite to handle timers & cell lists.*
*  Rev, 03/02/02, GNR - Allow keyword CLIST with cell type index.      *
*  Rev, 03/10/02, GNR - Broken out from d3grp3.c                       *
*  ==>, 01/06/08, GNR - Last mod before committing to svn repository   *
*  V8E, 02/10/09, GNR - Add ramps, inform 'N(' code                    *
*  Rev, 07/01/09, GNR - Add FIRSTCYC=cyc1                              *
*  R66, 02/27/16, GNR - Longs to si32 in PRBDEF                        *
*  R67, 11/06/16, GNR - No bad membcst ptrs, allow >1 probe/celltype   *
***********************************************************************/

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

enum prbkeyt { punct_err = -1, unrec_opt = 0, sprb_opt, ramp_opt,
   pptr_opt, cpp_opt, cyc1_opt, enb_opt, dsb_opt, opt_opt, timer_opt,
   uprog_opt, clist_opt, cells_opt } ;
enum rdopret { rdopt_ok = 0, rdopt_error } ;
enum typekwd { tpcnone = 0, tpcnew, tpccopy, tpcmod, tpcoff } ;

/*=====================================================================*
*                           rdprbopt(pprb)                             *
*  Read probe options.  This routine is called after getprobe() has    *
*     determined the type and either created a new PRBDEF or located   *
*     an existing one to be modified.                                  *
*  Arguments:                                                          *
*     pprb     Ptr to PRBDEF block to be filled in.                    *
*  Return value (see enum rdopret):                                    *
*     0     Normal return.                                             *
*     1     An error was found -- discontinue processing this card.    *
*           Any errors are reported in RK.iexit and/or RK.highrc.      *
*=====================================================================*/

static enum rdopret rdprbopt(struct PRBDEF *pprb) {

   int    nclopt = 0;         /* Number of cell list options entered */
   char   lea[SCANLEN1];      /* Scanning buffer */

#define PRBKNUM 12            /* Number of possible keys  */
   static char *prbkeys[] = { "SPRB", "RAMP", "PPTR", "CPP",
      "FIRSTCYC", "ENABLE", "DISABLE", "OPT", "TIMER", "UPROG",
      "CLIST", "CELLS" };
   enum prbkeyt smret;

/* Read keyword parameters */

   while (!(scanck(lea, RK_NEWKEY,
         ~(RK_COMMA|RK_EQUALS|RK_ENDCARD)) & RK_ENDCARD)) {
      smret = (enum prbkeyt)smatch(0, lea, prbkeys, PRBKNUM);

      switch (smret) {

      case punct_err:
      case unrec_opt:            /* Unrecognized or punct error */
         return rdopt_error;     /* (Msg issued by smatch) */

      case sprb_opt:             /* SPRB (activation level) */
         eqscan(&pprb->sprb, "B20/27IJ$mV", RK_EQCHK);
         break;

      case ramp_opt:             /* RAMP (activation level) */
         eqscan(&pprb->ramp, "B20/27IJ$mV", RK_EQCHK);
         break;

      case pptr_opt:             /* PPTR (probes per trial) */
         eqscan(&pprb->pptr, "VIJ", RK_EQCHK);
         break;

      case cpp_opt:              /* CPP (cycles per probe) */
         eqscan(&pprb->cpp, "V>UH", RK_EQCHK);
         break;

      case cyc1_opt:             /* FIRSTCYC */
         eqscan(&pprb->cyc1, "V>UH", RK_EQCHK);
         break;

      case enb_opt:
         if (RK.scancode & ~RK_COMMA) ermark(RK_PUNCERR);
         pprb->prbdis &= ~PRBDUSR;
         break;

      case dsb_opt:
         if (RK.scancode & ~RK_COMMA) ermark(RK_PUNCERR);
         pprb->prbdis |= PRBDUSR;
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
         /* If already got a CELLS or CLIST on this card, error */
         if (nclopt++) {
            cryout(RK_E1, "0***MORE THAN ONE CELL LIST "
               "ON PROBES CARD.", RK_LN2, NULL);
            return rdopt_error; }
         /* Delete any existing unique cell list--this would be a
         *  legal change from CELLS= to CLIST= style for this PROBE. */
         if (pprb->prbclloc.clnum < 0) d3clrm(pprb->prbclloc.clnum);
         /* Read new cell list num (and possibly index).  Request
         *  to delete cell list is no longer valid here.  */
         if (getclid(&pprb->prbclloc)) {
            cryout(RK_E1, "0***USE CLIST CARD TO DELETE CELL LIST.",
               RK_LN2, NULL);
            return rdopt_error;
            }
         break;

      case cells_opt:            /* CELLS= and an explicit list */
         if (RK.scancode != RK_EQUALS) {
            ermark(RK_PUNCERR);
            /* Probably hopeless to read more options now */
            return rdopt_error; }

         /* If already got a CELLS or CLIST on this card, error */
         if (nclopt++) {
            cryout(RK_E1, "0***MORE THAN ONE CELL LIST "
               "ON PROBES CARD.", RK_LN2, NULL);
            return rdopt_error; }
         /* Create a unique list number unless there is already
         *  one associated with this block. */
         if (pprb->prbclloc.clnum >= 0) {
            pprb->prbclloc.clnum = --RP0->nextclid;
            pprb->prbclloc.clndx = 0; }
         /* Read new cell list, return code to delete PRBDEF if
         *  list is turned OFF.  The address of the CLHDR is not
         *  saved--makes later code more similar to CLIST case.  */
         if (!d3clst(pprb->prbclloc.clnum, CLKM_SINGLE)) {
            /* If there was no explicit OFF option, and an errmsg
            *  was not yet generated, then RK.highrc is 0 here. This
            *  means an unidentified field was passed into d3clst()
            *  but not processed, so now an errmsg is needed.  */
            if (RK.highrc == 0)
               ilstreq();
            return rdopt_error;
            }
         break;

      default:          /* Probably a punctuation error */
         continue;

         } /* End option switch */

      /* Flag entry of this option for copying below */
      pprb->prob1st |= 1 << (int)smret;

      } /* End field scan */

   return rdopt_ok;

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
   if (pprb->prbclloc.clnum < 0) d3clrm(pprb->prbclloc.clnum);

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

   struct REPBLOCK *ir;
   struct CELLTYPE *il;
   struct CLHDR    *pclh;
   struct PRBDEF   *pprb,*ptprb,**ppprb;
   int    jrm;                /* Which probe to remove */
   enum typekwd kprbk;        /* PROBE card format */
   enum rdopret krmprb;       /* Result of reading card options */
   short  oldhighrc;          /* Saved value of RK.highrc */
   rlnm   trlnm;              /* Region and layer name */
   char   lea[SCANLEN1];      /* Scanning buffer */
   static char *k3keys[] = { "NEW", "COPY", "MOD", "OFF" };

/* Save RK.highrc and set it to zero so we can detect syntax
*  errors found on the current control card.  */

   oldhighrc = RK.highrc;
   RK.highrc = 0;

/* Determine which form of the PROBE card we have.  All forms
*  (except full OFF) are now required to specify first a cell type.
*  This may be followed by one of the keywords NEW, COPY, MOD, OFF
*     or no keyword, which is equivalent to NEW.
*  Any unexpected punctuation is probably a serious error, so
*  this card is abandoned in that case.  */

   /* Check for 'PROBE[S] OFF' format */
   cdscan(RK.last, 1, SCANLEN, RK_WDSKIP);
   if (scanck(lea, RK_REQFLD, ~(RK_BLANK|RK_COMMA)) &
      ~(RK_BLANK|RK_COMMA)) goto NEXT_CARD;
   if (RK.length >= LSNAME) ermark(RK_LENGERR);
   if (ssmatch(lea, "OFF", 3)) {
      /* Delete all the PRBDEF blocks and any explicit CLBLKs that
      *  they refer to.  */
      if (!(RK.scancode & RK_ENDCARD)) {
         cryout(RK_E1, "0***'PROBES OFF' CANNOT HAVE OTHER OPTIONS.",
            RK_LN2, NULL); goto NEXT_CARD; }
      for (pprb=RP->pfprb; pprb; ) {
         struct PRBDEF *nextprb = pprb->pnprb;
         if (pprb->prbclloc.clnum < 0) d3clrm((int)pprb->prbclloc.clnum);
         freepv(pprb, "Probe data");
         pprb = nextprb;
         }
      RP->pfprb = NULL;
      goto NEXT_CARD;
      } /* End 'PROBES OFF' case */

   strncpy(trlnm.rnm, lea, LSNAME);
   if (scanck(lea, RK_REQFLD, ~(RK_BLANK|RK_COMMA)) &
      ~(RK_BLANK|RK_COMMA)) goto NEXT_CARD;
   strncpy(trlnm.lnm, lea, LSNAME);
   jrm = match(RK_NMERR, RK_MREQF,
      ~(RK_COMMA|RK_EQUALS|RK_LFTPAREN), 0, k3keys, 4);
   if (jrm < 0) goto NEXT_CARD;
   kprbk = (enum typekwd)jrm;
   switch (kprbk) {

   case tpcnone:
      scanagn();
      kprbk = tpcnew;
      /* Fall through to case NEW */
   case tpcnew:               /* Add and fill a new PRBDEF */
   case tpccopy:
      pprb = allocpcv(Dynamic, 1, IXstr_PRBDEF, "Probe data");
      if (kprbk == tpcnew) {  /* New PROBE */
         prbdflt(pprb);
         /* Read optional positional parameters until a nonnumeric
         *  value or parentheses are encountered.  */
         inform("(S=KN(,B20/27IJ$mV,IJ,UH,B20/27IJ$mV)",
            &pprb->sprb, &pprb->pptr, &pprb->cpp, &pprb->ramp, NULL);
         } /* End NEW setup */
      else {                  /* Copy existing probe */
         if (!RP->pfprb) goto BAD_COPY;
         jrm = INT_MAX;
         if (RK.scancode & RK_LFTPAREN) {    /* Specified number */
            inform("(S=V(V>I6V))", &jrm, NULL);
            for (ptprb=RP->pfprb; ptprb && --jrm > 0;
               ptprb=ptprb->pnprb) ;
            }
         else {                              /* Use most recent */
            for (ptprb=RP->pfprb; ptprb->pnprb; ptprb=ptprb->pnprb) ;
            }
         if (!ptprb) goto BAD_COPY;
         /* Copy parameter fields (less awkward than putting these
         *  in an inner struct and copying that)  */
         pprb->pupsprb = ptprb->pupsprb;
         pprb->pptr    = ptprb->pptr;
         pprb->sprb    = ptprb->sprb;
         pprb->ramp    = ptprb->ramp;
         pprb->prbclloc= ptprb->prbclloc;
         pprb->cpp     = ptprb->cpp;
         pprb->cyc1    = ptprb->cyc1;
         pprb->prbopt  = ptprb->prbopt;
         pprb->jprbt   = ptprb->jprbt;
         }  /* End COPY setup */

      /* Insert at end of PROBE list (so copy number counts fwd).
      *  (There won't be many of these, simple walk is OK.)  */
      for (ppprb=&RP->pfprb; *ppprb; ppprb=&(*ppprb)->pnprb) ;
      *ppprb = pprb;

      /* Read keyword parameters */
      krmprb = rdprbopt(pprb);

      /* Save cell type reference in cell list */
      if (krmprb != rdopt_error && pprb->prbclloc.clnum < 0) {
         if ((pclh = findcl(&pprb->prbclloc, TRUE))) {  /* Asgn intended */
            pprb->pclb = pclh->pfclb;
#if 0   /*** DEBUG ***/
            pprb->pclb->clnm = trlnm;
#endif
            }
         }

      /* Insert this in pctprb list for the named cell type--
      *  (we currently don't have a hash for this lookup).  It is
      *  expected that a given cell type will have no more than a
      *  handful of probes, therefore a simple walk to the end of
      *  the list is implemented here.  */
      for (ir=RP->tree; ir; ir=ir->pnrp) {
         if (strncmp(trlnm.rnm, ir->sname, LSNAME)) continue;
         for (il=ir->play1; il; il=il->play) {
            int ictp = 1;     /* Probe number on a cell type */
            if (strncmp(trlnm.lnm, il->lname, LSNAME)) continue;
            pprb->psct = il;
            for (ppprb=&il->pctprb; *ppprb; ppprb=&(*ppprb)->pnctp)
               ++ictp;
            pprb->ictp = (ui16)ictp;
            *ppprb = pprb;
            goto NEXT_CARD;
            }
         }
      /* Error--cell type name not matched */
      cryout(RK_E1, "0***", RK_LN2, fmtrlnm(&trlnm), RK_CCL,
         " NOT FOUND FOR THIS PROBE.", RK_CCL, NULL);
      goto NEXT_CARD;

   case tpcmod:                /* Modify an existing probe */
      if (!RP->pfprb) goto BAD_MOD;
      jrm = INT_MAX;
      if (RK.scancode & RK_LFTPAREN)      /* Specified number */
         inform("(S=V(V>I6V))", &jrm, NULL);
      /* Make a temporary PRBDEF and read parameters into it */
      ptprb = callocv(1, sizeof(struct PRBDEF), "Probe data");
      krmprb = rdprbopt(ptprb);
      if (krmprb == rdopt_error) goto NEXT_CARD;
      /* Scan probes for specified cell type */
      for (ir=RP->tree; ir; ir=ir->pnrp) {
         if (strncmp(trlnm.rnm, ir->sname, LSNAME)) continue;
         for (il=ir->play1; il; il=il->play) {
            if (strncmp(trlnm.lnm, il->lname, LSNAME)) continue;
            for (pprb=il->pctprb; pprb; pprb=pprb->pnctp) {
               if (jrm == INT_MAX || --jrm == 0) {
         /* Modify only parameters that were entered.
         *  (Three indents suppressed here to end of block) */
         if (ptprb->prob1st & 1 << (int)sprb_opt)
            pprb->sprb = ptprb->sprb;
         if (ptprb->prob1st & 1 << (int)ramp_opt)
            pprb->ramp = ptprb->ramp;
         if (ptprb->prob1st & 1 << (int)pptr_opt)
            pprb->pptr = ptprb->pptr;
         if (ptprb->prob1st & 1 << (int)cpp_opt)
            pprb->cpp = ptprb->cpp;
         if (ptprb->prob1st & 1 << (int)cyc1_opt)
            pprb->cyc1 = ptprb->cyc1;
         if (ptprb->prob1st & (1<<(int)enb_opt|1<<(int)dsb_opt))
            pprb->prbdis = ptprb->prbdis;
         /* Controls for OPT= are still in RK.mcbits, etc. */
         if (ptprb->prob1st & 1 << (int)opt_opt)
            mcodopc(&pprb->prbopt);
         if (ptprb->prob1st & 1 << (int)timer_opt)
            pprb->jprbt = ptprb->jprbt;
         if (ptprb->prob1st & 1 << (int)uprog_opt)
            pprb->pupsprb = ptprb->pupsprb;
         if (ptprb->prob1st & (1<<(int)clist_opt|1<<(int)cells_opt))
            pprb->prbclloc = ptprb->prbclloc;

                  } /* End indent suppression */
               } /* End scan of existing probes on cell type */
            freev(ptprb, "Probe data");  /* Remove temp PRBDEF */
            if (jrm != INT_MAX && jrm != 0) goto BAD_MOD;
            goto NEXT_CARD;
            }  /* End il loop */
         }  /* End ir loop */
      /* Error: Cell type for MOD not matched */
      freev(ptprb, "Probe data");
      goto BAD_MOD;

   case tpcoff:                /* Remove a probe or probes */
      if (!RP->pfprb) goto BAD_OFF;
      jrm = INT_MAX;
      if (RK.scancode & RK_LFTPAREN)      /* Specified number */
         inform("(S=V(V>I6V))", &jrm, NULL);
      if (!(scan(lea,0) & RK_ENDCARD)) {
         cryout(RK_E1, "0***SUPERFLUOUS OPTIONS AFTER 'OFF'.",
            RK_LN2, NULL);
         goto NEXT_CARD;
         }
      /* Scan probes for specified cell type */
      for (ir=RP->tree; ir; ir=ir->pnrp) {
         if (strncmp(trlnm.rnm, ir->sname, LSNAME)) continue;
         for (il=ir->play1; il; il=il->play) {
            if (strncmp(trlnm.lnm, il->lname, LSNAME)) continue;
            for (pprb=il->pctprb; pprb; pprb=pprb->pnctp) {
               if (jrm == INT_MAX || --jrm == 0) {
                  rmprobe(pprb, FALSE);
                  }
               }  /* End scan of existing probes on cell type */
            if (jrm != INT_MAX && jrm != 0) goto BAD_OFF;
            goto NEXT_CARD;
            } /* End il loop */
         } /* End ir loop */
      /* Error: Cell type for OFF not matched */
      goto BAD_OFF;
      } /* End kprbk switch */

BAD_OFF:
   cryout(RK_E1, "0***SPECIFIED PROBE(S) TO BE REMOVED NOT FOUND.",
      RK_LN2, NULL);
   goto NEXT_CARD;
BAD_MOD:
   cryout(RK_E1, "0***SPECIFIED PROBE(S) TO BE MODIFIED NOT FOUND.",
      RK_LN2, NULL);
   goto NEXT_CARD;
BAD_COPY:
   cryout(RK_E1, "0***SPECIFIED PROBE TO COPY NOT FOUND.",
      RK_LN2, NULL);
NEXT_CARD:
   skip2end();
   if (oldhighrc > RK.highrc) RK.highrc = oldhighrc;
   return;

   } /* End getprobe() */
