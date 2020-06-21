/* (c) Copyright 2018, The Rockefeller University *11115* */
/* $Id: d3g1imin.c 78 2018-08-02 18:36:58Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                             d3g1imin.c                               *
*                                                                      *
*  This file contains the g1cam, g1kern, and g1prep functions, which   *
*  are called by d3grp1() to interpret options on the CAMERA, KERNEL,  *
*  and PREPROC control cards relating to image preprocessing.          *
*                                                                      *
************************************************************************
*  V8F, 04/15/10, GNR - New routines, move CAMERA input from d3grp1    *
*  ==>, 05/15/10, GNR - Last mod before committing to svn repository   *
*  Rev, 07/26/10, GNR - Reverse EDGE x due to left-handed image coords *
*  V8H, 11/07/10, GNR - Implement BBD SENSE and CAMERA modalities      *
*  R74, 05/09/17, GNR - Add STG2H2 kernel type, kernel convolutions    *
*  R74, 05/16/17, GNR - New color/type controls for cameras            *
*  R78, 04/10/18, GNR - Allocate (and keep) image space per tvr[xy]e   *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "sysdef.h"
#include "d3global.h"
#include "tvdef.h"
#include "rocks.h"
#include "rkxtra.h"
#include "bapkg.h"
#include "plots.h"

/*=====================================================================*
*                              getcmode                                *
*  Auxiliary function to get new-style color mode assuming CMODE has   *
*  been scanned on a control card.  These options have been chosen to  *
*  accept most of the possible image types used in Matlab.             *
*     Names will consist of the word "GRAYSCALE" or "COLOR" followed   *
*  by an optional hyphen (minus sign) and a type or length indication. *
*  Acceptable type names are "FLOAT", "SINGLE", or "DOUBLE".  These    *
*  refer to the type and length of one gray value or one color.        *
*  Explicit numerical lengths are used only with unsigned integer      *
*  pixels and refer to the length of one gray value or one entire      *
*  three-colored pixel in bits.  Acceptable values for grayscale       *
*  images are 8, 16, or 32.  Acceptable values for colored images are  *
*  8, 16, 24, or 48.  C-8 is the old 2-3-3 bit B-G-R pixel and C-16 is *
*  a 1-5-5-5 R-G-B pixel with the left bit ignored.                    *
*     If no type/length is given, defaults are G-8 for "GRAY" and      *
*  C-24 for "COLOR".  If no CMODE option is present, default is G-8.   *
*     The usual abbreviation conventions apply, e.g. C-D is a colored  *
*  image with double-precision floating-point pixels. FLOAT and SINGLE *
*  are synonyms.                                                       *
*     All images are converted in d3imgt() to one of the formats G-8,  *
*  G-16, C-24, or C-48.  Currently C-8 and C-16 are defined here but   *
*  code will be supplied to read them only the need arises.  Palette   *
*  mode is defined in the TVC codes but there is no plan to implement  *
*  it in CNS.                                                          *
*     Extraction of gray values from color images will be done at the  *
*  sjbr() level so same image can be used as color and gray as input   *
*  to different connection types.                                      *
*=====================================================================*/

static int getcmode(void) {
   int  km,kt;                /* match results */
   static char *gcol[] = { "GRAYSCALE", "COLOR" };
   static char *glen[] = { "-8", "-16", "-32",
      "-FLOAT", "-SINGLE", "-DOUBLE" };
   static byte gmodes[] = { TVC_GS|TVC_B1, TVC_GS|TVC_B2,
      TVC_GS|TVC_B4, TVC_GS|TVC_Flt|TVC_B4, TVC_GS|TVC_Flt|TVC_B4,
      TVC_GS|TVC_Flt|TVC_B8 };
   static char *clen[] = { "-24", "-48", "-8", "-16",
      "-FLOAT", "-SINGLE", "-DOUBLE" };
   static byte cmodes[] = { TVC_Col|TVC_B1, TVC_Col|TVC_B2,
      TVC_C8, TVC_C16, TVC_Col|TVC_Flt|TVC_B4,
      TVC_Col|TVC_Flt|TVC_B4, TVC_Col|TVC_Flt|TVC_B8 };

   km = match(RK_EQCHK, RK_MPMRQ, ~(RK_PMINUS|RK_COMMA), 0,
      gcol, sizeof(gcol)/sizeof(char *));
   switch (km) {
   case 1:                    /* Grayscale */
      if (!(RK.scancode & RK_PMINUS)) return (int)gmodes[0];
      kt = match(0, RK_MREQF, ~(RK_COMMA|RK_ENDCARD), 0,
         glen, sizeof(glen)/sizeof(char *));
      return (int)(kt > 0 ? gmodes[kt-1] : gmodes[0]);
   case 2:                    /* Color */
      if (!(RK.scancode & RK_PMINUS)) return (int)cmodes[0];
      kt = match(0, RK_MREQF, ~(RK_COMMA|RK_ENDCARD), 0,
         clen, sizeof(clen)/sizeof(char *));
      return (int)(kt > 0 ? cmodes[kt-1] : cmodes[0]);
      } /* End km switch */
   return (int)gmodes[0];
   } /* End getcmode() */


/*=====================================================================*
*                                g1cam                                 *
*                 Process a CAMERA or TV control card                  *
*=====================================================================*/

