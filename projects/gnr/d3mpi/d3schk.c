/* (c) Copyright 1995-2017, The Rockefeller University *11115* */
/* $Id: d3schk.c 76 2017-12-12 21:17:50Z  $ */
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
*  (4) If nsxl,nsya parameters not yet set, apply appropriate defaults.*
*      Give an error for any VJ or VW blocks that do not have 'nsxl'   *
*      at least 2, otherwise intercell interval is not defined in      *
*      d3vjnt.  These checks done here to catch errors in SENSE or     *
*      ARM/WINDOW cards in whatever order they may have been entered.  *
*  (5) If cpt parameter not yet set, set from global cpt.              *
*  (6) Fill in default tla,tll where not entered.                      *
*  (7) (Calc of nvytot, nvcells removed in R66)                        *
*  (8) Locate excitatory and inhibitory cells for effectors.           *
*  (9) Find requested kernels for all image preprocessors, copy sizes. *
*      Calculate nytot = the number of kernels times y size of one     *
*      kernel, and nyvis, number of those visible to connections.      *
* (10) Find cameras for all preprocessors and be sure image size is    *
*      large enough to accommodate specified preprocessor output.      *
* (11) Compute default preprocessor offsets if not entered by user.    *
* (12) If GFUTV is set, find the camera and mark it active.            *
*                                                                      *
*  Design note:  Calculations here relating to image sizes at various  *
*      stages of data reduction are for the max image size specified   *
*      on the TV or CAMERA card and are used for memory allocation.    *
*      If individual images are smaller, d3imgt() is expected to       *
*      embed them in the standard image array and add borders so that  *
*      e.g. preprocessors and connection Lij can find pixels at the    *
*      same offsets.                                                   *
*                                                                      *
*  d3g1imin now convolves multiple kernels under one preprocessor, so  *
*     this code only has to work with the combined nkx,nky             *
*                                                                      *
*  The point of doing all this before d3tchk is so that VT can be      *
*      treated just the same as all the other senses and sizes of      *
*      preprocessed images are known when cell inputs are attached.    *
************************************************************************
*  V8A, 05/15/95, GNR - Initial version                                *
*  V8D, 02/20/07, GNR - Move effector setup code from d3tchk           *
*  Rev, 02/05/08, GNR - Add VKUSED marking for SIMDATA output          *
*  ==>, 02/05/08, GNR - Last mod before committing to svn repository   *
*  V8F, 02/21/10, GNR - Remove PLATFORM support                        *
*  Rev, 04/20/10, GNR - Add image preprocessor support                 *
*  V8G, 09/08/10, GNR - Remove preprocessor BC option (not supported)  *
*  V8H, 01/27/11, GNR - Remove obsolete CHANGES block initialization   *
*  Rev, 12/24/12, GNR - New overflow checking                          *
*  R64, 12/09/15, GNR - ljnwd calc is now an estimate for d3echo,      *
*                       final calc and alloc to d3news, needs ntc1     *
*  Rev, 12/19/15, GNR - Change 'arbs' to 'arbsep[NEXIN]'               *
*  R66, 03/11/16, GNR - Rename nvx->nsxl, nvy->nsya so != ix->nv[xy]   *
*                       Remove calc of nvytot,nvcells to d3snsa()      *
*  R67, 09/14/16, GNR - Copy UPREPR nip[xy] to PREPROC for d3lplt nodes*
*  R70, 01/19/17, GNR - Remove d3exit when RP->ovflow>0, now in main   *
*  R74, 06/03/17, GNR - Eliminate old ColorMode, use BM_xxxx codes     *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "d3global.h"
#include "jntdef.h"
#include "armdef.h"
#include "wdwdef.h"
#include "simdata.h"
#include "tvdef.h"
#include "rocks.h"
#include "rkxtra.h"
#include "rkarith.h"
#include "plots.h"
#include "swap.h"


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
*                                                                      *
*  Rev, 12/23/15, GNR - Remove kefnm labels array, use pea->kef switch *
*                       to eliminate dependence on keff numeric defs.  *
*=====================================================================*/

