/* (c) Copyright 1991-2018, The Rockefeller University *11115* */
/* $Id: d3snsa.c 78 2018-08-02 18:36:58Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                               d3snsa                                 *
*                                                                      *
*  Allocate in shared memory the general input areas, which contain:   *
*     (1) arm/window positions (parallel only)                         *
*     (2) value array (VDTDEF blocks)                                  *
*     (3) input array                                                  *
*     (4) television images and preprocessed images                    *
*     (5) virtual cells for all kinds of VCELL blocks                  *
*     (6) bits marking stimuli present in each modality                *
*     (7) si32 data, currently image normalization + NOPVP info        *
*  Also allocate the autoscale and BBD effector output area and space  *
*     for the histogram needed by the TV_RESCL option.                 *
*  These components have the common characteristic that they cannot    *
*     be parallelized.  The input areas are filled on the host and     *
*     broadcast to all nodes at the start of each trial.  In the       *
*     serial version, the area still exists, but there is no broad-    *
*     cast and the arm/window positions are therefore not included.    *
*  Allocate the arm/window and value data areas separately, as they    *
*     have their own types.  This allows automatic membcst handling.   *
*  The length of each area is calculated either here or in d3tchk.     *
*     These length variables, whose names begin with 'cum', are also   *
*     used to control calling the various input routines, because      *
*     they will reflect the fact that input is not needed if the       *
*     particular input class is not referred to by any conntype.       *
*  Also replace KGEN=L coeffs c00, c01 etc. with normalized floats.    *
*     (This must occur after d3echo but before first broadcast.)       *
*  The offset to each data area is stored in MODVAL and CONNTYPE       *
*     blocks as needed to facilitate locating the input data.          *
*  In the case of BBD inputs, also store pointers to the data arrays   *
*     in the BBDDev structures.                                        *
*  This routine is called only on host node.                           *
************************************************************************
*  V5C, 12/10/91, GNR - Initial version, built from d3tchk and main    *
*  V6A, 07/24/93, GNR - Add platform sensors                           *
*  V7B, 08/20/94, GNR - Add adjustment of c00, c01, etc. for KGNLM     *
*  V8A, 04/15/95, GNR - Handle VJ,VW,VT as VCELLs                      *
*  Rev, 04/19/97, GNR - Include value blocks in general broadcast      *
*  V8B, 12/10/00, GNR - Move to new memory management routines         *
*  V8D, 02/23/07, GNR - Deal with BBDs                                 *
*  ==>, 07/27/07, GNR - Last mod before committing to svn repository   *
*  V8F, 02/15/10, GNR - Remove conditionals on TV inputs, PLATFORM     *
*  Rev, 04/22/10, GNR - Add image preprocessors                        *
*  Rev, 12/07/12, GNR - Convert ns[xy], ku[xy] to ui16, nst to ui32    *
*  R64, 12/09/15, GNR - Move pjnwd allocation to d3news                *
*  R65, 01/02/16, GNR - n[gq][xyg] to ui16, ngrp to ui32, ncpg to ubig *
*  R66, 03/11/16, GNR - Rename nvx->nsxl, nvy->nsya, add oltvy setting *
*  R67, 04/27/16, GNR - Remove Darwin 1 support                        *
*  R74, 06/08/17, GNR - Changes to allow for 2-byte gray,color values  *
*  Rev, 07/14/17, GNR - Add KGNLM LOPTW Cij generating method          *
*  Rev, 07/26/17, GNR - Remove copying cam->otvi to IMGTPAD otvi       *
*  Rev, 09/09/17, GNR - Add cumsi32 data, cam->otvmx, pip->oppmx       *
*  R75, 09/21/17, GNR - Add space for osfmx (NOPVP Sj method)          *
*  R78, 04/10/18, GNR - Keep entered tvr[xy]e across images            *
*  R78, 04/24/18, GNR - Move mdincr,mdnel setting here from d3tchk     *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "d3global.h"
#include "armdef.h"
#include "jntdef.h"
#include "tvdef.h"
#include "rocks.h"
#include "rkxtra.h"

/*=====================================================================*
*                               d3snsa                                 *
*=====================================================================*/

