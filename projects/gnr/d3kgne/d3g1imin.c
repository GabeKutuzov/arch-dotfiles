/* (c) Copyright 2010, Neurosciences Research Foundation, Inc. */
/* $Id: d3g1imin.c 42 2011-01-03 21:37:10Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                             d3g1imin.c                               *
*                                                                      *
*  This file contains the g1cam, g1kern, and g1prep functions, which   *
*  are called by d3grp1() to interpret options on the CAMERA, KERNEL,  *
*  and PREPROC control cards relating to image preprocessing.          *
*                                                                      *
*  Kernel storage is arranged as follows for rapid run-time processing:*
*  Each row begins with (1) the positive offset to the first element   *
*  to be processed, to which the image row size is added for each row  *
*  after the first at execution time, to allow the same kernel to be   *
*  applied to images of different size, (2) the number of elements to  *
*  be processed in that row, and (3) the actual data in FS24 format.   *
*  The end of the kernel is followed by the entry count.               *
************************************************************************
*  V8F, 04/15/10, GNR - New routines, move CAMERA input from d3grp1    *
*  ==>, 05/15/10, GNR - Last mod before committing to svn repository   *
*  Rev, 07/26/10, GNR - Reverse EDGE x due to left-handed image coords *
*  V8H, 11/07/10, GNR - Implement BBD SENSE and CAMERA modalities      *
***********************************************************************/

#define TVTYPE  struct TVDEF
#define KNTYPE  struct KERNEL
#define PPTYPE  struct PREPROC

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "sysdef.h"
#include "d3global.h"
#ifdef BBDS
#include "bbd.h"
#endif
#include "tvdef.h"
#include "rocks.h"
#include "rkxtra.h"
#include "bapkg.h"
#ifndef PAR
#include "plots.h"
#endif

static char *cmodes[] = {  /* Order as in bbd.h */
   "GRAYSCALE", "8-BIT", "16-BIT", "24-BIT" };

/*=====================================================================*
*                                g1cam                                 *
*                 Process a CAMERA or TV control card                  *
*=====================================================================*/

