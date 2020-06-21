/* (c) Copyright 1986-2018, The Rockefeller University *21115* */
/* $Id: d3tchk.c 77 2018-03-15 21:08:14Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                               d3tchk                                 *
*                                                                      *
*     Check dynamic repertoire tree, find connection sources.          *
*     Create MODVAL blocks for modulation and noise modulation.        *
*     Initialize celltype and conntype variables that do not depend    *
*        on node number (see d3nset) or Group III cards (see d3news).  *
*     Calculate defaults for nqx, nqy, values for lnqx, lnqy, lnqyprev.*
*                                                                      *
************************************************************************
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
*  Rev, 06/17/12, GNR - Move kaffck checks to d3lafs so can change     *
*                       more cnopt options at Group III time           *
*  Rev, 06/29/12, GNR - Add qpull, KAMQNB                              *
*  Rev, 12/07/12, GNR - Convert ns[xy], ku[xy] to ui16, nst to ui32    *
*  Rev, 12/24/12, GNR - New overflow checking, revise ssck setting     *
*  Rev, 05/16/13, GNR - Add defaults, new checks for AUTOSCALE OPT=H|W *
*  Rev, 08/21/13, GNR - Revise type-dep detail print to all 32-bit     *
*  R63, 10/31/15, GNR - Remove setting NOPVC if OLDRANGE && vdha != 0  *
*  R63, 11/04/15, GNR - Move volt-dep options to vdopt, add 'A' code   *
*  R65, 01/08/16, GNR - Move INHIBBLK box size var setup to d3allo()   *
*  R66, 03/11/16, GNR - Update controls and variables for senses       *
*                       Calc of nvcells, VT nsya moved from d3schk()   *
*  R67, 04/27/16, GNR - Remove Darwin 1 support, node, nel etc to ints *
*  R72, 02/05/17, GNR - Add KGEN=K ("Kernel"), more longs to si32      *
*  R72, 03/08/17, GNR - Add il->lnqx, eliminate KRPRR, KGNDG setup     *
*  R72, 03/27/17, GNR - Change nrx,nry for KGNCF to srcngx,srcngy      *
*  R73, 04/18/17, GNR - Convert KGNAN to circular or ellipsoidal areas *
*  R74, 06/08/17, GNR - New image size parameters and color modes      *
*  R75, 09/21/17, GNR - Add nbsi32 space for PP_GETP, f[st]ainc        *
*  R75, 10/02/17, GNR - Move value,sense,effector checks to d3vchk()   *
*  R75, 10/02/17, GNR - Add setup code for TERNARY option              *
*  R76, 11/05/17, GNR - Eliminate early phase convolution              *
*  R76, 11/06/17, GNR - Remove vdopt=C, pctt, NORM_SUMS w/cons tables  *
*  R77, 01/30/18, GNR - Add vdmm w/VDOPT=N interaction                 *
*  R77, 03/12/18, GNR - Add kctp=H, hold plot                          *
*  R78, 03/15/18, GNR - Allow any KGEN 1st conn code with KGNKN kernel *
*  R78, 04/24/18, GNR - Change mdincr, mdnel setting for EXGfmC case   *
***********************************************************************/

#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "d3global.h"
#include "celldata.h"
#include "jntdef.h"
#include "armdef.h"
#include "wdwdef.h"
#include "tvdef.h"
#include "swap.h"
#include "rocks.h"
#include "rkxtra.h"
#include "rkarith.h"
#include "bapkg.h"
#include "plots.h"

extern struct CELLDATA CDAT;

/*=====================================================================*
*                               kamchk                                 *
*  This function handles any checking of KAM bits and setting of the   *
*  option combination bits in kam that needs to be done both at d3tchk *
*  time (used in d3allo) and again at d3news time (used in d3go).      *
*=====================================================================*/

void kamchk(struct CONNTYPE *ix) {

   ix->Cn.kam &= ~(KAMVNC|KAMTNU|KAMVNU|KAMQNB);
   if (!(ix->Cn.kam & KAMCG) && ix->Cn.kam & (KAMVM|KAMUE))
                                       ix->Cn.kam |= KAMVNC;
   if (!(ix->Cn.kam & KAMUE)) {
      if (ix->Cn.kam & (KAMVM|KAMCG))  ix->Cn.kam |= KAMVNU;
      if (ix->Cn.kam & KAMTS)          ix->Cn.kam |= KAMTNU;
      }
   if (!(ix->Cn.kam & KAMBCM) && ix->Cn.kam & (KAMDQ|KAMPA))
                                       ix->Cn.kam |= KAMQNB;

   } /* End kamchk() */


/*=====================================================================*
*                               itpchk                                 *
*  This function handles matching of cameras to IMGTPAD sensors.  It   *
*  marks the image source as used (TV_ON), places a pointer to the     *
*  relevant camera in the touch pad block, and fills in default sclx,  *
*  scly if not already entered.  This code is placed here, rather than *
*  in d3schk, so it will not be invoked unless the IMGTPAD is actually *
*  used as input to some connection type.                              *
*                                                                      *
*  R74, 07/26/17, GNR - Modified so d3vitp code accesses camera pmyim  *
*  and tvix,tviy via pcam pointer rather than copies made here.  This  *
*  should allow better access to current info if we go to processing   *
*  images of different sizes within the same run (done in R78).        *
*  R78, 04/14/18, GNR - Remove default setting of scl[xy] to d3vitp    *
*  so can change with each image, put code in d3echo for temp default. *
*  R78, 04/22/18, GNR - Modify flags for requesting gray-from-color.   *
*=====================================================================*/

