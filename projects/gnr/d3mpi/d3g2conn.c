/* (c) Copyright 1996-2018, The Rockefeller University *11116* */
/* $Id: d3g2conn.c 77 2018-03-15 21:08:14Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                              d3g2conn                                *
*       Read group II input cards relating to connection types         *
*                                                                      *
*  When d3grp2 encounters a control card bearing connection type       *
*  parameters, it allocates and initializes a new parameter block of   *
*  the appropriate type, then calls one of the routines in this file   *
*  to process the control card.                                        *
*                                                                      *
************************************************************************
*  V8A, 10/05/96, GNR - Broken out from d3tree                         *
*  Rev, 09/16/97, GNR - Add CONNTYPE SVITMS                            *
*  Rev, 10/02/97, GNR - Independent dynamic allocations per conntype,  *
*                       allow AMPLIF card to follow CONNTYPE, SFHMATCH *
*  Rev, 12/23/97, GNR - Add ADECAY, remove c(ij) decay from PARAMS     *
*  Rev, 02/04/98, GNR - Allow OPT=AC on SUBARBOR card                  *
*  Rev, 07/30/00, GNR - Use kwscan R code to vet CONNTYPE options      *
*  Rev, 02/15/05, GNR - Force all color names to upper case            *
*  V8D, 01/15/07, GNR - Disallow phase on GCONN or MODULATE inputs     *
*  Rev, 07/20/07, GNR - Add ckkam routine                              *
*  Rev, 10/25/07, GNR - Add CONNTYPE UPROP                             *
*  Rev, 11/03/07, GNR - g2param, g2thresh can set LCnD before CONNTYPE *
*  ==>, 01/08/08, GNR - Last mod before committing to svn repository   *
*  Rev, 03/26/08, GNR - Add RNAME, RFILE, RCT to CONNTYPE card         *
*  Rev, 05/20/08, GNR - Add getthr() and calls to it                   *
*  V8E, 05/05/09, GNR - Revised getthr and all input & amp test vars.  *
*  V8F, 04/20/10, GNR - Add PP source type                             *
*  Rev, 06/16/10, GNR - Break up cnopt, add SUBARBOR OPT=I             *
*  Rev, 07/01/10, GNR - Add getnapc, calls from CONNTYPE and CHANGE    *
*  V8H, 11/23/10, GNR - Remove COMPAT=C neg test for mtj, mnsj, etc.   *
*  Rev, 03/30/11, GNR - Relax GCONN radius test from OR to AND         *
*  Rev, 05/17/12, GNR - Add GBCM, 06/30/12 Add qpull                   *
*  Rev, 04/23/13, GNR - Add mxsi to AMPLIF, PARAMS                     *
*  Rev, 05/11/13, GNR - Remove gconn falloff                           *
*  Rev, 07/17/13, GNR - Add getpspov() and calls to it                 *
*  R63, 11/03/15, GNR - Add vdopt codes                                *
*  R66, 01/30/16, GNR - Invert beta sign convention unless COMPAT=B    *
*  R66, 02/17/16, GNR - Change long random seeds to typedef si32 orns  *
*  R66, 03/11/16, GNR - Rename nvx->nsxl, nvy->nsya so != ix->nv[xy]   *
*  R66, 03/15/16, GNR - Eliminate '-' incrementing for VJ,VW,VT        *
*  R67, 04/27/16, GNR - Remove Darwin 1 support                        *
*  R72, 02/05/17, GNR - Add KGEN=K ("Kernel"), cijmin, NAPC_EXIN       *
*  R75, 09/30/17, GNR - Add g2vpfb to process TERNARY card             *
*  R77, 12/23/17, GNR - Use muscan unit controls on thresholds         *
*  R77, 12/28/17, GNR - Move getthr to its own source file             *
*  R77, 01/30/18, GNR - Add vdmm minimum vdep multiplier               *
*  R78, 03/15/18, GNR - Allow any KGEN 1st conn code with KGNKN kernel *
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
#include "bapkg.h"

/*---------------------------------------------------------------------*
*                              getpspov                                *
*  This is a kwsreg routine to read a usage override code option from  *
*  a control card and check for illegal combinations.                  *
*---------------------------------------------------------------------*/

void getpspov(byte *ppspov) {

   char tpspov[SCANLEN1];

   scanck(tpspov, RK_REQFLD, ~(RK_BLANK|RK_COMMA));
   mcodes(tpspov, okusage, NULL);
   mcodopc(ppspov);
   if ((*ppspov & (USG_EXC|USG_HYP)) == (USG_EXC|USG_HYP) ||
       (*ppspov & (USG_IIR|USG_XIR)) == (USG_IIR|USG_XIR) ||
       (*ppspov & (USG_DEP|USG_HYP)) == (USG_DEP|USG_HYP))
      ermark(RK_EXCLERR);

   } /* End getpspov() */


/*---------------------------------------------------------------------*
*                                ckkam                                 *
*  This little routine checks for conflicting rule options for use     *
*  by g2param() or d3chng().  It is no longer necessary that the       *
*  amplification rule given later should agree with that on the        *
*  AMPLIF card, individual conntypes can differ.  User is allowed      *
*  to enter code U with or without T, but not B,S,K,Z and T ==> S.     *
*---------------------------------------------------------------------*/

void ckkam(ui32 tkam) {

   ui32 tui32 = tkam & ANYAMASK;
   if (bitcnt((byte *)&tui32, sizeof(tui32)) > 1) {
      ermark(RK_EXCLERR);
      cryout(RK_E1, "0***MORE THAN ONE OF B,E,H,I,J,4,3 RULE OPTIONS.",
         RK_LN2, NULL);
      }
   tui32 = tkam & (KAMDQ|KAMPA|KAMKI);
   if (bitcnt((byte *)&tui32, sizeof(tui32)) > 1) {
      ermark(RK_EXCLERR);
      cryout(RK_E1, "0***MORE THAN ONE OF K,P,Q RULE OPTIONS.",
         RK_LN2, NULL);
      }
   if (tkam & KAMBCM && tkam & KAMKI) {
      ermark(RK_EXCLERR);
      cryout(RK_E1, "0***INVALID RULE OPTION B with K.",
         RK_LN2, NULL);
      }
   if (tkam & KAMUE && tkam & (KAMBCM|KAMSA|KAMZS|KAMKI)) {
      ermark(RK_EXCLERR);
      cryout(RK_E1, "0***INVALID RULE OPTION U with B,K,S or Z.",
         RK_LN2, NULL);
      }
   if ((tkam & (KAMWM|KAMTS|KAMSA)) == KAMWM) {
      ermark(0);
      cryout(RK_E1, "0***INVALID RULE OPTION W without S or T.",
         RK_LN2, NULL);
      }
   } /* End ckkam() */


