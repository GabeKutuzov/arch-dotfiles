/* (c) Copyright 2004-2005 Neurosciences Research Foundation, Inc. */
/* $Id: d3g2cond.c 30 2010-07-09 21:26:56Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                             d3g2cond.c                               *
*                                                                      *
*  This file contains functions used to parse information from         *
*  control cards relating to conductances and ions in all groups.      *
*                                                                      *
*  Call this routine on node 0 only.                                   *
************************************************************************
*  V8D, 02/01/04, GNR - New routines                                   *
*  ==>, 01/06/08, GNR - Last mod before committing to svn repository   *
***********************************************************************/

#define  NEEDCDEFS 1             /* Activate local conductance defs */
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "sysdef.h"
#include "d3global.h"
#include "d3opkeys.h"
#include "rocks.h"
#include "rkxtra.h"

/* Globals used here and for echoing and error checking */

/* Conductance type and activation options */
char *CondTypes[CDTP_N] = {
   "PASSIVE", "LINEAR", "ALPHA", "DOUBLE-EXP" };
char *CondActiv[CDAC_N] = {
   "CELLFIRE", "SYNAPTIC", "ION", "POISSON" };

/* Codes for parameters invalidated by each conductance type */
const long InvCdvinByType[CDTP_N] = { CDKY_REFR|CDKY_ACTV|CDKY_VTH|
   CDKY_VMAX|CDKY_DCY |CDKY_TTPK|CDKY_TAUM|CDKY_TAUR|CDKY_TAUD,
   CDKY_REFR|CDKY_ACTV|CDKY_TTPK|CDKY_TAUM|CDKY_TAUR|CDKY_TAUD,
   CDKY_VTH |CDKY_VMAX|                    CDKY_TAUR|CDKY_TAUD,
   CDKY_VTH |CDKY_VMAX|CDKY_TTPK|CDKY_TAUM};

/* Codes for parameters invalidated by each activation type */
const long InvCdvinByActv[CDAC_N] = {
   CDKY_IONA|CDKY_PPPC|CDKY_SEED,
   CDKY_IONA|CDKY_PPPC|CDKY_SEED,
   CDKY_PPPC|CDKY_SEED|CDKY_AT,
   CDKY_IONA|CDKY_AT };

/*=====================================================================*
*                              g3cptest                                *
*                                                                      *
*  Test whether it is allowed to update a parameter from a Group III   *
*  CONDUCTANCE card.  If so, set the bit indicating the parameter has  *
*  been entered and return YES.  If not, issue a warning and return NO.*
*=====================================================================*/

static struct {                  /* Unchanging parms to g3cptest */
   struct CONDUCT *qcnn;
   struct CONDUCT *qcnd;
   struct CELLTYPE *jjl;
   } CPTP;

static int g3cptest(ui32 bit, int test, char *pnm) {

   if (CPTP.qcnn->cdvin & bit && test) {
      CPTP.qcnd->cdvin |= bit;
      return YES; }
   else {
      cryout(RK_P1, " -->WARNING: Parameter ", RK_LN2, pnm, RK_CCL,
         " ignored for conductance ", getrktxt(CPTP.qcnn->hcdnm),
         RK_CCL, " in ", 4, fmtlrlnm(CPTP.jjl), RK_CCL,
         "        because incompatible with conductance type or "
         "activation source.", RK_LN0, NULL);
      return NO;
      }

   } /* End g3cptest() */


/***********************************************************************
*                               getcond                                *
*                                                                      *
*  Process the CONDUCTANCE card.                                       *
*                                                                      *
*  Arguments:  il, ix are pointers to the current CELLTYPE and         *
*     CONNTYPE blocks if called from Group II, otherwise NULL.         *
*                                                                      *
*  A Group I CONDUCTANCE card or Group II before the first CELLTYPE    *
*     defines a prototype with a name.  This is not necessarily used   *
*     unless referred to by some CELLTYPE.                             *
*  After a CELLTYPE card, establishes a conductance for that CELLTYPE. *
*     A prototype with the same name is copied in to give defaults.    *
*  After a CONNTYPE card, establishes a synaptic conductance.  A       *
*     prototype with the same name is copied in to give defaults.      *
*  A Group III CONDUCTANCE card with a rlnm identifier in parens       *
*     modifies just the named conductance in that layer, otherwise     *
*     all conductances with that name in all layers.  The user is      *
*     not allowed to change the TYPE or ACTIVATION parameters here.    *
*  Parameter values are carried forward from all same-named sources.   *
***********************************************************************/