void g1cam(void) {

   struct TVDEF *cam;
   struct UTVDEF *putv;
   ui32 ic;                   /* kwscan field recognition codes */
   int kwret;                 /* kwscan return code */
   int kxrstat = FALSE;       /* XRSTATS option flag */
   txtid thcamnm;             /* Temporary camera name handle */
   txtid hmname = 0;          /* Modality name locator */
   txtid hudata = 0;          /* User data string locator */
#ifdef BBDS
   txtid hhname = 0;          /* Host name locator */
#endif
   char lea[SCANLEN1];        /* Card data */
   byte tflgs = 0;            /* Temp interface type flags */

   static char *gfreqs[] = {  /* Order as in bbd.h */
      "TRIAL", "EVENT", "SERIES" };
#define NUPLBLS 2
   static char *uplbls[NUPLBLS] = { "TVINIT", "TVGRAB" };
#define NENDIANS 2
   static char *endians[NENDIANS] = { "LITTLE", "BIG" };

   /* Image storage modes as a function of image input modes */
   static byte omode[] = { BM_GS, BM_GS16, BM_GS16, BM_GS16,
      BM_BAD, BM_BAD, BM_GS16, BM_GS16,
      BM_C24, BM_C48, BM_C48, BM_C48,
      BM_BAD, BM_BAD, BM_C48, BM_C48,
      BM_C8,  BM_C16, BM_COLOR, BM_BAD };

   /* Input image pixel size (bytes) as a function of color mode */
   static byte ipxsz[] = { 1, 2, 4, 8,  0, 0, 4, 8,
      3, 6, 12, 24,  0, 0, 12, 24,  1, 2, 1, 0 };

   /* Terminal error if too many cameras */
   if (RP->ntvs >= BYTE_MAX)
      d3exit("TV", LIMITS_ERR, BYTE_MAX);

   /* Get camera name and give error if it is a duplicate.
   *  (The following code is designed to allow input for either
   *  VVTV or BBD or both, even though both is unlikely.) */
   cdscan(RK.last, 1, SCANLEN, RK_WDSKIP);
   scanck(lea, RK_REQFLD, ~RK_COMMA);
#ifdef VVTV
   /* If first parameter is "VVTV", second is camera id.
   *  (This probably has to be a number, but for now
   *  treat it as text).  */
   if (ssmatch(lea, "VVTV", 1)) {
      tflgs = TV_VVTV;
      inform("(S,TH" qTV_MAXNM ")", &thcamnm, NULL);
      }
   else
#endif
#ifdef BBDS
   /* If first parameter is "BBD", second is camera id */
   if (ssmatch(lea, "BBD", 1)) {
      tflgs = TV_BBD;
      g1bbdn(&hhname, &thcamnm);
      }
   else
#endif
   /* If first parameter is "UPGM", second is camera id */
   if (ssmatch(lea, "UPGM", 1)) {
      tflgs = TV_UPGM;
      inform("(S,TH" qTV_MAXNM ")", &thcamnm, NULL);
      }
   else {
      ermark(RK_IDERR);
      skip2end();
      return;
      }

   /* Give error if camera name is a duplicate (but process the
   *  card anyway to detect any other errors).  It is not correct
   *  to just check whether the name is already entered in the text
   *  database--it may exist in another name space.)  */
   if (findtv(thcamnm, NULL)) dupmsg("CAMERA", NULL, thcamnm);

   /* Note: The TVDEF structure is currently not needed on comp nodes
   *  but is shared to accommodate possible future implementations of
   *  vvgrab() or other code that would bring different picture tiles
   *  to different nodes.  */
   cam = *RP0->pltv = (struct TVDEF *)
      allocpcv(Static, 1, IXstr_TVDEF|MBT_Unint, "tv");
   putv = &cam->utv;
   RP0->pltv = &cam->pnxtv;
   putv->icam = ++RP->ntvs;      /* Count cameras */
   tvdflt(cam);                  /* Set defaults */
   /* Insert name and type flags scanned earlier */
   cam->hcamnm = thcamnm;
   putv->tvsflgs = tflgs;

   /* Read TV options */
   inform("(S=K,2*UH)", &putv->tvx, &putv->tvy, NULL);
   ic = 0;
   while (kwret = kwscan(&ic, /* Assignment intended */
         "TVX%V>UH",       &putv->tvx,    /* ic bit 1 */
         "TVY%V>UH",       &putv->tvy,    /* ic bit 2 */
         "TVRX%V>UH",      &putv->tvrxe,  /* ic bit 3 */
         "TVRY%V>UH",      &putv->tvrye,  /* ic bit 4 */
         "TVXO%UH",        &putv->tvxo,
         "TVYO%UH",        &putv->tvyo,
         "TVRXO%UH",       &putv->tvrxo,
         "TVRYO%UH",       &putv->tvryo,
         "SLOT%UC",        &putv->islot,
         "SPACEAVG%V>UC",  &putv->tvsa,
         "TIMEAVG%V>UC",   &putv->tvta,
         "CMODE%X",                       /* kwret = 1 */
         "FREQUENCY%X",                   /* kwret = 2 */
         "UPROG%X",                       /* kwret = 3 */
         "UFILE%X",                       /* kwret = 4 */
         "UDATA%X",                       /* kwret = 5 */
         "ENDIAN%X",                      /* kwret = 6 */
         "RSLVL%X",                       /* kwret = 7 */
         "OPT%KHFDCAB3ROIP21", &putv->tvuflgs,
         "RSPCT%V>0<100IH",&cam->rspct,
         "X%VF",           &cam->tvplx,
         "TVPLX%VF",       &cam->tvplx,
         "Y%VF",           &cam->tvply,
         "TVPLY%VF",       &cam->tvply,
         "W%VF",           &cam->tvwd,
         "WIDTH%V>F",      &cam->tvwd,
         "MODALITY%TH" qLTF, &hmname,
         "XRSTATS%S1",     &kxrstat,
         NULL)) {
      switch (kwret) {
      case 1:           /* COLORMODE */
         putv->tvkicol = (byte)getcmode();
         break;
      case 2:           /* FREQUENCY */
         putv->tvkfreq = (byte)match(
            RK_EQCHK, RK_REQFLD, ~(RK_BLANK|RK_COMMA), 0,
            gfreqs, sizeof(gfreqs)/sizeof(char *));
         break;
      case 3:           /* UPROG */
         if (!(putv->tvsflgs & TV_UPGM)) ermark(RK_IDERR);
         switch (match(RK_EQCHK, RK_MREQF, ~RK_COMMA,
            RK_INPARENS, uplbls, NUPLBLS)) {
         case 1:           /* TVINIT */
            cam->tvinitfn = (int (*)())d3uprg(FALSE);
            break;
         case 2:           /* TVGRAB */
            cam->tvgrabfn = (int (*)())d3uprg(FALSE);
            break;
         default:          /* Bad name */
            while (RK.scancode & RK_INPARENS) scan(NULL, 0) ;
            scanagn();
            break;
            } /* End UPROG switch */
         break;
      case 4:           /* UFILE */
         if (!(putv->tvsflgs & TV_UPGM)) ermark(RK_IDERR);
         eqscan(&cam->hcamuf, "TH" qTV_MAXUD , RK_EQCHK);
         break;
      case 5:           /* UDATA */
         if (!(putv->tvsflgs & TV_UPGM)) ermark(RK_IDERR);
         eqscan(&hudata, "TH" qTV_MAXUD , RK_EQCHK);
         break;
      case 6:           /* ENDIAN */
         /* Errors already marked by match(), can ignore here */
         switch (match(RK_EQCHK, RK_MREQF, ~(RK_COMMA|RK_ENDCARD), 0,
            endians, NENDIANS)) {
         case 1:           /* LITTLE-ENDIAN */
            cam->tviflgs &= ~TV_BigE;
            break;
         case 2:           /* BIG-ENDIAN */
            cam->tviflgs |= TV_BigE;
            break;
            } /* End ENDIAN switch */
         break;
      case 7:           /* RSLVL = rescale levels */
         scanck(lea, RK_REQFLD,
            ~(RK_COMMA|RK_INPARENS|RK_RTPAREN|RK_ENDCARD));
         if (RK.scancode & RK_INPARENS) {
            scanagn();
            inform("(SV(3V<=1>0B14UH.2V))", cam->rslvl, NULL);
            }
         else
            wbcdin(lea, cam->rslvl, (FBim<<RK_BS)|(2<<RK_DS)|
               RK_CTST|RK_NUNS|RK_NHALF|SCANLEN-1);
         break;
         }  /* End kwret switch */
      }  /* End kwret while */

   /* Error if both 'F' and 'C' options selected */
   if ((putv->tvuflgs & (TV_IFIT|TV_ICTR)) == (TV_IFIT|TV_ICTR))
      cryout(RK_E1, "0***OPTIONS F,C ARE INCOMPATIBLE.", RK_LN2, NULL);

   /* If MODALITY or XRSTATS option given, link up to
   *  a modality block.  If mname not given here, use
   *  "TV" as a default.  */
   if (kxrstat || hmname) {
      struct MODALITY *pmdlt;    /* Pointer to modality info */
      if (!hmname) hmname = savetxt("TV");
      pmdlt = findmdlt(getrktxt(hmname), 0);
      if (pmdlt) {
         cam->ptvmdlt = pmdlt;
         pmdlt->mdltflgs |= MDLTFLGS_RQST; }
      } /* End processing modality */

   /* Use entered raw image sizes unless a driver changes them */
   putv->tvrx = putv->tvrxe, putv->tvry = putv->tvrye;
   /* If this is a UPRG camera, be sure the program names
   *  were entered, then call the user initializer program.
   *  This program is allowed to set tvrx,tvry,tvkicol so
   *  postpone derived sizes and sanity check until below.  */
   if (putv->tvsflgs & TV_UPGM) {
      if (!cam->tvgrabfn) cryout(RK_E1, "0***TVGRAB "
         "FUNCTION NOT FOUND", RK_LN2, NULL);
      if (!cam->tvinitfn) cryout(RK_E1, "0***TVINIT "
         "FUNCTION NOT FOUND", RK_LN2, NULL);
      else {
         int uirc;

/*---------------------------------------------------------------------*
**                           ****WARNING**                            **
*  When we decide to allow one run to read multiple tiff images, the   *
*  part of tvinitfn that checks the image size and color will need to  *
*  be moved to tvgrabfn where it will be called from d3imgt(). What is *
*  missing to keep us from doing this right now is that we have no     *
*  mechanism to specify different image file names in different trials.*
*  Meanwhile, values of tvr[xy], tvr[xy]o returned by tvinitfn must    *
*  not be used or changed from here to d3imgt.                         *
**                         ***END WARNING***                          **
*---------------------------------------------------------------------*/

         /* tvinitfn may want to work from entered tvkicol and
         *  corresponing tvipxsz, but redo this setting below in
         *  case tvinitfn sets the tvkicol it wants to use.  */
         putv->tvipxsz = ipxsz[putv->tvkicol & TVC_MASK];
         uirc = cam->tvinitfn(putv, getrktxt(cam->hcamnm),
            cam->hcamuf ? getrktxt(cam->hcamuf) : NULL,
            hudata ? getrktxt(hudata) : NULL, TV_INIT);
         if (uirc) {
            convrt("(E1,'0***TVINIT FUNCTION RETURNED ERROR "
               "CODE ',J0I6)", &uirc, NULL);
            RK.iexit |= 1;
            }
         }
      }

#ifdef BBDS
   else if (putv->tvsflgs & TV_BBD) {
      /* Register with the BBD package */
      int kcol = (int)putv->tvkicol & BBDColMask;
      if (cam->tviflgs & TV_BigE) kcol |= BBDBig;
      putv->ptvsrc = bbdsregt(getrktxt(cam->hcamnm),
         hhname ? getrktxt(hhname) : NULL,
         (int)putv->tvrx, (int)putv->tvry,
         kcol, (int)putv->tvkfreq);
      /* Set BBDMdlt flag if assigned to a modality */
      if (cam->ptvmdlt && putv->ptvsrc /*JIC*/ )
         ((BBDDev *)putv->ptvsrc)->bdflgs |= BBDMdlt;
      if (putv->tvkfreq == UTVCFr_EVENT)
         RP0->needevnt |= NEV_CAM;
      }
#endif

   /* Now derive the final input and output colors from values that
   *  may have been modified by the tvinitfn.  If an impossible color
   *  option was selected, exit now */
   putv->tvkocol = omode[putv->tvkicol & TVC_MASK];
   if (putv->tvkocol == BM_BAD) {
      cryout(RK_E1, "0***INVALID COLOR MODE", RK_LN2, NULL);
      return;
      }
   putv->tvipxsz = ipxsz[putv->tvkicol & TVC_MASK];
   putv->tvopxsz = bmpxsz((int)putv->tvkocol);
   putv->tvncol  = qColored(putv->tvkocol) ? NColorDims : 1;

   /* Derive amount of space to allocate for max expected input image.
   *  As of R78, this is derived from tvr[xy]e.  Actual images will
   *  have to be no larger and ltvrxy will change accordingly.  */
#if ZSIZE < 8
   if ((ui32)putv->tvrxe > SI32_MAX/
         ((ui32)putv->tvrye*(ui32)putv->tvipxsz))
      d3lime("d3g1imin", PLcb(PLC_TVSZ)+putv->icam);
#endif
   putv->ltvrxy = (size_t)putv->tvrxe * (size_t)putv->tvrye *
      (size_t)putv->tvipxsz;

   /* Derive maximum size of intermediate image for allocation and
   *  some sanity checks.  Note that this space will not actually be
   *  allocated unless d3tchk or d3vchk finds this camera is named as
   *  source for some network input or plot.  Actual values will be
   *  determined in d3imgt for each image and may be smaller.  */
   {  ui16 avgrat = (ui16)putv->tvsa;
      putv->tvix = (putv->tvrxe + avgrat - 1)/avgrat;
      putv->tviy = (putv->tvrye + avgrat - 1)/avgrat;
      putv->ltvixy = (size_t)putv->tvix * (size_t)putv->tviy *
         (size_t)putv->tvopxsz;
      }  /* End avgrat local scope */

   /* Compute and save image size seen by network */
#if ZSIZE < 8
   if ((ui32)putv->tvx > SI32_MAX/
         ((ui32)putv->tvy*(ui32)putv->tvopxsz))
      d3lime("d3g1imin", PLcb(PLC_TVSZ)+putv->icam);
#endif
   if (putv->tvix > RP0->mxtvixy) RP0->mxtvixy = putv->tvix;
   if (putv->tviy > RP0->mxtvixy) RP0->mxtvixy = putv->tviy;
   putv->ntvxy = (ui32)putv->tvx * (ui32)putv->tvy;
   putv->ltvxy = (size_t)putv->ntvxy * (size_t)putv->tvopxsz;

   /* Set tviflgs to tell d3nset and d3imgt what image reduction ops
   *  are needed (TV_Filt bit is added in d3schk, TV_GfmC in d3snsa).
   *  But (as of R78) it is not known until d3imgt whether size fitting
   *  is needed and indeed some images may need it and some not, so
   *  d3vchk and d3snsa are now set to always allocate this space.  */
   if (putv->tvipxsz < 2) cam->tviflgs &= ~TV_Swap;
#if BYTE_ORDRE > 0      /* Big-endian, flip TW_BigE bit-->TV_swap */
   else                   cam->tviflgs ^= TV_BigE;
#endif
   if (putv->tvta > 1) {
      /* Code will be:  cam->tviflgs |= TV_TAvg but error for now.
      *  Maybe best plan is to implement only in VVTV real driver.  */
      cryout(RK_E1, "0***IMAGE TIME AVERAGING IS NOT CURRENTLY "
         "IMPLEMENTED.", RK_LN2, NULL);
      }
   if (putv->tvipxsz > putv->tvopxsz) cam->tviflgs |= TV_Reduc;
   if (putv->tvsa > 1) cam->tviflgs |= TV_SpAvg;
   if (putv->tvuflgs & TV_RESCL) {
      cam->tviflgs |= TV_Scale,     RP0->n0flags |= N0F_TVRSCL;
      if (putv->tvuflgs & TV_RESC3) {
         if (putv->tvncol != 3) cryout(RK_E1, "0***OPT=3 INVALID "
            "WITH GRAYSCALE IMAGES.", RK_LN2, NULL);
         else {
            RP0->n0flags |= N0F_TVRSCC;
            if (cam->rslvl[1] == 0) cam->rslvl[1] = cam->rslvl[0];
            if (cam->rslvl[2] == 0) cam->rslvl[2] = cam->rslvl[1];
            }
         }
      }

   /* If plotting is requested, mark the camera as used directly */
   if (putv->tvuflgs & TV_PLOT)
      putv->tvsflgs |= (TV_ON|TV_ONDIR);

   } /* End g1cam() */


