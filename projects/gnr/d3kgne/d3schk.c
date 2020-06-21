/* (c) Copyright 1995-2011, Neurosciences Research Foundation, Inc. */
/* $Id: d3schk.c 51 2012-05-24 20:53:36Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                              d3schk.c                                *
*                                                                      *
*     Postprocessing of effector arbors, VCELL blocks for senses,      *
*             and PREPROC blocks for image preprocessors               *
*                    (after d3grp2, before d3tchk)                     *
*                                                                      *
*  This routine performs the following actions:                        *
*  (1) Append list of BBD senses to regular senses.                    *
*  (2) Mark senses as used if requested for SIMDATA output.            *
*  (3) Give an error for any VJ or VT blocks that do not reference an  *
*      ARMDEF and for any VW blocks that do not reference a WDWDEF.    *
*      (These are from supernumerary SENSE cards or any failure in the *
*      d3grp2 mechanisms that check before creating new ones. An error *
*      is given even if the sense is not connected to anything, as a   *
*      user error or misunderstanding is probable.)                    *
*  (4) If nvx,nvy parameter not yet set, fill in appropriate default.  *
*      Give an error for any VJ or VW blocks that do not have 'nvx' at *
*      least 2, otherwise intercell interval is not defined in d3vjnt. *
*      These checks done here to catch errors in SENSE or ARM/WINDOW   *
*      cards in whatever order they may have been entered.             *
*  (5) If cpt parameter not yet set, set from global cpt.              *
*  (6) Calculate nvytot for all senses.  This is same as nvy, except   *
*      for the special case of VT (touch) blocks, where it is equal    *
*      to nvy times the number of joints that have nonzero touch pads. *
*      Fill in default tla where it was not entered.  (This is not     *
*      technically right, because in theory more than one VT block     *
*      may point to the same arm, however, no harm will be done,       *
*      because once tla is set, the code will not be repeated.)        *
*  (7) Calculate total number of virtual cells needed to represent     *
*      this sense.                                                     *
*  (8) Locate excitatory and inhibitory cells for effectors.           *
*      Allocate space in pjnwd array for motor commands.               *
*  (9) Find requested kernels for all preprocessors, copy sizes.       *
*      Calculate nytot, like nvytot above, is number of kernels times  *
*      y size of one kernel.                                           *
* (10) Find cameras for all preprocessors and be sure image size is    *
*      large enough to accommodate specified preprocessor output.      *
* (11) Compute default preprocessor offsets if not entered by user.    *
* (12) If GFUTV is set, find the camera and mark it active.            *
*                                                                      *
*  The point of doing all this before d3tchk is so that VT can be      *
*      treated just the same as all the other senses and sizes of      *
*      preprocessed images are known when cell inputs are attached.    *
*                                                                      *
*  V8A, 05/15/95, GNR - Initial version                                *
*  V8D, 02/20/07, GNR - Move effector setup code from d3tchk           *
*  Rev, 02/05/08, GNR - Add VKUSED marking for SIMDATA output          *
*  ==>, 02/05/08, GNR - Last mod before committing to svn repository   *
*  V8F, 02/21/10, GNR - Remove PLATFORM support                        *
*  Rev, 04/20/10, GNR - Add image preprocessor support                 *
*  V8G, 09/08/10, GNR - Remove preprocessor BC option (not supported)  *
*  V8H, 01/27/11, GNR - Remove obsolete CHANGES block initialization   *
***********************************************************************/

#define NEWARB          /* Use new EFFARB mechanism */
#define EFFTYPE struct EFFARB
#define KNTYPE  struct KERNEL
#define PPTYPE  struct PREPROC
#define TVTYPE  struct TVDEF

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "sysdef.h"
#include "d3global.h"
#include "jntdef.h"
#include "armdef.h"
#include "wdwdef.h"
#include "simdata.h"
#ifdef BBDS
#include "bbd.h"
#endif
#include "tvdef.h"
#include "rocks.h"
#include "rkxtra.h"
#include "rkarith.h"
#include "swap.h"