/*---------------------------------------------------------------------*
*                               getnapc                                *
*  This is a kwsreg routine to read a neuroanatomy plot color option   *
*  from a control card.  (It is set up this way mostly to avoid a      *
*  separate switch case in d3chng()).                                  *
*---------------------------------------------------------------------*/

void getnapc(byte *napc) {

   int ic;
   static char *nacops[] = { "EXIN", "CIJ", "SJ", "MIJ", "SUBARBOR",
      "SOLID" };

   ic = match(RK_EQCHK, RK_MREQF, ~(RK_COMMA|RK_BLANK), 0,
      nacops, sizeof(nacops)/sizeof(char *));
   /* If bad input, set to default NAPC_EXIN, error will terminate
   *  CNS later anyway.  */
   *napc = (ic > 0) ? ic - 1 : NAPC_EXIN;
   } /* End getnapc() */


/*---------------------------------------------------------------------*
*                               readbc                                 *
*  This little function reads the BC (boundary condition) option on    *
*  a PREPROC, CONNTYPE, or GCONN card and returns the appropriate      *
*  selector.                                                           *
*                                                                      *
*  Argument:                                                           *
*     bcmask      A bit mask with '1' at position 1<<selector if that  *
*                 selector is valid for this call.                     *
*  Returns:  Specified member of BoundaryCdx enum or -1 if the         *
*     selector was not matched or not included in bcmask.              *
*---------------------------------------------------------------------*/

int readbc(int bcmask) {

   int bcs;                   /* BoundaryCdx selector */

   /* Names of boundary condition options (BC on PREPROC,GCONN,CONNTYPE
   *  cards).  The positions of the items in 'bckeys' must correspond
   *  to the values of the BoundaryCdx enum defined in d3global.h.  */
   static char *bckeys[BC_NKEYS] = {
      "ZERO", "NORM", "NOISE", "EDGE", "MIRROR", "TOROIDAL",
      "MEAN", "HELICAL" };

   bcs = match(RK_EQCHK, RK_MREQF, ~RK_COMMA, 0, bckeys, BC_NKEYS) - 1;
   if (bcs < 0 || (bcmask & 1<<bcs) == 0) {
      ermark(RK_IDERR);
      bcs = -1;
      }
   return bcs;

   } /* End readbc() */


/*---------------------------------------------------------------------*
*                              readmdlt                                *
*  This little function reads the MODALITY option on the CONNTYPE card *
*  and sets the cnflgs NOMDLT bit if 'NONE' is specified. Otherwise, a *
*  pointer to the named MODALITY block is stored in ix->psmdlt and the *
*  MDLTFLGS_RQST bit in that block is set, indicating it was requested *
*  explicitly.  The reserved name 'NONE' was introduced to avoid need  *
*  for additional option keywords on the CONNTYPE card.                *
*  Note:  If KGNST and hbcnm is < 0, earlier code will have already    *
*  issued an error and findmdlt() harmlessly returns NULL, so there    *
*  is no need for a separate check here for this error.                *
*---------------------------------------------------------------------*/

static void readmdlt(struct CONNTYPE *ix) {

   char mname[LTEXTF+1];      /* Temp storage for modality name */

   inform("(SA" qLTF ")", mname, NULL);   /* Get name */
   if (strcmp(mname, "NONE")) {
      int iwdw = ix->kgen & KGNST ? ix->srcnm.hbcnm : 0;
      ix->psmdlt = findmdlt(mname, iwdw);
      if (ix->psmdlt) {
         ix->psmdlt->mdltflgs |= MDLTFLGS_RQST;
         ix->cnflgs |= OVMDLT; }
      }
   else
      ix->cnflgs |= NOMDLT;

   } /* End readmdlt() */


/*---------------------------------------------------------------------*
*   Process 'GCONN' card (old name--INHIB--is also still accepted)     *
*                                                                      *
*  Synopsis:  void g2gconn(struct CELLTYPE *il, struct INHIBBLK *ib)   *
*---------------------------------------------------------------------*/

