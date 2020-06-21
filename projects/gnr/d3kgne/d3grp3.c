/* (c) Copyright 1991-2011, Neurosciences Research Foundation, Inc. */
/* $Id: d3grp3.c 51 2012-05-24 20:53:36Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                               d3grp3                                 *
*                                                                      *
*             Read and interpret Group III control cards               *
*                                                                      *
*  Synopsis:  int d3grp3(void)                                         *
*                                                                      *
*  Return value:  A possible setting for RP->CP.outnow indicating      *
*     normal continuation (D3_NORMAL), normal termination (OUT_USER_   *
*     QUIT), or immediate (HX) termination (OUT_QUICK_EXIT).           *
*                                                                      *
*  The current value of RP->CP.outnow is checked during interpretation *
*  of the CYCLE card to determine whether cards were being read in     *
*  response to a user interruption, in which case a CYCLE card with    *
*  no arguments is the signal to continue normally.                    *
*                                                                      *
*  Rev, 11/21/89, JMS - Added snfr option to snoise card               *
*  Rev, 04/17/90, JWT - Added ENVVAL card                              *
*  Rev, 10/01/90, GNR - Add MOVIE card                                 *
*  Rev, 11/17/90, GNR - Add checking for NO_INPT on env cards          *
*  Rev, 08/28/91, GNR - Add PLATFORM and SNOUT cards                   *
*  Rev, 12/05/91, GNR - Add vbase to VALUE card                        *
*  Rev, 01/18/92, GNR - Add D1BARS to COLORS card                      *
*  V5C, 02/14/92, GNR - Eliminate ability to change value scheme type  *
*  Rev, 02/18/92, GNR - Add CAMEL card                                 *
*  Rev, 03/17/92, ABP - Add PXQQ card                                  *
*  Rev, 03/22/92, GNR - Move CIJPLOT,MIJPLOT cards to d3grp1           *
*  Rev, 04/16/92, ABP - Add RECORD card                                *
*  Rev, 02/27/93, GNR - Remove snapshot support                        *
*  V6A, 03/29/93, GNR - Add NOSS card                                  *
*  Rev, 05/20/93, GNR - Add PERSLIM to RESP FN, move STEP to runflags  *
*  V7A, 05/16/94, GNR - Add SENSE and OUTPUT cards                     *
*  V8A, 04/15/95, GNR - Merge kchng,flags into WDWDEF, add CYCLE KP=U, *
*                       MODALITY card, DETAIL PR=N, empty ENVVAL kills *
*                       list, remove TRACE and RESP FN PERSLIM|ADDPERS *
*  Rev, 08/22/96, GNR - Finish adding SENSE card                       *
*  Rev, 02/03/97, GNR - Add PAF, PCF, and MODALITY cards               *
*  Rev, 01/02/98, GNR - Command-line TEST overrides nts,ntr,ntc, DP=P  *
*  V8B, 12/31/00, GNR - Implement detailed print via ilst package      *
*  Rev, 01/20/01, GNR - Add trial timers, allow item changes on SAVE   *
*  Rev, 07/14/01, GNR - Add numbered CLIST card, text cache            *
*  Rev, 08/26/01, GNR - Add CIJPLOT, CIJ0PLOT, MIJPLOT, PLOC, PLABEL,  *
*                       PLINE, PPFPLOT, fancier probes                 *
*  V8D, 01/31/04, GNR - Add conductances and ions, new RESET options   *
*  Rev, 01/27/07, GNR - Look to next group if unrecognized card        *
*  Rev, 01/30/07, GNR - Allow words or numbers as RESET options        *
*  Rev, 11/24/07, GNR - Remove call arg, rationalize return values     *
*  ==>, 02/28/08, GNR - Last mod before committing to svn repository   *
*  Rev, 03/22/08, GNR - Add CAMERA|TV and PSCALE cards from Group I,   *
*                       ntr1,ntr2 to CYCLE, QUIT removed to cryin()    *
*  Rev, 03/27/08, GNR - Add EVTRIAL to CYCLE card                      *
*  Rev, 06/16/08, GNR - Add CSELPLOT                                   *
*  V8E, 04/14/09, GNR - Add MOVIE BMODE, 11/07/09 NOSS out, STATS in   *
*  V8F, 02/14/10, GNR - Remove conditionals on IMGTOUCH, CAMERA, TV    *
*  Rev, 02/20/10, GNR - Remove PLATFORM, VDR support                   *
*  V8G, 09/04/10, GNR - Change CYCLE KP to KUPD w/U and A options      *
*  Rev, 09/07/10, GNR - Add UDATA option to Group III CAMERA card      *
*  V8H, 11/21/10, GNR - No effector cell list changes if KEOP_CELLS    *
*  Rev, 11/24/10, GNR - Eliminate obsolete npxqqtti and PXQQ card      *
*  Rev, 01/29/11, GNR - Generalize EFFECTOR card options               *
*  Rev, 04/01/11, GNR - Change RP_IMMAMP to adefer                     *
***********************************************************************/

#define CLTYPE  struct CLBLK
#define TVTYPE  struct TVDEF

#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "d3global.h"
#include "objdef.h"
#include "d3opkeys.h"
#include "clblk.h"
#include "tvdef.h"
#ifdef BBDS
#include "bbd.h"
#endif
#include "rocks.h"
#include "rkxtra.h"
#include "bapkg.h"
#include "plots.h"

#define STR_TO_INT  (RK_CTST+RK_QPOS+RK_IORF+SCANLEN-1)

/* Prototypes of routines called only from d3grp3 */
void d3chng(void);
void d3g3rset(void);
void d3g3timr(mdlt_type wmdl, char *timlet);
#ifdef VM
void npcmd(char *);
#endif

/*---------------------------------------------------------------------*
*  This little routine can be called to get a pointer to the Group 3   *
*  control card keys and the number of keys for use in d3grp2 look-    *
*  ahead.  It seems that just making cc1 global doesn't do the trick.  *
*---------------------------------------------------------------------*/

char **getcc3(int *pncc3) {

/* Keyword strings for and number of Group III control cards */


   static char *cc3[] = {
      "CHANGE",
      "CIJPLOT",
      "CIJ0PLOT",
      "CLIST",
#ifdef VM
      "CMS",
      "CP",
#endif
      "COLORS",
      "CONDUCTANCE",
      "CONFIGURATION PLOT",
      "CREATE",
      "CSELPLOT",
      "CYCLE",
      "DELETE",
      "DETAIL",
      "EFFECTOR",
      "ENVVAL",
      "ESJPLOT",
      "FACTOR",
      "GRPNO",
      "IMGTOUCH",
      "INPUT",
      "ION",
      "MIJPLOT",
      "MODALITY",
      "MOVE",
      "MOVIE",
      "PAF",
      "PCF",
      "PLABEL",
      "PLIMITS",
      "PLINE",
      "PLOC",
      "PLOT",
      "PPFPLOT",
      "PRINT",
      "PROBES",
      "PSCALE",
      "PX",
      "REGENERATION",
      "RESET",
      "RESPONSE FUNCTION",
      "RJPLOT",
      "SAVE",
      "SELECT",
      "SENSE",
      "SHAPE",
      "SJPLOT",
      "SNOISE",
      "STATISTICS",
      "STATS",
      "TIMER",
      "CAMERA",
      "TV",          /* Old synonym for "CAMERA" */
      "VALUE"
      };

   *pncc3 = sizeof(cc3)/sizeof(char *);
   return cc3;
   } /* End getcc3() */