char *fmteffnm(struct EFFARB *pea) {

#if defined(BBDS) && MaxDevNameLen > 15
#error d3schk: Make sure effnmtxt holds MaxDevNameLen and check uses
#endif
   static char effnmtxt[32];

   switch (pea->keff) {
   case KEF_ARM: {
      unsigned int iarm = pea->heffnm & BYTE_MAX;
      unsigned int ijnt = pea->heffnm >> BITSPERBYTE;
      sconvrt(effnmtxt, "(4HARM ,J0UI6,8H, JOINT ,J0UI6)",
         &iarm, &ijnt, NULL);
      break;
      } /* End KEF_ARM local scope */
   case KEF_UJ:
      sconvrt(effnmtxt, "(4HARM ,J0UH6,17H, UNIVERSAL JOINT)",
         &pea->heffnm, NULL);
      break;
   /* Note:  Code KEF_GRP is currently not used--there is an extra
   *  JNTDEF for the gripper but not an EFFARB.  */
   case KEF_GRP:        /* JIC */
      sconvrt(effnmtxt, "4HARM ,J0UH6,9H, GRIPPER)",
         &pea->heffnm, NULL);
      break;
   case KEF_WDW:
      sconvrt(effnmtxt, "(7HWINDOW ,J0UH6)", &pea->heffnm, NULL);
      break;
#ifdef BBDS
   case KEF_BBD:
      sconvrt(effnmtxt, "(4HBBD ,J1A" qqv(MaxDevNameLen) ")",
         pea->heffnm ? getrktxt(pea->heffnm) : "(Unnamed)", NULL);
#endif
   default:
      strcpy(effnmtxt, "Unknown effector type");
      break;
      } /* End keff switch */

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
                  d3exit(NULL, BBD_ERR, pgsns->hgsnm);
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

      /* Perform type-specific checks and/or assign nsxl,nsya from
      *  type-specific or generic defaults.  nsxl,nsya defaults for
      *  USRSRC are in d3grp1, because needed for bbdsregs() call.  */
      switch (ivcid) {

      case VJ_SRC:
         /* Check VJ blocks for legitimate arm links */
         if (pvc->pvcsrc) {
            struct ARMDEF *pa = (struct ARMDEF *)pvc->pvcsrc;
            if (pvc->nsxl < 0) pvc->nsxl = 12;
            else if (pvc->nsxl <= 1)
               cryout(RK_E1, "0***", RK_LN2+4, fmtvcn(pvc), RK_CCL,
                  " HAS < 2 CELLS PER JOINT.", RK_CCL, NULL);
            if (pvc->nsya < 0) pvc->nsya =
               pa->njnts + ((pa->jatt & ARMAU) ? 2 : 0);
            else if (pvc->nsya > pa->njnts)
               cryout(RK_E1, "0***", RK_LN2+4, fmtvcn(pvc), RK_CCL,
                  " HAS nsya > num joints.", RK_CCL, NULL);
            }
         else
            cryout(RK_E1, "0***", RK_LN2+4, fmtvcn(pvc), RK_CCL,
               " IS NOT MATCHED WITH AN ARM.", RK_CCL, NULL);
         break;

      case VW_SRC:
         /* Check VW blocks for legitimate window links */
         if (pvc->pvcsrc) {
            struct WDWDEF *pw = (struct WDWDEF *)pvc->pvcsrc;
            int nsya0 = wdw_nvwr(pw);
            if (pvc->nsxl < 0) pvc->nsxl = 12;
            else if (pvc->nsxl <= 1)
               cryout(RK_E1, "0***", RK_LN2+4, fmtvcn(pvc), RK_CCL,
                  " HAS < 2 CELLS PER DOF.", RK_CCL, NULL);
            if (pvc->nsya < 0) pvc->nsya = nsya0;
            else if (pvc->nsya > nsya0)
               cryout(RK_E1, "0***", RK_LN2+4, fmtvcn(pvc), RK_CCL,
                  " HAS nsya > DOFs.", RK_CCL, NULL);
            }
         else
            cryout(RK_E1, "0***", RK_LN2+4, fmtvcn(pvc), RK_CCL,
               " IS NOT MATCHED WITH A WINDOW.", RK_CCL, NULL);
         break;

      case VT_SRC:
         /* Because an arm can have multiple touch pads, and we now
         *  allow them to have different dimensions, the nsxl,nysa
         *  dimensions in the VCELL block are not used.  To give
         *  reasonable defaults for d3echo, we put in the max values
         *  over the joints.  Defaults for tla and tll are supplied
         *  in g1arm,g1jnt, but user can set either of these to 0 to
         *  turn off processing of this touch pad.  nsya is replaced
         *  late in d3tchk() by sum over joints in jntinVT of nta.
         *  N.B.: Joint info is stored in an array, not a linked list.
         */
         if (pvc->pvcsrc) {
            struct ARMDEF *pa = (struct ARMDEF *)pvc->pvcsrc;
            struct JNTDEF *pj,*pje;
            si32 tll0 = 0, tla0 = 0;
            short nxl0 = 0, nya0 = 0;
            pje = pa->pjnt + pa->njnts;
            /* N.B.  First njnts joints are the normal ones */
            for (pj=pa->pjnt; pj<pje; ++pj) {
               if (pj->u.jnt.tla > tla0) tla0 = pj->u.jnt.tla;
               if (pj->u.jnt.tll > tll0) tll0 = pj->u.jnt.tll;
               if (pj->u.jnt.ntl > nxl0) nxl0 = pj->u.jnt.ntl;
               if (pj->u.jnt.ntl > nya0) nya0 = pj->u.jnt.nta;
               }
            if (pvc->nsxl < 0) pvc->nsxl = nxl0;
            if (pvc->nsya < 0) pvc->nsya = nya0;
            }
         else
            cryout(RK_E1, "0***", RK_LN2+4, fmtvcn(pvc), RK_CCL,
               " IS NOT MATCHED WITH  AN ARM.", RK_CCL, NULL);
         break;

         } /* End ivcid switch */

      /* If cpt not entered by user, insert global default */
      if (pvc->vcpt == -1) pvc->vcpt = RP0->vcpt;

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
*                                                                      *
*  R64, 12/09/15, GNR - This code now estimates ljnwd (cannot allow    *
*     for ntc1 cell outputs when EOPT=N), fixes nexin bug, see d3news. *
*---------------------------------------------------------------------*/

   RP0->ljnwd = 0;
   e64dec(EAfl(OVF_ALLO));
   for (pea=RP0->pearb1; pea; pea=pea->pneb) {
      struct CELLTYPE *il;
      si64 tlock,tt;          /* Temps for length overflow check */
      long tjhigh;            /* High cell in output (long for ilst */
      long tnelt;             /* Total cells in celltype */
      si32 tljnwd;            /* Entries in output */
      int  iei;               /* Index over excit,inhib */
      int  nexin = 0    ;     /* Number pjnwd entries this EFFARB */
      int  nrows;             /* Number of rows of arbw outputs */
      static const char *mexin[NEXIN] = { " EXCIT ", " INHIB " };
      static const ui32 ctfaei[NEXIN] = { CTFAE, CTFAI };

      /* Save checking this several places inside the iei loop */
      if (pea->nchgs == 0) continue;

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
         tnelt = (long)il->nelt; /* Total cells */
         if (pea->pefil[iei]) {  /* Cell list found */
            /* Check cell list against cells in celltype.  This
            *  could be two different ilsts operating on the same
            *  celltype, or the same ilst operating on different
            *  celltypes.  So just run ilstchk without the ifs.  */
            if (ilstchk(pea->pefil[iei], tnelt, fmteffnm(pea))) {
               RK.iexit |= 1; continue; }
            pea->arbw[iei] = (ui32)ilstitct(pea->pefil[iei], IL_MASK);
            tjhigh = ilsthigh(pea->pefil[iei]) + 1L;
            }
         else {                  /* No cell list found */
            pea->arbw[iei] = il->nelt/pea->nchgs;
            tjhigh = (long)pea->arbw[iei];
            } /* End synthetic arbor setting mechanism */
         tljnwd = (pea->keop & KEOP_CELLS) ? pea->arbw[iei] : 1;

         /* Determine default arbsep if no value was entered */
         if (pea->arbsep[iei] < 0)
            pea->arbsep[iei] = il->nelt/pea->nchgs;

         /* Check that d3go will not exceed the top of the layer in
         *  doing nchgs repeats of the cell list.  This test must
         *  be repeated at d3news time in case arbsep changes.  */
#if LSIZE == 8
         tlock = jaswd(tjhigh, (pea->nchgs-1)*pea->arbsep[iei]);
         if (tlock > tnelt)
#else
         tlock = jmsw(pea->nchgs-1, pea->arbsep[iei]);
         /* tjhigh - tnelt cannot underflow, OK to combine */
         tlock = jaslod(tlock, tjhigh - tnelt);
         if (qsw(tlock) > 0)
#endif
            cryout(RK_E1, "0***", RK_LN2+4, mexin[iei]+1, RK_CCL,
               "EFFECTOR ARBOR FOR ", RK_CCL, fmteffnm(pea), RK_CCL,
               "EXCEEDS SIZE OF CELL LAYER", RK_CCL, NULL);
         /* Compute total space and add to nexin and grand total
         *  (nexin calc kept parallel to d3news where it sets ldat). */
         nrows = pea->nchgs;
         if (pea->entc > 1) {
            pea->keop |= KEOP_NITS;
            nrows = jmsld(nrows, pea->entc);
            }
         tlock = jmsw(tljnwd, nrows);
         if (qsw(tlock) > 0) il->ctf |= ctfaei[iei];
         tt = jaswd(jesl(nexin), tlock);
         swlodm(nexin, tt);
         tt = jaswd(jesl(RP0->ljnwd), tlock);
         swlodm(RP0->ljnwd, tt);

         /* Thread EFFECTOR blocks for faster locating in d3go.
         *  Do not thread inhib if same as excit.  This code
         *  skipped by tests above if no cells specified.  */
         if (iei == EXCIT || il != pea->pect[EXCIT]) {
            EFFTYPE **ppea = (EFFTYPE **)&il->pfea;
            while (*ppea) ppea = &(*ppea)->pnetct;
            *ppea = pea;
            }
         } /* End iei for-if loop */
      if (nexin == 0) {
         pea->nchgs = 0;
         il->ctf &= ~(CTFAE|CTFAI);
         }

      } /* End loop over EFFARB blocks */