static void itpchk(struct VCELL *pvc, char *errmsg) {
   struct IMGTPAD *pitp = (struct IMGTPAD *)pvc;
   struct VCELL *qvc;
   /* Find the camera, mark it TV_ON, copy tvx,tvy */
   struct TVDEF *cam = findtv(pitp->hcamnm, errmsg);
   if (cam) {
      pvc->vcxc.tvmode = cam->utv.tvkocol;
      getcshm(&pvc->vcxc, "image touch pad");
      cam->utv.tvsflgs |= pvc->vcxc.tvcsel & EXGfmC ?
         (TV_ONPPGC|TV_ON) : (TV_ONDIR|TV_ON);
      pitp->pcam = cam;
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
*                                                                      *
*  N.B.  It is now necessary to call getcshm for each case to get the  *
*  correct pixel size for EXGfmC case without complex added tests.     *
*  mdincr is always being set to 1 for IA inputs--this must change     *
*  when we make IA equivalent to an image.                             *
*=====================================================================*/

static struct MODVAL *modchk(struct MODBY *im, char *errmsg) {

   struct MODVAL   *pmv,**ppmv;
   struct CELLTYPE *jl;
   struct VCELL    *pvc;
   struct TVDEF    *cam = NULL;
   void            *pms = NULL;     /* Ptr to modulation source */
   ui32 lmdnel;                     /* Number entries in sums */
   int  isndx = im->msrcnm.hbcnm;   /* Index if source is virtual */
   int  istyp = (int)im->msrctyp;   /* Source type code */
   ui16 TVUse = 0;                  /* TV_ON code */
   byte mincr = 1;                  /* Source increment (default) */

   /* All sources can be excitatory */
   im->mssck0 = SJSGN_POS;

/* Handle case of repertoire source separately because some
*  code after the istyp switch is different.  */

   if (istyp == REPSRC) {
      if ((jl = findln(&im->msrcnm)) == NULL) {
         cryout(RK_E1, "0***", RK_LN2+4, fmtrlnm(&im->msrcnm),
            LXRLNM, " NOT FOUND", RK_CCL, errmsg, RK_CCL, NULL);
         return NULL;         /* ERROR EXIT */
         }

      /* This just finds error if color specified for REPSRC */
      getcshm(&im->Mc.mvxc, errmsg+5);
      lmdnel = (ui32)jl->nelt;
      pms = jl;

      /* If using new mV scale, source can also be inhibitory */
      im->mssck0 = RP0->rssck;

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
            RP->kcolor == ENV_MONO ? BM_GS : BM_C8);
         getcshm(&im->Mc.mvxc, errmsg+5);
         lmdnel = (ui32)RP->nsx*(ui32)RP->nsy;
         break;

      case VALSRC:            /* Input from a value */
         /* R66, 03/17/16, GNR - Removed limit test now in readrlnm */
         lmdnel = 1;
         mincr = JSIZE;
         break;

      case VS_SRC:            /* Input from scan window */
         RP->CP.runflags &= ~(RP_NOINPT);
         /* Set the color mode of the input array */
         im->Mc.mvxc.tvmode = (byte)(
            RP->kcolor == ENV_MONO ? BM_GS : BM_C8);
         getcshm(&im->Mc.mvxc, errmsg+5);
         /* Take care of looking through a window */
         if (isndx > 0) {
            struct WDWDEF *pw = pms = RP0->ppwdw[isndx - 1];
            lmdnel = (ui32)(pw->wht*pw->wwd); }
         else
            lmdnel = (ui32)RP->nsx*(ui32)RP->nsy;
         break;

      case VT_SRC: {          /* Input from a touch sense */
         /* At present, VT can have a color.  This code should
         *  be removed when VT is revised to select color at
         *  sense level rather than at conntype level.  (Touch
         *  on an image is now separate ITPSRC code.)  */
         struct ARMDEF *pa;
         struct JNTDEF *pj;
         si16 anjm1;          /* Number of joints minus 1 */
         pvc = findvc(im->msrcnm.rnm, istyp, isndx, errmsg);
         if (!pvc) return NULL;
         im->Mc.mvxc.tvmode = (byte)(
            RP->kcolor == ENV_MONO ? BM_GS : BM_C8);
         getcshm(&im->Mc.mvxc, errmsg+5);
         /* mincr can be left at default 1 here because input is
         *  from IA which is currently all byte size pixels. */
         /* Determine which joint is called for and pick up
         *  size of VT array for that joint  */
         pa = (struct ARMDEF *)pvc->pvcsrc;
         anjm1 = (si16)pa->njnts - 1;
         if (im->msrcnm.hjnt < 0) im->msrcnm.hjnt = anjm1;
         else if (im->msrcnm.hjnt > anjm1) {
            cryout(RK_E1, "0***JOINT > NUM JNTS ON ARM", RK_LN2,
               errmsg, RK_CCL, NULL);
            return NULL; }
         pj = pa->pjnt + im->msrcnm.hjnt;
         if (!pj->u.jnt.tla) {
            cryout(RK_E1, "0***TOUCHPAD ", RK_LN2, im->msrcnm.lnm,
               LSNAME, " IS EMPTY", RK_CCL, errmsg, RK_CCL, NULL);
            return NULL; }
         pvc->vcflags |= VKUSED;    /* Indicate sense is used */
         pvc->jntinVT |= 1 << im->msrcnm.hjnt;
         lmdnel = (ui32)pj->u.jnt.ntl*(ui32)pj->u.jnt.nta;
         break;
         } /* End VT_SRC local scope */

      case TV_SRC: {          /* Input from a camera */
         cam = findtv(isndx, errmsg);
         if (!cam) return NULL;
         TVUse = (TV_ONDIR|TV_ON);
         /* Copy the color mode of the camera */
         im->Mc.mvxc.tvmode = cam->utv.tvkocol;
         getcshm(&im->Mc.mvxc, errmsg+5);
         mincr = (byte)im->Mc.mvxc.tvlpxgc;
         lmdnel = cam->utv.ntvxy;
         pms = cam;
         break;
         } /* End cam local scope */

      case PP_SRC: {          /* Input from an image preprocessor */
         struct PREPROC *pip = findpp(isndx, errmsg);
         if (!pip) return NULL;
         cam = pip->pipcam;
         pip->ipsflgs |= PP_ON;
         TVUse = (TV_ONPP|TV_ON);
         getcshm(&im->Mc.mvxc, errmsg+5);
         mincr = (byte)im->Mc.mvxc.tvlpxgc;
         lmdnel = (ui32)pip->upr.nppx * (ui32)pip->upr.nppy;
         pms = pip;
         break;
         } /* End pip local scope */

      case ITPSRC:            /* Input from image touch */
   #ifdef BBDS
      case USRSRC:            /* Input from a user source */
   #endif
         pvc = findvc(im->msrcnm.rnm, istyp, isndx, errmsg);
         if (!pvc) return NULL;
         if (istyp == ITPSRC) {
            itpchk(pvc, errmsg);
            mincr = (byte)im->Mc.mvxc.tvlpxgc;
            }
         else {   /* Must be USRSRC */
            if (pvc->vcflags & VKR4) {
               im->Mc.mvxc.tvmode = BM_NONSTD, mincr = ESIZE;
               im->mssck0 = (SJSGN_POS|SJSGN_NEG); }
            else
               im->Mc.mvxc.tvmode = BM_GS, mincr = BSIZE;
            getcshm(&im->Mc.mvxc, errmsg+5);
            }
         pvc->vcflags |= VKUSED;    /* Indicate sense is used */
         lmdnel = (ui32)pvc->nsxl * (ui32)pvc->nsya;
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

   /* Set camera usage flags for use in memory alloc, etc.  */
   if (cam) cam->utv.tvsflgs |= im->Mc.mvxc.tvcsel & EXGfmC ?
      (TV_ONGfmC|TV_ON) : TVUse;

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
      if ((pmv->Mdc.mopt ^ im->Mc.mopt) &
         (MOPUNQ|IBOPMX|IBOPKR|IBOPFS)) continue;
      if ((int)pmv->mdsrcndx != isndx) continue;
      if ((int)pmv->mdsrctyp != istyp) continue;
      if (pmv->Mdc.mpspov != im->Mc.mpspov) continue;
      return pmv;
      }

/* Create and initialize new MODVAL block.
*  Note:  lmdnel is a ui32 because mostly is product of 2 ui16s.
*  But pmv->mdnel is an si32 for its use in d3go.  So we need to
*  check for overflow into the sign.  */

   if (lmdnel > SI32_MAX) {
      cryout(RK_E1,"0***SOURCE AREA EXCEEDS 31-BIT LIMIT", RK_LN2,
         errmsg, RK_CCL, NULL);
      lmdnel = SI32_MAX;
      }

   *ppmv = pmv = (struct MODVAL *)
      allocpcv(Static, 1, IXstr_MODVAL, "MODVAL structure");
   pmv->umds.pgsrc = pms;
   pmv->mdnel      = (si32)lmdnel;
   pmv->Mdc        = im->Mc;
   pmv->iovec      = (int)RP->novec++;
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
   struct CONNTYPE *punk,*punq;  /* Ptrs to cn w/unique K,Q tables */
   struct INHIBBLK *ib;
   struct ARMDEF   *pa;
   struct JNTDEF   *pj;
   struct WDWDEF   *pw;
   struct VCELL    *pvc;
   struct MODBY    *im;
   struct TVDEF    *cam;
   struct PREPROC  *pip;

   ui32 lnc;                  /* Copy of nc for detecting changes */
   ui32 nsarb;                /* Number of subarbor repeats */
   int  maxnqx;
   ui32 tops;                 /* Temp for option bits */
   int  ivc;                  /* Virtual cell numeric id */
   int  jsrc;                 /* Source type code */
   short lseqn;               /* Layer sequence number */
   short xseqn;               /* Conntype sequence number */

   /* Work spaces for potential error and warning messages */
   char errmsg[48+2*LSNAME],wrnmsg[48+2*LSNAME];

   /* Table of value source items used for error messages */
   /* Values for kgfl as a function of jsrc */
   static byte kgfl0[NRST+1] = {
                0 /* REPSRC */,  KGNXR|KGNIA /* IA_SRC */,
      KGNXR|KGNVG /* VALSRC */,  KGNXR|KGNVG /* VJ_SRC */,
      KGNXR       /* VH_SRC */,  KGNXR|KGNVG /* VW_SRC */,
      KGNXR|KGNIA /* VS_SRC */,  KGNXR|KGNVG /* VT_SRC */,
      KGNXR|KGNTV /* TV_SRC */,  KGNXR|KGNTV /* PP_SRC */,
      KGNXR|KGNVG /* ITPSRC */,  KGNXR|KGNVG /* USRSRC */ };

/* Be sure there is at least one active repertoire */

   if (!RP->tree)
      cryout(RK_E1, "0***ALL REGIONS/REPERTOIRES ARE EMPTY.",
         RK_LN2, NULL);

/*---------------------------------------------------------------------*
*     Loop over all repertoires--                                      *
*        Be sure there is at least one active cell type                *
*        Acquire information needed to set default nqx,nqy             *
*        Locate source celltypes for modulation and noise modulation   *
*        Locate all targets and fill in target pointers                *
*        Determine final values of nq[xy]                              *
*---------------------------------------------------------------------*/

   lseqn = xseqn = 0;         /* Init layer and conntype sequences */
   punk = punq = NULL;        /* Init lists of unique K,Q tables */

/* The following may be a zero-trip loop: if it is, then a message
*     will have already been written out above.  */

   for (ir=RP->tree; ir; ir=ir->pnrp) {

      /* Error if no cell types in this repertoire */
      if (!ir->nlyr) {
         cryout(RK_E1, "0***NO CELL TYPES IN REGION ", RK_LN2,
            getrktxt(ir->hrname), ir->lrn, NULL);
         continue; }

      /* Error if plotting gaps are too large.  Note:  At this time,
      *  we don't know for sure whether KRPPL plots will be done, but
      *  this is most convenient place to report the error.  */
      if (ir->Rp.xgap >= 1.0 || ir->Rp.ygap >= 1.0)
         cryout(RK_E1, "0***PLOT GAPS LEAVE NO ROOM FOR CELLS IN "
            "REGION ", RK_LN2, getrktxt(ir->hrname), ir->lrn, NULL);

/* Loop over all cell types in this repertoire */

      maxnqx = 0;

      for (il=ir->play1; il; il=il->play) {
         CDAT.cdlayr = il;       /* For d3lime */

         /* Hold original kstat for d3chng */
         il->urkstat = il->Ct.kstat;

         /* If rspmeth was not explicitly set (by CELLTYPE, AEIF,
         *  or IZHI card), but a spike value was entered, set rspmeth
         *  to RF_SPIKE.  If a stanh value was entered, set rspmeth
         *  to RF_TANH.  Next in line, use value from MASTER CELLTYPE.
         *  If none of the above, set it to RF_STEP or RF_KNEE
         *  according to OPTDR option.  */
         if (!(il->ctwk.tree.krmop & KRM_RFENT)) {
            if (il->ctwk.tree.krmop & KRM_THENT)
               il->Ct.rspmeth = RF_TANH;
            else if (il->ctwk.tree.krmop & KRM_SPENT)
               il->Ct.rspmeth = RF_SPIKE;
            else if (il->Ct.rspmeth == RF_NEVER_SET)
               il->Ct.rspmeth = il->Ct.ctopt & OPTDR ?
                  RF_STEP : RF_KNEE;
            }

         /* Set bits indicating stats matrices must be allocated */
         if (il->Ct.kstat & KSTXR)              RP->kxr |= XSTATS;
         if (il->Ct.kstat & KSTYM)              RP->kxr |= YSTATS;
         if (il->Ct.kstat & KSTHR)              RP->kxr |= HSTATS;
         if (il->Ct.kstat & KSTCL)              RP->kxr |= CSTATS;
         if (il->Ct.kstat & (KSTGP|KSTMG|KSTHR))RP->kxr |= GSTATS;
         if (il->Ct.kstat & KSTZC)              RP->kxr |= ZSTATS;
         if (il->Ct.ctopt & OPTCFS)             RP->kxr |= ISOPTC;

         /* Pick up information needed for assignment of default nqx */
         if (il->nel > maxnqx && !(il->Ct.kctp & CTPHP))
            maxnqx = il->nel;

         /* Find largest nct of any cell type, does malloc in d3go */
         RP->mxnct = max(RP->mxnct,il->nct);

         /* Set stride and total si,phi length */
         il->lsp = LSmem + il->phshft;
         il->wspt = jmuw(il->nelt, il->lsp);
         il->lspt = uw2zd(il->wspt);

         /* il->kautu is used (anytime after d3tchk) to test autoscale
         *  options without need to check il->pauts != NULL, although
         *  kautu is maintained (and some bits modified) by d3asck().
         *  Hold off setting RP0->nautsc until now to avoid error if
         *  user enters > 1 AUTOSCALE card for this cell type, but
         *  count even if navg == 0 in case navg is changed later.
         *  Warn if autoscale used with Gerstner or Izhikevich RF.  */
         if (il->pauts) {
            struct AUTSCL *paut = il->pauts;
            if (paut->navg < 0) {
               int dnav = il->nelt/25;
               if (dnav < 10) dnav = min(10, il->nelt);
               else if (dnav > SHRT_MAX) dnav = SHRT_MAX;
               paut->navg = (si16)dnav;
               }
            else if ((int)paut->navg > il->nelt)
               cryout(RK_E1, "0***navg > ncells FOR ", RK_LN2,
                  fmturlnm(il), LXRLNM, ".", 1, NULL);
            il->kautu = paut->kaut;
            ++RP0->nautsc;
            if (il->Ct.rspmeth >= RF_BREG) cryout(RK_P1,
               "0-->WARNING:  Autoscale with AEIF or Izhikevich "
               "RF will give incorrect adaptation for ", RK_LN2,
               fmtlrlnm(il), LXRLNM, ".", 1, NULL);
            }

         /* Set default siet,siscl according to phase option */
         {  int gotspcf = il->pctpd && il->pctpd->pselfpcf;
            if (il->Dc.siet == -1)
               il->Dc.siet = gotspcf ? JFIXP(12.8,FBwk) : 0;
            if (il->Dc.siscl == -1)
               il->Dc.siscl = gotspcf ? JFIXP(5.657,FBsc) : 0;
            } /* End gotspcf local scope */

         /* Error if attempt to plot a conntype that doesn't exist */
         if (il->Ct.kctp & KRPNPL && il->jijpl > (short)il->nct)
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
            if (qdecay(&im->MDCY)) pmv->Mdc.mdefer = 0;
            else if (im->Mc.mdefer < pmv->Mdc.mdefer)
               pmv->Mdc.mdefer = im->Mc.mdefer;
            im->Mc.jself = (il == (struct CELLTYPE *)pmv->umds.pgsrc);
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
            ix->sssck0 = SJSGN_POS;

            /* Implement COMPAT=K.  (Modify cnopt after possible
            *  settings by CONNTYPE, PARAMS, or THRESH cards)  */
            if (RP->compat & OLDKNEE) ix->Cn.cnopt |= NOPKR;

            /* Masks related to nbc */
            ix->cmsk = (~(si32)0)<<(BITSPERUI32-ix->Cn.nbc);
            ix->bigc16 = SRA(ix->cmsk,FBCij) & (si32)0x0000ffff;
            ix->cijlen = BytesInXBits(ix->Cn.nbc);

            /* N.B.  Be sure this code runs before first d3lkni() call.
            *  Set kgen bit for easy detection of tuning curve version.
            *  Let cijmin be in units of 2^(-nbc) if > 1, otherwise
            *  set default.  */
            if (ix->kgen & KGNLM && ix->ucij.l.flags & LOPTW)
               ix->kgen = (ix->kgen & ~KGNLM) | KGNTW;
            if (ix->Cn.cijmine == SI32_SGN)     /* Default cijmin */
               ix->cijmin = (ix->kgen & (KGNTW|KGNE2)) ?
                  SI32_MAX >> (ix->Cn.nbc-1) : 0;
            else if (ix->Cn.cijmine >= S15) {
               if ((ui32)ix->Cn.cijmine >= (ui32)S15 << ix->Cn.nbc)
                  cryout(RK_E1, "0***CMIN < NBC FRAC BITS",
                     RK_LN2, errmsg, RK_CCL, NULL);
               else if (ix->Cn.nbc < Ss2hi)
                  ix->cijmin = ix->Cn.cijmine << (17 - ix->Cn.nbc);
               /* JIC 0 shift ==> 32 */
               else ix->cijmin = ix->Cn.cijmine;
               }
            else ix->cijmin = ix->Cn.cijmine << Ss2hi;

            /* Old cnopt V is now vdopt S (No compat, C is gone) */
            if (ix->Cn.cnopt & NOPVD) ix->Cn.vdopt |= VDOPS;
            /* And vdmm is >= 0 if VDOPN (here for d3echo and again
            *  in d3news in case changed at Group III time)  */
            if (ix->Cn.vdopt & VDOPN && ix->Cn.vdmm < 0)
               ix->Cn.vdmm = 0;

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
               ix->sssck0  = RP0->rssck;
               break;

case IA_SRC:   RP->CP.runflags &= ~(RP_NOINPT);
               ix->iawxsz = (ui32)RP->nsx;
               ix->iawysz = (ui32)RP->nsy;
               /* Take care of extracting the specified color */
               ix->cnxc.tvmode = (byte)(
                  RP->kcolor == ENV_MONO ? BM_GS : BM_C8);
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

case VALSRC:   ix->cnflgs |= NOMDLT;
               ix->srcngx = RP->nvblk;
               ix->srcngy = 1;
               break;

case VS_SRC:   RP->CP.runflags &= ~(RP_NOINPT);
               /* Establish pointer to window data for scan and save
               *  size of window for bounds checking */
               ix->psrc = pw = RP0->ppwdw[ix->wdwi];
               if (!pw) {
                  /* No message here, this is from an earlier error */
                  jsrc = BADSRC; break;
                  }
               ix->iawxsz = (ui32)pw->wwd;
               ix->iawysz = (ui32)pw->wht;
               /* Take care of extracting the specified color */
               ix->cnxc.tvmode = (byte)(
                  RP->kcolor == ENV_MONO ? BM_GS : BM_C8);
               getcshm(&ix->cnxc, errmsg+4);
               /* Caution:  See note under IA_SRC regarding potential
               *  problems if the planned window dilation option is
               *  implemented.  Motor maps use nux,nuy always. */
               ix->srcngx = RP->nsx;
               ix->srcngy = RP->nsy;
               break;

case VT_SRC:
               /* At present, VT can have a color.  This code should
               *  be removed when VT is revised to select color at
               *  sense level rather than at conntype level.
               *  R66, 03/17/16, GNR - size of VT_SRC is now restricted
               *     to one touchpad for all kgen types.  */
               if (pvc = findvc(ix->srcnm.rnm, jsrc, ivc, errmsg)) {
                  si16 anjm1;
                  pa = (struct ARMDEF *)pvc->pvcsrc;
                  anjm1 = (si16)pa->njnts - 1;
                  if (ix->srcnm.hjnt < 0) ix->srcnm.hjnt = anjm1;
                  else if (ix->srcnm.hjnt > anjm1) {
                     cryout(RK_E1, "0***JOINT > NUM JNTS ON ARM",
                        RK_LN2, errmsg, RK_CCL, NULL);
                     jsrc = BADSRC; break;
                     }
                  pj = pa->pjnt + ix->srcnm.hjnt;
                  if (!pj->u.jnt.tla) {
                     cryout(RK_E1, "0***TOUCHPAD ", RK_LN2,
                        im->msrcnm.lnm, LSNAME, " IS EMPTY", RK_CCL,
                        errmsg, RK_CCL, NULL);
                     jsrc = BADSRC; break; }
                  pvc->vcflags |= VKUSED; /* Indicate sense is used */
                  pvc->jntinVT |= 1 << ix->srcnm.hjnt;
                  ix->psrc = (void *)pvc;
                  ix->srcngx = (ui16)pj->u.jnt.ntl;
                  ix->srcngy = (ui16)pj->u.jnt.nta;
                  if (pvc->vcflags & VKR4)
                     ix->sssck0 = (SJSGN_POS|SJSGN_NEG);
                  ix->cnxc.tvmode = (byte)(
                     RP->kcolor == ENV_MONO ? BM_GS : BM_C8);
                  getcshm(&ix->cnxc, errmsg+4);
                  }
               else
                  jsrc = BADSRC;
               break;

               /* Locate the requested camera--Assignment intended */
case TV_SRC:   if (cam = findtv(ivc, errmsg)) {
                  ix->psrc = cam;
                  ix->srcngx = cam->utv.tvx;
                  ix->srcngy = cam->utv.tvy;
                  /* Take care of extracting the specified color */
                  ix->cnxc.tvmode = cam->utv.tvkocol;
                  getcshm(&ix->cnxc, errmsg+4);
                  cam->utv.tvsflgs |= ix->cnxc.tvcsel & EXGfmC ?
                     (TV_ONGfmC|TV_ON) : (TV_ONDIR|TV_ON);

                  if (ix->Cn.cnopt & NOPNI) cam->utv.tvsflgs |= TV_GETN;
                  }
               else
                  jsrc = BADSRC;
               break;

               /* Locate preprocessor--Assignment intended */
case PP_SRC:   if (pip = findpp(ivc, errmsg)) {
                  /* Take care of extracting the specified color */
                  ix->cnxc.tvmode = pip->upr.ipkcol;
                  getcshm(&ix->cnxc, errmsg+4);
                  /* If getcshm marks EXGfmC, this is an error because
                  *  a preprocessor with PP_GfmC and thus no color in
                  *  its output should have been requested.  */
                  if (ix->cnxc.tvcsel & EXGfmC) {
                     cryout(RK_E1, "0***USE PRPROC OPT=G", RK_LN2,
                        errmsg, RK_CCL, NULL);
                     jsrc = BADSRC; break; }
                  pip->ipsflgs |= PP_ON;
                  pip->pipcam->utv.tvsflgs |= TV_ONPP | TV_ON;
                  ix->psrc = pip;
                  ix->srcngx = pip->upr.nppx;
                  /* pip->nytot checked in d3schk < UI16MAX */
                  ix->srcngy = pip->nyvis;
                  ix->srcnimg = pip->upr.lppi3;
                  if (ix->Cn.cnopt & NOPNI) pip->ipsflgs |= PP_GETN;
                  /* If this is a PP_STGH preprocessor, find out if
                  *  our layer nel is unique and allocate space for
                  *  the sin/cos tables needed by the sjbr and
                  *  PP_GETN routines for PP_GETP  */
                  if (pip->ipsflgs & PP_STGH) {
                     struct CELLTYPE *jjl;
                     if (ix->Cn.cnopt & NOPVP) pip->ipsflgs |= PP_GETP;
                     for (jjl=RP->pfct; jjl->sfnel; jjl=jjl->pnct) {
                        if (il->nel == jjl->sfnel) {
                           ix->oPPsct = jjl->sfoff;
                           goto GotOldPPsctTable;
                           }
                        }
                     /* No match, new unique nel.  Note:  I am allo-
                     *  cating for 2*nel trig entries JIC nel is odd. */
                     ix->oPPsct = jjl->sfoff = RP->tuqnel;
                     RP->tuqnel += 2*(jjl->sfnel = il->nel);
                     ++RP->nuqnel;
GotOldPPsctTable:    ix->kgfl |= KGNSF;
                     }
                  }
               else
                  jsrc = BADSRC;
               break;

case VH_SRC:   RP->CP.runflags &= ~(RP_NOINPT);
               /* Locate the requested arm */
               ix->psrc = RP0->pparm[ix->armi];
               ix->srcngx = RP->nsx;
               ix->srcngy = RP->nsy;
               break;

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
                  ix->srcngx = (ui16)pvc->nsxl;
                  ix->srcngy = (ui16)pvc->nsya;
                  ix->cnxc.tvmode = pvc->vcxc.tvmode;
                  if (pvc->vcflags & VKR4)
                     ix->sssck0 = (SJSGN_POS|SJSGN_NEG);
                  }
               else
                  jsrc = BADSRC;
               break;

               } /* End jsrc switch */

/* Omit all remaining code in CONNTYPE loop if source not found.
*  (This is mainly to avoid invalid memory reference via psrc.)  */

            if (jsrc == BADSRC) continue;

/* Set values of srcnel, srcnelt based on the above.
*  Flag whether input is from same layer for autoscale
*  and turn off self-avoidance if not self input.
*  Also, turn on the generic input type flags in kgfl
*  (moved here from d3go, 01/12/97).
*  R67, 10/06/16, GNR - Add KGNPB flag so d3kij can sense windows
*     that need d3go boundary checking without access to WDWDEF */

            ix->kgfl |= kgfl0[jsrc];

            /* Error if NOPNI and !KGNTV */
            if (ix->Cn.cnopt & NOPNI && !(ix->kgfl & KGNTV))
               cryout(RK_E1, "0***CNOPT=N ONLY VALID WITH IMAGE "
                  "INPUT", RK_LN2, errmsg, RK_CCL, NULL);
            /* Error if NOPVP and !PP_STGH or 1 < nel < 256 or color
            *  request other than GS16 (change when code written).  */
            if (ix->Cn.cnopt & NOPVP && (jsrc != PP_SRC ||
                  !(((struct PREPROC *)ix->psrc)->ipsflgs & PP_GETP) ||
                  il->nel < 2 || il->nel > BYTE_MAX ||
                  !qGray(ix->cnxc.tvmode)))
               cryout(RK_E1,"0***CNOPT=P REQUIRES GRAY IMAGE, STGH2 "
                  "PREPROCESSOR  AND 1 < NEL < 256", RK_LN2, errmsg,
                  RK_CCL, NULL);

            /* Check for piggybacked scan window.  Window may go out
            *  of bounds if kchng=4,5 or kchng<0.  Check for these
            *  cases with one test.  This trick will fail if new
            *  options are added with 4 bit set.  */
            if ((ix->kgen & KGNST) &&
                  (((struct WDWDEF *)(ix->psrc))->kchng & 4))
               ix->kgfl |= KGNPB;
            if (jsrc == REPSRC) {
               ix->srcnel  = jl->nel;
               ix->srcnelt = jl->nelt;
               if (!(ix->jself = (il == jl))) ix->kgen &= ~KGNSA;
               }
            else {
               ix->srcnel = 1;
               ix->srcnelt = (ui32)ix->srcngx * (ui32)ix->srcngy;
               ix->kgen &= ~KGNSA;
               }
            /* Our size spec guarantees that celltype cannot have
            *  more than 2^31 cells, therefore srccpr cannot exceed
            *  the size of an si32, but the product of the components
            *  could, so here is the place to test for overflow.  */
            {  ui64 tcpr = jmuw(ix->srcnel, (ui32)ix->srcngx);
               if (qcuw(jeul(SI32_MAX), tcpr)) {
                  cryout(RK_E1, "0***MORE THAN 2^31 CELLS/ROW",
                     RK_LN2, errmsg, RK_CCL, NULL);
                  ix->srccpr = SI32_MAX; }
               else
                  ix->srccpr = uwlo(tcpr);
               } /* End tcpr local scope */

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
                  ix->lsg = (si32)ix->srcnel;
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
                  ix->lsg = (si32)ix->srcngx;
                  break;

               /* Input from "VT"--
               *  default is size of one touch pad */
               case VT_SRC:
                  ix->lsg = (si32)ix->srcngx*(si32)ix->srcngy;
                  break;

                  } /* End jsrc switch */

               ix->lsg <<= 1;    /* Scale lsg to S1 */
               } /* End setting lsg default */

