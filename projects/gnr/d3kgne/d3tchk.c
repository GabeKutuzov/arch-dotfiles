/* (c) Copyright 1986-2012, Neurosciences Research Foundation, Inc. */
/* $Id: d3tchk.c 52 2012-06-01 19:54:05Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                               d3tchk                                 *
*                                                                      *
*     Check dynamic repertoire tree, find connection sources.          *
*     Create MODVAL blocks for modulation and noise modulation.        *
*     Initialize celltype and conntype variables that do not depend    *
*        on node number (see d3nset) or Group III cards (see d3news).  *
*     Calculate defaults for nqx, nqy, nqg                             *
*        (needed for printing by d3echo before final setup stage).     *
************************************************************************
*                                                                      *
*  V1A, 01/25/86, G. N. Reeke - Split from d3tree                      *
*  V4A, 08/23/88, SCD - Translated to 'C' from version V3C             *
*  Rev, 11/09/89, JWT - Wallace code removed                           *
*  Rev, 03/28/90, GNR - Check consistency with max object ref num      *
*  Rev, 04/12/90, GNR - Check for overflow in scale factor             *
*  Rev, 04/30/90, JMS - Changed alloc for touch & kinesthesia arrays   *
*  Rev, 03/23/90,  MC - Keep count of # of voltage arrays needed       *
*  Rev, 04/27/91, GNR - Add generalized CHANGES, make pgdcl direct ptr *
*  V5C, 11/15/91, GNR - Add Darwin 1, reorganize connection setup      *
*  Rev, 12/02/91, GNR - Remove sense unit allocation to d3snsa         *
*  Rev, 02/14/92, GNR - Revise value setup for D1 value                *
*  V5E, 07/12/92, GNR - Moved INHIBBLK constants here from d3nset      *
*  Rev, 07/15/92, GNR - Fix UPWITH linking bug, move code from d3genr  *
*  Rev, 08/19/92, GNR - Scale compatibility, move mntsmxmp to d3go,    *
*                       turn off pressure-sens touch if colored IA     *
*  Rev, 10/11/92, GNR - Allow G,H,N,O from source with same ngx,ngy    *
*  Rev, 11/06/92, GNR - Better error message when gconn src not found  *
*  Rev, 07/09/93, GNR - Add conntype match up for virtual senses       *
*  V6D, 01/31/94, GNR - Add DELAY, activate ot                         *
*  V7A, 05/01/94, GNR - Add DELAY to GCONN, MODULATE, and NMOD         *
*  V7B, 06/22/94, GNR - Determine max nsa with OPT=A for allocation,   *
*                       move orkgen calc here from d3allo, set mxnct   *
*  V7C, 01/19/95, GNR - Correct repnqx for d3rplt, tests for KGEN=Q    *
*  V8A, 04/15/95, GNR - Merge kchng,flags into WDWDEF, handle VJ,VT,VW *
*                       as VCELL senses, add VH_SRC for KGEN=W, delete *
*                       unused VCELL built-ins, move H,X,Y,Z,C,F,U     *
*                       stats to CELLTYPE, find modality for ENVVAL,   *
*                       better no-excit error check                    *
*  Rev, 11/28/96, GNR - Remove support for non-hybrid version          *
*  Rev, 02/15/97, GNR - Allow modulation from input array              *
*  Rev, 04/16/97, GNR - Indiv. arm color                               *
*  Rev, 09/28/97, GNR - Independent dynamic allocations per conntype   *
*  Rev, 02/04/98, GNR - Default nsa, check FDM size vs nsa, not nc     *
*  Rev, 07/23/00, GNR - Allow xoff, yoff with rules J,X                *
*  Rev, 11/24/00, GNR - Separate RADIUS parameter                      *
*  V8B, 12/10/00, GNR - Move to new memory management routines         *
*  V8C, 02/27/03, GNR - Cell responses in millivolts, add conductances *
*  V8D, 02/13/07, GNR - Add BBD support, move effector setup to d3schk *
*  Rev, 11/02/07, GNR - Remove et setting from ot                      *
*  ==>, 12/29/07, GNR - Last mod before committing to svn repository   *
*  Rev, 03/15/08, GNR - Move way scale warning here from wayset, now   *
*                       that the final value of kam is known           *
*  Rev, 04/21/08, GNR - Add ssck (sum sign check) mechanism            *
*  Rev, 06/04/08, GNR - Add SUBARBOR options L and 3, J,X nux,nuy dflts*
*  Rev, 12/23/08, GNR - Restore original KGNXO nux,nuy = 0 defaults    *
*  V8E, 05/13/09, GNR - Reorganize MODCOM-MODBY-MODVAL, add MOPUNQ bit *
*  V8F, 02/14/10, GNR - Remove conditionals on TV, PLATFORM support    *
*  Rev, 04/21/10, GNR - Add PREPROC inputs                             *
*  Rev, 06/16/10, GNR - Break up cnopt, clear KGNSA in kgen, not cnopt *
*  V8H, 02/28/11, GNR - Add KAMCG option and kamchk() routine          *
*  Rev, 04/09/12, GNR - Minimum mdefer in MODVAL, IBOPFS must agree    *
*  Rev, 05/09/12, GNR - Add AUTOSCALE OPT=T, remove self-update warning*
*  Rev, 05/26/12, GNR - Set to store raw specific affsums as si64      *
***********************************************************************/

#define NEWARB          /* Use new EFFARB mechanism */
#define D1TYPE  struct D1BLK
#define TVTYPE  struct TVDEF
#define PPTYPE  struct PREPROC

#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "d3global.h"
#include "jntdef.h"
#include "armdef.h"
#include "wdwdef.h"
#ifdef D1
#include "d1def.h"
#endif
#ifdef BBDS
#include "bbd.h"
#endif
#include "tvdef.h"
#include "swap.h"
#include "rocks.h"
#include "rkxtra.h"
#include "rkarith.h"

/*=====================================================================*
*                               kamchk                                 *
*  This function handles any checking of KAM bits and setting of the   *
*  option combination bits in kam that needs to be done both at d3tchk *
*  time (used in d3allo) and again at d3news time (used in d3go).      *
*=====================================================================*/

void kamchk(struct CONNTYPE *ix) {

   ix->Cn.kam &= ~(KAMVNC|KAMTNU|KAMVNU);
   if (!(ix->Cn.kam & KAMCG) && ix->Cn.kam & (KAMVM|KAMUE))
                                       ix->Cn.kam |= KAMVNC;
   if (!(ix->Cn.kam & KAMUE)) {
      if (ix->Cn.kam & (KAMVM|KAMCG)) ix->Cn.kam |= KAMVNU;
      if (ix->Cn.kam & KAMTS)         ix->Cn.kam |= KAMTNU;
      }

   } /* End kamchk() */


/*=====================================================================*
*                               itpchk                                 *
*  This function handles matching of cameras to IMGTPAD sensors.  It   *
*  marks the image source as used (TV_ON), copies relevant camera      *
*  parameters to the touch pad block, and fills in default sclx,scly   *
*  if not already entered.  This code is placed here, rather than in   *
*  d3schk, so it will not be invoked unless the IMGTPAD is actually    *
*  used as input to some connection type.                              *
*=====================================================================*/

static void itpchk(struct VCELL *pvc, char *errmsg) {
   struct IMGTPAD *pitp = (struct IMGTPAD *)pvc;
   struct VCELL *qvc;
   /* Find the camera, mark it TV_ON, copy tvx,tvy */
   struct TVDEF *cam = findtv(pitp->hcamnm, errmsg);
   if (cam) {
      cam->utv.tvsflgs |= TV_ON|TV_ONDIR;
      pvc->vcxc.tvmode = cam->utv.tvkcol;
      getcshm(&pvc->vcxc, "image touch pad");
      pitp->pcam = cam;
      pitp->tvx = cam->utv.tvx;
      pitp->tvy = cam->utv.tvy;
      if (pitp->sclx == 0.0) pitp->sclx = (float)pitp->tvx;
      if (pitp->scly == 0.0) pitp->scly = (float)pitp->tvy;
      pitp->usca = pitp->scla * DEGS_TO_RADS;
      }
      /* Find the sensory input, mark it VKUSED */
   pvc->pvcsrc = qvc = findvc(pvc->vcname,
      pitp->itop & ITOP_USR ? USRSRC : 0, pvc->hvname, errmsg);
   if (qvc) qvc->vcflags |= VKUSED;
   } /* End itpchk() */


/*=====================================================================*
*                               modchk                                 *
*  This function handles common aspects of source checking and creates *
*  needed MODVAL blocks for normal and noise modulation.               *
*  For specific connections, many of these checks are in g2conn.       *
*  Arguments are:                                                      *
*     im          Ptr to MODBY block with the parameters               *
*     errmsg      Text for body of error message identifying target    *
*  Function returns pointer to MODVAL structure, null if not created.  *
*=====================================================================*/

static struct MODVAL *modchk(struct MODBY *im, char *errmsg) {

   struct MODVAL   *pmv,**ppmv;
   struct CELLTYPE *jl;
   struct VCELL    *pvc;
   void            *pms = NULL;     /* Ptr to modulation source */
   long lmdnel;                     /* Number entries in sums */
   int isndx = im->msrcnm.hbcnm;    /* Index if source is virtual */
   int istyp = (int)im->msrctyp;    /* Source type code */
   int mincr = 1;                   /* Source increment */

   /* All sources can be excitatory */
   im->mssck = SJSGN_POS;

/* Handle case of repertoire source separately because some
*  code after the istyp switch is different.  */

   if (istyp == REPSRC) {
      if ((jl = findln(&im->msrcnm)) == NULL) {
         cryout(RK_E1, "0***", RK_LN2+4, fmtrlnm(&im->msrcnm),
            LXRLNM, " NOT FOUND", RK_CCL, errmsg, RK_CCL, NULL);
         return NULL;         /* ERROR EXIT */
         }

      lmdnel = jl->nelt;
      pms = jl;

      /* If using new mV scale, source can also be inhibitory */
      im->mssck = RP0->rssck;

      /* If delay is requested, propagate back to source */
      if (im->Mc.mdelay > 0) {
         int mxmd1 = im->Mc.mdelay + 1;
         jl->mxsdelay1 = max((int)jl->mxsdelay1, mxmd1);
         }

      /* Real inputs--chain anchored at pmrv1 in source cell block */
      ppmv = &jl->pmrv1;
      } /* End repertoire source */

/* Source is a virtual cell type */