void g2gconn(struct CELLTYPE *il, struct INHIBBLK *ib) {

   struct SFHMATCH *phm = getsfhm(SFHMGCON, ib);
   struct REPBLOCK *ir = il->pback;
   ui32 radius;
   ui32 ic;                   /* kwscan field recognition codes */
   int ibc = 0;               /* Temp for boundary condition */
   int num = ib->nib;         /* Number of inhibitory bands (int) */
   int kwret;                 /* kwscan return code */
   int structsize = sizeof(struct INHIBBAND);

   ib->igt = ++il->ngct;

/* Read data from GCONN/INHIB card */

   inform("(S=V>UH,J" KWJ_THR "CR,V(+^RB20IJV))", &ib->ngb,
      &ib->itt, &structsize, &num, &ib->inhbnd->beta, NULL);

   while (kwret = kwscan(&ic,    /* Assignment intended */
         "SIZE%V>UH",            &ib->ngb,
         "GJREV%J" KWJ_THR "R",  &ib->gjrev,
         "IT%J"    KWJ_THR "CR", &ib->itt,
         "ITHI%J"  KWJ_THR "CR", &ib->itt,
         "ITLO%J"  KWJ_THR "CR", &ib->ittlo,
         "BETA%X",
         "MXIB%X",
         "BC%X",
         "OPT%KH" okibopt,       &ib->ibopt,
         "GBCM%VB24IJ",          &ib->gbcm,
         "DELAY%V<=65534UH",     &ib->ihdelay,
         "GUSAGE%N" KWS_PSPOV,   &ib->gpspov,
         "GDECAY%N" KWS_DECAY,   &ib->GDCY,
         "GDEFER%UH",            &ib->gdefer,
         "RNAME%N" KWS_RLNM,     &phm->rrennm,
         "RFILE%VIH",            &phm->fno,
         "RCT%VIH",              &phm->rct,
         NULL))

      switch (kwret) {

/* Process 'BETA' option */

case 1:  if (RK.scancode != RK_EQUALS) ermark(RK_EQRQERR);
         inform("(SV(^RB20IJV))", &structsize, &num,
            &ib->inhbnd->beta, NULL);
         break;

/* Process 'MXIB' option */

case 2:  if (RK.scancode != RK_EQUALS) ermark(RK_EQRQERR);
         inform("(SV(^RB20/27IJ$mV,V))", &structsize, &num,
            &ib->inhbnd->mxib, NULL);
         break;

/* Process 'BC' (boundary condition) option */

case 3:  ibc = readbc((int)0x3f);
         if (ibc > BC_ZERO && num <= 1) {
            cryout(RK_P1, "0-->BC ignored with nib=1.", RK_LN2, NULL);
            ibc = BC_ZERO; }
         break;

         } /* End kwret switch */

/* Other error checking for GCONN card */

   /* Area required for specified bands cannot exceed size of
   *  source layer.  (Note:  First ring (always 1 group) is not
   *  included in diam calc.  Test is >= instead of > to allow
   *  for the extra center group.)  */
   radius = (ui32)ib->ngb*(ui32)(num-1);  /* Cannot overflow */
   if ((radius >= (ui32)ir->ngx) && (radius >= (ui32)ir->ngy))
      cryout(RK_E1, "0***GCONN ZONE EXCEEDS REP SIZE.",
         RK_LN2, NULL);
   if (radius > MXIBRNGS)
      cryout(RK_E1, "0***GCONN RINGS EXCEED PGM LIMIT ("
         qqv(MXIBRNGS) ").", RK_LN2, NULL);
   ib->l1n1 = (ui16)radius;

   /* Cannot use OPT=X except with at least 2 bands and
   *  BC = < ZERO | NORM | TOROID > */
   if (ib->ibopt & IBOPTX) {
      if ((ibc > BC_NORM) && (ibc != BC_TOROIDAL))
         cryout(RK_E1, "0***OPT=X NOT VALID WITH GIVEN BC.",
            RK_LN2, NULL);
      if (num < 2)
         cryout(RK_E1, "0***NEED 2 BANDS FOR OPT=X.", RK_LN2, NULL);
      }

/* For compatibility, bring in square|shunting options from
*  CELLTYPE card, then check that don't have both at once.
*  Implement COMPAT=K and COMPAT=B.  */

   if (il->Ct.ctopt & OPTSQ) ib->ibopt |= IBOPTQ;
   if (il->Ct.ctopt & OPTSH) ib->ibopt |= IBOPTS;
   if ((ib->ibopt & (IBOPTQ|IBOPTS)) == (IBOPTQ|IBOPTS))
      cryout(RK_E1, "0***INCOMPATIBLE INHIB OPTIONS.", RK_LN2, NULL);
   if (RP->compat & OLDKNEE) ib->ibopt |= IBOPKR;
   if (RP->compat & OLDBETA) {
      int iband;
      for (iband=0; iband<num; ++iband)
         ib->inhbnd[iband].beta = -ib->inhbnd[iband].beta;
      }

/* Now set boundary condition and restore/rename options */

   /* Load boundary condition control */
   ib->kbc = (byte)ibc;

   /* Stash away any restore/rename information */
   if (!(ib->ibopt & IBOPOR)) linksfhm(phm, SFHMGCON);

   } /* End g2gconn() */


/*---------------------------------------------------------------------*
*                       Process 'MODULATE' card                        *
*                                                                      *
*  Synopses:  void g2modul(struct CELLTYPE *il, struct MODBY *im)      *
*                                                                      *
*  R66, 03/15/16, GNR:  readrlnm now checks the input index for all    *
*  kinds of virtual cells -- currently only VT_SRC and ITPSRC are      *
*  eligible for modulation -- checked in d3tchk().                     *
*---------------------------------------------------------------------*/

void g2modul(struct CELLTYPE *il, struct MODBY *im) {

   struct SFHMATCH *phm = getsfhm(SFHMMODU, im);
   ui32 ic;                   /* kwscan field recognition codes */

   ++il->nmct;                /* Count modulation blocks */

/* Read options from 'MODULATE' card */

   im->msrctyp = RP0->ksrctyp = (byte)readrlnm(&im->msrcnm, il);
   inform("(S=J" KWJ_THR "X,B20IL,J" KWJ_THR "X)",
      &im->Mc.mt, &im->mscl, &im->mto, NULL);
   kwscan(&ic,
         "COLOR%N" KWS_CSEL,     &im->Mc.mvxc,
         "MDECAY%N" KWS_DECAY,   &im->MDCY,
         "MDEFER%UH",            &im->Mc.mdefer,
         "MT%J" KWJ_THR   "X",   &im->Mc.mt,
         "MTHI%J" KWJ_THR "X",   &im->Mc.mt,
         "MTLO%J" KWJ_THR "X",   &im->Mc.mtlo,
         "MJREV%J" KWJ_THR "X",  &im->Mc.mjrev,
         "MSCL%B20IL",           &im->mscl,
         "MTO%J" KWJ_THR   "X",  &im->mto,
         "MTOHI%J" KWJ_THR "X",  &im->mto,
         "MTOLO%J" KWJ_THR "X",  &im->mtolo,
         "MXMOD%B20/27IJ$mV",    &im->mxmod,
         "MUSAGE%N" KWS_PSPOV,   &im->Mc.mpspov,
         "OPT%KC" okmodop,       &im->Mc.mopt,
         "DELAY%UC",             &im->Mc.mdelay,
         "RNAME%N" KWS_RLNM,     &phm->rrennm,
         "RFILE%VIH",            &phm->fno,
         "RCT%VIH",              &phm->rct,
         NULL);

   /* Implement COMPAT=K */
   if (RP->compat & OLDKNEE)  im->Mc.mopt |= IBOPKR;

   /* Stash away any restore/rename information */
   if (!(im->Mc.mopt & IBOPOR)) linksfhm(phm, SFHMMODU);

   } /* End g2modul() */


