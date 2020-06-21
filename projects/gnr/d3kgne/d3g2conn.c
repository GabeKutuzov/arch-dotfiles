/* (c) Copyright 1996-2012, Neurosciences Research Foundation, Inc. */
/* $Id: d3g2conn.c 51 2012-05-24 20:53:36Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                              d3g2conn                                *
*                                                                      *
*  Read group II input cards relating to connection types              *
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
*  Rev, 05/17/12, GNR - Add GBCM                                       *
***********************************************************************/

#define TVTYPE  void
#define D1TYPE  struct D1BLK

#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "d3global.h"
#include "d3opkeys.h"
#ifdef D1
#include "d1def.h"
#endif
#include "rocks.h"
#include "rkxtra.h"
#include "bapkg.h"

/*---------------------------------------------------------------------*
*                                ckkam                                 *
*  This little routine checks for conflicting rule options for use     *
*  by g2param() or d3chng().  It is no longer necessary that the       *
*  amplification rule given later should agree with that on the        *
*  AMPLIF card, individual conntypes can differ.  User is allowed      *
*  to enter code U with or without T, but not S and T ==> S.           *
*                                                                      *
*  Add LHF, BCM, etc. vs Hebb consistency check here when any such     *
*  rules are defined.                                                  *
*---------------------------------------------------------------------*/

