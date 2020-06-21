/* (c) Copyright 1991-2010, Neurosciences Research Foundation, Inc. */
/* $Id: d3snsa.c 43 2011-02-24 04:00:37Z  $ */
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
*     (7) Darwin 1 input data                                          *
*  Also allocate the autoscale and BBD effector output area and space  *
*     for the histogram needed by the TV_RESCL option.                 *
*  These components have the common characteristic that they cannot    *
*     be parallelized.  The input areas are filled on the host and     *
*     broadcast to all nodes at the start of each cycle.  In the       *
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
*  V8G, 08/16/10, GNR - Add psclmul (AUTOSCALE)                        *
***********************************************************************/

#define NEWARB          /* Use new EFFARB mechanism */
#define TVTYPE  struct TVDEF
#define PPTYPE  struct PREPROC
#ifdef BBDS
#define BBDTYPE struct BBDDev_t
#endif
#ifdef D1
#define D1TYPE  struct D1BLK
#endif

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "d3global.h"
#include "jntdef.h"
#include "wdwdef.h"
#ifdef D1
#include "d1def.h"
#endif
#ifdef BBDS
#include "bbd.h"
#endif
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
   struct EFFARB   *pea;
   struct VCELL    *pvc;
   struct TVDEF    *cam;
   struct PREPROC  *pip;
   struct IMGTPAD  *pitp;
#ifdef D1
   struct D1BLK    *pd1;
#endif
#ifdef BBDS
   BBDTYPE         *pbbd;
   struct VBDEF    *pvb;
#endif
   long             ibo;      /* Offset in byte bcst array */
   long             ifo;      /* Offset in float bcst array */
   long             iso;      /* Offset in image source array */

/*---------------------------------------------------------------------*
*               Arm/window and float sensory data area                 *
*---------------------------------------------------------------------*/

/* (Originally the arm/window space was allocated only if KGNHV|KGNST
*  options were used.  This was changed 1/30/91 to allocate and broad-
*  cast whenever arms or windows exist, so d3woff can access these
*  data for plotting in d3ijpz and d3rplt.  This was further changed
*  05/05/07 to allocate space here also for float sensory data.)  */

   ifo = RP0->cumnaw + RP0->cumnvfc;
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
         else d3exit(BBD_ERR, "No BBDDev for BBD value", pvb->ivdt);
         }
#endif
      }

/*---------------------------------------------------------------------*
*     Byte input (IA, Sense, TV, PREPROC, D1, Modality) data area      *
*---------------------------------------------------------------------*/