/*---------------------------------------------------------------------*
*                       Process 'CONNTYPE' card                        *
*                                                                      *
*  Synopsis:  void g2conn(struct CELLTYPE *il, struct CONNTYPE *ix)    *
*                                                                      *
*  N.B. (06/04/10):  It is a temporary restriction that KGNE2 is only  *
*  allowed with, and implies, third letter code 'I'.  This is because  *
*  otherwise we need to set up a whole machinery for skipping Cij      *
*  generation for the first connection, with all that implies for      *
*  maps and seed skipping.                                             *
*                                                                      *
*  Note:  As of 06/16/10, SUBARBOR options are removed from the        *
*  CONNTYPE card and can only be entered on the SUBARBOR card.         *
*  (This is because otherwise 'I' option would conflict.)              *
*---------------------------------------------------------------------*/

void g2conn(struct CELLTYPE *il, struct CONNTYPE *ix) {

   ui32 ic;                   /* kwscan field recognition codes */
   ui32 kode,tui32;
   si32 tcmn,tcmn2,tcsg;      /* Temps for cmn,cmn2,csg */
   int jsrc;                  /* Source type */
   int jt;                    /* Input type */
   int kabk,kq,kk,kwk;        /* TRUE for codes indicated */
   int kbckqm,kfstjwx,kjnoy,kjxyh;
   int klcij,kmcij,krcij;     /* Flags for L,M,R types */
   int kwret;
   int ngn1, ngn2, ngn3;      /* Option bit counts */
   char lea[SCANLEN1];

   /* CONNTYPE user function types -- at the moment, just one */
#define NCNUFN 1
   static char *ucnfn[NCNUFN] = { "GetSj" };

/* Illegal kgen codes as a function of input types (bltsnm order) */
   static ui32 badkgn[NRST+1] = {
/*REP*/  KGNST+KGNHV,
/*IA*/   KGNGP+KGNG1+KGNJN+KGNND+KGNOG+KGNST+KGNHG+KGNHV,
/*VAL*/  KGNGP+KGNG1+KGNND+KGNOG+KGNST+KGNTP+KGNHV+KGNHG+KGNBL+
         KGNCF+KGNAN+KGNKN,
/*VJ*/   KGNGP+KGNG1+KGNND+KGNOG+KGNST+KGNTP+KGNHG+KGNBL+KGNCF+
         KGNAN+KGNKN,
/*VH*/   KGNBL+KGNCF+KGNFU+KGNGP+KGNG1+KGNHG+KGNJN+KGNND+KGNOG+KGNKN+
         KGNPT+KGNAN+KGNST+KGNTP+KGNUD+KGNXO+KGNYO+KGNLM+KGNEX+KGNE2,
/*VW*/   KGNGP+KGNG1+KGNND+KGNOG+KGNST+KGNTP+KGNHV+KGNHG+KGNBL+KGNCF+
         KGNAN+KGNKN,
/*VS*/   KGNUD+KGNGP+KGNG1+KGNFU+KGNXO+KGNJN+KGNHV+KGNYO+KGNEX+KGNND+
         KGNOG+KGNHG,
/*VT*/   KGNGP+KGNG1+KGNND+KGNOG+KGNST+KGNHV+KGNHG,
/*TV*/   KGNGP+KGNG1+KGNND+KGNOG+KGNST+KGNHV+KGNHG,
/*PP*/   KGNGP+KGNG1+KGNND+KGNOG+KGNST+KGNHV+KGNHG,
/*ITP*/  KGNGP+KGNG1+KGNND+KGNOG+KGNST+KGNHV+KGNHG,
/*USR*/  KGNGP+KGNG1+KGNND+KGNOG+KGNST+KGNHG+KGNHV,
      };

/* Illegal code combinations:  Any code in second word is invalid in
*  combination with any code in the first word.  (These are in
*  addition to tests below for conflicting codes in any group.)  */
   static ui32 badkg2[][2] = {
/*AJ*/   {KGNAJ, KGNFU+KGNTP+KGNST+KGNSA },
/*PT*/   {KGNPT, KGNEX+KGNHV+KGNND+KGNTP+KGNST+KGNXO+KGNYO },
      };

/* Read source codes and determine source type from srcnm.rnm.
*  Set defaults relating to modality and restoration from a file.
*
*  N.B.  il->Ct.ctopt bit OPTOR is now set by d3fchk if no match
*  found for restoring cell-level data, therefore, this bit can no
*  longer be used also directly to control generation of connection-
*  level data.  To restore traditional behavior that user-entered
*  OPTOR implies connection-level generating, logic was added here
*  to set Cn.cnopt NOPOR bit if OPTOR bit is set.   02/08/94, GNR  */

   ix->ict = ++il->nct;
   /* Note:  Bad input detected in readrlnm leaves jsrc = BADSRC */
   jsrc = ix->cnsrctyp;

   /* Set default file and source id for restore */
   ix->cnwk.tree.sfhm = *getsfhm(SFHMCONN, ix);
   if (il->Ct.ctopt & OPTOR) ix->Cn.cnopt |= NOPOR;
   /* If source is not a repertoire, cancel inherited delay
   *  option to eliminate error in d3tchk.  It is still an
   *  error to enter an explicit DELAY card after CONNTYPE. */
   if (jsrc != REPSRC) ix->Cn.kdelay = 0;
   /* If source is VALSRC, copy type number (-1) to x offset,
   *  making this a default if offset not entered.  */
   if (jsrc == VALSRC) ix->loff = (si32)ix->srcnm.hbcnm - 1;

/* Read kgen, apply kgen defaults, and check for type conflicts.
*  R72, 02/13/17, GNR - Code '2' is now allowed with 'K' or 'E' so
*     code that enforced 'E' with '2' has been removed.  Temporary
*  restriction documented above is enforced in this code block.  */

   inform("(SXA" qDML ")", lea, NULL);    /* Skip nc, read kgen */
   /* This is always an original key, no RK_PMEQPM here */
   mcodes(lea, okcnkgen, &kode);
   tui32 = kode & (KGNE2+KGNMC+KGNRC+KGNLM);
   ngn1 = bitcnt((byte *)&tui32, sizeof(tui32));
   tui32 = kode & (KGNEX+KGNGP+KGNG1+KGNND+KGNOG+KGNUD+KGNTP+
      KGNST+KGNFU+KGNXO+KGNYO+KGNJN+KGNHV+KGNHG);
   ngn2 = bitcnt((byte *)&tui32, sizeof(tui32));
   tui32 = kode & (KGNAJ+KGNBL+KGNCF+KGNAN+KGNIN+KGNPT+KGNKN);
   ngn3 = bitcnt((byte *)&tui32, sizeof(tui32));

   if (!ngn1) {
      if (kode & (KGNEX|KGNKN)) kode |= KGNE2;
      else kode |= KGNRC;
      }

   if (!ngn2) {
      if (kode & KGNKN) kode |= KGNG1;
      else if (jsrc == VALSRC || kode & KGNLM) kode |= KGNXO;
      else if (kode & KGNMC) kode |= KGNTP;
      else kode |= KGNUD;
      }

   if (!ngn3) {
      if (kode & (KGNST+KGNTP)) kode |= KGNBL;
      else if (kode & KGNHV) kode |= KGNAJ;
      else kode |= KGNIN;
      }

   if ((ngn1|ngn2|ngn3) > 1) {
      ermark(RK_EXCLERR);
      cryout(RK_P1, "0***CONFLICTING KGEN CODES.", RK_LN2, NULL);
      }

/* Make general tests for invalid combinations of codes and inputs.
*  Do these before any more input so ermark() points to error.  */

   /* Check for illegal source type vs Lij-generating combos */
   if (kode & badkgn[jsrc]) {
      ermark(RK_MARKFLD);
      cryout(RK_E1, "0***INVALID COMBINATION OF SRCID AND KGEN--",
         RK_LN2, "        SEE TABLE IN WRITEUP.", RK_LN1, NULL);
      }

   /* Check for illegal option combos */
   for (jt=0; jt<(sizeof(badkg2)/(2*sizeof(ui32))); jt++) {
      if ((kode & badkg2[jt][0]) && (kode & badkg2[jt][1])) {
         ermark(RK_MARKFLD);
         cryout(RK_E1, "0***INVALID COMBINATION OF (FIRST-LIJ OR "
            "CIJ) AND SUBSEQUENT-LIJ GENERATING CODES--", RK_LN2,
            "        SEE TABLE IN WRITEUP.", RK_LN1, NULL);
         }
      } /* End checking kgen codes */

   if (kode & KGNE2 && !(kode & (KGNEX|KGNKN))) {
      ermark(RK_MARKFLD);
      cryout(RK_E1, "0***KGEN CODE '2' REQUIRES 'K' or 'E'.",
         RK_LN2, NULL);
      }

/* Preliminary error checking based on generating code and source */

   /* Turn on T if S is requested.  To eliminate errors later if old
   *  'IA' input was specified instead of 'VS', change it now.  */
   if (kode & KGNST) {
      kode |= KGNTP;
      jsrc = ix->cnsrctyp = VS_SRC;
      }
   /* Turn on KGNBV if W is requested with USRSRC */
   if (kode & KGNHV && jsrc == USRSRC) kode |= KGNBV;

   /* Save tests that will be repeated often */
   klcij   = kode & KGNLM;
   kmcij   = kode & KGNMC;
   krcij   = kode & KGNRC;
   kabk    = kode & (KGNAJ|KGNBL|KGNKN);
   kbckqm  = kode & (KGNBL|KGNCF|KGNKN|KGNAN|KGNMC);
   kq      = kode & KGNAN;
   kfstjwx = kode & (KGNFU|KGNST|KGNTP|KGNJN|KGNXO|KGNYO|KGNBV);
   kjnoy   = kode & (KGNJN|KGNND|KGNOG|KGNYO);
   kjxyh   = kode & (KGNJN|KGNXO|KGNYO|KGNHV);
   kk      = kode & KGNKN;
   kwk     = kode & (KGNKN|KGNHV);

   /* Error checking for W */
   if (kode & KGNHV) {
      /* For compatibility, allow VJ_SRC or IA_SRC */
      if (jsrc == VJ_SRC || jsrc == IA_SRC) jsrc = VH_SRC;
      if (jsrc == VH_SRC || jsrc == USRSRC) {
         if (ix->nc != 1) {
            ermark(RK_MARKFLD);
            cryout(RK_P1, "0***NC=1 REQUIRED WITH CODE W.",
               RK_LN2, NULL); }
         }
      else { /* Non-VH (hand detector) Wallace not supported */
         ermark(RK_MARKFLD);
         cryout(RK_P1, "0***WALLACE NO LONGER SUPPORTED.",
            RK_LN2, NULL); }
      } /* End of KGEN=W error checking */

   /* Warning if regenerating and Cij or Lij not randomized */
   if (il->Ct.ctopt & OPTRG) {
      if (kode & (KGNLM|KGNMC|KGNE2)) {
         ermark(RK_WARNING+RK_MARKFLD);
         cryout(RK_P1, "0-->WARNING: Regeneration will not change "
         "Cij of types L,M,2.", RK_LN2, NULL); }
      if (kode & (KGNEX|KGNHV|KGNXO|KGNYO|KGNKN|KGNG1)) {
         ermark(RK_WARNING+RK_MARKFLD);
         cryout(RK_P1, "0-->WARNING: Regeneration will not change "
         "Lij of types E,K,W,X,Y,1.", RK_LN2, NULL); }
      }

/* Store the final generation code, then establish
*  defaults that depend on value of kgen code.  */

   ix->kgen = kode;
   cndflt2(il, ix);

/* Now read remaining parameters.
*
*  Note:  The 'R' code in kwscan is now used to check that each
*     parameter is entered only when valid.  This eliminates the
*     three separate calls to kwscan formerly used for L,M,R cases.
*     Color is not checked here--source bits are not set yet.  */

   if (klcij) {
      /* Input for 'L' ('lambda' or motor map) coefficients */
      tcmn = tcmn2 = tcsg = -1;     /* For old-style input */
      inform("(S=V>1<=16IHB24IJVB28IJB24IJ)", &ix->Cn.nbc, &tcmn,
         &tcsg, &tcmn2, NULL);
      }
   else if (kmcij) {
      /* Input for 'M' (matrix) coefficients */
      inform("(S=V>1<=16IH,VI,2*V>I)", &ix->Cn.nbc,
         &ix->ucij.m.mno, &ix->ucij.m.nks, &ix->ucij.m.nkt, NULL);
      /* Warning if Lij is not a map */
      if (!(kode & (KGNAJ|KGNBL)))
         cryout(RK_P1, "0-->WARNING: Type M Cijs usually need "
            "A,B maps.", RK_LN2, NULL);
      }
   else if (kode & KGNE2) {
      /* Input for '2' (external or kernel) coefficients */
      inform("(S=V>1<=16IH)", &ix->Cn.nbc, NULL);
      }
   else {
      /* Input for 'R' (random) or '2' coefficients */
      tcmn = tcsg = -1;
      inform("(S=V>1<=16IHVB24IJVB28IJVB16IJ)",&ix->Cn.nbc,
         &tcmn, &tcsg, &ix->ucij.r.pp, NULL);
      }

   while (kwret = kwscan(&ic,    /* Assignment intended */
         "BITS%V>1<=16IH",             &ix->Cn.nbc,
         "C00%RB24IJ",    klcij,       &ix->ucij.l.c00,
         "C01%RB24IJ",    klcij,       &ix->ucij.l.c01,
         "C10%RB24IJ",    klcij,       &ix->ucij.l.c10,
         "C11%RB24IJ",    klcij,       &ix->ucij.l.c11,
         "CMN2%RB24IJ",   klcij,       &tcmn2,
         "MEAN%RV<1B24IJ",klcij|krcij, &tcmn,
         "SIGMA%RVB28IJ", klcij|krcij, &tcsg,
         "PP%RB16IJ",     krcij,       &ix->ucij.r.pp,
         "POSFRAC%RB16IJ",krcij,       &ix->ucij.r.pp,
         "SET%RVI",       kmcij,       &ix->ucij.m.mno,
         "NKS%RV>I",      kmcij,       &ix->ucij.m.nks,
         "NKT%RV>I",      kmcij,       &ix->ucij.m.nkt,
         "BC%X",
         "COLOR%N" KWS_CSEL3,          &ix->cnxc,
         "OPT%KH" okcnopt,             &ix->Cn.cnopt,
         "CNOPT%KH" okcnopt,           &ix->Cn.cnopt,
         "KNOPT%RKC" okcnkopt, kk,     &ix->knopt,
         "SAOPT%KC" oksaopt,           &ix->Cn.saopt,
         "MAPOPT%RKH" okcnmop, klcij,  &ix->ucij.l.flags,
         "MODALITY%X",
         "NAPC%N" KWS_NAPC,            &ix->Cn.napc,
         "NHX%RIH",       kq,          &ix->nhx,
         "NHY%RIH",       kq,          &ix->nhy,
         "NRX%RVUH",      kbckqm,      &ix->nrx,
         "NRY%RVUH",      kbckqm,      &ix->nry,
         "NUX%RVIL",      kfstjwx,     &ix->nux,
         "NUY%RVIL",      kfstjwx,     &ix->nuy,
         "OFFSET%RIJ",    kjxyh,       &ix->loff,
         "SUSAGE%N" KWS_PSPOV,         &ix->spspov,
         "STRIDE%RIJ",    kabk,        &ix->stride,
         "SVITMS%KJ" oksimcn,          &ix->svitms,
         "UPROG%X",
         "VDOPT%KC" okvdopt,           &ix->Cn.vdopt,
         "WIDTH%RVB1IJ",  kjnoy,       &ix->lsg,
         "RADIUS%RV>B16UJ", kwk,       &ix->hradius,
         "XOFF%RIJ",      kfstjwx,     &ix->xoff,
         "YOFF%RIJ",      kfstjwx,     &ix->yoff,
         "RNAME%N" KWS_RLNM,           &ix->cnwk.tree.sfhm.rrennm,
         "RFILE%VIH",     &ix->cnwk.tree.sfhm.fno,
         "RCT%VIH",       &ix->cnwk.tree.sfhm.rct,
         NULL))

      switch (kwret) {

         /* Process 'BC' (boundary condition) option.
         *  (Note:  This option is currently not used) */
case 1:  ix->cbc = (char)readbc((int)0x7e);
         break;

         /* Process 'MODALITY' option */
case 2:  readmdlt(ix);
         break;

         /* Process 'UPROG' option */
case 3:  switch (match(0, RK_MREQF, ~RK_COMMA, RK_INPARENS,
            ucnfn, NCNUFN)) {
         case 1:           /* GetSj function */
            ix->sjup = (si32 (*)())d3uprg(TRUE);
            break;
         default: while (RK.scancode & RK_INPARENS) scan(NULL, 0) ;
            scanagn();
            break;
            }
         break;

         } /* End kwret switch */

   if (klcij) {                  /* Final fixup for code L */
      if (ix->ucij.l.flags & LOPTW) {
         if (ix->ucij.l.flags & ~LOPTW)
            cryout(RK_P2, "0-->WARNING: Other MAPOPT codes "
               "ignored with MAPOPT=W.", RK_LN2, NULL);
         }
      else {
         /* If old-style constants were entered, use them */
         if (tcmn != -1)  ix->ucij.l.c00 = ix->ucij.l.c01 = tcmn;
         if (tcmn2 != -1) ix->ucij.l.c10 = ix->ucij.l.c11 = tcmn2;
         if (tcsg != -1)  ix->ucij.l.csg = tcsg;
         }
      }
   else if (krcij) {             /* Final fixup for code R */
      if (tcmn != -1)  ix->ucij.r.cmn = tcmn;
      if (tcsg != -1)  ix->ucij.r.csg = tcsg;
      if (ix->ucij.r.pp > (S16-1)) ix->ucij.r.pp = (S16-1);
      }

/* Final error checking for CONNTYPE card.
*  R66, 03/17/16, GNR - Note that readrlnm() now checks that requested
*  arms, windows, etc. exist so code here now only needs to store arm
*  and window indexes and perform any more specific checking.  */

   switch (jsrc) {

   /* For input from a value scheme, check that it exists.  */
   case VALSRC:
      if (ix->loff >= RP->nvblk)
         cryout(RK_E1, "0***OFFSET > NUM OF VALUES", RK_LN2, NULL);
      break;

   /* For input from VJ or VT, save arm number */
   case VJ_SRC:
   case VT_SRC:
      ix->armi = (byte)(ix->srcnm.hbcnm - 1);
      break;

   /* For input from a window, save window number */
   case VW_SRC:
   case VS_SRC:
      ix->wdwi = (byte)(ix->srcnm.hbcnm - 1);
      break;

   /* For input from VH (not USRSRC), if 'loff' option is given,
   *  be sure specified window exists (readrlnm() now checks arm
   *  number).  Store arm and window numbers for later use.   */
   case VH_SRC:
      ix->armi = (byte)(ix->srcnm.hbcnm - 1);
      /* ix->loff == 0 is not an error--means to use whole IA */
      if (ix->loff > (int)RP->nwdws) {
         cryout(RK_E1, "0***HAND VIS. WINDOW NUM EXCEEDS "
            "NUM OF WINDOWS.", RK_LN2, NULL);
         jsrc = BADSRC; }
      else if (ix->loff > 0)
         ix->wdwi = (byte)(ix->loff - 1);
      break;

   /* Checking for other cases is in d3tchk() */
   default:
      break;

      } /* End jsrc switch */

/* Further tests relating to nux,nuy,loff,xoff,yoff,nhx,nhy,nsxl,nsya
*     removed to d3tchk 11/16/91.  All code to locate source blocks
*     in ix->psrc removed to d3tchk 04/22/95.  */

/* Update quantities derived from quantities on CONNTYPE card */

   ix->cnsrctyp = (byte)jsrc;
   } /* End g2conn */