/*=====================================================================*
*                               g1prep                                 *
*                   Process a PREPROC control card                     *
*  Note:  Preprocessing is currently set up to be performed on Node 0  *
*  in a parallel computer, basically because it would be difficult to  *
*  arrange to handle the overlap between pixels belonging to adjacent  *
*  kernels if the work were to be split over multiple comp nodes.  It  *
*  may be useful to change this (or employ high-speed image processing *
*  hardware) if this becomes a bottleneck.                             *
*                                                                      *
*  Note:  In anticipation of wanting preprocessors that work on the    *
*  output of other preprocessors, they are linked here in a LIFO list  *
*  and then rethreaded in d3schk to the source camera on another LIFO  *
*  list, thereby restoring the original control-card order.            *
*                                                                      *
*  Per CNS doc, nkern is only needed with certain UPROG preprocessors. *
*  R74, 06/01/17, GNR - Preprocessors no longer have COLORMODE option, *
*     but OPT=G will cause them to use gray-from-color input.          *
*     nk[xy] and nip[xy] may be needed for UPROGs--if entered for ker- *
*     nel types d3schk will be sure they agree with defaults computed  *
*     from npp[xy] and nk[xy].  npp[xy] will be set equal to tv[xy] if *
*     not entered here.                                                *
*=====================================================================*/

