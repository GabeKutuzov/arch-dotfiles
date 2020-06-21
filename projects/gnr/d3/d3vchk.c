/* (c) Copyright 1986-2018, The Rockefeller University *21115* */
/* $Id: d3vchk.c 78 2018-08-02 18:36:58Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                               d3vchk                                 *
*                                                                      *
*  Initialization and error checking for values, senses, arms, and     *
*  windows and calculation of memory space for broadcast that must     *
*  happen on every trial (inputs and values).  This Code should run    *
*  just after the network tree checks in d3tchk() but removed from     *
*  that routine (R75) just to make it more focussed                    *
*                                                                      *
************************************************************************
*  V1A, 01/25/86, G. N. Reeke - d3tchk split out from d3tree           *
*  R75, 09/21/17, GNR - Add nbsi32 space for PP_GETP, f[st]ainc        *
*  R75, 10/01/17, GNR - Split out from d3tchk                          *
*  R78, 04/10/18, GNR - Always allocate for image cropping             *
***********************************************************************/

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
#include "tvdef.h"
#include "swap.h"
#include "rocks.h"
#include "rkxtra.h"
#include "rkarith.h"

void d3vchk(void) {

/* Local Declarations */

   struct ARMDEF   *pa;
   struct JNTDEF   *pj,*pje;
   struct WDWDEF   *pw;
   struct VCELL    *pvc,**ppvc;
   struct VBDEF    *ivb;
   struct TVDEF    *cam;
   struct PREPROC  *pip;
   int  jarms, jjnts, jwdws;     /* Arm, joint, window counters */
   int  nj, nje, njg;            /* Numbers of joints */

/*---------------------------------------------------------------------*
*     Check and initialize value schemes.                              *
*     Calculate starting Y coordinate for value label and bars.        *
*---------------------------------------------------------------------*/

   for (ivb=RP->pvblk; ivb; ivb=ivb->pnxvb) {
      int kv = ivb->kval;
      int kid = ivb->vbkid;
      int ia,iw;                    /* Arm and window indices */
      int ivx;                      /* Error message selector */

      /* Table of value source items used for error messages */
      static char *BVIMsg[] = { "arm", "window" };

      /* Perform checking specific to type of value scheme */
      switch (kid) {

      case VBK_REP:              /* Repertoire-type value */
         if ((ivb->u1.vrp.pvsrc = findln(&ivb->valnm)) == NULL)
            cryout(RK_E1, "0***", RK_LN2+4, fmtrlnm(&ivb->valnm),
               LXRLNM, " NOT FOUND FOR VALUE SCHEME.",RK_CCL, NULL);
         /* Using /2 to avoid SRA(,1) */
         ivb->u1.vrp.omvbd2 = ((1<<FBvl) - ivb->vbase)/2;
         break;

      case VBK_DIST:             /* Distance-type value */
         wbcdin(ivb->valnm.lnm, &iw, VSBIC);
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
         wbcdin(ivb->valnm.rnm, &ia, VSBIC);
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
      if ((pw->wwd > (float)RP->nsx) ||
            (pw->wht > (float)RP->nsy)) {
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
*        Compute nvcells and VT nsya (added in R66, 03/18/16, GNR).    *
*           (VT nsya is total height of used joints to allow touchpads *
*           from one arm to be plotted in a column in d3lplt,d3vplt.)  *
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
         if (pvc->vtype == VT_SRC) {
            ui16 jbit = 1;
            pa = (struct ARMDEF *)pvc->pvcsrc;
            pje = pa->pjnt + pa->njnts;
            pvc->nsya = 0;
            for (pj=pa->pjnt; pj<pje; jbit<<=1,++pj) {
               if (pvc->jntinVT & jbit) {
                  pj->u.jnt.olvty = pvc->nsya;
                  pvc->nsya += pj->u.jnt.nta;
                  pj->u.jnt.tnvc = pvc->nvcells;
                  pvc->nvcells +=
                     (ui32)pj->u.jnt.nta * (ui32)pj->u.jnt.ntl;
                  }
               }
            }
         else {         /* Not VT_SRC */
            pvc->nvcells = (ui32)pvc->nsya * (ui32)pvc->nsxl;
            }
         if (pvc->nvcells <= 0)
            cryout(RK_E1, "0***", RK_LN2+4, fmtvcn(pvc), RK_CCL,
               " HAS NO CELLS.", RK_CCL, NULL);
         else if (pvc->vcflags & VKR4) RP0->cumnvfc += pvc->nvcells;
         else                          RP0->cumnvbc += pvc->nvcells;
         /* Usage marking at sources */
         switch (pvc->vtype) {
         case VJ_SRC:
            ((struct ARMDEF *)(pvc->pvcsrc))->dovjtk = TRUE;
            break;
         case VT_SRC:
            RP0->n0flags |= N0F_VTCHSU;
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
*         Count space needed for TV and PREPROC input arrays.          *
*---------------------------------------------------------------------*/

   /* Count cumulative TV image space and calculate plot height to
   *  preserve aspect ratio--can be zero-pass loop.  Set usage flags
   *  in kevtb, ortvfl to simplify special event trial controls.  */
   for (cam = RP->pftv; cam; cam = cam->pnxtv) {
      struct UTVDEF *putv = &cam->utv;
      int ncol;
      if (!(putv->tvsflgs & TV_ON)) continue;
      /* Set tvsflgs corresponding to combinations of frequency and
      *  interface options for active cameras only for faster tests in
      *  main trial loop.  CAUTION:  This code depends on the numeric
      *  bit definitions.  */
      putv->tvsflgs |= ((putv->tvsflgs &
         (TV_VVTV|TV_BBD|TV_UPGM)) << (3*putv->tvkfreq));
      if (putv->tvkfreq == UTVCFr_TRIAL) {
         if (putv->tvsflgs & TV_UPGM) RP->kevtb |= EVTUTV;
#ifdef BBDS
         if (putv->tvsflgs & TV_BBD)  RP->kevtb |= EVTBTV;
#endif
#ifdef VVTV
         if (putv->tvsflgs & TV_VVTV) RP->kevtb |= EVTVTV;
#endif
         }

      /* Note:  Space for raw images that are being reduced will be
      *  put in the CDAT.psws space by d3nset.  RP->pjdat includes
      *  space for normalization info that is created whether needed
      *  or not but only once even if multiple conntypes need to use
      *  it (so not added in ix loop above).  */
      ncol = (int)putv->tvncol;
      if (putv->tvsflgs & TV_ONDIR) {
         /* Space for passing reduced images to network */
         RP0->cumntv += putv->ltvxy;
         RP0->nbsi32 += 1;
         }
      /* Crazy special case:  TV_ONGfmC but no TV_ONDIR--put the
      *  fit output in RP0->pipimin space, not broadcast.  */
      else if (putv->tvsflgs & TV_ONGfmC)
         RP0->cumntvi += putv->ltvxy;
      /* Gray-from-color output for broadcast to networks */
      if (putv->tvsflgs & TV_ONGfmC)
         RP0->cumntv += putv->ltvxy/ncol;
      /* And another chunk if TV_ONPPGC (gray-from-color for
      *  input to preprocessor or virtual touch).  */
      if (putv->tvsflgs & TV_ONPPGC)
         RP0->cumntvi += putv->ltvixy/ncol;
      /* We now allocate space for spatially averaged image all the
      *  time, using max size computed in d3g1imin, because we can-
      *  not know until d3imgt whether it is needed or not.  */
      RP0->cumntvi += putv->ltvixy;
      RP->ortvfl |= putv->tvsflgs;
      } /* End camera loop */

   for (pip = RP->pfip; pip; pip = pip->pnip) {
      if (!(pip->ipsflgs & PP_ON)) continue;
      /* Preprocessor output bytes per image */
      RP0->cumntv += (long)(HSIZE*pip->upr.nker)*(long)pip->upr.lppi3;
      RP0->nbsi32 += RP->nuqnel;
      if (pip->ipsflgs & PP_GETP)
         RP0->nbsi32 += (ulng)RP->nuqnel*(ulng)pip->upr.lppi3;
      {  ui16 lkpc = pip->upr.nker;    /* Will be kernels/column */
         ui16 ncol = (ui16)pip->dcols; /* Number of plot columns */
         if (pip->upr.ipuflgs & PP_2SYMM) lkpc <<=1;
         if (ncol > 1) lkpc = (lkpc + ncol - 1)/ncol;
         pip->kpc = lkpc;
         } /* End lkpc,ncol local scope */
      } /* End preprocessor loop */

/*---------------------------------------------------------------------*
*  There are now three general areas to hold for Shared broadcast byte,*
*     si32, and float inputs plus a dedicated area for VDTDEF blocks.  *
*     This allows automatic endian and real format conversions by mem- *
*     bcst as needed.  The byte area (length in cumbcst) has IA, TV,   *
*     PP, and byte senses.  The si32 area (nbsi32) currently has only  *
*     max pixel values for conntype input normalization.  The float    *
*     area has arm/window locations (cumnaw, parallel only) and float  *
*     sense (cumnvfc) data.                                            *
*---------------------------------------------------------------------*/

   /* Store space for input array only if it exists */
   if (!(RP->CP.runflags & RP_NOINPT))
      RP->nst = (ui32)RP->nsx*(ui32)RP->nsy;

   /* Count cumulative arm/window position broadcast space.
   *  N.B.  If serial, cumnaw exists but is 0 so d3echo can print */
#ifdef PAR
   RP0->cumnaw = RP->narms*BARM_VALS + RP->nwdws*BWDW_VALS;
#endif

   /* Count cumulative VDTDEF (value) block broadcast space */
   RP0->cumval = RP->nvblk*sizeof(struct VDTDEF);

   /* Count cumulative pixel/sense broadcast space */
   RP->cumbcst = RP0->cumnvbc + RP0->cumntv + RP->nst + RP->nst;

   } /* End d3vchk() */