/* nrx,nry defaults for KGNKN:  If both are zero, make them 16, other-
*  wise the zero one equals the other.  Determine whether needed table
*  is unique.  Compute size of needed table.  Then nc is determined by
*  number required for kernel, overriding any user input.  */

            if (ix->kgen & KGNKN) {
               struct CONNTYPE *jx;
               if ((ix->nrx|ix->nry) == 0) ix->nrx = ix->nry = 16;
               else if (ix->nrx == 0) ix->nrx = ix->nry;
               else if (ix->nry == 0) ix->nry = ix->nrx;
               for (jx=punk;jx;jx=(struct CONNTYPE *)jx->ul2.k.pkern) {
                  /* Look for an earlier conntype with same params */
                  if (ix->nrx == jx->nrx && ix->nry == jx->nry &&
                      ix->Cn.tcwid == jx->Cn.tcwid &&
                      ix->cijmin == jx->cijmin &&
                      ix->stride == jx->stride &&
                      ix->hradius == jx->hradius &&
                      ix->srcnel == jx->srcnel) {
                     ix->ul2.k.pkern = (kern *)jx; /* Point to it */
                     ix->nc = jx->nc;
                     goto NOT_A_NEW_K_TABLE;
                     }
                  }  /* End search loop for unique params */
               /* Unique KGNKN config found, do d3lkni init */
               ix->ul2.k.pkern = (kern *)punk, punk = jx;
               ix->nc = d3lkni(ix, &ix->ul2.k.lkern);
               RP->lkall += ix->ul2.k.lkern;
#ifdef PAR0
               if (ix->knopt & KNO_PRINT && RP->CP.runflags & RP_OKPRIN)
                  RP0->lknprt += ix->ul2.k.lkern;
#endif
NOT_A_NEW_K_TABLE: ;
               }  /* End tests for KGNKN */

