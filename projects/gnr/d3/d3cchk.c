/* (c) Copyright 2004-2006, The Rockefeller University *11115* */
/* $Id: d3cchk.c 78 2018-08-02 18:36:58Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                              d3cchk.c                                *
*                                                                      *
*     Link cell types and regions to their conductances and ions       *
*                        (must follow d3tchk)                          *
*                                                                      *
*  This routine performs the following actions:                        *
*  (1) If activation type is SYNAPTIC, locate the relevant CONNTYPE    *
*      and set its cnflgs SCOND bit.  Generate error if not found.     *
*      Turn off phase processing for such CONNTYPEs.                   *
*  (2) Scan conductance data structures to be sure all required        *
*      (nondefaulted) parameters have been entered and incompatible    *
*      parameters (shared in a union) have not been entered.  Count    *
*      the numbers of conductances and ions of various types as        *
*      needed in d3allo and d3cndl for variable allocation.            *
*  (3) Identify gated conductances.  Count them and compute time       *
*      needed to decay to specified decay cutoff.                      *
*  (4) Be sure all named ion types have been defined.  Look up the     *
*      referenced ion types and store pointers to ion data.            *
*  (5) Scan ion data structures to be sure all required parameters     *
*      have been entered.  At the same time, issue warnings for and    *
*      delete any ions that are not modified by any conductance.       *
*  (6) If parallel, allocate broadcast space in Shared pool and        *
*      rethread for faster access those ion types whose external       *
*      concentrations need to be consolidated across cells that        *
*      may be on different computational nodes.                        *
*  (7) Delete the Group I conductance and ion default structures.      *
*                                                                      *
*  Construction of look-up tables for activation functions is done     *
*      in d3ctbl(), not here, because parameters may change at         *
*      Group III time (if using DECAY rather than full evaluation).    *
*                                                                      *
*  N.B.  When this code is actually implemented, be sure to set up     *
*  an ssck machanism to indicate to d3lafs/d3oafs whether this is a    *
*  positive or a negative conductance.                                 *
************************************************************************
*  V8D, 02/23/04, GNR - New routines                                   *
*  ==>, 01/06/08, GNR - Last mod before committing to svn repository   *
***********************************************************************/

#define  NEEDCDEFS 1             /* Activate local conductance defs */
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "sysdef.h"
#include "d3global.h"
#include "rocks.h"
#include "rkxtra.h"
#include "bapkg.h"
#ifdef PAR
#include "swap.h"
#endif

extern const long InvCdvinByType[CDTP_N]; /* Invalid parms by type */
extern const long InvCdvinByActv[CDAC_N]; /* Invalid parms by actv */

static struct IonCheckData {
   struct REPBLOCK *jr;
   struct CELLTYPE *jl;
#ifdef PAR
   struct IONTYPE **ppnix;       /* Ptr to ptr to next ion exch */
   int    nbcst;                 /* Number of ions to broadcast */
#endif
   int    FirstMsg;
   txtid  hion;
   txtid  hcnd;
   short  rionc;                 /* Ion alloc for REPBLOCK ions */
   short  tionc;                 /* Total ion alloc for celltype */
   } ICD;


/*=====================================================================*
*                              fmtcndnm                                *
*                                                                      *
*  Format conductance and celltype names for error messages.           *
*=====================================================================*/

static char *fmtcndnm(void) {
   return ssprintf(NULL, "       FOR CONDUCTANCE \"%s\" ON %s.",
      getrktxt(ICD.hcnd), fmturlnm(ICD.jl));
   } /* End fmtcndnm() */


/*=====================================================================*
*                               dtgated                                *
*                                                                      *
*  Compute time a gated conductance remains activated.                 *
*  (This function may also be used by d3news().)                       *
*  (At present, implemented for ALPHA and DOUBLE-EXP types.)           *
*  (Note:  Doubles are used here mostly to minimize conversions        *
*     back and forth for the library functions.  A long is returned    *
*     so overflow in the ui16 stored values can be detected.)          *
*  Argument:                                                           *
*     pcnd     Ptr to conductance struct.                              *
*     il       Ptr to parent cell type (for error messages)            *
*  Returns:                                                            *
*     tc       Time until reaches cdcycut cutoff (also in pcnd->nc2dc).*
*=====================================================================*/