void g1prep(void) {

   struct PREPROC *pip;
   ui32 ic;                   /* kwscan field recognition codes */
   int kwret;                 /* kwscan return code */
   txtid thipnm;              /* Temporary name handle */

   /* PREPROC user function types -- at the moment, just one */
#define NUPRFN 1
   static char *uprfn[NUPRFN] = { "PREPROC" };

   /* Get preprocessor name and give error if it is a duplicate
   *  (but process the card anyway to detect any other errors).  */
   inform("(SW1,TH" qLTF ")", &thipnm, NULL);
   if (findpp(thipnm, NULL))
      dupmsg("PREPROCESSOR", NULL, thipnm);
   /* Create PREPROC struct and initialize it.  The PREPROC structure
   *  is currently not needed on comp nodes but is shared to accommo-
   *  date possible future code that would need it there.  */
   pip = (struct PREPROC *)
      allocpcv(Static, 1, IXstr_PREPROC|MBT_Unint, "PREPROC info");
   pip->hipnm = thipnm;
   prepdflt(pip);             /* Set defaults */
   pip->pnip = RP->pfip;      /* Link into list */
   RP->pfip = pip;
   /* Read optional positional size parameters */
   inform("(S=2*V>IH)", &pip->upr.nppx, &pip->upr.nppy, NULL);
   while (kwret = kwscan(&ic, /* Assignment intended */
         "CAMERA%TH" qTV_MAXNM,  &pip->hcamnm,
         "KERNEL%TH" qTV_MAXNM,  &pip->hkernm,
         "UPROG%X",
         "BC%X",
         "NIPX%V>UH",         &pip->upr.nipx,
         "NIPY%V>UH",         &pip->upr.nipy,
         "NPPX%V>UH",         &pip->upr.nppx,
         "NPPY%V>UH",         &pip->upr.nppy,
         "NKERN%V>UH",        &pip->upr.nker,
         "NKX%V>UH",          &pip->upr.nkx,
         "NKY%V>UH",          &pip->upr.nky,
         "XOFF%UH",           &pip->upr.oipx,
         "YOFF%UH",           &pip->upr.oipy,
         "OPT%KCRDG2OP",      &pip->upr.ipuflgs,
         "X%VF",              &pip->ipplx,
         "PPPLX%VF",          &pip->ipplx,
         "Y%VF",              &pip->ipply,
         "PPPLY%VF",          &pip->ipply,
         "W%V>F",             &pip->ipwd,
         "WIDTH%V>F",         &pip->ipwd,
         "DCOFF%UH",          &pip->dcoff,
         "DCOLS%V>UC",        &pip->dcols,
         NULL)) {
      switch (kwret) {
      case 1:              /* UPROG */
         switch (match(0, RK_MREQF, ~RK_COMMA, RK_INPARENS,
            uprfn, NUPRFN)) {
         case 1:              /* PREPROC function */
            pip->pipfn = (int (*)())d3uprg(FALSE);
            pip->ipsflgs |= PP_UPGM;
            break;
         default: while (RK.scancode & RK_INPARENS) scan(NULL, 0);
            scanagn();
            break;
            }
         break;
      case 2:              /* BC (Boundary Condition) */
         pip->ipkbc = (byte)readbc((int)0x4d);
         break;
         } /* End kwret switch */
      } /* End kwret while */

/* Error checking that can be done before this preprocessor is
*  attached to a camera (see d3schk and d3imgt for checking that
*  can only be done after preprocessor is united with its camera
*  and kernels, e.g. values of nipx,nipy,nkx,nky.  */

   if (pip->hkernm) pip->ipsflgs |= PP_KERN;
   switch (pip->ipsflgs & (PP_KERN|PP_UPGM)) {
   case 0:
      /* Error if neither KERNEL nor UPROG entered.  */
      cryout(RK_E1, "0***KERNEL OR UPROG MUST BE ENTERED",
         RK_LN2, NULL);
      break;
   case PP_UPGM:
      /* User program--nker is quietly set to 1 if not entered.  */
      if (pip->upr.nker <= 0) pip->upr.nker = 1;
      break;
   case PP_KERN:
      /* Kernel without UPGM -- no tests at present */
   case (PP_KERN|PP_UPGM):
      /*  It is valid to enter both --the user program may want to
      *   access the kernel(s).  None of the above errors applies.  */
      break;
      } /* End ipsflgs switch */

   /* If plotting is requested, mark the preprocessor as used */
   if (pip->upr.ipuflgs & PP_PLOT)
      pip->ipsflgs |= PP_ON;

   /* If density histo is requested, ask preprocessor to store info */
   if (pip->upr.ipuflgs & PP_HIST)
      pip->ipsflgs |= PP_GETN;

   } /* End g1prep() */