void getcond(struct CELLTYPE *il, struct CONNTYPE *ix) {

   struct CELLTYPE *jl;          /* Ptr to CELLTYPE info */
   struct CELLTYPE *il1,*il2;    /* CELLTYPE loop controls */
   struct CONDUCT  *pcnd;        /* Ptr to default conductance */
   struct CONDUCT  **ppcnd;      /* Ptr to parent of pcnd */
   struct CONDUCT  *pcnn;        /* Ptr to new conductance */
   int    notg3;                 /* FALSE if in Group III */
   int    ktdcy;                 /* Copy of kcond if DECAY type */
   int    kwret;                 /* kwscan() return code */
   txtid  hnewcdnm;              /* Handle to conductance name */
   rlnm   rlname;                /* Temps for rep and layer names */

   /* Pick up and store the conductance name */
   inform("(SW1TH" qLTF ")", &hnewcdnm, NULL);

   /* If there is an rlnm in parens, pick it up now */
   setrlnm(&rlname, NULL, NULL);
   if (RK.scancode & RK_LFTPAREN) d3rlnm(&rlname);

   notg3 = (RP0->ccgrp & CCGRP3) == 0;
   if (notg3) {               /* This is Group I or II */
      if (rlname.rnm[0]) {
         ermark(RK_MARKFLD);
         cryout(RK_P1, "0***CELLTYPE SPECIFIER NOT ALLOWED HERE.",
            RK_LN2, NULL);
         skip2end();
         return;
         }
      if (il) {                /* Processing a CELLTYPE? */
         /* Assignment intended in for condition */
         for (ppcnd=&il->pcnd1; pcnd=*ppcnd; ppcnd=&pcnd->pncd) {
            if (pcnd->hcdnm == hnewcdnm) {
               pcnn = pcnd;         /* Name matched in layer list */
               goto ReadCondOpts;   /* Just go modify layer data */
               }
            }
         /* Create a new conductance for this CELLTYPE */
         pcnn = *ppcnd = (struct CONDUCT *)allocpcv(Static,
            1, IXstr_CONDUCT, "perm CONDUCTANCE data");
         pcnn->hcdnm = hnewcdnm;
         pcnn->gexp  = 1;
         /* Check for global defaults */
         for (pcnd=RP0->pcndg; pcnd; pcnd=pcnd->pncd) {
            if (pcnd->hcdnm == hnewcdnm) {
               *pcnn = *pcnd;       /* Copy in global defaults */
               break;
               }
            }
         /* If card follows a CONNTYPE, save ict, which is needed
         *  later if activation ends up being SYNAPTIC.  Then, if
         *  activation was not set globally, make it SYNAPTIC now.
         *  Otherwise, it remains at CELLFIRE default.  CDKY_ACTV
         *  bit is not set, as that would indicate user input.
         *  Activation threshold and cnflgs bit SCOND are dealt
         *  with in d3cchk when ACTIVATION is known for sure.   */
         if (ix) {
            pcnn->iact = ix->ict;
            if (!(pcnn->cdvin & CDKY_ACTV))
               pcnn->kactv = CDAC_SYNAPTIC;
            }
         } /* End Group II */
      else {                        /* Must be Group I */
         /* No CELLTYPEs yet.  Enter a global conductance default. */
         /* Assignment intended in for condition */
         for (ppcnd=&RP0->pcndg; pcnd=*ppcnd; ppcnd=&pcnd->pncd) {
            if (pcnd->hcdnm == hnewcdnm) {
               pcnn = pcnd;         /* Name matched in global list */
               goto ReadCondOpts;
               }
            }
         pcnn = *ppcnd = (struct CONDUCT *)callocv(
            1, sizeof(struct CONDUCT), "CONDUCTANCE data");
         pcnn->hcdnm = hnewcdnm;
         pcnn->gexp  = 1;
         }
      } /* End Groups I-II */
   else {                           /* This is Group III */
      /* Establish a temporary CONDUCT to hold modified params */
      pcnn = (struct CONDUCT *)callocv(
         1, sizeof(struct CONDUCT), "CONDUCTANCE change data");
      pcnn->kcond = pcnn->kactv = -1;  /* For error trapping */
      /* Establish looping over all or just one cell type */
      if (rlname.rnm[0]) {
         if (!(jl = findln(&rlname))) { /* Assignment intended */
            ermark(RK_MARKFLD);  /* Set RK.iexit and RK.highrc */
            cryout(RK_P1,"0***", RK_LN2+4, fmtrlnm(&rlname),
               LXRLNM, " NOT FOUND.", RK_CCL, NULL);
            skip2end();
            return;
            }
         il1 = jl, il2 = jl->pnct;
         }
      else
         il1 = RP->pfct, il2 = NULL;
      }

/*---------------------------------------------------------------------*
*                        Scan the card options                         *
*---------------------------------------------------------------------*/

ReadCondOpts:
   /* Warning:  Code below depends on having only one %K option in
   *  this kwscan call that is allowed in Group III (currently none).
   *  Else must save RK.mcbits and RK.mckpm separately for each such
   *  option.  Also, order of options must match order of CDKY bits in
   *  conduct.h.  */
   while (kwret = kwscan(&pcnn->cdvin, /* Assignment intended */
         "TYPE%RX",           notg3,
         "IONDEP%X",
         "VREV%B20/27IL$mV",  &pcnn->Vrev,
         "GEXP%V~B8IH",       &pcnn->gexp,
         "OPT%RKC" okcdopt,   notg3, &pcnn->cdopts,
         "REFRAC%V>UH",       &pcnn->refrac,
         "IONCON%RTH",        notg3, &pcnn->hionr,
         "CDCYCUT%RV>0<1F",   notg3, &pcnn->cdcycut,
         "DECAY%N" KWS_DECAY, &pcnn->gDCY,
         "ACTIVATION%RX",     notg3,
         "GBAR%V>B20UL$nS",   &pcnn->gbar,
         "VTH%B20/27IL$mV",   &pcnn->ugt.lin.vth,
         "VMAX%B20/27IL$mV",  &pcnn->ugt.lin.vmax,
         "TTPK%V>B20UL$ms",   &pcnn->ugt.alf.ttpk,
         "TAUM%V>B20UJ$ms",   &pcnn->ugt.alf.taum,
         "TAUR%V>F$ms",       &pcnn->ugt.dbl.taur,
         "TAUD%V>F$ms",       &pcnn->ugt.dbl.taud,
         "IONACTV%X",
         "PROB%V>B28UL",      &pcnn->uga.psn.pppc,
         "SEED%V>UL",         &pcnn->uga.psn.seed,
         "AT%B20/27IL$mV",    &pcnn->uga.at,
         NULL))
      switch (kwret) {

      case 1:                 /* TYPE */
         /* Any necessary errors are generated by match() */
         pcnn->kcond = (schr)match(RK_EQCHK, RK_MREQF,
            ~RK_COMMA, 0, CondTypes, CDTP_N) - 1;
         if (pcnn->kcond >= 0) {
            pcnn->cdvin &= ~InvCdvinByType[pcnn->kcond];
            if (pcnn->kcond >= CDTP_GATED)
               RP0->n0flags |= N0F_MKTBLS;
            }
         break;

      case 2:                 /* IONDEP */
         if (notg3) {
            if (RK.scancode == RK_EQUALS)
               inform("(SV(TH" qLTF ",V>F$/uM,V))",
                  &pcnn->hiond, &pcnn->gfac, NULL);
            else ermark(RK_PUNCERR);
            }
         else                 /* Group III - can't change hiond */
            eqscan(&pcnn->gfac, "V>F$/uM", RK_EQCHK);
         break;

      case 3:                 /* ACTIVATION */
         pcnn->kactv = (schr)match(RK_EQCHK, RK_MREQF,
            ~(RK_COMMA|RK_EQUALS), 0, CondActiv, CDAC_N) - 1;
         if (pcnn->kactv >= 0)
            pcnn->cdvin &= ~InvCdvinByActv[pcnn->kactv];
         break;

      case 4:                 /* IONACTV */
         if (RK.scancode == RK_EQUALS) {
            if (notg3) {
               char IEcode[2] = { 0, 0 };
               pcnn->uga.ion.react = 0.0;
               inform("(SV(TH" qLTF ",A1,F$uM,NF$uM,V))",
                  &pcnn->uga.ion.hiont, IEcode,
                  &pcnn->uga.ion.ionth, &pcnn->uga.ion.react, NULL);
               if (IEcode[0] && !IEcode[1]) {
                  char IEup = toupper(IEcode[0]);
                  if (IEup == 'E')      pcnn->cdflgs |= CDFL_ACTEXT;
                  else if (IEup == 'I') pcnn->cdflgs &= ~CDFL_ACTEXT;
                  else ermark(RK_IDERR);
                  }
               if (pcnn->uga.ion.react == 0.0)
                  pcnn->uga.ion.react = pcnn->uga.ion.ionth;
               }
            else {               /* Group III - can't change hiont */
               inform("(SV(F$uM,NF$uM,V))",
                  &pcnn->uga.ion.ionth, &pcnn->uga.ion.react, NULL);
               }
            }                 /* End equals found */
         else ermark(RK_PUNCERR);
         break;

         } /* End kwret switch and while */

/*---------------------------------------------------------------------*
*  If in Group III store the new options in the affected CONDUCTances. *
*  Note that user is not allowed to change TYPE or ACTIVATION or mode  *
*  of decay at this time. If parameters are entered that do not match  *
*  the original TYPE, ACTIVATION, or mode of decay, they are ignored   *
*  and a warning issued, thus allowing one Group III CONDUCTANCE card  *
*  to have different effects on different name-matched conductances.   *
*---------------------------------------------------------------------*/

   if (RP0->ccgrp & CCGRP3) {

      /* If CELLTYPE was not selected, loop over all cell types.
      *  Then loop over conductances associated with each selected
      *  cell type, but modify only the ones with matching names.  */
      CPTP.qcnn = pcnn;
      for (jl=il1; jl!=il2; jl=jl->pnct) {
         CPTP.jjl = jl;
         for (pcnd=jl->pcnd1; pcnd; pcnd=pcnd->pncd) {
            if (hnewcdnm == pcnd->hcdnm) {
               CPTP.qcnd = pcnd;
               ktdcy = pcnd->gDCY.omega ? pcnd->kcond : CDTP_NDECAY;
               if (pcnn->cdvin & CDKY_IOND)              /* IONDEP */
                  pcnd->gfac   = pcnn->gexp;
               if (pcnn->cdvin & CDKY_VREV)              /* VREV */
                  pcnd->Vrev   = pcnn->Vrev;
               if (pcnn->cdvin & CDKY_GEXP)              /* GEXP */
                  pcnd->gexp   = pcnn->gexp;
               if (g3cptest(CDKY_REFR,                   /* REFRAC */
                     pcnd->cdflgs & CDFL_HITNT, "REFRAC"))
                  pcnd->refrac = pcnn->refrac;
               if (g3cptest(CDKY_DCY,                    /* DECAY */
                     (pcnd->gDCY.omega != 0) ==
                     (pcnn->gDCY.omega != 0), "DECAY"))
                  pcnd->gDCY = pcnn->gDCY;
               if (pcnn->cdvin & CDKY_GBAR)              /* GBAR */
                  pcnd->gbar   = pcnn->gbar;
               if (g3cptest(CDKY_VTH,                    /* VTH */
                     pcnd->kcond == CDTP_LINEAR, "VTH"))
                  pcnd->ugt.lin.vth = pcnn->ugt.lin.vth;
               if (g3cptest(CDKY_VMAX,                   /* VMAX */
                     pcnd->kcond == CDTP_LINEAR, "VMAX"))
                  pcnd->ugt.lin.vmax = pcnn->ugt.lin.vmax;
               if (g3cptest(CDKY_TTPK,                   /* TTPK */
                     ktdcy == CDTP_ALPHA, "TTPK") &&
                     pcnd->ugt.alf.ttpk != pcnn->ugt.alf.ttpk) {
                  pcnd->ugt.alf.ttpk = pcnn->ugt.alf.ttpk;
                  RP0->n0flags |= N0F_MKTBLS; }
               if (g3cptest(CDKY_TAUM,                   /* TAUM */
                     ktdcy == CDTP_ALPHA, "TAUM") &&
                     pcnd->ugt.alf.taum != pcnn->ugt.alf.taum) {
                  pcnd->ugt.alf.taum = pcnn->ugt.alf.taum;
                  RP0->n0flags |= N0F_MKTBLS; }
               if (g3cptest(CDKY_TAUR,                   /* TAUR */
                     ktdcy == CDTP_DOUBLE, "TAUR") &&
                     pcnd->ugt.dbl.taur != pcnn->ugt.dbl.taur) {
                  pcnd->ugt.dbl.taur = pcnn->ugt.dbl.taur;
                  RP0->n0flags |= N0F_MKTBLS; }
               if (g3cptest(CDKY_TAUD,                   /* TAUD */
                     ktdcy == CDTP_DOUBLE, "TAUD") &&
                     pcnd->ugt.dbl.taud != pcnn->ugt.dbl.taud) {
                  pcnd->ugt.dbl.taud = pcnn->ugt.dbl.taud;
                  RP0->n0flags |= N0F_MKTBLS; }
               if (g3cptest(CDKY_IONA,                   /* IONACTV */
                     pcnd->kactv == CDAC_IONIC, "IONACTV"))
                  pcnd->uga.ion.ionth = pcnn->uga.ion.ionth;
               if (g3cptest(CDKY_PPPC,                   /* PPPC */
                     pcnd->kactv == CDAC_POISSON, "PROB"))
                  pcnd->uga.psn.pppc = pcnn->uga.psn.pppc;
               if (g3cptest(CDKY_SEED,                   /* SEED */
                     pcnd->kactv == CDAC_POISSON, "SEED"))
                  pcnd->uga.psn.seed = pcnn->uga.psn.seed;
               if (g3cptest(CDKY_AT,                     /* AT */
                     pcnd->kactv <= CDAC_SYNAPTIC, "AT"))
                  pcnd->uga.at = pcnn->uga.at;
               } /* End handling name match */
            } /* End conductance loop */
         } /* End celltype loop */

      freev(pcnn, "CONDUCTANCE change data");

      } /* End processing Group III options */

   } /* End getcond() */