/* Store angle increments for cell within a group as used for KGNTW.
*  (We know nel > 0 from test in d3grp2).  */

            if (ix->kgen & KGNTW) {
               float ainc = (float)PI*(float)180./ix->Cn.tcwid;
               ix->ucij.lw.fainc = ainc;
               ix->ucij.lw.fsainc = ainc/(float)ix->srcnel;
               ix->ucij.lw.ftainc = ainc/(float)il->nel;
               }

/* Generate default for nsa.  nsa is needed later whether or not
*  SUBARBOR exists.  If nsa was not entered, derive it from nrx,nry if
*  they are relevant and were entered, otherwise from nc as entered.
*  In this latter case, defaulting of nsa is normal, so no message is
*  given.  */

            lnc = ix->nc;        /* Copy of nc before nc *= nsarb */
            if (ix->nsa == 0) {
               if (ix->Cn.saopt & SAOPTL) ix->nsa = NColorDims;
               else if (ix->kgen & (KGNBL|KGNMC) &&
                     ix->nrx > 0 && ix->nry > 0)
                  ix->nsa = (ui32)ix->nrx*(ui32)ix->nry;
               else ix->nsa = lnc;
               }
            nsarb = (lnc + ix->nsa - 1)/ix->nsa;

/* Generate defaults for nrx,nry if 'B','C','Q', or 'M' codes.
*  (If M used w/o B,C,Q, nrx,nry are still used to enter coeffs.
*  Code moved here from d3tree 03/09/93 so can use nsa if entered.
*  Rev, 04/19/17, GNR - For C,Q, if both of nrx,nry are 0, set equal
*     to srcngx,srcngy, otherwise set the 0 one equal to the other.  */

            if (ix->kgen & (KGNBL+KGNCF+KGNAN+KGNMC)) {
               ui32 lnsa = ix->nsa;    /* Copy of nsa before change */
               /* Size of one repeating box */
               ui32 lnsr = (ix->Cn.saopt & SAOPTL) ? nsarb : lnsa;
               if (ix->kgen & (KGNCF|KGNAN)) {
                  if ((ix->nrx|ix->nry) == 0)
                     ix->nrx = ix->srcngx, ix->nry = ix->srcngy;
                  else if (ix->nrx == 0) ix->nrx = ix->nry;
                  else if (ix->nry == 0) ix->nry = ix->nrx;
                  }
               else {
                  if (ix->nrx == 0) ix->nrx = sqrt((double)lnsr) + 0.5;
                  if (ix->nry == 0) ix->nry = lnsr/ix->nrx; }
               if (ix->kgen & (KGNBL+KGNMC)) {
                  nsarb = ix->nrx*ix->nry;
                  if (!(ix->Cn.saopt & SAOPTL)) {
                     ix->nsa = nsarb;
                     if (ix->nsa != lnsa) convrt("('0-->WARNING: "
                        "nsa reset to ',J1UJ8,'= nrx*nry'/4X,A120)",
                        &ix->nsa, wrnmsg, NULL);
                     nsarb = (lnc + ix->nsa - 1)/ix->nsa;
                     }
                  } /* End if B or M */
               } /* End if B,C,Q, or M */