long dtgated(struct CONDUCT *pcnd, struct CELLTYPE *il) {

   double tc,tco;

   ICD.hcnd = pcnd->hcdnm;
   ICD.jl = il;
   if (pcnd->kcond == CDTP_ALPHA) {
      /* Decay time calculation for ALPHA function */
      double K,tpk = (1.0E3/dS20)*
         (double)pcnd->ugt.alf.ttpk/(double)RP->timestep;
      if (tpk <= 1.0) cryout(RK_E1, "0***TTPK IS <= TIMESTEP",
         RK_LN3, fmtcndnm(), RK_LN0, NULL);
      K = log(tpk*pcnd->cdcycut) - 1.0;
      tc = (K < 0.0) ? -K*tpk : tpk;
      do { tc = -(K - log(tco = tc))*tpk; }
         while (fabs(tc - tco) >= 1.0);
      }
   else {
      /* Decay time calculation for DOUBLE-EXP function.
      *  Caution: Negative td,tr simplify calculations. */
      double gcNi,lnrd;
      double td = -(double)pcnd->ugt.dbl.taud;
      double tr = -(double)pcnd->ugt.dbl.taur;
      if ((tc = td - tr) >= 0.0) cryout(RK_E1, " ***TAUD "
         "MUST BE > TAUR", RK_LN3, fmtcndnm(), RK_LN0, NULL);
      lnrd = log(tr/td)/tc;
      gcNi = pcnd->cdcycut*(exp(tr*lnrd) - exp(td*lnrd));
      tc = td*log(gcNi);
      do { tc = td*log(gcNi + exp((tco = tc)/tr)); }
         while (fabs(tc - tco) >= 1.0);
      }

   pcnd->nc2dc = (ui16)tc;
   return (long)tc;

   } /* End dtgated() */


/*=====================================================================*
*                               findion                                *
*=====================================================================*/

static struct IONTYPE *findion(txtid hion) {

   struct IONTYPE *pion,**ppion,*pjon;

   /* First check IONTYPES linked to the cell type.
   *  (Assignment intended in for condition).  */
   for (ppion=&ICD.jl->pion1; pion=*ppion; ppion=&pion->pnion) {
      if (pion->hionnm == hion) goto GotIonBlock;
      }

   /* If not found, check IONTYPES linked to the parent region */
   for (pion=ICD.jl->pback->pionr; pion; pion=pion->pnion) {
      if (pion->hionnm == hion) goto GotIonBlock;
      }

   /* If still not found, look for generic definition and copy it */
   for (pjon=RP0->piong; pjon; pjon=pjon->pnion) {
      if (pjon->hionnm == hion) {
         pion = *ppion = (struct IONTYPE *)allocpcv(Static,
            1, IXstr_IONTYPE, "perm ION data");
         *pion = *pjon;
         goto GotIonBlock;
         }
      }

   cryout(RK_E1, "***ION TYPE \"", RK_LN3, getrktxt(hion), RK_CCL,
      "\" NOT FOUND", RK_CCL, fmtcndnm(), RK_LN0, NULL);
   return NULL;

GotIonBlock:
   pion->ionflgs |= ION_USED;
   return pion;

   } /* End findion() */


/*=====================================================================*
*                             IonParmMsg                               *
*=====================================================================*/

static void IonParmMsg(char *parmnm) {

   if (ICD.FirstMsg) {
      char *rlnmmsg,regnmsg[10+LSNAME];
      if (ICD.jl)
         rlnmmsg = fmturlnm(ICD.jl);
      else
         sconvrt(rlnmmsg = regnmsg, "(8HREGION \"A" qLSN ",H\")",
            ICD.jr->sname, NULL);
      cryout(RK_E1, "0***ION TYPE \"", RK_LN3, getrktxt(ICD.hion),
         RK_CCL, "\" in ", RK_CCL, rlnmmsg, RK_CCL, NULL);
      if (parmnm) {
         cryout(RK_E1, " IS MISSING REQUIRED PARAMETERS:", RK_CCL,
            "    ", RK_LN0, NULL);
         ICD.FirstMsg = FALSE; }
      else cryout(RK_E1, "    HAS > 1 Cext LOCATION OPTION.", RK_LN0,
            NULL);
      }
   else
      cryout(RK_E1, ", ", 2, NULL);
   cryout(RK_E1, parmnm, RK_CCL, NULL);
   } /* End IonParmMsg() */