/* Defaults for nvx,nvy are defined in d3dflt.c */
extern short snsnvx0[NRST];
extern short snsnvy0[NRST];


/*=====================================================================*
*                              fmteffnm                                *
*  Function to format an effector name (or number) for a warning or    *
*  error message.                                                      *
*                                                                      *
*  Call by:  char *msg = fmteffnm(struct EFFARB *pea)                  *
*                                                                      *
*  Argument:                                                           *
*     'pea' is a pointer to the EFFARB block in question.              *
*                                                                      *
*  Return:                                                             *
*     'msg' is pointer to a string that can be included in an error    *
*  message.  This is a static string which remains valid only until    *
*  the next call to fmteffn().                                         *
*=====================================================================*/

char *fmteffnm(struct EFFARB *pea) {

   /* Table of effector names according to EFFARB keff code */
   static char *kefnm[] = { "ARM JOINT ", "UNIV JOINT ", "WINDOW ",
#ifdef BBDS
       "BBD DEVICE "
#endif
       };
#if defined(BBDS) && MaxDevNameLen > 15
#error d3schk: Increase alloc for effnmtxt to MaxDevNameLen + 13
#endif
   static char effnmtxt[32];

   if (pea->keff == KEF_ARM)
      sconvrt(effnmtxt, "(4HARM ,J0IC6,8H, JOINT ,J0IC6)",
#if BYTE_ORDRE > 0
         &pea->heffnm+1, &pea->heffnm, NULL);
#else
         &pea->heffnm, &pea->heffnm+1, NULL);
#endif
   else if (pea->keff <= KEF_WDW)
      sconvrt(effnmtxt, "(J0A12,J1IH6)", kefnm[pea->keff],
         &pea->heffnm, NULL);
   else
      sconvrt(effnmtxt, "(J0A12,J1A15)", kefnm[pea->keff],
         pea->heffnm ? getrktxt(pea->heffnm) : "(Unnamed)", NULL);

   return effnmtxt;
   } /* End fmteffnm() */


/*=====================================================================*
*                               d3schk                                 *
*=====================================================================*/