/* Set final value of nc as a multiple of subarbor size and
*  give a warning if this is a change from the input value.
*  Give an error if published 2^28 limit is exceeded.
*  (Now nce is not added up until here, after nc might change.)  */

            if (nsarb >= S16) {  /* Per spec */
               nsarb = S16 - 1;
               cryout(RK_P1,"0--->WARNING: Number of subarbors reset "
                  " to 65535", RK_LN2, wrnmsg, RK_CCL, NULL); }
            ix->nsarb = nsarb;
            if (ix->nsa >= (1 << IBnc)/nsarb) {
               cryout(RK_E1, "0***nc EXCEEDS 2^28 LIMIT", RK_LN2,
                  errmsg, RK_CCL, NULL);
               ix->nc = (1 << IBnc); }
            else ix->nc = nsarb*ix->nsa;
            if (ix->nc != lnc) convrt("('0-->WARNING: nc reset "
               "to ',J1IJ8,'= mult of nrx*nry or nsa'/4X,A120)",
               &ix->nc, wrnmsg, NULL);
            lnc = ix->nc;
            ix->ocdat = il->nce; /* Cumulative conns before these */
            il->nce += lnc;      /* Accumulate total conns/element */

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
            if ((tops != 0) != (ix->nsa < lnc)) {
               if (tops)   cryout(RK_P1, "0-->WARNING: SUBARBOR "
                  "option ignored, too few connections",
                  RK_LN3, "    ", RK_LN0, wrnmsg, RK_CCL, NULL);
               else        cryout(RK_P1, "0-->WARNING: SUBARBOR "
                  "activated, nc > matrix size or nsa",
                  RK_LN3, "    ", RK_LN0, wrnmsg, RK_CCL, NULL);
               ix->Cn.saopt ^= DOSARB;
               }
            if (ix->nsax == 0)
               ix->nsax = (ui32)sqrt((double)nsarb) + 0.5;
            if (ix->Cn.saopt & DOSARB) {
               if (ix->Cn.saopt & SAOPTA) {
                  /* Subarbor Cij must be cloned if averaged */
                  ix->Cn.saopt |= SAOPTC;
                  if (ix->nsa > RP->mxsaa) RP->mxsaa = ix->nsa;
                  }
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
                  ui32 lfdm;
                  curpfdh = RP->pfdh + ix->ucij.m.mno - 1;
                  lfdm = curpfdh->mrx*curpfdh->mry;
                  /* We can never use more than one matrix per cell,
                  *  but we can reuse 1 matrix if have subarboring. */
                  if (ix->Cn.saopt & SAOPTL) {
                     if (nsarb > lfdm) cryout(RK_E1,
                        "0***NSUBARBS > SIZE OF FD MATRICES", RK_LN2,
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
                  if ((ix->kgen & KGNBL) &&
                        (ix->nrx != curpfdh->mrx ||
                        ix->nry > curpfdh->mry)) {
                     cryout(RK_E1, "0***NRX,NRY DO NOT COMPLY WITH "
                        "SIZE OF MATRICES READ", RK_LN2,
                        errmsg, RK_LN1, NULL);
                     } /* End dealing with mrx,mry mismatch */
                  /* Set default nkt, changed in V8A to min(nel,npat) */
                  if (ix->ucij.m.nkt <= 0)
                     ix->ucij.m.nkt = min(il->nel, curpfdh->npat);
                  lfdm = ix->ucij.m.nks * ix->ucij.m.nkt;
                  if (lfdm > curpfdh->npat)
                     cryout(RK_E1, "0***NKS*NKT > NUMBER OF PATTERNS "
                        "IN FD MATRIX FILE", RK_LN2,
                        errmsg, RK_LN1, NULL);
                  } /* End else */
               } /* End tests for KGEN=M */

/* Tests on connection-generating parameters (first code).
*  (Extensively revised, 11/16/91, GNR to consolidate tests
*  previously located in d3tree, d3allo, and d3go.)  */

            /* Generate default values for nux, nuy, and offsets for
            *  kgen=F,S,T maps.  Adjust for box matrix size.  Perform
            *  range checking on nux,nuy,xoff,yoff.  */
            if (ix->kgen & (KGNST+KGNFU+KGNTP)) {
               si32 nuxdflt,nuydflt,nuxeff,nuyeff;
               if (ix->kgen & KGNST) {
                  nuxdflt = ix->iawxsz; nuydflt = ix->iawysz; }
               else {
                  nuxdflt = (si32)ix->srcngx;
                  nuydflt = (si32)ix->srcngy; }
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
                  ix->nux = (si32)ix->srcngx;
                  ix->nuy = (si32)ix->srcngy; }
               else if (ix->kgen & KGNTP) {
                  if (ix->nux % ir->ngx || ix->nuy % ir->ngy)
                     cryout(RK_P1, "0-->WARNING: Topographic mapping "
                        "is nonintegral",RK_LN2,wrnmsg,RK_CCL,NULL);
                  }

               if (ix->xoff == SI32_SGN) /* -0 used to request dflt */
                  ix->xoff = (nuxdflt - ix->nux)>>1;
               if (ix->yoff == SI32_SGN) /* -0 used to request dflt */
                  ix->yoff = ((nuydflt - ix->nuy)>>1);

               /* The following tests take subarboring into account but
               *  not the extent of any B, C arbors except insofar as
               *  B decreases default nux,nuy.  Maybe they should.  */
               nuxeff = ix->nux, nuyeff = ix->nuy;
               if ((ix->Cn.saopt & (DOSARB|SAOPTL)) == DOSARB) {
                  nuxeff += ix->nsax - 1;
                  nuyeff += (ix->nc - 1)/(ix->nsa*ix->nsax);
                  }
               /* It is an error if the entire box is off the source */
               if (ix->xoff >= (si32)ix->srcngx ||
                   ix->yoff >= (si32)ix->srcngy ||
                   ix->xoff + nuxeff <= 0 || ix->yoff + nuyeff <= 0)
                  cryout(RK_E1, "0***SPECIFIED SOURCE AREA HAS NO "
                     "OVERLAP WITH SOURCE", RK_LN2,
                     errmsg, RK_CCL, NULL);
               /* It is a warning if part of source is out-of-bounds */
               else if (ix->xoff < 0 || ix->yoff < 0 ||
                     ix->xoff + nuxeff > (si32)ix->srcngx ||
                     ix->yoff + nuyeff > (si32)ix->srcngy)
                  cryout(RK_P1, "0-->WARNING: Specified source area is "
                     "partly outside bounds of source", RK_LN3,
                     "    ", RK_LN0, wrnmsg, RK_CCL, NULL);
               } /* End KGEN=F,S,T */

            else if (ix->kgen & KGNG1) {
               /* Defaults for xoff,yoff:  Center the target on the
               *  source, even if negative overlaps.  */
               if (ix->xoff == SI32_SGN)
                  ix->xoff = ((si32)ix->srcngx - (si32)ir->ngx)>>1;
               if (ix->yoff == SI32_SGN)
                  ix->yoff = ((si32)ix->srcngy - (si32)ir->ngy)>>1;
               } /* End KGEN=1 */

            else if (ix->kgen & KGNBV) {
               /* vcmax > vcmin is checked earlier */
               float dv = pvc->vcmax - pvc->vcmin;
               if (!ix->nux) ix->nux = ir->ngx;
               if (!ix->nuy) ix->nuy = ir->ngy;
               /* This a convenience for KGNBV, else harmless--
               *  note that KGNAJ is forced with KGNBV  */
               if (ix->cnxc.tvmode == BM_NONSTD) {
                  ix->ul2.a.bvv1 = pvc->vcmin;
                  ix->ul2.a.bvxdv = (float)ix->nux/dv;
                  ix->ul2.a.bvydv = (float)ix->nuy/dv;
                  }
               else {
                  ix->ul2.a.bvxdv = (float)ix->nux/dS8;
                  ix->ul2.a.bvydv = (float)ix->nuy/dS8;
                  }
               /* Check offsets--ui32 test catches negatives */
               if ((ui32)ix->xoff > (ui32)pvc->nsxl ||
                   (ui32)ix->yoff > (ui32)pvc->nsya ||
                   ix->xoff + ix->loff > pvc->nsxl)
                  cryout(RK_E1, "0***SPECIFIED OFFSETS ARE OUTSIDE "
                     "SOURCE SENSE", RK_LN2, errmsg, RK_CCL, NULL);
               }

            /* KGEN=J,X,Y:  Error if nux or nuy too large.  Check if
            *  initial offsets are outside the source.  This is a
            *  warning for J (the generating routine can handle it,
            *  and sometimes it is what user wants), but an error
            *  for X,Y (generating routines cannot handle this case).
            *  Rev, 06/27/08, GNR - Add nux,nuy defaults for J,X,Y.
            *  Rev, 12/23/08, GNR - Remove new KGNXO nux default--it
            *     snafu'd some test runs, also 0 may often be wanted.
            */

            if (ix->kgen & (KGNJN|KGNXO|KGNYO)) {
               /* This code is OK for IA, given the test below */
               long totoff = ((long)ix->xoff + (long)ix->yoff*
                  (long)ix->srcngx)*(long)ix->srcnel + (long)ix->loff;
               if ((ui32)ix->nux > (ui32)ix->srccpr)
                  cryout(RK_E1, "0***NUX EXCEEDS ROW SIZE",
                     RK_LN2, errmsg, RK_CCL, NULL);
               if ((ui32)ix->nuy > (ui32)ix->srcngy)
                  cryout(RK_E1, "0***NUY EXCEEDS NUMBER OF ROWS"
                     " IN SOURCE", RK_LN2, errmsg, RK_CCL, NULL);
               if (ix->kgen & KGNJN) { /* Offset test for rule J */
                  if (!(ix->nux|ix->nuy)) ix->nuy = 1;
                  if (totoff < 0 ||
                        (totoff<<1)+ix->lsg >= (long)ix->srcnelt<<1)
                     cryout(RK_P1, "0-->WARNING: Offset < 0 or > "
                        "layer size", RK_LN2, wrnmsg, RK_CCL, NULL);
                  } /* End tests for KGEN=J */
               else {                  /* Offset test for rule X,Y */
                  if (totoff < 0 || totoff >= (long)ix->srcnelt)
                     cryout(RK_E1, "0***OFFSET < 0 OR > LAYER SIZE",
                        RK_LN2, errmsg, RK_CCL, NULL);
                  } /* End tests for KGEN=X,Y */
               }  /* End combined X,J tests */