/***********************************************************************
*                               getcset                                *
*                                                                      *
*  Process the CSET card.                                              *
*                                                                      *
*  Argument:  il is a pointer to the current CELLTYPE if called        *
*     after a CELLTYPE has been entered, otherwise NULL.               *
*                                                                      *
*  If 'il' is NULL, the control card has the form:                     *
*  CSET  name  (name, name, name, ...)                                 *
*  The first name defines the name of this conductance set and         *
*     the names in parens define the conductances included in          *
*     the set.  The corresponding CONDUCTANCE cards need not be        *
*     entered before the CSET card.  Duplicate names are merged.       *
*                                                                      *
*  If 'il' is not NULL, the control card has the form:                 *
*  CSET  name, name, name, ...                                         *
*  All the conductances in all the named conductance sets are          *
*     added to the specified cell type.  A warning is issued if a      *
*     conductance is duplicated, and it is NOT merged, i.e.            *
*     overrides should be entered AFTER the CSET card.                 *
*                                                                      *
*  This card has no purpose in Group III and is not recognized there.  *
***********************************************************************/

void getcset(struct CELLTYPE *il) {

   struct CONDSET *pcs;          /* Ptr to CSET being created */
   struct CONDSET **ppcs;        /* Ptr to parent of pcs */
   struct CONDUCT *pcnd;         /* Ptr to existing conductance */
   struct CONDUCT **ppcnd;       /* Ptr to parent of pcnd */
   struct CONDUCT *pcnn;         /* Ptr to new conductance */
   txtid *hnl,*hnle;             /* Ptrs for scanning name list */
   txtid  hnewcsnm;              /* Handle for a new CSET name */

   if (il) {                     /* Apply CSET to cell type */
      cdscan(RK.last, 1, SCANLEN, RK_WDSKIP);
      while (1) {
         /* Read a name and check punctuation */
         inform("(SNTH" qLTF ")", &hnewcsnm, NULL);
         if (RK.scancode & RK_ENDCARD) break;
         if (RK.scancode & ~RK_COMMA) ermark(RK_PUNCERR);
         /* Search for matching previously entered CSET */
         for (pcs=RP0->pcnds; pcs; pcs=pcs->pncs) {
            if (pcs->hcsnm == hnewcsnm) goto AddCondsToCellType;
            }
         /* Name does not exist, generate error */
         cryout(RK_E1, " ***CSET NAME ", RK_LN1, getrktxt(hnewcsnm),
            RK_CCL, " NOT FOUND.", RK_CCL, NULL);
         continue;

AddCondsToCellType:
         /* Found name, transfer conductances to celltype list */
         hnl = pcs->phcsnms; hnle = hnl + pcs->nhused;
         for ( ; hnl<hnle; ++hnl) {
            /* Assignment intended in for condition */
            for (ppcnd=&il->pcnd1; pcnd=*ppcnd; ppcnd=&pcnd->pncd) {
               if (pcnd->hcdnm == *hnl) {
                  /* This one is already there--print warning and
                  *  ignore (needs complex code to merge properly) */
                  cryout(RK_P1, " -->WARNING: CSET conductance ",
                     RK_LN1, getrktxt(hnewcsnm), RK_CCL, " is dup"
                     "licated in this cell type.", RK_CCL, NULL);
                  goto NextCSETMember;
                  }
               } /* End scanning for duplicates */
            /* Find global default to match this name */
            for (pcnd=RP0->pcndg; pcnd; pcnd=pcnd->pncd) {
               if (pcnd->hcdnm == *hnl) goto AddCSETMemberToCellType;
               }
            /* This conductance does not exist--generate error */
            cryout(RK_E1, " ***CONDUCTANCE ", RK_LN1, getrktxt(*hnl),
               RK_CCL, " IN CSET ", getrktxt(pcs->hcsnm), RK_CCL,
               " NOT FOUND.", RK_CCL, NULL);
            continue;

AddCSETMemberToCellType:
            /* Create a new conductance for this CELLTYPE */
            pcnn = *ppcnd = (struct CONDUCT *)allocpcv(Static,
               1, IXstr_CONDUCT, "perm CONDUCTANCE data");
            *pcnn = *pcnd;    /* Copy data for this conductance */
NextCSETMember: ;
            } /* End CSET member list scan */

         } /* End name list scan */

      } /* End applying CSETs to cell type */

   else {                        /* Create CSET */
      /* Pick up and store the name */
      inform("(SW1TH" qLTF ")", &hnewcsnm, NULL);
      /* See if a CSET with this name already exists */
      /* Assignment intended in for condition */
      for (ppcs=&RP0->pcnds; pcs=*ppcs; ppcs=&pcs->pncs) {
         if (pcs->hcsnm == hnewcsnm) goto AddNamesToCSET;
         }
      /* No, make a new one */
      pcs = *ppcs = (struct CONDSET *)callocv(
         1, sizeof(struct CONDSET), "CSET data");
      pcs->hcsnm = hnewcsnm;

      /* Create or expand the name list as needed */
AddNamesToCSET:
      while (1) {
         inform("(SNTH" qLTF ")", &hnewcsnm, NULL);
         if (RK.scancode & RK_ENDCARD) break;
         if (RK.scancode & ~(RK_COMMA|RK_RTPAREN) ^ RK_INPARENS)
            ermark(RK_PUNCERR);
         hnl = pcs->phcsnms; hnle = hnl + pcs->nhused;
         for ( ; hnl<hnle; ++hnl) if (hnewcsnm == *hnl) {
            cryout(RK_P1, " -->WARNING: Conductance name ", RK_LN1,
               getrktxt(hnewcsnm), RK_CCL,
               " is duplicated in this CSET.", RK_CCL, NULL);
            goto AddNamesToCSET;
            }
         if (pcs->nhallo <= pcs->nhused) {
            pcs->phcsnms = (txtid *)reallocv(pcs->phcsnms,
               CSETINCR*sizeof(txtid), "CSET name list");
            pcs->nhallo += CSETINCR;
            }
         pcs->phcsnms[pcs->nhused++] = hnewcsnm;
         } /* End name list scan */

      } /* End create CSET */

   } /* End getcset() */


