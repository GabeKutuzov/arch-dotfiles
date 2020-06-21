/* (c) Copyright 2018, The Rockefeller University *11115* */
/* $Id: d3imgt.c 78 2018-08-02 18:36:58Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                              d3imgt.c                                *
*                                                                      *
*  This routine is called from the darwin3 main program to read images *
*  of all implemented types and preprocess them if required.  It is    *
*  called separately for series-frequency and trial-frequency cameras. *
*                                                                      *
*  Argument:  UTVCFr_TRIAL, UTVCFr_EVENT, or UTVCFr_SERIES indicating  *
*     which kind of time period the call is for.                       *
*  Return value:  0 if normal, nonzero indicates OUT_USER_QUIT request *
*     received from a BBD camera.                                      *
*  Error action:  All errors result in calls to d3exit.                *
*                                                                      *
*  Design notes:  It is very likely that in any one run, only one of   *
*  the available camera interfaces will be used.  Therefore, it was    *
*  deemed not worthwhile to thread the cameras separately by type.     *
*                                                                      *
*  Current design assumes that only UPROG cameras can use individual   *
*  image sizes to return different values for tvr[xy] and tvr[xy]o.    *
*  If needed, code for BBD drivers (but first BBD driver interface)    *
*  can be modified to carry out this function.  Presumably VVTV is a   *
*  real attached camera that always returns same image size.           *
*                                                                      *
*  As of R78, the image-processing pipeline is modified to handle      *
*  variable-size input images using the following scheme and rules:    *
*  (1) Memory allocation will be fixed and based on tvr[xy]e entered   *
*     or defaulted, divided by tvsa for later stages.  Images can be   *
*     smaller, not larger.  This is not likely ever to need realloc(). *
*  (2) Images are read with a user program (TV_UPGM), BBD client       *
*     (TV_BBD), or hardware driver (TV_VVTV, not implemented) on PAR0  *
*     in a parallel computer.  If image is larger than the allocated   *
*     space, it will be cropped.  If TV_RCTR is set, the part used is  *
*     centered on the full image, otherwise specified offsets tvr[xy]o *
*     are used.  If byte-swapping and/or reduction of large pixels to  *
*     16 bits is required, it is done at this stage.                   *
*  (3) Integer-ratio spatial averaging can be done at this stage to    *
*     minimize later processing burden (tvsa).  Gray-scale remapping,  *
*     if requested (TV_Scale), is also done at this stage.  The image  *
*     size is now tvix x tviy (computed from tvr[xy]/tvsa). This image *
*     is input to virtual touch (vitp) and any preprocessors.          *
*  (4) TV_Filt preprocessing has its own output size spec (npp[xy])    *
*     and offsets (oip[xy]) which default to centering.  If image is   *
*     too small (nipx[y]) an error is given.                           *
*  (5) Code that supplies images to networks (full color, gray, or     *
*     TV_GfmC) must provide a constant image of size (tvx x tvy) to    *
*     work with Lij selection.  We provide the following options:      *
*  (5A) User may want image scale to remain unchanged.  Larger images  *
*     will be cropped using tv[xy]o offsets or centered if TV_ICTR.    *
*     Smaller images will get a border added, zeros by default, other- *
*     wise computed provisionally from adjacent background if TV_ADJB. *
*  (5B) User may want to rescale image size to fit tvx x tvy.  TV_IFIT *
*     will request this.  Scaling will be bilinear--a fancier inter-   *
*     polation will be programmed if needed.                           *
*  (6) Program can now handle mixed colored+gray images if camera is   *
*     defined as colored so early code allocates enough memory for     *
*     colored images.  Then code here will use tvkicol returned by     *
*     image acquisition to decide whether color pixels are present.    *
*                                                                      *
*  The key idea for memory management in this code is that there are   *
*  three image storage areas that are used in succession as the image  *
*  size changes during processing: (1) There is an input area praw,    *
*  (2) an intermediate work area pmyim, and (3) an output area (pbcst+ *
*  cam->otvi) where fully processed images are stored for broadcast to *
*  networks on a parallel computer.  (Preprocessor outputs and gray-   *
*  from-color have their own broadcast areas.)  The last process puts  *
*  its output in area (3) so copying is never needed.  Area (2) is     *
*  needed if larger than area (3) (to avoid excess broadcast length)   *
*  or if more than one output function (image size fitting, preproces- *
*  sor(s), or gray-from-color) use the same image input.  Area (1) is  *
*  the same as area (2) unless it is larger, in which case space in    *
*  psws is used.                                                       *
*                                                                      *
*     Color modes C-8 and C-16 are defined and handled on input cards, *
*  but there is currently no code here to process them.  Can be added  *
*  if needed--possibly using the TV_Reduc code to expand to C-24.      *
*                                                                      *
*  The idea that UPROG preprocessors should be allowed to access raw   *
*  images has been left for a future version--would involve complex    *
*  bookkeeping on numbers of rows and columns, possible color mode     *
*  change, access to alpha channel, more data in UPREPR struct, etc.   *
*  Need for any of this is not apparent.                               *
*                                                                      *
*  N.B.  As currently implemented, this code is called only on Node 0  *
*  of a parallel computer.  Obsolete VVTV drivers ran in parallel. If  *
*  a new VVTV is implemented and needs access on all nodes, this code  *
*  and the call in main must be reviewed for node specificity.         *
************************************************************************
*  V8F, 04/25/10, GNR - New routine, some code moved here from main    *
*  ==>, 05/14/10, GNR - Last mod before committing to svn repository   *
*  R74, 06/08/17, GNR - Add more image modes, pixel size reduction     *
*  R75, 09/24/17, GNR - Add code for NOPVP (angle of max pix to Sj)    *
*  R78, 04/09/18, GNR - Correct tests for moving output to pmyim,      *
*                       allow ltvrxy, ltvixy to change per image,      *
*                       implement TV_RESCL and new TV_IFIT and TV_ICTR *
*  R78, 05/26/18, GNR - Correct TV_SpAvg code to handle colored images *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "sysdef.h"
#include "d3global.h"
#include "celldata.h"
#include "lijarg.h"
#include "tvdef.h"
#include "rocks.h"
#include "rkxtra.h"
#include "rkarith.h"

/* Pound-define STGHINFO to print highest preprocessor outputs */
/*** #define STGHINFO ***/

/* Pound-define DBGIMGS to draw images at various stages (!PAR) */
/*** #define DBGIMGS ***/
#ifdef DBGIMGS
#include "plots.h"
#endif

extern struct CELLDATA CDAT;
#ifndef PAR0
extern struct LIJARG Lija;
#endif