/*---------------------------------------------------------------------*
*      Match PREPROC blocks with associated kernels and cameras        *
*                                                                      *
*  R74, 05/14/17, GNR - Move pip->nk[xy] to pip->upr, default to 1     *
*                       if UPROG uses no kernels.                      *
*---------------------------------------------------------------------*/

   for (pip=RP->pfip; pip; pip=pip->pnip) {
      struct UPREPR   *pup = &pip->upr;
      struct KERNEL   *pker;
      struct TVDEF    *cam;
      if (pip->ipsflgs & PP_KERN) {       /* Requests a kernel */
         for (pker=RP0->pfkn; pker; pker=pker->pnkn) {
            if (pker->hknm == pip->hkernm) {
               pup->pipk = pker;
               if (pup->nker == 0) pup->nker = pker->nker;
               if (pup->nkx == 0)  pup->nkx  = pker->nkx;
               if (pup->nky == 0)  pup->nky =  pker->nky;
               /* Slightly redundant but shorter code w/o "else" */
               if (pup->nkx != pker->nkx ||
                   pup->nky != pker->nky ||
                   pup->nker != pker->nker)
                  cryout(RK_E1, "0***PREPROC ", RK_LN2,
                     getrktxt(pip->hipnm), RK_CCL, " AND KERNEL "
                     "SIZES DO NOT MATCH", RK_CCL, NULL);
               if (pker->kflgs & IPK_STGH) pip->ipsflgs |= PP_STGH;
               if (pker->kflgs & IPK_NEGS) pip->ipsflgs |= PP_NEGS;
               goto GotKernelMatch;
               }
             } /* End kernel match loop */
         cryout(RK_E1, "0***SPECIFIED KERNEL NOT FOUND FOR PREPROC ",
            RK_LN2, getrktxt(pip->hipnm), RK_CCL, NULL);
         } /* End handling KERNEL type */
      else {                              /* No kernel needed */
         if (pup->nker == 0) pup->nker = 1;
         if (pup->nkx == 0)  pup->nkx  = 1;
         if (pup->nky == 0)  pup->nky =  1;
         }
GotKernelMatch: ;

      /* Now find the source camera.  Compute nip[xy] from kernel size
      *  and desired final image size (nppx,nppy).  Check that camera
      *  dimensions with oip[xy] offsets are big enough to accommodate
      *  these nip[xy] values.  */
      cam = findtv(pip->hcamnm, ssprintf(NULL, " FOR PREPROC %s",
         getrktxt(pip->hcamnm)));
      if (cam) {
         pip->pipcam = cam;

         /* If user did not enter preprocessor output image size, use
         *  default provided by camera.  Larger sizes will produce
         *  error in test on preprocessor input size below.  */
         if (pup->nppx == 0) pup->nppx = cam->utv.tvx;
         if (pup->nppy == 0) pup->nppy = cam->utv.tvy;

         /* Compute default (= max) preprocessor input image size */
         if (pip->ipsflgs & PP_UPGM) {
            if (pup->nipx == 0) pup->nipx = pup->nppx;
            if (pup->nipy == 0) pup->nipy = pup->nppy;
            }
         else {                        /* Standard kernel method */
            ui32 tnpp;
            /* Calc nipx default and test for overflow */
            tnpp = (ui32)UI16_MAX - (ui32)pup->nkx;
            if ((ui32)pup->nppx > tnpp) {
               d3lime("d3schk", PLcb(PLC_PPSZ) + (int)cam->utv.icam);
               pup->nppx = tnpp;
               }
            if (pup->nipx == 0) pup->nipx = pup->nppx + pup->nkx - 1;
            /* Calc nipy default and test for overflow */
            tnpp = (ui32)UI16_MAX - (ui32)pup->nky;
            if ((ui32)pup->nppy > tnpp) {
               d3lime("d3schk", PLcb(PLC_PPSZ) + (int)cam->utv.icam);
               pup->nppy = tnpp;
               }
            if (pup->nipy == 0) pup->nipy = pup->nppy + pup->nky - 1;
            } /* End else kernel method */

         /* Color mode of preprocessor output is camera mode promoted
         *  to 16 bits or, if PP_GfmC is set, it is always BM_GS16.  */
         pup->ipkcol = (pup->ipuflgs & PP_GfmC ||
            qGray(cam->utv.tvkocol)) ? BM_GS16 : BM_C48;

         /* Establish nytot from number of kernels and nppy */
         pip->nytot = (ui32)pup->nker * (ui32)pup->nppy;
         if (pip->nytot > UI16_MAX)
            cryout(RK_E1, "0***PREPROC ", RK_LN2,
               getrktxt(pip->hipnm), RK_CCL, " NKER*YSIZE "
               "EXCEEDS 16-BIT LIMIT", RK_CCL, NULL);
         pip->nyvis =
            pip->ipsflgs & PP_STGH ? pup->nppy : pip->nytot;
         if (pup->ipuflgs & PP_2SYMM)
            pip->nytot <<= 1, pip->nyvis <<= 1;
         if (pup->ipuflgs & PP_GfmC)
            cam->utv.tvsflgs |= TV_ONPPGC;

         /* R74, 06/29/17, GNR - Bug fix:  corrected for
         *  size of single pixel can now be 1 or 2 */
         pip->jkr0 = qColored(pup->ipkcol) ? 0 : 2;
         /* Fiat:  All preprocessor outputs will be 16 bits per color
         *  value (S14).  This reduces the number of sjbr routines and
         *  allows S16 trig fns for STGH and fewer shifts elsewhere. */
         {  ulng tlppi3;
            pup->sncol = qColored(pup->ipkcol) ? 3 : 1;
            tlppi3 = (ulng)pup->sncol * (ulng)pup->nppx *
               (ulng)pup->nppy;
            if (tlppi3 > UI32_MAX) {
               cryout(RK_E1, "0***PREPROC ", RK_LN2,
                  getrktxt(pip->hipnm), RK_CCL, " OUTPUT IMAGE "
                  "EXCEEDS 32-BIT LIMIT", RK_CCL, NULL);
               tlppi3 = UI32_MAX;
               }
            pup->lppi3 = (ui32)tlppi3;
            }

         /* Add this preprocessor to list associated with its camera.
         *  (Avoids whole list scan for each freq. in each trial.)  */
         pip->pnipc = cam->pmyip;
         cam->pmyip = pip;
         cam->tviflgs |= TV_Filt;
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
