/* (c) Copyright 1988-2018, The Rockefeller University *21116* */
/* $Id: d3grp2.c 78 2018-08-02 18:36:58Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                      d3grp2 (formerly d3tree)                        *
*  Read group II input and construct dynamic repertoire tree           *
*                                                                      *
************************************************************************
*  Written by  G. N. Reeke - 08/30/85                                  *
*  V4A, 07/30/88, SCD/JWT - Translated to 'C' from version V3B         *
*  Rev, 03/22/90, GNR - Removed mean prev response DECAY (kind = 'M')  *
*  Rev, 01/xx/91,  MC - Added PHASE code                               *
*  V5C, 11/12/91, GNR - Added Darwin 1 and new save/restore codes      *
*  V5E, 07/07/92, GNR - Eliminate length err when repname > 16 chars   *
*  Rev, 07/25/92, GNR - New controls for Cij0, mtj to Cn, KGNLM        *
*  Rev, 10/11/92, GNR - Add KGEN=H, PARAMS OPT=T codes                 *
*  V5F, 10/30/92, GNR - Add MASTER, fix GCONN (sn,ln) bug              *
*  V6A, 03/01/93, GNR - Add subarbors                                  *
*  Rev, 03/26/93, GNR - Add KAMDR, KAMDQ, do not propagate kam bits    *
*                       to later connection types                      *
*  Rev, 12/10/93, GNR - Trap misplaced AMPLIF, DECAY cards             *
*  V6D, 01/04/94, GNR - Add GCONN self-avoidance, DELAY card, VXO,VYO, *
*                       propagate CELL OPT=O to CONN OPT=O, spikes     *
*  V7A, 04/30/94, GNR - Add user-defined senses                        *
*  V7B, 06/22/94, GNR - Add OPT=A on PARAMS card, forbid KGNLM+KGNHV,  *
*                       provide eight explicit amplification scales    *
*  Rev, 07/20/94, GNR - Add rsd, amp. rule L, ptscll, stscll, -ctscll  *
*  Rev, 07/28/94, GNR - Add cell OPT=R, connection BC and KGEN=V       *
*  Rev, 08/19/94, GNR - Add input for bilinear Cij maps, GCONN decay   *
*  V7C, 11/19/94, GNR - Add KRP=C,F stats, move stat alloc to d3allo   *
*  Rev, 01/28/95, GNR - Add sigmoid volt-dep conns and KGEN=Q          *
*  V8A, 04/15/95, GNR - Merge kchng,flags into WDWDEF, move getsnsid,  *
*                       handle VJ,VT,VW as VCELLs, hand vis as VH_SRC, *
*                       H,X,Y,Z,C,F,U stats to CELLTYPE, MODALITY to   *
*                       CONNTYPE, CELLTYPE decay, OPT=P sets kdcy,     *
*                       siet,siscl to DECAY card, FALLOFF D metric,    *
*                       individual square,shunt opts, add REFRACTORY   *
*                       and pht, delete "LTP", phase parms to PHASEDEF *
*  Rev, 10/07/96, GNR - Card processing removed to g2cell,g2conn       *
*  Rev, 09/04/97, GNR - Ignore whitespace cards                        *
*  Rev, 10/02/97, GNR - Allow AMPLIF card in CONNTYPE group            *
*  Rev, 01/03/98, GNR - Add PPF card                                   *
*  V8B, 12/09/00, GNR - Move to new memory management routines         *
*  V8C, 03/27/03, GNR - Cell s(i) in mV, kill INOISE                   *
*  V8D, 01/31/04, GNR - Add conductances and ions                      *
*  Rev, 01/27/07, GNR - Look to next group if unrecognized card        *
*  Rev, 04/07/07, GNR - Use readrlnm input style on GCONN card         *
*  Rev, 10/21/07, GNR - Add TANH, tiered threshold defaults            *
*  ==>, 01/05/08, GNR - Last mod before committing to svn repository   *
*  V8E, 01/14/09, GNR - Add IZHIKEVICH cards and response function     *
*  Rev, 09/01/09, GNR - Add BRETTE-GERSTNER card and response function *
*  V8G, 08/12/10, GNR - Add AUTOSCALE card                             *
*  Rev, 12/24/12, GNR - New overflow checking                          *
*  R65, 01/02/16, GNR - n[gq][xyg] to ui16, ngrp to ui32, ncpg to ubig *
*  R66, 02/17/16, GNR - Change long random seeds to typedef si32 orns  *
*  R75, 09/30/17, GNR - Add TERNARY card                               *
*  R78, 04/03/18, GNR - Add FADE-IN card                               *
***********************************************************************/

#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "d3global.h"
#include "d3opkeys.h"
#include "rocks.h"
#include "rkxtra.h"

extern char **getcc3(int *pncc3);

/* Prototypes for functions uniquely called from d3grp2 */
void g2rep(struct REPBLOCK *ir);
void g2cell(struct REPBLOCK *ir, struct CELLTYPE *il);
void g2gconn(struct CELLTYPE *il, struct INHIBBLK *ib);
void g2modul(struct CELLTYPE *il, struct MODBY *im);
void g2conn(struct CELLTYPE *il, struct CONNTYPE *ix);
void g2master(orns *inseed);
void g2autsc(struct AUTSCL *paut);
void g2refrac(struct CELLTYPE *il);
void g2seeds(struct CONNTYPE *ix);
void g2subarb(struct CONNTYPE *ix);
void g2vpfb(struct CONNTYPE *ix);

/*---------------------------------------------------------------------*
*  This little routine can be called to get a pointer to the Group 2   *
*  control card keys and the number of keys for use in d3grp1 look-    *
*  ahead.  It seems that just making cc2 global doesn't do the trick.  *
*---------------------------------------------------------------------*/