int d3imgt(int freq) {

   struct TVDEF *cam;         /* Ptr to current camera */
   struct PREPROC *pip;       /* Ptr to current preprocessor */
   int grc = 0;               /* bbdsgett return code  */

/*---------------------------------------------------------------------*
*                          Fetch the images                            *
*---------------------------------------------------------------------*/

   /* These flags allow testing of frequency-interface combo bits */
   int fshft3 = 3*freq;
   ui16 flgmsk = (freq == UTVCFr_TRIAL) ?
      (RP->kevtu & (EVTVTV|EVTBTV|EVTUTV)) : ~0;
   ui16 uflgs = (TV_UPGM & flgmsk) << fshft3;    /* UPGM at this time */
#ifdef VVTV
   ui16 vflgs = (TV_VVTV & flgmsk) << fshft3;    /* VVTV at this time */
#endif
   ui16 aflgs = ((TV_VVTV|TV_BBD|TV_UPGM) & flgmsk) << fshft3; /* ANY */

#if defined(BBDS) || defined(VVTV)
/* BBD cameras have their own loop in bbdsgett() and there is
*  one vvgchk/vvgrab sequence for all VVTV cameras, hence these
*  calls are outside the camera loop below.  This switch could
*  be eliminated by putting the bbdsgett() and vvgchk() calls
*  back in darwin3 main, but this is cleaner.  */

   switch (freq) {
   case UTVCFr_TRIAL:
#ifdef BBDS
      if (RP->kevtu & EVTBTV)
         grc = bbdsgett(BBDCFr_TRIAL);
#endif
#ifdef VVTV
      if (RP->kevtu & EVTVTV) {
         int rc = vvgchk(OFF);
         if (rc) d3exit("VVGCHK", VIDEO_ERR, rc);
         }
#endif
      break;
   case UTVCFr_EVENT:
#ifdef BBDS
      if (RP->ortvfl & TV_EvtBBD)
         grc = bbdsgett(BBDCFr_EVENT);
#endif
#ifdef VVTV
      if (RP->ortvfl & TV_EvtVVTV) {
         int rc = vvgchk(OFF);
         if (rc) d3exit("VVGCHK", VIDEO_ERR, rc);
         }
#endif
      break;
   case UTVCFr_SERIES:
#ifdef BBDS
      if (RP->ortvfl & TV_SerBBD)
         grc = bbdsgett(BBDCFr_SERIES);
#endif
#ifdef VVTV
      if (RP->ortvfl & TV_SerVVTV) {
         int rc = vvgchk(OFF);
         if (rc) d3exit("VVGCHK", VIDEO_ERR, rc);
         }
#endif
      break;
      } /* End frequency switch */
#endif

/* Loop over active cameras.  For UPROG cameras, must acquire image
*  and preprocess.  For BBD and VVTV cameras, just preprocess.  */

   for (cam=RP->pftv; cam; cam=cam->pnxtv) {

      struct UTVDEF *putv = &cam->utv;
      PIXEL *praw, *pint;     /* Raw, intermediate image locations */
      PIXEL *pr,*pre,*pi;     /* Image loop indexes */
      PIXEL *pfit;            /* Space for IFit output */
      size_t tvcsz;           /* Current image size (bytes) */
      size_t pixsz;           /* Intermediate image pixel size */
#ifdef DBGIMGS
      ui16 *pdb16,*pdb16e;
      int imghi,imglo;
#endif
      int ils, iadv, ncol;

      /* If utv.tsflgs do not match aflgs, skip this camera now */
      if (!(putv->tvsflgs & aflgs)) continue;

/* Set up address for raw image acquisition (praw) and intermediate
*  stage output (pint).  pint is set to the broadcast output location
*  (pbcst+otvi) when there are no more transformations to do.  */

      praw = pint = cam->tviflgs & (TV_Reduc|TV_SpAvg) ?
         (PIXEL *)CDAT.psws : cam->pmyim;

/* Process UPROG cameras.  Note that this call is allowed to change
*  tvr[xy], which are otherwise set from tvr[xy]e in d3g1imin and
*  stay constant throughout the run.  I.e. any image input routine
*  that changes tvr[xy] must change it every time it is called.  */

      if (putv->tvsflgs & uflgs) {
         int rc = cam->tvgrabfn(putv, praw);
         if (rc) d3exit(getrktxt(cam->hcamnm), VIDEO_ERR, rc);

         if (RP->kdprt & PROBJ) convrt("(P4,' Camera ',J1A" qLTF
            ",'loaded object ',J1UH6,'from class ',J0UH6)",
            getrktxt(cam->hcamnm), &putv->isn, &putv->ign,
            NULL);
         }

#ifdef VVTV
/* Process VVTV cameras */

      if (putv->tvsflgs & vflgs) {
         int rc = vvget2(praw, putv->icam, putv->tvkcol,
            cam->tvrxo, cam->tvryo, putv->tvrx, putv->tvry);
         if (rc) d3exit("VVGET2", VIDEO_ERR, rc);
         } /* End if EVTVTV */
#endif

      /* Now adapt for color or gray image as returned by image driver
      *  in tvkicol.  (Currently tvkicol is set once and for all in
      *  d3g1imin, but this code should be ready to allow change for
      *  each new image if needed.  This will require much work, as
      *  various row and image length variables are currently preset.)
      *  Store left shift for pixel size, makes nice simple test too */
      ils = qBytePixels(cam) ? 0 : 1;
      iadv = 1 << ils;        /* Byte size of a pixel or one color */
      putv->tvncol =          /* Number of colors in image */
         qColored(putv->tvkocol) ? NColorDims : 1;
      ncol = (int)putv->tvncol;

      /* Compute tvi[xy] which may now be smaller than tvr[xy]e.
      *  (Note that d1g1imin sets tvrx=tvrxe, tvry=tvrye for use
      *  if driver does not modify according to image size.) */
      pixsz = (size_t)putv->tvopxsz;
      {  ui16 avgrat = (ui16)putv->tvsa;
         putv->tvix = (putv->tvrx + avgrat - 1)/avgrat;
         putv->tviy = (putv->tvry + avgrat - 1)/avgrat; }
      /* Compute and save actual raw and intermediate image sizes.
      *  Note:  Max sizes in tvr[xy]e were already checked for product
      *  overflow in d3g1imin, actual values must be no larger, so
      *  no need to check again for product overflow.  */
      putv->ltvrxy = (size_t)putv->tvrx * (size_t)putv->tvry *
         (size_t)putv->tvipxsz;
      putv->ltvixy = (size_t)putv->tvix * (size_t)putv->tviy * pixsz;
      /* Now that actual sizes are known, we can set for cropping or
      *  rescaling or none of the above (filtering always uses ltvixy
      *  size).  Sad to relate, we cannot just use pbcst+otvi in place
      *  of pmyim when there is no TV_IFIT, because we need to set
      *  praw before image is read and thus before size is known.  So
      *  if no TV_IFit and no intermediate process that can move the
      *  image to otvi space, there will have to be an image copy
      *  instead.  */
      cam->tviflgs &= ~TV_IFit;
      if (putv->tvsflgs & (TV_ONDIR|TV_ONGfmC) &&
            (putv->tvx != putv->tvix || putv->tvy != putv->tviy))
         cam->tviflgs |= TV_IFit;
      /* Set where IFit output goes (skipping TV_ONGfmC test here) */
      pfit = cam->otvi + (PIXEL *)
         ((putv->tvsflgs & TV_ONDIR) ? RP->pbcst : RP0->pipimin);
      tvcsz = putv->ltvrxy;   /* Current size = raw size */

#ifdef DBGIMGS    /*** DEBUG ***/
      /* R78, 04/12/18, GNR - Debug code moved here so it can use
      *  modified values of tvr[xy], ltvrxy returned by drivers.  */
      imghi = 0, imglo = UI16_MAX;
      pdb16 = (ui16 *)praw;
      pdb16e = (ui16 *)(praw + tvcsz);
      while (pdb16<pdb16e) {
         if (*pdb16 > imghi) imghi = *pdb16;
         if (*pdb16 < imglo) imglo = *pdb16;
         ++pdb16;
         }
      convrt("(P1,'0==>imglo = ',J0U8,', imghi = ',J0U8)",
         &imglo, &imghi, NULL);
      newplt(8.5, 8.5, 0, 0, "DEFAULT", "BLACK", "DEFAULT", 0);
      pencol("RED");
      rect(0.4, 0.4, 7.7, 7.7, 0);
      bitmap(praw, (int)putv->tvrx, (int)putv->tvry, 0.5, 0.5,
         0, 0, (int)putv->tvrx, (int)putv->tvry, (int)putv->tvkocol, 0);
      symbol(0.1, 0.1, 0.2, "Initial raw image", 0, 17);
      finplt();
#endif

/*---------------------------------------------------------------------*
*        Swap byte order if image does not map this processor          *
*                                                                      *
*  It doesn't matter whether image or memory is big-endian here...     *
*---------------------------------------------------------------------*/

      if (cam->tviflgs & TV_Swap) {
         /* If this is last step, store output for broadcast */
         if (!(cam->tviflgs & (TV_TAvg|TV_Reduc|TV_SpAvg|TV_Scale|
            TV_Filt|TV_IFit))) pint = pfit;
         else if (!(cam->tviflgs & (TV_Reduc|TV_SpAvg)))
            pint = cam->pmyim;
         pre = praw + tvcsz;
         if (pint == praw) {        /* Tricky way to swap in place */
            switch (putv->tvipxsz) {
            case 2:
               for (pr=praw; pr<pre; pr+=HSIZE)
                  pr[0] ^= pr[1], pr[1] ^= pr[0], pr[0] ^= pr[1];
               break;
            case 4:
               for (pr=praw; pr<pre; pr+=JSIZE) {
                  pr[0] ^= pr[3], pr[3] ^= pr[0], pr[0] ^= pr[3];
                  pr[1] ^= pr[2], pr[2] ^= pr[1], pr[1] ^= pr[2]; }
               break;
            case 8:
               for (pr=praw; pr<pre; pr+=WSIZE) {
                  pr[0] ^= pr[7], pr[7] ^= pr[0], pr[0] ^= pr[7];
                  pr[1] ^= pr[6], pr[6] ^= pr[1], pr[1] ^= pr[6];
                  pr[2] ^= pr[5], pr[5] ^= pr[2], pr[2] ^= pr[5];
                  pr[3] ^= pr[4], pr[4] ^= pr[3], pr[3] ^= pr[4]; }
               break;
               }  /* End size switch */
            }
         else {                     /* Simpler if target differs */
            pi = pint;
            switch (putv->tvipxsz) {
            case 2:
               for (pr=praw; pr<pre; pr+=HSIZE)
                  *pi++ = pr[1], *pi++ = pr[0];
               break;
            case 4:
               for (pr=praw; pr<pre; pr+=JSIZE) {
                  *pi++ = pr[3], *pi++ = pr[2],
                  *pi++ = pr[1], *pi++ = pr[0]; }
               break;
            case 8:
               for (pr=praw; pr<pre; pr+=WSIZE) {
                  *pi++ = pr[7], *pi++ = pr[6],
                  *pi++ = pr[5], *pi++ = pr[4];
                  *pi++ = pr[3], *pi++ = pr[2],
                  *pi++ = pr[1], *pi++ = pr[0]; }
               break;
               }  /* End size switch */
            }  /* End else target differs */
         praw = pint;      /* New source = old target (maybe same) */
         }  /* End byte swap */

/*---------------------------------------------------------------------*
*               Time average goes here when implemented                *
*---------------------------------------------------------------------*/


/*---------------------------------------------------------------------*
*                        Pixel size reduction                          *
*                                                                      *
*  Program now accepts images in a variety of Matlab formats including *
*  32-bit uint and single- and double-precision floating point pixels, *
*  but for internal use, all are reduced to 8 or 16 bit uints.         *
*  This code does not care whether images are gray or color.  Input    *
*  images have been forced to CDAT.psws, so alignment is guaranteed.   *
*  Work can be done in place if other operations follow, otherwise     *
*  output is placed in output slot at cam->pmyim.  If we later add     *
*  code to expand C-8 or C-16 to C-24, new setup tests may be needed.  *
*---------------------------------------------------------------------*/

      if (cam->tviflgs & TV_Reduc) {
         ui16 *pi16;
         /* If this is last step, store output for broadcast */
         if (!(cam->tviflgs & (TV_SpAvg|TV_Scale|TV_Filt|TV_IFit)))
            pint = pfit;
         else if (!(cam->tviflgs & TV_SpAvg)) pint = cam->pmyim;
         pre = praw + tvcsz;
         pi16 = (ui16 *)pint;
         /* Modes that do not require reduction do not come here */
         switch (putv->tvkicol & (TVC_Flt|TVC_B8)) {
         case TVC_B4:            /* Four-byte uint pixels */
            for (pr=praw; pr<pre; pr+=JSIZE) {
               ui32 tpix = *(ui32 *)pr;
               *pi16++ = (ui16)(tpix >> 16);
               }
            break;
         case TVC_B8:            /* Eight-byte uint pixels */
            for (pr=praw; pr<pre; pr+=WSIZE) {
               ui64 tpix = *(ui64 *)pr;
#ifdef HAS_I64
               *pi16++ = (ui16)(tpix >> 48);
#else
               *pi16++ = (ui16)(uwlo(jsruw(tpix, 48)));
#endif
               }
            break;
         case (TVC_Flt|TVC_B4):  /* Four-byte floating point */
            for (pr=praw; pr<pre; pr+=ESIZE) {
               ui32 tpix = (ui32)(*(float *)pr * dS16);
               if (tpix > UI16_MAX) tpix = UI16_MAX;
               *pi16++ = (ui16)tpix;
               }
            break;
         case (TVC_Flt|TVC_B8):  /* Eight-byte floating point */
            for (pr=praw; pr<pre; pr+=DSIZE) {
               ui32 tpix = (ui32)(*(double *)pr * dS16);
               if (tpix > UI16_MAX) tpix = UI16_MAX;
               *pi16++ = (ui16)tpix;
               }
            break;
            }  /* End input color mode switch */
         praw = pint;            /* New source = old target */
         tvcsz = (PIXEL *)pi16 - pint;
         }  /* End pixel mode reduction */

/*---------------------------------------------------------------------*
*                      Perform spatial averaging                       *
*                                                                      *
*  N.B.  This operation reduces the numbers of pixels per row and per  *
*  column, but does not affect the size of each pixel, which is now    *
*  either 8 bits or 16 bits (x3 if colored), cf. reduction, which      *
*  reduces pixel size, but not number.                                 *
*                                                                      *
*  N.B.  If the raw image size is not an even multiple of tvsa in      *
*  either direction, averaging is over whatever data are there.        *
*                                                                      *
*  N.B.  This code loops over the reduced image with byte pointers and *
*  has separate loops for byte and ui16 pixels only in the loop over   *
*  columns in the current box being averaged over.  There are three    *
*  sum/num accumulators for three colors because otherwise the box row *
*  loop would have to repeat three times for colored images.           *
*                                                                      *
*  (This option was always contemplated, but only implemented in R74.  *
*---------------------------------------------------------------------*/

      if (cam->tviflgs & TV_SpAvg) {
         PIXEL *por,*pore,*porei;   /* Ptrs to outer row, row end */
         PIXEL *poc;                /* Ptr to outer column */
         PIXEL *pir,*pire;          /* Ptrs to inner row, row end */
         PIXEL *pavb;               /* Ptr to output (bytes) */
         ui32 rlen;                 /* Length of one row, bytes */
         ui32 bxht,bxwd;            /* Input box height,width (bytes) */
         int  icol;                 /* Index over colors */
         /* If this is last step, store output for broadcast */
         pint = (!(cam->tviflgs & (TV_Scale|TV_Filt|TV_IFit))) ?
            pfit : cam->pmyim;
         pavb = pint;
         rlen = putv->tvrx * (ui32)pixsz;
         bxht = putv->tvsa * rlen;
         bxwd = putv->tvsa * (ui32)pixsz;
         pre = praw + tvcsz;
         /* Loop over boxes, vertically */
         for (por=praw; por<pre; por+=bxht) {
            pore = por + rlen;
            /* Loop over boxes, horizontally */
            for (poc=por; poc<pore; poc+=bxwd) {
               ui32 sum[NColorDims];   /* Pixel data sums */
               /* num is not just tvsa^2--may be less at edges */
               ui32 num[NColorDims];   /* Pixel data numbers */
               /* Probably faster than loop on ncol... */
               sum[0] = sum[1] = sum[2] = 0;
               num[0] = num[1] = num[2] = 0;
               /* Loop over rows within a box */
               porei = pore;
               pire = min(poc + bxht, pre);
               for (pir=poc; pir<pire; pir+=rlen,porei+=rlen) {
                  /* Loop over colors */
                  for (icol=0; icol<ncol; ++icol) {
                     /* Time to separate byte from ui16 pixels--
                     *  put the loop inside the if for a little
                     *  better speed.  */
                     if (ils) {
                        ui16 *pic,*pice;
                        pice = (ui16 *)min(pir + bxwd, porei);
                        for (pic=(ui16 *)pir+icol; pic<pice; pic+=ncol)
                           sum[icol] += (ui32)*pic, ++num[icol];
                        }  /* End ui16 pixels */
                     else {
                        byte *pic,*pice;
                        pice = min(pir + bxwd, porei);
                        for (pic=pir+icol; pic<pice; pic+=ncol)
                           sum[icol] += (ui32)*pic, ++num[icol];
                        }  /* End byte pixels */
                     }  /* End loop over colors */
                  }  /* End vertical loop within a box */
               /* Loop over colors to compute averages and store--
               *  should never get here with num == 0, he said.  */
               for (icol=0; icol<ncol; ++icol) {
                  ui32 tnum = num[icol];
                  ui32 avg = (sum[icol] + (tnum>>1))/tnum;
                  if (ils) *(ui16 *)pavb = (ui16)avg, pavb += HSIZE;
                  else     *pavb++ = (byte)avg;
                  }
               }  /* End horizontal loop over boxes */
            }  /* End vertical loop over boxes */
         praw = pint;
         tvcsz = pavb - pint;

#ifdef DBGIMGS    /*** DEBUG ***/
      newplt(5.0, 5.0, 0, 0, "DEFAULT", "BLACK", "DEFAULT", 0);
      pencol("RED");
      rect(0.4, 0.4, 4.2, 4.2, 0);
      bitmap(pint, (int)putv->tvix, (int)putv->tviy, 0.5, 0.5,
         0, 0, (int)putv->tvix, (int)putv->tviy, (int)putv->tvkocol, 0);
      symbol(0.1, 0.1, 0.2, "After spatial averaging", 0, 23);
      finplt();
#endif
         }  /* End spatial image averaging */

/*---------------------------------------------------------------------*
*                   Perform rescaling (KP=R option)                    *
*                                                                      *
*  This code derives a histogram of image intensities, determines the  *
*  gray level corresponding to a specified percentile, and rescales    *
*  linearly so that graylevel becomes a given percentage of 256.       *
*  Random rounding is applied to the scaled values to eliminate any    *
*  visible step contours in the output.                                *
*                                                                      *
*  If image is colored and option TV_RESC3 was selected, there is a    *
*  separate histogram and rslvl for each color, otherwise one histo-   *
*  gram is made and applied globally.  But rspct is same for all.      *
*                                                                      *
*  Because this rescaling may or may not be applied, we need to leave  *
*  the binary scale of the pixel unchanged, i.e. S8 (byte), S16 (ui16).*
*  FBirs is chosen to allow iscl*pixel(S16) to always fit a ui32.      *
*  FBirs = 12 allows iscl < 16 (S16-S12) ==> itgt > rslvl/4 (S16-S14). *
*---------------------------------------------------------------------*/

      if (cam->tviflgs & TV_Scale) {

/* The following are constants, not parameters:  */
#define GRBITS     8       /* Number of bits in NRSHIST bins */
#define FBirs     12       /* Fraction bits in rescale scale */
#define RRshft  (BITSPERUI32-FBirs-1)  /* Random round shift */
#define NPCT100  100       /* Number of steps in 100 percent */

/* Two levels of indenting suppressed to end of rescale processing */

   ui32  *pih,*pihe;       /* Ptrs to histogram table and its end */
   size_t nrsh;            /* Size of RP0->ptvrsi array (words) */
   long  ntgt,ntgt0;       /* Target gray level offset in histo */
   ui32  iscl[NColorDims]; /* Scales for each (or all) colors */
   int   jcol;             /* Index for color loop */
   int   nhis;             /* Number of histograms */
   int   isii;             /* iscl index increment */

/* Scan through the image and accumulate intensity histogram(s) */

   /* If this is last step, store output for broadcast */
   pint = (!(cam->tviflgs & (TV_Filt|TV_IFit))) ? pfit : cam->pmyim;
   pih = (ui32 *)RP0->ptvrsi;
   ntgt0 = (long)putv->tvix * (long)putv->tviy *
      (NPCT100 - cam->rspct) / NPCT100;
   pre = praw + tvcsz;

   if (putv->tvuflgs & TV_RESC3) {
      /* Make a separate histogram for each color */
      nhis = ncol;
      pihe = pih + (nrsh = ncol * NRSHIST);
      memset((char *)pih, 0, JSIZE*nrsh);
      for (pr=praw; pr<pre; pr+=iadv) {
#if BYTE_ORDRE < 0
         pih[pr[ils]] += 1;
#else
         pih[pr[0]] += 1;
#endif
         pih += NRSHIST;
         if (pih >= pihe) pih = (ui32 *)RP0->ptvrsi;
         }
      }
   else {
      /* Make one histogram with 1 or all 3 colors.  Theoretically,
      *  (but very unlikely) could get an overflow in a 32-bit machine
      *  if all three colors are added into one histogram and the
      *  image is very large and every pixel is same color.  */
#ifndef BIT64
      if (ncol*(size_t)putv->tviy > UI32_MAX/(size_t)putv->tvix)
         d3exit(NULL, TV_SCALE_ERR, (int)cam->icam);
#endif
      ntgt0 *= ncol;
      nhis = 1;
      memset((char *)pih, 0, JSIZE*NRSHIST);
      for (pr=praw; pr<pre; pr+=iadv) {
#if BYTE_ORDRE < 0
         pih[pr[ils]] += 1;
#else
         pih[pr[0]] += 1;
#endif
         }
      }

/* Get scales for 1 or 3 histograms.  (An earlier version of this
*  code computed individual scales for each histogram bin, but
*  now we scale each pixel individually--a little more accurate.  */

   pih = (ui32 *)RP0->ptvrsi;
   for (jcol=0; jcol<nhis; pih+=NRSHIST,++jcol) {
      /* Determine the value of the rspct percentile(s) */
      ui32 itgt = NRSHIST;
      ntgt = ntgt0;
      while (itgt > 0) {
         long newtgt = ntgt - (long)pih[--itgt];
         if (newtgt <= 0) break;
         ntgt = newtgt;
         }
      /* R78, 04/20/18, GNR - Remove "impossible" error exit here */
      /* Expand itgt to S8 integer + fraction, effective 16 bit frac */
      itgt = (itgt << GRBITS) +
         (si32)((ntgt << GRBITS)/(long)pih[itgt]);
      iscl[jcol] = ((ui32)cam->rslvl[jcol] << (FBirs+2))/itgt;
      }  /* End loop over 1 or 3 colors */

/* Make second pass through image, separately for 8-vs 16-bit pixels,
*  and now treating colors individually if requested (isii > 0).
*  Scale each pixel, applying random rounding to eliminate intensity
*  steps in output.  isii gives us a tricky way to step through scales
*  without an 'if' or '%'.  Design choice for 3 colors:  Could have
*  had outer loop over colors, inner loop over pixels, or one loop
*  over all pixels, selecting scale according to color.  Decided best
*  for cache memory was to have one loop over all pixels.  */

   pi = pint;
   isii = nhis >> 1;             /* 0 if one scale, 1 if 3 scales */
   jcol = 0;
   if (ils) {
      for (pr=praw; pr<pre; pr+=HSIZE) {
         ui32 iv = ((ui32)(*(ui16 *)pr)*iscl[jcol] +
            ((ui32)udev(&cam->rsseed) >> RRshft)) >> FBirs;
         if (iv > UI16_MAX) iv = UI16_MAX;
         *(ui16 *)pi = (ui16)iv, pi += HSIZE;
         jcol = (jcol >> 1) + isii & NColorDims;
         }
      }  /* End scaling two-byte pixels */
   else {
      for (pr=praw; pr<pre; ++pr) {
         ui32 iv = ((ui32)(*pr)*iscl[jcol] +
            ((ui32)udev(&cam->rsseed) >> RRshft)) >> FBirs;
         if (iv > BYTE_MAX) iv = BYTE_MAX;
         *pi++ = (PIXEL)iv;
         jcol = (jcol >> 1) + isii & NColorDims;
         }
      }  /* End scaling one-byte pixels */

   praw = pint;
   tvcsz = pi - pint;

#ifdef DBGIMGS    /*** DEBUG ***/
      newplt(5.0, 5.0, 0, 0, "DEFAULT", "BLACK", "DEFAULT", 0);
      pencol("GREEN");
      rect(0.4, 0.4, 4.2, 4.2, 0);
      bitmap(pint, (int)putv->tvix, (int)putv->tviy, 0.5, 0.5,
         0, 0, (int)putv->tvix, (int)putv->tviy, (int)putv->tvkocol, 0);
      symbol(0.1, 0.1, 0.2, "After pixel rescaling", 0, 23);
      finplt();
#endif
   } /* End prescaling */

/* End two levels of indenting suppressed */

/*---------------------------------------------------------------------*
*                 Compute gray level from color image                  *
*   (for input to preprocessing or virtual touch at tvix*tviy size)    *
*                                                                      *
*  This code derives a gray level from each pixel of a color image     *
*  using the formula:  Gray = 0.21*Red + 0.72*Green + 0.07*Blue.       *
*  PPGC output is not calculated unless used, so it is stored at       *
*  pbcst+cam->otvppgc in Node 0 (or serial) storage.                   *
*                                                                      *
*  Bytes per pixel is preserved:  1 or 2.                              *
*  If the image is already gray, this step can be skipped, no error    *
*                                                                      *
*  R78, 04/21/18, GNR - Use final-size output from fitting/cropping.   *
*---------------------------------------------------------------------*/

      if (cam->tviflgs & TV_PPGC && ncol == NColorDims) {
#define RGB2GRnd (1 << (RGB2GScl-1))
         pre = pint + tvcsz;
         if (ils) {
            /* Pixels are 16 bits per color */
            ui16 *pr16, *pre16 = (ui16 *)pre;
            ui16 *po16 = (ui16 *)(RP0->pipimin + cam->otvppgc);
            for (pr16=(ui16 *)pint; pr16<pre16; pr16+=NColorDims) {
               ui32 gray = Red2G*pr16[0] + Grn2G*pr16[1] +
                  Blu2G*pr16[2];
               *po16++ = (ui16)((gray + RGB2GRnd) >> RGB2GScl);
               }
            }
         else {
            /* Pixels are 8 bits per color */
            byte *po8 = RP0->pipimin + cam->otvppgc;
            for (pr=pint; pr<pre; pr+=NColorDims) {
               ui32 gray = Red2G*pr[0] + Grn2G*pr[1] + Blu2G*pr[2];
               *po8++ = (byte)((gray + RGB2GRnd) >> RGB2GScl);
               }  /* End byte pixel loop */
            }

#ifdef DBGIMGS    /*** DEBUG ***/
      newplt(5.0, 5.0, 0, 0, "DEFAULT", "BLACK", "DEFAULT", 0);
      pencol("RED");
      rect(0.4, 0.4, 4.2, 4.2, 0);
      bitmap(RP0->pipimin+cam->otvppgc, (int)putv->tvix,
         (int)putv->tviy, 0.5, 0.5, 0, 0, (int)putv->tvix,
         (int)putv->tviy, (int)col2gr(putv->tvkocol), 0);
      symbol(0.1, 0.1, 0.2, "After color to gray for preproc", 0, 31);
      finplt();
#endif
         }  /* End PP gray-from-color */

/*---------------------------------------------------------------------*
*          Do any preprocessing associated with this camera            *
*                                                                      *
*  Design Notes:  As of R74, kernels from multiple kernel header cards *
*  associated with one preprocessor are convolved at at g1kern() time  *
*  into larger kernels (nker must be equal or one of them == 1), but   *
*  there can still be multiple kernels from one header, such as -STGH  *
*  type.  At present, non -STGH multikernel types are made available   *
*  to connections as a single array of nppx * (nker*nppy) * (1 or 3    *
*  colors), therefore determination of max pixel for conntype normali- *
*  zation is over all of these output pixels.  That must change if     *
*  single kernel or color acces is provided as CONNTYPE controls.      *
*  And one camera can still have multiple preprocessors, leading to    *
*  separate outputs that can act as inputs to connection types, etc.   *
*                                                                      *
*  Looping is over input image, not output image, because must extract *
*  subimage from input via row and column counts, while output is via  *
*  a simple pointer increment.                                         *
*                                                                      *
*  All preprocessor outputs are now 16 bits per color value (S14).     *
*  Negative values are allowed, as these occur with the STGH kernels.  *
*  This reduces the number of sjbr routines and allows S16 trig fns    *
*  for STGH and fewer shifts elsewhere.                                *
*---------------------------------------------------------------------*/

      if (cam->tviflgs & TV_Filt) {
         for (pip=cam->pmyip; pip; pip=pip->pnipc)
            if (pip->ipsflgs & PP_ON) {

/* Three levels of indenting suppressed to end of kernel processing */

   struct UPREPR *pup = &pip->upr;
   PIXEL *pfiltsrc;
   int oshft = ils ? (FBik+2) : (FBik-6); /* Output shift to S14 */
   pup->tncol = min(pup->sncol, (byte)ncol);
   /* Determine source (raw image or gray-from-color) */
   pfiltsrc = (ncol == NColorDims && pup->ipuflgs & PP_GfmC) ?
      RP0->pipimin + cam->otvppgc : praw;
   if (pip->ipsflgs & PP_UPGM) {
      int rc = pip->pipfn(pup, pfiltsrc, RP->pbcst+pip->oppoi);
      if (rc) d3exit(getrktxt(pip->hipnm), PREPROC_ERR, rc);
      }
   else {                  /* Must be kernel type */
      si64 sum,asum;       /* One evaluation, abs(sum) */
      si32 *pkv1,*pkv;     /* Ptrs to kernel values */
      PIXEL *pin0;         /* Ptr to first image pixel used */
      PIXEL *pin1,*pin1e;  /* Ptrs to this,last input for output row */
      PIXEL *pin2,*pin2e;  /* Ptrs to this,last input for output col */
      PIXEL *pin3,*pin3e;  /* Ptrs to this,last input for kernel row */
      PIXEL *pin4,*pin4e;  /* Ptrs to this,last input for kernel col */
      si16 *poh;           /* Ptr to halfword output */
      size_t lirow;        /* Length of nppx pixels on input image */
      size_t lpixsz;       /* pixsz maybe after gray-from-color */
      size_t o2off;        /* Option 2 offset */
      size_t oipxu,oipyu;  /* Used values of oip[xy] */
      size_t tvix3;        /* Byte length of full input row */
      si32 newval;         /* New value of pixel */
      int badfit = FALSE;  /* TRUE if image too small for kernel */
      int ik;              /* Kernel loop index */
      int jkr,jki;         /* Kernel repeat controls */
      int ttx,tty;         /* Temps for oip[xy]u calc and checks */

      /* This must be consistent with checks in d3schk:
      *  When kernel size is even, small half goes on top/left */
      lpixsz = (size_t)pup->tncol * iadv;
      lirow = lpixsz*(size_t)pup->nppx;
      tvix3 = lpixsz*(size_t)putv->tvix;
      pkv1 = &pup->pipk->kval;

      /* R78, 04/19/18, GNR - This code moved here from d3schk to
      *  handle variable-size input images.  If preprocessor image
      *  offsets were not entered, default to half the border space
      *  and error if this is negative.  Note that if the kernel size
      *  is even, the smaller border goes on the left or top.  If odd,
      *  border is same on both sides (center point is on output
      *  grid).  If offsets were entered, only need to check if there
      *  is space on one side of image (top or left edge).  Use int
      *  calcs to defend against ui16 overflows.  */
      ttx = (int)putv->tvix - (int)pup->nipx;
      if (ttx < 0) badfit = TRUE, ttx = 0;
      tty = (int)putv->tviy - (int)pup->nipy;
      if (tty < 0) badfit = TRUE, tty = 0;
      if (pup->oipx == UI16_MAX) oipxu = (size_t)(ttx >> 1);
      else if ((int)pup->oipx > ttx) badfit = TRUE;
      else oipxu = (size_t)pup->oipx;
      if (pup->oipy == UI16_MAX) oipyu = (size_t)(tty >> 1);
      else if ((int)pup->oipy > tty) badfit = TRUE;
      else oipyu = (size_t)pup->oipy;
      if (badfit) d3exit(getrktxt(pip->hipnm), IMTOOSM_ERR, 0);

      /* Compute offset to first input pixel */
      pin0 = pfiltsrc + tvix3*oipyu + lpixsz*oipxu;
      pin1e = pin0 + tvix3 * (size_t)pup->nppy;
      poh = (si16 *)(RP->pbcst + pip->oppoi);
      jki = (int)pip->jkr0 + 1;
      o2off = (pup->ipuflgs & PP_2SYMM) ?
         (size_t)pup->nker * (size_t)pup->lppi3 : 0;

      /* Loop over kernels.  This will recycle over the input but
      *  continue filling the output array.  Control of repeat use
      *  of kernel for multicolor input is "cute" code designed to
      *  avoid an "if" in the inner loop.  But I have not yet coded
      *  separation of colors when color mode is Col_C16 and that
      *  mode is forbidden at startup time--new code needed here
      *  for Col_C16.  We can use gvin_fn to read any legal color
      *  input, but we do not have the corresponding code to write
      *  any color.  Probably each color mode should have its own
      *  loop here.  */

      for (ik=0; ik<pup->nker; ++ik) {
#ifdef STGHINFO
         si16 *bigpoh0 = poh;
         int bigabs = 0, nover = 0, bigoff = 0;
#endif
#ifdef DBGIMGS    /*** DEBUG ***/
   si16 *pohdbg = poh;
   si16 *sim16,*sim16e;
   si32 simlo,simhi;
   int tnkx = (int)pup->nkx;
   convrt("(P1,' -->Kernel ',I4)", &ik, NULL);
   convrt("(P1#R?,(3X,2IJ5,RB24IJ8.6)/)", &pup->pipk->nkxy,
      &tnkx, pkv1, NULL);
#endif
         /* Loop over nppy image rows.  Note that pin1,pin2 point
         *  to ULHC of convolution area, not its center */
         for (pin1=pin0; pin1<pin1e; pin1+=tvix3) {
            /* Loop over nppx image columns */
            pin2e = pin1 + lirow;
            for (pin2=pin1; pin2<pin2e; pin2+=iadv) {
               sum = jesl(0);
               pkv = pkv1;
               jkr = (int)pip->jkr0;
               /* Loop over kernel rows */
               pin3e = pin2 +
                  (size_t)pup->nky * tvix3;
               for (pin3=pin2; pin3<pin3e; pin3+=tvix3) {
                  /* Locate start of input row (this is to
                  *  handle kernels that are not rectangular) */
                  pin4  = pin3 + (*pkv++ << ils);
                  pin4e = pin4 + (*pkv++ << ils);
                  /* Loop over kernel columns */
                  for ( ; pin4<pin4e; pin4+=iadv) {
                     int jk = jkr >> 1;
                     ui32 uval = ils ?
                        (ui32)(*(ui16 *)pin4) : (ui32)(*pin4);
                     sum = jasw(sum, jmsw(*pkv, (si32)uval));
                     /* Reuse kernel value for each color */
                     pkv += jk;
                     jkr = (jkr + jk + jki) & 3;
                     } /* End one row of kernel */
                  } /* End loop over kernel rows */

               /* Round and shift one output point.  All values now
               *  stored as FL2S14.  Overflow test is done on 64-bit
               *  sum JIC kernel is perverse. */
               sum = jsrrsw(sum, oshft);  /* Reduce to S14 */
               asum = abs64(sum);
               if (o2off) {         /* PP_2SYMM symmetry expansion */
                  newval = (qsw(jrsl(asum, (S14-1))) > 0) ?
                     (S14-1) : swlo(asum);
                  if (qsw(sum) < 0) {
                     poh[0] = 0;
                     poh[o2off] = (si16)newval; }
                  else {
                     poh[0] = (si16)newval;
                     poh[o2off] = 0; }
                  }  /* End PP_2SYMM expansion */
               else {               /* Default single output */
#ifdef STGHINFO
                  int ival = swlo(asum);
                  if (ival > bigabs)
                     bigabs = ival, bigoff = poh - bigpoh0;
                  if (ival > (S14-1)) ++nover;
#endif
                  newval = (qsw(jrsl(asum, (S14-1))) > 0) ?
                     (qsw(sum) > 0 ? (S14-1) : -(S14-1)) : swlo(sum);
                  poh[0] = (si16)newval;
                  } /* End normal processed image storage */
               ++poh;
               } /* End one image row */
            } /* End one output block (= one kernel) */
         pkv1 = pkv;       /* Now use next kernel */
#ifdef STGHINFO
   convrt("(P1,' +++STGHINFO for kernel ',J0I4,', bigabs = ',"
      "J0B14I10.4,', at offset ',J0I8,', nover = ',J0I8)",
      &ik, &bigabs, &bigoff, &nover, NULL);
   bigoff = poh - bigpoh0;
   convrt("(P1,'    Ending offset = ',J0I8)", &bigoff, NULL);
#endif
#ifdef DBGIMGS    /*** DEBUG ***/
   simhi = -SI32_MAX, simlo = SI32_MAX;
   sim16 = pohdbg;
   sim16e = sim16 + pup->lppi3;
   while (sim16<sim16e) {
      if ((si32)(*sim16) > simhi) simhi = (si32)(*sim16);
      if ((si32)(*sim16) < simlo) simlo = (si32)(*sim16);
      ++sim16;
      }
   convrt("(P1,'0==>kernel ',J0I4,', imglo = ',J0I8,', imghi = ',J0I8)",
      &ik, &simlo, &simhi, NULL);
   newplt(5.0, 5.0, 0, 0, "DEFAULT", "BLACK", "DEFAULT", 0);
   pencol("RED");
   rect(0.4, 0.4, 4.2, 4.2, 0);
   bitmap((byte *)pohdbg, (int)pup->nppx, (int)pup->nppy,
      0.5, 0.5, 0, 0, (int)pup->nppx, (int)pup->nppy,
      (int)pup->ipkcol, 0);
   {  char *tmsg = ssprintf(NULL, "After preproc %s, kernel %d",
      getrktxt(pip->hipnm), ik);
      symbol(0.1, 0.1, 0.2, tmsg, 0, strlen(tmsg));
      }
   finplt();
#endif
         } /* End kernel loop */
      } /* End else not UPROG */

   /* Determine highest pixel in processor output for conntype scale
   *  normalization (PP_GETN) or pix from angle with max (PP_GETP).
   *  PP_GETN is also set if density histogram is requested.  */
   if (pip->ipsflgs & (PP_GETN|PP_GETP)) {
      si16 *poh, *pohe, *poh0 = (si16 *)(RP->pbcst + pip->oppoi);
      int dhist[NPDHISTB];             /* Small enough for stack */
      int dohisto = (int)(pup->ipuflgs & PP_HIST);
      int doroot = (int)(pup->ipuflgs & PP_ROOT);
      if (dohisto) memset(dhist, 0, sizeof(dhist));
      if (pip->ipsflgs & PP_STGH) {
         struct CELLTYPE *jl;          /* Locate unique nel info */
         size_t limg = pup->lppi3; /* Number of pp output pixels */
         size_t joppmx = pip->oppmx;   /* Global max pix offset */
         size_t josfmx = pip->osfmx;   /* Pixel max by angle offset */
         pohe = poh0 + limg;
         /* Loop over unique nel in system.  In principle, different
         *  preprocessors might be connected to cells with different
         *  subsets of the total set of unique nel in the system,
         *  but the bookkeeping to keep track of this would be a
         *  nightmare, so here we just find the max pixels for all
         *  unique nel--most likely just one in most models.  */
         for (jl=RP->pfct; jl && jl->sfnel; jl=jl->pnct) {
#ifdef PAR0                            /* Ptr to trig tables */
            si32 *psc = RP0->psfcst + jl->sfoff;
#else
            si32 *psc = Lija.psfcst + jl->sfoff;
#endif
            si32 gimmx = 0;            /* Global image stgh max */
            si32 oimmx = 0;            /* Offset of gimmx pixel */
            div_t pixxy;               /* Pixel location */
            /* Loop over pixels, then nel angles.  PP_GETN and PP_GETP
            *  are so similar that it is less work to find info for
            *  both even if only one of them is needed, recalling that
            *  PP_GETP storage is not present if not used.  Note that
            *  STGH (uim) output is always positive (sum of squares) */
            for (poh=poh0; poh<pohe; ++poh) {
               si32 pimmx = 0;         /* Pixel image stgh max */
               si32 aimmx = 0;         /* Angle of pimmx pixel */
               int  toff;              /* Angle table offset */
               for (toff=0; toff<jl->sfnel; ++toff) {
                  si64 gh2sum;         /* Sum g^2 + h^2 */
                  si32 uim;            /* STGH output  */
                  si32 ocos = psc[ 2*toff ];
                  si32 osin = psc[2*toff+1];
                  si32 vg = ocos*(si32)poh[0] +
                     osin*(si32)poh[limg];
                  si32 vh = ocos*(si32)poh[2*limg] +
                     osin*(si32)poh[3*limg];
                  gh2sum = jasw(jmsw(vg,vg),jmsw(vh,vh));
                  uim = doroot ?
#ifdef HAS_I64
                     swlo(jsrrsw((si64)sqrt(swdbl(gh2sum)), 16)) :
#else
                     swlo(jsrrsw(dbl2swd(sqrt(swdbl(gh2sum))), 16)) :
#endif
                     swlo(jsrrsw(gh2sum, 46));
                  if (uim > pimmx) pimmx = uim, aimmx = toff;
                  if (dohisto) {
                     int hbin = uim >> 10;   /* Shift S14 to IB4 */
                     if (hbin >= NPDHISTB) hbin = (NPDHISTB-1);
                     ++dhist[hbin]; }
                  } /* End orientation loop at one pixel */
               /* pimmx cannot overflow SjVPB = 24 bits because of
               *  46 bit right shift above */
               if (pip->ipsflgs & PP_GETP) RP->pjdat[josfmx++] =
                  pimmx | aimmx << SjVPB;
               if (pimmx > gimmx) gimmx = pimmx, oimmx = poh - poh0;
               } /* End loop over pixels in preprocessor output */
            RP->pjdat[joppmx++] = gimmx;
            pixxy = div(oimmx, (int)pup->nppx);
            convrt("(P4,' Preproc ',J1A" qLTF ",'max STGH pixel for "
               "nel = ',J1I8,'at ('J0I8,1H,J0I8,') is ',J0B14IJ8.4)",
               getrktxt(pip->hipnm), &jl->sfnel, &pixxy.rem,
               &pixxy.quot, &gimmx, NULL);
            } /* End loop over unique nel */
         } /* End finding max for PP_STGH filters */
      else {            /* Not STGH, no PP_GETP here */
         si32 immx = 0, immxo = 0;
         div_t pixxy, pixyk;
         pohe = poh0 + (size_t)pup->nker*(size_t)pup->lppi3;
         for (poh=poh0 ; poh<pohe; ++poh) {
            si32 uim = abs32((si32)(*poh));
            if (uim > immx) immx = uim, immxo = poh - poh0;
            if (dohisto) {
               int hbin = uim >> 10;   /* Shift S14 to IB4 */
               if (hbin >= NPDHISTB) hbin = (NPDHISTB-1);
               ++dhist[hbin]; }
            } /* End halfword pixel loop */
         RP->pjdat[pip->oppmx] = immx;
         pixxy = div(immxo, (int)pup->nppx);
         pixyk = div(pixxy.quot, (int)pup->nppy);
         convrt("(P4,' Preproc ',J1A" qLTF ",'max pixel at ('J0I8,1H,"
            "J0I8,') in kernel ',J0I8,' is ',J0B14IJ8.4)",
            getrktxt(pip->hipnm), &pixxy.rem, &pixyk.rem, &pixyk.quot,
            &immx, NULL);
         } /* End not PP_STGH preprocessor */
      if (dohisto) {
         static char pdbins[] =
         "   < 0.06    0.13    0.19    0.25    0.31"
          "    0.38    0.44    0.50    0.56    0.63    0.69"
          "    0.75    0.81    0.88    0.94     INF";
         convrt("(P4,'0Pixel density histogram:'/"
            "A129/1H " qqv(NPDHISTB) "I8)", pdbins, dhist, NULL);
         }
      } /* End getting max pixel */

/* End three levels of indenting suppressed */

            } /* End preprocessor loop */
         } /* End if filtering */

/*---------------------------------------------------------------------*
*       Implement four methods of getting image to network size        *
*  Note:  If user tries to rescale an image to a larger size, code     *
*  currently only allows the image to be centered into a background    *
*  rectangle.  There are different ways to do scaling in different     *
*  situations and it is best left to an external program (or tvgrabfn) *
*                                                                      *
*  Design Note for full 2-D interpolation:  At first, I stored inter-  *
*  mediate (horiz. interp.) O/P over input and used separate "lsum" &  *
*  "rsum" accumulators to avoid storing over a pixel before it was     *
*  used as input.  But this destroyed the array for the tvixy plot,    *
*  so switched to using psws space.  Now justified lsum,rsum as avoid- *
*  ing need to zero entire output array before starting.  But noted    *
*  that by doing just that, lsum & rsum could be eliminated and thus   *
*  an "if" in the main loop, which should be faster.  But we have to   *
*  avoid storing last pixel, multiplied by its zero fraction, off the  *
*  end of the intermediate (stage II) or final (stage IV) array.  Now  *
*  loops are simple enough to have separate byte and halfword loops.   *
*---------------------------------------------------------------------*/

/* Define scale for fractions of coordinates so cannot overflow
*  32 bits when multiplied by 16-bit pixels.  */
#define FBff   16
      if (cam->tviflgs & TV_IFit) {    /* TV_IFit ==> do something */
         PIXEL *por0,*por;             /* Ptrs to output pixel */
         size_t pixsz = (size_t)putv->tvopxsz;
         size_t lirow = pixsz * (size_t)putv->tvix;
         size_t lorow = pixsz * (size_t)putv->tvx;

         if (putv->tvx > putv->tvix || putv->tvy > putv->tviy) {
            /* tvxy image is larger than tvixy image (in at least
            *  one dimension).  No scaling is attempted (see above) */
            size_t lcpy = lirow;
            int xoff = SRA((int)putv->tvx - (int)putv->tvix, 1);
            int yoff = SRA((int)putv->tvy - (int)putv->tviy, 1);
            memset(pfit, 0, putv->ltvxy);    /* Default border */
            pr = praw, por = pfit;
            if (yoff >= 0)
               por += yoff*lirow, pre = pr + tvcsz;
            else
               pr -= yoff*lirow, pre = pr + lirow*putv->tvy;
            if (xoff >= 0)
               por += xoff;
            else
               pr += xoff, lcpy = lorow;
            /* Copy the image */
            while (pr < pre) {
               memcpy(por, pr, lcpy);
               por += lorow;
               pr += lirow;
               }
            /* Copy border of image out to edge of ltvxy box.
            *  Not clear how to get "adjacent edge" at a corner...  */
            if (putv->tvuflgs & TV_ADJB) {
               d3exit("Camera OPT=A", NYIMPL_ERR, 0);
               }
            }  /* End expanding image */

         else if (putv->tvuflgs & TV_IFIT) {
            /* Full 2-D interpolation.  Algorithm:  First interpolate
            *  all rows down from tvix to tvx cols storing in psws
            *  space, then interpolate all cols down from tviy to tvy
            *  rows, storing output finally in pfit array.  */
            struct Fif *pfif0,*pfif;
            PIXEL *prc,*prce;    /* Ptrs to first pxl in row/col */
            size_t lpor0;        /* Length of intermediate array */
            ui64 ipxyi;          /* Increment in scaled coords */
            ui64 ipxy,ipxye;     /* Input pixel center, loop end */
            ui32 tvx32 = (ui32)putv->tvx, tvix32 = (ui32)putv->tvix;
            ui32 tvy32 = (ui32)putv->tvy, tviy32 = (ui32)putv->tviy;
            ui32 szrat;
            if (tvx32*tviy32 != tvix32*tvy32)
               cryout(RK_P1, " -->WARNING: Camera ", RK_LN2,
               getrktxt(cam->hcamnm), RK_CCL, " IMAGE RESIZE DOES "
               "NOT CONSERVE ASPECT RATIO.", RK_CCL, NULL);
            /* Stage I:  Make interpolation table for columns--
            *  Using a few extra mult-divs for better accuracy... */
            pfif = pfif0 = (struct Fif *)RP0->ptvrsi;
            ipxy = jeul(tvix32 >> 1);  /* Rounding constant */
            ipxyi = jeul(tvx32 << FBff);
            ipxye = jmuwj(ipxyi, tvix32);
            szrat = jduwq(jauw(ipxyi,ipxy), tvix32);
            for ( ; qcuw(ipxy,ipxye); ++pfif,ipxy=jauw(ipxy,ipxyi)) {
               ui32 sx = jduwq(ipxy, tvix32);
               ui32 sxf = (1<<FBff) - (sx & (1<<FBff)-1);
               pfif->ipix = sx >> FBff;
               pfif->frac = min(szrat, sxf);
               }
            /* Stage II:  Loop over rows, interpolate columns */
            prce = praw + putv->ltvixy;
            por = por0 = (PIXEL *)CDAT.psws;
            lpor0 = tviy32*lorow;
            memset(por0, 0, lpor0);
            for (prc=praw; prc<prce; por+=lorow,prc+=lirow) {
               ui32 opo;         /* Offset to output pixel */
               int  icol = 0;
               pre = prc + lirow - pixsz;
               pfif = pfif0;
               /* Startup at end of row: full input pixel to output */
               if (ils) {
                  ui16 *poh = (ui16 *)por;
                  for (pr=prc; pr<pre; pr+=iadv) {
                     ui32 pix = (ui32)(*(ui16 *)pr);
                     ui32 frc = (ui32)pfif->frac;
                     opo = ncol*(ui32)pfif->ipix + icol;
                     poh[opo] += (pix * frc) >> FBff;
                     if (szrat > frc) poh[opo+ncol] +=
                        (pix * (szrat - frc)) >> FBff;
                     if (++icol >= ncol) icol = 0, ++pfif;
                     } /* End column loop */
                  } /* End halfword pixels */
               else {         /* Byte pixels */
                  for (pr=prc; pr<pre; pr+=iadv) {
                     ui32 opo = ncol*(ui32)pfif->ipix + icol;
                     ui32 pix = (ui32)(*pr);
                     ui32 frc = (ui32)pfif->frac;
                     por[opo] += (pix * frc) >> FBff;
                     if (szrat > frc) por[opo+ncol] +=
                        (pix * (szrat - frc)) >> FBff;
                     if (++icol >= ncol) icol = 0, ++pfif;
                     } /* End column loop */
                  } /* End byte pixels */
               } /* End row loop */
#ifdef DBGIMGS    /*** DEBUG ***/
         newplt(5.0, 5.0, 0, 0, "DEFAULT", "BLACK", "DEFAULT", 0);
         pencol("ORANGE");
         rect(0.4, 0.4, 4.2, 4.2, 0);
         bitmap(por0, (int)putv->tvx, (int)putv->tviy,
            0.5, 0.5, 0, 0, (int)putv->tvx, (int)putv->tviy,
            (int)putv->tvkocol, 0);
         symbol(0.1, 0.1, 0.2, "After fitting stage II", 0, 22);
         finplt();
#endif
            /* Stage III:  Make interpolation table for rows.
            *  (Different only if aspect being changed, but often
            *  it will change a little in the rounding anyway.)  */
            pfif = pfif0;
            ipxy = jeul(tviy32 >> 1);
            ipxyi = jeul(tvy32 << FBff);
            ipxye = jmuwj(ipxyi, tviy32);
            szrat = jduwq(jauw(ipxyi,ipxy), tviy32);
            for ( ; qcuw(ipxy,ipxye); ++pfif,ipxy=jauw(ipxy,ipxyi)) {
               ui32 sx = jduwq(ipxy, tviy32);
               ui32 sxf = (1<<FBff) - (sx & (1<<FBff)-1);
                  /* Cleanup at end of row */
               pfif->ipix = sx >> FBff;
               pfif->frac = min(szrat, sxf);
               }
            /* Stage IV:  Loop over columns, interpolate rows */
            prce = por0 + lorow;
            pre = por0 + lpor0;
            memset(por=pfit, 0, putv->ltvxy);
            for (prc=por0; prc<prce; por+=iadv,prc+=iadv) {
               pfif = pfif0;
               if (ils) {
                  ui16 *poh;
                  size_t hlorow = lorow >> ils;
                  for (pr=prc; pr<pre; ++pfif,pr+=lorow) {
                     ui32 opo = lorow*pfif->ipix;
                     ui32 pix = (ui32)(*(ui16 *)pr);
                     ui32 frc = (ui32)pfif->frac;
                     poh = (ui16 *)(por+opo);
                     *poh += (pix * frc) >> FBff;
                     if (szrat > frc) poh[hlorow] +=
                        (pix * (szrat - frc)) >> FBff;
                     } /* End column loop */
                  } /* End halfword pixels */
               else {         /* Byte pixels */
                  for (pr=prc; pr<pre; ++pfif,pr+=lorow) {
                     ui32 opo = lorow*pfif->ipix;
                     ui32 pix = (ui32)(*pr);
                     ui32 frc = (ui32)pfif->frac;
                     por[opo] += (pix * frc) >> FBff;
                     if (szrat > frc) por[opo+lorow] +=
                        (pix * (szrat - frc)) >> FBff;
                     } /* End column loop */
                  }  /* End byte pixels */
               } /* End column loop */
            }  /* End TV_IFIT image size scaling */

         else {
            /* Crop */
            size_t xoff,yoff;
            if (putv->tvuflgs & TV_ICTR) {
               /* Crop with centering */
               xoff = (size_t)((putv->tvix - putv->tvx) >> 1);
               yoff = (size_t)((putv->tviy - putv->tvy) >> 1); }
            else xoff = (size_t)putv->tvxo, yoff = (size_t)putv->tvyo;
            pr  = praw + pixsz * (yoff*(size_t)putv->tvix + xoff);
            pre = pr + lirow*(size_t)putv->tvy;
            por = pfit;
            while (pr < pre) {
               memcpy(por, pr, lorow);
               por += lorow;
               pr += lirow;
               }
            }  /* End cropping */

#ifdef DBGIMGS    /*** DEBUG ***/
         newplt(5.0, 5.0, 0, 0, "DEFAULT", "BLACK", "DEFAULT", 0);
         pencol("RED");
         rect(0.4, 0.4, 4.2, 4.2, 0);
         bitmap(pfit, (int)putv->tvx, (int)putv->tvy,
            0.5, 0.5, 0, 0, (int)putv->tvx, (int)putv->tvy,
            (int)putv->tvkocol, 0);
         symbol(0.1, 0.1, 0.2, "After fitting or cropping", 0, 25);
         finplt();
#endif
         }  /* End fitting or cropping to tvx * tvy size */

/*---------------------------------------------------------------------*
*                 Compute gray level from color image                  *
*                 (for network input at tvx*tvy size)                  *
*                                                                      *
*  This code derives a gray level from each pixel of a color image     *
*  using the formula:  Gray = 0.21*Red + 0.72*Green + 0.07*Blue.       *
*  GfmC output is not calculated unless used, so it is stored at       *
*  pbcst+cam->otvg in shared storage on a parallel computer.           *
*                                                                      *
*  Bytes per pixel is preserved:  1 or 2.                              *
*                                                                      *
*  R78, 04/21/18, GNR - Use final-size output from fitting/cropping.   *
*---------------------------------------------------------------------*/

      if (cam->tviflgs & TV_GfmC) {
         pre = pfit + putv->ltvxy;
         if (ils) {
            /* Pixels are 16 bits per color */
            ui16 *pr16, *pre16 = (ui16 *)pre;
            ui16 *po16 = (ui16 *)(RP->pbcst + cam->otvg);
            for (pr16=(ui16 *)pfit; pr16<pre16; pr16+=NColorDims) {
               ui32 gray = Red2G*pr16[0] + Grn2G*pr16[1] +
                  Blu2G*pr16[2];
               *po16++ = (ui16)((gray + RGB2GRnd) >> RGB2GScl);
               }
            }
         else {
            /* Pixels are 8 bits per color */
            byte *po8 = (byte *)(RP->pbcst + cam->otvg);
            for (pr=pfit; pr<pre; pr+=NColorDims) {
               ui32 gray = Red2G*pr[0] + Grn2G*pr[1] + Blu2G*pr[2];
               *po8++ = (byte)((gray + RGB2GRnd) >> RGB2GScl);
               }  /* End byte pixel loop */
            }
         }  /* End gray-from-color */

/*---------------------------------------------------------------------*
*     Determine highest pixel in image for conntype normalization      *
*                                                                      *
*  N.B.  In this initial version, we are not worrying about the type   *
*  of color reference by any attached conntypes, just look at all the  *
*  pixels.  To make this match conntype color, in principle code is    *
*  needed for the four possible cases:  gray, GFmC, color, opponents.  *
*                                                                      *
*  Whether TV_IFit was done or not, image seen by network is at pfit.  *
*---------------------------------------------------------------------*/

      if (cam->utv.tvsflgs & TV_GETN) {
         ui32 immx = 0;
         if (ils) {
            /* Pixels are 16 bits per color */
            ui16 *pr16 = (ui16 *)pfit;
            ui16 *pre16 = (ui16 *)(pfit + cam->utv.ltvxy);
            for ( ; pr16<pre16; ++pr16) {
               ui32 uim = (ui32)(*pr16);
               if (uim > immx) immx = uim;
               } /* End halfword pixel loop */
            RP->pjdat[cam->otvmx] = (si32)(immx >> 2);   /* To S14 */
            }
         else {
            /* Pixels are 8 bits per color */
            pr = pfit;
            pre = pr + cam->utv.ltvxy;
            for ( ; pr<pre; ++pr) {
               ui32 uim = (ui32)(*pr);
               if (uim > immx) immx = uim;
               }  /* End byte pixel loop */
            RP->pjdat[cam->otvmx] = (si32)(immx << Sv2mV); /* To S14 */
            }
         convrt("(P4,' Camera ',J1A" qLTF ",'max pixel is ',J0UJ8)",
            getrktxt(cam->hcamnm), &immx, NULL);
         } /* End getting max pixel */

      } /* End camera loop */

/*---------------------------------------------------------------------*
*                  Camera-independent final actions:                   *
*  Initiate next VVTV grab.                                            *
*---------------------------------------------------------------------*/

#ifdef VVTV
/* For VVTV, can immediately tell hardware to grab next
*  image while simulation trial is being done in CNS.  */

   if (RP->ortvfl & TV_VVTV) {
      int rc = vvgrab((int)RP->pftv->utv.tvta, OFF);
      if (rc) d3exit("VVGRAB", VIDEO_ERR, rc);
      }
#endif

   return grc;
   } /* End d3imgt() */