/*---------------------------------------------------------------------*
*                Auxiliary functions used with g1kern                  *
*---------------------------------------------------------------------*/

/* Allocate kernel space using dimensions in temp kernel */

static KNTYPE *allokern(KNTYPE *ptker) {
   KNTYPE *pnker;                /* Ptr to new space */
   size_t lkrn;
   /* JIC test for unlikely overflow, so no fancy message */
   ui32 lxy = (ui32)ptker->nkx * (ui32)ptker->nky;
   ui32 lxx = 2*(ui32)ptker->nky;
   ui32 lmx = (UI32_MAX - sizeof(KNTYPE))/sizeof(si32);
   if (lxy > lmx-lxx || lxy+lxx > lmx/(ui32)ptker->nker) {
      ermark(RK_NUMBERR); return NULL; }
   ptker->nkxy = lxy + lxx;
   lkrn = sizeof(si32)*(size_t)(lxy+lxx)*(size_t)ptker->nker +
      sizeof(KNTYPE);
   pnker = (KNTYPE *)allocpmv(Host, lkrn, "Preproc kernel");
   if (pnker) *pnker = *ptker;   /* Copy the core kernel info */
   return pnker;
   } /* End allokern() */

/* Little function to be sure we never use an all-zero kernel */

static void ckqnz(si32 qnz) {
   if (qnz == 0) {
      cryout(RK_E1, "0***KERNEL HAS NO NONZERO ELEMENTS",
         RK_LN2, NULL);
      }
   } /* End ckqnz() */

/* Little function to plot kernels (in color) if DBG_PPKERN is on */
#ifndef PAR
static void pltkern(KNTYPE *pker) {

   si32 *pk1,*pke,*pk = &pker->kval;
   byte *pkdbg,*pkd,*pkd1;
   char *kname = getrktxt(pker->hknm);
   size_t nmap = 3 * (size_t)pker->nkx * (size_t)pker->nky;
   si32 mxe,mxi,nxu;
   int ik,iky;
   pkdbg = mallocv(nmap, "Preproc kernel debug");
   /* Scan kernel for max and min entries */
   for (ik=0; ik<(int)pker->nker; ++ik) {
      mxe = mxi = 0;
      pk1 = pk;
      memset(pkdbg, 0, nmap);
      for (iky=0; iky<(int)pker->nky; ++iky) {
         pk++;
         nxu = *pk++;
         pke = pk + nxu;
         while (pk<pke) {
            si32 kv = *pk++;
            if (kv > mxe) mxe = kv;
            if (kv < mxi) mxi = kv;
            }
         }
      /* Scan again and draw */
      mxe >>= 8;
      mxi = abs32(mxi) >> 8;
      if (mxe == 0) mxe = 1;  /* Avoid divide-by-0 error */
      if (mxi == 0) mxi = 1;
      pk = pk1;
      pkd1 = pkdbg;
      for (iky=0; iky<(int)pker->nky; ++iky) {
         pkd = pkd1;
         pkd += 3*(*pk++);
         nxu = *pk++;
         pke = pk + nxu;
         while (pk<pke) {
            si32 kv = *pk++;
            if (kv > 0) {
               kv = (kv+S15)/mxe;
               pkd[1] = (kv > 255) ? 255 : (byte)kv;
               }
            else if (kv < 0) {
               kv = (abs32(kv)+S15)/mxi;
               pkd[0] = (kv > 255) ? 255 : (byte)kv;
               }
            pkd += 3;
            }
         pkd1 += 3*(int)pker->nkx;
         }
      pk++;  /* Skip over the kernel size count */
      /* Now make the plot.  (This code will not work on a parallel
      *  computer, where all the other graphics calls are set up to
      *  run in parallel, but then, what do you expect from debug
      *  code?  Also, the newplt return code is ignored--otherwise
      *  this call should go through the d3ngph wrapper.  */
      newplt(3.0, 3.0, 0.0, 0.0, "DEFAULT", "CYAN", "CLIP", 0);
      bitmaps(pkdbg, (int)pker->nkx, (int)pker->nky, 0.5, 0.5,
         2.0, 2.0, 0, 0, (int)pker->nkx, (int)pker->nky,
         BM_C24|BM_NSME, GM_SET);
      pencol("BLUE");
      rect(0.5, 0.5, 2.0, 2.0, 0);
      symbol(0.5, 0.25, 0.21, kname, 0, strlen(kname));
      } /* End kernel loop */
   freev(pkdbg, "Kernel debug");
   } /* End pltkern */