/* Allocate the byte area as a single block for quicker broadcast */

   ibo = iso = 0;
   if (RP->cumbcst | RP0->cumntvi) {
      if (RP->cumbcst) RP->pbcst =
         allocpmv(Shared, RP->cumbcst, "General input area");
      if (RP0->cumntvi) RP0->pipimin =
         mallocv(RP0->cumntvi, "PREPROC image area");

/* As a matter of notational convenience, the name 'pstim' is used
*  for 'pbcst' when the input array is being accessed.  N.B.  The
*  offset of the IA must be 0 or membcst won't be able to find it.
*/

      if (!(RP->CP.runflags & RP_NOINPT)) {
         RP->pstim = (stim_type *)RP->pbcst;
         ibo += (RP->nst + RP->nst);
         }

/* Hand out space for images.  If a camera is not used, the client is
*  so informed and the data are never sent to CNS, so there is no need
*  to allocate buffer space.  */

      for (cam = RP->pftv; cam; cam = cam->pnxtv) {
         char *etxt = cam->hcamnm ?
            getrktxt(cam->hcamnm) : "(unnamed)";
         if (cam->utv.tvsflgs & TV_ON) {
            long ltvim = (long)cam->utv.tvx * (long)cam->utv.tvy *
               (long)max(cam->utv.tvkcol,1);
#ifdef BBDS /* Assignment intended in second 'if' statement */
            if (cam->utv.tvsflgs & TV_BBD) {
               if (pbbd = (BBDTYPE *)cam->utv.ptvsrc) {
                  pbbd->bdflgs |= BBDUsed;
                  pbbd->UD.Cam.pim = RP->pbcst + ibo;
                  }
               else
                  cryout(RK_E1, "0***BBD CAMERA ", RK_LN2,
                     etxt, RK_CCL, " IS NOT SET UP.", RK_CCL, NULL);
               }
#endif
            /* The 'pmyin' pointers are valid only on Node 0 and logic
            *  must be changed if camera input and/or preprocessing is
            *  someday moved to comp nodes.  */
            if (cam->utv.tvsflgs & TV_ONDIR) {
               cam->otvi = ibo;
               cam->pmyin = RP->pbcst + ibo;
               ibo += ltvim;
               }
            else if (cam->utv.tvsflgs & TV_ONPP) {
               cam->otvi = iso;
               cam->pmyin = RP0->pipimin + iso;
               iso += ltvim;
               }
            }
         else
            cryout(RK_P1, "0-->WARNING: Camera ", RK_LN2, etxt, RK_CCL,
               " is not used, input will be discarded.", RK_CCL, NULL);
         }

      for (pip = RP->pfip; pip; pip = pip->pnip) {
         if (pip->ipsflgs & PP_ON) {
            pip->otvi = ibo;
            ibo += (long)pip->nytot * (long)pip->upr.nipx3;
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
         ibo += RP->cumnds;
         }

#ifdef D1
/* Hand out space for Darwin 1 inputs */

      if (RP0->cumnd1 > 0) {
         RP0->od1in = ibo;
         ibo += RP0->cumnd1;
         }
#endif

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

   ifo = RP0->cumnaw;      /* Skip over arm/window data */
   for (pvc=RP0->pvc1; pvc; pvc=pvc->pnvc) {
      if (pvc->vcflags & VKUSED) {
         byte *psar;       /* Ptr for BBD device buffer */
         if (pvc->vcflags & VKR4) {
            psar = (byte *)(RP->paw + (pvc->ovc = ifo));
            ifo += pvc->ncells; }
         else {
            psar = RP->pbcst + (pvc->ovc = ibo);
            ibo += pvc->ncells; }
#ifdef BBDS /* Assignment intended in 'if' statement */
         if (pvc->vtype == USRSRC &&
               (pbbd = (BBDTYPE *)pvc->pvcsrc)) {
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
*                Link image touchpads to their cameras                 *
*---------------------------------------------------------------------*/

   for (pitp=(struct IMGTPAD *)RP0->pvc1; pitp;
         pitp=(struct IMGTPAD *)pitp->vc.pnvc) {
      if (pitp->vc.vtype != ITPSRC) continue;
      pitp->otvi = pitp->pcam->otvi;
      } /* End IMGTPAD list scan */

/*---------------------------------------------------------------------*
*                      Modulation value offsets                        *
*---------------------------------------------------------------------*/

/* Store offsets in MODVAL blocks of their virtual sources,
*  because the VCELL blocks are not replicated on comp nodes.
*  These will be used by d3nset() to locate the cell data.  */

   for (imv=RP->pmvv1; imv; imv=imv->pmdv) {

      switch (imv->mdsrctyp) {

      case IA_SRC:
      case VALSRC:            /* Because not in pbcst array */
         imv->omsrc = 0;
         break;

      case TV_SRC:
         imv->omsrc = ((struct TVDEF *)imv->umds.pgsrc)->otvi;
         break;

      case PP_SRC:
         imv->omsrc = ((struct PREPROC *)imv->umds.pgsrc)->otvi;
         break;

#ifdef D1
      case D1_SRC:
         imv->omsrc = ((struct D1DEF *)imv->umds.pgsrc)->od1in;
         break;
#endif

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

         case TV_SRC:
            ix->osrc = ((struct TVDEF *)ix->psrc)->otvi;
            break;

         case PP_SRC:
            ix->osrc = ((struct PREPROC *)ix->psrc)->otvi;
            break;
#ifdef D1
         case D1_SRC:
            ix->osrc = ((struct D1DEF *)ix->psrc)->od1in;
            break;
#endif
         case VJ_SRC:
         case VW_SRC:
         case VT_SRC:
         case ITPSRC:
            ix->osrc = ((struct VCELL *)ix->psrc)->ovc;
            break;
#ifdef BBDS
         case USRSRC:
            pvc = (struct VCELL *)ix->psrc;
            ix->osrc = pvc->ovc;
            if (ix->kgen & KGNBV)
               ix->osrc += ix->xoff + ix->yoff*pvc->nvx;
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
            struct LMINPUT l1;   /* Local input params */
            float rwh;           /* Reciprocal of map area */
            long  lwid,lhgt;     /* Local width and height */
            ui16  n;             /* Temp for increment calc */
            l1 = ix->ucij.l;
            if (l1.flags & LOPTC) {          /* Target coords */
               lwid = ir->ngx, lhgt = ir->ngy; }
            else if (ix->kgen & KGNST) {     /* Scanned topo */
               lwid = ix->nux, lhgt = ix->nuy; }
            else {                           /* Source coords */
               lwid = ix->srcngx, lhgt = ix->srcngy; }
            lwid -= 1; if (lwid <= 0) lwid = 1;
            lhgt -= 1; if (lhgt <= 0) lhgt = 1;
            n = (l1.flags >> 3) & 3;
            rwh = 1.0/(float)(lwid*lhgt);
            ix->ucij.l2.wc[0] = rwh*(float)l1.c00;
            ix->ucij.l2.wc[1] = rwh*(float)l1.c01;
            ix->ucij.l2.wc[2] = rwh*(float)l1.c11;
            ix->ucij.l2.wc[3] = rwh*(float)l1.c10;
            ix->ucij.l2.csg   = l1.csg;
            ix->ucij.l2.cldx  = lwid;
            ix->ucij.l2.cldy  = lhgt;
            ix->ucij.l2.flags = l1.flags;
            ix->ucij.l2.clinc = (byte)(n ? n : 3);
            } /* End if KGNLM */
         } /* End conntype loop */
      } /* End cell type loop */

/*---------------------------------------------------------------------*
*                            Effector data                             *
*---------------------------------------------------------------------*/

/* Allocate changes array (RP0->pjnwd)--host only.
*  ljnwd = length of changes array (words).  It includes:
*     peff->pbdev->ldat elements for each effector +
*     1 change for each joint (2 for universal joints) +
*     wdw_narb(pw) for each window.  */

   if (RP0->ljnwd > 0) {
      RP0->pjnwd = (float *)allocpcv(Host,
         RP0->ljnwd, sizeof(float), "Changes array");
      for (pea=RP0->pearb1; pea; pea=pea->pneb) {
         if (!pea->peff) d3exit(BBD_ERR, ssprintf(NULL,
            "EFFECTOR %32s NOT SET UP", fmteffnm(pea)), 0);
         switch (pea->keff) {
         case KEF_ARM:
         case KEF_UJ:
            ((JOINT *)pea->peff)->ojnwd = pea->nchgs ?
               pea->jjnwd[EXCIT] : LONG_MAX;
            break;
         case KEF_WDW: {
            WINDOW *pw = (WINDOW *)pea->peff;
            if (pea->nchgs == 0) {
               pw->kchng = WDWZIP;
               pw->ojnwd = LONG_MAX; }
            else
               pw->ojnwd = pea->jjnwd[EXCIT];
            break;
            }
#ifdef BBDS
         case KEF_BBD:
            pbbd = (BBDTYPE *)pea->peff;
            pbbd->bdflgs |= BBDUsed;
            pbbd->UD.Eff.pmno = RP0->pjnwd + pea->jjnwd[EXCIT];
            break;
#endif
            } /* End keff switch */
         } /* End effector loop */
      } /* End jnwd allocation */

/*---------------------------------------------------------------------*
*                      Image rescaling histogram                       *
*---------------------------------------------------------------------*/

   if (RP0->n0flags & N0F_TVRSCL) {
      RP0->ptvrsh = (si32 *)mallocv((MAXGRAY+1)*I32SIZE,
         "Rescale histo");
      /* A totally arbitrary seed for random rounding--
      *  not worth an input parameter */
      if (RP0->ptvrsh) RP0->ptvrsh[MAXGRAY] = RS_SEED;
      }

/*---------------------------------------------------------------------*
*              Scale factors resulting from autoscaling                *
*---------------------------------------------------------------------*/

   if (RP0->nautsc) {
      int ias;
      RP->psclmul = (si32 *)allocpcv(Shared, RP0->nautsc, IXsi32,
         "Autoscales");
      /* Initialize all scale multipliers to 1.0 (S24) */
      for (ias=0; ias<RP0->nautsc; ++ias)
         RP->psclmul[ias] = S24;
      }

   } /* End d3snsa() */