/*---------------------------------------------------------------------*
*             Process 'PARAMETERS' and 'THRESHOLDS' cards              *
*                                                                      *
*  Synopsis:  void g2param(struct CONNTYPE *ix, struct CNDFLT *pdd,    *
*             int iamthr);                                             *
*  Where: 'iamthr' is TRUE if this was a THRESHOLDS card               *
*                                                                      *
*  Note:  As of 07/28/92, new decay parameters now go on the DECAY     *
*  card.  For compatibility, decay parameters already on the PARAMS    *
*  card were left here, but these are deprecated and can be removed.   *
*                                                                      *
*  Note:  As of 02/24/97, DELAY parameter is renamed RETARD to avoid   *
*  confusion with DELAY card delay.                                    *
*  As of 12/23/97, GAMMA and EQUIL are removed, ADECAY is added.       *
*                                                                      *
*  Note:  As of 11/04/07, the THRESHOLDS and PARAMETERS cards are      *
*  synonyms:  each has all the keyword options, but keeps the original *
*  positional options of the separate predecessors.                    *
*                                                                      *
*  Note:  As of 03/26/08, the RFILE, RNAME, and RCT parameters were    *
*  moved to the CONNTYPE card (they were originally here due to a      *
*  limit on the number of parameters in one call on the NCUBE).        *
*  As of 04/21/11, these deprecated parameters were removed from the   *
*  THRESHOLDS and PARAMETERS cards.                                    *
*---------------------------------------------------------------------*/