/***********************************************************************
*                               getion                                 *
*                                                                      *
*  Process the ION card.                                               *
*                                                                      *
*  A Group I ION card or Group II before the first REGION defines a    *
*     prototype with a name.  This is not necessarily used.            *
*  After a REGION|REPERTOIRE card, establishes an ion type that        *
*     applies to all cell types in that region.  All the cell types    *
*     in the region contribute to the external pool of this ion.  A    *
*     prototype ION entry with the same name is copied in to give      *
*     defaults.                                                        *
*  After a CELLTYPE card, establishes an ion that is only contributed  *
*     to by that cell type, even if external.  A prototype or region   *
*     ION entry with the same name is copied in to give defaults.      *
*  A Group III ION card with an rname (which must be followed by a     *
*     comma and no lname) or an rlnm in standard form in parens        *
*     modifies just the named conductance in that region or layer,     *
*     otherwise all conductances with that name in all layers.         *
*  Parameter values are carried forward from all same-named sources.   *
*  Since all parameter values must be nonzero, zero entries rather     *
*     than key codes are used to determine where defaults are used.    *
***********************************************************************/

void getion(struct REPBLOCK *ir, struct CELLTYPE *il) {

   struct REPBLOCK *jr;          /* Ptr to REPBLOCK info */
   struct CELLTYPE *jl;          /* Ptr to CELLTYPE info */
   struct CELLTYPE *il1,*il2;    /* CELLTYPE loop controls */
   struct IONTYPE  *pion;        /* Ptr to old ion entry */
   struct IONTYPE **ppion;       /* Ptr to parent of pion */
   struct IONTYPE  *pjon;        /* Ptr to new ion entry */
   ui32   ic;                    /* Keys-found codes--not used */
   int    notg3;                 /* FALSE if in Group III */
   txtid  hnewionn;              /* Handle to conductance name */
   rlnm   rlname;                /* Temps for rep and layer names */

   /* Pick up and store the ion name */
   inform("(SW1TH" qLTF ")", &hnewionn, NULL);

   /* If there is an rlnm in parens, pick it up now */
   setrlnm(&rlname, NULL, NULL);
   if (RK.scancode & RK_LFTPAREN) d3rlnm(&rlname);

   notg3 = TRUE;
   if (RP0->ccgrp & CCGRP3) {    /* Processing in Group III? */
      notg3 = FALSE;
      /* Establish a temporary IONTYPE to hold modified params */
      pjon = (struct IONTYPE *)callocv(
         1, sizeof(struct IONTYPE), "ION change data");
      /* If there was an rlnm in parens, analyze it now */
      if (rlname.rnm[0] == '\0') {
         /* No qualifier entered, set to scan all ions */
         il1 = RP->pfct, il2 = NULL;
         }
      else if (rlname.lnm[0] == '\0') {
         /* Got a repertoire name alone */
         for (jr=RP->tree; jr; jr=jr->pnrp) {
            if (!strncmp(rlname.rnm, jr->sname, LSNAME)) break;
            }
         if (!jr) {
            ermark(RK_MARKFLD);  /* Set RK.iexit and RK.highrc */
            cryout(RK_P1, "0***REGION ", RK_LN2+11, rlname.rnm,
               LSNAME, " NOT FOUND.", RK_CCL, NULL);
            skip2end();
            return;
            }
         il1 = jr->play1, il2 = jr->pnrp ? jr->pnrp->play1 : NULL;
         }
      else {
         /* Got a full rlnm */
         if (!(jl = findln(&rlname))) { /* Assignment intended */
            ermark(RK_MARKFLD);  /* Set RK.iexit and RK.highrc */
            cryout(RK_P1,"0***", RK_LN2+4, fmtrlnm(&rlname),
               LXRLNM, " NOT FOUND.", RK_CCL, NULL);
            skip2end();
            return;
            }
         il1 = jl, il2 = jl->pnct;
         }
      } /* End Group III setup */

   else if (il) {                /* Processing a CELLTYPE? */
      if (!ir) d3exit(IONREG_ERR, fmturlnm(il), 0);
      /* Assignment intended in for condition */
      for (ppion=&il->pion1; pion=*ppion; ppion=&pion->pnion) {
         if (pion->hionnm == hnewionn) {
            pjon = pion;         /* Name matched in layer list */
            goto ReadIonOpts;    /* Just go modify layer data */
            }
         }
      /* Create a new ion for this CELLTYPE */
      pjon = *ppion = (struct IONTYPE *)allocpcv(Static,
         1, IXstr_IONTYPE, "perm ION data");
      pjon->hionnm = hnewionn;
      /* Check for region defaults */
      for (pion=ir->pionr; pion; pion=pion->pnion) {
         if (pion->hionnm == hnewionn) {
            *pjon = *pion;       /* Copy in region defaults */
            goto ReadIonOpts;
            }
         }
      /* Check for global defaults */
      for (pion=RP0->piong; pion; pion=pion->pnion) {
         if (pion->hionnm == hnewionn) {
            *pjon = *pion;       /* Copy in global defaults */
            goto ReadIonOpts;
            }
         }
      } /* End establishing a cell layer ion */

   else if (ir) {                /* Processing a regional ion? */
      /* Assignment intended in for condition */
      for (ppion=&ir->pionr; pion=*ppion; ppion=&pion->pnion) {
         if (pion->hionnm == hnewionn) {
            pjon = pion;         /* Name matched in region list */
            goto ReadIonOpts;    /* Just go modify region data */
            }
         }
      /* Create a new ion for this region */
      pjon = *ppion = (struct IONTYPE *)allocpcv(Static,
         1, IXstr_IONTYPE, "perm ION data");
      pjon->hionnm = hnewionn;
      /* Check for global defaults */
      for (pion=RP0->piong; pion; pion=pion->pnion) {
         if (pion->hionnm == hnewionn) {
            *pjon = *pion;       /* Copy in global defaults */
            goto ReadIonOpts;
            }
         }
      } /* End establishing a regional ion */

   else {                        /* Must be Group I */
      /* No CELLTYPEs yet.  Enter a global ion default */
      /* Assignment intended in for condition */
      for (ppion=&RP0->piong; pion=*ppion; ppion=&pion->pnion) {
         if (pion->hionnm == hnewionn) {
            pjon = pion;         /* Name matched in global list */
            goto ReadIonOpts;
            }
         }
      pjon = *ppion = (struct IONTYPE *)callocv(
         1, sizeof(struct IONTYPE), "ION data");
      pjon->hionnm = hnewionn;
      } /* End establishing global ion default block */

/*---------------------------------------------------------------------*
*                        Scan the card options                         *
*---------------------------------------------------------------------*/

ReadIonOpts:
   /* Warning:  Code below depends on having <= one %K option in this
   *  kwscan() call that can cause updates in Group III--else must
   *  save RK.mcbits and RK.mckpm separately for each such option.  */
   kwscan(&ic, /* Assignment intended */
      "Z%IH",              &pjon->z,
      "C0EXT%V>F$mM",      &pjon->C0ext,
      "C0INT%V>F$mM",      &pjon->C0int,
      "CMINEXT%V>F$mM",    &pjon->Cminext,
      "CMININT%V>F$mM",    &pjon->Cminint,
      "VOLEXT%V>F$um3",    &pjon->Vext,
      "VOLINT%V>F$um3",    &pjon->Vint,
      "TAUEXT%V>F$ms",     &pjon->tauext,
      "TAUINT%V>F$ms",     &pjon->tauint,
      "OPT%RKC" okionop, notg3, &pjon->ionopts,
      NULL);

/*---------------------------------------------------------------------*
*  If in Group III, store the new options in the affected IONTYPEs.    *
*---------------------------------------------------------------------*/

   if (RP0->ccgrp & CCGRP3) {

      /* If CELLTYPE was not selected, loop over all cell types.
      *  Then loop over conductances associated with each selected
      *  cell type, but modify only the ones with matching names.  */
      for (jl=il1; jl!=il2; jl=jl->pnct) {
         for (pion=jl->pion1; pion; pion=pion->pnion) {
            if (hnewionn == pion->hionnm) {
               if (pjon->C0ext)   pion->C0ext   = pjon->C0ext;
               if (pjon->C0int)   pion->C0int   = pjon->C0int;
               if (pjon->Cminext) pion->Cminext = pjon->Cminext;
               if (pjon->Cminint) pion->Cminint = pjon->Cminint;
               if (pjon->Vext)    pion->Vext    = pjon->Vext;
               if (pjon->Vint)    pion->Vint    = pjon->Vint;
               if (pjon->tauext)  pion->tauext  = pjon->tauext;
               if (pjon->tauint)  pion->tauint  = pjon->tauint;
               if (pjon->z)       pion->z       = pjon->z;
               } /* End handling name match */
            } /* End IONTYPE loop */
         } /* End celltype loop */

      freev(pjon, "IONTYPE change data");
      } /* End processing Group III options */

   } /* End getion() */