/*=====================================================================*
*                               chkion                                 *
*                                                                      *
*  N.B.  The convention is that if an ion is used, then its internal   *
*  and external concentrations are both allocated and stored, but do   *
*  not necessarily vary with time (typically, Cext might be buffered   *
*  to a constant C0ext value).                                         *
*=====================================================================*/

static void chkion(struct IONTYPE **ppion) {

   struct IONTYPE *pion;
   byte   topts;

   while (pion = *ppion) {       /* Assignment intended */

      if (pion->ionflgs & ION_USED) {
         pion->oCi = ESIZE*ICD.tionc++;   /* Allocate Cint */
         ICD.FirstMsg = TRUE;
         /* Check for redundant location options */
         topts = pion->ionopts & (ION_REGION|ION_GROUP|ION_CELL);
         if (bitcnt(&topts, 1) > 1) IonParmMsg(NULL);
         /* Ion is in use, check parameters */
         if (pion->ionflgs & (CDOP_IONINT|CDOP_IONREV)) {
            if (pion->C0int   == 0.0) IonParmMsg("C0INT");
            if (pion->Cminint == 0.0) IonParmMsg("CMININT");
            if (pion->Vint    == 0.0) IonParmMsg("VOLINT");
            if (pion->tauint  == 0.0) IonParmMsg("TAUINT");
            } /* End checking internal conc parameters */
         /* If external concentration is constant, ionflgs test will
         *  drop through and no checking or counting will be done. */
         if (pion->ionflgs & (CDOP_IONEXT|CDOP_IONREV)) {
            if (pion->C0ext   == 0.0) IonParmMsg("C0EXT");
            if (pion->Cminext == 0.0) IonParmMsg("CMINEXT");
            if (pion->Vext    == 0.0) IonParmMsg("VOLEXT");
            if (pion->tauext  == 0.0) IonParmMsg("TAUEXT");
            switch (topts) {
            case 0:              /* Set external ion default OPT=C */
               pion->ionopts = ION_CELL;
               /* ... drop through to ION_CELL case ... */
            case ION_CELL:
               ICD.tionc += 1;   /* Allocate space in cell data */
               break;
            case ION_GROUP:
               /* If by group, need to store individually */
               if (ICD.jr->niong >= BYTE_MAX)
                  d3exit("group ion", LIMITS_ERR, BYTE_MAX);
               ICD.jr->niong += 1;
               /* ... drop through to ION_REGION case ... */
            case ION_REGION:
#ifdef PAR
               /* If parallel and used and external is per group or
               *  per region and varying, ion concs will need to be
               *  exchanged across nodes.  */
               *ICD.ppnix = pion;
               ICD.ppnix = &pion->pnionexch;
               ICD.nbcst +=
                  (pion->ionopts & ION_GROUP) ? ICD.jr->ngrp : 1;
#else
               ;
#endif
               } /* End topts switch */
            } /* End checking external conc parameters */
         else {   /* External conc is constant, force regional store */
            pion->ionopts &= ~(ION_GROUP|ION_CELL);
            pion->ionopts |= ION_REGION;
            }
         if (pion->z == 0) IonParmMsg("Z");

         ppion = &pion->pnion;   /* On to next ion type */
         } /* End parameter checking */
      else {
         char *warnmsg,regnmsg[10+LSNAME];
         if (ICD.jl)
            warnmsg = fmtlrlnm(ICD.jl);
         else
            sconvrt(warnmsg = regnmsg, "(8Hregion \"A" qLSN ",H\")",
               ICD.jr->sname, NULL);
         cryout(RK_P1, "0-->WARNING: Ion type \"", RK_LN2,
            getrktxt(pion->hionnm), RK_CCL, "\" in ", RK_CCL,
            warnmsg, RK_CCL, " is not used and has been removed "
            "from the model.", RK_CCL, NULL);
         *ppion = pion->pnion;   /* Remove this ion type */
         freepv(pion, "Unused ION type");
         } /* End ion removal */

      } /* End IONTYPE loop */

   } /* End chkion() */