void d3snsa(void) {

/* Local Declarations */

   struct REPBLOCK *ir;
   struct CELLTYPE *il;
   struct CONNTYPE *ix;
   struct MODVAL   *imv;
   struct VCELL    *pvc;
   struct TVDEF    *cam;
   struct PREPROC  *pip;
#ifdef BBDS
   BBDDev          *pbbd;
   struct VBDEF    *pvb;
#endif
   size_t           ibo;      /* Offset in byte bcst array */
   size_t           ifo;      /* Offset in float bcst array */
   size_t           iso;      /* Offset in image source array */
   size_t           ijo;      /* Word offset in si32 bcst array */

/*---------------------------------------------------------------------*
*               Arm/window and float sensory data area                 *
*---------------------------------------------------------------------*/

/* (Originally the arm/window space was allocated only if KGNHV|KGNST
*  options were used.  This was changed 1/30/91 to allocate and broad-
*  cast whenever arms or windows exist, so d3woff can access these
*  data for plotting in d3ijpz and d3rplt.  This was further changed
*  05/05/07 to allocate space here also for float sensory data.)  */

   ifo = (size_t)(RP0->cumnaw + RP0->cumnvfc);
   if (ifo) RP->paw = (armwdw_type *)allocpcv(Shared,
      ifo, IXarmwdw_type, "Position & sense data");

/*---------------------------------------------------------------------*
*                           Value data area                            *
*---------------------------------------------------------------------*/

   if (RP->nvblk > 0) {       /* Value data array */
      RP->pvdat = (struct VDTDEF *)allocpcv(Shared,
         RP->nvblk, IXstr_VDTDEF, "Value data");
#ifdef BBDS
      /* For any value that is from a BBD, the BBDDev block needs
      *  a pointer to the actual data.  */
      for (pvb = RP0->pvblke; pvb; pvb=pvb->pnxvt)
            if (pvb->kval == VB_BBD) {
         if (pbbd = pvb->u1.vbbd.pbvdev) {   /* Assignment intended */
            pbbd->bdflgs |= BBDUsed;
            pbbd->UD.Val.pval = &pvb->u1.vbbd.fbbdval;
            }
         else d3exit("No BBDDev for BBD value", BBD_ERR, pvb->ivdt);
         }
#endif
      }

/*---------------------------------------------------------------------*
*                           si32 data area                             *
*---------------------------------------------------------------------*/

   if (RP0->nbsi32 > 0) RP->pjdat = (si32 *)allocpcv(Shared,
      RP0->nbsi32, IXjdat_type, "Numeric data");
   ijo = 0;

/*---------------------------------------------------------------------*
*       Byte input (IA, Sense, TV, PREPROC, Modality) data area        *
*---------------------------------------------------------------------*/

/* Allocate the byte area as a single block for quicker broadcast */

   ibo = iso = 0;
   if (RP->cumbcst | RP0->cumntvi) {
      if (RP->cumbcst) RP->pbcst =
         allocpmv(Shared, RP->cumbcst, "General input area");
      /* pipimin is currently only needed here in d3snsa, but is kept
      *  in RP0 block in case other items get added to this space.  */
      if (RP0->cumntvi) RP0->pipimin =
         mallocv(RP0->cumntvi, "PREPROC image area");

/* As a matter of notational convenience, the name 'pstim' is used
*  for 'pbcst' when the input array is being accessed.  N.B.  The
*  offset of the IA must be 0 or membcst won't be able to find it.
*/

      if (!(RP->CP.runflags & RP_NOINPT)) {
         RP->pstim = (stim_type *)RP->pbcst;
         ibo += (size_t)RP->nst << 1;
         }

/* Hand out PAR0 space for images (PARn setup is in d3nset).  Sizes
*  are now based on tvr[xy]e, which user should make large enough for
*  largest image expected in run. If a camera is not used, the client
*  (if BBD) is so informed and the data are never sent to CNS, so
*  there is no need to allocate buffer space.
*  Define:  ltvrxy = tvrxe*tvrye*tvipxsz, ltvixy (after space avg) =
*     (tvrxe/tvsa)*(tvrye/tvsa)*tvopxsz, ltvxy = tvx*tvy*tvopxsz.
*  Camera input space:  This is always ltvrxy bytes.  If this is
*     larger than ltvixy (because d3imgt is doing TV_Reduc or
*     TV_SpAvg), camera reads into psws, reduces image into normal
*     input space at pmyim.  Otherwise, d3imgt just does everything
*     in normal input space.  FIAT:  Always test TV_Reduc|TV_SpAvg
*     not ltvrxy > ltvixy.
*  Camera output space:
*     The decision whether or not TV_IFit (resize or crop) is needed
*     is now made at d3imgt time after new image size is known.
*     Complex code formerly located here to decide whether pmyim
*     space could be shared with pbcst+otvi space is no longer tenable.
*     Instead, otvi space is always allocated if TV_ONDIR is set, otvg
*     space if TV_ONPPGC is set.  pmyim space is always allocated of
*     size ltvixy, which should hold the largest expected image after
*     any TV_Reduc or TV_SpAvg.
*  Special weird case:  TV_IFit could be done only to make input for
*     TV_GfmC and not for TV_ONDIR.  In this case, there would be no
*     pbcst+otvi space.  Instead, this space is added to cumntvi,
*     allocated in the pipimin space, and the offset stored in otvi.
*  NOPVP space:  For each unique value of nel, si32 space for each
*     STGH type preprocessor output to store for each pixel the
*     highest preprocessor output and angle at which it occurs.
*     Design Decision:  I could have kept track of which of the
*        various unique nel are used by conntypes that access each
*        STGH preprocessor and allocated and computed only those,
*        but it is very likely that more than one unique nel will
*        never occur, so this code would likely never be used.
*        The current version is designed to give correct answers
*        although with this unlikely redundancy.
*  Space (one si32) is allocated for max image data for conntype
*     normalization whether needed or not, thus could be turned on
*     in later trial series and is too small to bother turning off.
*/

      for (cam = RP->pftv; cam; cam = cam->pnxtv) {
         struct UTVDEF *putv = &cam->utv;
         char *etxt = cam->hcamnm ?
            getrktxt(cam->hcamnm) : "(unnamed)";
         if (putv->tvsflgs & TV_ON) {
            int ncol = putv->tvncol;
#ifdef BBDS /* Assignment intended in second 'if' statement */
            if (putv->tvsflgs & TV_BBD) {
               if (pbbd = (BBDDev *)putv->ptvsrc) {
                  pbbd->bdflgs |= BBDUsed;
                  }
               else
                  cryout(RK_E1, "0***BBD CAMERA ", RK_LN2,
                     etxt, RK_CCL, " IS NOT SET UP.", RK_CCL, NULL);
               }
#endif
            /* Activate gray-from-color if somebody wants it */
            if (putv->tvsflgs & TV_ONGfmC) cam->tviflgs |= TV_GfmC;
            if (putv->tvsflgs & TV_ONPPGC) cam->tviflgs |= TV_PPGC;

            /* Alocate space to broadcast final-size
            *  image to processor (or serial) nodes.  */
            if (putv->tvsflgs & TV_ONDIR) {
               cam->otvi = ibo;
               ibo += putv->ltvxy;
               cam->otvmx = ijo++;
               }
            /* Weird case documented above */
            else if (putv->tvsflgs & TV_ONGfmC) {
               cam->otvi = iso;
               iso += putv->ltvxy;
               }
            /* Gray-from-color going to networks */
            if (putv->tvsflgs & TV_ONGfmC) {
               cam->otvg = ibo;
               ibo += putv->ltvxy/ncol;
               }
            /* Gray-from-color going to preprocessors or vtouch */
            if (putv->tvsflgs & TV_ONPPGC) {
               cam->otvppgc = iso;
               iso += putv->ltvixy/ncol;
               }
            /* Intermediate image space always allocated, see above */
            cam->pmyim = RP0->pipimin + iso;
            iso += putv->ltvixy;
            }
         else
            cryout(RK_P1, "0-->WARNING: Camera ", RK_LN2, etxt, RK_CCL,
               " is not used, input will be discarded.", RK_CCL, NULL);
         }

      for (pip = RP->pfip; pip; pip = pip->pnip) {
         if (pip->ipsflgs & PP_ON) {
            pip->oppoi = ibo;
            ibo += (size_t)(HSIZE*pip->upr.nker)*(size_t)pip->upr.lppi3;
            if (pip->ipsflgs & PP_GETP) {
               pip->osfmx = ijo;
               ijo += (size_t)RP->nuqnel*(size_t)pip->upr.lppi3;
               }
            pip->oppmx = ijo;
            ijo += RP->nuqnel;
            }
         else
            cryout(RK_P1, "0-->WARNING: Preprocessor ", RK_LN2,
               getrktxt(pip->hipnm), RK_CCL,
               " is not used.", RK_CCL, NULL);
         }

/* If sensory modalities have been defined for categorization
*  statistics, allocate space for broadcasting stimulus ids.  */

      if (RP->cumnds > 0) {
         RP->ombits = ibo;
         ibo += (size_t)RP->cumnds;
         }

      } /* End if cumbcst or cumntvi */


/*---------------------------------------------------------------------*
*    Allocate for sense data (bytes in cumbcst, floats in cumnaw)      *
*---------------------------------------------------------------------*/

/* If virtual sense cells are needed for input to connections or just
*  for plotting, allocate space for them now.  Note that for unused
*  devices communicating via sockets, one cannot just skip the data
*  with a seek, so instead a flag is sent to the client in bbdschk()
*  and the client omits sending CNS the data.  (Non-BBD sense blocks
*  are removed in d3tchk if not used.)  */

   ifo = (size_t)RP0->cumnaw; /* Skip over arm/window data */
   for (pvc=RP0->pvc1; pvc; pvc=pvc->pnvc) {
      if (pvc->vcflags & VKUSED) {
         byte *psar;          /* Ptr for BBD device buffer */
         if (pvc->vcflags & VKR4) {
            psar = (byte *)(RP->paw + (pvc->ovc = ifo));
            ifo += (size_t)pvc->nvcells; }
         else {
            psar = RP->pbcst + (pvc->ovc = ibo);
            ibo += (size_t)pvc->nvcells; }
#ifdef BBDS /* Assignment intended in 'if' statement */
         if (pvc->vtype == USRSRC &&
               (pbbd = (BBDDev *)pvc->pvcsrc)) {
            pbbd->bdflgs |= BBDUsed;
            pbbd->UD.Sns.psa = psar;
            pbbd->UD.Sns.pisgn = &pvc->isn;
            }
#endif
         if (!pvc->pvcsrc)
            cryout(RK_E1, "0***", RK_LN2+4, fmtvcn(pvc), RK_CCL,
               " IS USED BUT HAS NO SOURCE.", RK_CCL, NULL);
         }
      else if (pvc->pvcsrc) {
         cryout(RK_P1,"0-->WARNING: ", RK_LN2, fmtvcn(pvc), RK_CCL,
            " is not used, input will be discarded.", RK_CCL, NULL);
         }
      } /* End virtual sense loop */


/*---------------------------------------------------------------------*
*                      Modulation value offsets                        *
*---------------------------------------------------------------------*/

/* Store offsets (not ptrs) in MODVAL blocks of their virtual sources,
*  because the VCELL blocks are not replicated on comp nodes.
*  These will be used by d3nset() to locate the cell data.  */

   for (imv=RP->pmvv1; imv; imv=imv->pmdv) {

      switch (imv->mdsrctyp) {

      case IA_SRC:
      case VALSRC:            /* Because not in pbcst array */
      case VS_SRC:
         imv->omsrc = 0;
         break;

      case TV_SRC: {
         struct TVDEF *cam = (struct TVDEF *)imv->umds.pgsrc;
         imv->omsrc = (imv->Mdc.mvxc.tvcsel & EXGfmC) ?
            cam->otvg : cam->otvi;  }
         break;

      case PP_SRC:
         imv->omsrc = ((struct PREPROC *)imv->umds.pgsrc)->oppoi;
         break;

      case VT_SRC:
      case ITPSRC:
#ifdef BBDS
      case USRSRC:
#endif
         imv->omsrc = ((struct VCELL *)imv->umds.pgsrc)->ovc;
         break;

         } /* End srcflag switch */
      imv->umds.pgsrc = NULL;
      } /* End imv loop */

/*---------------------------------------------------------------------*
*                Specific connection type data offsets                 *
*---------------------------------------------------------------------*/

/* Store pbcst offsets of VG or TV sources in ix->osrc for use on comp
*  nodes, because the VCELL blocks where some of the items are located
*  are not duplicated on comp nodes.  (Ptrs cannot be stored because
*  of membcst restriction on pointers into a block.  This code must
*  come after the assignment of sensor space done above.  Hence, it
*  was moved here from d3genr, at the cost of an extra set of il,ix
*  loops.  Consolidated from d3allo and pvtop abolished, 11/18/91,
*  GNR.)  Code to set wc00, etc.  for KGNLM added here in V7B.
*  Normally, user data are not written over, but CONNTYPE space keeps
*  getting larger...  */

   for (il=RP->pfct; il; il=il->pnct) {
      ir = il->pback;
      for (ix=il->pct1; ix; ix=ix->pct) {

         switch (ix->cnsrctyp) {

         case TV_SRC: {
            struct TVDEF *cam = (struct TVDEF *)ix->psrc;
            ix->osrc = ix->cnxc.tvcsel & EXGfmC ?
               (ulng)cam->otvg : (ulng)cam->otvi;
            ix->omxpix = cam->otvmx;
            break;
            }

         case PP_SRC: {
            /* Code to set rppc moved here from d3cpli to avoid need
            *  for access to PREPROC struct from that code.  */
            struct PREPROC *pip = (struct PREPROC *)ix->psrc;
            ix->osrc = (ulng)pip->oppoi;
            ix->omxpix = pip->oppmx;
            /* Have to find where nel for this conntype is in the
            *  unique list to get the correct osfmx from this pip.
            *  I just do a linear search because there will rarely
            *  be more than one of these.  */
            if (ix->Cn.cnopt & NOPVP) {
               struct CELLTYPE *jjl;
               size_t myosfmx = pip->osfmx;
               for (jjl=RP->pfct; jjl->sfnel; jjl=jjl->pnct) {
                  if (il->nel == jjl->sfnel) break;
                  myosfmx += (size_t)pip->upr.lppi3;
                  }
               ix->cnosfmx = myosfmx;
               }
            ix->rppc = (ui32)pip->kpc*pip->upr.nppy;
            break;
            }

         case VT_SRC: {
            /* ix->olvty is only used in d3vplt and d3lplt to locate
            *  source touchpad in multijoint arm sequence.  This is
            *  the only place to put this code that does not require
            *  an extra loop over conntypes.  */
            struct VCELL *pvc = (struct VCELL *)ix->psrc;
            struct JNTDEF *pj = pvc->pjnt0 + ix->srcnm.hjnt;
            ix->osrc = (ulng)pvc->ovc + pj->u.jnt.tnvc;
            if (pvc->vcflags & VKR4) ix->cnflgs |= CNVKR4;
            ix->olvty = pj->u.jnt.olvty;
            break;
            }

         case VJ_SRC:
         case VW_SRC:
         case ITPSRC:
            pvc = (struct VCELL *)ix->psrc;
            ix->osrc = (ulng)pvc->ovc;
            if (pvc->vcflags & VKR4) ix->cnflgs |= CNVKR4;
            break;
#ifdef BBDS
         case USRSRC:
            pvc = (struct VCELL *)ix->psrc;
            ix->osrc = (ulng)pvc->ovc;
            if (ix->kgen & KGNBV)
               ix->osrc += (ulng)(ix->xoff + ix->yoff*pvc->nsxl);
            if (pvc->vcflags & VKR4) ix->cnflgs |= CNVKR4;
            break;
#endif
            } /* End cnsrctyp switch */

         /* If KGNLM, determine width (cldx) and height (cldy) of map
         *  and replace input coefficients with scaled floats.  As of
         *  V8C, these are given permanent space in ucij and need not
         *  be recalculated when needed.  LMINPUT struct is copied to
         *  a temp in case lengths of variables do not align on some
         *  crazy machine.  This code revised 07/31/94 and 8/20/94,
         *  GNR.  Previous versions stored gradient/range--now store
         *  just range as needed for bilinear interpolation. If range
         *  is null, set to 1 so interpolation will work.  If using
         *  source coords, pick up nux,nuy if KGNST, otherwise
         *  srcngx,srcngy.  Pick up my ngx,ngy if using target cell
         *  coords.  Choice was previously undocumented.  */
         if (ix->kgen & KGNLM) {
            struct LMINPUT l1 = ix->ucij.l;  /* Copy input params */
            float rwh;           /* Reciprocal of map area */
            si32  lwid,lhgt;     /* Local width and height */
            ui16  n;             /* Temp for increment calc */
            ix->ucij.l2.flags = l1.flags;
            if (l1.flags & LOPTC) {       /* Target coords */
               lwid = (si32)ir->ngx, lhgt = (si32)ir->ngy; }
            else if (ix->kgen & KGNST) {  /* Scanned topo */
               lwid = ix->nux, lhgt = ix->nuy; }
            else {                        /* Source coords */
               lwid = (si32)ix->srcngx, lhgt = (si32)ix->srcngy; }
            lwid -= 1; if (lwid <= 0) lwid = 1;
            lhgt -= 1; if (lhgt <= 0) lhgt = 1;
            n = (l1.flags >> 3) & 3;
            rwh = 1.0/(float)((long)lwid*(long)lhgt);
            ix->ucij.l2.wc[0] = rwh*(float)l1.c00;
            ix->ucij.l2.wc[1] = rwh*(float)l1.c01;
            ix->ucij.l2.wc[2] = rwh*(float)l1.c11;
            ix->ucij.l2.wc[3] = rwh*(float)l1.c10;
            ix->ucij.l2.csg   = l1.csg;
            ix->ucij.l2.cldx  = lwid;
            ix->ucij.l2.cldy  = lhgt;
            ix->ucij.l2.clinc = (byte)(n ? n : 3);
            } /* End if KGNLM */
         } /* End conntype loop */
      } /* End cell type loop */

/*---------------------------------------------------------------------*
*                      Image rescaling histogram                       *
*---------------------------------------------------------------------*/

   {  ui32 lptvrsi = (ui32)RP0->mxtvixy*sizeof(struct Fif);
      if (RP0->n0flags & N0F_TVRSCL) {
         ui32 lrsh = NRSHIST*JSIZE;
         if (RP0->n0flags & N0F_TVRSCC) lrsh *= NColorDims;
         if (lrsh > lptvrsi) lptvrsi = lrsh; }
      if (lptvrsi > 0) RP0->ptvrsi =
         mallocv((size_t)lptvrsi, "Rescale info");
      }

   } /* End d3snsa() */