/* Tests on connection-generating parameters (second code) */

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
               else if (jsrc == REPSRC && ix->stride >= ix->srccpr)
                  cryout(RK_P1, "0-->WARNING: Stride > row size",
                     RK_LN2, wrnmsg, RK_CCL, NULL);
               } /* End tests for KGEN=A,B */

            /* KGEN=Q:  Default inner area to a circle if only one
            *  axis was entered.  Make sure difference area is
            *  nonempty.  Determine whether needed table is unique.
            *  Compute size of needed table.  */

            if (ix->kgen & KGNAN) {
               struct CONNTYPE *jx;
               /* (This code leaves nhx==nhy==0 unchanged) */
               if      (ix->nhy == 0) ix->nhy = ix->nhx;
               else if (ix->nhx == 0) ix->nhx = ix->nhy;
               if (ix->nhx >= ix->nrx && ix->nhy >= ix->nry)
                  cryout(RK_E1, "0***nrx,nry BOX IS INSIDE nhx,nhy BOX",
                     RK_LN2, errmsg, RK_CCL, NULL);
               for (jx=punq; jx; jx=(struct CONNTYPE *)jx->ul2.q.paxy) {
                  /* Look for an earlier conntype with same params */
                  if (ix->nrx == jx->nrx && ix->nry == jx->nry &&
                      ix->nhx == jx->nhx && ix->nhy == jx->nhy) {
                     ix->ul2.q.paxy = (xysh *)jx;  /* Point to it */
                     goto NOT_A_NEW_Q_TABLE;
                     }
                  }  /* End search loop for unique params */
               /* Unique KGNAN config found, do d3lani init */
               ix->ul2.q.paxy = (xysh *)punq, punq = ix;
               d3lani(ix, &ix->ul2.q.lannt);
               RP->lkall += ALIGN_UP(sizeof(xysh)*ix->ul2.q.lannt);
NOT_A_NEW_Q_TABLE: ;
               } /* End tests for KGEN=Q */

            /* KGEN=B,C,K or Q: Test for crazy values of nrx,nry */
            if ((ix->kgen & (KGNBL+KGNCF+KGNKN+KGNAN)) &&
                  (ix->nrx > ix->srcngx || ix->nry > ix->srcngy))
               cryout(RK_P1, "0-->WARNING: B,C,K,Q map exceeds size "
                  "of source", RK_LN2, wrnmsg, RK_CCL, NULL);

