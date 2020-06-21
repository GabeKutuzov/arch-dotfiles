/* (c) Copyright 1985-2018, The Rockefeller University *21116* */
/* $Id: d3echo.c 77 2018-03-15 21:08:14Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                               d3echo                                 *
*                    Report repertoire parameters                      *
*                                                                      *
*     Note:  Repertoire, cell, and connection specifications are       *
*        printed separately rather than hierarchically.  This is       *
*        basically to avoid the need for repeated interleaving of      *
*        title lines.  Lines are kept to 96 columns for convenient     *
*        terminal reading.                                             *
*     Note:  Where conditional line formatting is present, code        *
*        generally assumes that source rep and layer names will        *
*        use 4 cols. each, i.e. conditional on LSNAME is not used.     *
*                                                                      *
************************************************************************
*  V1A, 10/21/85, G. N. Reeke                                          *
*  V4A, 08/30/88, Translated to 'C' from version V3F                   *
*  Rev, 01/xx/91,  MC - Added PHASE code                               *
*  V5C, 11/19/91, GNR - Added Darwin 1, reformat phase params          *
*  V5E, 07/26/92, GNR - Add RHO, DVI parameters, fix phase params      *
*  V6A, 03/01/93, GNR - Add SUBARBOR, add cnopt to CONNTYPE part 2     *
*  Rev, 07/13/93, GNR - Add cumnvc to memory allocation listing        *
*  V6D, 01/31/94, GNR - Add DELAY, spike, ctscll, ot, and s(i) length  *
*  V7A, 04/24/94, GNR - Report node max mem alloc data for tree, reps, *
*                       D1 only if exists, add ihdelay,mdelay,nmdelay  *
*  V7B, 07/21/94, GNR - Add rsd, new ltp options                       *
*  Rev, 08/19/94, GNR - Add c00, c01, c10, c11 for KGEN=L, decay for   *
*                       modulation and GCONN                           *
*  V7C, 12/22/94, GNR - Reorganize n[hv][xy], stride, voltage-depend.  *
*  V8A, 05/26/95, GNR - Add cumnds.  Add decay,siet,siscl,REFRACTORY   *
*                       PERIOD for regular cell types, delete "LTP",   *
*                       better bc codes                                *
*  Rev, 02/25/97, GNR - Add ls0all, modalities, senses, pcf names, R~  *
*  Rev, 12/24/97, GNR - Add ADECAY, remove CSC to make room for it     *
*  Rev, 01/05/98, GNR - Add PPF, move d3inhs call here from d3genr     *
*  Rev, 02/18/98, GNR - Add resting potential, afterhyperpolarization  *
*  V8B, 12/10/00, GNR - Move to new memory management routines         *
*  V8D, 03/22/05, GNR - Add conductances and ions                      *
*  Rev, 01/06/07, GNR - List beta coefficients in a separate group     *
*  Rev, 02/19/07, GNR - Add effector listing                           *
*  Rev, 11/03/07, GNR - Add rdamp,mabrsj,sjrev, sep vd listing, ot out *
*  ==>, 01/08/08, GNR - Last mod before committing to svn repository   *
*  Rev, 03/15/08, GNR - Add listing of way scales, low thresholds      *
*  V8E, 01/15/09, GNR - Add Izhikevich parameters, phase to il->pctpd  *
*  Rev, 03/15/09, GNR - Add pdash(), dseed to seeds, reformat comments *
*  Rev, 09/03/09, GNR - Add Brette-Gerstner (aEIF) neurons, remove 'rt'*
*  V8F, 02/14/10, GNR - Remove conditionals on IMGTOUCH and TV         *
*  V8G, 08/13/10, GNR - Add AUTOSCALE parameters                       *
*  V8H, 04/24/12, GNR - Add gdefer and gbcm, remove d3inhs() call      *
*  Rev, 06/30/12, GNR - Add qpull                                      *
*  V8I, 09/29/12, GNR - Add ALPHA and DOUBLE EXPONENTIAL decay types   *
*  Rev, 12/07/12, GNR - Convert ns[xy], ku[xy] to ui16, nst to ui32    *
*  Rev, 04/05/13, GNR - Fix bug printing omega1 as IH, correct is UH   *
*  Rev, 04/24/13, GNR - Add mxsi                                       *
*  Rev, 05/11/13, GNR - Remove gconn falloff                           *
*  Rev, 05/16/13, GNR - Add parameters for AUTOSCALE 'H' option        *
*  Rev, 08/22/14, GNR - Si only EXP decay, remove DECAYDEF from DCYDFLT*
*  R63, 11/03/15, GNR - Add vdopt codes                                *
*  R65, 01/02/16, GNR - n[gq][xyg] to ui16, ngrp to ui32, ncpg to ubig *
*  R66, 02/17/16, GNR - Change long random seeds to typedef si32 orns  *
*  R66, 03/11/16, GNR - Rename nvx->nsxl, nvy->nsya so != ix->nv[xy]   *
*  R67, 04/27/16, GNR - Remove Darwin 1 support                        *
*  R72, 02/07/17, GNR - Add KGNKN output, show RP0->lkall, mx line 100 *
*  R72, 03/18/17, GNR - Remove nhx,nhy,nvx,nvy output for KGNGD        *
*  R74, 07/13/17, GNR - Print radius for KGNKN|KGNE2, remove [iu]bcdwt *
*  R75, 09/22/17, GNR - Move tcwid to CONNTYPE (Part 2)                *
*  R77, 01/31/18, GNR - Add MxESJ to CONNTYPE (Part 2)                 *
*  R78, 03/15/18, GNR - xoff,yoff no longer used with KGNKN            *
*  R78, 04/14/18, GNR - Calc & print default vitp scales w/o storing   *
*  R80, 08/25/18, GNR - Add epsilon integration decay parameter        *
***********************************************************************/

#define NEEDCDEFS

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "d3global.h"
#include "d3opkeys.h"
#include "rocks.h"
#include "rkxtra.h"

#define MXELNL   100             /* Maximum echo line length */
extern char *phifnnm[2][FFN_N];  /* PHIFN names */

/*=====================================================================*
*                              decayinfo                               *
*                                                                      *
*  This routine fills in formatted information from a DECAYDEF block.  *
*                                                                      *
*  Arguments:                                                          *
*     pdcy     Ptr to DECAYDEF block.                                  *
*     pdt      Ptr to area to receive output text (20 chars, no NULL)  *
*=====================================================================*/

static void decayinfo(struct DECAYDEF *pdcy, char *pdt) {

   static char kdcynms[][4] = { "    ", " EXP", " LIM", " SAT",
      " ALF", " DBL" };

   memcpy(pdt, kdcynms[pdcy->kdcy], 4);
   wbcdwt(&pdcy->omega, pdt+4, FBod*RK_BSCL|6*RK_D|RK_IORF|RK_NI32|7);
   switch (pdcy->kdcy) {
   case DCYSAT:
      bcdout(RK_UFLW|RK_AUTO|RK_IORF|7, pdt+12, pdcy->du1.dsa.hlfeff);
      break;
   case DCYDBLE:
      wbcdwt(&pdcy->du1.ddb.omeg2, pdt+12,
         FBod*RK_BSCL|6*RK_D|RK_IORF|RK_NI32|7);
      break;
   default:
      memset(pdt+12, ' ', 8);
      break;
      } /* End kdcy switch */
   } /* End decayinfo() */


/*=====================================================================*
*                                pdash                                 *
*                                                                      *
*  This routine prints a region/repertoire name followed by dashes     *
*                                                                      *
*  Arguments:                                                          *
*     ir       Ptr to repblock                                         *
*     ll       Length of line to be filled                             *
*=====================================================================*/

static void pdash(struct REPBLOCK *ir, int ll) {

#define LRR 18                   /* Length of " REGION/REPERTOIRE" */
   int ldash;                    /* Length of dashes to print */
   int mxlrn;                    /* Max print length of rep name */
   static char *rdash = {
      " REGION/REPERTOIRE -----------------------------------------"
      "-----------------------------------------" };

   mxlrn = min(ir->lrn, MXELNL-(LRR+1));
   ldash = ll - mxlrn - 1;
   if (ldash <= 0) ldash = RK_SKIP;
   cryout(RK_P4, " ", RK_LN1+1, getrktxt(ir->hrname), mxlrn,
      rdash, ldash, NULL);

   } /* End pdash() */


/*=====================================================================*
*                             setbscompat                              *
*                                                                      *
*  This routine calls bscompat() to set scale for conntype thresholds  *
*  based on source type.                                               *
*=====================================================================*/

void setbscompat(int ksrctyp) {

   int kbs = RP0->bssel;
   if (kbs || ksrctyp != REPSRC) kbs |= RK_BSVERTB;
   bscompat(kbs);

   } /* End setbscompat() */


/*=====================================================================*
*                               d3echo                                 *
*=====================================================================*/