void g2param(struct CONNTYPE *ix, struct CNDFLT *pdd, int iamthr) {

   ui32 ic;                   /* kwscan field recognition codes */
   int hampf = 0;             /* Temp for PAF name locator */
   int hpcfn = 0;             /* Temp for PCF name locator */
   int kwret;                 /* kwscan return code        */
   int oldbssel = RK.bssel;

   if (!(RP->compat & OLDRANGE)) bscomset(RK_BSUNITS);
   if (iamthr) {
      /* Positional parameters for THRESHOLDS card */
      inform("(S=J" KWJ_THR "XA,J" KWJ_THR "XD,J" KWJ_THR "SA)",
         &pdd->et, &pdd->mtj, &pdd->scl, NULL);
      }
   else {
      /* Positional parameters for PARAMETERS card.
      *  All but first 6 params removed, 07/25/92--useless code.
      *  mtj added when merged with THRESHOLDS, 11/04/07.  */
      inform("(S=VB28IJ,J" KWJ_THR "CR,J" KWJ_THR "XD,B" qqv(FBvl)
         "IH,J" KWJ_THR "CR,J" KWJ_THR "XC)",
         &pdd->mdelta, &pdd->mti, &pdd->mtj, &pdd->mtv,
         &pdd->mnsi, &pdd->mnsj, NULL);
      }

   while (kwret = kwscan(&ic,    /* Assignment intended */
         "ADECAY%N" KWS_DECAY,   &pdd->ADCY,
         "AMPF%T"  qLTF,         &hampf,
         "CMIN%VB15IJ",          &pdd->cijmine,
         "DELTA%VB28IJ",         &pdd->mdelta,
         "ET%J" KWJ_THR "XA",    &pdd->et,
         "ETHI%J" KWJ_THR "XA",  &pdd->et,
         "ETLO%J" KWJ_THR "XB",  &pdd->etlo,
         "MNAX%VB20/27IJ$?mV",   &pdd->mnax,
         "MXAX%VB20/27IJ$?mV",   &pdd->mxax,
         "MNSI%J" KWJ_THR "CR",  &pdd->mnsi,
         "MNSJ%J" KWJ_THR "XC",  &pdd->mnsj,
         "MTI%J" KWJ_THR "CR",   &pdd->mti,
         "MTICKS%V>IH",          &pdd->mticks,
         "MTJ%J" KWJ_THR "XD",   &pdd->mtj,
         "MTV%B" qqv(FBvl) "IH", &pdd->mtv,
         "EMXSJ%J" KWJ_THR "XG", &pdd->emxsj,
         "MXIMG%VB20/27IJ$?mV",  &pdd->mximg,
         "MXMIJ%VB14IH$?7mV",    &pdd->mxmij,
         "MXMP%VB20IJ$?7mV",     &pdd->mxmp,
         "MXSI%J" KWJ_THR "CR",  &pdd->mxsi,
         "OPT%KH" okcnopt,       &pdd->cnopt,
         "CNOPT%KH" okcnopt,     &pdd->cnopt,
         "SAOPT%KC" oksaopt,     &pdd->saopt,
         "PHASE%X",
         "QPULL%V<=1B15UH",      &pdd->qpull,
         "RDAMP%V<=1B15UH",      &pdd->rdamp,
         "RETARD%VIC",           &pdd->retard,
         "RULE%KL" okcnkam,      &pdd->kam,
         "SCALE%J" KWJ_THR "SA", &pdd->scl,
         "SCLMUL%VB" qqv(FBsc) "IJ", &pdd->sclmul,
         "SJMIN%J" KWJ_THR "XA", &pdd->et,
         "SJPCF%T" qLTF,         &hpcfn,
         "SJREV%J" KWJ_THR "XE", &pdd->sjrev,
         "TCWIDTH%V>F",          &pdd->tcwid,
         "UPSM%B16IJ",           &pdd->upsm,
         "VDHA%B20/27IJ$?mV",    &pdd->vdha,
         "VDMM%B" qqv(FBsc) "IJ",&pdd->vdmm,
         "VDOPT%KC" okvdopt,     &pdd->vdopt,
         "VDT%B20/27IJ$?mV",     &pdd->vdt,
         "VI%VIH",               &pdd->vi,
         "VT%VB" qqv(FBvl) "IH", &pdd->mtv,
         "WAY%X",
         "ZETAM%VB16IJ",         &pdd->zetam,
         NULL))
      switch (kwret) {

case 1:     /* PHASE option */
         eqscan(&pdd->phi, "VIC", RK_EQCHK);
         pdd->phi &= PHASE_MASK;
         pdd->cnopt |= PHASC;
         break;
case 2:     /* WAY option */
         getway(pdd);
         break;

         } /* End switch and while */
   bscomset(oldbssel);

   /* Error checking for conflicting rule options */
   ckkam(pdd->kam);

   /*  Locate specified phase convolution functions */
   if (hampf) pdd->pamppcf = findpcf(hampf);
   if (hpcfn) pdd->ppcf = findpcf(hpcfn);

   /* Rev, 11/03/07, GNR - Code dealing with checking options
   *  related to phased input removed to d3tchk because options
   *  now can be entered on CELLTYPE and CONNTYPE cards as well
   *  as here.  */

   } /* End g2param() */