/*=====================================================================*
*                               d3cchk                                 *
*=====================================================================*/

void d3cchk(void) {

   struct REPBLOCK *ir;
   struct CELLTYPE *il;
   struct CONNTYPE *ix;
   struct CONDUCT  *pcnd;

   ui32   missem[2];             /* Bits for params missing/incompat */
   ui32   needem;                /* Bits for params needed */
   long   i;
   int    iet;                   /* Error type looper   */
   int    lactv;                 /* Local copy of kactv */
   int    lcond;                 /* Local copy of kcond */

   static ui32 KeysReqdByType[CDTP_N] = {
      CDKY_GBAR,                                   /* PASSIVE  */
      CDKY_GBAR|CDKY_VTH|CDKY_VMAX,                /* LINEAR   */
      CDKY_GBAR|CDKY_ACTV|CDKY_TTPK,               /* ALPHA    */
      CDKY_GBAR|CDKY_ACTV|CDKY_TAUR|CDKY_TAUD };   /* DOUBLE   */
   static ui32 KeysReqdByActv[CDAC_N] = {
      0,                                           /* CELLFIRE */
      0,                                           /* SYNAPTIC */
      CDKY_IONA,                                   /* ION      */
      CDKY_PPPC };                                 /* POISSON  */

   /* Text for printing error messages for required parameters,
   *  in left-to-right order of cdvin starting at CDKY_REQD1.  */
   static char *cndreqnm[] = { "ACTIVATION", "GBAR", "VTH", "VMAX",
      "TTPK", "TAUM", "TAUR", "TAUD", "IONACTV", "PPPC" };

   /* Text for printing error messages for missing/incompat parms */
   static char *ietmsg[2] = {
      "    THE FOLLOWING REQUIRED PARAMETER(S) ARE MISSING:",
      "    THESE PARAMETERS ARE INCOMPATIBLE WITH TYPE/ACTIVATION:" };

/*---------------------------------------------------------------------*
*  Scan all cell types and their conductances.                         *
*---------------------------------------------------------------------*/

   ICD.jr = NULL;
#ifdef PAR
   ICD.ppnix = &RP->pfionexch;   /* Prepare linked list of external */
   ICD.nbcst = 0;                /* ion types that need exchanging. */
#endif
   for (il=RP->pfct; il; il=il->pnct) {

      ICD.jl = il;
      for (pcnd=il->pcnd1; pcnd; pcnd=pcnd->pncd) {

         lactv = (int)pcnd->kactv;
         lcond = (int)pcnd->kcond;
         ICD.hcnd = pcnd->hcdnm;

/*---------------------------------------------------------------------*
*  1. If activation type is SYNAPTIC, locate the relevant CONNTYPE and *
*     set its cnflgs SCOND bit. Generate an error if the conntype num- *
*     ber was not set in iact (or is too large) or if the CONNTYPE is  *
*     marked squared or shunting (volt-dep is weird but allowable).    *
*  Rev, 01/18/07, GNR - d3go changed so phase convolution allowable.   *
*---------------------------------------------------------------------*/

         if (lactv == CDAC_SYNAPTIC) {
            int ic = pcnd->iact;
            if (ic > 0) {
               for (ix=il->pct1; ix && ix->ict != ic; ix=ix->pct) ;
               if (!ix) d3exit(  /* A development error */
                  fmturlnm(il), CONDCT_ERR, (int)pcnd->iact);
               if (ix->Cn.cnopt & (NOPSQ|NOPSH)) cryout(RK_E1,
                  "ACTIVATING CONNTYPE IS MARKED SHUNTING OR SQUARED",
                  RK_LN3, fmtcndnm(), RK_LN0, NULL);
               ix->cnflgs |= SCOND;
               }
            else {
               cryout(RK_E1, "0***INVALID SYNAPTIC ACTIVATION OUTSIDE "
                  "A CONNTYPE", RK_LN3, fmtcndnm(), RK_LN0, NULL);
               continue;
               }
            }

/*---------------------------------------------------------------------*
*  2. Be sure all required conductance parameters have been entered    *
*     and incompatible parameters have not been entered.  Accumulate   *
*     counts, etc. needed for variable allocation.                     *
*---------------------------------------------------------------------*/

         /* If cdcycut not entered, use global defaults.
         *  Note:  This is done here rather than in d3g2cond because
         *  defaults on RESPONSE FUNCTION card may have been entered
         *  after a global CONDUCTANCE card that depends on them.  */
         if (!(pcnd->cdvin & CDKY_DCUT)) pcnd->cdcycut = RP0->cdcycut;

         /* If activation type is POISSON and a seed was not entered,
         *  generate one from the master seed for the celltype.  */
         if (lactv == CDAC_POISSON && !(pcnd->cdvin & CDKY_SEED))
            pcnd->uga.psn.seed = il->ctwk.tree.mastseed + 44444L;

         /*  If activation threshold was not entered, set default */
         if (!(pcnd->cdvin & CDKY_AT)) {
            switch (lactv) {
            case CDAC_CELLFIRE:
               pcnd->uga.at = il->Ct.st;
               break;
            case CDAC_SYNAPTIC:
               /* N.B.  This is not a very good default--fix later */
               pcnd->uga.at = (long)ix->Cn.et << (FBwk-FBsi);
               break;
            default:
               break;
               } /* End default at switch */
            }

         /* If ALPHA and taum not entered, set default */
         if (lcond == CDTP_ALPHA && !(pcnd->cdvin & CDKY_TAUM))
            pcnd->ugt.alf.taum = il->Ct.taum;

         /* Check for missing or incompatible parameters */
         needem = KeysReqdByType[lcond] |
                  KeysReqdByActv[lactv];
         missem[0] = (pcnd->cdvin & needem) ^ needem;
         needem = InvCdvinByType[lcond] |
                  InvCdvinByType[lactv];
         missem[1] = pcnd->cdvin & needem;
         for (iet=0; iet<2; iet++) {
            if (needem = missem[iet]) {   /* Assignment intended */
               cryout(RK_E1, "0***", RK_LN4, "FOR CONDUCTANCE \"",
                  RK_CCL, getrktxt(pcnd->hcdnm), RK_CCL,
                  "\" ON ", 5, fmturlnm(il), RK_CCL,
                  ietmsg[iet], RK_LN0, "    ", RK_LN0, NULL);
               for (i=0; needem; i++) {
                  if (bittst((byte *)&needem, i+CDKY_REQD1)) {
                     bitclr((byte *)&needem, i+CDKY_REQD1);
                     cryout(RK_P1, cndreqnm[i], RK_CCL,
                        ", ", needem ? RK_CCL : RK_SKIP, NULL);
                     }
                  } /* End parameter name-message loop */
               } /* End if missem */
            } /* End error type loop */

         /* OPT=[IER] requires IONCON */
         if (pcnd->cdopts & (CDOP_IONREV|CDOP_IONEXT|CDOP_IONINT) &&
               !(pcnd->cdvin & CDKY_IONC))
            cryout(RK_E1, "0***A CONTROLLING ION IS REQUIRED ", RK_LN3,
               fmtcndnm(), RK_LN0, NULL);

         if (il->ncnd >= BYTE_MAX)
            d3exit("conductance", LIMITS_ERR, BYTE_MAX);
         il->ncnd += 1;    /* OK, count it */

/*---------------------------------------------------------------------*
*  3. Identify gated conductances.  Count them and compute time needed *
*     to decay to specified decay cutoff.  Error if > UI16_MAX.        *
*     Set CTFgD flag if any conductance (gated or not) has gDCY != 0   *
*---------------------------------------------------------------------*/

         if (qdecay(&pcnd->gDCY)) il->ctf |= CTFgD;
         if (lcond == CDTP_ALPHA || lcond == CDTP_DOUBLE) {
            long ngcyc = dtgated(pcnd, il);
            RP->ncndg += 1;
            if (qdecay(&pcnd->gDCY))
               pcnd->cdflgs |= CDFL_GgDCY;
            else {
               pcnd->cdflgs |= CDFL_GNDCY;
               /* Check for nc2dc overflow if not DECAY type */
               if (ngcyc > UI16_MAX) {
                  char cct[10];
                  wbcdwt(&ngcyc, cct, RK_LFTJ|RK_IORF|RK_NLONG|8);
                  cryout(RK_E1, "0***TIME TO CUTOFF (", RK_LN3, cct,
                  RK.length+1, ") EXCEEDS PROGRAM LIMIT (65535 "
                  "TIMESTEPS)", RK_CCL, fmtcndnm(), RK_LN0, NULL);
                  ngcyc = UI16_MAX;    /* To avoid further errors */
                  }
               }
            } /* End setup for gated conductances */

/*---------------------------------------------------------------------*
*  4. Be sure all referenced ion types have been defined, store ptrs   *
*     to the definitions, and turn on ION_USED.  Turn on ionflgs to    *
*     indicate whether internal or external state or both may vary.    *
*     Note:  Any ion name that was stored is checked, even if for an   *
*     option that is not used.  Such cases are very likely to be the   *
*     result of user errors.  If an ion type was defined generically   *
*     but not specifically for this cell type, copy the definition.    *
*---------------------------------------------------------------------*/

         /* Assignment intended in three following 'if's */
         if (pcnd->piond = findion(pcnd->hiond))
            pcnd->piond->ionflgs |= CDOP_IONINT;
         if (pcnd->pionr = findion(pcnd->hionr))
            pcnd->pionr->ionflgs |=
               pcnd->cdopts & (CDOP_IONEXT|CDOP_IONINT|CDOP_IONREV);
         if (pcnd->piont = findion(pcnd->uga.ion.hiont))
            pcnd->piont->ionflgs |= (pcnd->cdflgs & CDFL_ACTEXT) ?
               CDOP_IONEXT : CDOP_IONINT;

         RP0->orcdfl |= pcnd->cdflgs;
         } /* End conductance scan */

      } /* End cell type scan */

/*---------------------------------------------------------------------*
*  5. Scan ion data structures to be sure all required parameters are  *
*     present.  Count ion allocations for d3allo.  Issue warnings for  *
*     ions that are not referenced by any conductance and delete them. *
*---------------------------------------------------------------------*/

   for (ir=RP->tree; ir; ir=ir->pnrp) {
      ICD.jr = ir, ICD.jl = NULL; ICD.tionc = 0;
      chkion(&ir->pionr);
      ICD.rionc = ICD.tionc;

      for (il=ir->play1; il; il=il->play) {
         ICD.jl = il; ICD.tionc = ICD.rionc;
         chkion(&il->pion1);
         if (ICD.tionc > BYTE_MAX)
            d3exit("ion", LIMITS_ERR, BYTE_MAX);
         il->nionc = ICD.tionc;
         }
      }

/*---------------------------------------------------------------------*
*  6. Allocate space in Shared pool for broadcast of external ion      *
*     concentrations that are not local.                               *
*---------------------------------------------------------------------*/

#ifdef PAR
   if (ICD.nbcst) RP->pionxb = allocpmv(Shared,
      ICD.nbcst * FMESIZE, "Ion exchange area");
#endif

/*---------------------------------------------------------------------*
*  7. Delete the group 1 (global default) allocations.                 *
*---------------------------------------------------------------------*/

   {  struct CONDUCT *qcnd, *pcnd = RP0->pcndg;
      while (pcnd) {
         qcnd = pcnd->pncd;
         freev(pcnd, "Conductance defaults");
         pcnd = qcnd;
         }
      } /* End conductance local scope */

   {  struct CONDSET *qcns, *pcns = RP0->pcnds;
      while (pcns) {
         qcns = pcns->pncs;
         freev(pcns, "Conductance sets");
         pcns = qcns;
         }
      } /* End conductance set local scope */

   {  struct IONTYPE *qion, *pion = RP0->piong;
      while (pion) {
         qion = pion->pnion;
         freev(pion, "Ion type defaults");
         pion = qion;
         }
      } /* End ion type local scope */

   } /* End d3cchk() */