void g1cam(void) {

   struct TVDEF *cam;
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

   /* Terminal error if too many cameras */
   if (RP->ntvs >= BYTE_MAX)
      d3exit(LIMITS_ERR, "TV", BYTE_MAX);

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

   /* Note: The TVDEF structure is shared to comp nodes so
   *  that the plotting parameters are available to tvplot()
   *  and to accommodate possible implementations of vvgrab()
   *  that would bring picture tiles to different nodes.  */
   cam = *RP0->pltv = (struct TVDEF *)
      allocpcv(Static, 1, IXstr_TVDEF|MBT_Unint, "tv");
   RP0->pltv = &cam->pnxtv;
   cam->utv.icam = ++RP->ntvs;   /* Count cameras */
   tvdflt(cam);                  /* Set defaults */
   /* Insert name and type flags scanned earlier */
   cam->hcamnm = thcamnm;
   cam->utv.tvsflgs = tflgs;

   /* Read TV options */
   inform("(S=K,2*UH)", &cam->utv.tvx, &cam->utv.tvy, NULL);
   while (kwret = kwscan(&ic, /* Assignment intended */
         "TVX%V>UH",       &cam->utv.tvx,
         "TVY%V>UH",       &cam->utv.tvy,
         "TVIX%V>UH",      &cam->utv.tvix,
         "TVIY%V>UH",      &cam->utv.tviy,
         "SLOT%UC",        &cam->utv.islot,
         "SPACEAVG%V>UC",  &cam->utv.tvsa,
         "TIMEAVG%V>UC",   &cam->utv.tvta,
         "COLORMODE%X",
         "FREQUENCY%X",
         "UPROG%X",
         "UFILE%X",
         "UDATA%X",
         "OPT%KCBROIP21",  &cam->utv.tvuflgs,
         "RSPCT%V>0<100IH",&cam->rspct,
         "RSLVL%V>0<256IH",&cam->rslvl,
         "X%VF",           &cam->tvplx,
         "TVPLX%VF",       &cam->tvplx,
         "Y%VF",           &cam->tvply,
         "TVPLY%VF",       &cam->tvply,
         "W%VF",           &cam->tvwd,
         "WIDTH%V>F",      &cam->tvwd,
         "XOFF%UH",        &cam->utv.tvxo,
         "YOFF%UH",        &cam->utv.tvyo,
         "MODALITY%TH" qLTF, &hmname,
         "XRSTATS%S1",     &kxrstat,
         NULL)) {
      switch (kwret) {
      case 1:           /* COLORMODE */
         cam->utv.tvkcol = (byte)match(
            RK_EQCHK, RK_REQFLD, ~(RK_BLANK|RK_COMMA), 0,
            cmodes, sizeof(cmodes)/sizeof(char *)) - 1;
         break;
      case 2:           /* FREQUENCY */
         cam->utv.tvkfreq = (byte)match(
            RK_EQCHK, RK_REQFLD, ~(RK_BLANK|RK_COMMA), 0,
            gfreqs, sizeof(gfreqs)/sizeof(char *));
         break;
      case 3:           /* UPROG */
         if (!(cam->utv.tvsflgs & TV_UPGM)) ermark(RK_IDERR);
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
         if (!(cam->utv.tvsflgs & TV_UPGM)) ermark(RK_IDERR);
         eqscan(&cam->hcamuf, "TH" qTV_MAXUD , RK_EQCHK);
         break;
      case 5:           /* UDATA */
         if (!(cam->utv.tvsflgs & TV_UPGM)) ermark(RK_IDERR);
         eqscan(&hudata, "TH" qTV_MAXUD , RK_EQCHK);
         break;
         }  /* End kwret switch */
      }  /* End kwret while */

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

   /* If this is a UPRG camera, be sure the program names
   *  were entered, then call the user initializer program.
   *  This program is allowed to set tvix,tviy, so postpone
   *  sanity check until below.  */
   if (cam->utv.tvsflgs & TV_UPGM) {
      if (!cam->tvgrabfn) cryout(RK_E1, "0***TVGRAB "
         "FUNCTION NOT FOUND", RK_LN2, NULL);
      if (!cam->tvinitfn) cryout(RK_E1, "0***TVINIT "
         "FUNCTION NOT FOUND", RK_LN2, NULL);
      else {
         int uirc = cam->tvinitfn(&cam->utv, getrktxt(cam->hcamnm),
            cam->hcamuf ? getrktxt(cam->hcamuf) : NULL,
            hudata ? getrktxt(hudata) : NULL, TV_INIT);
         if (uirc) {
            convrt("(P1,'0***TVINIT FUNCTION RETURNED ERROR "
               "CODE ',J0I6)", &uirc, NULL);
            RK.iexit |= 1;
            }
         }
      }

#ifdef BBDS
   else if (cam->utv.tvsflgs & TV_BBD) {
      /* Register with the BBD package */
      cam->utv.ptvsrc = bbdsregt(getrktxt(cam->hcamnm),
         hhname ? getrktxt(hhname) : NULL,
         (int)cam->utv.tvx, (int)cam->utv.tvy,
         (int)cam->utv.tvkcol, (int)cam->utv.tvkfreq);
      /* Set BBDMdlt flag if assigned to a modality */
      if (cam->ptvmdlt && cam->utv.ptvsrc /*JIC*/ )
         ((BBDDev *)cam->utv.ptvsrc)->bdflgs |= BBDMdlt;
      if (cam->utv.tvkfreq == UTVCFr_EVENT)
         RP0->needevnt |= NEV_CAM;
      }
#endif

   /* Perform sanity check on image size parameters */
   if (cam->utv.tvxo + cam->utv.tvx > cam->utv.tvix ||
         cam->utv.tvyo + cam->utv.tvy > cam->utv.tviy)
      cryout(RK_E1, "0***x,y OFFSETS PLUS tvx,tvy "
         "EXCEED RAW IMAGE SIZE", RK_LN2, NULL);

   if (cam->utv.tvuflgs & TV_RESCL) {
      if (cam->utv.tvkcol != UTVCol_GS &&
            cam->utv.tvkcol != UTVCol_C24)
         cryout(RK_E1, "0***IMAGE RESCALE R OPTION IS NOT "
            "COMPATIBLE WITH 8 OR 16 BIT COLOR", RK_LN2, NULL);
      else
         RP0->n0flags |= N0F_TVRSCL;
      }

   /* If plotting is requested, mark the camera as used directly */
   if (cam->utv.tvuflgs & TV_PLOT)
      cam->utv.tvsflgs |= (TV_ON|TV_ONDIR);

   } /* End g1cam() */


/*=====================================================================*
*                               g1kern                                 *
*                    Process a KERNEL control card                     *
*=====================================================================*/