/*---------------------------------------------------------------------*
*                        Process 'SEEDS' card                          *
*                                                                      *
*  Synopsis:  void g2seeds(struct CONNTYPE *ix)                        *
*---------------------------------------------------------------------*/

void g2seeds(struct CONNTYPE *ix) {

   ui32 ic;                   /* kwscan field recognition codes */

/* Read data from SEEDS card */

   inform("(SW1=4*VIJ)", &ix->cseed, &ix->lseed, &ix->phseed,
            &ix->rseed, NULL);
   kwscan(&ic,
      "CSEED%VIJ", &ix->cseed,
      "LSEED%VIJ", &ix->lseed,
      "NSEED%VIJ", &ix->phseed,
      "RSEED%VIJ", &ix->rseed, NULL);
   } /* End g2seeds() */


/*---------------------------------------------------------------------*
*                       Process 'SUBARBOR' card                        *
*                                                                      *
*  Synopsis:  void g2subarb(struct CONNTYPE *ix)                       *
*---------------------------------------------------------------------*/

void g2subarb(struct CONNTYPE *ix) {

   ui32 ic;                   /* kwscan field recognition codes */

/* Read data from SUBARBOR card */

   inform("(SW1=V>UJV>UJ)", &ix->nsa, &ix->nsax, NULL);
   kwscan(&ic,
      "NSA%V>UJ",       &ix->nsa,
      "NSAX%V>UJ",      &ix->nsax,
      "OPT%KC" oksaopt, &ix->Cn.saopt,
      NULL);

   /* Temporarily turn DOSARB on, for if all the numeric values are
   *  defaulted, this is only way d3tchk can know a SUBARB card was
   *  in fact entered.  DOSARB will get turned back off in d3tchk
   *  if the numeric values are such that nsa and nc are equal.
   *  Error checks moved to d3tchk, 06/17/10, because saopt bits
   *  can be modified on CONNTYPE, PARAMS, and SUBARBOR cards.  */
   ix->Cn.saopt |= DOSARB;
   } /* End g2subarb() */