   else {
      switch (istyp) {

      case IA_SRC:            /* Input from the input array */
         RP->CP.runflags &= ~(RP_NOINPT);
         /* Set the color mode of the input array */
         im->Mc.mvxc.tvmode = (byte)(
            RP->kcolor == ENV_MONO ? Col_GS : Col_C8);
         /* Take care of looking through a window */
         if (isndx > 0) {
            struct WDWDEF *pw = pms = RP0->ppwdw[isndx - 1];
            lmdnel = (long)(pw->wht*pw->wwd); }
         else
            lmdnel = RP->nsx*RP->nsy;
         break;

      case VALSRC:            /* Input from a value */
         /* Use unsigned compare to give error on isndx < 0 */
         if ((unsigned)isndx > (unsigned)RP->nvblk) {
            cryout(RK_E1, "0***SRCTYP > NUM OF VALUES OR NOT "
               "A NUMBER", RK_LN2, errmsg, RK_CCL, NULL);
            return NULL; }
         isndx = max(isndx,1);
         lmdnel = 1;
         mincr = 4;
         break;

      case TV_SRC: {          /* Input from a camera */
         struct TVDEF *cam = findtv(isndx, errmsg);
         if (!cam) return NULL;
         cam->utv.tvsflgs |= TV_ON|TV_ONDIR;
         /* Copy the color mode of the camera */
         im->Mc.mvxc.tvmode = cam->utv.tvkcol;
         lmdnel = cam->utv.tvx*cam->utv.tvy;
         mincr = max(cam->utv.tvkcol, 1);
         pms = cam;
         break;
         } /* End cam local scope */

      case PP_SRC: {          /* Input from an image preprocessor */
         struct PREPROC *pip = findpp(isndx, errmsg);
         if (!pip) return NULL;
         pip->ipsflgs |= PP_ON;
         pip->pipcam->utv.tvsflgs |= TV_ON|TV_ONPP;
         lmdnel = (long)pip->upr.nipx * (long)pip->upr.nipy;
         mincr = max(pip->upr.ipkcol, 1);
         pms = pip;
         break;
         } /* End pip local scope */

   #ifdef D1
      case D1_SRC: {          /* Input from Darwin 1 */
         struct D1BLK *pd1;
         int jmt;
         /* Use unsigned compare to give error on isndx < 0 */
         if ((unsigned)isndx > (unsigned)RP->nd1s) {
            cryout(RK_E1, "0***SRCTYP > NUM OF D1 ARRAYS OR NOT ",
               "A NUMBER", RK_LN2, errmsg, RK_CCL, NULL);
            return NULL; }
         for (pd1=RP->pd1b1,jmt=1; jmt<isndx; jmt++)
            pd1=pd1->pnd1b;
         pd1->kd1f |= KD1_ALLO;
         lmdnel = pd1->nd1r*pd1->nepr;
         pms = pd1;
         break;
         } /* End pd1 local scope */
   #endif

      case VT_SRC:            /* Input from a touch sense */
         /* At present, VT can have a color.  This code should
         *  be removed when VT is revised to select color at
         *  sense level rather than at conntype level */
         im->Mc.mvxc.tvmode = (byte)(
            RP->kcolor == ENV_MONO ? Col_GS : Col_C8);
         /* Fall through to other cases ... */
         isndx = max(isndx,1);
      case ITPSRC:            /* Input from image touch */
   #ifdef BBDS
      case USRSRC:            /* Input from a user source */
   #endif
         pvc = findvc(im->msrcnm.rnm, istyp, isndx, errmsg);
         if (!pvc) return NULL;
         if (istyp == ITPSRC) itpchk(pvc, errmsg);
         pvc->vcflags |= VKUSED;    /* Indicate sense is used */
         lmdnel = pvc->ncells;
         if (pvc->vcflags & VKR4) {
            im->Mc.mvxc.tvmode = Col_R4, mincr = ESIZE;
            im->mssck = (SJSGN_POS|SJSGN_NEG); }
         else
            im->Mc.mvxc.tvmode = Col_GS, mincr = BSIZE;
         pms = pvc;
         break;

      /* All other cases--illegal.  Modulation by hand vision or
      *  kinesthetic reps is probably not useful, since integration
      *  over their cells would yield an approximately constant
      *  modulating factor.  */
      default:
         cryout(RK_E1, "0***UNALLOWED MODULATION INPUT SOURCE",
            RK_LN2, errmsg, RK_CCL, NULL);
         return NULL;
         }   /* End of switch */

   /* If delay is requested, give error message */
   if (im->Mc.mdelay > 0)
      cryout(RK_E1, "0***DELAY REQUIRES REGION SOURCE", RK_LN2,
         errmsg, RK_CCL, NULL);

   /* Virtual inputs--chain anchored at pmvv1 in RP */
   ppmv = &RP->pmvv1;
   } /* End else virtual source */

/* Take care of extracting the specified color. This is done after
*  the above switch in order to generate an error message when a
*  color is requested for a black-and-white input.  */

   getcshm(&im->Mc.mvxc, errmsg+5);

/* Search for a pre-existing MODVAL block with same parameters--
*  if found, return its address.  It is not necessary to check the
*  tvmode, because one input could not have two different modes.
*  We do not do a memcmp on the Mdc vs. Mc structs because only
*  some of the option codes need to agree.
*  Rev, 04/09/12, GNR - We now store min(mdefer) in MODVAL, do not
*  require identity as d3go can start evaluating at min defer.  */

   if (!(im->Mc.mopt & MOPUNQ)) for ( ; pmv=*ppmv; ppmv=&pmv->pmdv) {
      if (pmv->umds.pgsrc != pms) continue;
      if (pmv->Mdc.mvxc.tvcsel != im->Mc.mvxc.tvcsel) continue;
      if (pmv->Mdc.mt != im->Mc.mt) continue;
      if (pmv->Mdc.mtlo != im->Mc.mtlo) continue;
      if (pmv->Mdc.mjrev != im->Mc.mjrev) continue;
      if (pmv->Mdc.mdelay != im->Mc.mdelay) continue;
      if ((pmv->Mdc.mopt ^ im->Mc.mopt) & (MOPUNQ|IBOPKR|IBOPFS))
         continue;
      if ((int)pmv->mdsrcndx != isndx) continue;
      if ((int)pmv->mdsrctyp != istyp) continue;
      return pmv;
      }

/* Create and initialize new MODVAL block.  */

   *ppmv = pmv = (struct MODVAL *)
      allocpcv(Static, 1, IXstr_MODVAL, "MODVAL structure");
   pmv->umds.pgsrc = pms;
   pmv->mdnel      = lmdnel;
   pmv->Mdc        = im->Mc;
   pmv->mdincr     = (byte)mincr;
   pmv->mdsrcndx   = (byte)isndx;
   pmv->mdsrctyp   = (byte)istyp;

   return pmv;
   } /* End modchk() */


/*=====================================================================*
*                               d3tchk                                 *
*=====================================================================*/