#endif

/*=====================================================================*
*                               g1kern                                 *
*                    Process a KERNEL control card                     *
*                                                                      *
*  Design notes (R74, 05/12/17):                                       *
* --This is for the case of mathematically defined filter kernels.     *
*  User can write a plug-in function if what is needed is not here.    *
* --It needs to be possible to create families of kernels, typically   *
*  to handle different orientations, under one KERNEL card and struct. *
* --It also needs to be possible to enter multiple kernels under one   *
*  KERNEL card and have them convolved together (applied as a unit     *
*  to an image).  It should be possible to convolve a single kernel    *
*  with a family of kernels already entered. These provisions require  *
*  that it be possible to specify dimensions for the individual kern-  *
*  els on their definition cards (new). Convolutions will also require *
*  temporary allocations in this code.                                 *
* --Given that each kernel definition card will now do its own alloc,  *
*  it is no problem for STG2H2 kernels to implicitly set nker to 4.    *
* --For now, all convolutions will assume zero boundary conditions.    *
*  This code will have to be rewritten if other cases are needed.      *
* --The present arrangment does not allow multiple kernels to be       *
*  concatenated, rather than convolved, under one name (except via     *
*  the -BOX card).  Not clear how that would ever be useful.           *
*                                                                      *
*  The new control card formats implemented here are thus:             *
*  KERNEL %kernm                                                       *
*  -BOX %nkx %nky %nker %data                                          *
*  -DOG %nkx %nky %wexc %sigexc %winh %siginh                          *
*  -EDGE %nkx %nky %nker %wexc %dexc %winh %dinh [%ang1] [%angincr]    *
*  -STG2H2 %nkx %nky %sigxy                                            *
*=====================================================================*/