char **getcc2(int *pncc2) {

/* Keyword strings for and number of Group II control cards */

   static char *cc2[] = {
      "MASTER",
      "REGION",
      "REPERTOIRE",
      "CELLTYPE",
      "AMPLIFICATION",
      "AEIF",
      "AUTOSCALE",
      "CONDUCTANCE",
      "CSET",
      "DECAY",
      "DELAY",
      "DEPRESSION",
      "FADE-IN",
      "GCONN",
      "INHIBITION",
      "ION",
      "IZHI2003",
      "IZHI2007",
      "IZHI",           /* Synonyms for IZHI2007 */
      "IZHIKEVICH",
      "MODULATE",
      "NOISE",
      "NMOD",
      "PHASE",
      "PPF",
      "PAIRED",
      "REFRACTORY",
      "CONNTYPE",
      "PARAMS",
      "PARAMETERS",
      "SEEDS",
      "SUBARBOR",
      "TERNARY",
      "THRESHOLDS", };

   *pncc2 = sizeof(cc2)/sizeof(char *);
   return cc2;
   } /* End getcc2() */

/*---------------------------------------------------------------------*
*                              mkctphsd                                *
*                                                                      *
*  Make a PHASEDEF block and link it to a CELLTYPE block.  Copy the    *
*  global defaults to it and set the phshft and pdshft flags.  (Called *
*  when CELLTYPE card or PHASE card first encounters need for phase.)  *
*  Rev, 12/24/12, GNR - Set iovec for new overflow counting            *
*---------------------------------------------------------------------*/

static void mkctphsd(struct CELLTYPE *il) {
   il->pctpd = (struct PHASEDEF *)allocpcv(Static, 1,
      IXstr_PHASEDEF, "PHASEDEF structure");
   *il->pctpd = RP0->GPdD;
   il->pctpd->iovec = RP->novec++;
   il->phshft = 1;
   il->pdshft = PHASE_EXP;
   } /* End mkctphsd() */

/*---------------------------------------------------------------------*
*                               ckctrf                                 *
*                                                                      *
*  Check response function from CELLTYPE card.  This is called when an *
*  AEIF or IZHIKEVICH-type card is processed. If a response function   *
*  was already specified on the CELLTYPE card (but ignoring any from a *
*  MASTER CELLTYPE card), and it is different from the one requested   *
*  now, generate an error and return 1.  Otherwise, set the response   *
*  function to the requested value, indicate in krmop that it has been *
*  set, and return 0.                                                  *
*---------------------------------------------------------------------*/

static int ckctrf(struct CELLTYPE *il, int krf) {
   if (il->ctwk.tree.krmop & KRM_RFENT) {
      if ((int)il->Ct.rspmeth != krf) {
         ermark(RK_IDERR);
         cryout(RK_E1, "0***RESPONSE FUNCTION CONTRADICTS "
            "CELLTYPE CARD.", RK_LN2, NULL);
         return 1;
         }
      }
   else {
      il->Ct.rspmeth = (byte)krf;
      il->ctwk.tree.krmop |= KRM_RFENT;
      }
   return 0;
   } /* End ckctrf() */

/*---------------------------------------------------------------------*
*                               d3grp2                                 *
*                                                                      *
*  N.B.  Random number seeds are initialized here from 'inseed' to     *
*  avoid need to make inseed a global variable.                        *
*---------------------------------------------------------------------*/