void ckkam(long tkam) {

   long tlong = tkam & ANYAMASK;
   if (bitcnt((byte *)&tlong, sizeof(tlong)) > 1) {
      ermark(RK_EXCLERR);
      cryout(RK_E1, "0***MORE THAN ONE OF E,H,I,J,4,3 RULE OPTIONS.",
         RK_LN2, NULL);
      }
   if (tkam & KAMUE && tkam & (KAMSA|KAMZS|KAMKI)) {
      ermark(RK_EXCLERR);
      cryout(RK_E1, "0***INVALID RULE OPTION U with K, S or Z.",
         RK_LN2, NULL);
      }
   if (tkam & (KAMWM|KAMTS|KAMSA) == KAMWM) {
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
   static char *nacops[] = { "CIJ", "SJ", "MIJ", "SUBARBOR" };

   ic = match(RK_EQCHK, RK_MREQF, ~(RK_COMMA|RK_BLANK), 0,
      nacops, sizeof(nacops)/sizeof(char *));
   /* If bad input, set to default NAPC_CIJ, error will terminate
   *  CNS later anyway.  */
   if (ic > 0) ic -= 1;
   *napc = (byte)ic;
   } /* End getnapc() */


/*---------------------------------------------------------------------*
*                               getthr                                 *
*  This is a kwjreg routine to read a threshold value from a control   *
*  card.  It takes account of (1) whether or not negative values are   *
*  allowed, and (2) whether the scale is B7/14, B8, or depends on the  *
*  input source.  These cases are coded in the first argument as a     *
*  text string that can be passed from kwscan or inform, as follows:   *
*  (1) The first character is 'V' if negatives are never allowed, 'C'  *
*  if negatives are not allowed in COMPAT=C mode, otherwise no code.   *
*  (2) The second character is 'U' if the variable is a value type     *
*  (B8IH), 'R' if the input is known to be a repertoire type but the   *
*  scale depends on COMPAT=C (i.e.  GCONNs), or 'X' if the scale       *
*  depends on the input source.  In this case, if the source (code     *
*  stored in RP0->ksrctyp) is virtual input (IA, TV, sense, etc.), the *
*  scale is always S14.  If the source is a repertoire, the scale is   *
*  S14 on OLDRANGE, otherwise S7 with possible mV units modifier.  If  *
*  the input scale is not yet known, as indicated by ksrctyp ==        *
*  BADSRC, the value is stored temporarily in the RP0->dfthr array for *
*  later scaling and truncation.                                       *
*---------------------------------------------------------------------*/

void getthr(char *pfmt, short *thresh) {        /* kwjreg J1 */

   char *pgf;
   int novck = 1;
   /* Define names for the different format cases and the eqscan()
   *  format codes for each--the 'V' is skipped over if not needed */
#define GTT_MVRANGE  0
#define GTT_OLDRANGE 1
#define GTT_VALRANGE 2
#define GTT_UNKRANGE 3
   static char *gtfmt[] =
      { "VB7IH$mV", "VB14IH", "VB8IH", "VB14IL$mV" };

   /* Check for negative test */
   if (pfmt[0] == 'V')
      novck = 0, ++pfmt;
   else if (pfmt[0] == 'C') {
      if (RP->compat & OLDRANGE) novck = 0;
      ++pfmt; }

   /* Select format string according to last code */
   switch (pfmt[0]) {
   case 'X':               /* Input type may not be known yet */
      if (RP0->ksrctyp == BADSRC) {
         si32 *pdft;
         int ith = RP0->ndfth++;
         if (RP0->ndfth > NCNThr)
            d3exit(LIMITS_ERR, "Threshold", NCNThr);
         RP0->dfthr[ith].pthr = thresh;
         pdft = &RP0->dfthr[ith].s14thr;
         pgf = gtfmt[GTT_UNKRANGE] + novck;
         eqscan(pdft, pgf, RK_EQCHK|RK_BEQCK);
         return;
         }
      if (RP0->ksrctyp != REPSRC) {
         pgf = gtfmt[GTT_OLDRANGE];
         break;
         }
      /* If repertoire input, fall into case 1 ... (no break) */
   case 'R':               /* Repertoire input test */
      pgf = RP->compat & OLDRANGE ?
         gtfmt[GTT_OLDRANGE] : gtfmt[GTT_MVRANGE];
      break;
   case 'U':               /* Value test */
      pgf = gtfmt[GTT_VALRANGE];
      break;
   default:                /* Error if bad format passed */
      d3exit(GETTHR_ERR, pfmt, 0);
      } /* End format code switch */
   eqscan(thresh, pgf + novck, RK_EQCHK|RK_BEQCK);

   } /* End getthr() */


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
*                               readlid                                *
*  This little function checks whether an index number for a virtual   *
*  input type was coded as a minus sign, as indicated by a '-1' left   *
*  in the rlnm.hbcnm field by readrlnm.  If so, it increments the      *
*  appropriate global index stored in RP0 (second argument) and stores *
*  the binary value in hbcnm and the printable value in lnm for use by *
*  d3tchk.  It generates an error and returns -1 if the index exceeds  *
*  the maximum number of items of its type.                            *
*                                                                      *
*  N.B.  Revised, 02/13/97 to use ibcdin/ibcdwt instead of sinform/    *
*  sconvrt to avoid losing cdscan state when called during parsing.    *
*  Changed first arg from ix to rlnm so can use with MODUL types too.  *
*---------------------------------------------------------------------*/

int readlid(rlnm *idtyp, short *itemnum, int limit, char *name) {

   si16 index = idtyp->hbcnm;

   if (index < 0) {           /* Replace -1 with next index */
      idtyp->hbcnm = index = ++(*itemnum);
      ibcdwt(RK_IORF+RK_LFTJ+LSNAME-1, idtyp->lnm, (long)index);
      if (RK.length < LSNAME-1) idtyp->lnm[RK.length+1] = '\0';
      }

   if (index > limit) {
      ermark(RK_MARKDLM);
      cryout(RK_P1, "0***SOURCE INDEX EXCEEDS NUMBER OF ",
         RK_LN2, name, RK_CCL, NULL);
      index = -1; }

   return index;
   } /* End readlid() */


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
   int ifo = 0;               /* Temp for falloff */
   int num = ib->nib;         /* Number of inhibitory bands (int) */
   int kwret;                 /* kwscan return code */
   int structsize = sizeof(struct INHIBBAND);

   /* Names of falloff options */
   static char *foffkeys[NFOFFOPTS] = {
      "1/D", "1/DSQ", "E-D", "E-DSQ",
      "1/R", "1/RSQ", "E-R", "E-RSQ",
      "1/S", "1/SSQ", "E-S", "E-SSQ" };

   ib->hlfrad = (float)max(ir->ngx, ir->ngy);
   ib->igt = ++il->ngct;

/* Read data from GCONN/INHIB card */

   inform("(S=V>IH,J" KWJ_THR "CR,V(+^RB20IJV))", &ib->ngb,
      &ib->itt, &structsize, &num, &ib->inhbnd->beta, NULL);

   while (kwret = kwscan(&ic,    /* Assignment intended */
         "SIZE%V>IH",            &ib->ngb,
         "GJREV%J" KWJ_THR "R",  &ib->gjrev,
         "IT%J"    KWJ_THR "CR", &ib->itt,
         "ITHI%J"  KWJ_THR "CR", &ib->itt,
         "ITLO%J"  KWJ_THR "CR", &ib->ittlo,
         "HR%F.0",               &ib->hlfrad,
         "BETA%X",
         "MXIB%X",
         "BC%X",
         "FOFF%X",
         "OPT%KH" okibopt,       &ib->ibopt,
         "GBCM%VB24IJ",          &ib->gbcm,
         "DELAY%VIH",            &ib->ihdelay,
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
         inform("(SV(^RB20/27IL$mV,V))", &structsize, &num,
            &ib->inhbnd->mxib, NULL);
         break;

/* Process 'BC' (boundary condition) option */

case 3:  ibc = readbc((int)0x3f);
         if (ibc > BC_ZERO && num <= 1) {
            cryout(RK_P1, "0-->BC ignored with nib=1.", RK_LN2, NULL);
            ibc = BC_ZERO; }
         break;

/* Process 'FOFF' (fall-off) option */

case 4:  ifo = match(RK_EQCHK, RK_MSCAN, ~RK_COMMA, 0,
            foffkeys, NFOFFOPTS);
         break;

         } /* End kwret switch */

/* Other error checking for GCONN card */

   /* Area required for specified bands cannot exceed size of
   *  source layer.  (Note:  First ring (always 1 group) is not
   *  included in diam calc.  Test is >= instead of > to allow
   *  for the extra center group.)  */
   radius = ib->ngb*(num-1);
   if ((radius >= ir->ngx) && (radius >= ir->ngy))
      cryout(RK_E1, "0***GCONN ZONE EXCEEDS REP SIZE.",
         RK_LN2, NULL);

   /* Cannot use OPT=X except with at least 2 bands and
   *  BC = < ZERO | NORM | TOROID (with no falloff) > */
   if (ib->ibopt & IBOPTX) {
      if ((ibc > BC_NORM) && ((ibc != BC_TOROIDAL) || ifo))
         cryout(RK_E1, "0***OPT=X NOT VALID WITH GIVEN BC, FOFF.",
            RK_LN2, NULL);
      if (num < 2)
         cryout(RK_E1, "0***NEED 2 BANDS FOR OPT=X.", RK_LN2, NULL);
      }

/* For compatibility, bring in square|shunting options from
*  CELLTYPE card, then check that don't have both at once.
*  Implement COMPAT=K.  */

   if (il->Ct.ctopt & OPTSQ) ib->ibopt |= IBOPTQ;
   if (il->Ct.ctopt & OPTSH) ib->ibopt |= IBOPTS;
   if ((ib->ibopt & (IBOPTQ|IBOPTS)) == (IBOPTQ|IBOPTS))
      cryout(RK_E1, "0***INCOMPATIBLE INHIB OPTIONS.", RK_LN2, NULL);
   if (RP->compat & OLDKNEE) ib->ibopt |= IBOPKR;

/* Now set inhibition, falloff, and boundary condition options */

   /* Load falloff control and signal if falloff is on */
   if (ifo) {
      /* Subtract 1 so that the 12 options are numbered 0-11 */
      ib->kfoff = (byte)(ifo - 1);
      ib->ibopt |= IBOPFO;
      }

   /* Load boundary condition control */
   ib->kbc = (byte)ibc;

   /* Stash away any restore/rename information */
   if (!(ib->ibopt & IBOPOR)) linksfhm(phm, SFHMGCON);

   } /* End g2gconn() */


/*---------------------------------------------------------------------*
*                       Process 'MODULATE' card                        *
*                                                                      *
*  Synopses:  void g2modul(struct CELLTYPE *il, struct MODBY *im)      *
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
         "MXMOD%B20/27IL$mV",    &im->mxmod,
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
   ui32 kode,tlong;
   long num;
   si32 tcmn,tcmn2,tcsg;      /* Temps for cmn,cmn2,csg */
   int jsrc;                  /* Source type */
   int jt;                    /* Input type */
   int ka,kd,kdq;             /* TRUE for codes indicated */
   int kbcdqm,kfstjwx,kjnoy,kjxyh;
   int klcij,kmcij,krcij;     /* Flags for L,M,R types */
   int kwret;
   int ngn1, ngn2, ngn3;      /* Option bit counts */
   char lea[SCANLEN1];

   /* CONNTYPE user function types -- at the moment, just one */
#define NCNUFN 1
   static char *ucnfn[NCNUFN] = { "GetSj" };

/* Illegal kgen codes as a function of input types (bltsnm order) */
   static unsigned long badkgn[USRSRC+1] = {
/*REP*/  KGNST+KGNHV,
/*IA*/   KGNGP+KGNJN+KGNND+KGNOG+KGNHG,
/*VAL*/  KGNGP+KGNND+KGNOG+KGNST+KGNTP+KGNHV+KGNHG+KGNBL+KGNCF+KGNDG+
         KGNAN,
/*VJ*/   KGNGP+KGNND+KGNOG+KGNST+KGNTP+KGNHG+KGNBL+KGNCF+KGNDG+KGNAN,
/*VW*/   KGNGP+KGNND+KGNOG+KGNST+KGNTP+KGNHV+KGNHG+KGNBL+KGNCF+KGNDG+
         KGNAN,
/*VT*/   KGNGP+KGNND+KGNOG+KGNST+KGNHV+KGNHG,
/*TV*/   KGNGP+KGNND+KGNOG+KGNST+KGNHV+KGNHG,
/*PP*/   KGNGP+KGNND+KGNOG+KGNST+KGNHV+KGNHG,
/*D1*/   KGNGP+KGNND+KGNOG+KGNST+KGNTP+KGNHV+KGNHG,
/*HV*/   KGNBL+KGNCF+KGNDG+KGNFU+KGNGP+KGNHG+KGNJN+KGNND+KGNOG+
         KGNPT+KGNAN+KGNST+KGNTP+KGNUD+KGNXO+KGNYO+KGNLM+KGNEX+KGNE2,
/*ITP*/  KGNGP+KGNND+KGNOG+KGNST+KGNHV+KGNHG,
/*USR*/  KGNGP+KGNND+KGNOG+KGNST+KGNHG,
      };

/* Illegal code combinations:  Any code in second word is invalid
*  in combination with any code in the first word.  */
   static unsigned long badkg2[][2] = {
/*AJ*/   {KGNAJ, KGNFU+KGNTP+KGNST+KGNSA },
/*PT*/   {KGNPT, KGNEX+KGNHV+KGNND+KGNTP+KGNST+KGNXO+KGNYO },
/*E2*/   {KGNE2, KGNAJ+KGNBL+KGNCF+KGNDG+KGNPT+KGNAN },
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
   num = ix->nc;
   jsrc = ix->cnsrctyp;

   /* Set default file and source id for restore */
   ix->cnwk.tree.sfhm = *getsfhm(SFHMCONN, ix);
   if (il->Ct.ctopt & OPTOR) ix->Cn.cnopt |= NOPOR;
   /* If source is not a repertoire, cancel inherited delay
   *  option to eliminate error in d3tchk.  It is still an
   *  error to enter an explicit DELAY card after CONNTYPE. */
   if (jsrc != REPSRC) ix->Cn.kdelay = 0;

/* Read kgen, apply kgen defaults, and check for type conflicts.
*  Code '2' is unusual in that it implies 'E' but 'E2' is allowed,
*  so don't count twice in ngn2.  Temporary restriction documented
*  above is enforced in this code block.  */

   inform("(SXA" qDML ")", lea, NULL);    /* Skip nc, read kgen */
   /* This is always an original key, no RK_PMEQPM here */
   mcodes(lea, okcnkgen, &kode);
   if (kode & KGNE2) kode |= KGNEX;
   tlong = kode & (KGNE2+KGNMC+KGNRC+KGNLM);
   ngn1 = bitcnt((byte *)&tlong, sizeof(tlong));
   tlong = kode & (KGNEX+KGNGP+KGNND+KGNOG+KGNUD+KGNTP+
      KGNST+KGNFU+KGNXO+KGNYO+KGNJN+KGNHV+KGNHG);
   ngn2 = bitcnt((byte *)&tlong, sizeof(tlong));
   tlong = kode & (KGNAJ+KGNBL+KGNCF+KGNDG+KGNAN+KGNIN+KGNPT);
   ngn3 = bitcnt((byte *)&tlong, sizeof(tlong));

   if (!ngn1) kode |= KGNRC;

   if (!ngn2) {
      if (jsrc == VALSRC || kode & KGNLM) kode |= KGNXO;
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

/* Make general tests for invalid combinations of codes and inputs */

   /* Check for illegal source type vs Lij-generating combos */
   if (kode & badkgn[jsrc]) {
      ermark(RK_MARKFLD);
      cryout(RK_P1, "0***INVALID COMBINATION OF SRCID AND KGEN--",
         RK_LN2, "        SEE TABLE IN WRITEUP.", RK_LN1, NULL);
      }

   /* Check for illegal option combos */
   for (jt=0; jt<(sizeof(badkg2)/(2*sizeof(long))); jt++) {
      if ((kode & badkg2[jt][0]) && (kode & badkg2[jt][1])) {
         ermark(RK_MARKFLD);
         cryout(RK_P1, "0***INVALID COMBINATION OF (FIRST-LIJ OR"
            "CIJ) AND SUBSEQUENT-LIJ GENERATING CODES--", RK_LN2,
            "        SEE TABLE IN WRITEUP.", RK_LN1, NULL);
         }
      } /* End checking kgen codes */

/* Preliminary error checking based on generating code and source */

   /* Turn on T if S is requested */
   if (kode & KGNST) kode |= KGNTP;
   /* Turn on KGNBV if W is requested with USRSRC */
   if (kode & KGNHV && jsrc == USRSRC) kode |= KGNBV;

   /* Save tests that will be repeated often */
   klcij   = kode & KGNLM;
   kmcij   = kode & KGNMC;
   krcij   = kode & KGNRC;
   ka      = kode & KGNAJ;
   kbcdqm  = kode & (KGNBL|KGNCF|KGNDG|KGNAN|KGNMC);
   kd      = kode & KGNDG;
   kdq     = kode & (KGNDG|KGNAN);
   kfstjwx = kode & (KGNFU|KGNST|KGNTP|KGNJN|KGNXO|KGNYO|KGNBV);
   kjnoy   = kode & (KGNJN|KGNND|KGNOG|KGNYO);
   kjxyh   = kode & (KGNJN|KGNXO|KGNYO|KGNHV);

   /* Error checking for W */
   if (kode & KGNHV) {
      /* For compatibility, allow VJ_SRC or IA_SRC */
      if (jsrc == VJ_SRC || jsrc == IA_SRC) jsrc = VH_SRC;
      if (jsrc == VH_SRC || jsrc == USRSRC) {
         if (num != 1) {
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
      if (kode & (KGNLM|KGNMC)) {
         ermark(RK_WARNING+RK_MARKFLD);
         cryout(RK_P1, "0-->WARNING: Regeneration will not change "
         "Cij of types L,M.", RK_LN2, NULL); }
      if (kode & (KGNEX|KGNHV|KGNXO|KGNYO)) {
         ermark(RK_WARNING+RK_MARKFLD);
         cryout(RK_P1, "0-->WARNING: Regeneration will not change "
         "Lij of types E,W,X,Y.", RK_LN2, NULL); }
      }

/* For any kind of input from a window (VW or IA with kgen=S),
*     check that a non-existent window is not requested.
*  If there is an error, poison jsrc so later code won't bomb.
*  Do this before any calls to readmdlt().  */

   if (jsrc == VW_SRC || (kode & KGNST)) {
      int mwdw = readlid(&ix->srcnm, &RP0->jwdw, RP->nwdws,
         "windows.");
      if (mwdw > 0)  ix->wdwi = (byte)(mwdw - 1);
      else           jsrc = BADSRC;
      } /* End window setup */

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
      inform("(S=V>1<=16IHB24ILVB28ILB24IL)", &ix->Cn.nbc, &tcmn,
         &tcsg, &tcmn2, NULL);
      }
   else if (kmcij) {
      /* Input for 'M' (matrix) coefficients */
      inform("(S=V>1<=16IH,VIL,2*V>IL)", &ix->Cn.nbc,
         &ix->ucij.m.mno, &ix->ucij.m.nks, &ix->ucij.m.nkt, NULL);
      /* Warning if Lij is not a map */
      if (!(kode & (KGNAJ|KGNBL|KGNDG)))
         cryout(RK_P1, "0-->WARNING: Type M Cijs usually need "
            "A,B,D maps.", RK_LN2, NULL);
      }
   else {
      /* Input for 'R' (random) coefficients */
      tcmn = tcsg = -1;
      inform("(S=V>1<=16IHVB24ILVB28ILVB16IL)",&ix->Cn.nbc,
         &tcmn, &tcsg, &ix->ucij.r.pp, NULL);
      }

   while (kwret = kwscan(&ic,    /* Assignment intended */
         "BITS%V>1<=16IH",             &ix->Cn.nbc,
         "C00%RB24IL",    klcij,       &ix->ucij.l.c00,
         "C01%RB24IL",    klcij,       &ix->ucij.l.c01,
         "C10%RB24IL",    klcij,       &ix->ucij.l.c10,
         "C11%RB24IL",    klcij,       &ix->ucij.l.c11,
         "CMN2%RB24IL",   klcij,       &tcmn2,
         "MEAN%RB24IL",   klcij|krcij, &tcmn,
         "SIGMA%RVB28IL", klcij|krcij, &tcsg,
         "PP%RB16IL",     krcij,       &ix->ucij.r.pp,
         "POSFRAC%RB16IL",krcij,       &ix->ucij.r.pp,
         "SET%RVIL",      kmcij,       &ix->ucij.m.mno,
         "NKS%RV>IL",     kmcij,       &ix->ucij.m.nks,
         "NKT%RV>IL",     kmcij,       &ix->ucij.m.nkt,
         "BC%X",
         "COLOR%N" KWS_CSEL3,          &ix->cnxc,
         "OPT%KH" okcnopt,             &ix->Cn.cnopt,
         "CNOPT%KH" okcnopt,           &ix->Cn.cnopt,
         "SAOPT%KC" oksaopt,           &ix->Cn.saopt,
         "MAPOPT%RKH" okcnmop, klcij,  &ix->ucij.l.flags,
         "MODALITY%X",
         "NAPC%N" KWS_NAPC,            &ix->Cn.napc,
         "NHX%RIH",       kdq,         &ix->nhx,
         "NHY%RIH",       kdq,         &ix->nhy,
         "NRX%RV>IL",     kbcdqm,      &ix->nrx,
         "NRY%RV>IL",     kbcdqm,      &ix->nry,
         "NUX%RVIL",      kfstjwx,     &ix->nux,
         "NUY%RVIL",      kfstjwx,     &ix->nuy,
         "NVX%RIH",       kd,          &ix->nvx,
         "NVY%RIH",       kd,          &ix->nvy,
         "OFFSET%RIL",    kjxyh,       &ix->loff,
         "STRIDE%RIL", (int)(kode & (KGNAJ|KGNBL)), &ix->stride,
         "SVITMS%KJ" oksimcn,          &ix->svitms,
         "UPROG%X",
         "WIDTH%RVB1IL",  kjnoy,       &ix->lsg,
         "RADIUS%RV>B16UJ", (int)(kode & KGNHV), &ix->radius,
         "XOFF%RIL",      kfstjwx,     &ix->xoff,
         "YOFF%RIL",      kfstjwx,     &ix->yoff,
         "PRADIJ%X",
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
            ix->sjup = (long (*)())d3uprg(TRUE);
            break;
         default: while (RK.scancode & RK_INPARENS) scan(NULL, 0) ;
            scanagn();
            break;
            }
         break;

         /* Process 'PRADIJ' option */
case 4:  inform("(S,V(2ILV))", &ix->pradi, NULL);
         break;

         } /* End kwret switch */

   if (klcij) {                  /* Final fixup for code L */
      /* If old-style constants were entered, use them */
      if (tcmn != -1)  ix->ucij.l.c00 = ix->ucij.l.c01 = tcmn;
      if (tcmn2 != -1) ix->ucij.l.c10 = ix->ucij.l.c11 = tcmn2;
      if (tcsg != -1)  ix->ucij.l.csg = tcsg;
      }
   else if (krcij) {             /* Final fixup for code R */
      if (tcmn != -1)  ix->ucij.r.cmn = tcmn;
      if (tcsg != -1)  ix->ucij.r.csg = tcsg;
      if (ix->ucij.r.pp > (S16-1)) ix->ucij.r.pp = (S16-1);
      }

/* Final error checking for CONNTYPE card */

   /* Error checking for code 'M'--matrix input of Cij's */
   if (kmcij && (ix->ucij.m.nks > 0)) {
      if (8%ix->ucij.m.nks)
         cryout(RK_E1, "0***NKS MUST DIVIDE 8 EVENLY.", RK_LN2, NULL);
      }

/* Increment proper global index for VH,VJ,VT,D1 inputs, and check
*  that the source exists for all input types where that information
*  is available at this time.  All other checking is delayed until
*  d3tchk().  If there is an error, poison jsrc so later code won't
*  bomb.  */

   switch (jsrc) {

   /* For input from a value scheme, check that it exists.  */
   case VALSRC:
      if (ix->loff >= RP->nvblk)
         cryout(RK_E1, "0***OFFSET > NUM OF VALUES", RK_LN2, NULL);
      break;

   /* For input from VJ or VT, readlid() increments hbcnm if
   *  '-' is coded and checks that the requested arm exists.
   *  More detailed checks are also done in d3schk().  */
   case VJ_SRC:
   case VT_SRC:
      if (readlid(&ix->srcnm, &RP0->jarm, RP->narms, "ARMS.") <= 0)
         jsrc = BADSRC;
      break;

   /* For input from TV or PP, all checking is done in d3tchk().  */
   case TV_SRC:
   case PP_SRC:
      break;

#ifdef D1
   /* For input from a Darwin 1 array, check
   *  that a non-existent array is not requested.  */
   case D1_SRC:
      if (readlid(&ix->srcnm, &RP0->jd1, RP->nd1s,
            "DARWIN I ARRAYS.") <= 0) jsrc = BADSRC;
      break;
#endif

   /* For input from VH (not USRSRC), be sure specified arm (and
   *  window if 'loff' option is given) exist.  Store their index
   *  numbers for later use.  N.B.  For compatibility with earlier
   *  CNS versions, the index 'jarm' is incremented by readlid()
   *  when arm number is coded by user as '-'.  */
   case VH_SRC:
      {  int marm = readlid(&ix->srcnm, &RP0->jarm, RP->narms,
            "ARMS.");
         int mwdw = ix->loff;

         if (marm > 0)  ix->armi = (byte)(marm - 1);
         else           jsrc = BADSRC;

         /* mwdw == 0 is not an error--means to use whole IA */
         if (mwdw > (int)RP->nwdws) {
            cryout(RK_E1, "0***HAND VIS. WINDOW NUM EXCEEDS "
               "NUM OF WINDOWS.", RK_LN2, NULL);
            jsrc = BADSRC; }
         else if (mwdw > 0)
            ix->wdwi = (byte)(mwdw - 1);
         } /* End VH local scope */
      break;

      } /* End jsrc switch */

/* Further tests relating to nux,nuy,loff,xoff,yoff,nhx,nhy,nvx,nvy
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

   if (iamthr) {
      /* Positional parameters for THRESHOLDS card */
      inform("(S=2*J" KWJ_THR "X,B24IJ)",
         &pdd->et, &pdd->mtj, &pdd->scl, NULL);
      }
   else {
      /* Positional parameters for PARAMETERS card.
      *  All but first 6 params removed, 07/25/92--useless code.
      *  mtj added when merged with THRESHOLDS, 11/04/07.  */
      inform("(S=B28IL,J" KWJ_THR "CR,J" KWJ_THR "X,B8IH,J"
         KWJ_THR "CR,J" KWJ_THR "X)",
         &pdd->delta, &pdd->mti, &pdd->mtj, &pdd->mtv,
         &pdd->mnsi, &pdd->mnsj, NULL);
      }

   while (kwret = kwscan(&ic,    /* Assignment intended */
         "ADECAY%N" KWS_DECAY,   &pdd->ADCY,
         "AMPF%T"  qLTF,         &hampf,
         "DELTA%VB28IL",         &pdd->delta,
         "ET%J" KWJ_THR "X",     &pdd->et,
         "ETHI%J" KWJ_THR "X",   &pdd->et,
         "ETLO%J" KWJ_THR "X",   &pdd->etlo,
         "MNAX%B20/27IJ$mV",     &pdd->mnax,
         "MXAX%B20/27IJ$mV",     &pdd->mxax,
         "MNSI%J" KWJ_THR "CR",  &pdd->mnsi,
         "MNSJ%J" KWJ_THR "X",   &pdd->mnsj,
         "MTI%J" KWJ_THR "CR",   &pdd->mti,
         "MTICKS%V>IH",          &pdd->mticks,
         "MTJ%J" KWJ_THR "X",    &pdd->mtj,
         "MTV%B8IH",             &pdd->mtv,
         "MXMIJ%B7/14IH$mV",     &pdd->mxmij,
         "MXMP%VB13/20IL$mV",    &pdd->mxmp,
         "NEGAMP%VIC",           &pdd->csc,
         "OPT%KH" okcnopt,       &pdd->cnopt,
         "CNOPT%KH" okcnopt,     &pdd->cnopt,
         "SAOPT%KC" oksaopt,     &pdd->saopt,
         "PHASE%X",
         "PHIFN%X",
         "RDAMP%V<=1B15UH",      &pdd->rdamp,
         "RETARD%VIC",           &pdd->retard,
         "RULE%KL" okcnkam,      &pdd->kam,
         "SCALE%B24IJ",          &pdd->scl,
         "SJMIN%J" KWJ_THR "X",  &pdd->et,
         "SJPCF%T" qLTF,         &hpcfn,
         "SJREV%J" KWJ_THR "X",  &pdd->sjrev,
         "UPSM%B16IL",           &pdd->upsm,
         "VDHA%V~B20/27IL$mV",   &pdd->vdha,
         "VI%VIH",               &pdd->vi,
         "VDT%B20/27IL$mV",      &pdd->vdt,
         "VT%VB8IH",             &pdd->mtv,
         "WAY%X",
         "ZETAM%VB16IL",         &pdd->zetam,
         NULL))
      switch (kwret) {

case 1:     /* PHASE option */
         eqscan(&pdd->phi, "VIC", RK_EQCHK);
         pdd->phi &= PHASE_MASK;
         pdd->cnopt |= PHASC;
         break;
case 2:     /* PHIFN */
         pdd->phifn = (byte)getffn();
         break;
case 3:     /* WAY option */
         getway(pdd);
         break;

         } /* End switch and while */

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

   inform("(SW1=4*VIL)", &ix->cseed, &ix->lseed, &ix->phseed,
            &ix->rseed, NULL);
   kwscan(&ic,
      "CSEED%VIL", &ix->cseed,
      "LSEED%VIL", &ix->lseed,
      "NSEED%VIL", &ix->phseed,
      "RSEED%VIL", &ix->rseed, NULL);
   } /* End g2seeds() */


/*---------------------------------------------------------------------*
*                       Process 'SUBARBOR' card                        *
*                                                                      *
*  Synopsis:  void g2subarb(struct CONNTYPE *ix)                       *
*---------------------------------------------------------------------*/

void g2subarb(struct CONNTYPE *ix) {

   ui32 ic;                   /* kwscan field recognition codes */

/* Read data from SUBARBOR card */

   inform("(SW1=V>ILV>IL)", &ix->nsa, &ix->nsax, NULL);
   kwscan(&ic,
      "NSA%V>IL",       &ix->nsa,
      "NSAX%V>IL",      &ix->nsax,
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