/*---------------------------------------------------------------------*
*                       Process 'TERNARY' card                         *
*                                                                      *
*  Synopsis:  void g2vpfb(struct CONNTYPE *ix)                         *
*                                                                      *
*  Note:  Ultimately, this should be replaced with a more general      *
*  ternary synapse control whereby any other real or virtual cell type *
*  can be used to multiply the input from the current conntype.  This  *
*  will need all sorts of design work, particularly, how to locate the *
*  correct cell that affects the current cell, whether maybe this can  *
*  be treated as a variant of a voltge-dependent connection, etc. The  *
*  current code avoids all this by using a reference to earlier conn-  *
*  type on the same cell to locate the multiplier input.  (This must   *
*  be earlier so d3go will have already obtained the Sj.)              *
*                                                                      *
*  d3grp2() constructs ix->pvpfb ptr to TERNARY data structure so if   *
*  multiple cards received, only data from last one are saved.         *
*---------------------------------------------------------------------*/

void g2vpfb(struct CONNTYPE *ix) {

   struct TERNARY *pvp = ix->pvpfb;
   ui32 ic;                   /* kwscan field recognition codes */
   kwscan(&ic,
      "MODSCALE%V>B" qqv(FBsc) "IJ",  &pvp->modscl,
      "BCKSCALE%V>B" qqv(FBsc) "IJ",  &pvp->bckscl,
      "BCKGND%V>B"   qqv(FBtm) "IJ",  &pvp->bckgnd,
      "MXIMAGE%V>B"  qqv(FBIm) "IJ",  &pvp->mximage,
      NULL);

   /* Make sure the input is available from an earlier conntype */
   if (pvp->vpict >= ix->ict) cryout(RK_E1, "0***TERNARY must access "
      "input from an earlier connection type.", RK_LN2, NULL);

   } /* End g2vpfb() */