/* Group III control card switch cases (alphabetic) */

/*---------------------------------------------------------------------*
*                               d3grp3                                 *
*---------------------------------------------------------------------*/

int d3grp3(void) {

/* N.B.  The enum values must match the corresponding cc3 entries!
*/
   enum G3CC {
      CHANGE = 1,
      CIJPLOT,
      CIJ0PLOT,
      CLIST,
#ifdef VM
      CMS,
      CP_CD,
#endif
      COLORS,
      CONDUCT,
      CONFGPLT,
      CREATE,
      CSELPLOT,
      CYCLE,
      DELETE,
      DETAIL,
      EFFECTOR,
      ENVVAL,
      ESJPLOT,
      FACTOR,
      GRPNO,
      IMGTOUCH,
      INPUT,
      IONCD,
      MIJPLOT,
      MODALTY,
      MOVE,
      MOVIE,
      PAF,
      PCF,
      PLTLAB,
      PLTLIMS,
      PLTLIN,
      PLOC,
      PLOT,
      PPFPLOT,
      PRINT,
      PROBE,
      PSCALE,
      PX,
      REGENR,
      RESET,
      RESPFN,
      RJPLOT,
      SAVE,
      SELECT,
      SENSE,
      SHAPE_CD,
      SJPLOT,
      SNOISE,
      STATS1,
      STATS2,
      TIMER,
      CAMERA,
      TV,
      VALUE
      } ccindex;

#define ECCNUM 4     /* Number of group3 cards which terminate run */
   static char *ecc[] = { "END", "EXIT", "HX", "STOP" } ;
   enum EndType { END_END=1, END_EXIT, END_HX, END_STOP };

   char **cc3;                /* Ptr to Group 3 card keys */
   char lea[SCANLEN1], *card; /* Input buffers */
   struct MODALITY *pmdlt;    /* Ptr to current modality */
   ui32 ic;                   /* kwscan field recognition codes */
   int  kwret,smret;          /* kwret,smatch return vals */
   int  ncc3;                 /* Number of Group 3 card keys */
   enum EndType endchk = END_END;
   txtid hvision = findtxt("VISION");
   mdlt_type wmdlts = 0;      /* Working modality list */

   RP0->ccgrp = CCGRP3;       /* Indicate we are in group III */
   cc3 = getcc3(&ncc3);       /* Get ptr to Group 3 keys */

/* Set default modality to include all VISION subtypes.
*  Set ppgpno pointers to rebuilt GRPNO lists when encountered.  */

   for (pmdlt=RP->pmdlt1; pmdlt; pmdlt=pmdlt->pnmdlt) {
      pmdlt->um1.ppgpno = &pmdlt->pgpno;
      if (pmdlt->hmdltname == hvision)
         bitset((unsigned char *)&wmdlts, (long)pmdlt->mdltno);
      }

/* Clear flags in all cell list headers */

   {  struct CLHDR *pclh;
      for (pclh=RP0->pclh1; pclh; pclh=pclh->pnclh)
         pclh->clflgs = 0;
      } /* End pclh local scope */

/* Read input until end card recognized */

   for (;;) {

      /* If error detected while on-line, prompt for re-entry */
      if (RP->CP.runflags & RP_ONLINE && RK.iexit) {
         /* Pop input unit to force on-line read */
         cdunit(0);
         cryout(RK_P1+1, " ==> Re-enter command or type QUIT to quit",
            RK_LN1+RK_FLUSH, NULL);
         RK.iexit = 0;
         }

      /* Leave if EOF or got an END card--Two assignments intended */
      if (!(card = cryin()) || (endchk = (enum EndType)
         smatch(RK_NMERR, card, ecc, ECCNUM))) break;

      if (!(ccindex=(enum G3CC)  /* Assignment intended */
         smatch(OFF, card, cc3, ncc3))) ermark(RK_CARDERR);

      else  /* Process GROUP3 card */  {
         cdprt1(RK.last);     /* Print the card (if oprty <= 3) */
         switch (ccindex) {

/*---------------------------------------------------------------------*
*                             CHANGE card                              *
*---------------------------------------------------------------------*/

         case CHANGE:
            d3chng();
            break;

/*---------------------------------------------------------------------*
*                     CIJPLOT and CIJ0PLOT cards                       *
*---------------------------------------------------------------------*/

         case CIJPLOT:
            getijpl(KCIJC);
            break;

         case CIJ0PLOT:
            getijpl(KCIJC0);
            break;

/*---------------------------------------------------------------------*
*                             CLIST card                               *
*---------------------------------------------------------------------*/

         case CLIST: {
            int clid;
            inform("(SW1V>I)", &clid, NULL);
            d3clst(clid, CLKM_MULTI);
            } /* End CLIST local scope */
            break;

#ifdef VM
/*---------------------------------------------------------------------*
*                              CMS card                                *
*---------------------------------------------------------------------*/

         case CMS:
            strcat(card, ";");
            npcmd(card+4);
            break;

/*---------------------------------------------------------------------*
*                               CP card                                *
*---------------------------------------------------------------------*/

         case CP_CD:
            strcat(card, ";");
            npcmd(card);
            break;
#endif

/*---------------------------------------------------------------------*
*                             COLORS card                              *
*---------------------------------------------------------------------*/

         case COLORS:
            getcolor();
            break;

/*---------------------------------------------------------------------*
*                          CONDUCTANCE card                            *
*---------------------------------------------------------------------*/

         case CONDUCT:
            getcond(NULL, NULL);
            break;

/*---------------------------------------------------------------------*
*                          CONFIG PLOT card                            *
*---------------------------------------------------------------------*/

         case CONFGPLT:
            /* Note: with first arg 0, D3CNFG won't access stim array */
            if (RP->CP.runflags & RP_OKPLOT) d3cnfg(0, RP->pstim);
            break;

/*---------------------------------------------------------------------*
*                       CREATE and SELECT cards                        *
*---------------------------------------------------------------------*/

         case CREATE:
         case SELECT:
            /* If in a replay, ignore with no error message */
            if (RP->CP.runflags & RP_REPLAY)
               goto NEXT_CARD;
            if (RP->CP.runflags & RP_NOINPT) {
               cryout(RK_E1, "0***ENV NOT ACTIVE.", RK_LN2, NULL);
               goto NEXT_CARD;
               }
            else {
               struct OBJDEF *pobj;          /* Ptr to new object */
               pobj = envctl(RP0->Eptr, card);
               if (pobj) pobj->userdata |= (long)wmdlts; }
            break;

/*---------------------------------------------------------------------*
*                    DELETE, MOVE, and SHAPE cards                     *
*---------------------------------------------------------------------*/

         case DELETE:
         case MOVE:
         case SHAPE_CD:
            /* If in a replay, ignore with no error message */
            if (RP->CP.runflags & RP_REPLAY)
               goto NEXT_CARD;
            if (RP->CP.runflags & RP_NOINPT) {
               cryout(RK_E1, "0***ENV NOT ACTIVE.", RK_LN2, NULL);
               goto NEXT_CARD;
               }
            else
               envctl(RP0->Eptr, card);
            break;

/*---------------------------------------------------------------------*
*                            CSELPLOT card                             *
*---------------------------------------------------------------------*/

         case CSELPLOT:
            getijpl(KCIJSEL);
            break;

/*---------------------------------------------------------------------*
*                             CYCLE card                               *
*                                                                      *
*  Note that the print and plot control options on this card (DP, SP,  *
*  KUPD, PLOT, EPL) remain in effect until explicitly changed or nul-  *
*  lified by being set to 0 on a later CYCLE card.  The KUPD=U bit     *
*  should evetually also be controllable from the PX card, as in the   *
*  old days.                                                           *
*                                                                      *
*  Rev, 03/23/08, GNR - nt[src] unsigned, add FRZTRIALS, expand FRZCYC *
*                       to FRZCYCLES, ntr2,ntc2 defaults to ntr1, ntc1 *
*  V8G, 09/04/10, GNR - Change KP to KUPD with U and A bits.           *
*---------------------------------------------------------------------*/

         case CYCLE:  {

            /* Hold status of RP_NOUPAS bit for NOUPDATE option */
            int knoupas = (RP->CP.runflags & RP_NOUPAS) ? 1 : 0;
            long iskip = -1;
            ui16 lntcs;
            ui16 lntrs;

            /* Card with no parameters ok ONLY if entered from PX
            *  (in which case RP->ntr set to cycle forever)...  */
            cdscan(card, 1, SCANLEN, RK_WDSKIP);
            if (RP->CP.outnow == OUT_USER_PAUSE) {
               /* Entered from PX command */
               if (scan(lea, OFF) & RK_ENDCARD) {
                  RP->nts = 1;
                  RP->ntr = 999999UL;
                  return D3_NORMAL; }
               else scanagn();
               }

            /* Read positional (required) parameters */
            inform("(S,2*UL,UH)",
               &RP->nts, &RP->ntr1, &RP->ntc1, NULL);
            /* Defaults for freeze series */
            RP->ntr2 = RP->ntr1, RP->ntc2 = RP->ntc1;

            /* Read optional parameters...
            *  Note: "EPL" == "PLOT" for downward compatibility.  */
            while (kwret = kwscan(&ic, /* Assignment intended */
                  "FRZINT%IH",               &RP->ifrz,
                  "FRZTRIALS%UL",            &RP->ntr2,
                  "FRZCYCLES%UH",            &RP->ntc2,
                  "NTCS%X",
                  "STMDUR%X",
                  "NTRS%X",
                  "SPMFTIMER%X",
                  "TRIAL%VIL",               &iskip,
                  "DP%KCICORVSAF",           &RP->kdprt,
                  "EPL%KH" okkplop,          &RP0->kplt2,
                  "EVHOSTS%VI",              &RP0->evhosts,
            /*  N.B.: See note in rpdef.h re defs of kevtr bits.  */
                  "EVTRIAL%KHV1DESUTGWPFI",  &RP->kevtr,
                  "KP%KCQU",                 &RP->kpsta,
                  "NOUPDATE%KIA",            &knoupas,
                  "PLOT%KH" okkplop,         &RP0->kplt2,
                  "SPR%KCNSAF",              &RP0->kiarr,
                  "DCPTIMER%N" KWS_TIMNW,    &RP->jdcptim,
                  "EPTIMER%N" KWS_TIMNW,     &RP->jeptim,
                  "SPTIMER%N" KWS_TIMNW,     &RP->jsptim,
                  "VCPTIMER%N" KWS_TIMNW,    &RP->jvcptim,
                  NULL)) switch (kwret) {

               case 1:  /* NTCS */
               case 2:  /* STMDUR */
                  eqscan(&lntcs, "VIH", RK_EQCHK);
                  for (pmdlt=RP->pmdlt1; pmdlt; pmdlt=pmdlt->pnmdlt)
                     pmdlt->ntcs = lntcs;
                  break;
               case 3:  /* NTRS */
                  eqscan(&lntrs, "VIH", RK_EQCHK);
                  for (pmdlt=RP->pmdlt1; pmdlt; pmdlt=pmdlt->pnmdlt)
                     pmdlt->ntrs = lntrs;
                  break;
               case 4:  /* SPMFTIMER */
                  RP->jspmftim = gettimno(NULL, TRUE);
                  RP0->n0flags |= N0F_SPMFTM;
                  break;

               } /* End kwret switch */

            /* If SPMFTIMER not entered, make it same as SPTIMER */
            if (!(RP0->n0flags & N0F_SPMFTM))
               RP->jspmftim = RP->jsptim;
            /* If in a test run, do one series of one trial */
            if (RP->CP.runflags & RP_TEST) {
               RP->nts = RP->ntr1 = RP->ntr2 = 1;
               RP->ntc1 = min(RP->ntc1,2);
               RP->ntc2 = min(RP->ntc2,2);
               }
            /* GCONN border-verification plot (currently only
            *  implemented in serial version) can only be done if
            *  superposition plots are turned off.  */
            if (RP0->kplt2 & PLEDG && RP0->kplt2 & (PLSUP|PLVTS)) {
               cryout(RK_P1, " -->WARNING: E option ignored, "
                  "incompatible with S or V.", RK_LN1, NULL);
               RP0->kplt2 &= ~PLEDG;
               }
            /* Transfer KP=U, NOUPDATE=A bits to runflags */
            RP->CP.runflags &= ~(RP_NOUPAS|RP_NOCYST);
            if (RP->kpsta & KPSUN) RP->CP.runflags |= RP_NOCYST;
            if (knoupas & 1) RP->CP.runflags |= RP_NOUPAS;

            if (iskip > 0) RP->CP.rpirec = RP0->rprecp*(iskip-1) + 1;
            if (!(RP->CP.runflags & RP_ONLINE) || !RK.iexit)
               return D3_NORMAL;
            } /* End CYCLE local scope */
            break;

/*---------------------------------------------------------------------*
*                          DETAIL PRINT card                           *
*  V8B, 12/31/00, GNR - Complete rewrite, use iteration lists          *
*  Rev, 07/21/01, GNR - Now also handle numbered cell lists            *
*  Rev, 05/04/02, GNR - Add DISABLE/ENABLE options                     *
*                                                                      *
*  When explicit cell lists are found, they are processed immediately. *
*  This gives better locality of error messages and less allocating of *
*  DPDATA blocks.  Processing of numbered cell lists must be deferred  *
*  to d3news() because the list may not yet have been entered when the *
*  reference to it is encountered.  The relevant parameters are stored *
*  temporarily in DPDATA blocks.  Explicit lists take precedence. This *
*  has the added benefit that we don't have to keep track of changes   *
*  in CLIST lists that may be interleaved with DETAIL PRINT cards.     *
*  OFF or PRINT options with no cell lists result in removing or       *
*  modifying all existing cell lists.  The keyword PRINT is accepted   *
*  without any codes as a syntax filler.                               *
*---------------------------------------------------------------------*/

         case DETAIL:  {
            /* Set all temp data fields as if nothing entered */
            struct DPDATA tdpd = { NULL, 0, 0, 0, -1, -1, 0 };
            struct DPDATA *pdpd,**ppdpd;
            struct CLHDR  *pclh;
            struct CELLTYPE *il;
            int nocl = TRUE;        /* TRUE when no lists found */
            int tdprm;              /* TRUE to remove CLIST ref */

#define DTLNUM 6     /* Number of DETAIL keys */
            static char *dtl[] = { "CLIST", "PRINT", "TIMER",
               "DISABLE", "ENABLE", "OFF" };

            cdscan(card, 1, SCANLEN, RK_WDSKIP);

            while ((smret = match(RK_NMERR, RK_MNEWK,
                  ~(RK_COMMA|RK_EQUALS|RK_INPARENS|RK_LFTPAREN),
                  0, dtl, DTLNUM)) != RK_MENDC) {

               /* If in parens, this has to be a cell list.  Reset
               *  smret so that if a region name happens to be the
               *  same as one of the recognized keywords, we will
               *  treat it as a cell list and not as one of those
               *  keywords.  */
               if (RK.scancode & RK_INPARENS) smret = 0;

               switch (smret) {

               case 6:           /* OFF */
                  /* Peek ahead.  If there are no more fields,
                  *  remove references to all cell lists.  */
                  if (scan(lea, 0) & RK_ENDCARD) {
                     /* First zero out the ctclid's (which will
                     *  prompt d3news() to kill the dpclb's) and
                     *  any explicit lists that they refer to.  */
                     d3clrx(CTCL_DPRINT);
                     /* Then delete all the DPDATA (CLIST refs) */
                     for (pdpd=RP0->pdpd1; pdpd; ) {
                        struct DPDATA *nextdpd = pdpd->pndpd;
                        freev(pdpd, "Detail print data");
                        pdpd = nextdpd;
                        }
                     RP0->pdpd1 = NULL;
                     goto NEXT_CARD;
                     }
                  /* Otherwise, drop through to case 0 ... */

               case 0:           /* Possible explicit cell list */
                  scanagn();
                  if (tdpd.clnum >= 0) tdpd.clnum = --RP0->nextclid;
                  pclh = d3clst(tdpd.clnum, CLKM_INDIV);
                  if (pclh) {
                     d3clck(pclh, &tdpd, CLCK_DPRINT);
                     nocl = FALSE;
                     }
                  continue;

               case 1:           /* Reference to numbered CLIST */
                  tdprm = getclid(&tdpd.clnum, NULL);
                  /* Find any existing DPDATA block for this clid */
                  for (ppdpd=&RP0->pdpd1;
                        pdpd=*ppdpd;   /* Assignment intended */
                        ppdpd=&(*ppdpd)->pndpd) {
                     if (pdpd->clnum == tdpd.clnum) {
                        /* Found it.  Remove if requested.
                        *  (d3news() will remove pointer to it.)  */
                        if (tdprm) {
                           *ppdpd = pdpd->pndpd;
                           freev(pdpd, "Detail print data");
                           goto NextDPField;
                           }
                        goto GotDPDATA;
                        }
                     }
                  /* Not found. Warning if OFF, otherwise make one */
                  if (tdprm) {
                     cryout(RK_P1, "0-->WARNING:  Previous reference "
                        "not found for CLIST OFF--deletion ignored.",
                        RK_LN2, NULL);
                     continue;
                     }
                  pdpd = *ppdpd = mallocv(sizeof(struct DPDATA),
                     "Detail print data");
                  pdpd->pndpd = NULL;
                  /* Store parameters for this clid in DPDATA.
                  *  (but preserve value of pdpd->pndpd).  */
GotDPDATA:        tdpd.pndpd = pdpd->pndpd;
                  *pdpd = tdpd;
                  nocl = FALSE;
                  continue;

               case 2:           /* PRINT item codes */
                  if (RK.scancode & RK_EQUALS) {
                     scanck(lea, RK_REQFLD|RK_PMEQPM,
                        ~(RK_COMMA|RK_LFTPAREN));
                     mcodes(lea, "ZVOYNAPDBRSMLC", NULL);
                     tdpd.dpitem = RK.mcbits;
                     tdpd.mckpm  = RK.mckpm;
                     }
                  continue;         /* Look for another key word */

               case 3:           /* TIMER */
                  tdpd.jdpt = (short)gettimno(NULL, YES);
                  continue;         /* Look for another key word */

               case 4:           /* DISABLE */
                  tdpd.dpdis = TRUE;
                  continue;

               case 5:           /* ENABLE */
                  tdpd.dpdis = FALSE;
                  continue;

               default:          /* Probably a punctuation error */
                  goto NEXT_CARD;

                  } /* End keyword switch */

NextDPField:   ;
               } /* End field scan */

            /* If no cell lists were found, propagate last item keys
            *  (still in RK.mcbits), timer numbers, and enable status
            *  to all layers and all private DPDATA blocks. */
            if (nocl) {
               for (pdpd=RP0->pdpd1; pdpd; pdpd=pdpd->pndpd) {
                  if (tdpd.jdpt > 0)
                     pdpd->jdpt = tdpd.jdpt;
                  if (tdpd.mckpm >= 0)
                     mcodopl(&pdpd->dpitem);
                  if (tdpd.dpdis >= 0)
                     pdpd->dpdis = tdpd.dpdis;
                  } /* End DPDATA loop */
               for (il=RP->pfct; il; il=il->pnct) {
                  if (il->ctclid[CTCL_DPRINT]) {
                     if (tdpd.jdpt > 0)
                        il->jdptim = tdpd.jdpt;
                     if (tdpd.mckpm >= 0)
                        mcodoph(&il->dpitem);
                     if (tdpd.dpdis >= 0) {
                        if (tdpd.dpdis) il->ctf |= CTDPDIS;
                        else            il->ctf &= ~CTDPDIS;
                        }
                     } /* End CTCL_PRINT if */
                  } /* End layer loop */
               } /* End nocl if */
            } /* End DETAIL local scope */
            break;

/*---------------------------------------------------------------------*
*                            EFFECTOR card                             *
*---------------------------------------------------------------------*/

         case EFFECTOR: {
            struct EFFARB *pea;     /* Ptr to EFFARB block */
            ui32   ic;              /* For kwscan() */
            int    iei;             /* Index over EXCIT, INHIB */
            int    ij;              /* Joint number */
            int    kp;              /* Protocol type */
            txtid  heffh = 0;       /* Handle to host */
            static char *eimsg[NEXIN] =
               { "EXCIT cells", "INHIB cells" };
            static char *g3ekeys1[] = { "ARM", "WINDOW",
#ifdef BBDS
               "BBD"
#endif
               };
            static char *g3ekeys2[] = { "JOINT", "UJOINT" };

            /* Read protocol name and switch */
            cdscan(RK.last, 1, DFLT_MAXLEN, RK_WDSKIP);
            kp = match(0, RK_MREQF, ~RK_COMMA, 0, g3ekeys1,
               sizeof(g3ekeys1)/sizeof(char *));
            switch (kp) {
            default:          /* Not ARM,WINDOW,BBD */
               ermark(RK_IDERR);
               goto NEXT_CARD;
            case 1:           /* EFFECTOR ARM ia */
               if (eqscan(&heffh, "V>IH", 0)) goto NEXT_CARD;
               kp = match(0, RK_MREQF, ~RK_COMMA, 0, g3ekeys2, 2);
               switch (kp) {
               default:       /* Not JOINT or UJOINT */
                  ermark(RK_IDERR);
                  goto NEXT_CARD;
               case 1:        /* EFFECTOR ARM ia JOINT ij */
                  if (eqscan(&ij, "V>IH", 0)) goto NEXT_CARD;
                  heffh |= ij << BITSPERBYTE;
                  kp = KEF_ARM;
                  break;
               case 2:        /* EFFECTOR ARM ia UJOINT */
                  kp = KEF_UJ;
                  break;
                  } /* End inner JOINT switch */
               break;
            case 2:           /* EFFECTOR WINDOW iw */
               if (eqscan(&heffh, "V>IH", 0)) goto NEXT_CARD;
               kp = KEF_WDW;
               break;
#ifdef BBDS
            case 3:           /* EFFECTOR BBD name */
               if (eqscan(&heffh, "TH" qMDNL, 0)) goto NEXT_CARD;
               kp = KEF_BBD;
               break;
#endif
               } /* End outer device type switch */

            /* Locate the specified EFFARB block */
            for (pea=RP0->pearb1; pea; pea=pea->pneb) {
               if ((int)pea->keff != kp) continue;
               if (pea->heffnm == heffh) goto EFF_READ_OPTS;
               }
            cryout(RK_E1, "0***SPECIFIED EFFECTOR "
               "NOT FOUND", RK_LN2, NULL);
            goto NEXT_CARD;

            /* Read keyword fields */
EFF_READ_OPTS:
            ic = 0;
            kwscan(&ic, "%%J" KWJ_GARB "E3", pea, NULL);

            /* Check that cell lists are still valid.  (For
            *  simplicity, do check even if there was no change.)  */
            for (iei=EXCIT; iei<NEXIN; ++iei) {
               if (pea->pefil[iei] && pea->pect[iei])
                  ilstchk(pea->pefil[iei], pea->pect[iei]->nelt,
                     eimsg[iei]);
               }
            break;
            } /* End EFFECTOR local scope */

/*---------------------------------------------------------------------*
*                             ENVVAL card                              *
*---------------------------------------------------------------------*/

         case ENVVAL: {
            short *tv, *tval = NULL;
            int ndsi = (int)RP->mxnds;

            /* Find out whether there is anyplace to store ENVVAL */
            if (ndsi <= 0) {
               cryout(RK_E1, "0***A MODALITY OR VALUE SCHEME IS"
                  " MISSING FOR THIS ENVVAL.", RK_LN2, NULL);
               goto NEXT_CARD;
               }

            /* Find out whether card has any data on it */
            cdscan(RK.last, 1, SCANLEN, RK_WDSKIP);
            if (!(scan(lea, OFF) & RK_ENDCARD)) {
               scanagn();     /* Push first number back */
               /* Allocate temporary space for input values */
               tval = (short *)mallocv(ndsi*sizeof(short),
                  "envval temp");
               /* Set all to 0.5 (S8) in case too few specified */
               for (tv=tval; tv<tval+ndsi; ) *tv++ = SFIXP(0.5, 8);
               /* Scan the card, storing up to mxnds values */
               inform("(SNRB8IH)", &ndsi, tval, NULL);
               thatsall();
               } /* End reading data */

            /* Add to or remove from relevant modality blocks */
            for (pmdlt=RP->pmdlt1; pmdlt; pmdlt=pmdlt->pnmdlt) {
               /* Skip modality if not named on MODALITY card */
               if (!bittst((unsigned char *)&wmdlts,
                  (long)pmdlt->mdltno)) continue;

               /* If ENVVAL card had data on it, copy to modality */
               if (tval) {
                  int ndsi = (int)pmdlt->nds * sizeof(short);
                  /* If no ENVVAL list, make one now */
                  if (!pmdlt->penvl) pmdlt->penvl =
                     (short *)mallocv(ndsi, "envval array");
                  /* Copy, ignore extra entries with no warning. */
                  memcpy((char *)pmdlt->penvl, (char *)tval, ndsi);
                  }

               /* Otherwise, delete any existing penvl data */
               else if (pmdlt->penvl) {
                  free(pmdlt->penvl); pmdlt->penvl = NULL; }
               } /* End modality scan */

            /* Release temp storage */
            if (tval) free(tval);

            } /* End ENVVAL local scope */
            break;

/*---------------------------------------------------------------------*
*                            ESJPLOT card                              *
*---------------------------------------------------------------------*/

         case ESJPLOT:
            getijpl(KCIJES);
            break;

/*---------------------------------------------------------------------*
*                             FACTOR card                              *
*---------------------------------------------------------------------*/

         case FACTOR:
            inform("(SW1,VF)", &RP0->pfac, NULL);
            factor(RP0->pfac);
            thatsall();
            break;

/*---------------------------------------------------------------------*
*                             GRPNO card                               *
*                                                                      *
*  Revised in V3C to permit a linked list of GRPNO blocks to be con-   *
*  structed, giving cross-reaction stats for several classifications   *
*  of the stimuli in a single run.                                     *
*  Further revised in V8A to allow each GRPNO card to apply to one or  *
*  more stimulus modalities, as given on previous MODALITY card.       *
*  Rev, 07/27/10, GNR - Deleted code to zero ngpnt--done in d3msgl     *
*---------------------------------------------------------------------*/

         case GRPNO:  {
            struct GRPDEF *Gptr;
            short *tgrp,*pg;
            int ndsi = (int)RP->mxnds;

            /* Allocate temporary space for group number list */
            tgrp = (short *)mallocv(ndsi*sizeof(short), "GRPNO temp");

            /* Set all stimuli to class 1 in case too few specified */
            for (pg=tgrp; pg<tgrp+ndsi; ) *pg++ = 1;

            /* Scan the card, storing up to ndsi group numbers */
            inform("(SW1NRVIH)", &ndsi, tgrp, NULL);
            thatsall();

            /* For each modality to which this GRPNO card applies,
            *  copy the group number list into its GRPDEF list.
            *  Card is syntax-checked but data not stored if no
            *  X, G, or C statistics in this run.  */
            if (RP->kxr & (XSTATS|GSTATS|CSTATS))
                  for (pmdlt=RP->pmdlt1; pmdlt; pmdlt=pmdlt->pnmdlt) {

               /* Skip modality if not named on MODALITY card */
               if (!bittst((unsigned char *)&wmdlts,
                  (long)pmdlt->mdltno)) continue;

               /* If existing linked list exhausted, extend it */
               ndsi = (int)pmdlt->nds;
               Gptr = *pmdlt->um1.ppgpno;
               if (!Gptr) Gptr = *pmdlt->um1.ppgpno = mkgrpdef(ndsi);

               /* Move list to array dangling off GRPDEF struct */
               memcpy((char *)Gptr->grp,
                  (char *)tgrp, ndsi*sizeof(short));
               pmdlt->um1.ppgpno = &Gptr->pnxgrp;
               ++pmdlt->ngpnt;

               } /* End modality scan */

            /* Release temp list storage */
            free(tgrp);

            } /* End GRPNO local scope */
            break;

/*---------------------------------------------------------------------*
*                            IMGTOUCH card                             *
*---------------------------------------------------------------------*/

         case IMGTOUCH:
            getitp();
            break;

/*---------------------------------------------------------------------*
*                             INPUT card                               *
*---------------------------------------------------------------------*/

         case INPUT:
            cdscan(card, 1, SCANLEN, RK_WDSKIP);
            kwscan(&ic, "LO%UC", &RP0->eilo, "HI%UC", &RP0->eihi, NULL);
            break;

/*---------------------------------------------------------------------*
*                              ION card                                *
*---------------------------------------------------------------------*/

         case IONCD:
            getion(NULL, NULL);
            break;

/*---------------------------------------------------------------------*
*                            MIJPLOT card                              *
*---------------------------------------------------------------------*/

         case MIJPLOT:
            getijpl(KCIJM);
            break;

/*---------------------------------------------------------------------*
*                            MODALITY card                             *
*                                                                      *
*  N.B.  More than one MODALITY block with the same name may exist     *
*  due to the existence of submodalities visible only through some     *
*  particular window.  All of these must be marked when the name is    *
*  matched.  The switch on the kwscan is because all values of ntcs,   *
*  ntrs are legal (negatives cancel previous options), therefore,      *
*  there is no value that can be used to flag option found.            *
*  N.B.  Negative inputs to ntcs,ntrs become very large ui16 values,   *
*  effectively disabling the tests in d3go().                          *
*---------------------------------------------------------------------*/

         case MODALTY:  {
            int   matched;
            txtid hmname;
            ui16  lntcs;
            ui16  lntrs;
            char  mname[LTEXTF+1];

            wmdlts = 0;             /* Clear modality list */
            cdscan(card, 1, LTEXTF, RK_WDSKIP);
            while ((scan(mname, 0) & ~(RK_BLANK|RK_COMMA)) == 0) {
               matched = FALSE;
               /* We don't want to use findmdlt() here, because there
               *  may be multiple modalities with the same name.  */
               hmname = findtxt(mname);
               for (pmdlt=RP->pmdlt1; pmdlt; pmdlt=pmdlt->pnmdlt)
                     if (hmname == pmdlt->hmdltname) {
                  bitset((unsigned char *)&wmdlts, (long)pmdlt->mdltno);
                  matched = TRUE;
                  }
               if (!matched) {
                  ermark(RK_MARKFLD);
                  cryout(RK_P1, "0***\"", RK_LN2, mname, LTEXTF,
                     "\" IS NOT A RECOGNIZED MODALITY.", RK_CCL, NULL);
                  }
               } /* End positional scan */
            if (RK.scancode == RK_EQUALS) {
               scanagn();
               while (kwret = kwscan(&ic,    /* Assignment intended */
                     "NTCS%X",
                     "NTRS%X", NULL)) switch (kwret) {
                  case 1:  /* NTCS */
                     eqscan(&lntcs, "IH", RK_EQCHK);
                     for (pmdlt=RP->pmdlt1; pmdlt; pmdlt=pmdlt->pnmdlt)
                        if (bittst((unsigned char *)&wmdlts,
                           (long)pmdlt->mdltno)) pmdlt->ntcs = lntcs;
                     break;
                  case 2:  /* NTRS */
                     eqscan(&lntrs, "IH", RK_EQCHK);
                     for (pmdlt=RP->pmdlt1; pmdlt; pmdlt=pmdlt->pnmdlt)
                        if (bittst((unsigned char *)&wmdlts,
                           (long)pmdlt->mdltno)) pmdlt->ntrs = lntrs;
                     break;
                  } /* End kwret switch */
               } /* End keyword scan */
            if (!(RK.scancode & RK_ENDCARD)) {
               ermark(RK_TOOMANY);
               goto NEXT_CARD;
               }
            } /* End MODALTY local scope */
            break;

/*---------------------------------------------------------------------*
*                             MOVIE card                               *
*  N.B.  Don't allow this card to override a command-line movie mode   *
*  parameter until after the first trial series--main program clears   *
*  N0F_MPARME bit to allow this.                                       *
*---------------------------------------------------------------------*/

         case MOVIE:
            cdscan(card, 1, SCANLEN, RK_WDSKIP);
            while (kwret = kwscan(&ic,    /* Assignment intended */
                  "NEXP%vih",  &RP0->nexp,
                  "MATRIX%X",
                  "BEEPER%X",
                  "BMODE%X",
                  "MMODE%X",
                  "SMODE%X",
                  "TIMER%N" KWS_TIMNW, &RP->jmvtim,
                  NULL)) switch (kwret) {
               case 1:  /* MATRIX device */
                  RP0->mmdev = MD_MATRIX;
                  break;
               case 2:  /* BEEPER device */
                  RP0->mmdev = MD_BEEPER;
                  break;
               case 3:  /* BATCH mode */
                  if (!(RP0->n0flags & N0F_MPARME))
                     RP0->mmode = MM_BATCH;
                  break;
               case 4:  /* MOVIE mode */
                  if (!(RP0->n0flags & N0F_MPARME))
                     RP0->mmode = MM_MOVIE;
                  break;
               case 5:  /* STILL mode */
                  if (!(RP0->n0flags & N0F_MPARME))
                     RP0->mmode = MM_STILL;
                  break;
               } /* End kwret switch and while */
            break;

/*---------------------------------------------------------------------*
*                           PAF or PCF card                            *
*---------------------------------------------------------------------*/

         case PAF:
         case PCF:  {
            struct PCF *ppcf;    /* Ptr to table to be modified */
            int hpcfn;           /* Locator for name of table */

            inform("(SW1T" qLTF ")", &hpcfn, NULL);
            ppcf = findpcf(hpcfn);
            if (ppcf)
               getpcf(ppcf, (ccindex == PCF) ? PCFN_MAX : PCFN_NONE);
            } /* End PAF/PCF local scope */
            break;

/*---------------------------------------------------------------------*
*                             PLABEL card                              *
*---------------------------------------------------------------------*/

         case PLTLAB:
            getplab();
            break;

/*---------------------------------------------------------------------*
*                            PLIMITS card                              *
*---------------------------------------------------------------------*/

         case PLTLIMS: {
#define NG3PLOP 6
            static char *g3plop[NG3PLOP] = { "RETRACE", "NORETRACE",
               "SQUARE", "NOSQUARE", "STDLHT", "GRIDS" };

            cdscan(RK.last, 1, SCANLEN, RK_WDSKIP);
            while ((smret = match(OFF, RK_MSCAN, ~(RK_COMMA|RK_EQUALS),
                  0, g3plop, NG3PLOP)) != RK_MENDC) switch (smret) {

               case 1:        /* RETRACE */
                  RP0->kplt1 |= PLRTR;
                  break;
               case 2:        /* NORETRACE */
                  RP0->kplt1 &= ~PLRTR;
                  break;
               case 3:        /* SQUARE */
                  RP0->kplt1 |= PLSQR;
                  break;
               case 4:        /* NOSQUARE */
                  RP0->kplt1 &= ~PLSQR;
                  break;
               case 5:        /* STDLHT */
                  /* For historical reasons, this option may have
                  *  no parameter, in which case there's nothing
                  *  to do (use of stdlht is now always enabled) */
                  if (RK.scancode == RK_EQUALS)
                     eqscan(&RP->stdlht, "VF", 0);
                  break;
               case 6:        /* GRIDS */
                  eqscan(&RP0->grids, "VIH", RK_BEQCK);
                  break;
               }  /* End smret switch and while */
            } /* End PLIMITS local scope */
            break;

/*---------------------------------------------------------------------*
*                             PLINE card                               *
*---------------------------------------------------------------------*/

         case PLTLIN:
            getpline();
            break;

/*---------------------------------------------------------------------*
*                              PLOC card                               *
*---------------------------------------------------------------------*/

         case PLOC:
            getploc();
            break;

/*---------------------------------------------------------------------*
*                              PLOT card                               *
*                                                                      *
*  This is for compatibility--replaced by generic TIMER card.  User    *
*  must add parentheses around stimulus and EVERY list components.     *
*---------------------------------------------------------------------*/

         case PLOT:
            cdscan(card, 1, SCANLEN, RK_WDSKIP);
            d3g3timr(wmdlts, PLTRT);
            break;

/*---------------------------------------------------------------------*
*                            PPFPLOT card                              *
*---------------------------------------------------------------------*/

         case PPFPLOT:
            getijpl(KCIJPPF);
            break;

/*---------------------------------------------------------------------*
*                             PRINT card                               *
*                                                                      *
*  This is for compatibility--replaced by generic TIMER card.  User    *
*  must add parentheses around stimulus and EVERY list components.     *
*---------------------------------------------------------------------*/

         case PRINT:
            cdscan(card, 1, SCANLEN, RK_WDSKIP);
            d3g3timr(wmdlts, PRTRT);
            break;

/*---------------------------------------------------------------------*
*                             PROBE card                               *
*                                                                      *
*  V8B, 03/10/02, GNR - Broken out as getprobe() to file d3g3prob.c    *
*---------------------------------------------------------------------*/

         case PROBE:
            getprobe();
            break;

/*---------------------------------------------------------------------*
*                             PSCALE card                              *
*---------------------------------------------------------------------*/

         case PSCALE:
            gtpscale();       /* Now handled in d3g1g3.c */
            break;

/*---------------------------------------------------------------------*
*                               PX card                                *
*---------------------------------------------------------------------*/

         case PX: /* Needed when replay reaches end of input */
            if (RP->CP.runflags & RP_REPLAY) {
               long iskip = 0;
               inform("(SW1NIL)", &iskip, NULL);
               if (jfind(card, "+", 2) || jfind(card, "-", 2))
                  RP->CP.rpirec += RP0->rprecp*(--iskip);
               else RP->CP.rpirec = 1 + RP0->rprecp*(--iskip);
               }
            if (!(RP->CP.runflags & RP_ONLINE) || !RK.iexit)
               return D3_NORMAL;
            break;

/*---------------------------------------------------------------------*
*                          REGENERATION card                           *
*---------------------------------------------------------------------*/

         case REGENR:
            gtkregen();
            break;

/*---------------------------------------------------------------------*
*                             RESET card                               *
*---------------------------------------------------------------------*/

         case RESET:
            d3g3rset();
            break;

/*---------------------------------------------------------------------*
*                       RESPONSE FUNCTION card                         *
*  Rev, 10/20/07, GNR - Eliminate changing of STEP/KNEE--logic would   *
*                       be too complex to handle SPIKE and TANH also.  *
*---------------------------------------------------------------------*/

         case RESPFN: {
#define RFNUM 3      /* Number of response functions */
            static char *rf[] =
               { "ADEFER", "DELAY", "IMMEDIATE" } ;

            cdscan(card, 2, SCANLEN, RK_WDSKIP);
            while ((smret = match(OFF, RK_MSCAN, ~RK_COMMA, 0,
                  rf, RFNUM)) != RK_MENDC)
               switch (smret) {
               case 1:  /* ADEFER */
                  eqscan(&RP->adefer, "UH", RK_EQCHK); break;
               case 2:  /* DELAY */
                  RP->adefer = 1; break;
               case 3:  /* IMMEDIATE */
                  RP->adefer = 0; break;
                  }
               } /* End RESPFN local scope */
            break;

/*---------------------------------------------------------------------*
*                             RJPLOT card                              *
*---------------------------------------------------------------------*/

         case RJPLOT:
            getijpl(KCIJR);
            break;

/*---------------------------------------------------------------------*
*                              SAVE card                               *
*---------------------------------------------------------------------*/

         case SAVE:
            getsave();
            break;

/*---------------------------------------------------------------------*
*                             SENSE card                               *
*---------------------------------------------------------------------*/

         case SENSE: {
            struct VCELL *pvc;
            char vsrcnm[LSNAME];
            int ivctyp = 1;         /* Default sense index */
            inform("(SW1A" qLSN ")", vsrcnm, NULL);   /* Get name */
#ifdef BBDS
            /* If type is "BBD", get handle on name */
            if (ssmatch(vsrcnm, "BBD", 3))
               inform("(S,T" qLTF ")", &ivctyp, NULL);
            else
#endif
            /* Interpret index parameter "(i)" if present */
            if (RK.scancode & RK_LFTPAREN)
               inform("(S,V(I4V))", &ivctyp, NULL);

            /* Attempt to locate the VCELL block */
            pvc = findvc(vsrcnm, 0, ivctyp,
               "FOR GROUP III SENSE CARD.");
            if (!pvc) break;     /* No can do */
            /* Interpret keyword parameters */
            if (pvc) kwscan(&ic,
               "VMIN%F",   &pvc->vcmin,
               "VMAX%F",   &pvc->vcmax,
               "X%VF",     &pvc->vcplx,
               "Y%VF",     &pvc->vcply,
               "W%VF",     &pvc->vcplw,
               "H%VF",     &pvc->vcplh,
               "KP%KHNIOKQ2GB",  &pvc->kvp,
               "CPT%B14IH",      &pvc->vcpt,
               "HWIDTH%VF",      &pvc->vhw,
               NULL);
            /* Can turn off B but can turn it on only if VKUSED bit
            *  is set, indicating memory allocation in place.  */
            if (!(pvc->vcflags & VKUSED)) pvc->kvp &= ~VKPPL;

            } /* End SENSE local scope */
            break;

/*---------------------------------------------------------------------*
*                             SJPLOT card                              *
*---------------------------------------------------------------------*/

         case SJPLOT:
            getijpl(KCIJS);
            break;

/*---------------------------------------------------------------------*
*                             SNOISE card                              *
*---------------------------------------------------------------------*/

         case SNOISE: {
            /* The following is a little intricate because the snfr
            *  and snseed fields are both optional.  Therefore, if the
            *  user supplies three values on the SNOISE card, the third
            *  field may be either the snfr or the snseed value.  Hence
            *  acrobatics with the char buffer, etc. */
            char snfrbuf[SCANLEN1]; /* Buffer for snfr or snseed val */
            long seedbuf = -1L;     /* Binary buf for seed val */
            long msd;

            /* Clear SNFRBUF before inform */
            snfrbuf[0] = '\0';
            inform("(SW1NVB24IJ,VB28IJ,A" qDML ",V>IL)",
               &RP->snmn, &RP->snsg, snfrbuf, &seedbuf, NULL);
            if (snfrbuf[0] != '\0') {
               wbcdin(snfrbuf, &msd,
                  RK_CTST+RK_QPOS+RK_IORF+RK_NLONG+SCANLEN-1);
               if (msd > 1) {
                  RP->snseed = msd;
                  if (seedbuf != -1) ermark(RK_TOOMANY);
                  }
               else {
                  wbcdin(snfrbuf, &RP->snfr, 24*RK_BSCL+
                     RK_CTST+RK_QPOS+RK_IORF+RK_NI32+SCANLEN-1);
                  if (seedbuf > 0) RP->snseed = seedbuf;
                  }
               }

            kwscan(&ic, "MEAN%VB24IJ", &RP->snmn,
                        "SIGMA%VB28IJ", &RP->snsg,
                        "FRAC%VB24IJ", &RP->snfr,
                        "SEED%V>IL", &RP->snseed,
                        NULL);
            } /* End SNOISE local scope */
            break;

/*---------------------------------------------------------------------*
*                      STATISTICS or STATS card                        *
*---------------------------------------------------------------------*/

         case STATS1:
         case STATS2:
            getstat();
            break;

/*---------------------------------------------------------------------*
*                             TIMER card                               *
*---------------------------------------------------------------------*/

         case TIMER:
            cdscan(card, 1, SCANLEN, RK_WDSKIP);
            scanck(lea, RK_REQFLD, ~(RK_BLANK|RK_COMMA));
            d3g3timr(wmdlts, lea);
            break;

/*---------------------------------------------------------------------*
*                          CAMERA or TV card                           *
*                                                                      *
*  In Group III, all that can be done is to modify plotting options    *
*  for a camera that was already defined in Group I.                   *
*---------------------------------------------------------------------*/

         case CAMERA:
         case TV: {
            struct TVDEF *cam;
            int   kwret;
            txtid thcamnm;

            /* Following code is designed to allow input for any of
            *  UPGM, VVTV or BBD.  First positional parameter is
            *  interface type, second is camera name.  Interface
            *  type is not checked against TVDEF block, because
            *  all interfaces share same camera namespace.  */
            cdscan(RK.last, 1, SCANLEN, RK_WDSKIP);
            scanck(lea, RK_REQFLD, ~RK_BLANK);
            if (ssmatch(lea, "UPGM", 1)) goto ReadTVOptions;
#ifdef VVTV
            if (ssmatch(lea, "VVTV", 1)) goto ReadTVOptions;
#endif
#ifdef BBDS
            if (ssmatch(lea, "BBD", 1)) goto ReadTVOptions;
#endif
            cryout(RK_E1, "0***CAMERA or TV INTERFACE NOT "
               "RECOGNIZED.", RK_LN2, NULL);
            goto NEXT_CARD;

ReadTVOptions:
            /* Get name of camera and find correct TVDEF */
            inform("(S,TH" qTV_MAXNM ")", &thcamnm, NULL);
            cam = findtv(thcamnm, NULL);
            if (!cam) goto NEXT_CARD;

            /* Read TV options */
            while (kwret = kwscan(&ic, /* Assignment intended */
                  "UDATA%X",
                  "X%VF",        &cam->tvplx,
                  "TVPLX%VF",    &cam->tvplx,
                  "Y%VF",        &cam->tvply,
                  "TVPLY%VF",    &cam->tvply,
                  "W%VF",        &cam->tvwd,
                  "WIDTH%V>F",   &cam->tvwd,
                  NULL)) {
               switch (kwret) {
               case 1:           /* UDATA */
                  if ((cam->utv.tvsflgs & (TV_ON|TV_UPGM)) ==
                        (TV_ON|TV_UPGM)) {
                     int uirc;
                     txtid thudata;
                     eqscan(&thudata, "TH" qTV_MAXUD, RK_EQCHK);
                     uirc = cam->tvinitfn(&cam->utv,
                        getrktxt(cam->hcamnm),
                        cam->hcamuf ? getrktxt(cam->hcamuf) : NULL,
                        getrktxt(thudata), TV_RESTART);
                     if (uirc) {
                        convrt("(P1,'0***TVINIT FUNCTION RETURNED ERROR "
                           "CODE ',J0I6)", &uirc, NULL);
                        RK.iexit |= 1; }
                     }
                  else {
                     ermark(RK_IDERR);
                     scan(NULL, 0);
                     }
                  break;
                  } /* End kwret switch */
               } /* End kwret while */

            } /* End TV local scope */
            break;

/*---------------------------------------------------------------------*
*                             VALUE card                               *
*  Ability to change type and source of value removed, 02/15/92, GNR   *
*  This takes a lot of code space and seems not in the spirit of CNS.  *
*  NOPLOT removed and VOP added, 09/23/01, GNR - NOPLOT could be con-  *
*  fused with valnm args in d3grp1(), VOP options do not affect memory *
*  allocation and give a way to control plotting.                      *
*---------------------------------------------------------------------*/

         case VALUE: {
            struct VBDEF *ivb = RP->pvblk;
            int vindex,i,kv;
            if (RP->nvblk <= 0) goto VI_ERR;
            cdscan(card, 1, SCANLEN, RK_WDSKIP);

            /* Check for value index in parens and advance ivb */
            if (scan(lea,RK_REQFLD) & RK_ENDCARD)
               goto NEXT_CARD;
            if (RK.scancode == RK_INPARENS+RK_RTPAREN) {
               vindex = ibcdin(STR_TO_INT, lea);
               for (i=1; ivb && i<vindex; i++,ivb=ivb->pnxvb) ;
               if (!ivb) goto VI_ERR;
               }
            /* If it's not value index, push it back */
            else scanagn();

            /* Read keyword parameters */
            kv = ivb->kval;
            kwscan(&ic,
                  "SCALE%B24IL",       &ivb->vscl,
                  "DAMPFAC%V<=1B15UH", &ivb->vdamp,
                  "VDAMP%V<=1B15UH",   &ivb->vdamp,
                  "VDELAY%UH",         &ivb->vdelay,
                  "BASE%B8IL",         &ivb->vbase,
                  "BEST%RVIL", (int)(ivb->kval<=VB_FIRE), &ivb->vbest,
                  "HWIDTH%RVIL",(int)(ivb->kval<=VB_UJMD), &ivb->vhalf,
                  "MIN%B8IL",          &ivb->vmin,
                  "VOP%KC" okvalue,    &ivb->vbopt,
                  NULL);

            /* Set ivb->u1 according to type of value */
            if (ivb->vbkid == VBK_REP)          /* Repertoire type */
               ivb->u1.vrp.omvbd2 = (S8 - ivb->vbase)>>1;
            else if (ivb->vbkid == VBK_DIST) {  /* Distance type */
               float vhf = (float)ivb->vhalf;
               ivb->u1.vhh.vradm2 = ((kv==VB_DLIN || kv==VB_RLIN) ?
                  -0.5/vhf : -1.0/(vhf*vhf));
               }

            } /* End VALUE local scope */
            break;

/*---------------------------------------------------------------------*
*                      Unidentified control card                       *
*---------------------------------------------------------------------*/

         default:
            ermark(RK_CARDERR);
            break;

            } /* End ccindex switch */
         } /* End ENDCHK else */

      continue;

VI_ERR:  cryout(RK_E1, "0***VI > NUM VALUE CARDS", RK_LN2, NULL);
      skip2end();
      continue;

NEXT_CARD:
      skip2end();
      } /* End big while */

/* Determine return type */

   if (endchk == END_HX) return OUT_QUICK_EXIT;
   else                  return OUT_USER_QUIT;
   } /* End D3GRP3 */