/* The following tests apply only if the source is REPSRC */

            if (jsrc == REPSRC) {  /* Found cell layer */

               jr = jl->pback;

               /* KGEN=G,1,H,N,O: Test for same source,target rep shape--
               *  replaces stricter test in d3tree that source==self,
               *  10/11/92, GNR  */
               if (ix->kgen & (KGNGP+KGNG1+KGNHG+KGNND+KGNOG) &&
                     (ix->srcngx != ir->ngx || ix->srcngy != ir->ngy))
                  cryout(RK_E1, "0***SRC NGX,NGY NE TARGET", RK_LN2,
                     errmsg, RK_CCL, NULL);

               /* KGEN=H: Test for same source,target grid divisions */
               if (ix->kgen & KGNHG) {
                  if (jr->Rp.ngridx != ir->Rp.ngridx ||
                      jr->Rp.ngridy != ir->Rp.ngridy)
                     cryout(RK_E1, "0***SRC GRID DIVS NE TARGET",
                        RK_LN2, errmsg, RK_CCL, NULL);
                  }

               /* KGEN=O: Test for valid combination of width and
               *  source size */
               if (ix->kgen & KGNOG && ix->lsg >= (jl->nelt<<1))
                  cryout(RK_E1, "0***LSG > SRC SIZE", RK_LN2,
                     errmsg, RK_CCL, NULL);

               } /* End REPSRC checks */

            if (jsrc == IA_SRC || jsrc == VS_SRC)
               ix->iaxoff = (long)ix->xoff << RP->ky1;
            else {
               ix->iawxsz = (ui32)ix->srcngx;
               ix->iawysz = (ui32)ix->srcngy;
               }

/* Supply a scale if it was defaulted, then adjust the scale actually
*  used according to nc if OLDSCALE.  Changed in V3L to report error
*  if scaled value overflows.  Do after any code that resets nc.
*  Rev, no date,   MC - Remove scale normalization.
*  Rev, 08/19/92, GNR - Add compat with old version. */

            if (RP->compat & OLDSCALE) {
               double dscl = sqrt(32.0/(double)ix->nc);
               if (ix->Cn.scl == SI32_SGN) ix->Cn.scl =
                  (si32)((double)(1<<ix->sclb)*dscl);
               else {
                  dscl *= (double)ix->Cn.scl;
                  if (fabs(dscl) >= dS31)
                     cryout(RK_E1, "0***SCALE MOD OVERFLOWED", RK_LN2,
                        errmsg, RK_CCL, NULL);
                  else ix->Cn.scl = (si32)dscl;
                  }
               }
            else if (ix->Cn.scl == SI32_SGN)
               ix->Cn.scl = (si32)1 << ix->sclb;
            /*** KLUDGE *** Modify by sclmul */
            ix->Cn.scl = mrsrsle(ix->Cn.scl, ix->Cn.sclmul, FBsc,
               E64_ABEXIT);

/* Following code must follow any possible changes to kgen, kam,
*  or kdecay bits.  */

            kamchk(ix);
            if (ix->Cn.kam & KAMFS) ix->cnflgs |= CNDOFS;
            if (ix->Cn.kam & KAMCG) RP->kxr |= ISKAMC;
            RP0->orkgen            |= ix->kgen;
            il->orkam              |= ix->Cn.kam;
            il->ctwk.tree.orkdecay |= ix->Cn.kdecay;

/* Calc number of udev calls per Lij--used to call udevskip
*  if skipping connections, also in PARn to skip locells */

            /* Calc number of udev calls for first Lij */
            ix->uplij = (ix->kgen & (KGNHV|KGNXO|KGNYO|KGNEX)) ? 0 : 1;

            /* Calc number of udev calls per subsequent Lij */
            if (ix->kgen & (KGNIN | KGNPT))
               ix->uplijss = ix->uplij;
            else if (ix->kgfl & KGNIA && ix->kgen & (KGNCF|KGNAN))
               ix->uplijss = 1;
            else if (ix->kgen & (KGNCF|KGNAN)) ix->uplijss = 2;
            else ix->uplijss = 0;

            /* Combine these to give number for all Lij.  Since nc
            *  is nsarb*nsa, lsarb*uplijss cannot possibly overflow */
            {  ui32 lsrep = (ix->Cn.saopt & SAOPTL) ?
                  nsarb : ix->nsa;
               if (!(ix->kgen & KGNAN)) lsrep -= 1;
               ix->uplij += ix->uplijss*lsrep;
               } /* End local lsrep scope */