void d3tchk(void) {

/* Local Declarations */

   struct REPBLOCK *ir, *jr;
   struct CELLTYPE *il, *jl;
   struct CONNTYPE *ix;
   struct INHIBBLK *ib;
   struct ARMDEF   *pa;
   struct JNTDEF   *pj;
   struct WDWDEF   *pw;
   struct VCELL    *pvc,**ppvc;
   struct MODBY    *im;
   struct VBDEF    *ivb;
   struct TVDEF    *cam;
   struct PREPROC  *pip;
#ifdef D1
   struct D1BLK    *pd1,**pld1;
#endif

   long lnc;                  /* Copy of nc for detecting changes */
   long nsrep;                /* Number of subarbor repeats */
   long maxmpx, maxnqx;
   long repnqx, repnqy;
   long repnqg;
   ui32 tops;                 /* Temp for option bits */
   int  ivc;                  /* Virtual cell numeric id */
   int  jarms, jjnts, jwdws;  /* Arm, joint, window counters */
   int  jsrc;                 /* Source type code */
   int  kaffck;               /* Afference type check bits */
#define HAS_EHQ  1            /* Cell type has type E,H,Q conns */
#define HAS_SV   2            /* Cell type has type S,V conns */
   int nj, nje, njg;          /* Numbers of joints */
   short lseqn;               /* Layer sequence number */
   short xseqn;               /* Conntype sequence number */

   /* Work spaces for potential error and warning messages */
   char errmsg[48+2*LSNAME],wrnmsg[48+2*LSNAME];

   /* Table of value source items used for error messages */
   static char *BVIMsg[] = { "arm", "window",
#ifdef D1
      "D1 array", "D1 rep", "D1 V2 input",
#endif
      };

   /* Values for kgfl as a function of jsrc */
   static byte kgfl0[NRST+1] = {
                0 /* REPSRC */,  KGNXR|KGNIA /* IA_SRC */,
      KGNXR|KGNVG /* VALSRC */,  KGNXR|KGNVG /* VJ_SRC */,
      KGNXR|KGNVG /* VW_SRC */,  KGNXR|KGNVG /* VT_SRC */,
      KGNXR|KGNTV /* TV_SRC */,  KGNXR|KGNTV /* PP_SRC */,
      KGNXR|KGNVG /* D1_SRC */,  KGNXR       /* VH_SRC */,
      KGNXR|KGNVG /* ITPSRC */,  KGNXR|KGNVG /* USRSRC */ };


/* Be sure there is at least one active repertoire */

   if (!RP->tree)
      cryout(RK_E1, "0***ALL REGIONS/REPERTOIRES ARE EMPTY.",
         RK_LN2, NULL);

/*---------------------------------------------------------------------*
*     Loop over all repertoires--                                      *
*        Be sure there is at least one active cell type                *
*        Acquire information needed to set default nqx                 *
*        Locate source celltypes for modulation and noise modulation   *
*        Locate all targets and fill in target pointers                *
*        Determine final values of nqx and nqy (small print boxsize)   *
*---------------------------------------------------------------------*/

   lseqn = xseqn = 0;         /* Init layer and conntype sequences */

/* The following may be a zero-trip loop: if it is, then a message
*     will have already been written out above.  */

   for (ir=RP->tree; ir; ir=ir->pnrp) {

      /* Error if no cell types in this repertoire */
      if (!ir->nlyr) {
         cryout(RK_E1, "0***NO CELL TYPES IN ", RK_LN2, ir->sname,
            LSNAME, " REGION/REPERTOIRE.", RK_CCL, NULL);
         continue; }

/* Loop over all cell types in this repertoire */

      maxnqx = 0;

      for (il=ir->play1; il; il=il->play) {

         /* Hold original kctp for d3chng */
         il->urkctp = il->Ct.kctp;

         /* If rspmeth was not explicitly set (by CELLTYPE, AEIF,
         *  or IZHI card), but a spike value was entered, set rspmeth
         *  to RF_SPIKE.  If a stanh value was entered, set rspmeth
         *  to RF_TANH.  Next in line, use value from MASTER CELLTYPE.
         *  If none of the above, set it to RF_KNEE.  */
         if (!(il->ctwk.tree.krmop & KRM_RFENT)) {
            if (il->ctwk.tree.krmop & KRM_THENT)
               il->Ct.rspmeth = RF_TANH;
            else if (il->ctwk.tree.krmop & KRM_SPENT)
               il->Ct.rspmeth = RF_SPIKE;
            else if (il->Ct.rspmeth == RF_NEVER_SET)
               il->Ct.rspmeth = RF_KNEE;
            }

         /* Set bits indicating stats matrices must be allocated */
         if (il->Ct.kctp & KRPXR)               RP->kxr |= XSTATS;
         if (il->Ct.kctp & KRPYM)               RP->kxr |= YSTATS;
         if (il->Ct.kctp & KRPHR)               RP->kxr |= HSTATS;
         if (il->Ct.kctp & KRPCL)               RP->kxr |= CSTATS;
         if (il->Ct.kctp & (KRPGP|KRPMG))       RP->kxr |= GSTATS;
         if (il->Ct.kctp & KRPZC)               RP->kxr |= ZSTATS;
         if (il->Ct.ctopt & OPTCFS)             RP->kxr |= ISOPTC;

         /* Pick up information needed for assignment of default nqx */
         maxnqx = max(maxnqx,il->nel);

         /* Find largest nct of any cell type, does malloc in d3go */
         RP->mxnct = max(RP->mxnct,il->nct);

         /* Set stride and total si,phi length */
         il->lsp = LSmem + il->phshft;
         il->lspt = il->nelt * il->lsp;

         /* Set autoscale ngtht if it was defaulted */
         if (il->pauts && il->pauts->ngtht < 0)
            il->pauts->ngtht = il->nelt/10;

         /* Set default siet,siscl according to phase option */
         {  int gotspcf = il->pctpd && il->pctpd->pselfpcf;
            if (il->Dc.siet == -1)
               il->Dc.siet = gotspcf ? LFIXP(12.8,FBwk) : 0;
            if (il->Dc.siscl == -1)
               il->Dc.siscl = gotspcf ? JFIXP(5.657,FBsc) : 0;
            } /* End gotspcf local scope */

         /* Clear flags used to test conntype prerequisites */
         kaffck = 0;

         /* il->kautu is used (anytime after d3tchk) to test autoscale
         *  options without need to check il->pauts != NULL, although
         *  it may be cleared by d3news or d3go if CHANGE card sets
         *  ngtht == 0.  Hold off setting il->oautsc & RP0->nautsc
         *  until now to avoid error if user enters > 1 AUTOSCALE card
         *  for this cell type, but allocate even if ngtht == 0 in
         *  case ngtht is changed later.  Give warning if autoscale
         *  used with Gerstner or Izhikevich RF.  */
         if (il->pauts) {
            struct AUTSCL *paut = il->pauts;
            if (paut->ngtht > 0) il->kautu = paut->kaut;
            il->oautsc = RP0->nautsc++;
            if (il->Ct.rspmeth >= RF_BREG) cryout(RK_P1,
               "0-->WARNING:  Autoscale with AEIF or Izhikevich "
               "RF will give incorrect adaptation for ", RK_LN2,
               fmtlrlnm(il), LXRLNM, ".", 1, NULL);
            }

         /* Error if attempt to plot a conntype that doesn't exist */
         if ((il->Ct.kctp & (KRPNP|KRPTP|CTPLV|KRPDR|KRPPJ)) &&
               (il->jijpl > (short)il->nct))
            cryout(RK_E1, "0***PLOTCT EXCEEDS CONNECTION TYPES TO ",
               RK_LN2, fmturlnm(il), LXRLNM, ".", 1, NULL);

/*---------------------------------------------------------------------*
*  Locate source cells for MODULATE and NMOD options                   *
*---------------------------------------------------------------------*/

         /* Splice any NMOD block into the beginning of the chain */
         if (il->ctf & CTFMN) {
            il->ctwk.tree.pnmby->pmby = il->pmby1;
            il->pmby1 = il->ctwk.tree.pnmby;
            }

         for (im=il->pmby1; im; im=im->pmby) {
            struct MODVAL *pmv;
            /* Prepare identification string for possible errmsgs */
            int mnmod = (im->mct == 0);
            sconvrt(errmsg, "(' FOR ',RA6,'MODULATION OF ',J0A" qLXN
               ",H.)", &mnmod, "NOISE ", fmturlnm(il), NULL);
            /* Skip rest of tests if modchk gives error return */
            if (!(pmv = im->pmvb = modchk(im, errmsg))) continue;
            if (im->mct > 0)
               kaffck |= (im->Mc.mopt & IBOPTS) ? HAS_SV : HAS_EHQ;
            if (im->MDCY.omega) pmv->Mdc.mdefer = 0;
            else if (im->Mc.mdefer < pmv->Mdc.mdefer)
               pmv->Mdc.mdefer = im->Mc.mdefer;
            if (il == (struct CELLTYPE *)pmv->umds.pgsrc)
               im->Mc.modself = TRUE;
            }


/*---------------------------------------------------------------------*
* Initialization related to 'UPWITH' option                            *
*---------------------------------------------------------------------*/

         /* This code constructs a threading of the CELLTYPE list in
         *  the pusrc field that links all CELLTYPEs that must be
         *  updated together.  Code revised and merged with code from
         *  d3genr, 07/15/92, GNR, in order correctly to handle data
         *  input in which related UPWITH fields do not all point to
         *  the last CELLTYPE in the chain.  In addition, the sequence
         *  number of the CELLTYPE at the head of each chain is stored
         *  in the upseq field of each CELLTYPE in the chain.  */
         if (il->ctwk.tree.usrcnm.hbcnm) {
            struct CELLTYPE *ul = findln(&il->ctwk.tree.usrcnm);
            if (!ul) {
               cryout(RK_E1, "0***SRCID '", RK_LN2,
                  il->ctwk.tree.usrcnm.rnm, LSNAME,
                  "' AND TYPE '", RK_CCL, il->ctwk.tree.usrcnm.lnm,
                  LSNAME, "' NOT FOUND FOR DEFERRED UPDATE OF ",
                  RK_CCL, fmturlnm(il), LXRLNM, ".", 1, NULL);
               }
            else if (ul->ctf & CTFULCHK)
               cryout(RK_E1, "0***INVALID BACKWARD UPWITH OF ", RK_LN2,
                  fmturlnm(il), LXRLNM, ".", 1, NULL);
            else {
               il->ctf |= CTFDU;
               /* Track to end of any existing chain */
               while (ul->pusrc) ul=ul->pusrc;
               ul->pusrc = il;
               }
            } /* End if UPWITH */
         /* No UPWITH, il must be updated alone or is a list head--
         *  store its sequence number in all dependent CELLTYPE's.  */
         else
            for (jl=il; jl; jl=jl->pusrc) jl->upseq = lseqn;

         /* Set layer sequence for dprnt and drive plot */
         il->seqvmo = lseqn++;

/*---------------------------------------------------------------------*
*  If there are any connection types, loop over them                   *
*---------------------------------------------------------------------*/

         for (ix=il->pct1; ix; ix=ix->pct) {

            xseqn++;          /* Count conntypes globally */

            /* Prepare identification strings for possible error
            *  and warning messages.  This wastes a little time
            *  if no errors occur, but shortens the code.  */
            sconvrt(errmsg, "(14H FOR CONNTYPE J1UH4,3HTO J0A" qLXN
               ",H.)", &ix->ict, fmturlnm(il), NULL);
            sconvrt(wrnmsg, "(14H for conntype J1UH4,3Hto J0A" qLXN
               ",H.)", &ix->ict, fmtlrlnm(il), NULL);

            /* All sources can be excitatory */
            ix->cssck = SJSGN_POS;

            /* Implement COMPAT=K.  (Modify cnopt after possible
            *  settings by CONNTYPE, PARAMS, or THRESH cards)  */
            if (RP->compat & OLDKNEE) ix->Cn.cnopt |= NOPKR;

            /* Masks related to nbc */
            ix->cmsk = (~0L)<<(BITSPERUI32-ix->Cn.nbc);
            ix->bigc16 = SRA(ix->cmsk,15) & 0x0000ffffL;
            ix->cijlen = BytesInXBits(ix->Cn.nbc);

            /* Mark cell types that have voltage-dependent conntypes.
            *  If OLDRANGE and vdha is not zero, activate cosh scaling
            *  curve, else if vdha is zero, set it to the default 0.5
            *  (S27) = 64 (S20).  Record in celltype for d3pafs(). */
            if (ix->Cn.cnopt & NOPVA) {
               if (RP->compat & OLDRANGE) {
                  if (ix->Cn.vdha) ix->Cn.cnopt |= NOPVC;
                  else             ix->Cn.vdha = LFIXP(0.5, 27); }
               if (ix->Cn.cnopt & NOPVP) {
                  ix->Cn.cnopt &= ~NOPVD;
                  il->ctf |= CTFVP; }
               if (ix->Cn.cnopt & NOPVD)
                  il->ctf |= CTFVD;
               }

            /* Mark kaffck according to mode d'employ */
            kaffck |=
               (ix->Cn.cnopt & (NOPVD|NOPSH)) ? HAS_SV : HAS_EHQ;

/* Locate source cells for this connection type.
*
*  Setting of srcngx, etc. moved here from d3allo, 11/15/91, GNR,
*     to facilitate generic defaults and error checks.  Also set
*     lijshft according to whether source cells have phase.
*  N.B.  Any consistency checks that can be done without a source
*     pointer, and anything that prepares output printed by d3echo,
*     should go BEFORE this switch.  Any code that would access
*     invalid memory if ix->psrc is not set should go AFTER this
*     switch.  If an invalid source (e.g. arm) index was specified,
*     cnsrctyp was set to BADSRC in g2conn and nothing happens here.
*/
            ivc = (int)ix->srcnm.hbcnm;
            switch (jsrc = ix->cnsrctyp) {   /* Assignment intended */

               /* Not recognized by getsnsid() as a sense name.
               *  Look for it now as a repertoire/celltype.  */
case REPSRC:   if ((jl = findln(&ix->srcnm)) == NULL) {
                  cryout(RK_E1,
                     "0***", RK_LN2+4, fmtrlnm(&ix->srcnm), LXRLNM,
                     " NOT FOUND", RK_CCL, errmsg, RK_CCL, NULL);
                  jsrc = BADSRC;
                  break;
                  }
               ix->psrc = (void *)jl;
               ix->srcngx  = jl->pback->ngx;
               ix->srcngy  = jl->pback->ngy;
               /* If the source layer has phase, lijshft should know */
               ix->lijshft = jl->phshft;
               /* If using mV scale, source can also be inhibitory */
               ix->cssck   = RP0->rssck;
               break;

case IA_SRC:   RP->CP.runflags &= ~(RP_NOINPT);
               /* Establish pointer to window data for scan and save
               *  size of full IA or window for bounds checking */
               if (ix->kgen & KGNST) {
                  ix->psrc = pw = RP0->ppwdw[ix->wdwi];
                  ix->iawxsz = (unsigned long)pw->wwd;
                  ix->iawysz = (unsigned long)pw->wht;
                  }
               else {
                  ix->iawxsz = (unsigned long)RP->nsx;
                  ix->iawysz = (unsigned long)RP->nsy;
                  }
               /* Take care of extracting the specified color */
               ix->cnxc.tvmode = (byte)(
                  RP->kcolor == ENV_MONO ? Col_GS : Col_C8);
               getcshm(&ix->cnxc, errmsg+4);
               /* Caution:  Code accessing windows becomes problematic
               *  if the planned window dilation option is implemented.
               *  srcngx,srcngy should in any event refer to the size
               *  of the full window--they are used e.g. to set lijlen.
               *  Cij plots and neuroanatomy plots will call d3woff()
               *  and move map to area seen by window in current cycle.
               *  Motor maps, however, will consider initial size of
               *  window as range of map and map Cij gradient to it.
               */
               ix->srcngx = RP->nsx;
               ix->srcngy = RP->nsy;
               break;

case VALSRC:   ix->srcngx = RP->nvblk;
               ix->srcngy = 1;
               break;

               /* Locate the requested camera--Assignment intended */
case TV_SRC:   if (cam = findtv(ivc, errmsg)) {
                  cam->utv.tvsflgs |= TV_ON|TV_ONDIR;
                  ix->psrc = cam;
                  ix->srcngx = cam->utv.tvx;
                  ix->srcngy = cam->utv.tvy;
                  /* Take care of extracting the specified color */
                  ix->cnxc.tvmode = cam->utv.tvkcol;
                  getcshm(&ix->cnxc, errmsg+4);
                  }
               else
                  jsrc = BADSRC;
               break;

               /* Locate preprocessor--Assignment intended */
case PP_SRC:   if (pip = findpp(ivc, errmsg)) {
                  pip->ipsflgs |= PP_ON;
                  pip->pipcam->utv.tvsflgs |= TV_ON|TV_ONPP;
                  ix->psrc = pip;
                  ix->srcngx = pip->upr.nipx;
                  ix->srcngy = pip->nytot;
                  /* Take care of extracting the specified color */
                  ix->cnxc.tvmode = pip->upr.ipkcol;
                  getcshm(&ix->cnxc, errmsg+4);
                  }
               else
                  jsrc = BADSRC;
               break;

#ifdef D1
               /* (Code in g2conn() already checked that hbcnm does
               *  not exceed the number of D1 blocks available.)  */
case D1_SRC:   for (pd1=RP->pd1b1; pd1 && --ivc > 0; pd1=pd1->pnd1b) ;
               pd1->kd1f |= KD1_ALLO;
               ix->psrc = pd1;
               ix->srcngx = pd1->nepr;
               ix->srcngy = pd1->nd1r;
               break;
#endif

case VH_SRC:   RP->CP.runflags &= ~(RP_NOINPT);
               /* Locate the requested arm */
               ix->psrc = RP0->pparm[ix->armi];
               ix->srcngx = RP->nsx;
               ix->srcngy = RP->nsy;
               break;

case VT_SRC:
               /* At present, VT can have a color.  This code should
               *  be removed when VT is revised to select color at
               *  sense level rather than at conntype level */
               ix->cnxc.tvmode = (byte)(
                  RP->kcolor == ENV_MONO ? Col_GS : Col_C8);
               getcshm(&ix->cnxc, errmsg+4);
               /* Fall through to other cases ... */
case VJ_SRC:
case VW_SRC:   ivc = max(ivc,1);
               /* Deliberate drop through and assignment intended */
case ITPSRC:
#ifdef BBDS
case USRSRC:
#endif
               if (pvc = findvc(ix->srcnm.rnm, jsrc, ivc, errmsg)) {
                  pvc->vcflags |= VKUSED; /* Indicate sense is used */
                  if (jsrc == ITPSRC) itpchk(pvc, errmsg);
                  ix->psrc = (void *)pvc;
                  ix->srcngx = pvc->nvx;
                  ix->srcngy = pvc->nvytot;
                  if (jsrc != VT_SRC)
                     ix->cnxc.tvmode = pvc->vcxc.tvmode;
                  if (pvc->vcflags & VKR4)
                     ix->cssck = (SJSGN_POS|SJSGN_NEG);
                  }
               else
                  jsrc = BADSRC;
               break;

               } /* End jsrc switch */

/* Omit all remaining code in CONNTYPE loop if source not found.
*  (This is mainly to avoid invalid memory reference via psrc.)  */

            if (jsrc == BADSRC) continue;

/* Set values of srcnel, srcnelt based on the above.
*  Turn off self-avoidance if input not from same layer.
*  Also, turn on the generic input type flags in kgfl
*  (moved here from d3go, 01/12/97).  */

            ix->kgfl = kgfl0[jsrc];
            if (jsrc == REPSRC) {
               ix->srcnel  = ((struct CELLTYPE *)(ix->psrc))->nel;
               ix->srcnelt = ((struct CELLTYPE *)(ix->psrc))->nelt;
               if (il != jl) ix->kgen &= ~KGNSA;
               }
            else {
               ix->srcnel = 1;
               ix->srcnelt = ix->srcngx * ix->srcngy;
               ix->kgen &= ~KGNSA;
               }

/* Propagate maximum delay needed from target to source rep.
*  If source is not a cell repertoire, give error message.  */

            if (ix->Cn.kdelay) {
               if (jsrc == REPSRC) {
                  ui16 mxcd1 = (ui16)ix->Cn.mxcdelay + 1;
                  jl->mxsdelay1 = max(jl->mxsdelay1,mxcd1);
                  }
               else
                  cryout(RK_E1, "0***DELAY REQUIRES REGION SOURCE",
                     RK_LN2, errmsg, RK_CCL, NULL);
               } /* End delay setup */

/* Link SFHMATCH structure.  This cannot be done in g2conn because
*  we must wait for parameters, including NOPOR bit, that are
*  traditionally entered on the PARAMS card.  */

            if (!(ix->Cn.cnopt & NOPOR))
               linksfhm(&ix->cnwk.tree.sfhm, SFHMCONN);

/* Set default values for lsg (*2 for scale S1)--
*  The only uses of lsg are:
*     kgen=J and input from VG
*     kgen=<J|N|O> and input from a repertoire.
*     kgen=Y and any input (default set in cndflt2()
*  VG cases moved here from d3tree, 11/16/91, GNR */

            if ((ix->lsg <= 0) && (ix->kgen &
                  (KGNJN|KGNND|KGNOG))) {
               switch (jsrc) {

               /* Input from a normal repertoire--
               *  set lsg to pick up from one group */
               case REPSRC:
                  ix->lsg = ix->srcnel;
                  break;

               /* Input from "V" or "TV"--
               *  set lsg to pick up from just one group */
               case VALSRC:
               case TV_SRC:
               case PP_SRC:
                  ix->lsg = 1;
                  break;

               /* Input from kinesthetic-style sensors--
               *  default is size of one kinesthetic array */
               case USRSRC:
               case VJ_SRC:
               case VW_SRC:
                  ix->lsg = ix->srcngx;
                  break;

               /* Input from "VT"--
               *  default is size of one touch pad */
               case VT_SRC:
                  ix->lsg = (int)pvc->nvx*(int)pvc->nvy;
                  break;

#ifdef D1
               /* Input from "D1"--
               *  default is size of entire recognizer array */
               case D1_SRC:
                  ix->lsg = ix->srcnelt;
                  break;

#endif
                  } /* End jsrc switch */

               ix->lsg <<= 1;    /* Scale lsg to S1 */
               } /* End setting lsg default */

/* Generate default for nsa.  nsa is needed later whether or not
*  SUBARBOR exists--If it was not entered, derive it from nrx,nry
*  if they are relevant and were entered, otherwise from nc.
*  Defaulting of nsa is normal, so no message is given.  */

            lnc  = ix->nc;        /* Copy of nc before change */
            if (ix->nsa <= 0) {
               if (ix->Cn.saopt & SAOPTL) ix->nsa = NColorDims;
               else if (ix->kgen & (KGNBL|KGNDG|KGNMC) &&
                     ix->nrx > 0 && ix->nry > 0)
                  ix->nsa = ix->nrx*ix->nry;
               else ix->nsa = lnc;
               }
            nsrep = (lnc + ix->nsa - 1)/ix->nsa;

/* Generate defaults for nrx,nry if 'B', 'C', 'D', 'Q', or 'M' codes.
*  (If M used w/o B,C,D,Q, nrx,nry are still used to enter coeffs.
*  Code moved here from d3tree 03/09/93 so can use nsa if entered.  */

            if (ix->kgen & (KGNBL+KGNCF+KGNDG+KGNAN+KGNMC)) {
               long lnsa = ix->nsa;    /* Copy of nsa before change */
               /* Size of one repeating box */
               long lnsr = (ix->Cn.saopt & SAOPTL) ? nsrep : lnsa;
               if (ix->nrx <= 0) ix->nrx = sqrt((double)lnsr) + 0.5;
               if (ix->nry <= 0) ix->nry = lnsr/ix->nrx;
               if (ix->kgen & (KGNBL+KGNDG+KGNMC)) {
                  nsrep = ix->nrx*ix->nry;
                  if (!(ix->Cn.saopt & SAOPTL)) {
                     ix->nsa = nsrep;
                     if (ix->nsa != lnsa) convrt("('0-->WARNING: "
                        "nsa reset to ',J1IL8,'= nrx*nry'/4X,A120)",
                        &ix->nsa, wrnmsg, NULL);
                     nsrep = (lnc + ix->nsa - 1)/ix->nsa;
                     }
                  } /* End if B,D, or M */
               } /* End if B,C,D,Q, or M */

/* Set final value of nc as a multiple of subarbor size and
*  give a warning if this is a change from the input value.
*  (Now nce is not added up until here, after nc might change.)  */

            if (nsrep >= S16) {
               nsrep = S16 - 1;
               cryout(RK_P1,"0--->WARNING: Number of subarbors reset "
                  " to 65535", RK_LN2, wrnmsg, RK_CCL, NULL); }
            ix->nsarb = (ui16)nsrep;
            ix->nc = nsrep*ix->nsa;
            if (ix->nc != lnc) convrt("('0-->WARNING: nc reset "
               "to ',J1IL8,'= mult of nrx*nry'/4X,A120)",
               &ix->nc, wrnmsg, NULL);
            il->nce += ix->nc;   /* Accumulate total conns/element */

/* Check for invalid subarbor options.  Set DOSARB switch for easier
*  testing later, based strictly on whether nsa < nc, regardless of
*  whether SUBARBOR card was entered.  nsax is not used if SAOPTL or
*  SAOPTI, otherwise if not entered default is set to make a roughly
*  square array (divide check is not possible, nsa is > 0).  Update
*  memory alloc needed in d3go() for averaging Cij's in different
*  subarbors after amplification.  For simpler testing, turn off
*  suboptions if not DOSARB.  This code must follow adjustments of
*  nsa,nc above.  */

            tops = ix->Cn.saopt & DOSARB;
            if ((tops != 0) != (ix->nsa < ix->nc)) {
               if (tops)   cryout(RK_P1, "0-->WARNING: SUBARBOR "
                  "option ignored, too few connections",
                  RK_LN3, "    ", RK_LN0, wrnmsg, RK_CCL, NULL);
               else        cryout(RK_P1, "0-->WARNING: SUBARBOR "
                  "activated, nc > matrix size or nsa",
                  RK_LN3, "    ", RK_LN0, wrnmsg, RK_CCL, NULL);
               ix->Cn.saopt ^= DOSARB;
               }
            if (ix->nsax <= 0) ix->nsax = sqrt((double)nsrep) + 0.5;
            if (ix->Cn.saopt & DOSARB) {
               if (ix->Cn.saopt & SAOPTA) {
                  /* Subarbor Cij must be cloned if averaged */
                  ix->Cn.saopt |= SAOPTC;
                  RP->mxsaa = max(RP->mxsaa, ix->nsa); }
               if (ix->kgen & (KGNPT|KGNHV))
                  cryout(RK_E1, "0***SUBARBOR INVALID WITH KGEN = "
                     "P OR W", RK_LN2, errmsg, RK_CCL, NULL);
               if (ix->kgen & KGNIN && !(ix->Cn.saopt & SAOPTI))
                  cryout(RK_E1, "0***SUBARBOR W/O OPT=I INVALID "
                     "WITH KGEN = I", RK_LN2, errmsg, RK_CCL, NULL);
               if (ix->Cn.saopt & SAOPTL) {
                  if (ix->Cn.saopt & SAOPTI) cryout(RK_E1,
                     "0***SUBARBOR OPTIONS I and L ARE INCOM"
                     "PATIBLE", RK_LN2, errmsg, RK_CCL, NULL);
                  if (ix->cnxc.tvcsel & EXTR3C &&
                        ix->nsa != NColorDims) cryout(RK_P1,
                     "0-->WARNING: Subarbor size with "
                     "color triad option is not 3", RK_LN3,
                     "    ", RK_LN0, wrnmsg, RK_CCL, NULL);
                  }
               }
            else
               ix->Cn.saopt = 0;

/* Error checking for matrix Cij and setting of default nks,nkt */

            if (ix->kgen & KGNMC) {
               struct FDHEADER *curpfdh;
               if (ix->ucij.m.mno > RP->nfdh)
                  cryout(RK_E1, "0***MNO > NUM MATRICES LOADED",
                     RK_LN2, errmsg, RK_CCL, NULL);
               else {
                  long lfdm;
                  curpfdh = RP->pfdh + ix->ucij.m.mno-1;
                  lfdm = curpfdh->mrx*curpfdh->mry;
                  /* We can never use more than one matrix per cell,
                  *  but we can reuse 1 matrix if have subarboring. */
                  if (ix->Cn.saopt & SAOPTL) {
                     if (nsrep > lfdm) cryout(RK_E1,
                        "0***NC/NSA > SIZE OF FD MATRICES", RK_LN2,
                        errmsg, RK_CCL, NULL);
                     }
                  else if (ix->nc > lfdm) {
                     if (ix->Cn.saopt & DOSARB) {
                        cryout(RK_P1, "0-->WARNING: nc > size of FD "
                           "matrices, OPT=C cloning forced", RK_LN3,
                           "    ", RK_LN0, wrnmsg, RK_CCL, NULL);
                        ix->Cn.saopt |= SAOPTC; }
                     else
                        cryout(RK_E1, "0***NC > SIZE OF FD MATRICES",
                           RK_LN2, errmsg, RK_CCL, NULL);
                     } /* End dealing with nc too big */
                  /* Make sure nrx,nry comply with matrix sizes */
                  if ((ix->kgen & (KGNBL+KGNDG)) &&
                        (ix->nrx != curpfdh->mrx ||
                        ix->nry > curpfdh->mry)) {
                     cryout(RK_E1, "0***NRX,NRY DO NOT COMPLY WITH "
                        "SIZE OF MATRICES READ", RK_LN2,
                        errmsg, RK_LN1, NULL);
                     } /* End dealing with mrx,mry mismatch */
                  /* Set default nkt, changed in V8A to min(nel,npat) */
                  if (ix->ucij.m.nkt <= 0)
                     ix->ucij.m.nkt = min(il->nel, curpfdh->npat);
                  if (ix->ucij.m.nks*ix->ucij.m.nkt > curpfdh->npat)
                     cryout(RK_E1, "0***NKS*NKT > NUMBER OF PATTERNS "
                        "IN FD MATRIX FILE", RK_LN2,
                        errmsg, RK_LN1, NULL);
                  } /* End else */
               } /* End tests for KGEN=M */

/* Generate default values for nux, nuy, and offsets for
*  kgen=F,S,T maps.  Adjust for box matrix size.
*  Perform range checking on nux,nuy,xoff,yoff.  */

            if (ix->kgen & (KGNST+KGNFU+KGNTP)) {
               long nuxdflt,nuydflt,nuxeff,nuyeff;
               if (ix->kgen & KGNST) {
                  nuxdflt = ix->iawxsz; nuydflt = ix->iawysz; }
               else if (jsrc == VT_SRC)
                  { nuxdflt = pvc->nvx; nuydflt = pvc->nvy; }
               else
                  { nuxdflt = ix->srcngx; nuydflt = ix->srcngy; }
               if ((ix->nc > 1) && (ix->kgen & KGNBL)) {
                  /* Note:  If KGNFU, d3g2conn does not allow
                  *  input of stride, so just multiplies by 1.  */
                  nuxdflt -= (ix->nrx - 1)*ix->stride;
                  nuydflt -= ix->nry - 1;
                  }
               if (!ix->nux) ix->nux = nuxdflt;
               if (!ix->nuy) ix->nuy = nuydflt;

               /* This test not done at card input time--zero nux,nuy
               *  are OK for KGNJN, KGNXO, KGNYO */
               if (ix->nux <= 0 || ix->nuy <= 0) {
                  cryout(RK_E1, "0***NUX or NUY BOX SIZE IS <= 0",
                     RK_LN2, errmsg, RK_CCL, NULL);
                  /* Squelch further errors from negative nux,nuy */
                  ix->nux = ix->srcngx, ix->nuy = ix->srcngy; }
               else if (ix->kgen & KGNTP) {
                  if (ix->nux % ir->ngx || ix->nuy % ir->ngy)
                     cryout(RK_P1, "0-->WARNING: Topographic mapping "
                        "is nonintegral",RK_LN2,wrnmsg,RK_CCL,NULL);
                  }

               if (ix->xoff == MINUS0) /* -0 used to request dflt */
                  ix->xoff = (nuxdflt - ix->nux)>>1;
               if (ix->yoff == MINUS0) /* -0 used to request dflt */
                  ix->yoff = ((nuydflt - ix->nuy)>>1) +
                     (jsrc == VT_SRC ? pvc->nvytot - pvc->nvy : 0);

               /* The following tests take subarboring into account but
               *  not the extent of any B, C, D arbors except insofar
               *  as B decreases default nux,nuy.  Maybe they should.  */
               nuxeff = ix->nux, nuyeff = ix->nuy;
               if (ix->Cn.saopt & (DOSARB|SAOPTL) == DOSARB) {
                  nuxeff += ix->nsax - 1;
                  nuyeff += (ix->nc - 1)/(ix->nsa*ix->nsax);
                  }
               /* It is an error if the entire box is off the source */
               if (ix->xoff >= ix->srcngx || ix->yoff >= ix->srcngy ||
                   ix->xoff + nuxeff <= 0 || ix->yoff + nuyeff <= 0)
                  cryout(RK_E1, "0***SPECIFIED SOURCE AREA HAS NO "
                     "OVERLAP WITH SOURCE", RK_LN2,
                     errmsg, RK_CCL, NULL);
               /* It is a warning if part of source is out-of-bounds */
               else if (
                  ix->xoff < 0 || ix->xoff + nuxeff > ix->srcngx ||
                  ix->yoff < 0 || ix->yoff + nuyeff > ix->srcngy)
                  cryout(RK_P1, "0-->WARNING: Specified source area is "
                     "partly outside bounds of source", RK_LN3,
                     "    ", RK_LN0, wrnmsg, RK_CCL, NULL);

               } /* End KGEN=F,S,T */

            else if (ix->kgen & KGNBV) {
               /* vcmax > vcmin is checked earlier */
               float dv = pvc->vcmax - pvc->vcmin;
               if (!ix->nux) ix->nux = ir->ngx;
               if (!ix->nuy) ix->nuy = ir->ngy;
               /* This a convenience for KGNBV, else harmless--
               *  note that KGNAJ is forced with KGNBV  */
               if (ix->cnxc.tvmode == Col_R4) {
                  ix->ul2.a.bvv1 = pvc->vcmin;
                  ix->ul2.a.bvxdv = (float)ix->nux/dv;
                  ix->ul2.a.bvydv = (float)ix->nuy/dv;
                  }
               else {
                  ix->ul2.a.bvxdv = (float)ix->nux/dS8;
                  ix->ul2.a.bvydv = (float)ix->nuy/dS8;
                  }
               /* Check offsets--ui32 test catches negatives */
               if ((ui32)ix->xoff > (ui32)pvc->nvx ||
                   (ui32)ix->yoff > (ui32)pvc->nvy ||
                   ix->xoff + ix->loff > pvc->nvx)
                  cryout(RK_E1, "0***SPECIFIED OFFSETS ARE OUTSIDE "
                     "SOURCE SENSE", RK_LN2, errmsg, RK_CCL, NULL);
               }

/* Tests on connection-generating parameters
*  (Extensively revised, 11/16/91, GNR to consolidate tests
*  previously located in d3tree, d3allo, and d3go.)  */

            /* KGEN=J,X:  Error if nux or nuy too large.  Check if
            *  initial offsets are outside the source.  This is a
            *  warning for J (the generating routine can handle it,
            *  and sometimes it is what user wants), but an error
            *  for X (generating routines cannot handle this case).
            *  Rev, 06/27/08, GNR - Add nux,nuy defaults for J,X.
            *  Rev, 12/23/08, GNR - Remove new KGNXO nux default--it
            *     snafu'd some test runs, also 0 may often be wanted.
            */

            if (ix->kgen & (KGNJN|KGNXO|KGNYO)) {
               /* This code is OK for IA, given the test below */
               long totoff = (ix->xoff + ix->yoff*ix->srcngx)*
                     ix->srcnel + ix->loff;
               if (labs(ix->nux) > ix->srcngx*ix->srcnel)
                  cryout(RK_E1, "0***NUX EXCEEDS ROW SIZE",
                     RK_LN2, errmsg, RK_CCL, NULL);
               if (labs(ix->nuy) > ix->srcngy)
                  cryout(RK_E1, "0***NUY EXCEEDS NUMBER OF ROWS"
                     " IN SOURCE", RK_LN2, errmsg, RK_CCL, NULL);
               if (ix->kgen & KGNJN) { /* Offset test for rule J */
                  if (!(ix->nux|ix->nuy)) ix->nuy = 1;
                  if (totoff < 0 ||
                        (totoff<<1) + ix->lsg >= (ix->srcnelt<<1))
                     cryout(RK_P1, "0-->WARNING: Offset < 0 or > "
                        "layer size", RK_LN2, wrnmsg, RK_CCL, NULL);
                  } /* End tests for KGEN=J */
               else {                  /* Offset test for rule X,Y */
                  if (totoff < 0 || totoff >= ix->srcnelt)
                     cryout(RK_E1, "0***OFFSET < 0 OR > LAYER SIZE",
                        RK_LN2, errmsg, RK_CCL, NULL);
                  } /* End tests for KGEN=X,Y */
               }  /* End combined X,J tests */

            /* KGEN=A or B: Test stride.  Add a gratuitous warning
            *  if input from a repertoire and stride > rowlength.  */

            if ((ix->kgen & (KGNAJ+KGNBL)) && (ix->nc > 1)) {
               if (jsrc == IA_SRC) {
                  if (ix->stride & RP->xymaskc)
                     cryout(RK_E1, "0***STRIDE EXCEEDS IA SIZE",
                        RK_LN2, errmsg, RK_CCL, NULL);
                  }
               else if (ix->stride >= ix->srcnelt)
                  cryout(RK_E1, "0***STRIDE EXCEEDS SOURCE SIZE",
                     RK_LN2, errmsg, RK_CCL, NULL);
               else if (jsrc == REPSRC &&
                     (ix->stride >= ix->srcngx*ix->srcnel))
                  cryout(RK_P1, "0-->WARNING: Stride > row size",
                     RK_LN2, wrnmsg, RK_CCL, NULL);
               } /* End tests for KGEN=A,B */

            /* KGEN=D: Test for excessively large nhy,nhx,nvy,nvx
                  increments; nrx,nry are not tested because can fit
                  diagonally.  VG and IA cases have srcnel == 1. */

            if ((ix->kgen & KGNDG) && (ix->nc > 1)) {
               long rowmax = ix->srcngx*ix->srcnel;
               if (abs(ix->nhx)>rowmax || abs(ix->nhy)>rowmax)
                  cryout(RK_E1, "0***NH{X|Y} EXCEEDS ROW SIZE", RK_LN2,
                     errmsg, RK_CCL, NULL);
               if (abs(ix->nvx)>ix->srcngy || abs(ix->nvy)>ix->srcngy)
                  cryout(RK_E1, "0***NV{Y|X} EXCEEDS SRC NGY", RK_LN2,
                     errmsg, RK_CCL, NULL);
               } /* End tests for KGEN=D */

            /* KGEN=Q: Make sure box sizes are odd numbers and outer
            *  box is larger than inner box.  */

            if ((ix->kgen & KGNAN) && (ix->nc > 1)) {
               if (!(ix->nrx&1) || !(ix->nry&1) ||
                     !(ix->nhx&1) || !(ix->nhy&1))
                  cryout(RK_E1, "0***BOX SIZES MUST BE ODD", RK_LN2,
                     errmsg, RK_CCL, NULL);
               if (ix->nhx >= ix->nrx || ix->nhy >= ix->nry)
                  cryout(RK_E1, "0***INNER BOX SIZE IS >= OUTER BOX",
                     RK_LN2, errmsg, RK_CCL, NULL);
               } /* End tests for KGEN=Q */

/* The following tests apply only if the source is REPSRC or IA_SRC */

            if (jsrc == REPSRC) {  /* Found cell layer */

               jr = jl->pback;

               /* KGEN=G,H,N,O: Test for same source,target rep shape--
               *  replaces stricter test in d3tree that source==self,
               *  10/11/92, GNR  */
               if (ix->kgen & (KGNGP+KGNHG+KGNND+KGNOG) &&
                     (ix->srcngx != ir->ngx || ix->srcngy != ir->ngy))
                  cryout(RK_E1, "0***SRC NGX,NGY NE TARGET", RK_LN2,
                     errmsg, RK_CCL, NULL);

               /* KGEN=H: Test for same source,target grid divisions */
               if (ix->kgen & KGNHG &&
                     (jr->Rp.ngridx != ir->Rp.ngridx ||
                      jr->Rp.ngridy != ir->Rp.ngridy))
                  cryout(RK_E1, "0***SRC GRID DIVS NE TARGET", RK_LN2,
                     errmsg, RK_CCL, NULL);

               /* KGEN=O: Test for valid combination of width and
               *  source size */
               if (ix->kgen & KGNOG && ix->lsg >= (jl->nelt<<1))
                  cryout(RK_E1, "0***LSG > SRC SIZE", RK_LN2,
                     errmsg, RK_CCL, NULL);

               /* KGEN=B or C: Test for allowed value of nrx,nry */
               if ((ix->kgen & (KGNBL+KGNCF+KGNAN)) && (ix->nc > 1) &&
                     (ix->nrx>jr->ngx || ix->nry>jr->ngy))
                  cryout(RK_E1, "0***B,C,Q MAP EXCEEDS SIZE OF "
                     "INPUT REP", RK_LN2, errmsg, RK_CCL, NULL);

               } /* End REPSRC checks */

            if (jsrc == IA_SRC)
               ix->iaxoff = ix->xoff << RP->ky1;
            else {
               ix->iawxsz = (unsigned long)ix->srcngx;
               ix->iawysz = (unsigned long)ix->srcngy;
               }

/* Adjust the scale actually used according to number of
*  connections.  Changed in V3L to report error if scaled
*  value overflows.  Do after any code that resets nc.
*  Rev, no date,   MC - Remove scale normalization.
*  Rev, 08/19/92, GNR - Add compat with old version. */

            if (RP->compat & OLDSCALE) {
               double dscl = (double)ix->Cn.scl*
                  sqrt(32.0/(double)ix->nc);
               if (dscl >= 2147483648.0)
                  cryout(RK_E1, "0***SCALE OVERFLOWED", RK_LN2,
                     errmsg, RK_CCL, NULL);
               else ix->Cn.scl = (long)dscl;
               }

/* Following code must follow any possible changes to kgen, kam,
*  or kdecay bits.  Put OR of all such bits in CELLTYPE block.  */

            kamchk(ix);
            if (ix->Cn.kam & KAMFS) ix->cnflgs |= CNDOFS;
            if (ix->Cn.kam & KAMCG) RP->kxr |= ISKAMC;
            il->orkgen             |= ix->kgen;
            il->orkam              |= ix->Cn.kam;
            il->ctwk.tree.orkdecay |= ix->Cn.kdecay;

/* Warning if any way scales were entered, but fewer than the number
*  appropriate for the selected amplification rule.  This code was
*  moved here from getway() to eliminate meaningless warnings when
*  the final rule was not known at getway() time.  Changes at Group
*  III time will not be detected, but then it's a bit late anyway.  */

            {  byte nnwayrd = ix->Cn.nwayrd & ~NWSCLE;
               if (nnwayrd > 0 && ix->Cn.kam & ANYAMASK &&
                     nnwayrd < (ix->Cn.kam & KAMVNU ?
                     MAX_WAYSET_CODES : MAX_WAYSET_CODES/2))
                  cryout(RK_P1,"0-->WARNING: Missing amp scale", RK_LN2,
                     ix->Cn.nwayrd & NWSCLE ? "s" : " codes", RK_CCL,
                     " set to 0", RK_CCL, wrnmsg, RK_CCL, NULL);
               } /* End nnwayrd local scope */

/* Calc number of udev calls per Lij--used to call udevskip
*  if skipping connections, also in PARn to skip locells */

            /* Calc number of udev calls for first Lij */
            ix->uplij = (ix->kgen & (KGNHV|KGNXO|KGNYO|KGNEX)) ? 0 : 1;

            /* Calc number of udev calls per subsequent Lij */
            if (ix->kgen & (KGNIN | KGNPT))
               ix->uplijss = ix->uplij;
            else if (ix->kgfl & KGNIA && ix->kgen & KGNCF)
               ix->uplijss = 1;
            else if (ix->kgen & KGNCF) ix->uplijss = 2;
            else if (ix->kgen & KGNAN) ix->uplijss = 1;
            else ix->uplijss = 0;

            /* Combine these to give number for all Lij */
            {  long lsrep = (ix->Cn.saopt & SAOPTL) ?
                  nsrep : ix->nsa;
               if (!(ix->kgen & KGNAN)) lsrep -= 1;
               ix->uplij += ix->uplijss*lsrep;
               } /* End local lsrep scope */

/* Do checks related to phasing.
*  Copy phase option bits from cnopt to phopt to facilitate AND
*     tests on multiple bits with one instruction in d3go, etc.
*  Set lpax as len of phase-dep part of 32-bit det-print data.
*  Set lpax64 as len of 64-bit part of det-print data.  (lpax and
*     lpax64 do not include decay term, can change at Group 3.)
*  Set lrax,lsax to establish lengths of summation arrays in d3go:
*  rax[pn] arrays are used for raw sums and delayed convolution
*     scratch, so require 1 si64 each for sums ignoring phase plus
*     32 si64 each if PHASJ|PHASR (synapse loop, lrax1) or if
*     PHASJ|PHASR|PHASDCNV (total allocation).
*  sax[pn] arrays are used for subarbor sums and early convolutions
*     and to hold the results going into the total cell sums, so
*     they require 1 si64 if no phase, otherwise 32 plus one more
*     with subarbors, regardless of phase assignment option.  */

            ix->phopt = (byte)(tops = ix->Cn.cnopt & NOPPHBITS);
            ix->lpax = ix->lpax64 = 1;
            ix->lsax = ix->lrax = ix->lrax1 = 1;

   /*** THREE LEVELS OF INDENTING SUPPRESSED TO END OF THIS BLOCK ***/

   if (il->phshft) {
      if (tops) {
         /* Checks moved here from g2param() because option
         *  bits now can be entered in multiple ways.  */
         if (bitcnt(&ix->phopt, sizeof(ix->phopt)) > 1)
            cryout(RK_E1, "0***MORE THAN ONE OF J,U,R,C "
               "PHASE OPTIONS.", RK_LN2, errmsg, RK_CCL, NULL);
         if (tops & PHASU && ix->Cn.ppcf) cryout(RK_E1,
            "0***USELESS CONVOLUTION WITH UNIFORM PHASE DIST.",
            RK_LN2, errmsg, RK_CCL, NULL);
         /* If cells have phase and user did not specify a
         *  phasing option for this input, force PHASJ option
         *  if input cells also have phase, otherwise PHASU
         *  (no warning needed).  PHASC,PHASU,PHASR, but not
         *  PHASJ, are valid with non-repertoire inputs.
         *  Rev, 11/03/07, GNR - Removed error when ADECAY is
         *  present--now handled as a warning in d3news in case
         *  decay is added by d3chng().
         *  N.B.  d3go() assumes one of PHAS[CJRU] is set
         *  when cells have phase.  */
         if (tops & PHASJ && ix->lijshft != 1) cryout(RK_E1,
            "0***CANNOT REQUEST SOURCE CELL PHASE (OPT=J) WITH "
            "NON-PHASED INPUTS", RK_LN2, errmsg, RK_CCL, NULL);
         }
      else {   /* No phase option specified */
         if (ix->lijshft == 1) ix->phopt |= PHASJ;
         else                  ix->phopt |= PHASU;
         }

      /* From here on, always access PHAS[JRCU] bits via ix->phopt */

      /* Type PHASU when voltage-dependent can have different
      *  values in different phase bins and so all 32 must be
      *  saved for detail statistics.  */
      if (ix->phopt & PHASU && ix->Cn.cnopt & NOPVA)
         ix->phopt |= PHASVDU;

      /* Set PHASECNV if convolution should be done early (nc < 32,
      *  PHASJ or PHASR, not voltage-dep, and no subarbors--mnax test
      *  would be too complex).  Set PHASDCNV if convolution should
      *  be deferred.  Leave both bits unset if no convolutions at
      *  all.  Tests must follow possible reset of nc above.  */
      if (ix->Cn.ppcf) ix->phopt |=
            (ix->nc < PHASE_RES && ix->phopt & (PHASJ|PHASR) &&
            !(ix->Cn.cnopt & NOPVA) && !(ix->Cn.saopt & DOSARB)) ?
         PHASECNV : PHASDCNV;

      /* Phase assignment option is now final--set lengths */
      if (ix->phopt & (PHASJ|PHASR|PHASECNV|PHASDCNV|PHASVDU))
         ix->lpax = PHASE_RES;
      if (ix->phopt & (PHASR|PHASJ|PHASDCNV))
         ix->lrax = (PHASE_RES+1);
      if (ix->phopt & (PHASR|PHASJ))
         ix->lrax1 = (PHASE_RES+1), ix->lpax64 = PHASE_RES;
      ix->lsax = PHASE_RES + ((ix->Cn.saopt & DOSARB) != 0);
      /* Pretend d3go already did an update so next cycle
      *  can always start with updated last working seed.  */
      ix->upphseed = ix->phseed;

      } /* End phasing checks */

   else {
      /* No target cell phase.  Error if phase option specified.
      *  Kill any phase print, etc.  If an amplification PAF function
      *  has been specified, inactivate it and warn.  (This code
      *  allows a PAF even if source cells have no phase but one
      *  is assigned by R,U,C, even if this is a bit silly.  */
      if (tops) cryout(RK_E1,
         "0***J,U,R,C PHASE OPTION INVALID--CELLTYPE DOES NOT "
         "HAVE PHASE.", RK_LN2, errmsg, RK_LN2, NULL);
      ix->phopt = 0;
      if (ix->Cn.pamppcf) {
         ix->Cn.pamppcf = NULL;
         cryout(RK_P1, "0-->WARNING: PAF ignored because "
            "cells have no phase", RK_LN2, wrnmsg, RK_CCL, NULL);
         }
      } /* End else (cells have no phase) */

   /*** END SUPPRESSED INDENTING ***/

            /* Store max lrax+lsax for d3nset memory allocation
            *  of CDAT.rsumaij portion for rax[pn]+sax[pn].  */
            {  ui16 lax = ix->lrax + ix->lsax;
               if (lax > RP->mxlax) RP->mxlax = lax; }

            }  /* End for loop over ix */

/*---------------------------------------------------------------------*
*  Check any INHIBBLK's.  Locate source cells.                         *
*  Make sure source repertoire has same dimensions as target.          *
*  Calculate various constants used elsewhere to shorten code.         *
*  Scale the betas, here and again in d3chng() when betas change.      *
*  Flag CELLTYPE if any blocks have IBOPT=V requiring d3inhc calc.     *
*  Propagate requested delay back to source, which is always REPSRC.   *
*---------------------------------------------------------------------*/

         for (ib=il->pib1; ib; ib=ib->pib) {
            long ll;             /* Temps for ihsf calc */
            long nrm1;           /* Number of rings - 1 */
            long twonrm1;        /* Twice nrm1 */
            int  lgsn;           /* Log of size of gcon area */

            /* Prepare identification strings for possible errmsgs */
            sconvrt(errmsg, "(' FOR GCONN TYPE ',J1IH4,3HTO J0A" qLXN
               "H.)", &ib->igt, fmturlnm(il), NULL);
            sconvrt(wrnmsg, "(' for gconn type ',J1IH4,3Hto J0A" qLXN
               "H.)", &ib->igt, fmtlrlnm(il), NULL);

            jl = findln(&ib->gsrcnm);
            if (jl == NULL) {
               cryout(RK_E1,
                  "0***", RK_LN2+4, fmtrlnm(&ib->gsrcnm), LXRLNM,
                  " NOT FOUND", RK_CCL, errmsg, RK_CCL, NULL);
               /* Fake self-inhibition to kill further error msgs */
               jl = il;
               }

            ib->pisrc = jl;
            jr = jl->pback;
            kaffck |=
               (ib->ibopt & IBOPTS) ? HAS_SV : HAS_EHQ;
            if ((ir->ngx != jr->ngx) || (ir->ngy != jr->ngy))
               cryout(RK_E1, "0***GCONN SRC,TARGET SIZES DIFFER",
                  RK_LN2, errmsg, RK_CCL, NULL);

            /* Input signs depend on COMPAT=C mode */
            ib->gssck = RP0->rssck;

            /* Propagate any delay back to source */
            if (ib->ihdelay > 0) {
               ui16 mxihd1 = (ui16)ib->ihdelay + 1;
               jl->mxsdelay1 = max(jl->mxsdelay1,mxihd1);
               }

            /* Accumulate max(nib) for d3echo */
            if (ib->nib > ir->mxnib) ir->mxnib = ib->nib;
            if (ir->mxnib > RP0->mxnib) RP0->mxnib = ir->mxnib;

/* Translation Note   9/23/88
*  The following variables were a factor of 4 larger in the assembler
*  version. Now that they will be used as indices to C arrays of type
*  long, the factor of sizeof(long) will be provided by C.  In places
*  where space is allocated using these variables a factor of
*  sizeof(long) must be introduced. */

            ib->l1n1 = nrm1 = ib->ngb*(ib->nib-1);
            twonrm1 = nrm1 + nrm1;
            ib->l1area = (twonrm1+1)*(twonrm1+1);
            ib->l1x = jr->ngx;
            ib->l1y = jr->ngy;
            ib->l1xn2 = jr->ngx + twonrm1;
            ib->l1yn2 = jr->ngy + twonrm1;
            ib->l1xy  = jr->ngx * jr->ngy;
#ifndef PAR
            /* Constants used only in serial version */
            ib->l1n1x = jr->ngx*nrm1;
            ib->l1n1xn2 = ib->l1xn2*nrm1;
            ib->l1yxn2  = jr->ngy*ib->l1xn2;
            /* Total boxsum size */
            ib->l1xn2yn2 = ib->l1yxn2 + (ib->l1n1xn2<<1);
#endif
            /* Constant needed to scale bandsums */
            ll = (ib->ibopt & IBOPTX) ? jl->nelt :
               (jl->nel * (twonrm1+1) * (twonrm1+1));
            ib->ihsf = (byte)(lgsn = bitsz(ll));

            if ((lgsn -= 16) > 0) convrt(
               "('0-->WARNING: ',J1I4,'bits of precision lost',A120)",
               &lgsn, wrnmsg, NULL);

            }  /* End ib loop */

/* Give error if ALL conntypes to this cell type are voltage-
*  dependent, shunting, or squared inhibition. */

         if (il->pcnd1) kaffck |= HAS_EHQ;
         if (kaffck == HAS_SV)
            cryout(RK_E1, "0***", RK_LN2, fmturlnm(il), LXRLNM,
               " HAS NO EXCIT/HYPERPOL/SQ CONNECTIONS NEEDED WITH"
               " VOLTAGE-DEP OR SHUNTING INHIB.", RK_CCL, NULL);

/* Set flag used to test for invalid update loops */

         il->ctf |= CTFULCHK;

/* Set flag to request sbar fast start (see d3resetn) */

         if (il->orkam & KAMFS) il->ctf |= CTFDOFS;

/* Count total number of gconn and modulatory types */

         RP0->totgcon += il->ngct;
         RP0->totmodu += il->nmct;

         }  /* End of big for loop over il */

/* End of first pass--
*  Set final values of nqx and nqy.  Unlike Darwin II,
*  Darwin III does not change size of repertoire specified
*  by user to make rectangular print boxes.  If this decision
*  is ever changed, please note that several defaults and
*  error messages in the preceding code would have to be
*  postponed until the final values of ngx, ngy, ngrp, and
*  nelt became available. */

/* If there is more than one cell per group, then nqx is the smal-
*  ler of the input value or the largest number of cells per group
*  in any one cell type.  If there is just one cell per group, then
*  then it is the input value or the default of 8.  In either case,
*  it may not exceed 'mpx' or columns per page (allowing for layer
*  names when more than one celltype in the repertoire).  */

      repnqx = ir->nqx;
      repnqy = ir->nqy;
      maxmpx = (ir->nlyr > 1) ? LNSIZE - (LSNAME+1) : LNSIZE;
      maxmpx = min(maxmpx,RP0->mpx);
      if (ir->ncpg == 1) { /* One cell per group */
         if (repnqx <= 0) repnqx = 8;
         }
      else {               /* More than one cell per group */
         if (repnqx <= 0) repnqx = maxnqx;
         else             repnqx = min(repnqx,maxnqx);
         }
      ir->nqx = repnqx = min(repnqx,maxmpx);

/* Now nqg can be calculated.  This is the total number of rows
*  needed to print one group, given that only nqx cells can go
*  in any one row and a new row is started for each cell type.
*  The contribution to nqg from each cell type is lnqy. */

      repnqg = 0;
      for (jl=ir->play1; jl; jl=jl->play) {
         jl->lnqyprev = repnqg;
         jl->lnqy = (jl->nel - 1)/repnqx + 1;
         repnqg += jl->lnqy; }
      ir->nqg = repnqg;

/* Final values of nqy can now be calculated:
*  If ncpg=1, default is 8.
*  Otherwise, default is nqg, and any user-entered value is
*     adjusted to the next higher multiple of nqg.
*  This value may exceed what is needed to print the entire
*     repertoire--d3rprt will adjust, but d3epsi will use to
*     assign graph space, so user can deliberately set it too
*     large to stretch the graph vertically.  */

      if (ir->ncpg == 1) { /* One cell per group */
         if (repnqy <= 0) repnqy = 8;
         }
      else                 /* More than one cell per group */
         repnqy = lmulup(repnqy,repnqg);
      ir->nqy = repnqy;

      } /* End of big for loop over ir */

   RP0->totnlyr = lseqn;      /* Total number of celltypes */
   RP0->totconn = xseqn;      /* Total number of conntypes */

/*---------------------------------------------------------------------*
*     Check and initialize value schemes.                              *
*     Calculate starting Y coordinate for value label and bars.        *
*---------------------------------------------------------------------*/

   for (ivb=RP->pvblk; ivb; ivb=ivb->pnxvb) {
      int kv = ivb->kval;
      int kid = ivb->vbkid;
      int ia,iw;                    /* Arm and window indices */
      int ivx;                      /* Error message selector */

      /* Perform checking specific to type of value scheme */
      switch (kid) {

      case VBK_REP:              /* Repertoire-type value */
         if ((ivb->u1.vrp.pvsrc = findln(&ivb->valnm)) == NULL)
            cryout(RK_E1, "0***", RK_LN2+4, fmtrlnm(&ivb->valnm),
               LXRLNM, " NOT FOUND FOR VALUE SCHEME.",RK_CCL, NULL);
         ivb->u1.vrp.omvbd2 = (S8 - ivb->vbase)>>1;
         break;

      case VBK_DIST:             /* Distance-type value */
         iw = ibcdin(VSBIC,ivb->valnm.lnm);
         /* Check window number and find WDWDEF block */
         if (iw > (int)RP->nwdws) { ivx = 1; goto BadValueIndex; }
         ivb->u1.vhh.mvwdw = RP0->ppwdw[iw - 1];
         /* Calculate needed constant from half-radius */
         {  float vhf = (float)ivb->vhalf;
            ivb->u1.vhh.vradm2 = ((kv==VB_DLIN || kv==VB_RLIN) ?
               -0.5/vhf : -1.0/(vhf*vhf));
            }
         /* Fall-through to case VBK_UJG is intended... */
      case VBK_UJG:              /* Universal joint or grip value */
         ia = ibcdin(VSBIC,ivb->valnm.rnm);
         /* Check arm number and find ARMDEF block */
         if (ia > (int)RP->narms) { ivx = 0; goto BadValueIndex; }
         ivb->u1.vhh.mvarm = RP0->pparm[ia - 1];
         break;

      case VBK_ENV:              /* Environmental value */
         /* Setup for ENVVAL is handled in d3grp1() because the
         *  maximum length for a modality name is (or might be)
         *  too long to fit in the valnm.rnm field.  Findmdlt can
         *  successfully be called at d3grp1() time, but findln
         *  for repertoire matching must not be called until here.
         */
         break;

#ifdef D1
      case VBK_D1:
      {  int ja;
         ia = ibcdin(VSBIC,ivb->valnm.rnm);
         if (ia > RP->nd1s) { ivx = 2; goto BadValueIndex; }
         /* Find D1BLK for this value scheme */
         for (pd1=RP->pd1b1,ja=1; ja<ia; ja++,pd1=pd1->pnd1b) ;
         iw = ibcdin(VSBIC,ivb->valnm.lnm);
         if (iw > pd1->nd1r) { ivx = 3; goto BadValueIndex; }
         if (pd1->kd1o & KD1_VER1) { ivx = 4; goto BadValueIndex; }
         pd1->kd1f |= KD1_ALLO;     /* Force allocation for this D1 */
         ivb->u1.vd1.pvsd1 = pd1;   /* Save ptr to D1BLK */
         ivb->u1.vd1.repnm1 = iw-1; /* Save repertoire offset */
         }  /* End local scope */
         break;
#endif
         } /* End kid switch */
      continue;            /* Advance to next value block */

BadValueIndex:
      cryout(RK_E1, "0***VALUE REFERS TO NONEXISTENT ", RK_LN2,
         BVIMsg[ivx], RK_CCL, NULL);
      }  /* End of loop over value blocks */

/*---------------------------------------------------------------------*
*     Checking for arms--                                              *
*        Be sure max angle is greater than min angle.                  *
*        Set default color if not entered.                             *
*        Turn off pressure-sensitive touch if input array is colored.  *
*---------------------------------------------------------------------*/

   for (pa=RP0->parm1,jarms=1; pa; jarms++,pa=pa->pnxarm) {

      nje = nj = pa->njnts;
      if (pa->jatt & ARMAU) nje++;
      njg = nje;
      if (pa->jatt & ARMGE) njg++;

      if (!strcmp(pa->acolr, "DEFAULT"))
         strncpy(pa->acolr, RP0->ecolarm, ECOLMAXCH);

      for (pj=pa->pjnt,jjnts=1; jjnts<=njg; pj++,jjnts++) {
         if (jjnts > nje) /* GRIP JOINT */ {
            if (pj->u.grip.grvi > RP->nvblk)
               cryout(RK_E1, "0***GRVI EXCEEDS NUMBER OF "
                  "VALUE SCHEMES.", RK_LN2, NULL);
            } /* End GRIP JOINT */
         else if (jjnts > nj) /* UNIVERSAL JOINT */ {
            if (pj->u.ujnt.ujvi > RP->nvblk)
               cryout(RK_E1, "0***UJVI EXCEEDS NUMBER OF "
                  "VALUE SCHEMES.", RK_LN2, NULL);
            } /* End UNIVERSAL JOINT */
         else /* REGULAR JOINT */ {
            /* Be sure maximum angle is no smaller than minimum */
            if (pj->u.jnt.jmx < pj->u.jnt.jmn) {
               convrt("(P1,'0***ARM ',J1I4,'JOINT ',J1I4,': MIN"
                  " ANGLE EXCEEDS MAX.')", &jarms, &jjnts, NULL);
               RK.iexit |= 1; }
            /* Save starting position for possible reset */
            pj->u.jnt.ja0 = pj->u.jnt.ja;
            } /* End REGULAR JOINT */

         }  /* End loop over jjnts */
      } /* End loop over jarms */

/*---------------------------------------------------------------------*
*     Checking for windows--                                           *
*        Be sure windows fit on input array.                           *
*        Set default color if not entered.                             *
*---------------------------------------------------------------------*/

   for (pw=RP0->pwdw1,jwdws=1; pw; jwdws++,pw=pw->pnxwdw) {

      /* Be sure windows fit on the input array */
      if (((long)(pw->wwd) > RP->nsx) ||
            ((long)(pw->wht) > RP->nsy)) {
         convrt("(P1,'0***WINDOW',I3,' EXCEEDS IA SIZE.')",
            &jwdws, NULL);
         RK.iexit |= 1; }
      if (!strcmp(pw->wcolr, "DEFAULT"))
         strncpy(pw->wcolr, RP0->ecolwin, ECOLMAXCH);

      /* Save starting parameters for possible resets */
      pw->wx0  = pw->wx;
      pw->wy0  = pw->wy;
      pw->wwd0 = pw->wwd;
      pw->wht0 = pw->wht;

      }  /* End loop over windows */

/*---------------------------------------------------------------------*
*     Checking for built-in and user-defined senses--                  *
*        Accumulate cumnv[bf]c = total space required for sense cells. *
*        Delete VCELL blocks for unused built-in senses (leave user-   *
*           defined senses intact, may exist for external reasons).    *
*        Give an error for blocks that are used but have no cells.     *
*        Flag ARMDEF blocks that are used for joint kinesthesia (this  *
*           info is used by envcyc, not good modular design).          *
*---------------------------------------------------------------------*/

   /* N.B.  Space is allocated in d3snsa() for virtual senses if used
   *  as input to a connection type or if plot requested.  Number of
   *  plots is counted in RP->nvcpl so comp nodes can do n3ngph in
   *  nonsuperposition mode.  */
   for (ppvc = &RP0->pvc1; pvc = *ppvc; ) {
      if (pvc->kvp & VKPPL) {
         pvc->vcflags |= VKUSED;
         RP->nvcpl++;
         }
      if (pvc->vcflags & VKUSED) {
         if (pvc->ncells <= 0)
            cryout(RK_E1, "0***", RK_LN2+4, fmtvcn(pvc), RK_CCL,
               " HAS NO CELLS.", RK_CCL, NULL);
         else if (pvc->vcflags & VKR4) RP0->cumnvfc += pvc->ncells;
         else                          RP0->cumnvbc += pvc->ncells;
         /* Usage marking at sources */
         switch (pvc->vtype) {
         case VJ_SRC:
            ((struct ARMDEF *)(pvc->pvcsrc))->dovjtk = TRUE;
            break;
         case VT_SRC:
            RP0->n0flags |= N0F_VTCHSU;
            break;
         case ITPSRC:
            /* Marking already done in itpchk call */
            break;
            } /* End vtype switch */
         ppvc = &pvc->pnvc;
         }
      else if (pvc->vtype == USRSRC)
         ppvc = &pvc->pnvc;
      else {
         *ppvc = pvc->pnvc;
         freep(pvc);
         }
      } /* End sense loop */

/*---------------------------------------------------------------------*
*     Count space needed for TV, PREPROC, and D1 input arrays.         *
*---------------------------------------------------------------------*/

   /* Count cumulative TV image space and calculate plot height to
   *  preserve aspect ratio--can be zero-pass loop.  Set usage flags
   *  in kevtb, ortvfl to simplify special event trial controls.  */
   for (cam = RP->pftv; cam; cam = cam->pnxtv) {
      long ltvim;
      if (!(cam->utv.tvsflgs & TV_ON)) continue;
      /* Set tvsflgs corresponding to combinations of frequency and
      *  interface options for active cameras only for faster tests in
      *  main trial loop.  CAUTION:  This code depends on the numeric
      *  bit definitions.  */
      cam->utv.tvsflgs |= ((cam->utv.tvsflgs &
         (TV_VVTV|TV_BBD|TV_UPGM)) << (3*cam->utv.tvkfreq));
      if (cam->utv.tvkfreq == UTVCFr_TRIAL) {
         if (cam->utv.tvsflgs & TV_UPGM) RP->kevtb |= EVTUTV;
#ifdef BBDS
         if (cam->utv.tvsflgs & TV_BBD)  RP->kevtb |= EVTBTV;
#endif
#ifdef VVTV
         if (cam->utv.tvsflgs & TV_VVTV) RP->kevtb |= EVTVTV;
#endif
         }
      ltvim = (long)cam->utv.tvx * (long)cam->utv.tvy *
         (long)max(cam->utv.tvkcol,1);
      if (cam->utv.tvsflgs & TV_ONDIR) RP0->cumntv += ltvim;
      else                             RP0->cumntvi += ltvim;
      cam->tvht = cam->tvwd *
         (float)cam->utv.tvy/(float)cam->utv.tvx;
      RP->ortvfl |= cam->utv.tvsflgs;
      } /* End camera loop */

   for (pip = RP->pfip; pip; pip = pip->pnip) {
      if (!(pip->ipsflgs & PP_ON)) continue;
      RP0->cumntv += pip->upr.nipx3 * pip->nytot;
      {  ui16 lkpc = pip->upr.nker;    /* Will be kernels/column */
         ui16 ncol = (ui16)pip->dcols; /* Number of plot columns */
         if (pip->upr.ipuflgs & PP_2SYMM) lkpc <<=1;
         if (ncol > 1) lkpc = (lkpc + ncol - 1)/ncol;
         pip->kpc = lkpc;
         } /* End lkpc,ncol local scope */
      } /* End preprocessor loop */

#ifdef D1
   /* Count cumulative D1 input space--can be zero-pass loop.
   *  Also calculate number of horizontal groups in bubble plot.
   *  Space is always allocated for ign information (unless OPT=1
   *  entered on D1ARRAY card) because we haven't opened the files
   *  yet and really are not ready to do so at this time.  If file
   *  is really version 1, this space is wasted.  At only two bytes
   *  per D1 repertoire, this is not a big hit.  (If OPT=1 entered
   *  and file is really version 2, an error occurs in d3fchk.  If
   *  OPT=1 entries are inconsistent, just part of space is wasted.)
   *  Darwin 1 blocks that are not referenced by any connections
   *  are eliminated from the linked list at this time. */
   for (pld1 = &RP->pd1b1; *pld1; ) {
      pd1 = *pld1;
      if (pd1->kd1f & KD1_ALLO) {
         /* Calculate length of input for one repertoire */
         int li1r = pd1->nbyt +
            ((pd1->kd1o & KD1_VER1) ? 0 : sizeof(short));
         /* INDX bit in first used block governs all of them--
         *  this test is not optimized out of loop because pd1b1
         *  ptr may be bad or may change if first block is unused. */
         RP0->cumnd1 += (RP->pd1b1->kd1o & KD1_INDX) ?
            sizeof(int) + li1r : pd1->nd1r * li1r;
         pd1->ngxd1 = (long)sqrt((double)pd1->nepr*pd1->d1w/pd1->d1h);
         if (pd1->ngxd1 <= 0) pd1->ngxd1 = 1;
         pd1->ngyd1 = (pd1->nepr + pd1->ngxd1 - 1)/pd1->ngxd1;
         pld1 = &pd1->pnd1b;
         }
      else {                  /* Delete unused D1BLK */
         *pld1 = pd1->pnd1b;
         freep(pd1);
         }
      } /* End loop over D1BLKs */
#endif

/*---------------------------------------------------------------------*
*  There are now three general holding areas for byte, long, and float *
*     inputs. This allows automatic endian and real format conversions *
*     by membcst as needed.  The byte area (length in cumbcst) has IA, *
*     TV, Darwin1, and byte senses.  The long area (cumval) has values.*
*     The float area has arm/window locations (cumnaw, parallel only)  *
*     and float sense (cumnvfc) data.                                  *
*---------------------------------------------------------------------*/

   /* Store space for input array only if it exists */
   if (!(RP->CP.runflags & RP_NOINPT)) RP->nst = RP->nsx*RP->nsy;

   /* Count cumulative arm/window position broadcast space.
   *  N.B.  If serial, cumnaw exists but is 0 so d3echo can print */
#ifdef PAR
   RP0->cumnaw = RP->narms*BARM_VALS + RP->nwdws*BWDW_VALS;
#endif

   /* Count cumulative VDTDEF (value) block broadcast space */
   RP0->cumval = RP->nvblk*sizeof(struct VDTDEF);

   /* Count cumulative pixel/sense broadcast space */
   RP->cumbcst = RP0->cumnvbc + RP0->cumntv +
#ifdef D1
      RP0->cumnd1 +
#endif
      RP->nst + RP->nst;

   } /* End d3tchk() */