void d3schk(void) {

   struct PREPROC  *pip;
   struct SDSELECT *pgd;
   struct VCELL    *pvc;
   struct EFFARB   *pea;
   short nvx0,nvy0;           /* Defaults for nvx,nvy */

/* Table indicating which senses require their nvy parameters
*  to be the same as the default--in same order as XX_SRC
*  constants defined in d3global.h except USRSRC at index 0.  */

   static const char kvychk[NRST] = { 0, 1, 1, 0, 0, 0, 1, 1, 1, 0, 0 };

#ifdef BBDS
/* Append the list of BBD senses to the list of internal senses.
*  This must be done before d3tchk is called so that d3tchk will
*  include BBD senses in its scan of VCELL blocks to set VKUSED.
*  If pbbds1 is NULL, this code is harmless, no need to test it.  */

   *RP0->plvc = RP0->pbbds1;
#endif

/*---------------------------------------------------------------------*
*                        Check the VCELL list                          *
*---------------------------------------------------------------------*/

   for (pvc=RP0->pvc1; pvc; pvc=pvc->pnvc) {

      int ivcid = pvc->vtype;       /* Get source type code */
      if (ivcid == USRSRC) ivcid = 0;
      nvx0 = snsnvx0[ivcid];
      nvy0 = snsnvy0[ivcid];

      /* Mark sense as used if it is requested for SIMDATA output.
      *  Check both pgdsns and pgdsid lists.
      *  Note:  This code must execute before d3tchk, so it is placed
      *  here, with redundant looping, rather than in d3gfcl, which
      *  must run after d3tchk.  */
      if (pgd = RP0->pgds) {        /* Assignment intended */
         int ilist;                 /* Control to check two lists */
         for (ilist=PGD_SENSE; ilist<=PGD_SNSID; ++ilist) {
            struct GDSNS *pgsns = pgd->pgdsns[ilist];
            for ( ; pgsns; pgsns=pgsns->pngsns) {
               if (pgsns->vcname[0] == 0) {  /* External sensor */
#ifdef BBDS
                  if (pgsns->hgsnm != pvc->hvname) continue;
#else
                  d3exit(BBD_ERR, NULL, pgsns->hgsnm);
#endif
                  }
               else  {                        /* Built-in sensor */
                  if (strncmp(pgsns->vcname, pvc->vcname, LSNAME) ||
                     pgsns->hgsnm != pvc->hvname) continue; }
               pvc->vcflags |= VKUSED;
               pgsns->pvsns = pvc;
               break;
               } /* End SIMDATA request loop */
            } /* End ilist loop */
         } /* End SIMDATA checking */

      /* Perform type-specific checks that can or must come before
      *  the nvx,nvy default assignment.  */
      switch (ivcid) {

      case IA_SRC:
      case VT_SRC:
         pvc->vcxc.tvmode =
            (byte)(RP->kcolor == ENV_MONO ? Col_GS : Col_C8);
         break;

      case VJ_SRC:
         /* Check VJ blocks for legitimate arm links.  At same
         *  time, set default nvy for test or assignment below.  */
         if (pvc->pvcsrc) {
            register struct ARMDEF *pa = (struct ARMDEF *)pvc->pvcsrc;
            nvy0 = pa->njnts + ((pa->jatt & ARMAU) ? 2 : 0);
            }
         else
            cryout(RK_E1, "0***", RK_LN2+4, fmtvcn(pvc), RK_CCL,
               " IS NOT MATCHED WITH AN ARM.", RK_CCL, NULL);
         break;

      case VW_SRC:
         /* Check VW blocks for legitimate window links.  At same
         *  time, set default nvy for test or assignment below.
         *  (No need to check for nvy overflow, max nchgs is 5.)  */
         if (pvc->pvcsrc)
            nvy0 = wdw_nvwr((WINDOW *)pvc->pvcsrc);
         else
            cryout(RK_E1, "0***", RK_LN2+4, fmtvcn(pvc), RK_CCL,
               " IS NOT MATCHED WITH A WINDOW.", RK_CCL, NULL);
         break;

         } /* End ivcid switch */

      /* If nvx not entered by user, insert default value.
      *  Otherwise, check for invalid changes.  */
      if (pvc->nvx < 0)
         pvc->nvx = nvx0;
      else if (ivcid == VJ_SRC) {
         if (pvc->nvx <= 1)
            cryout(RK_E1, "0***", RK_LN2+4, fmtvcn(pvc), RK_CCL,
               " HAS < 2 CELLS PER JOINT.", RK_CCL, NULL);
         }
      else if (ivcid == VW_SRC) {
         if (pvc->nvx <= 1)
            cryout(RK_E1, "0***", RK_LN2+4, fmtvcn(pvc), RK_CCL,
               " HAS < 2 CELLS PER DOF.", RK_CCL, NULL);
         }

      /* If nvy not entered by user, insert default values.
      *  Otherwise, check for invalid changes.  */
      if (pvc->nvy < 0)
         pvc->nvy = nvy0;
      else if (kvychk[ivcid] && pvc->nvy != nvy0)
         cryout(RK_E1, "0***", RK_LN2+4, fmtvcn(pvc), RK_CCL,
            " HAS INVALID nvy VALUE.", RK_CCL, NULL);

      /* If cpt not entered by user, insert global default */
      if (pvc->vcpt == -1) pvc->vcpt = RP0->vcpt;

      /* Set nvytot for all types */
      pvc->nvytot = pvc->nvy;

      /* Check VT blocks for legitimate arm links */
      if (ivcid == VT_SRC) {
         if (!pvc->pvcsrc)
            cryout(RK_E1, "0***", RK_LN2+4, fmtvcn(pvc), RK_CCL,
               " IS NOT MATCHED WITH  AN ARM.", RK_CCL, NULL);
         else {
            /* This VT is linked to an arm.  Supply default value
            *  for tla if none was entered, then count the number
            *  of normal joints where the touch pad was not set to
            *  zero size and use this to set nvytot.  Default tla
            *  must be set after d3grp1 has run, as corresponding
            *  joint card may not be entered at all.  N.B.: Joint
            *  info is stored in an array, not a linked list.
            */
            struct ARMDEF *pa = (struct ARMDEF *)pvc->pvcsrc;
            struct JNTDEF *pj;
            int jnj,nnj = pa->njnts;
            int ntpads = 0;   /* Number of touch pads */
            for (pj=pa->pjnt,jnj=0; jnj<nnj; pj++,jnj++) {
               if (pj->u.jnt.tla != 0.0) {
                  /* Supply default value for tla */
                  if (pj->u.jnt.tla < 0.0) pj->u.jnt.tla = pj->jl;
                  /* Count touchpads of nonzero area */
                  if (pj->u.jnt.tll > 0.0) ++ntpads;
                  }
               }
            pvc->nvytot = ntpads*pvc->nvy;
            } /* End joint counting */
         } /* End if VT */

      /* Store total number of cells used by this sense */
      pvc->ncells = (int)pvc->nvx * (int)pvc->nvytot;

      } /* End VCELL list scan */

/*---------------------------------------------------------------------*
*           Match EFFARB blocks with associated cell types             *
*                                                                      *
*  There are two ways to freeze an effector:  Omit celltype names, or  *
*  use OPT=F.  The former does not set up motor arbors to the device,  *
*  the latter sets them up but the values are always 0.  I considered  *
*  deleting EFFARBs with no celltypes, but unlinking everything could  *
*  be complicated.  Instead, they are not linked to any il->pfea list, *
*  d3go never bothers with them, and the device has a NULL pjnwd.      *
*---------------------------------------------------------------------*/

   e64set(E64_FLAGS, &RP->ovflow);
   for (pea=RP0->pearb1; pea; pea=pea->pneb) {
      struct CELLTYPE *il;
      si64 tlock;             /* Temp for length overflow check */
      long nexin = 0;         /* Number of pjnwd entries */
      long tljnwd,tjhigh;     /* Entries, high cell in output */
      int  iei;               /* Index over excit,inhib */
      static const char *mexin[NEXIN] = { " EXCIT ", " INHIB " };
      static const ui32 ctfaei[NEXIN] = { CTFAE, CTFAI };

      /* Even if there is only inhibition, d3snsa needs offset to
      *  first pjnwd to store in device block.  Note that we now
      *  allow an effector to draw excit and inhib cells from same
      *  celltype, but with different cell lists.  Such EFFARB
      *  blocks must be threaded only once onto the CELLTYPE block. */
      pea->jjnwd[EXCIT] = RP0->ljnwd;

      /* Save checking this several places inside the iei loop */
      if (pea->nchgs == 0) goto EFFARB_NO_CHANGES;

      /* Find and thread to the cell types for EXCIT,INHIB cells.
      *  If no name entered, hbcnm is 0 and this iei is skipped. */
      for (iei=EXCIT; iei<=INHIB; ++iei) if (pea->exinnm[iei].hbcnm) {
         pea->pect[iei] = il = findln(pea->exinnm+iei);
         if (!il) {              /* Bad celltype name */
            cryout(RK_E1, "0***", RK_LN2+4, fmtrlnm(pea->exinnm+iei),
               LXRLNM, " NOT FOUND TO", RK_CCL, mexin[iei], RK_CCL,
               fmteffnm(pea), RK_CCL, NULL);
            continue;
            }
         if (pea->pefil[iei]) {  /* Cell list found */
            /* Check cell list against cells in celltype.  This
            *  could be two different ilsts operating on the same
            *  celltype, or the same ilst operating on different
            *  celltypes.  So just run ilstchk without the ifs.  */
            if (ilstchk(pea->pefil[iei], il->nelt, fmteffnm(pea))) {
               RK.iexit |= 1; continue; }
            tljnwd = (pea->keop & KEOP_CELLS) ?
               ilstitct(pea->pefil[iei], IL_MASK) : 1L;
            tjhigh = ilsthigh(pea->pefil[iei]) + 1L;
            }
         else {                  /* No cell list found */
            pea->arbw = il->nelt/pea->nchgs;
            tljnwd = (pea->keop & KEOP_CELLS) ? pea->arbw : 1L;
            tjhigh = pea->arbw;
            } /* End synthetic arbor setting mechanism */
         /* Determine default arbs if no value was entered */
         if (pea->arbs < 0) pea->arbs = tjhigh;
         /* Check that d3go will not exceed the top of the layer
         *  in doing nchgs repeats of the cell list.  This test
         *  must be repeated at d3news time in case arbs changes.  */
#if LSIZE == 8
         tlock = jaswe(tjhigh, (pea->nchgs-1)*pea->arbs, OVF_ALLO);
         if (tlock > il->nelt)
#else
         tlock = jmsw(pea->nchgs-1, pea->arbs);
         /* tjhigh - il->nelt cannot underflow, OK to combine */
         tlock = jasle(tlock, tjhigh - il->nelt, OVF_ALLO);
         if (qsw(tlock) < 0)
#endif
            cryout(RK_E1, "0***", RK_LN2+4, mexin[iei]+1, RK_CCL,
               "EFFECTOR ARBOR FOR ", RK_CCL, fmteffnm(pea), RK_CCL,
               "EXCEEDS SIZE OF CELL LAYER", RK_CCL, NULL);
         pea->jjnwd[iei] = RP0->ljnwd;    /* Redundant for excit */
         if (pea->nchgs > 1) il->ctf |= ctfaei[iei];
         tlock = jmsw(tljnwd, pea->nchgs);
#if LSIZE == 8
         nexin = jaswe(nexin, tlock, OVF_ALLO);
         RP0->ljnwd = jaswe(tlock, RP0->ljnwd, OVF_ALLO);
#else
         tlock = jaswe(jesl(nexin), tlock, OVF_ALLO);
         if (swhi(tlock)) e64act("d3schk", OVF_ALLO);
         nexin = swlo(tlock);
         tlock = jaswe(tlock, jesl(RP0->ljnwd), OVF_ALLO);
         if (swhi(tlock)) e64act("d3schk", OVF_ALLO);
         RP0->ljnwd = swlo(tlock);
#endif
         if (RP->ovflow)
            d3exit(LIMITS_ERR, "Effector cells", LONG_MAX);

         /* Thread EFFECTOR blocks for faster locating in d3go.
         *  Do not thread inhib if same as excit.  This code
         *  skipped by tests above if no cells specified.  */
         if (iei == EXCIT || il != pea->pect[EXCIT]) {
            EFFTYPE **ppea = (EFFTYPE **)&il->pfea;
            while (*ppea) ppea = &(*ppea)->pnetct;
            *ppea = pea;
            }
         } /* End iei for-if loop */
      if (nexin == 0) pea->nchgs = 0;
EFFARB_NO_CHANGES: ;
#ifdef BBDS
      /* We will allow nexin == 0 here--if this is not allowed
      *  by the device, bbdschk will complain.  */
      if (pea->keff == KEF_BBD)
         ((BBDDev *)pea->peff)->ldat = (si32)nexin;
#endif

      } /* End loop over EFFARB blocks */

/*---------------------------------------------------------------------*
*      Match PREPROC blocks with associated kernels and cameras        *
*  Note:  If PREPROC is UPROG type, code in g1kern is expected to      *
*  check that nker, nkx, nky were specified there.                     *
*---------------------------------------------------------------------*/

   for (pip=RP->pfip; pip; pip=pip->pnip) {
      struct KERNEL   *pker;
      struct TVDEF    *cam;
      if (pip->ipsflgs & PP_KERN) {
         for (pker=RP0->pfkn; pker; pker=pker->pnkn) {
            if (pker->hknm == pip->hkernm) {
               pip->upr.pipk = pker;
               if (pip->upr.nker == 0) pip->upr.nker = pker->nker;
               if (pip->nkx == 0) pip->nkx = pker->nkx;
               if (pip->nky == 0) pip->nky = pker->nky;
               /* Slightly redundant but shorter code w/o "else" */
               if (pip->nkx != pker->nkx || pip->nky != pker->nky ||
                     pip->upr.nker != pker->nker)
                  cryout(RK_E1, "0***PREPROC ", RK_LN2,
                     getrktxt(pip->hipnm), RK_CCL, " AND KERNEL "
                     "SIZES DO NOT MATCH", RK_CCL, NULL);
               goto GotKernelMatch;
               }
             } /* End kernel match loop */
         cryout(RK_E1, "0***SPECIFIED KERNEL NOT FOUND FOR PREPROC ",
            RK_LN2, getrktxt(pip->hipnm), RK_CCL, NULL);
         } /* End handling KERNEL type */
GotKernelMatch:
      pip->nytot = (ui32)pip->upr.nker * (ui32)pip->upr.nipy;
      if (pip->upr.ipuflgs & PP_2SYMM) pip->nytot <<= 1;
      /* Now find the source camera and use camera dimensions
      *  (less kernel size) if not specified for preprocessor.
      *  Note that the order of computation here does not allow
      *  for the case of determining nipx,nipy from the camera
      *  and kernel sizes minus given offsets, but it seems
      *  strange anyone would want to do it that way.  If
      *  nipx,nipy are specified, check that camera dimensions
      *  are big enough to accommodate.  */
      cam = findtv(pip->hcamnm, ssprintf(NULL, " FOR PREPROC %s",
         getrktxt(pip->hcamnm)));
      if (cam) {
         int ttx,tty;         /* Temps for signed x,y checks */
         pip->pipcam = cam;
         /* Set preprocessor image x size if not entered */
         ttx = (int)cam->utv.tvx - (int)pip->nkx + 1;
         if (ttx <= 0) {
            cryout(RK_E1, "0***KERNEL X EXCEEDS CAMERA X FOR ",
               RK_LN2, getrktxt(pip->hipnm), RK_CCL, NULL);
            ttx = 1;
            }
         if (pip->upr.nipx == 0)
            pip->upr.nipx = ttx;
         else if (pip->upr.nipx > cam->utv.tvx) {
            cryout(RK_E1, "0***PREPROC X EXCEEDS CAMERA X FOR ",
               RK_LN2, getrktxt(pip->hipnm), RK_CCL, NULL);
            /* Modify to avoid further errors */
            pip->upr.nipx = ttx;
            }
         /* Set preprocessor image y size if not entered */
         tty = (int)cam->utv.tvy - (int)pip->nky + 1;
         if (tty <= 0) {
            cryout(RK_E1, "0***KERNEL Y EXCEEDS CAMERA Y FOR ",
               RK_LN2, getrktxt(pip->hipnm), RK_CCL, NULL);
            tty = 1;
            }
         if (pip->upr.nipy == 0)
            pip->upr.nipy = tty;
         else if (pip->upr.nipy > cam->utv.tvy) {
            cryout(RK_E1, "0***PREPROC Y EXCEEDS CAMERA Y FOR ",
               RK_LN2, getrktxt(pip->hipnm), RK_CCL, NULL);
            /* Modify to avoid further errors */
            pip->upr.nipy = tty;
            }
         /* Set preprocessor image offsets if not entered.  (These
         *  subtractions cannot go negative per tests above.)  */
         if (pip->upr.oipx == UI16_MAX)   /* Use default x offset */
            pip->upr.oipx = (cam->utv.tvx - pip->upr.nipx) >> 1;
         if (pip->upr.oipy == UI16_MAX)   /* Use default y offset */
            pip->upr.oipy = (cam->utv.tvy - pip->upr.nipy) >> 1;
         if (pip->dcols > 1 && pip->dcoff == 0.0)
            pip->dcoff = pip->ipwd ? 1.1*pip->ipwd : pip->upr.nipx/100;

         /* Check to make sure that there is enough border on either
         *  side of the preprocessor output to accommodate one-half
         *  the kernel within the camera image.  Note that if kernel
         *  size is even, we will put the smaller border on the left--
         *  if odd, border is same on both sides (center point is on
         *  output grid).  */
         ttx = (int)cam->utv.tvx - (int)(pip->upr.nipx + pip->upr.oipx);
         tty = (int)cam->utv.tvy - (int)(pip->upr.nipy + pip->upr.oipy);
         if (pip->upr.oipx < ((pip->nkx-1) >> 1) ||
             pip->upr.oipy < ((pip->nky-1) >> 1) ||
             ttx < (pip->nkx >> 1) || tty < (pip->nky >> 1))
            cryout(RK_E1, "0***PREPROC ", RK_LN2, getrktxt(pip->hipnm),
               RK_CCL, ", GIVEN KERNEL AND OFFSETS, DOES NOT FIT IN "
               "INPUT IMAGE.", RK_CCL, NULL);

         /* Check for consistency problems */
         if (!(pip->ipsflgs & PP_UPGM)) {
            if (pip->upr.ipkcol != Col_GS)
               cryout(RK_E1, "0***BUILT-IN PREPROC ", RK_LN2,
                  getrktxt(pip->hipnm), RK_CCL,
                  " REQUIRES GRAYSCALE COLOR MODE", RK_CCL, NULL);
            if (pip->upr.ipkcol != cam->utv.tvkcol)
               cryout(RK_E1, "0***BUILT-IN PREPROC ", RK_LN2,
                  getrktxt(pip->hipnm), RK_CCL,
                  " CANNOT CHANGE IMAGE COLOR MODE.", RK_CCL, NULL);
            }

         /* Compute a few constants used repeatedly in d3imgt() */
         {  ui32 pixsz = (ui32)max(pip->upr.ipkcol, 1);
            pip->upr.tvx3  = pixsz*(ui32)cam->utv.tvx;
            pip->upr.nipx3 = pixsz*(ui32)pip->upr.nipx;
            pip->upr.oin0 = pip->upr.tvx3*(ui32)pip->upr.oipy +
               pixsz*(ui32)pip->upr.oipx;
            pip->lbkup = pip->upr.tvx3*(ui32)((pip->nky-1)>>1) +
               pixsz*(ui32)((pip->nkx-1)>>1);
            pip->upr.lrowskip = pip->upr.tvx3 - pip->upr.nipx3;
            pip->jkr0 = (pixsz == 1) ? 2 : 0;
            pip->jkri = (byte)(4 - pixsz);
            } /* End pixsz, etc. local scope */

         /* Add this preprocessor to list associated with its camera.
         *  (Avoids whole list scan for each freq. in each trial.)  */
         pip->pnipc = cam->pmyip;
         cam->pmyip = pip;
         } /* End camera reconciliation code */
      } /* End image preprocessor loop */

/* As a convenience to the user, if camera info is requested for
*  SIMDATA output, we now make sure the camera exists and mark it
*  TV_ON.  If a different camera is named at Group III time, and
*  it is not already TV_ON for some other reason, an error occurs.
*/
   if (pgd = RP0->pgds) {        /* Assignment intended */
      if (pgd->hcamnm > 0) {
         pgd->pgdcam = findtv(pgd->hcamnm, "FOR SIMDATA INFO");
         if (pgd->pgdcam) pgd->pgdcam->utv.tvsflgs |= TV_ON;
         }
      } /* End checking SIMDATA camera */

   } /* End d3schk() */