/* Checks related to TERNARY option.  This is basically test code.
*  Locate the modulatory source and make sure it is TV or PP.
*  We do not look up default background from nmn, as it may change
*  at Group III time.  */

            if (ix->pvpfb) {
               struct TERNARY *pvp = ix->pvpfb;
               struct CONNTYPE *jx;
               int jct;
               /* Access ict'th conntype */
               for (jx=il->pct1,jct=1; jx; jx=jx->pct,jct+=1) {
                  if (jct >= pvp->vpict) break;
                  }
               pvp->qvpict = jx;    /* Save for d3go */
               if (jx->cnsrctyp == TV_SRC) {
                  if (pvp->mximage == 0)
                     ((struct TVDEF *)jx->psrc)->utv.tvsflgs |= TV_GETN;
                  }
               else if (jx->cnsrctyp == PP_SRC) {
                  if (pvp->mximage == 0)
                     ((struct PREPROC *)jx->psrc)->ipsflgs |= PP_GETN;
                  }
               else cryout(RK_E1, "0***TERNARY REQUIRES TV OR PP "
                  "INPUT", RK_LN2, errmsg, RK_CCL, NULL);
               } /* End TERNARY setup */

/* Do checks related to phasing.
*  Copy phase option bits from cnopt to phopt to facilitate AND
*     tests on multiple bits with one instruction in d3go, etc.
*  [Following comments regarding allocation lengths and implementing
*     code should agree with comments in d3go around l. 1850.]
*  Set npafraw as len of phase-dep part of raw detail-print data =
      PHASE_RES+OPhA entries if PHASEJ|PHASER, else 1.
*  Set npaffin as len of final part of det-print data = PHASE_RES
*     entries if PHASEJ|PHASER|PHASCONV, else 1.  (npafraw, npaffin
*     do not include decay term, because can change at Group 3.)
*  Set lrax,lsax to establish lengths of summation arrays in d3go:
*  rax[pn] arrays are used for raw and processed (volt-dep, phase
*     convolution, etc.) sums, so require 1 si64 each for sums
*     ignoring phase plus 32 si64 each if there is any phasing.
*     With PHASU|PHASC, early loops can ignore phase--their length
*     is coded in lrax1.
*  sax[pn] arrays are used for subarbor sums and convolution temps,
*     so they require lrax each for those uses, otherwise not used.
*  R76:  npafraw was expanded to allow unphased affsums before PHASR|
*     PHASJ sums--this simplifies some code in d3go at a small cost
*     in d3dprt.  It is now same as lrax, but kept as a separate
*     variable so can revert if needed later.  */

            ix->phopt = (byte)(tops = ix->Cn.cnopt & NOPPHBITS);
            ix->npafraw = ix->npaffin = ix->lrax = ix->lrax1 = 1;

   /*** THREE LEVELS OF INDENTING SUPPRESSED TO END OF THIS BLOCK ***/

   if (il->phshft) {
      if (ix->Cn.cnopt & NOPMX)
         cryout(RK_E1, "0***INVALID OPT=M WITH PHASE.", RK_LN2,
            errmsg, RK_CCL, NULL);
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
      if (ix->phopt & PHASU && ix->Cn.vdopt & VDANY)
         ix->phopt |= PHASVDU;

      /* Early phase convolution eliminated in R76 mostly because we
      *  want to move mxax testing after volt-dep modification and
      *  things get too complicated with two locations for phase
      *  convolution.  Maybe small speed penalty when nc < 32.  */
      if (ix->Cn.ppcf) ix->phopt |= PHASCNV;

      /* Phase assignment option is now final--set lengths */
      if (ix->phopt & (PHASR|PHASJ))
         ix->lrax1 = (PHASE_RES+OPhA), ix->npafraw = (PHASE_RES+OPhA);
      if (ix->phopt & (PHASJ|PHASR|PHASCNV|PHASVDU))
         ix->npaffin = PHASE_RES;
      if (ix->phopt & PHOPANY)
         ix->lrax = (PHASE_RES+OPhA);
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

   /* Set lsax and mxlax = max(lrax+lsax) for d3nset memory
   *  allocation of CDAT.praffs portion for rax[pn]+sax[pn].  */
   ix->lsax = (ix->Cn.saopt & DOSARB || ix->phopt & PHASCNV ||
      ix->Cn.vdopt & VDANY) ? ix->lrax : 0;
   {  ui16 lax = (ui16)(ix->lrax + ix->lsax);
      if (lax > RP->mxlax) RP->mxlax = lax; }

   }  /* End for loop over ix */

   /*** END SUPPRESSED INDENTING ***/

         /* Record max(nce) for CONNDATA allocation */
         if (il->nce > RP->mxnce) RP->mxnce = il->nce;

/*---------------------------------------------------------------------*
*  Check any INHIBBLK's.  Locate source cells.                         *
*  Make sure source repertoire has same dimensions as target.          *
*  Propagate requested delay back to source, which is always REPSRC.   *
*---------------------------------------------------------------------*/

         for (ib=il->pib1; ib; ib=ib->pib) {

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
            ib->jself = (jl == il);

            ib->pisrc = jl;
            jr = jl->pback;
            if ((ir->ngx != jr->ngx) || (ir->ngy != jr->ngy))
               cryout(RK_E1, "0***SRC,TARGET SIZES DIFFER",
                  RK_LN2, errmsg, RK_CCL, NULL);

            /* Propagate any delay back to source */
            if (ib->ihdelay > 0) {
               ui16 mxihd1 = ib->ihdelay + 1;
               jl->mxsdelay1 = max(jl->mxsdelay1,mxihd1);
               }

            /* Accumulate max(nib) for d3echo */
            if (ib->nib > ir->mxnib) ir->mxnib = ib->nib;
            if (ir->mxnib > RP0->mxnib) RP0->mxnib = ir->mxnib;

            }  /* End ib loop */

/* Set flag used to test for invalid update loops */

         il->ctf |= CTFULCHK;

/* Set flag to request sbar fast start (see d3resetn) */

         if (il->orkam & KAMFS) il->ctf |= CTFDOFS;

/* Count total number of gconn and modulatory types */

         RP0->totgcon += il->ngct;
         RP0->totmodu += il->nmct;

         }  /* End of big for loop over il */

/* End of first pass over cell types--
*  Set final values of nqx and nqy, then layer values lnqx and lnqy.
*  Unlike Darwin II, Darwin III does not change size of repertoire
*  specified by user to make rectangular group boxes.  If this deci-
*  sion is ever changed, please note that several defaults and error
*  messages in the preceding code would have to be postponed until
*  the final values of ngx, ngy, ngrp, and nelt became available.  */

/* nqx is never more than largest nel in any celltype and defaults
*  to sqrt of that number.  */

      if (ir->ncpg == 1) ir->nqx = 1;
      else {
         if (ir->nqx > maxnqx) ir->nqx = maxnqx;
         else if (ir->nqx <= 0)
            ir->nqx = (int)ceilf(sqrtf((float)maxnqx));
         }

/* Now nqy can be calculated.  This is the total number of rows needed
*  to plot one group, given that only nqx cells can go in any one row
*  and a new row is started for each cell type.  The contribution to
*  nqy from each cell type is lnqy.  lnqy,lnqyprev are stored even if
*  kctp=H, may be needed for CIJPLOT etc.
*  R72, 03/09/17, GNR - Input value of nqy is now ignored */

      {  long sumtnqy = 0;
         int tnqy, iln = 0;
         for (jl=ir->play1; jl; jl=jl->play) {
            jl->lnir = iln++;
            jl->lnqx = ir->nqx;  /* For easier lookup */
            jl->lnqyprev = (int)sumtnqy;
            tnqy = (jl->nel + ir->nqx - 1)/ir->nqx;
            jl->lnqy = tnqy;
            if (!(jl->Ct.kctp & CTPHP)) sumtnqy += (long)tnqy;
            }
         if (sumtnqy > (long)INT_MAX) {
            CDAT.cdlayr = jl;
            d3lime("d3tchk", PLcb(PLC_RNQY));
            }
         ir->nqy = (int)sumtnqy;
         } /* End iln, tnqy, sumtnqy local scope */

      } /* End of big for loop over ir */

   RP0->totnlyr = lseqn;      /* Total number of celltypes */
   RP0->totconn = xseqn;      /* Total number of conntypes */

   } /* End d3tchk() */