void d3grp2() {

/* Group II control card switch cases--
*  The enum values must match the corresponding cc2 entries! */
   enum G2CC {
      MASTER_CARD=1,
      REGION_CARD,
      REPERTOIRE_CARD,
      CELLTYPE_CARD,
      AMPLIFICATION_CARD,
      AEIF_CARD,
      AUTOSC_CARD,
      CONDUCT_CARD,
      CSET_CARD,
      DECAY_CARD,
      DELAY_CARD,
      DEPRESSION_CARD,
      FADEIN_CARD,
      GCONN_CARD,
      INHIBITION_CARD,
      ION_CARD,
      IZHI2003_CARD,
      IZHI2007_CARD,
      IZHI_CARD,
      IZHIKEVICH_CARD,
      MODULATE_CARD,
      NOISE_CARD,
      NMOD_CARD,
      PHASE_CARD,
      PPF_CARD,
      PAIRED_CARD,
      REFRACTORY_CARD,
      CONNTYPE_CARD,
      PARAMS_CARD,
      PARAMETERS_CARD,
      SEEDS_CARD,
      SUBARBOR_CARD,
      TERNARY_CARD,
      THRESHOLDS_CARD,
      } cc2index;

   struct REPBLOCK *ir = NULL;         /* Non-NULL if there is a
                                          current REPERTOIRE card */
   struct CELLTYPE *il = NULL;         /* Non-NULL if there is a
                                          current CELLTYPE card */
   struct CONNTYPE *ix = NULL;         /* Non-NULL if there is a
                                          current CONNTYPE card */
   struct REPBLOCK **next_REPBLOCK = &RP->tree;
   struct CELLTYPE **next_CELLTYPE = &RP->pfct;
   struct CELLTYPE **next_LAYER    = NULL;
   struct CONNTYPE **next_CONNTYPE = NULL;
   struct INHIBBLK **next_INHIBBLK = NULL;
   struct MODBY    **next_MODBY = NULL;
   char **cc2,**cc3;          /* Ptrs to Group 2,3 card keys */

   orns inseed = 0;           /* Global default random number seed */

/* 'ignore' controls skipping after cards specifying zero elements.
*  Idea is to catch these before allocation to avoid fragmentation
*  of storage by abandoned blocks.  The skipped cards will be
*  scanned to swallow continuations, but otherwise unchecked.
*  Values of 'ignore' are: */
#define IGN_ReadAll    0   /* Read all cards */
#define IGN_Conntype   1   /* Ignore cards subsidiary to CONNTYPE */
#define IGN_Celltype   2   /* Ignore cards subsidiary to CELLTYPE */
#define IGN_Repertoire 3   /* Ignore cards subsidiary to REPERTOIRE */
   int ignore = IGN_ReadAll;
   int imct = 0;           /* Number of modulatory conn types */
   int ncc2,ncc3;          /* Numbers of Group 2,3 card keys */

   RP->tree = NULL;        /* Initialize linked-list root */
   RP0->ccgrp = CCGRP2;    /* Indicate we are in group II */

/*=====================================================================*
*                                                                      *
*              Interpretation of group II control cards                *
*                                                                      *
*=====================================================================*/

   cc2 = getcc2(&ncc2);
   cc3 = getcc3(&ncc3);
   while (cryin()) {
      cc2index = (enum G2CC)smatch(RK_NMERR, RK.last, cc2, ncc2);
      if (cc2index <= 0) {
         /* If not in Group 2 or Group 3, give error and
         *  read next one here, otherwise go to Group 3.  */
         if (smatch(0, RK.last, cc3, ncc3)) break;
         else continue;
         }
      /* Print the card (if oprty <= 3) */
      if (cc2index <= REPERTOIRE_CARD) cdprnt(RK.last);
      else                             cdprt1(RK.last);
      RP0->notmaster = TRUE;        /* Allow seed inputs */
      switch (cc2index) {

/*---------------------------------------------------------------------*
*                            'MASTER' card                             *
*---------------------------------------------------------------------*/

      case MASTER_CARD:
         RP0->notmaster = FALSE;    /* Disallow seed inputs */
         g2master(&inseed);
         break;

/*---------------------------------------------------------------------*
*                    'REGION' or 'REPERTOIRE' card                     *
*                                                                      *
*  Regions were traditionally known as "repertoires" in the Darwin 3   *
*  days, and are so commented throughout the program.                  *
*                                                                      *
*  Caution: if krp, kctp codes changed, must change also in d3chng.    *
*                                                                      *
*  Rev, 08/18/01, GNR - Allow long rname, eliminate restriction that   *
*                       ngx, ngy had to be on first card               *
*  Rev, 12/27/07, GNR - Separate krp codes from kctp codes             *
*---------------------------------------------------------------------*/

      case REGION_CARD:
      case REPERTOIRE_CARD:
      {  ui16 lngx,lngy;         /* Temps for items scanned   */
         char lrnm[LTEXTF+1];    /* while picking up ngx, ngy */
         char srnm[LSNAME+1];
         cdscan(RK.last, 1, SCANLEN, RK_WDSKIP);
         /* Pick up rname, sname, ngx, ngy first */

         inform("(S,A" qLTF ",A" qLSN ",2*UH)",
            lrnm, srnm, &lngx, &lngy, NULL);
         /* If ngx or ngy is zero, start ignoring cards */
         if (lngx==0 || lngy==0) {
            cryout(RK_P1, "0-->Region(repertoire) ignored--"
               "ngrp == 0.", RK_LN2, NULL);
            ignore = IGN_Repertoire;
            goto IGNORE_CARD;
            }

/* Allocate storage for a new repertoire block,
*  link it to the tree structure, and initialize it.  */

         ignore = IGN_ReadAll;
         *next_REPBLOCK = ir = (struct REPBLOCK *)allocpcv(Static,
            1, IXstr_REPBLOCK|MBT_Unint, "REPBLOCK structure");
         next_REPBLOCK = &ir->pnrp;
         RP->nrep++;
         il = NULL;
         ix = NULL;
         next_LAYER = &ir->play1;
         next_CONNTYPE = NULL;
         next_INHIBBLK = NULL;
         next_MODBY = NULL;
         rpdflt(ir);

/* Store the positional data already scanned */

         ir->hrname = (txtid)savetxt(lrnm);
         ir->lrn = (short)strlen(lrnm);
         memcpy(ir->sname, srnm, LSNAME);
         ir->ngx = lngx;
         ir->ngy = lngy;

/* Read remaining data from REPERTOIRE card */

         g2rep(ir);
         break;
         } /* End REGION/REPERTOIRE local scope */

/*---------------------------------------------------------------------*
*                           'CELLTYPE' card                            *
*                                                                      *
*  Ignore if follows a REPERTOIRE card with zero groups.  Otherwise,   *
*     be sure there was a prior REPERTOIRE card.  Always increment     *
*     seed so defaults are preserved.  Prescan the number of cells     *
*     and, if zero, start ignoring cards.                              *
*                                                                      *
*---------------------------------------------------------------------*/

      case CELLTYPE_CARD:
      {  long lnel;
         orns tmseed = inseed;
         inseed += 11113;
         if (ignore >= IGN_Repertoire) goto IGNORE_CARD;
         if (!next_LAYER) {
            cryout(RK_E1, "0***REGION or REPERTOIRE CARD MISSING.",
               RK_LN2, NULL);
            ignore = IGN_Celltype;
            goto IGNORE_CARD;
            }
         inform("(SW1,XIL)", &lnel, NULL);
         if (lnel <= 0) {
            cryout(RK_P1, "0-->Cell type ignored--nel <= 0.",
               RK_LN2, NULL);
            ignore = IGN_Celltype;
            goto IGNORE_CARD;
            }

/* Allocate storage for a new celltype block, link it to the tree
*     structure, and initialize it plus a few conntype defaults.  */

         ignore = IGN_ReadAll;
         *next_CELLTYPE = *next_LAYER = il =
            (struct CELLTYPE *)allocpcv(Static, 1,
            IXstr_CELLTYPE|MBT_Unint, "CELLTYPE structure");
         next_CELLTYPE = &il->pnct;
         next_LAYER    = &il->play;
         next_CONNTYPE = &il->pct1;
         next_INHIBBLK = &il->pib1;
         next_MODBY    = &il->pmby1;
         imct = 0;
         ix = NULL;
         il->pback = ir;
         ctdflt(il);
         il->iovec = RP->novec; RP->novec += CTO_NOFF;
         il->ctwk.tree.mastseed = tmseed;
         il->No.nsd        = inseed + 33331L;
         il->rsd           = inseed + 23237L;
         RP0->GPdD.phiseed = inseed + 22229L;
         RP0->LCnD.dseed   = inseed +  1997L;
         RP0->GIz3D.Z.izseed = RP0->GIz7D.Z.izseed =
            inseed +  1999L;
         RP0->n0flags &= ~N0F_CTPPFE;
         RP0->ksrctyp = BADSRC;  /* For getthr */

/* Read data from CELLTYPE card */

         g2cell(ir, il);

/* If PHASE now requested, allocate PHASEDEF block and fill it
*  with defaults, as a separate PHASE card is no longer required. */

         if (il->Ct.ctopt & OPTPH) mkctphsd(il);

         break;
         } /* End CELL TYPE local scope */

/*---------------------------------------------------------------------*
*                        'AMPLIFICATION' card                          *
*                                                                      *
*  Logic changed, 10/02/97, to load LCnD default params if entered     *
*  before a CONNTYPE card, otherwise load specific ix->Cn params for   *
*  most recent CONNTYPE.  Be sure there was a preceding CELLTYPE card  *
*  to link to since the last REPERTOIRE card.  This is assured if il   *
*  != NULL.                                                            *
*---------------------------------------------------------------------*/

      case AMPLIFICATION_CARD:
         if (ignore >= IGN_Celltype) goto IGNORE_CARD;
         if (!il) goto NO_CELL_CARD;

         /* Read data from AMPLIFICATION card */
         cdscan(RK.last, 1, SCANLEN, RK_WDSKIP);
         if (ix) {
            RP0->ksrctyp = ix->cnsrctyp;
            g2amplif(&ix->Cn); }
         else {
            RP0->ksrctyp = BADSRC;
            g2amplif(&RP0->LCnD);
            }
         break;

/*---------------------------------------------------------------------*
*                             'AEIF' card                              *
*  N.B. 10/10/09, GNR - It is now policy that RespFn cannot be changed *
*  once specified.  It is an error to contradict the CELLTYPE value.   *
*---------------------------------------------------------------------*/

      case AEIF_CARD:
         if (ignore >= IGN_Celltype) goto IGNORE_CARD;
         if (!il) goto NO_CELL_CARD;
         if (ckctrf(il, RF_BREG)) goto IGNORE_CARD;
         /* Create a BREGDEF block if not already there and
         *  bring in the defaults */
         if (!il->prfi) { il->prfi = allocpcv(Static, 1,
            IXstr_BREGDEF, "aEIF parameters");
            *(struct BREGDEF *)il->prfi = RP0->GBGD;
            ((struct BREGDEF *)il->prfi)->iovec = RP->novec++; }
         cdscan(RK.last, 1, SCANLEN, RK_WDSKIP);
         g2breg((struct BREGDEF *)il->prfi);
         break;

/*---------------------------------------------------------------------*
*                          'AUTOSCALE' card                            *
*                                                                      *
*  Be sure there was a preceding CELLTYPE card to link to since the    *
*  last REPERTOIRE card.  This is assured if il != NULL.               *
*---------------------------------------------------------------------*/

      case AUTOSC_CARD:
         if (ignore >= IGN_Celltype) goto IGNORE_CARD;
         if (!il) goto NO_CELL_CARD;

         /* Create a default AUTSCL block if not already there */
         if (!il->pauts) {
            il->pauts = (struct AUTSCL *)allocpcv(Static,
               1, IXstr_AUTSCL, "AUTSCL structure");
            *il->pauts = RP0->GASD;
            }
         /* Read data from AUTOSCALE card */
         cdscan(RK.last, 1, SCANLEN, RK_WDSKIP);
         g2autsc(il->pauts);
         /* Assign an offset for recording overflows */
         il->pauts->iovec = RP->novec++;
         break;

/*---------------------------------------------------------------------*
*                         'CONDUCTANCE' card                           *
*---------------------------------------------------------------------*/

      case CONDUCT_CARD:
         getcond(il, ix);
         break;

/*---------------------------------------------------------------------*
*                             'CSET' card                              *
*---------------------------------------------------------------------*/

      case CSET_CARD:
         getcset(il);
         break;

/*---------------------------------------------------------------------*
*                            'DECAY' card                              *
*                                                                      *
*  Logic changed, 07/28/92, to load LCnD default params if entered     *
*     before a CONNTYPE card, otherwise load specific ix->Cn params    *
*     for most recent CONNTYPE.                                        *
*---------------------------------------------------------------------*/

      case DECAY_CARD:
         if (ignore >= IGN_Celltype) goto IGNORE_CARD;
         if (!il) goto NO_CELL_CARD;

         /* Read data from DECAY card */
         cdscan(RK.last, 1, SCANLEN, RK_WDSKIP);
         g2decay(il, &il->Dc, ix ? &ix->Cn : &RP0->LCnD);
         break;

/*---------------------------------------------------------------------*
*                            'DELAY' card                              *
*                                                                      *
*  As with DECAY and AMPLIF, loads LCnD default params if entered      *
*  before a CONNTYPE card, otherwise loads specific ix->Cn params      *
*  for most recent CONNTYPE only.  User-specified delay distribution   *
*  is not yet implemented.                                             *
*---------------------------------------------------------------------*/

      case DELAY_CARD:
         if (ignore >= IGN_Celltype) goto IGNORE_CARD;
         if (!il) goto NO_CELL_CARD;

         /* Read data from DELAY card */
         cdscan(RK.last, 1, SCANLEN, RK_WDSKIP);
         g2delay(ix ? &ix->Cn : &RP0->LCnD);
         break;

/*---------------------------------------------------------------------*
*                          'DEPRESSION' card                           *
*                                                                      *
*  Be sure there was a preceding CELLTYPE card to link to since the    *
*  last REPERTOIRE card.  This is assured if il != NULL.  Because      *
*  g2dprsn() is also called from the MASTER card, cell-type-specific   *
*  work is done here.                                                  *
*---------------------------------------------------------------------*/

      case DEPRESSION_CARD:
         if (ignore >= IGN_Celltype) goto IGNORE_CARD;
         if (!il) goto NO_CELL_CARD;

         /* Read data from 'DEPRESSION' card */
         cdscan(RK.last, 1, SCANLEN, RK_WDSKIP);
         g2dprsn(&il->Dp);
         il->ctf |= CTFDN;    /* Activate for this cell type */
         break;

/*---------------------------------------------------------------------*
*                           'FADE-IN' card                             *
*                                                                      *
*  Note:  This option implements scale factor modulation designed to   *
*  improve network stability by introducing external stimuli slowly.   *
*                                                                      *
*  Ignore if it follows a REGION or CELLTYPE with no groups.  Be sure  *
*  there was a preceding CONNTYPE card to link to.  This is assured if *
*  ix != NULL.                                                         *
*---------------------------------------------------------------------*/

      case FADEIN_CARD:
         {  si32 *tfade;
            char *fops[] = { "SERIES-BY-TRIAL", "SBT", 
               "SERIES-BY-CYCLE", "SBC", "TRIALS-BY-CYCLE", "TBC" };
            int kfo, nfades;
            if (ignore >= IGN_Conntype) goto IGNORE_CARD;
            if (!ix) goto NO_CONN_CARD;
            cdscan(RK.last, 1, SCANLEN, RK_WDSKIP);
            kfo = match( 0, RK_MREQF, ~(RK_BLANK|RK_LFTPAREN), 0,
               fops, sizeof(fops)/sizeof(char *)); 
            if (kfo <= 0) goto IGNORE_CARD;
            tfade = (si32 *)mallocv(sizeof(si32)<<BITSPERBYTE,
               "Fade temp");
            if (!tfade) goto IGNORE_CARD;
            nfades = 1<<BITSPERBYTE;
            inform("(SNRB" qqv(FBfi) "IJ)", &nfades, tfade, NULL);
            if (RK.numcnv <= 0) goto IGNORE_CARD;
            /* Input is all OK, store info in CONNTYPE */
            ix->fdiopt = (kfo + 1) >> 1;
            ix->fdinum = RK.numcnv;
            ix->fadein = (si32 *)allocpcv(Static, ix->fdinum,
               IXjdat_type, "fade-ins");
            memcpy(ix->fadein, tfade, ix->fdinum*JSIZE);
            freev(tfade, "Fade temp"); 
            } /* End FADEIN_CARD local scope */
         break;

/*---------------------------------------------------------------------*
*       'GCONN' card (old name--INHIB--is also still accepted)         *
*                                                                      *
*  Rev, 04/07/07, GNR - Read rname,lname from GCONN in readrlnm style, *
*                       but leave obsolete INHIB card with old style.  *
*  R66, 01/30/16, GNR - Make INHIB a pure synonym for GCONN.           *
*                                                                      *
*  Ignore if follows a REPERTOIRE or CONNTYPE card with zero groups.   *
*  Otherwise, be sure there was a prior CELLTYPE card.  Then prescan   *
*  the number of bands and, if zero, ignore the card.  If the card is  *
*  valid, allocate an inhibblk, fill in the source, and call g2gconn.  *
*---------------------------------------------------------------------*/

      case GCONN_CARD:
      case INHIBITION_CARD:
      {  rlnm  tsrcnm;           /* Temp source name */
         struct INHIBBLK *ib;    /* Ptr to new INHIBBLK */
         int lnib;               /* Number of bands */

         if (ignore >= IGN_Celltype) goto IGNORE_CARD;
         if (!il) goto NO_CELL_CARD;
         if (readrlnm(&tsrcnm, il) != REPSRC) cryout(RK_E1,
            "0***GCONN INPUT MUST BE A REGION-LAYER NAME.",
            RK_LN2, NULL);

         inform("(S,I)", &lnib, NULL);
         if (lnib >= UI16_MAX) ermark(RK_NUMBERR);
         if (lnib <= 0) {
            cryout(RK_P1, "0-->Inhibition ignored--nib <= 0.",
               RK_LN2, NULL);
            goto IGNORE_CARD;
            }

/* Allocate storage for new inhibblk, link it to the tree structure,
*  and initialize it  */

         *next_INHIBBLK = ib = (struct INHIBBLK *)
            allocpcv(Static, 1, IXstr_INHIBBLK|MBT_Unint,
            "INHIBBLK structure");
         ib->inhbnd = (struct INHIBBAND *)allocpcv(Static,
            lnib, IXstr_INHIBBAND, "INHIBBAND structure");
         next_INHIBBLK = &ib->pib;
         ib->gsrcnm = tsrcnm;
         ibdflt(ib, lnib);    /* Set defaults for new INHIBBLK */

/* Read data from GCONN or INHIB card */

         g2gconn(il, ib);
         /* Assign offset for overflow count */
         ib->iovec = RP->novec++;
         break;
         } /* End GCONN local scope */

/*---------------------------------------------------------------------*
*                             'ION' card                               *
*---------------------------------------------------------------------*/

      case ION_CARD:
         getion(ir, il);
         break;

/*---------------------------------------------------------------------*
*       'IZHI2003', 'IZHI2007', 'IZHI', and 'IZHIKEVICH' cards         *
*  N.B. 10/10/09, GNR - It is now policy that RespFn cannot be changed *
*  once specified.  It is an error to contradict the CELLTYPE value.   *
*---------------------------------------------------------------------*/

      case IZHI2003_CARD:
         if (ignore >= IGN_Celltype) goto IGNORE_CARD;
         if (!il) goto NO_CELL_CARD;
         if (ckctrf(il, RF_IZH3)) goto IGNORE_CARD;
         /* Create an IZ03DEF block if not already there and
         *  bring in the defaults */
         if (!il->prfi) { il->prfi = allocpcv(Static, 1,
            IXstr_IZ03DEF, "Izhi2003 parameters");
            *(struct IZ03DEF *)il->prfi = RP0->GIz3D;
            ((struct IZ03DEF *)il->prfi)->Z.iovec = RP->novec++; }
         cdscan(RK.last, 1, SCANLEN, RK_WDSKIP);
         g2iz03((struct IZ03DEF *)il->prfi);
         break;

      case IZHI2007_CARD:
      case IZHI_CARD:
      case IZHIKEVICH_CARD:
         if (ignore >= IGN_Celltype) goto IGNORE_CARD;
         if (!il) goto NO_CELL_CARD;
         if (ckctrf(il, RF_IZH7)) goto IGNORE_CARD;
         /* Create an IZ07DEF block if not already there and
         *  bring in the defaults */
         if (!il->prfi) { il->prfi = allocpcv(Static, 1,
            IXstr_IZ07DEF, "Izhi2007 parameters");
            *(struct IZ07DEF *)il->prfi = RP0->GIz7D;
            ((struct IZ07DEF *)il->prfi)->Z.iovec = RP->novec++; }
         cdscan(RK.last, 1, SCANLEN, RK_WDSKIP);
         g2iz07((struct IZ07DEF *)il->prfi);
         break;

/*---------------------------------------------------------------------*
*                           'MODULATE' card                            *
*                                                                      *
*  Be sure there was a preceding CELLTYPE card to link to since the    *
*  last REPERTOIRE card.  This is assured if il != NULL.               *
*---------------------------------------------------------------------*/

      case MODULATE_CARD:
      {  struct MODBY *im;

         if (ignore >= IGN_Celltype) goto IGNORE_CARD;
         if (!il) goto NO_CELL_CARD;

         /* Allocate storage for new modby block and initialize it */
         *next_MODBY = im = (struct MODBY *)
            allocpcv(Static, 1, IXstr_MODBY, "MODBY structure");
         next_MODBY = &(im->pmby);
         mddflt(im);
         im->mct = ++imct;       /* Assign it a number */
         il->ctf |= CTFMT;       /* Indicate MODULATE was read */

         /* Read options from 'MODULATE' card */
         g2modul(il, im);
         break;
         } /* End MODULATE local scope */

/*---------------------------------------------------------------------*
*                            'NOISE' card                              *
*                                                                      *
*  Be sure there was a preceding CELLTYPE card to link to since the    *
*  last REPERTOIRE card.  This is assured if il != NULL.               *
*---------------------------------------------------------------------*/

      case NOISE_CARD:
         if (ignore >= IGN_Celltype) goto IGNORE_CARD;
         if (!il) goto NO_CELL_CARD;

         /* Read data from NOISE card */
         cdscan(RK.last, 1, SCANLEN, RK_WDSKIP);
         g2noise(&il->No);
         break;

/*---------------------------------------------------------------------*
*                             'NMOD' card                              *
*                                                                      *
*  Be sure there was a preceding CELLTYPE card to link to since the    *
*  last REPERTOIRE card.  This is assured if il != NULL.               *
*                                                                      *
*  The MODBY block for NMOD must always be first on the pmby1 chain.   *
*  If one is already there, just modify it.                            *
*---------------------------------------------------------------------*/

      case NMOD_CARD:
      {  struct MODBY *im;

         if (ignore >= IGN_Celltype) goto IGNORE_CARD;
         if (!il) goto NO_CELL_CARD;

         if (il->pmby1 && il->pmby1->mct == 0)
            im = il->pmby1;
         else {
            /* Allocate new modby block and initialize it */
            il->ctwk.tree.pnmby = im = (struct MODBY *)
               allocpcv(Static, 1, IXstr_MODBY, "MODBY structure");
            mddflt(im);
            il->ctf |= CTFMN;       /* Indicate NMOD was read */
            }

         /* Read options same as from 'MODULATE' card */
         g2modul(il, im);

         /* But "shunting" option is not allowed */
         if (im->Mc.mopt & IBOPTS)
            cryout(RK_E1, "0***OPT=S NOT USABLE WITH NMOD.",
               RK_LN2, NULL);
         break;
         } /* End NMOD local scope */

/*---------------------------------------------------------------------*
*                            'PHASE' card                              *
*                                                                      *
*  Be sure there was a preceding CELLTYPE card to link to since the    *
*  last REPERTOIRE card.  This is assured if il != NULL.               *
*---------------------------------------------------------------------*/

      case PHASE_CARD:
         if (ignore >= IGN_Celltype) goto IGNORE_CARD;
         if (!il) goto NO_CELL_CARD;

         /* If not already done by CELLTYPE card, generate a
         *  PHASEDEF block and link it to the CELLTYPE.  */
         if (!il->pctpd) mkctphsd(il);

         /* Read data from PHASE card to override defaults */
         cdscan(RK.last, 1, SCANLEN, RK_WDSKIP);
         g2phase(il->pctpd, &il->Dc);
         break;

/*---------------------------------------------------------------------*
*              'PPF' or 'PAIRED PULSE FACILITATION' card               *
*                                                                      *
*  Load LCnD,LPpfD default params if entered before a CONNTYPE card,   *
*  otherwise load specific ix->Cn,ix->PP params for most recent        *
*  CONNTYPE.  Be sure there was a preceding CELLTYPE card to link      *
*  to since the last REPERTOIRE card.  This is assured if il != NULL.  *
*---------------------------------------------------------------------*/

      case PPF_CARD:          /* Fallthrough aliases these two names */
      case PAIRED_CARD:
         if (ignore >= IGN_Celltype) goto IGNORE_CARD;
         if (!il) goto NO_CELL_CARD;

         /* Read data from PPF card */
         cdscan(RK.last, 0, SCANLEN, RK_WDSKIP);
         if (ix) {
            /* Create a PPFDATA block if not already there
            *  and initialize it with cell type defaults.  */
            if (!ix->PP) {
               ix->PP = (struct PPFDATA *)allocpmv(Static,
                  IXstr_PPFDATA, "PPFDATA structure");
               *(ix->PP) = RP0->LPpfD;
               }
            RP0->ksrctyp = ix->cnsrctyp;
            g2ppf(&ix->Cn, ix->PP);
            }
         else {
            RP0->ksrctyp = BADSRC;
            g2ppf(&RP0->LCnD, &RP0->LPpfD);
            RP0->n0flags |= N0F_CTPPFE;
            }
         break;

/*---------------------------------------------------------------------*
*                      'REFRACTORY PERIOD' card                        *
*                                                                      *
*  Be sure there was a preceding CELLTYPE card to link to since the    *
*  last REPERTOIRE card.  This is assured if il != NULL.               *
*---------------------------------------------------------------------*/

      case REFRACTORY_CARD:
         if (ignore >= IGN_Celltype) goto IGNORE_CARD;
         if (!il) goto NO_CELL_CARD;

         /* Create a RFRCDATA block if not already there */
         if (!il->prfrc) il->prfrc = (struct RFRCDATA *)allocpcv(
            Static, 1, IXstr_RFRCDATA, "RFRCDATA structure");
         /* Read data from REFRACTORY PERIOD card */
         g2refrac(il);
         break;

/*---------------------------------------------------------------------*
*                           'CONNTYPE' card                            *
*                                                                      *
*  Ignore if follows a REGION or REPERTOIRE or CELLTYPE card with      *
*  zero groups.  Otherwise, be sure there was a prior CELLTYPE card.   *
*  Then prescan the number of connections and if 0, ignore the card,   *
*  otherwise allocate conntype.                                        *
*---------------------------------------------------------------------*/

      case CONNTYPE_CARD:
      {  ui32 lnc;
         orns tmseed = inseed;
         int  jsrc;

         inseed += 99991L;
         if (ignore >= IGN_Celltype) goto IGNORE_CARD;
         if (!il) goto NO_CELL_CARD;
         inform("(SW1,2XUJ)", &lnc, NULL);
         if (lnc == 0) {
            cryout(RK_P1, "0-->Conntype ignored--nc == 0.",
               RK_LN2, NULL);
            ignore = IGN_Conntype;
            goto IGNORE_CARD;
            }
         if (lnc > (1 << IBnc)) {
            cryout(RK_E1, "0***nc EXCEEDS 2^28 LIMIT.", RK_LN2, NULL);
            lnc = (1 << IBnc); }
         ignore = IGN_ReadAll;

         /* Allocate storage for a new CONNTYPE block, link it into
         *  the tree structure, and determine input source type.  */
         *next_CONNTYPE = ix = (struct CONNTYPE *)allocpcv(Static,
            1, IXstr_CONNTYPE|MBT_Unint, "CONNTYPE structure");
         next_CONNTYPE = &ix->pct;
         jsrc = ix->cnsrctyp = (byte)readrlnm(&ix->srcnm, il);

         /* Compute shifts to be used for dprinting Sj and its deri-
         *  vatives and when determining scl from max pixel (NOPNI
         *  option) and when applying scale to Aij to get Aff.  */
         if (RP->compat & OLDRANGE) {
            ix->sjbs = (FBsi+Sr2mV), ix->sclb = FBsc;
            ix->scls = (FBsc+FBrs-FBwk); }
         else if (jsrc == REPSRC) {
            ix->sjbs = FBsi, ix->sclb = FBsc;
            ix->scls = (FBsc+FBrs-FBwk); }
         else {
            ix->sjbs = (FBsi+Sr2mV), ix->sclb = (FBsc-Sr2vS);
            ix->scls = (FBsc+FBrs-FBwk-Sr2vS); }

         /* Initialize the CONNTYPE, including moving in RP0->LCnD. */
         cndflt(il, ix);
         ix->nc = lnc;
         ix->iovec = RP->novec; RP->novec += CNO_NOFF;

         /* Move any deferred thresholds and/or scale into ix->Cn.
         *  This provides inheritance of global then local defaults--
         *  any values entered on later cards override these.  Not
         *  done if OLDRANGE because scales are fixed and items are
         *  already in their final locations from getthr().  */
         if (!(RP->compat & OLDRANGE)) {
            ix->Cn.scl   =
               chkscl(RP0->LDfD.us.s.scl,  ix->sclb, "scl");
            if (ix->cnsrctyp == REPSRC) {
               ix->Cn.et    = RP0->LDfD.umVt.t.et;
               ix->Cn.etlo  = RP0->LDfD.umVt.t.etlo;
               ix->Cn.mnsj  = RP0->LDfD.umVt.t.mnsj;
               ix->Cn.mtj   = RP0->LDfD.umVt.t.mtj;
               ix->Cn.sjrev = RP0->LDfD.umVt.t.sjrev;
               if (ix->PP) ix->PP->ppft = RP0->LDfD.umVt.t.ppft;
               ix->Cn.emxsj = RP0->LDfD.umVt.t.emxsj;
               }
            else {
               ix->Cn.et    = RP0->LDfD.ufrt.t.et;
               ix->Cn.etlo  = RP0->LDfD.ufrt.t.etlo;
               ix->Cn.mnsj  = RP0->LDfD.ufrt.t.mnsj;
               ix->Cn.mtj   = RP0->LDfD.ufrt.t.mtj;
               ix->Cn.sjrev = RP0->LDfD.ufrt.t.sjrev;
               if (ix->PP) ix->PP->ppft = RP0->LDfD.ufrt.t.ppft;
               ix->Cn.emxsj = RP0->LDfD.ufrt.t.emxsj;
               }
            }

/* The seeds should not themselves be generated with udev or
*     all series will be same but for a phase shift.  Here we
*     generate them by just adding some primes together.  */

         ix->cnwk.tree.mastseed = tmseed;
         ix->cseed    = inseed + 59999L;
         ix->lseed    = inseed + 67777L;
         ix->phseed   = inseed + 79999L;
         ix->rseed    = inseed + 85999L;
         ix->Cn.dseed = RP0->LCnD.dseed, RP0->LCnD.dseed += 2003;

/* Read data from CONNTYPE card */

         g2conn(il, ix);
         break;
         } /* CONNTYPE local scope */

/*---------------------------------------------------------------------*
*               'PARAMETERS' card and 'THRESHOLDS' card                *
*                                                                      *
*  Logic changed, 11/03/07, to make PARAMETERS and THRESHOLDS into     *
*  synonyms.  Both have all the options of either.  Both behave like   *
*  AMPLIF, DECAY, etc. in that they load RP0->LCnD default params if   *
*  entered before a CONNTYPE card, otherwise load specific ix->Cn      *
*  params for most recent CONNTYPE.  However, these cards also have a  *
*  few options relating to renaming that can only be entered at        *
*  specific CONNTYPE time, hence the extra ix argument.                *
*---------------------------------------------------------------------*/

      case PARAMS_CARD:
      case PARAMETERS_CARD:
      case THRESHOLDS_CARD:
         if (ignore >= IGN_Conntype) goto IGNORE_CARD;
         if (!il) goto NO_CELL_CARD;

         /* Read data from PARAMETERS/THRESHOLDS card */
         cdscan(RK.last, 1, SCANLEN, RK_WDSKIP);
         {  int iamthr = (cc2index == THRESHOLDS_CARD);
            if (ix) {
               RP0->ksrctyp = ix->cnsrctyp;
               g2param(ix, &ix->Cn, iamthr);
               }
            else {
               RP0->ksrctyp = BADSRC;
               g2param(NULL, &RP0->LCnD, iamthr);
               }
            } /* End iamthr local scope */
         break;

/*---------------------------------------------------------------------*
*                            'SEEDS' card                              *
*                                                                      *
*  Be sure there was a preceding CONNTYPE card to link to since the    *
*  last REPERTOIRE card.  This is assured if ix != NULL.               *
*---------------------------------------------------------------------*/

      case SEEDS_CARD:
         if (ignore >= IGN_Conntype) goto IGNORE_CARD;
         if (!ix) goto NO_CONN_CARD;
         /* Read data from SEEDS card */
         g2seeds(ix);
         break;

/*---------------------------------------------------------------------*
*                           'SUBARBOR' card                            *
*                                                                      *
*  Be sure there was a preceding CONNTYPE card to link to since the    *
*  last REPERTOIRE card.  This is assured if ix != NULL.               *
*---------------------------------------------------------------------*/

      case SUBARBOR_CARD:
         if (ignore >= IGN_Conntype) goto IGNORE_CARD;
         if (!ix) goto NO_CONN_CARD;
         /* Read data from SUBARBOR card */
         g2subarb(ix);
         break;

/*---------------------------------------------------------------------*
*                           'TERNARY' card                             *
*                                                                      *
*  Note:  This option implements the connection strength modulation    *
*  method of Piech et al, PNAS, E4108 (2013) with some restrictions.   *
*                                                                      *
*  Ignore if vpict == 0 or if it follows a REGION or CELLTYPE with no  *
*  groups.  Be sure there was a preceding CONNTYPE card to link to     *
*  since the last REPERTOIRE card.  This is assured if ix != NULL.     *
*---------------------------------------------------------------------*/

      case TERNARY_CARD:
         {  ui32 lvpict;
            if (ignore >= IGN_Conntype) goto IGNORE_CARD;
            if (!ix) goto NO_CONN_CARD;
            inform("(SW1,UJ)", &lvpict, NULL);
            if (lvpict == 0) {
               cryout(RK_P1, "0-->Ternary ignored--vpict == 0.",
                  RK_LN2, NULL);
               ignore = IGN_Conntype;
               goto IGNORE_CARD;
               }

            /* Create a TERNARY block if not already there */
            if (!ix->pvpfb) {
               ix->pvpfb = (struct TERNARY *)allocpcv(
                  Static, 1, IXstr_TERNARY, "TERNARY struct");
               terdflt(ix->pvpfb);
               }
            ix->pvpfb->vpict = lvpict;
            /* Read data from TERNARY card */
            g2vpfb(ix);
            } /* End TERNARY_CARD local scope */
         break;

         } /* End of big switch */

      continue;  /* Skip over error exits and process next card. */

/*=====================================================================*
*            Common error exits for group II control cards             *
*=====================================================================*/

NO_CONN_CARD:
      cryout(RK_E1, "0***CONNTYPE CARD MISSING.", RK_LN2, NULL);
      skip2end();
      continue;

NO_CELL_CARD:
      cryout(RK_E1, "0***CELLTYPE CARD MISSING.", RK_LN2, NULL);
      skip2end();
      continue;

/* Here to ignore a card.  Scan to swallow continuations only. */

IGNORE_CARD:
      skip2end();
      } /* End of card reading while */

   rdagn();
   return;

   } /* End d3grp2() */