/* Little function to be sure we never store a zero sum count */

static si32 cktnsum(si32 tnsum) {
   if (tnsum == 0) {
      cryout(RK_E1, "0***KERNEL HAS NO NONZERO ELEMENTS",
         RK_LN2, NULL);
      tnsum = 1;
      }
   return tnsum;
   } /* End cktnsum() */

/* Little function to plot kernels (in color) if DBG_PPKERN is on */
#ifndef PAR
static void pltkern(struct KERNEL *pker, byte *pkdbg) {

   si32 *pk1,*pke,*pk = &pker->kval;
   byte *pkd,*pkd1;
   char *kname = getrktxt(pker->hknm);
   size_t nmap = 3 * (size_t)pker->nkx * (size_t)pker->nky;
   si32 mxe,mxi,nxu;
   int ik,iky;
   for (ik=0; ik<(int)pker->nker; ++ik) {      /* Scan kernel for max and min entries */
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
      mxi = abs(mxi) >> 8;
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
               kv = (abs(kv)+S15)/mxi;
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
      *  code?  */
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

void g1kern(void) {

   struct KERNEL tker;        /* Temporary kernel struct */
   struct KERNEL *pker;       /* Ptr to kernel struct */
   si32 *pk,*pk1,*pke;        /* Ptrs to kernel data */
#ifndef PAR
   byte *pkdbg = NULL;        /* Space for debug images */
#endif
   double nkxeven;            /* 0.5 if nkx is an even number */
   ui32 lxy,lxx,lmx;          /* Temps for length(KERNEL) calc */
   ui32 tnsum;                /* Temp for entry count */
   int  ik,ikt;               /* Kernel number and type */
   char lea[SCANLEN1];        /* Scanned data */
#define NKTYPES 3
#define KT_BOX  1
#define KT_DOG  2
#define KT_EDGE 3
   char *ktypes[NKTYPES] = { "-BOX", "-DOG", "-EDGE" };

   tker.pnkn = NULL;
   tker.nker = 1;             /* Default is one kernel per card */
   inform("(SW1TH" qTV_MAXNM ",2V>UH,NV>UH)",
      &tker.hknm, &tker.nkx, &tker.nker, NULL);
   /* Preset to skip over input if alloc or dup name error */
   pker = &tker, pk = NULL;

   /* Just-in-case test for unlikely overflow, so no fancy message */
   lxy = (ui32)tker.nkx * (ui32)tker.nky;
   lxx = 2*(ui32)tker.nky + 1;
   lmx = (UI32_MAX - sizeof(struct KERNEL))/sizeof(si32);
   if (lxy > lmx-lxx || lxy+lxx > lmx/(ui32)tker.nker)
      ermark(RK_NUMBERR);
   /* Error if kernel name is a duplicate */
   else if (findkn(tker.hknm, NULL))
      dupmsg("KERNEL", NULL, tker.hknm);
   else {
      /* Note:  There is no reason to maintain the kernel list in
      *  the input order, so a simple LIFO list is made.  */
      size_t lkrn = sizeof(si32)*(size_t)(lxy+lxx)*(size_t)tker.nker +
         sizeof(struct KERNEL);
      struct KERNEL *pkrm = (struct KERNEL *)mallocv(lkrn, "Kernel");
      if (pkrm) {
         *pkrm = tker;           /* Copy the core kernel info */
         pkrm->pnkn = RP0->pfkn; /* Add to start of linked list */
         pker = RP0->pfkn = pkrm;
         pk = &pker->kval;
#ifndef PAR
         if (RP->CP.dbgflags & DBG_PPKERN)
            pkdbg = mallocv(3*lxy, "Kernel debug");
#endif
         nkxeven = 0.5*(double)(pker->nkx & 1 ^ 1);
         }
      }
   /* If pk is NULL, either because given sizes were ridiculous,
   *  or because mallocv failed, we still go through the motions
   *  of reading the data cards without storing anything, just to
   *  try to keep aligned in the control file.  */
   for (ik=(int)pker->nker; ik>0; --ik) {
      if (!cryin()) goto KERNEL_END;
      cdprt1(RK.last);
      cdscan(RK.last, 0, SCANLEN, 0);
      if (!pk) { skip2end(); continue; }
      ikt = match(0, RK_MREQF, ~RK_COMMA, 0, ktypes, NKTYPES);
      switch (ikt) {
      case 0:                 /* Type not matched */
         skip2end();
         break;
      case KT_BOX: {          /* Explicit user box kernel */
         int  irow;           /* Row counter */
         tnsum = 0;
         for (irow=0; irow<(int)pker->nky; ++irow) {
            pk1 = pk;
            pke = (pk += 2) + pker->nkx;
            while (pk < pke) {
               scanck(lea, RK_NEWKEY|RK_SLDELIM,
                  ~(RK_COMMA|RK_SLASH));
               if (RK.scancode == RK_ENDCARD) goto KERNEL_END;
               wbcdin(lea, pk, 24<<RK_BS|RK_IORF|RK_CTST|RK_NI32);
               tnsum += 1;
               pk += 1;
               if (RK.scancode & RK_SLASH) break;
               } /* End column loop */
            pk1[1] = pk - pk1 - 2;
            pk1[0] = ((si32)pker->nkx - pk1[1]) >> 1;
            } /* End row loop */
         *pk++ = cktnsum(tnsum);
         } /* End KT_BOX local scope */
         break;
      case KT_DOG: {          /* Difference of Gaussians */
         /* The card recognized here has the format:
         *  DOG weight-excit sigma-excit weight-inhib sigma-inhib
         *  and the created kernel has diameter = max(nkx,nky).  */
         double wexc,vexc,winh,vinh,dx,dxc,dxe,dy,dyc,dy2,d2,r2;
         inform("(S,Q,V~Q,Q,V~Q)", &wexc, &vexc, &winh, &vinh, NULL);
         vexc = -0.5/(vexc*vexc), vinh = -0.5/(vinh*vinh);
         dxc = 0.5*((double)pker->nkx - 1.0);
         dyc = 0.5*((double)pker->nky - 1.0);
         dy2 = dyc*dyc;
         r2 = dxc*dxc; if (dy2 > r2) r2 = dy2;
         tnsum = 0;
         for (dy=-dyc; dy<=dyc; dy+=1.0) {
            dy2 = dy*dy;
            dxe = floor(sqrt(r2 - dy2)) + nkxeven;
            *pk++ = (si32)(dxc - dxe);
            *pk++ = (si32)(2*dxe + 1.1);
            for (dx=-dxe; dx<=dxe; dx+=1.0) {
               d2 = dx*dx + dy2;
               *pk++ = (si32)(dS24*
                  (wexc*exp(vexc*d2) - winh*exp(vinh*d2)));
               tnsum += 1;
               } /* End x loop */
            } /* End y loop */
         *pk++ = cktnsum(tnsum);
         } /* End KT_DOG local scope */
         break;
      case KT_EDGE: {         /* Series of edge detectors */
         /* The card recognized here has the format:
         *  EDGE wt-exc d-foff-exc wt-inh d-foff-inh [start] [incr]
         *  One kernel is generated for each unused value of nker
         *  at incr (default: 2pi/nker) intervals.  Each has diameter
         *  max(nkx,nky).
         */
         double wexc,dexc,winh,dinh,dx,dxc,dxe,dy,dyc,dy2,d,r2;
         double ang,dang,ca,sa;
         ang = 0.0;           /* Default starting angle and incr */
         dang = 360.0/(double)ik;
         inform("(S,Q,VQ,Q,VQ,NQ,V>Q)", &wexc, &dexc, &winh, &dinh,
            &ang, &dang, NULL);
         ang *= DEGS_TO_RADS;
         dang *= DEGS_TO_RADS;
         dexc = (dexc != 0.0) ? -1.0/dexc : 0.0;
         dinh = (dinh != 0.0) ?  1.0/dinh : 0.0;
         dxc = 0.5*((double)pker->nkx - 1.0);
         dyc = 0.5*((double)pker->nky - 1.0);
         dy2 = dyc*dyc;
         r2 = dxc*dxc; if (dy2 > r2) r2 = dy2;
         for ( ; ik>0; ang+=dang,--ik) {
            ca = cos(ang), sa = sin(ang);
            tnsum = 0;
            for (dy=-dyc; dy<=dyc; dy+=1.0) {
               dy2 = dy*dy;
               dxe = floor(sqrt(r2 - dy2)) + nkxeven;
               *pk++ = (si32)(dxc - dxe);
               *pk++ = (si32)(2*dxe + 1.1);
               for (dx=-dxe; dx<=dxe; dx+=1.0) {
                  d = dy*ca + dx*sa;
                  *pk++ = (si32)(dS24*((d >= 0) ?
                     wexc*exp(dexc*d) : -winh*exp(dinh*d)));
                  tnsum += 1;
                  } /* End x loop */
               } /* End y loop */
            *pk++ = cktnsum(tnsum);
            } /* End edge angle loop */
         } /* End KT_EDGE local scope */
      /* More cases can be added here as needed */
         } /* End kernel type switch */
      } /* End loop over kernels */
#ifndef PAR
   if (RP->CP.dbgflags & DBG_PPKERN) pltkern(pker, pkdbg);
#endif
   return;

KERNEL_END:
   cryout(RK_E1, "0***EXPECTED KERNEL DATA MISSING",
      RK_LN2, NULL);
   return;
   } /* End g1kern() */


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
   /* Create PREPROC struct and initialize it.  Note:  The PREPROC
   *  structure is shared to comp nodes so that the plotting para-
   *  meters are available to tvplot() on parallel computers.  */
   pip = (struct PREPROC *)
      allocpcv(Static, 1, IXstr_PREPROC|MBT_Unint, "PREPROC info");
   pip->hipnm = thipnm;
   prepdflt(pip);             /* Set defaults */
   pip->pnip = RP->pfip;      /* Link into list */
   RP->pfip = pip;
   /* Read optional positional size parameters */
   inform("(S=2*V>IH)", &pip->upr.nipx, &pip->upr.nipy, NULL);
   while (kwret = kwscan(&ic, /* Assignment intended */
         "CAMERA%TH" qTV_MAXNM,  &pip->hcamnm,
         "KERNEL%TH" qTV_MAXNM,  &pip->hkernm,
         "COLORMODE%X",
         "UPROG%X",
         "NPPX%V>UH",         &pip->upr.nipx,
         "NPPY%V>UH",         &pip->upr.nipy,
         "NKERN%V>UH",        &pip->upr.nker,
         "NKX%V>UH",          &pip->nkx,
         "NKY%V>UH",          &pip->nky,
         "XOFF%UH",           &pip->upr.oipx,
         "YOFF%UH",           &pip->upr.oipy,
         "OPT%KC2OP",         &pip->upr.ipuflgs,
         "X%VF",              &pip->ipplx,
         "PPPLX%VF",          &pip->ipplx,
         "Y%VF",              &pip->ipply,
         "PPPLY%VF",          &pip->ipply,
         "W%VF",              &pip->ipwd,
         "WIDTH%V>F",         &pip->ipwd,
         "DCOFF%V~F",         &pip->dcoff,
         "DCOLS%UC",          &pip->dcols,
         NULL)) {
      switch (kwret) {
      case 1:              /* COLORMODE */
         pip->upr.ipkcol = (byte)match(
            RK_EQCHK, RK_REQFLD, ~(RK_BLANK|RK_COMMA), 0,
            cmodes, sizeof(cmodes)/sizeof(char *)) - 1;
         break;
      case 2:              /* UPROG */
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
         } /* End kwret switch */
      } /* End kwret while */

/* Error checking that depends on whether this is PP_UPGM or PP_KERN */

   if (pip->hkernm) pip->ipsflgs |= PP_KERN;
   switch (pip->ipsflgs & (PP_KERN|PP_UPGM)) {
   case 0:
      /* Error if neither KERNEL nor UPROG entered.  */
      cryout(RK_E1, "0***KERNEL OR UPROG MUST BE ENTERED",
         RK_LN2, NULL);
      break;
   case PP_UPGM:
      /* User program--kernel size not needed and nker quietly
      *  set to 1 if not entered.  */
      if (pip->upr.nker <= 0) pip->upr.nker = 1;
      break;
   case PP_KERN:
      /* If this uses a stored kernel and no UPROG, color mode must
      *  be either grayscale or 24-bit color. */
      if (pip->upr.ipkcol != Col_GS && pip->upr.ipkcol != Col_C24)
         cryout(RK_E1, "0***BUILT-IN KERNELS ARE ONLY VALID "
            "WITH GRAYSCALE OR 24-BIT COLOR", RK_LN2, NULL);
      break;
   case (PP_KERN|PP_UPGM):
      /*  It is valid to enter both --the user program may want to
      *   access the kernel(s).  None of the above errors applies.  */
      break;
      } /* End ipsflgs switch */

   /* If plotting is requested, mark the preprocessor as used */
   if (pip->upr.ipuflgs & PP_PLOT)
      pip->ipsflgs |= PP_ON;

   } /* End g1prep() */