void d3echo(void) {

   struct REPBLOCK  *ir;
   struct CELLTYPE  *il;
   struct CONNTYPE  *ix;
   struct INHIBBLK  *ib;
   struct MODBY    *imb;
   struct MODALITY  *pm;
   struct VBDEF    *ivb;
   struct VCELL    *pvc;
   struct EFFARB   *pea;
   ui64 wkount;                  /* Grand totals -- 64-bit */
   long kounts[5];               /* Grand totals */
   long ncells;                  /* Cell/Rep counter */
   long nconns;                  /* Connection/Rep counter */
   long ntypes;                  /* Celltype/Rep counter */
   int ict;                      /* Connection type index */
   int structsize;
   ui16 eflgs;                   /* Printing flags */
#define KEP_IZHI3    0x0001         /* Print Izhikevich (2003) info */
#define KEP_IZHI7    0x0002         /* Print Izhikevich (2007) info */
#define KEP_DECAY    0x0004         /* Print decay info */
#define KEP_DELAY    0x0008         /* Print delay info */
#define KEP_REFRAC   0x0010         /* Print refrac info */
#define KEP_VDEP     0x0020         /* Print volt-dep info */
#define KEP_AMP      0x0040         /* Print amplif info */
#define KEP_SLOWA    0x0080         /* Print slow amp info */
#define KEP_PPF      0x0100         /* Print PPF info */
#define KEP_SUBARB   0x0200         /* Print subarbor info */
#define KEP_COND     0x0400         /* Print conductance info */
#define KEP_ION      0x0800         /* Print ion information */
#define KEP_BREG     0x1000         /* Print Brette-Gerstner info */
   char prtnam;                  /* Print repertoire name */
   char pmxib;                   /* Print max inhib info */
   char line[MXELNL+1];          /* Conversion buffer for sconvrt */

   static char *rfnms[] = {      /* Text for response functions */
      " KNEE", " KNEE", " STEP", "SPIKE", " TANH", " AEIF",
      "I2003", "I2007" };
   static char *kdlyo[5] = {     /* Text for printing delay options */
      "NONE", "CNST", "UNIF", "NORM", "USER" };
   static char *fiopt[5] = {     /* Text for printing phase options */
      "  CENTRD", "  RANDOM", "  FIRST ", "  MODE  ", "  CONST(" };
   static char *siopt[5] = {     /* Text for printing si options */
      "  CENTRD", "  HEIGHT", "  CONST(", "  SUMPOS", "  MEAN  " };
   static char *bcopt[7] = {     /* Text for boundary conditions */
      " ZRO", " NRM", " NOI", " EDG", " MIR", " TOR", " HLX" };
   extern const char *vbtnames[];

/*---------------------------------------------------------------------*
*               Print REGION/REPERTOIRE specifications                 *
*---------------------------------------------------------------------*/

   cryout(RK_P2,"0REGION/REPERTOIRE SPECIFICATIONS:", RK_SUBTTL+RK_LN2,
      "   NAME                SHORT GROUPS C/GP TYPES  CELLS "
      "     CONNS NGX NGY     STORAGE", RK_LN1+84 ,NULL);

   kounts[0] = kounts[1] = kounts[2] = kounts[3] = kounts[4] = 0;
   wkount = jeul(0);
   eflgs = 0;                 /* Turn off optional printouts */
   for (ir=RP->tree; ir; ir=ir->pnrp) {
      ncells = ir->ngrp * ir->ncpg;
      nconns = ntypes = 0;
      for (il=ir->play1; il; il=il->play) {
         nconns += (long)il->nelt * (long)il->nce;
         ntypes++;
         } /* End il loop */
      convrt("(P2,3XA20XA" qLSN ",UJ6UB6IL5IL8IL11,2*UH4UW12)",
         getrktxt(ir->hrname), ir->sname, &ir->ngrp, &ir->ncpg,
         &ntypes, &ncells, &nconns, &ir->ngx, &ir->ngy, &ir->lrt,
         NULL);
      kounts[0] += ir->ngrp;
      kounts[1] += ntypes;
      kounts[2] += ncells;
      kounts[3] += nconns;
      wkount = jauw(wkount, ir->lrt);
      } /* End ir loop */
   /* Print totals for all normal repertoires */
   convrt("(P2Y1,3X,'Totals',15XJUH4#4IL6IL11IL8IL11,6XUW14)",
      &RP->nrep, kounts, &wkount, NULL);

/*---------------------------------------------------------------------*
*                 Print memory allocation information                  *
*  Note:  This output is intended to show primarily information needed *
*  to optimize memory usage on a parallel computer. Information stored *
*  only on host node, or generally not under user control, is omitted. *
*---------------------------------------------------------------------*/

#ifdef PAR     /* Tag used to label memory totals */
#define MEMTAG "Max node "
#define MEMT2  " "
#else
#define MEMTAG "Total "
#define MEMT2  "    "
#endif   /* PAR */

   cryout(RK_P2,"0MEMORY ALLOCATION INFORMATION:",
      RK_NFSUBTTL+RK_LN2, NULL);
   {  size_t tmsz = memsize(MPS_Static);
      convrt("(P2Y5,' Total space for network tree control blocks',"
         "24XUZ16)", &tmsz, NULL); }
   convrt("(P2,' Total space for cell and connection statistics',"
      "21XIL16)", &RP0->ldstat, NULL);
#ifdef PAR
   convrt("(P2Y1,' Total host space for s(i) rapid lookup tables',"
      "22XUW16)", &RP0->ls0all, NULL);
#endif
   convrt("(P2,' " MEMTAG "space for s(i) rapid lookup tables"
      MEMT2 "',23XUW16)", &RP0->lsall, NULL);
   wkount = jauw(RP0->lrall, RP0->lgall);
   convrt("(P2,' " MEMTAG "space for cell and connection data"
      MEMT2 "',23XUW16)", &wkount, NULL);
   if (RP->lkall)
      convrt("(P2Y1,' Total space for conntype K,Q tables',"
         "32XUJ16)", &RP->lkall, NULL);
   if (RP0->cumcnd)
      convrt("(P2Y1,' " MEMTAG "space for conductance decay"
         " tables" MEMT2 "',23XIL16)", &RP0->cumcnd, NULL);
   convrt("(P2,' " MEMTAG "space for geometric connection work"
      " areas" MEMT2 "',16XUW16)", &RP0->liall, NULL);
   if (quw(RP0->ldecay))
      convrt("(P2Y1,' " MEMTAG "space for decay of connection"
         " activity" MEMT2 "',19XUW16)", &RP0->ldecay, NULL);

   cryout(1+RK_P2," Space for trial data (broadcast on parallel "
      "computer)", RK_LN2, "        IA   Images   Senses   Values"
      "  Stim Ids Effectors  Controls            TOTAL", RK_LN0+84,
      NULL);
   kounts[0] = (long)RP->nst + (long)RP->nst;
   kounts[1] = RP0->cumnvbc + sizeof(float)*RP0->cumnvfc;
   kounts[2] = sizeof(float)*(RP0->cumnaw + RP0->ljnwd);
   kounts[3] = sizeof(si32)*(RP0->nautsc + RP0->nbsi32);
   kounts[4] = RP->cumbcst + RP0->cumval +
      sizeof(float)*RP0->cumnvfc + kounts[2] + kounts[3];
   convrt("(P2Y1,X,4*IL9,3*IL10,IL17)", kounts, &RP0->cumntv,
      kounts+1, &RP0->cumval, &RP->cumnds, kounts+2, kounts+3,
      kounts+4, NULL);

/*---------------------------------------------------------------------*
*                     Print MODALITY information                       *
*---------------------------------------------------------------------*/

   cryout(RK_P4,"0MODALITY SPECIFICATIONS:", RK_NFSUBTTL+RK_LN3,
      "   MODALITY           NCS   NDS   NTRS  NTCS WDW", RK_LN0+48,
      NULL);

   for (pm=RP->pmdlt1; pm; pm=pm->pnmdlt)
      /* Kludge alert:  ntrs,ntcs defaults are xFFFF, a long cutoff
      *  delay, which will print here as '-1' by use of 'IH' code.  */
      convrt("(P4,3X,A16,2*UH6,2*IH6,IH4)", getrktxt(pm->hmdltname),
         &pm->ncs, &pm->nds, &pm->ntrs, &pm->ntcs, &pm->mdltwdw,
         NULL);

/*---------------------------------------------------------------------*
*                       Print SENSE information                        *
*---------------------------------------------------------------------*/

   /* Senses that are not VITPs */
   cryout(RK_P4,"0SENSE SPECIFICATIONS:", RK_NFSUBTTL+RK_LN3,
      "   TYPE  SENSE                  Vmin   Vmax   HWID"
      " NSXL NSYA  MODALITY", RK_LN0+70, NULL);

   for (pvc=RP0->pvc1; pvc; pvc=pvc->pnvc) {
      int kmdlt;
      char *psname,*pmname;
      if (pvc->vtype == ITPSRC) continue;
      if (pvc->pvcmdlt)
         kmdlt = 1, pmname = getrktxt(pvc->pvcmdlt->hmdltname);
      else
         kmdlt = 0, pmname = NULL;
      if (pvc->lvrn > 0)
         psname = getrktxt(pvc->hvname);
      else if (pvc->hvname > 0)
         psname = ssprintf(NULL, "%4d", (int)pvc->hvname);
      else
         psname = "--unnumbered--";
      convrt("(P4,3X,A" qLSN ",2XA20,3*F7.3,2*IH5,2XRA16)",
         pvc->vcname, psname, &pvc->vcmin, &pvc->vcmax,
         &pvc->vhw, &pvc->nsxl, &pvc->nsya, &kmdlt, pmname, NULL);
      }

   /* Senses that are VITPs */
   cryout(RK_P4,"0VITP SENSE SPECIFICATIONS:", RK_NFSUBTTL+RK_LN2,
      NULL);
   for (pvc=RP0->pvc1; pvc; pvc=pvc->pnvc) {
      struct IMGTPAD *pitp = (struct IMGTPAD *)pvc;
      char asclx[8],ascly[8];
      if (pvc->vtype != ITPSRC) continue;
      cryout(RK_P4,"   CAMERA           SENSE              NTA  NTL"
         "    TLL     TLA   COLOR  NCHNG OPS", RK_LN4+81, NULL);
      convrt("(P4,3XA15,2XA15,2X,2IH5,2F8.<3,2XA5,IC5,3XKC3)",
         getrktxt(pitp->hcamnm), getrktxt(pvc->hvname),
         &pvc->nsxl, &pitp->tll, fmtcsel(pvc->vcxc.tvcsel),
         &pitp->itchng, "PI", &pitp->itop, NULL);
      cryout(RK_P4,"       SCLX    OFFX    SCLY    OFFY    "
         "SCLA    OFFA", RK_LN0+51, NULL);
      if (pitp->sclx == 0.0) memcpy(asclx, " IMGWID ", 8);
      else  bcdout(3<<RK_DS|RK_AUTO|RK_IORF|7, asclx, pitp->sclx);
      if (pitp->scly == 0.0) memcpy(ascly, " IMGHGT ", 8);
      else  bcdout(3<<RK_DS|RK_AUTO|RK_IORF|7, ascly, pitp->scly);
      convrt("(P4,3X,A8,F8.<3,A8,3*F8.<3)", asclx, &pitp->offx,
         ascly, &pitp->offy, &pitp->scla, &pitp->offa, NULL);
      }

/*---------------------------------------------------------------------*
*                       Print VALUE information                        *
*---------------------------------------------------------------------*/

   cryout(RK_P4,"0VALUE SPECIFICATIONS:", RK_NFSUBTTL+RK_LN3,
      "   INDEX  TYPE    SOURCE            SCALE   VDAMP   BASE"
      "   BEST HWIDTH    MIN  DELAY  OPT", RK_LN0+89, NULL);

   for (ivb=RP->pvblk,ict=1; ivb; ivb=ivb->pnxvb,++ict) {
      struct MODALITY *pmdlt;
      int ikv = (int)ivb->kval;
      int rvbase = (int)EVBBASE >> ikv & 1;
      int rvbest = (int)EVBBEST >> ikv & 1;
      int rvdamp = (int)EVBDAMP >> ikv & 1;
      int rvhalf = (int)EVBHALF >> ikv & 1;
      char evsrc[16];
      switch (ivb->vbkid) {
      case VBK_REP:
         sconvrt(evsrc, "(A" qLSN ",2XA" qLSN ",5X)",
            ivb->valnm.rnm, ivb->valnm.lnm, NULL);
         break;
      case VBK_DIST:
         sconvrt(evsrc, "(4HARM A3,4HWIN A" qLSN ")",
            ivb->valnm.rnm, ivb->valnm.lnm, NULL);
         break;
      case VBK_UJG:
         sconvrt(evsrc, "(4HARM A" qLSN ",7X)",
            ivb->valnm.rnm, NULL);
         break;
      case VBK_ENV:
         if (pmdlt = ivb->u1.vev.pvmdl)   /* Assignment intended */
            sconvrt(evsrc, "(A12,1X,A2)",
               getrktxt(pmdlt->hmdltname), ivb->valnm.lnm, NULL);
         else
            sconvrt(evsrc, "(6HVISION,9X)", NULL);
         break;
      default:
         sconvrt(evsrc, "(15X)", NULL);
         break;
#ifdef BBDS
      case VBK_BBD:
         sconvrt(evsrc, "(A15)", getrktxt(ivb->valnm.hbcnm), NULL);
         break;
#endif
         }
      convrt("(P4I6,4X,A8A15,B" qqv(FBsc) "IJ8.5,R~B" qqv(FBdf)
         "UH8.5,R~B" qqv(FBvl) "IJ7.3,R~I7,R~I7,B" qqv(FBvl)
         "IJ7.3,UH7,2XJ0KC8)", &ict, vbtnames[ikv], evsrc, &ivb->vscl,
         &rvdamp, &ivb->vdamp, &rvbase, &ivb->vbase,
         &rvbest, &ivb->vbest, &rvhalf, &ivb->vhalf,
         &ivb->vmin, &ivb->vdelay, okvalue, &ivb->vbopt, NULL);
      } /* End value block loop */

/*---------------------------------------------------------------------*
*                     Print EFFECTOR information                       *
*---------------------------------------------------------------------*/

   cryout(RK_P4,"0EFFECTOR SPECIFICATIONS:", RK_NFSUBTTL+RK_LN3,
      "   EFFECTOR                     NCHGS  EXCIT CELLS   INHIB CELLS"
      "     EFT     IFT  ESCALE  ISCALE", RK_LN0+96, NULL);

   for (pea=RP0->pearb1; pea; pea=pea->pneb) {
      int iei,j,ke,k = 2;
      memset(line, ' ', 4*(LSNAME+2)+4);
      for (iei=EXCIT; iei<=INHIB; k+=2,++iei) {
         char *pnm = pea->exinnm[iei].rnm;
         for (ke=k+12; k<ke; pnm+=LSNAME,k+=6) {
            if (pnm[0] == '\0') memset(line+k, '-', LSNAME);
            else for (j=0; j<LSNAME && pnm[j] != '\0'; ++j)
               line[k+j] = pnm[j];
            }
         }
      convrt("(P4,3X,A28,I6,A27,2B7/14IH8.<4,2F8.<4)", fmteffnm(pea),
         &pea->nchgs, line, pea->eft, pea->escli, NULL);
      }

/*---------------------------------------------------------------------*
*               Print CELL TYPE specifications (part 1)                *
*---------------------------------------------------------------------*/

   cryout(RK_P4,"0CELLTYPE (LAYER) SPECIFICATIONS (PART 1):",
      RK_NFSUBTTL+RK_LN3,
      "   LNAME  CELLS   Cm     taum  RSPFN    PT     ST     NT "
      "    HT  SPK/SLP  Vrest    upsD  omegaD", RK_LN0+95, NULL);

   for (ir=RP->tree; ir; ir=ir->pnrp) {
      pdash(ir, 95);
      for (il=ir->play1; il; il=il->play) {
         ui32 bcdc = (FBwk + RP->bsdc) << RK_BS;
         int irf = (int)il->Ct.rspmeth;
         char spkslp[8];      /* Text for SPK/SPP column */
         switch (irf) {
         case RF_KNEE:
         case RF_STEP:
            wbcdwt(&il->Ct.sslope, spkslp, FBsc<<RK_BS|4<<RK_DS|
               RK_AUTO|RK_IORF|RK_NI32|6);
            break;
         case RF_SPIKE:
            wbcdwt(&il->Ct.sspike, spkslp, bcdc|4<<RK_DS|
               RK_AUTO|RK_IORF|RK_NI32|6);
            break;
         case RF_TANH:
            wbcdwt(&il->Ct.stanh, spkslp, bcdc|4<<RK_DS|
               RK_AUTO|RK_IORF|RK_NI32|6);
            break;
         default:
            strcpy(spkslp, "       ");
            } /* End rspmeth switch */
         /* Assume the 4 thresholds are adjacent in CELLTYPE */
         sconvrt(line,"(4XA" qLSN ",I6,B23UJ8.3,B20UJ8.<5,XA5,"
            qNCTT "B20/27IJ7.<4,A7,B20/27IJ8.<4)",
            il->lname, &il->nelt, &il->Ct.Cm, &il->Ct.taum, rfnms[irf],
            &il->Ct.pt, spkslp, &il->Ct.vrest, NULL);
         if (il->ctf & CTFDN)
            sconvrt(line,"(T80,B27/20IL8.<5,B30IL8.6)",
               &il->Dp.upsd, &il->Dp.omegad, NULL);
         cryout(RK_P4,line,RK_LN1,NULL);
         } /* End il loop */
      } /* End ir loop */

/*---------------------------------------------------------------------*
*               Print CELL TYPE specifications (part 2)                *
*  Note:  Shunting inhibition can be turned on by d3chng(), therefore  *
*  parameter siha is printed here whether or not used at this time.    *
*---------------------------------------------------------------------*/

   cryout(RK_P4,"0CELLTYPE (LAYER) SPECIFICATIONS (PART 2):",
      RK_NFSUBTTL+RK_LN3,
      "   LNAME  omega1   omega epsilon  SIHA   SiET  SiSCALE"
      "  SDAMP  QDAMP UPSPSI ZETAPSI OPTIONS", RK_LN0+91, NULL);

   for (ir=RP->tree; ir; ir=ir->pnrp) {
      pdash(ir, 91);
      if (ir->pionr) eflgs |= KEP_ION;
      for (il=ir->play1; il; il=il->play) {
         int kdecay = YES;    /* Decay info print switch */
         int kpsiop = (il->orkam & KAMLP) != 0;
         /* Set flags for printing parms for options */
         switch (il->Ct.rspmeth) {
         case RF_BREG:
            eflgs |= KEP_BREG, kdecay = NO; break;
         case RF_IZH3:
            eflgs |= KEP_IZHI3, kdecay = NO; break;
         case RF_IZH7:
            eflgs |= KEP_IZHI7, kdecay = NO; break;
            } /* End rspmeth switch */
         if (il->prfrc) eflgs |= KEP_REFRAC;
         if (il->pion1) eflgs |= KEP_ION;
         if (il->pcnd1) eflgs |= KEP_COND;
         convrt("(P4,4XA" qLSN ",B15UH8.5,R~B30UJ8.5,B30IJ8.5,"
            "2*B20/27IJ7.<4,B24IJ7.4,B15UH8.4,B15UH7.4,R~B22IJ7.<5,"
            "R~B15UH7.5,2XJ0KH13)", il->lname, &il->Dc.omega1, &kdecay,
            &il->Dc.omega, &il->Dc.epsilon, &il->Ct.siha, &il->Dc.siet,
            &il->Dc.siscl,&il->Dc.sdamp, &il->Dc.qdamp, &kpsiop,
            &il->Dc.upspsi, &kpsiop, &il->Dc.zetapsi, okctopt,
            &il->Ct.ctopt, NULL);
         } /* End il loop */
      } /* End ir loop */

/*---------------------------------------------------------------------*
*         Print AEIF (BRETTE-GERSTNER) neuron specifications           *
*---------------------------------------------------------------------*/

   if (eflgs & KEP_BREG) {
      cryout(RK_P4,"0aEIF (BRETTE-GERSTNER) CELL PARAMETERS:",
         RK_NFSUBTTL+RK_LN3, "   LNAME    gL     vPeak  vReset"
         "    vT     delT    tauW     A       B", RK_LN0+69, NULL);

      for (ir=RP->tree; ir; ir=ir->pnrp) {
         prtnam = TRUE;

         for (il=ir->play1; il; il=il->play) {
            if (il->Ct.rspmeth == RF_BREG) {
               struct BREGDEF *pbg = (struct BREGDEF *)il->prfi;
               if (prtnam) {
                  pdash(ir, 72); prtnam = FALSE; }
               convrt("(P4,4XA" qLSN "B20UJ8.<5,4B20/27IJ8.<5,"
                  "B20UJ8.<5,B24IJ8.<5,B20/27IJ8.<5)", il->lname,
                  &pbg->gL, &pbg->vPeak, &pbg->tauW, &pbg->bga,
                  &pbg->bgb, NULL);
               } /* End if RF_BREG */
            } /* End il loop */
         } /* End ir loop */
      } /* End printing aEIF info */

/*---------------------------------------------------------------------*
*            Print IZHIKEVICH (2003) neuron specifications             *
*---------------------------------------------------------------------*/

   if (eflgs & KEP_IZHI3) {

      cryout(RK_P4,"0IZHIKEVICH (2003) CELL PARAMETERS:",
         RK_NFSUBTTL+RK_LN3,
         "   LNAME         A       B       C       D    Vpeak    Vur  "
         "  Vrest     CV2     CV1     CV0", RK_LN0+91, NULL);

      for (ir=RP->tree; ir; ir=ir->pnrp) {
         prtnam = TRUE;

         for (il=ir->play1; il; il=il->play) {
            if (il->Ct.rspmeth == RF_IZH3) {
               struct IZ03DEF *pi3 = (struct IZ03DEF *)il->prfi;
               if (prtnam) {
                  pdash(ir, 94); prtnam = FALSE; }
               convrt("(P4,4XA" qLSN ",4X,2B28IJ8.<5,B20/27IJ8.<5,"
                  "B22/29IJ8.<5,2B20/27IJ8.<5,B20/27IJ8.<5,"
                  "#3,B30/23IJ8.<5,B24IJ8.<5,B20/27IJ8.<5)", il->lname,
                  &pi3->Z.iza, &pi3->Z.izc, &pi3->Z.izd, &pi3->Z.vpeak,
                  &pi3->i3vr, &pi3->izcv2, NULL);
               if (il->nizvar) convrt("(9X,3H+/-,2B14UH8.<5,"
                  "2B7/14UH8.<5,', SEED = ',J0IJ12)",
                     &pi3->Z.izra, &pi3->Z.izrc, &pi3->Z.izseed, NULL);
               } /* End if RF_IZH3 */
            } /* End il loop */
         } /* End ir loop */
      } /* End printing Izhikevich 2003 info */

/*---------------------------------------------------------------------*
*            Print IZHIKEVICH (2007) neuron specifications             *
*---------------------------------------------------------------------*/

   if (eflgs & KEP_IZHI7) {
      int kex = NO;           /* TRUE to print extensions */

      cryout(RK_P4,"0IZHIKEVICH (2007) CELL PARAMETERS:",
         RK_NFSUBTTL+RK_LN3,
         "   LNAME         A       B       C       D    VPEAK    VUR  "
         "     K       VT", RK_LN0+75, NULL);

      for (ir=RP->tree; ir; ir=ir->pnrp) {
         prtnam = TRUE;

         for (il=ir->play1; il; il=il->play) {
            il->ctwk.tree.krmop &= ~KRM_PIEXT;
            if (il->Ct.rspmeth == RF_IZH7) {
               struct IZ07DEF *pi7 = (struct IZ07DEF *)il->prfi;
               /* Check for extensions */
               if ((pi7->umc | pi7->umvp | pi7->uv3) != 0 ||
                     pi7->uv3lo != SI32_SGN || pi7->umax != SI32_MAX ||
                     pi7->bvlo != SI32_SGN)
                  kex |= il->ctwk.tree.krmop |= KRM_PIEXT;
               if (prtnam) {
                  pdash(ir, 76); prtnam = FALSE; }
               convrt("(P4,4XA" qLSN ",4X,B28IJ8.<5,B23IJ8.<5,"
                  "B20/27IJ8.<5,B19/26IJ8.<5,2B20/27IJ8.<5,"
                  "B24/17IJ8.<5,B20/27IJ8.<5)", il->lname, &pi7->Z.iza,
                  &pi7->iizb, &pi7->Z.izc, &pi7->iizd, &pi7->Z.vpeak,
                  &pi7->izk, &pi7->izvt, NULL);
               if (il->nizvar) convrt("(9X,3H+/-,B14UH8.<5,"
                  "B10UH8.<5,2*B7/14UH8.<5,', SEED = ',J0IJ12)",
                  &pi7->Z.izra, &pi7->iizrb, &pi7->Z.izrc,
                  &pi7->iizrd, &pi7->Z.izseed, NULL);
               } /* End if RF_IZH7 */
            } /* End il loop */
         } /* End ir loop */

      /* Print extension specs if any extensions used */
      if (kex & KRM_PIEXT) {
         int kumvp,kumc,kuv3,kuv3lo,kumax,kblo;
         cryout(RK_P4,"0IZHIKEVICH MODEL EXTENSION PARAMETERS:",
            RK_NFSUBTTL+RK_LN3,
            "   LNAME     UMVP     UMC     UV3   UV3LO    UMAX    BVLO",
            RK_LN0+58, NULL);

         for (ir=RP->tree; ir; ir=ir->pnrp) {
            prtnam = TRUE;

            for (il=ir->play1; il; il=il->play) {
               if (il->ctwk.tree.krmop & KRM_PIEXT) {
                  struct IZ07DEF *pi7 = (struct IZ07DEF *)il->prfi;
                  kumvp = pi7->umvp != 0;
                  kumc  = pi7->umc != 0;
                  kuv3  = pi7->uv3 != 0;
                  kuv3lo= pi7->uv3lo != SI32_SGN;
                  kumax = pi7->umax != SI32_MAX;
                  kblo  = pi7->bvlo != SI32_SGN;
                  if (prtnam) {
                     pdash(ir, 58); prtnam = FALSE; }
                  convrt("(P4,4XA" qLSN "R~B28IJ10.<5,R~B28IJ8.<5,"
                     "R~B30IJ8.<5,R~B30IJ8.<5,R~B19/26IJ8.<5,"
                     "R~B23IJ8.<5)",
                     il->lname, &kumvp, &pi7->umvp, &kumc, &pi7->umc,
                     &kuv3, &pi7->uv3, &kuv3lo, &pi7->uv3lo,
                     &kumax, &pi7->umax, &kblo, &pi7->bvlo, NULL);
                  } /* End if RF_IZH7 and has extensions */
               } /* End il loop */
            } /* End ir loop */
         } /* End if kex */
      } /* End printing Izhikevich 2007 info */

/*---------------------------------------------------------------------*
*                   Print AUTOSCALE specifications                     *
*  Note:  Deliberately printed if entered, even if navg or kaut == 0,  *
*  so user can see why autoscaling did not occur.                      *
*---------------------------------------------------------------------*/

   if (RP0->nautsc > 0) {
      cryout(RK_P4,"0AUTOSCALE PARAMETERS:",RK_NFSUBTTL+RK_LN3,
         "   LNAME    ASTT  ASTF1  ASTFL   NAVG   ADAMP   MXDAS"
         "   MNIMM   MXIMM   OPTS", RK_LN0+76, NULL);

      for (ir=RP->tree; ir; ir=ir->pnrp) {
         prtnam = TRUE;

         for (il=ir->play1; il; il=il->play) {
            struct AUTSCL *paut;
            if (paut = il->pauts) { /* Assignment intended */
               if (prtnam) {
                  pdash(ir, 79); prtnam = FALSE; }
               convrt("(P4,4XA" qLSN "B7/14IH8.<4,2B15UH7.<4,IH7,"
                  "B15UH8.5,3B24IJ8.<4,3XJ0KH11)", il->lname,
                  &paut->astt, &paut->astf1, &paut->navg, &paut->adamp,
                  &paut->asmxd, okautsc, &paut->kaut, NULL);
               } /* End if il->pauts */
            } /* End il loop */
         } /* End ir loop */
      } /* End printing autoscale info */

/*---------------------------------------------------------------------*
*                  Print CONDUCTANCE specifications                    *
*---------------------------------------------------------------------*/

   if (eflgs & KEP_COND) {
      cryout(RK_P4,"0CONDUCTANCE SPECIFICATIONS:", RK_NFSUBTTL+RK_LN3,
         "   LNAME  CONDUCTANCE         Gbar     Vrev   GEXP  OPT "
         "REFR  IONCON  IONDEP     GFAC    IONACTV", RK_LN0+96, NULL);

      for (ir=RP->tree; ir; ir=ir->pnrp) {
         prtnam = TRUE;

         for (il=ir->play1; il; il=il->play) {
            if (il->pcnd1) {
               struct CONDUCT *pcnd;
               int prtlnm = 1;
               if (prtnam) {
                  pdash(ir, 92); prtnam = FALSE; }
               for (pcnd=il->pcnd1; pcnd; pcnd=pcnd->pncd) {
                  /* Print first line with common info */
                  convrt("(P4L3,4XR~A" qLSN ",2XA16,B20UL10.5,B20/27"
                     "IL8.3,B8IH6.2,2XKC3,UH5)", &prtlnm, il->lname,
                     getrktxt(pcnd->hcdnm), &pcnd->gbar, &pcnd->Vrev,
                     &pcnd->gexp,okcdopt, &pcnd->cdopts, &pcnd->refrac,
                     NULL);
                  prtlnm = 0;
                  memset(line, ' ', 36);
                  if (pcnd->cdvin & CDKY_IONC) sconvrt(line,
                     "(OA7)", getrktxt(pcnd->hionr), NULL);
                  if (pcnd->cdvin & CDKY_IOND) sconvrt(line,
                     "(T9OA7,XE10.4)", getrktxt(pcnd->hiond),
                     &pcnd->gfac, NULL);
                  if (pcnd->cdvin & CDKY_IONA) sconvrt(line,
                     "(T29A8)", getrktxt(pcnd->uga.ion.hiont), NULL);
                  cryout(RK_P4, line, 36, NULL);
                  /* Print a second line with type, activation info */
                  memset(line, ' ', 92);
                  switch (pcnd->kcond) {
                  case CDTP_PASSIVE:
                     memcpy(line+12, "PASSIVE", 7);
                     break;
                  case CDTP_LINEAR:
                     sconvrt(line, "(OT13,13HLINEAR  Vth  ,B20/27"
                        "IL9.5,7H  Vmax ,B20/27IL9.5)",
                        &pcnd->ugt.lin.vth, &pcnd->ugt.lin.vmax,
                        NULL);
                     break;
                  case CDTP_ALPHA:
                     sconvrt(line, "(OT13,13HALPHA   TTPK ,B20UL9.5,"
                        "7H  tauM ,B20UJ9.5)",
                        &pcnd->ugt.alf.ttpk, &pcnd->ugt.alf.taum,
                        NULL);
                     break;
                  case CDTP_DOUBLE:
                     sconvrt(line, "(OT13,13HDBLEXP  tauR ,F9.5,"
                        "7H  tauD ,F9.5)",
                        &pcnd->ugt.dbl.taur, &pcnd->ugt.dbl.taud,
                        NULL);
                     } /* End kcond switch */
                  switch (pcnd->kactv) {
                  case CDAC_CELLFIRE:
                     sconvrt(line, "(T53,13HCELLFIRE  AT ,B20/27"
                        "IL10.5)", &pcnd->uga.at, NULL);
                  case CDAC_SYNAPTIC:
                     sconvrt(line, "(T53,13HSYNAPTIC  AT ,B20/27"
                        "IL10.5)", &pcnd->uga.at, NULL);
                     break;
                  case CDAC_IONIC:
                     sconvrt(line, "(T53,4HION A8,1H(A1,6H)  TH VF9.<5"
                        ",6H REAC VF9.<5)",
                        getrktxt(pcnd->uga.ion.hiont),
                        pcnd->cdflgs & CDFL_ACTEXT ? "E" : "I",
                        pcnd->uga.ion.ionth, pcnd->uga.ion.react, NULL);
                     break;
                  case CDAC_POISSON:
                     sconvrt(line, "(T53,15HPOISSON   PPPC ,B28UL7.6,"
                        "7H  SEED ,UJ11)",
                        &pcnd->uga.psn.pppc, &pcnd->uga.psn.seed,
                        NULL);
                     } /* End kactv switch */
                  cryout(RK_P4, line, RK_LN0+92, NULL);
                  /* Print a third line with evaluation/decay info */
                  if (qdecay(&pcnd->gDCY)) {
                     memcpy(line+12, "DECAY  ", 7);
                     decayinfo(&pcnd->gDCY, line+19);
                     cryout(RK_P4, line, RK_LN0+39, NULL);
                     }
                  else convrt("(P4,12X,13HEVAL  CUTOFF ,F8.6,8H  "
                     "CYCLES,UH6)", &pcnd->cdcycut, &pcnd->nc2dc, NULL);
                  } /* End conductance loop */
               } /* End if pcnd */
            } /* End il loop */
         } /* End ir loop */
      } /* End printing conductance */

/*---------------------------------------------------------------------*
*                      Print ION specifications                        *
*---------------------------------------------------------------------*/

   if (eflgs & KEP_ION) {
      cryout(RK_P4,"0ION SPECIFICATIONS:", RK_NFSUBTTL+RK_LN3,
         "   LNAME  ION        Z  OPT            C0      Cmin  "
         "     VOL     TAU", RK_LN0+69, NULL);

      for (ir=RP->tree; ir; ir=ir->pnrp) {
         struct IONTYPE *pion;
         prtnam = TRUE;

         /* Print data for any region-scale ions */
         for (pion=ir->pionr; pion; pion=pion->pnion) {
            if (prtnam) {
               pdash(ir, 72); prtnam = FALSE; }
            convrt("(P4,10H REGIONAL A8,IH4,2XKC6,3HINT,4*VF10.<5)",
               getrktxt(pion->hionnm), &pion->z, okionop,
               &pion->ionopts, &pion->C0int, &pion->Cminint,
               &pion->Vint, &pion->tauint, NULL);
            convrt("(P4,30X3HEXT,4*VF10.<5)", &pion->C0ext,
               &pion->Cminext, &pion->Vext, &pion->tauext, NULL);
            } /* End ion loop */

         /* Print data for cell-level ions */
         for (il=ir->play1; il; il=il->play) {
            for (pion=il->pion1; pion; pion=pion->pnion) {
               if (prtnam) {
                  pdash(ir, 72); prtnam = FALSE; }
               convrt("(P4,4XA" qLSN ",2XA8,IH4,2XKC6,3HINT,4*VF10.<5)",
                  il->lname, getrktxt(pion->hionnm), &pion->z, okionop,
                  &pion->ionopts, &pion->C0int, &pion->Cminint,
                  &pion->Vint, &pion->tauint, NULL);
               convrt("(P4,30X3HEXT,4*VF10.<5)", &pion->C0ext,
                  &pion->Cminext, &pion->Vext, &pion->tauext, NULL);
               } /* End ion loop */
            } /* End il loop */
         } /* End ir loop */
      } /* End printing ion info */

/*---------------------------------------------------------------------*
*                     Print NOISE specifications                       *
*---------------------------------------------------------------------*/

   cryout(RK_P4,"0NOISE SPECIFICATIONS:",RK_NFSUBTTL+RK_LN3,
      "   LNAME    MEAN   sigma    FRAC        SEED  TYPE",
         RK_LN0+50, NULL);

   for (ir=RP->tree; ir; ir=ir->pnrp) {
      prtnam = TRUE;
      for (il=ir->play1; il; il=il->play) {
         if (il->No.nmn | il->No.nsg | il->No.nfr) {
            if (prtnam) { pdash(ir, 50); prtnam = FALSE; }
            convrt("(P4,4XA" qLSN "B20/27IJ8.<4,B24/31IJ8.<4,B24IJ8.4"
               "IJ12,2XJ0KC6)", il->lname, &il->No.nmn, &il->No.nsg,
               &il->No.nfr, &il->No.nsd, oknoise, &il->No.noiflgs,
               NULL);
            } /* End if noise exists */
         } /* End il loop */
      } /* End ir loop */

/*---------------------------------------------------------------------*
*               Print REFRACTORY PERIOD specifications                 *
*---------------------------------------------------------------------*/

   if (eflgs & KEP_REFRAC) {
      cryout(RK_P4,"0REFRACTORY PERIOD PARAMETERS:",RK_NFSUBTTL+RK_LN3,
         "   LNAME  REFRAC  upsDST omegaDST  PSDST    AHP",
         RK_LN0,NULL);

      for (ir=RP->tree; ir; ir=ir->pnrp) {
         prtnam = TRUE;

         for (il=ir->play1; il; il=il->play) {
            struct RFRCDATA *prf;
            if (prf = il->prfrc) {  /* Assignment intended */
               if (prtnam) {
                  pdash(ir, 48); prtnam = FALSE; }
               convrt("(P4,4XA" qLSN "UH6,B16IJ10.4,B30IJ8.6,2*B20/"
                  "27IJ8.<4)", il->lname, &prf->refrac, &prf->upsdst,
                  &prf->omegadst, &prf->psdst, &prf->ahp, NULL);
               } /* End if il->prfrc */
            } /* End il loop */
         } /* End ir loop */
      } /* End printing refractory period info */

/*---------------------------------------------------------------------*
*            Print CONNECTION TYPE specifications (part 1)             *
*     Includes Cij and most Lij (second code letter) parameters.       *
*---------------------------------------------------------------------*/

   {  char msub[] = "                             "
         "Type M:   MNO    NKS    NKT";
      char lsub[] = "                             "
         "Type L:   C00    C01    C11    C10   sigma  MAPOPT";
      int  mlen,llen,sblines = RK_NFSUBTTL+RK_LN2;
      mlen = llen = RK_SKIP;
      if (RP0->orkgen & KGNMC)
         mlen = RK_LN0+strlen(msub), sblines += 256;
      if (RP0->orkgen & KGNLM)
         llen = RK_LN0+strlen(lsub), sblines += 256;
      cryout(RK_P4, "0CONNECTION SPECIFICATIONS (PART 1):",
         sblines, msub, mlen, lsub, llen,
         "   LNAME  SOURCE      NC KGEN    NBC  MEAN  sigma    PP "
         " NRX NRY STR NUX NUY XOF YOF OFF WID",RK_LN0+92,NULL);
      }

   for (ir=RP->tree; ir; ir=ir->pnrp) {
      prtnam = TRUE;

      for (il=ir->play1; il; il=il->play) {
         for (ix=il->pct1; ix; ix=ix->pct) {
            ui32 kode = ix->kgen, pkode = ix->kgen;
            int ktrig = TRUE;
            if (prtnam) {
               pdash(ir, 92); prtnam = FALSE; }
            /* Don't display info for subsequent connections if
            *  there aren't any, but leave kgen bits for "jic" */
            if (ix->nc <= 1) kode &=
               ~(KGNAJ|KGNBL|KGNCF|KGNIN|KGNPT);
            /* Make 'L' code print if have KGNTW */
            if (kode & KGNTW) pkode |= KGNLM;
            sconvrt(line,"(4XA" qLSN ",2X2A" qLSN "UJ6,XKL6IH4,57X)",
               il->lname, &ix->srcnm.rnm, &ix->nc, okcnkgen,
               &pkode, &ix->Cn.nbc, NULL);
            if (kode & KGNRC)
               sconvrt(line,"(OT36,B24IJ7.4B28IJ7.4B16IJ7.4)",
                  &ix->ucij.r.cmn, &ix->ucij.r.csg, &ix->ucij.r.pp,
                  NULL);
            else if (kode & KGNMC)
               sconvrt(line,"(OT36,3*I7)", &ix->ucij.m.mno,
                  &ix->ucij.m.nks, &ix->ucij.m.nkt, NULL);
            else if (kode & KGNLM) {   /* Use another line */
               sconvrt(line,"(OT36,4B24IJ7.4B28IJ8.4,2XKH12)",
                  &ix->ucij.l.c00, &ix->ucij.l.csg,okcnmop,
                  &ix->ucij.l.flags ,NULL);
               cryout(RK_P4,line,RK_LN1,NULL);
               memset(line,' ',92); ktrig = FALSE;
               }
            /* Print radius and PP = '0' or '1' if K2 specified */
            if ((kode & (KGNKN|KGNE2)) == (KGNKN|KGNE2)) {
               wbcdwt(&ix->hradius, line+42,
                  16<<RK_BS|4<<RK_DS|RK_NUNS|RK_IORF|RK_NI32|6);
               line[55] = ix->knopt & KNO_INHIB ? '0' : '1';
               }
            /* Print nrx,nry if B,C,K,Q, or M specified */
            if (kode & (KGNMC+KGNBL+KGNCF+KGNKN+KGNAN)) {
               sconvrt(line,"(OT57,2UH4)", &ix->nrx, NULL);
               ktrig = TRUE; }
            /* Print stride if A,B, or K specified */
            if (kode & (KGNAJ+KGNBL+KGNKN)) {
               sconvrt(line,"(OT65,IJ4)", &ix->stride, NULL);
               ktrig = TRUE; }
            /* Print nux,nuy,xoff,yoff if F,J,S,T,X,Y,W specified */
            if (kode & (KGNFU+KGNJN+KGNST+KGNTP+KGNXO+KGNYO+KGNBV)) {
               sconvrt(line,"(OT69,2IJ4,2IJ4)",
                  &ix->nux, &ix->xoff, NULL);
               ktrig = TRUE; }
            /* Print loff if J,W,X, or Y option specified */
            if (kode & (KGNJN+KGNHV+KGNXO+KGNYO)) {
               wbcdwt(&ix->loff, line+84, RK_IORF|RK_NI32|3);
               ktrig = TRUE; }
            /* Print width if J,N,O, or Y option specified */
            if (kode & (KGNJN+KGNND+KGNOG+KGNYO)) {
               wbcdwt(&ix->lsg, line+88, RK_BSCL|RK_IORF|RK_NI32|3);
               ktrig = TRUE; }
            /* Print radius if W option specified */
            if (kode & KGNHV) {
               wbcdwt(&ix->hradius, line+88,
                  16<<RK_BS|2<<RK_DS|RK_IORF|RK_NI32|3);
               ktrig = TRUE; }
            if (ktrig) cryout(RK_P4,line,RK_LN1+92,NULL);
            /* Print line with nhx, nhy if 'Q' specified */
            if (kode & KGNAN) convrt("(P4,40X16HCode Q: NHX NHY:2IH4)",
               &ix->nhx, NULL);
            } /* End ix loop */
         } /* End il loop */
      } /* End ir loop */

/*---------------------------------------------------------------------*
*            Print CONNECTION TYPE specifications (part 2)             *
*         Includes threshold, scaling, and option parameters           *
*---------------------------------------------------------------------*/

/*  N.B.  Add cbc to this output when implemented fully.  */

   cryout(RK_P4,"0CONNECTION SPECIFICATIONS (PART 2):",
      RK_NFSUBTTL + RK_LN3,
      "   LNAME  SOURCE     ETHI   ETLO  EMxSJ  SjREV  SCALE TCWID"
      "   CMIN   MNAX   MXAX  RDAMP  OPTIONS", RK_LN0+96, NULL);

   for (ir=RP->tree; ir; ir=ir->pnrp) {
      prtnam = TRUE;

      for (il=ir->play1; il; il=il->play) {
         for (ix=il->pct1; ix; ix=ix->pct) {
            int ktcwid =
               ix->kgen & (KGNKN|KGNTW) || ix->Cn.cnopt & NOPVP;
            int krdamp = cnexists(ix, RBAR);
            int isclb = (int)ix->sclb;
            if (prtnam) {
               pdash(ir,96); prtnam = FALSE; }
            setbscompat(ix->cnsrctyp);
            convrt("(P4,4XA" qLSN ",2X2A" qLSN ",4*B7|14IH7.<4,BIJ7.A,"
               "R~F6.<3,VB31IJ7.5,2*B20/27IJ7.<4,R~B15UH7.5,2XJ0KH10)",
               il->lname, &ix->srcnm.rnm, &ix->Cn.et, &ix->Cn.etlo,
               &ix->Cn.emxsj, &ix->Cn.sjrev, &isclb, &ix->Cn.scl,
               &ktcwid, &ix->Cn.tcwid, &ix->cijmin, &ix->Cn.mnax,
               &ix->Cn.mxax, &krdamp, &ix->Cn.rdamp, okcnopt,
               &ix->Cn.cnopt, NULL);
            /* Set flags for printing less common options */
            if (ix->Cn.kam & ANYAMASK)             eflgs |= KEP_AMP;
            if (ix->Cn.kam & (KAMSA|KAMTS|KAMUE))  eflgs |= KEP_SLOWA;
            if (ix->Cn.kdecay || qdecay(&ix->Cn.ADCY)) eflgs |= KEP_DECAY;
            if (ix->Cn.kdelay)                     eflgs |= KEP_DELAY;
            if (ix->PP)                            eflgs |= KEP_PPF;
            if (ix->Cn.saopt & DOSARB)             eflgs |= KEP_SUBARB;
            if (ix->Cn.vdopt & VDANY)              eflgs |= KEP_VDEP;
            } /* End ix loop */
         } /* End il loop */
      } /* End ir loop */

/*---------------------------------------------------------------------*
*                 Print AMPLIFICATION specifications                   *
*---------------------------------------------------------------------*/

   if (eflgs & KEP_AMP) {
      cryout(RK_P4,"0AMPLIFICATION SPECIFICATIONS:",RK_NFSUBTTL+RK_LN3,
         "   LNAME  SOURCE    RULE        delta   MTI    MTJ    MTV "
         "   MNSI   MNSJ   MXSI  QPULL VI  PHIFN", RK_LN0+96, NULL);

      for (ir=RP->tree; ir; ir=ir->pnrp) {
         prtnam = TRUE;

         for (il=ir->play1; il; il=il->play) {
            for (ix=il->pct1; ix; ix=ix->pct)
                  if (ix->Cn.kam & ANYAMASK) {
               int kqpull = (ix->Cn.kam & KAMDQ) != 0;
               if (prtnam) {
                  pdash(ir, 96); prtnam = FALSE; }
               setbscompat(ix->cnsrctyp);
               convrt("(P4,4XA" qLSN ",2X2A" qLSN ",2XKJ9,XB28IJ7.4,"
                  "B7/14IH7.<4,B7|14IH7.<4,B" qqv(FBvl) "IH7.4,"
                  "B7/14IH7.<4,B7|14IH7.<4,B7/14IH7.<4,R~B15UH7.5,"
                  "IH3,2XJ0A5)",
                  il->lname, &ix->srcnm.rnm, okcnkam, &ix->Cn.kam,
                  &ix->Cn.mdelta, &ix->Cn.mti, &ix->Cn.mtj, &ix->Cn.mtv,
                  &ix->Cn.mnsi, &ix->Cn.mnsj, &ix->Cn.mxsi, &kqpull,
                  &ix->Cn.qpull, &ix->Cn.vi, phifnnm[1][ix->Cn.phifn],
                  NULL);
               if (ix->Cn.kam & KAMEX || ix->Cn.nwayrd > 0) {
                  int mxlrn = ix->Cn.kam & KAMVNU ?
                     MAX_WAYSET_CODES : MAX_WAYSET_CODES/2;
                  convrt("(P4,20X,12HWAY SCALES  ,RB10IH8.4)",
                     &mxlrn, ix->Cn.wayset, NULL);
                  }
               } /* End ix loop */
            } /* End il loop */
         } /* End ir loop */
      } /* End KEP_AMP if */

/*---------------------------------------------------------------------*
*               Print SLOW AMPLIFICATION specifications                *
*---------------------------------------------------------------------*/

   if (eflgs & KEP_SLOWA) {
      cryout(RK_P4,"0SLOW AMPLIFICATION PARAMETERS:",
         RK_NFSUBTTL+RK_LN3,
         "   LNAME  SOURCE    upsM   zetaM   MXMij    MXMP MTICK",
         RK_LN0+54, NULL);

      for (ir=RP->tree; ir; ir=ir->pnrp) {
         prtnam = TRUE;

         for (il=ir->play1; il; il=il->play) {
            for (ix=il->pct1; ix; ix=ix->pct) {
               if (ix->Cn.kam & (KAMSA|KAMTS|KAMUE)) {
                  int ktick = (ix->Cn.kam & KAMTS) ? 1 : 0;
                  if (prtnam) {
                     pdash(ir, 54); prtnam = FALSE; }
                  convrt("(P4,4XA" qLSN ",2X2A" qLSN "B16IJ6.4"
                     "B16IJ8.4,B14IH8.<4,B20IJ8.<4,RIH5)",
                     il->lname, &ix->srcnm.rnm, &ix->Cn.upsm,
                     &ix->Cn.zetam, &ix->Cn.mxmij, &ix->Cn.mxmp,
                     &ktick, &ix->Cn.mticks, NULL);
                  }
               } /* End ix loop */
            } /* End il loop */
         } /* End ir loop */
      } /* End KEP_SLOWA if */

/*---------------------------------------------------------------------*
*           Print PAIRED-PULSE FACILITATION specifications             *
*---------------------------------------------------------------------*/

#define LN2S29 3.721305590E8  /* Ln(2) on scale S29 */

   if (eflgs & KEP_PPF) {
      cryout(RK_P4,"0PAIRED-PULSE FACILITATION PARAMETERS:",
         RK_NFSUBTTL+RK_LN3,
         "   LNAME  SOURCE      PPFT    upsP    tauP  PPFLIM"
         "    HTUP    HTDN", RK_LN0, NULL);

      for (ir=RP->tree; ir; ir=ir->pnrp) {
         prtnam = TRUE;
         for (il=ir->play1; il; il=il->play) {
            struct PPFDATA *ppfd;
            for (ix=il->pct1; ix; ix=ix->pct)
                  if (ppfd = ix->PP) {    /* Assignment intended */
               if (prtnam) {
                  pdash(ir, 66); prtnam = FALSE; }
               setbscompat(ix->cnsrctyp);
               if (ppfd->ppfopt & PPFABR)
                  convrt("(P4,4XA" qLSN ",2X2A" qLSN "B7|14IH8.<4,"
                     "16H  ABRUPT  ABRUPT,B12IH8.5,2*B1IH8.2)",
                     il->lname, &ix->srcnm.rnm, &ppfd->ppft,
                     &ppfd->ppflim, &ppfd->htup, &ppfd->htdn, NULL);
               else {
                  ppfd->upsp = (si32)(LN2S29/(double)ppfd->htup + 0.5);
                  ppfd->taup = (si32)(LN2S29/(double)ppfd->htdn + 0.5);
                  convrt("(P4,4XA" qLSN ",2X2A" qLSN "B7|14IH8.<4,"
                     "2*B28IJ8.6B12IH8.5,2*B1IH8.2)", il->lname,
                     &ix->srcnm.rnm, &ppfd->ppft, &ppfd->upsp,
                     &ppfd->taup, &ppfd->ppflim, &ppfd->htup,
                     &ppfd->htdn, NULL);
                  }
               } /* End ix loop-if */
            } /* End il loop */
         } /* End ir loop */
      } /* End KEP_PPF if */

/*---------------------------------------------------------------------*
*                     Print DELAY specifications                       *
*                                                                      *
*  Rev, 03/14/09, GNR - Delay seed moved to random number section      *
*---------------------------------------------------------------------*/

   if (eflgs & KEP_DELAY) {
      cryout(RK_P4,"0PRESYNAPTIC DELAY PARAMETERS:",
         RK_NFSUBTTL+RK_LN3,"   LNAME  SOURCE    KDLY MXDLY"
         "    MEAN   sigma",RK_LN0,NULL);

/* Loop over repertoires, setting a trigger to print the name */

      for (ir=RP->tree; ir; ir=ir->pnrp) {
         prtnam = TRUE;

/* Loop over cell types, printing delay information */

         for (il=ir->play1; il; il=il->play) {
            for (ix=il->pct1; ix; ix=ix->pct) {
               if (ix->Cn.kdelay) {
                  if (prtnam) {
                     pdash(ir, 46); prtnam = FALSE; }
                  if (ix->Cn.kdelay == DLY_CONST)
                     convrt("(P4,4XA" qLSN ",2X2A" qLSN ",2XA4UC6)",
                        il->lname, &ix->srcnm.rnm, kdlyo[ix->Cn.kdelay],
                        &ix->Cn.mxcdelay, NULL);
                  else
                     convrt("(P4,4XA" qLSN ",2X2A" qLSN ",2XA4UC6"
                        "B20IJ8.4B24IJ8.4)",
                        il->lname, &ix->srcnm.rnm, kdlyo[ix->Cn.kdelay],
                        &ix->Cn.mxcdelay, &ix->Cn.dmn, &ix->Cn.dsg,
                        NULL);
                  }
               } /* End ix loop */
            } /* End il loop */
         } /* End ir loop */
      } /* End KEP_DELAY if */

/*---------------------------------------------------------------------*
*           Print EPSP and connection DECAY specifications             *
*---------------------------------------------------------------------*/

   if (eflgs & KEP_DECAY) {
      cryout(RK_P4,"0EPSP/IPSP AND SYNAPTIC STRENGTH DECAY:",
         RK_NFSUBTTL+RK_LN3, "   LNAME  SOURCE     kDCY  omega"
         " om2/HE   gamma  TARGET    rho  VDI", RK_LN0+67, NULL);

      for (ir=RP->tree; ir; ir=ir->pnrp) {
         prtnam = TRUE;

         for (il=ir->play1; il; il=il->play) {
            for (ix=il->pct1; ix; ix=ix->pct) {
               if (ix->Cn.kdecay || qdecay(&ix->Cn.ADCY)) {
                  if (prtnam) {
                     pdash(ir, 67); prtnam = FALSE; }
                  decayinfo(&ix->Cn.ADCY, line);
                  convrt("(P4,4XA" qLSN ",2X2A" qLSN
                     ",2X,A20,B24IJ7.4B16IJ8.4B24IJ8.4IH4)",
                     il->lname, &ix->srcnm.rnm, line, &ix->Cn.gamma,
                     &ix->Cn.target, &ix->Cn.rho, &ix->Cn.dvi, NULL);
                  }
               } /* End ix loop */
            } /* End il loop */
         } /* End ir loop */
      } /* End KEP_DECAY if */

/*---------------------------------------------------------------------*
*                     Print PHASE specifications                       *
*---------------------------------------------------------------------*/

   cryout(RK_P4,"0PHASE PARAMETERS:",RK_NFSUBTTL+RK_LN3,
      "   LNAME  SOURCE     PHT   SIMETH         PHIMETH    OPT"
      "  RETARD  PCF", RK_LN0, NULL);

   for (ir=RP->tree; ir; ir=ir->pnrp) {
      prtnam = TRUE;

      for (il=ir->play1; il; il=il->play) {
         if (il->phshft) {
            struct PHASEDEF *ppd = il->pctpd;
            char phtxt[42];
            if (prtnam) {
               pdash(ir, 69); prtnam = FALSE; }
            strcpy(phtxt, siopt[ppd->simeth-1]);
            if (ppd->simeth == SI_CONST) sconvrt(phtxt+8,
               "(J0B20/27IJ6.<4,H))", &ppd->fixsi, NULL);
            strcpy(phtxt+16, fiopt[ppd->phimeth-1]);
            if (ppd->phimeth == PHI_CONST) sconvrt(phtxt+24,
               "(J0UC3H))", &ppd->fixpha, NULL);
            convrt("(P4,4XA" qLSN ",9H CelltypeB20/27IJ8.<4"
               "A15A13J0KC5)", il->lname, &ppd->pht,
               phtxt, phtxt+16, okphopt, &ppd->phops, NULL);
            /* Phase parameters for specific connection types */
            for (ix=il->pct1,ict=1; ix; ix=ix->pct,ict++) {
               memset(phtxt, ' ', 24);
               if (ix->phopt & (PHASJ|PHASR)) {
                  if (ix->phopt & PHASJ) {
                     memcpy(phtxt, "INPUT", 5);
                     wbcdwt(&ix->Cn.retard, phtxt+14,
                        RK_NBYTE+RK_IORF+5); }
                  else
                     memcpy(phtxt, "RANDOM", 6);
                  }
               else {
                  if (ix->phopt & PHASU) memcpy(phtxt, "UNIFORM", 7);
                  else sconvrt(phtxt, "(O6HCONST(,J0UC3,H))",
                     &ix->Cn.phi, NULL);
                  }
               if (ix->Cn.ppcf)
                  strncpy0(phtxt+24, getrktxt(ix->Cn.ppcf->hpcfnm), 17);
               else
                  phtxt[24] = '\0';
               convrt("(P4,4XA" qLSN ",2X,2A" qLSN ",8H  (CTypeI3H),"
                  "12XJ0A42)", il->lname, &ix->srcnm.rnm, &ict,
                  phtxt, NULL);
               } /* End ix loop */
            } /* End if phased */
         } /* End il loop */
      } /* End ir loop */

/*---------------------------------------------------------------------*
*          Print VOLTAGE-DEPENDENT connection specifications           *
*---------------------------------------------------------------------*/

   if (eflgs & KEP_VDEP) {
      cryout(RK_P4,"0VOLTAGE-DEPENDENT CONNECTIONS:",
         RK_NFSUBTTL+RK_LN3,
         "   LNAME  SOURCE      VDT     VDHA    VDMM  TYPE",
            RK_LN0+48, NULL);

      for (ir=RP->tree; ir; ir=ir->pnrp) {
         prtnam = TRUE;

         for (il=ir->play1; il; il=il->play) {
            for (ix=il->pct1; ix; ix=ix->pct) {
               if (ix->Cn.vdopt & VDANY) {
                  if (prtnam) {
                     pdash(ir, 48); prtnam = FALSE; }
                  convrt("(P4,4XA" qLSN ",2X2A" qLSN ",2*B20/27IJ8.<4,"
                     "B" qqv(FBsc) "IJ8.<4,3XJ0KC5)",il->lname,
                     &ix->srcnm.rnm, &ix->Cn.vdt, &ix->Cn.vdha,
                     &ix->Cn.vdmm, okvdopt, &ix->Cn.vdopt, NULL);
                  }
               } /* End ix loop */
            } /* End il loop */
         } /* End ir loop */
      } /* End KEP_VDEP if */

/*---------------------------------------------------------------------*
*                    Print SUBARBOR specifications                     *
*---------------------------------------------------------------------*/

   if (eflgs & KEP_SUBARB) {
      cryout(RK_P4,"0SUBARBORS:",RK_NFSUBTTL+RK_LN3,
         "   LNAME  SOURCE     NSA  NSAX  NSAY  OPT",RK_LN0,NULL);

      for (ir=RP->tree; ir; ir=ir->pnrp) {
         prtnam = TRUE;

         for (il=ir->play1; il; il=il->play) {
            for (ix=il->pct1; ix; ix=ix->pct) {
               if (ix->Cn.saopt & DOSARB) {
                  ui32 tnsxy,tnsaxy[2];
                  int  knsaxy = (ix->Cn.saopt & (SAOPTL|SAOPTI)) == 0;
                  if (prtnam) {
                     pdash(ir, 41); prtnam = FALSE; }
                  if (knsaxy) {
                     tnsxy = ix->nsa*ix->nsax;
                     tnsaxy[0] = ix->nsax;
                     tnsaxy[1] = (ix->nc + tnsxy - 1)/tnsxy;
                     }
                  convrt("(P4,4XA" qLSN ",2X2A" qLSN ",UJ6,2R~UJ6,"
                     "2XJ0KC5)", il->lname, &ix->srcnm.rnm, &ix->nsa,
                     &knsaxy, tnsaxy, oksaopt, &ix->Cn.saopt, NULL);
                  }
               } /* End ix loop */
            } /* End il loop */
         } /* End ir loop */
      } /* End KEP_SUBARB if */

/*---------------------------------------------------------------------*
*                     Print GCONN specifications                       *
*  N.B.  After the scales are printed, they are area-adjusted.         *
*---------------------------------------------------------------------*/

#define wdbi   8              /* Width of band info */
#define mxbpl  8              /* Max bands per line */
   if (RP0->mxnib > 0) {
      int inib,lnib,nnib = min(RP0->mxnib, mxbpl);
      cryout(RK_P4,"0GEOMETRIC CONNECTIONS:",RK_NFSUBTTL+RK_LN3,
         "   LNAME  SOURCE   OPT     BC BANDS WDTH  ITHI   ITLO"
         "  GjREV   GBCM kDCY  omega om2/HE DLAY DFER",RK_LN0+96,NULL);

      for (ir=RP->tree; ir; ir=ir->pnrp) {
         prtnam = TRUE;

         for (il=ir->play1; il; il=il->play) {
            for (ib=il->pib1; ib; ib=ib->pib) {
               if (prtnam) {
                  pdash(ir, 96); prtnam = FALSE; }

               /* Print inhibblk parameters */
               decayinfo(&ib->GDCY, line);
               convrt("(P4,4XA" qLSN ",2X2A" qLSN ",XKH6A4"
                  "UH4UH6,3*B7/14IH7.<4B24IJ7.<5A20UH4UH4)", il->lname,
                  &ib->gsrcnm.rnm, okibopt, &ib->ibopt, bcopt[ib->kbc],
                  &ib->nib, &ib->ngb, &ib->itt, &ib->ittlo, &ib->gjrev,
                  &ib->gbcm, line, &ib->ihdelay, &ib->gdefer, NULL);

               } /* End ib loop */
            } /* End il loop */
         } /* End ir loop */

      for (inib=1,lnib=0; inib<=nnib; inib+=1,lnib+=wdbi)
         wbcdwt(&inib, line+lnib, RK_IORF+(wdbi-1));
      cryout(RK_P4,"0BAND SCALES AND MAXIMA:",RK_NFSUBTTL+RK_LN3,
      "   LNAME  SOURCE  BAND:",RK_LN0,line,lnib,NULL);

      structsize = sizeof(struct INHIBBAND);
      for (ir=RP->tree; ir; ir=ir->pnrp) if (ir->mxnib > 0) {
         pdash(ir, 23+lnib);
         for (il=ir->play1; il; il=il->play) {
            for (ib=il->pib1; ib; ib=ib->pib) {
               nnib = (int)ib->nib; /* (convrt() needs int) */

               convrt("(P4,4XA" qLSN ",2X2A" qLSN ",5X|^#(8B20IJ8.4;"
                  "/))", il->lname, &ib->gsrcnm.rnm, &structsize,
                  &nnib, &ib->inhbnd[0].beta, NULL);

               /* Print mxib's */
               pmxib = FALSE;
               for (inib=0; inib<nnib; inib++)
                  pmxib |= (ib->inhbnd[inib].mxib != SI32_MAX);
               if (pmxib) convrt(
                  "(P4,4X'MAXIMUM INHIBITION '|^#(8B20/27IJ8.<4;/))",
                  &structsize, &nnib, &ib->inhbnd[0].mxib, NULL);

               } /* End ib loop */
            } /* End il loop */
         } /* End ir loop */
      } /* End if mxnib */

/*---------------------------------------------------------------------*
*                    Print MODULATE specifications                     *
*---------------------------------------------------------------------*/

   cryout(RK_P4,"0MODULATION:",RK_NFSUBTTL+RK_LN3,
      "   LNAME  SOURCE    MTHI  MTLO MTOHI MTOLO  MjREV   MSCL"
      " DELAY  MXMOD kDCY  omega om2/HE  OPTS",RK_LN0+94,NULL);

   for (ir=RP->tree; ir; ir=ir->pnrp) {
      prtnam = TRUE;

      for (il=ir->play1; il; il=il->play) {
         if (il->ctf & (CTFMT|CTFMN)) {
            if (prtnam) {
               pdash(ir, 94); prtnam = FALSE; }
            for (imb=il->pmby1; imb; imb=imb->pmby) {
               decayinfo(&imb->MDCY, line);
               setbscompat(imb->msrctyp);
               convrt("(P4,4XA" qLSN ",2X,2A" qLSN ",4*B7|14IH6.<4,"
                  "B7|14IH7.<4B20IL7.4IC4,B20|27IJ9.<4,A20,XJ0KC7)",
                  il->lname, &imb->msrcnm.rnm, &imb->Mc.mt,
                  &imb->Mc.mtlo, &imb->mto, &imb->mtolo,
                  &imb->Mc.mjrev, &imb->mscl, &imb->Mc.mdelay,
                  &imb->mxmod, line, okmodop, &imb->Mc.mopt, NULL);
               if (imb->mct == 0)
                  cryout(RK_P4, " (Noise Mod)", RK_CCL+12, NULL);
               } /* End imb loop */
            } /* End if (CTFMT|CTFMN) */
         } /* End il loop */
      } /* End ir loop */

/*---------------------------------------------------------------------*
*                      Print RANDOM NUMBER SEEDS                       *
*---------------------------------------------------------------------*/

   cryout(RK_P4,"0RANDOM NUMBER GENERATING SEEDS:",RK_NFSUBTTL+RK_LN3,
      "   LNAME  SOURCE     COEFFICIENT  CONNECTION    DELAY "
      "     PHASING     ROUNDING     MASTER", RK_LN0+90, NULL);

   for (ir=RP->tree; ir; ir=ir->pnrp) {
      pdash(ir, 90);

      for (il=ir->play1; il; il=il->play) {
         int kphis = (int)il->phshft;
         convrt("(P4,4XA" qLSN ",9H Celltype,37X,R~IJ12,IJ12,IJ12)",
            il->lname, &kphis, &il->pctpd->phiseed, &il->rsd,
            &il->ctwk.tree.mastseed, NULL);

         for (ix=il->pct1; ix; ix=ix->pct) {
            int kdsd = ix->Cn.kdelay >= DLY_UNIF;
            convrt("(P4,4XA" qLSN ",2X2A" qLSN ",2IJ12,R~IJ12,"
               "3*IJ12)", il->lname, &ix->srcnm.rnm, &ix->cseed,
               &kdsd, &ix->Cn.dseed, &ix->phseed, &ix->rseed,
               &ix->cnwk.tree.mastseed, NULL);
            } /* End ix loop */
         } /* End il loop */
      } /* End ir loop */

/*---------------------------------------------------------------------*
*                  Print INPUT ARRAY specifications                    *
*---------------------------------------------------------------------*/

   cryout(RK_P4,"0INPUT ARRAY PARAMETERS:", RK_NFSUBTTL+RK_LN2, NULL);
   convrt("(P4,4X'Input array size',UH6,' x',UH4,4X'Stim pixels used'"
      "UH6,' x',UH4)", &RP->nsx, &RP->nsy, &RP0->kux, &RP0->kuy,
      NULL);
   cryout(RK_P4," ",RK_NTSUBTTL+RK_LN1+1,NULL);
   bscompat(RP0->bssel);   /* Restore normal kbs setting */

   } /* End of d3echo() */