enum ktype { KT_BOX, KT_DOG, KT_EDGE, KT_STGH, NKTYPES };
void g1kern(void) {

   KNTYPE tker;            /* Temporary kernel struct */
   KNTYPE *pker;           /* Ptr to output kernel struct */
   KNTYPE *pkc1,*pkc2;     /* Ptrs to two kernels in a convolution */
   double dSik = (double)(1 << FBik);
   ui32 qbad = FALSE;      /* If TRUE, scan but do not store */
   char lea[SCANLEN1];     /* Scanned data */
   static char *ktypes[NKTYPES] =
      { "-BOX", "-DOG", "-EDGE", "-STGH2" };

   tker.pnkn = NULL;
   inform("(SW1TH" qTV_MAXNM ")", &tker.hknm, NULL);
   qbad |= RK.erscan;

   /* Error if kernel name is a duplicate */
   if (findkn(tker.hknm, NULL)) {
      qbad = TRUE; dupmsg("KERNEL", NULL, tker.hknm); }
   e64dec(OVF_KCON);

/* Loop reading kernel definitions */

   pker = NULL;                  /* No alloc yet */
   while (1) {
      si32 *pk,*pk1,*pke;        /* Ptrs to kernel data */
      double nkxeven;            /* 0.5 if nkx is an even number */
      si32 qnz;                  /* Nonzero data flag */
      int  ikt;                  /* Kernel type */
      if (!cryin()) goto KERNEL_END;
      cdscan(RK.last, 0, SCANLEN, 0);
      ikt = match(RK_NMERR, RK_MREQF, ~RK_COMMA, 0, ktypes, NKTYPES);
      if (ikt <= 0) { rdagn(); break; }   /* Type not matched */
      cdprt1(RK.last);
      /* Get nkx,nky (same position on all kernel descriptor cards) */
      inform("(S,2V~UH)", &tker.nkx, NULL);
      qbad |= RK.erscan;

      /* If qbad is TRUE, either because given sizes were ridiculous,
      *  or because allocpmv failed, we still go through the motions
      *  of reading the data cards without storing anything, just to
      *  try to keep aligned in the control file and find more errors.
      *  Note:  We do not check for overflowing the lkrn allocation
      *  because each case has careful loop controls.  */
      qnz = 0;
      switch ((enum ktype)(ikt-1)) {

      case KT_BOX: {          /* Explicit user box kernel */
         int  iknl,irow;      /* Kernel,row counters */
         inform("S,V~UH)", &tker.nker, NULL);
         if ((qbad |= RK.erscan)) { skip2end(); continue; }
         /* Parms OK, allocate kernel and read data */
         pkc2 = allokern(&tker);
         if ((qbad |= pkc2 == NULL)) { skip2end(); continue; }
         pkc2->kflgs = IPK_BOX;
         for (iknl=0; iknl<pkc2->nker; ++iknl) {
            pk = &pkc2->kval + iknl*pkc2->nkxy;
            for (irow=0; irow<(int)pkc2->nky; ++irow) {
               pk1 = pk;
               pke = (pk += 2) + pkc2->nkx;
               while (pk < pke) {
                  scanck(lea, RK_NEWKEY|RK_SLDELIM,
                     ~(RK_COMMA|RK_SLASH));
                  if (RK.scancode == RK_ENDCARD || cntrl(lea))
                     goto KERNEL_END;
                  wbcdin(lea, pk, FBik<<RK_BS|RK_IORF|RK_CTST|RK_NI32);
                  qnz |= *pk++;  /* qnz != 0 if any point != 0 */
                  if (RK.scancode & RK_SLASH) break;
                  } /* End column loop */
               pk1[1] = pk - pk1 - 2;
               /* This is the offset for a short row:  one for every
               *  two missing kernel entries */
               pk1[0] = ((si32)pkc2->nkx - pk1[1]) >> 1;
               } /* End row loop */
            } /* End nker loop */
         if (qnz < 0) pkc2->kflgs |= IPK_NEGS;
         ckqnz(qnz);
         } /* End KT_BOX local scope */
         break;

      case KT_DOG: {          /* Difference of Gaussians */
         /* The created kernel has diameter = max(nkx,nky) */
         double wexc,vexc,winh,vinh,dx,dxc,dxe,dy,dyc,dy2,d2,r2;
         inform("(S,Q,V~Q,Q,V~Q)", &wexc, &vexc, &winh, &vinh, NULL);
         if ((qbad |= RK.erscan)) continue;
         tker.nker = 1;
         pkc2 = allokern(&tker);
         if ((qbad |= pkc2 == NULL)) continue;
         pkc2->kflgs = IPK_DOG;
         pk = &pkc2->kval;
         nkxeven = 0.5*(double)(pkc2->nkx & 1 ^ 1);
         vexc = -0.5/(vexc*vexc), vinh = -0.5/(vinh*vinh);
         dxc = 0.5*((double)pkc2->nkx - 1.0);
         dyc = 0.5*((double)pkc2->nky - 1.0);
         dy2 = dyc*dyc;
         r2 = dxc*dxc; if (dy2 > r2) r2 = dy2;
         for (dy=-dyc; dy<=dyc; dy+=1.0) {
            dy2 = dy*dy;
            dxe = floor(sqrt(r2 - dy2)) + nkxeven;
            *pk++ = (si32)(dxc - dxe);
            *pk++ = (si32)(2*dxe + 1.1);
            for (dx=-dxe; dx<=dxe; dx+=1.0) {
               d2 = dx*dx + dy2;
               *pk = (si32)(dSik*
                  (wexc*exp(vexc*d2) - winh*exp(vinh*d2)));
               qnz |= *pk++;
               } /* End x loop */
            } /* End y loop */
         if (qnz < 0) pkc2->kflgs |= IPK_NEGS;
         ckqnz(qnz);
         } /* End KT_DOG local scope */
         break;

      case KT_EDGE: {         /* Series of edge detectors */
         /* One kernel is generated for each value of nker at intervals
         *  incr (default: 2pi/nker).  Each has diameter max(nkx,nky).
         */
         double wexc,dexc,winh,dinh,dx,dxc,dxe,dy,dyc,dy2,d,r2;
         double ang,dang,ca,sa;
         int ik;
         inform("S,V~UH)", &tker.nker, NULL);
         if ((qbad |= RK.erscan)) continue;
         ang = 0.0;           /* Default starting angle and incr */
         dang = 360.0/(double)tker.nker;
         inform("(S,Q,VQ,Q,VQ,NQ,V>Q)", &wexc, &dexc, &winh, &dinh,
            &ang, &dang, NULL);
         if ((qbad |= RK.erscan)) continue;
         pkc2 = allokern(&tker);
         if ((qbad |= pkc2 == NULL)) continue;
         pkc2->kflgs = IPK_EDGE;
         ang *= DEGS_TO_RADS;
         dang *= DEGS_TO_RADS;
         dexc = (dexc != 0.0) ? -1.0/dexc : 0.0;
         dinh = (dinh != 0.0) ?  1.0/dinh : 0.0;
         nkxeven = 0.5*(double)(pkc2->nkx & 1 ^ 1);
         dxc = 0.5*((double)pkc2->nkx - 1.0);
         dyc = 0.5*((double)pkc2->nky - 1.0);
         dy2 = dyc*dyc;
         r2 = dxc*dxc; if (dy2 > r2) r2 = dy2;
         for (ik=0; ik<pkc2->nker; ang+=dang,++ik) {
            ca = cos(ang), sa = sin(ang);
            pk = &pkc2->kval + ik*pkc2->nkxy;
            for (dy=-dyc; dy<=dyc; dy+=1.0) {
               dy2 = dy*dy;
               dxe = floor(sqrt(r2 - dy2)) + nkxeven;
               *pk++ = (si32)(dxc - dxe);
               *pk++ = (si32)(2*dxe + 1.1);
               for (dx=-dxe; dx<=dxe; dx+=1.0) {
                  d = dy*ca + dx*sa;
                  *pk = (si32)(dSik*((d >= 0) ?
                     wexc*exp(dexc*d) : -winh*exp(dinh*d)));
                  qnz |= *pk++;
                  } /* End x loop */
               } /* End y loop */
            if (qnz < 0) pkc2->kflgs |= IPK_NEGS;
            ckqnz(qnz);
            } /* End edge angle loop */
         } /* End KT_EDGE local scope */

      case KT_STGH: {         /* Steerable G2,H2 filters */
         /* Here we build in what is likely to be the only case of
         *  steered filters actually used, the G and H filters at 0
         *  and 90 deg.  They will be in order G2(0), G2(90), H2(0),
         *  H2(90).  scl = 1/sigxy is x,y units per pixel.  Numerical
         *  constants are from the Freeman & Adelson paper.  */
         double dex,dexg,dexh,x0,x,x2,x3,xmx,y0,y,y2,y3,ymx,scl,sigxy;
         /* Offsets between filters */
         ui32 ofgz,ofgn,ofhz,ofhn;     /* Offsets betwenn filters */
         inform("(S,V~Q)", &sigxy, NULL);
         if ((qbad |= RK.erscan)) continue;
         tker.nker = 4;
         pkc2 = allokern(&tker);
         if ((qbad |= pkc2 == NULL)) continue;
         pkc2->kflgs = (IPK_STGH|IPK_NEGS);
         ofgz = 0, ofgn = pkc2->nkxy;
         ofhz = 2*ofgn, ofhn = ofhz + ofgn;
         pk = &pkc2->kval;
         scl = 1.0/sigxy;
         x0 = -0.5*scl*((double)pkc2->nkx - 1.0);
         xmx = fabs(x0) + 0.1*scl;
         y0 = -0.5*scl*((double)pkc2->nky - 1.0);
         ymx = fabs(y0) + 0.1*scl;
         for (y=y0; y<ymx; y+=scl) {
            y2 = y*y, y3 = y2*y;
            pk[ofgz] = pk[ofgn] = pk[ofhz] = pk[ofhn] = 0;
            pk += 1;
            pk[ofgz] = pk[ofgn] = pk[ofhz] = pk[ofhn] = pkc2->nkx;
            pk += 1;
            for (x=x0; x<xmx; ++pk,x+=scl) {
               x2 = x*x, x3 = x2*x;
               dex = exp(-(x2+y2));
               dexg = 0.9213*dex, dexh = 0.9780*dex;
               pk[ofgn] = (si32)(dSik*(2.0*x2 - 1.0)*dexg);
               pk[ofgz] = (si32)(dSik*(2.0*y2 - 1.0)*dexg);
               pk[ofhn] = (si32)(dSik*(x3 - 2.254*x)*dexh);
               pk[ofhz] = (si32)(dSik*(y3 - 2.254*y)*dexh);
               } /* End x loop */
            } /* End y loop */
         } /* End KT_STGH local scope */
      /* More cases can be added here as needed, for example,
      *  individual G or H filters at nonstandard angles. */
         } /* End kernel type switch */

/* Now either store an only kernel or convolve with earlier one.
*  Note:  Our method of compressing short rows is not compatible with
*  finding locations for sums of products.  First store convolution
*  uncompressed, then compress result in place.  Kernels are small
*  enough that fft method is not faster.  */

      if (pker) {
         si32 *pkr1,*pkr2;          /* Output row ptrs */
         si32 *qkc1,*qkc2;          /* Output column ptrs */
         si32 *qk1,*qk2;            /* Initial data ptrs */
         si32 *qw1,*qe1,*qw2,*qe2;  /* Working data ptrs */
         si32 nc1,nc2;              /* Columns in current rows */
         si32 r1,r2;                /* Kernel 1 and 2 row counts */
         si32 qk2inc;               /* Kernel 2 data increment */
         /* Convolve new kernel(s) with existing one(s).
         *  Make existing kernel the one to be convolved */
         pkc1 = pker;
         RP0->pfkn = pker->pnkn;    /* Drop pker from list */
         /* Check that numbers of kernels are compatible */
         if ((qbad |= (pkc1->nker != pkc2->nker &&
               (pkc1->nker|pkc2->nker) != 1))) {
            cryout(RK_E1,"0***KERNEL SIZES ARE NOT COMPATIBLE "
               "FOR CONVOLUTION", RK_LN2, NULL);
            continue;
            }
         /* Make pkc1 have the definitive nker */
         if (pkc1->nker < pkc2->nker)
            pker = pkc1, pkc1 = pkc2, pkc2 = pker;
         /* Derive dimensions of convolved kernels */
         tker.hknm = pkc1->hknm;
         tker.nker = pkc1->nker;
         tker.nkx = pkc1->nkx + pkc2->nkx - 1;
         tker.nky = pkc1->nky + pkc2->nky - 1;
         /* Allocate space for convolved kernels
         *  and put result at start of list.  */
         pker = allokern(&tker);
         if ((qbad |= pker == NULL)) continue;
         pker->pnkn = RP0->pfkn;
         pker->kflgs = pkc1->kflgs | pkc2->kflgs | IPK_CONV;
         RP0->pfkn = pker;
         /* Clear convolution sums */
         {  size_t nall = (size_t)pker->nker * (size_t)pker->nkxy;
            pk1 = &pker->kval, pke = pk1 + nall;
            memset(pk1, 0, sizeof(si32)*nall);
            } /* End nall local scope */
         qk2inc = pkc2->nker > 1 ? pkc2->nkxy : 0;
         qk1 = &pkc1->kval, qk2 = &pkc2->kval;
         for ( ; pk1<pke; pk1+=pker->nkxy) {             /* Kernels */
            pkr1 = pk1 + 2*pker->nky;
            for (qw1=qk1,r1=0; r1<pkc1->nky; ++r1) {     /* K1 rows */
               qkc1 = pkr1 + *qw1++;
               nc1 = *qw1++;
               qe1 = qw1 + nc1;
               for ( ; qw1<qe1; ++qkc1,++qw1) {          /* K1 cols */
                  pkr2 = qkc1;
                  qw2 = qk2;
                  for (r2=0; r2<pkc2->nky; ++r2) {       /* K2 rows */
                     qkc2 = pkr2 + *qw2++;
                     nc2 = *qw2++;
                     qe2 = qw2 + nc2;
                     for ( ; qw2<qe2; ++qkc2,++qw2) {    /* K2 cols */
                        /* Finally compute and add the term... */
                        si32 prod = mrssld(*qw1, *qw2, FBik);
                        si32 osum = *qkc2;
                        jasjdm(*qkc2, osum, prod);
                        } /* End column 2 loop */
                     pkr2 += pker->nkx;
                     } /* End row 2 loop */
                  } /* End column 1 loop */
               pkr1 += pker->nkx;
               } /* End row 1 loop */

            /* Compress the result back to pk1 */
            pkr1 = (pkr2 = pk1) + 2*pker->nky;
            for (r1=0; r1<pker->nky; ++r1) {
               qw1 = pkr1 + pker->nkx + 1, qe1 = qw1 + 1;
               while (*pkr1 == 0) pkr2[0] += 1, ++pkr1;
               while (*qw1 == 0)  pkr2[1] += 1, --qw1;
               pkr2 += 2;
               if (pkr2 != pkr1)
                  while (pkr2 <= qw1) *pkr2++ = *pkr1++;
               pkr1 = qe1;
               }

            qk1 += pkc1->nkxy;
            qk2 += qk2inc;
            }
         freepv(pkc1, "Preproc kernel");
         freepv(pkc2, "Preproc kernel");
         } /* End convolution of two kernels */

      else {
         /* This is the first or only kernel */
         pker = RP0->pfkn = pkc2;
         }

      } /* End loop over kernels */

#ifndef PAR
   if (RP->CP.dbgflags & DBG_PPKERN) pltkern(pker);
#endif
   return;

KERNEL_END:
   cryout(RK_E1, "0***EXPECTED KERNEL DATA MISSING",
      RK_LN2, NULL);
   return;
   } /* End g1kern() */

