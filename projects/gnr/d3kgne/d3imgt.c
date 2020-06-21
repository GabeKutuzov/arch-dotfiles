/* (c) Copyright 2010, Neurosciences Research Foundation, Inc. */
/* $Id: d3imgt.c 44 2011-03-04 22:36:14Z  $ */
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
*  Error action:  All errors result in calls to d3exit, as in the      *
*     original main program code.                                      *
*                                                                      *
*  Design note:  It is very likely that in any one run, only one of    *
*  the available camera interfaces will be used.  Therefore, it was    *
*  deemed not worthwhile to thread the cameras separately by type.     *
*                                                                      *
*  N.B.  As currently implemented, this code is called only on Node 0  *
*  of a parallel computer.  Obsolete VVTV drivers ran in parallel.  If *
*  a new VVTV is implemented and needs access on all nodes, this code  *
*  and the call in main must be reviewed for node specificity.         *
************************************************************************
*  V8F, 04/25/10, GNR - New routine, some code moved here from main    *
*  ==>, 05/14/10, GNR - Last mod before committing to svn repository   *
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
#include "tvdef.h"
#ifdef  BBDS
#include "bbd.h"
#endif
#include "rocks.h"
#include "rkxtra.h"
#include "rkarith.h"

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
   ui16 uflgs = (TV_UPGM & flgmsk) << fshft3;
#ifdef VVTV
   ui16 vflgs = (TV_VVTV & flgmsk) << fshft3;
#endif
   ui16 ipflgs = ((TV_VVTV|TV_BBD|TV_UPGM) & flgmsk) << fshft3;

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
         if (rc) d3exit(VIDEO_ERR, "VVGCHK", rc);
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
         if (rc) d3exit(VIDEO_ERR, "VVGCHK", rc);
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
         if (rc) d3exit(VIDEO_ERR, "VVGCHK", rc);
         }
#endif
      break;
      } /* End frequency switch */
#endif

/* Loop over active cameras.  For non-BBD cameras, must acquire
*  image and preprocess.  For BBD cameras, just preprocess.  */

   for (cam=RP->pftv; cam; cam=cam->pnxtv) {

/* Process UPROG cameras */

      if (cam->utv.tvsflgs & uflgs) {
         int rc = cam->tvgrabfn(&cam->utv, cam->pmyin);
         if (rc) d3exit(VIDEO_ERR, getrktxt(cam->hcamnm), rc);
         if (cam->ptvmdlt) d3mark1(cam->ptvmdlt,
            cam->utv.isn, cam->utv.ign, cam->utv.iadapt);
         if (RP->kdprt & PROBJ) convrt("(P4,' CAMERA ',J1A" qLTF
            ",'LOADED OBJECT ',J1UH6,'FROM CLASS ',J0UH6)",
            getrktxt(cam->hcamnm), &cam->utv.isn, &cam->utv.ign,
            NULL);
         }

#ifdef VVTV
/* Process VVTV cameras */

      if (cam->utv.tvsflgs & vflgs) {
         int rc = vvget2(cam->pmyin, cam->utv.icam, cam->utv.tvkcol,
            cam->utv.tvxo, cam->utv.tvyo, cam->utv.tvx, cam->utv.tvy);
         if (rc) d3exit(VIDEO_ERR, "VVGET2", rc);
         } /* End if EVTVTV */
#endif

/*---------------------------------------------------------------------*
*                   Perform rescaling (KP=R option)                    *
*----------------------------------------------------------------------*
*  This code rescales an input image in the same manner as in the      *
*  earlier preprocessor "user program" prescale.c.  This code was      *
*  inserted here so it would be unnecessary at this date to add code   *
*  to CNS to allow one preprocessor to work on the output of another.  *
*  The scaling method is to derive a histogram of image intensities,   *
*  determine the gray level corresponding to a specified percentile,   *
*  and rescale linearly so that graylevel becomes a given percentage   *
*  of 256.  Random rounding is applied to the scaled values to elimi-  *
*  nate any visible step contours in the output.                       *
*                                                                      *
*  This program will not work correctly with UTVCol_C8 or UTVCol_C16   *
*  color images as it stands, and those cases are trapped in g1cam.    *
*---------------------------------------------------------------------*/

      if (cam->utv.tvuflgs & TV_RESCL) {

/* The following are constants, not parameters:  */
#define GRBITS     8       /* Number of bits in a pixel */
#define NPCT100  100       /* Number of steps in 100 percent */
#define HISTSZ  (MAXGRAY*I32SIZE)

/* Two levels of indenting suppressed to end of rescale processing */

   PIXEL *pi,*piie;        /* Ptrs to image and image end */
   si32  *pih;             /* Ptr to histogram & scale table */
   si32  iscl;             /* Unit scale */
   si32  ntot;             /* Total bytes in image */
   si32  ntgt;             /* Target gray level */
   ui32  pixsz;            /* Pixel size */
   int   itgt;             /* Intensity at target gray level (S8) */
   int   ii;               /* Intensity into table */

/* Scan through the image and accumulate a histogram of intensities */

   pixsz = (ui32)max(cam->utv.tvkcol, 1);
   ntot = pixsz * (ui32)cam->utv.tvx * (ui32)cam->utv.tvy;
   pih = RP0->ptvrsh;
   memset((char *)pih, 0, HISTSZ);
   pi = cam->pmyin;
   piie = pi + ntot;
   while (pi < piie) {     /* Scan image */
      pih[*pi++] += 1;
      }

/* Determine the value of the rspct percentile */

   ntgt = ntot * (NPCT100 - cam->rspct) / NPCT100;
   itgt = MAXGRAY;
   while (itgt > 0) {
      si32 newtgt = ntgt - pih[--itgt];
      if (newtgt <= 0) goto FoundTgtLevel;
      ntgt = newtgt;
      }
   d3exit(PREPROC_ERR, "d3imgt", 3);

/* Construct scaling table.  Table is constructed by repeated
*  addition for speed--Error of 1 in low bit after 256 adds is
*  still only one intensity unit in result */

FoundTgtLevel:
   /* Expand itgt to S8 integer + fraction */
   iscl = ((si32)itgt << GRBITS) + (ntgt << GRBITS)/pih[itgt];
   iscl = (cam->rslvl << (2*GRBITS))/iscl;
   pih[0] = 0;
   for (ii=1; ii<MAXGRAY; ++ii)
      pih[ii] = pih[ii-1] + iscl;

/* Make second pass through image.  Lookup scaled value
*  corresponding to each pixel value.  Apply random
*  rounding to eliminate intensity steps in output.  */

/*** KLUDGE ALERT:  FOR NOW, WE JUST PUT THE RESULT IN PLACE
***  OF THE ORIGINAL IMAGE.  THIS PRECLUDES PLOTTING THE
***  ORIGINAL IMAGE.  THIS MUST BE FIXED AFTER INITIAL TESTING ***/

   pi = cam->pmyin;
   while (pi < piie) {     /* Scan image */
      ui32 iv =
         (pih[*pi] /* + (udev(pih+MAXGRAY) >> 23) */) >> GRBITS;
      if (iv > (MAXGRAY-1)) iv = (MAXGRAY-1);
      *pi++ = (PIXEL)iv;
      }

   } /* End prescaling */

/* End two levels of indenting suppressed */

/*---------------------------------------------------------------------*
*          Do any preprocessing associated with this camera            *
*---------------------------------------------------------------------*/

      if (cam->utv.tvsflgs & ipflgs)
         for (pip=cam->pmyip; pip; pip=pip->pnipc)
            if (pip->ipsflgs & PP_ON) {

/* Two levels of indenting suppressed to end of kernel processing */

   if (pip->ipsflgs & PP_UPGM) {
      int rc = pip->pipfn(&pip->upr, cam->pmyin, RP->pbcst+pip->otvi);
      if (rc) d3exit(PREPROC_ERR, getrktxt(pip->hipnm), rc);
      }
   else {                  /* Must be kernel type */
      si64 sum;            /* One evaluation */
      si32 *pkv1,*pkv;     /* Ptrs to kernel values */
      byte *pin0,*pin1;    /* Output-level ptrs to input */
      byte *pin2,*pin3;    /* Kernel-level ptrs to input */
      byte *pie0,*pie3;    /* Ends of input block/row */
      byte *pire;          /* Ptr to input row end */
      byte *pou;           /* Ptr to output */
      size_t o2off;        /* Option 2 offset */
      si32 newval;         /* New value of pixel */
      int ik;              /* Kernel loop index */
      int ikr;             /* Kernel row counter */
      int jkr,jki;         /* Kernel repeat controls */

      /* This must be consistent with checks in d3schk:
      *  When kernel size is even, small half goes on top/left */
      pkv1 = &pip->upr.pipk->kval;
      pin0 = cam->pmyin + pip->upr.oin0;
      pie0 = pin0 + pip->upr.tvx3*(ui32)pip->upr.nipy;
      pou = RP->pbcst + pip->otvi;
      jki = (int)pip->jkri;
      o2off = (pip->upr.ipuflgs & PP_2SYMM) ? (size_t)pip->upr.nker *
         (size_t)pip->upr.nipx3 * (size_t)pip->upr.nipy : 0;

      /* Loop over kernels.  This will recycle over the input but
      *  continue filling the output array.  Control of repeat use
      *  of kernel for multicolor input is "cute" code designed to
      *  avoid an "if" in the inner loop.  But I have not yet coded
      *  separation of colors when color mode is Col_C16 and that
      *  mode is forbidden at startup time--new code needed here
      *  for Col_C16.  Probably each color mode should have its own
      *  loop.  */
      for (ik=0; ik<pip->upr.nker; ++ik) {
         pin1 = pin0;
         /* Loop over image rows */
         while (pin1 < pie0) {
            pire = pin1 + pip->upr.nipx3;
            /* Loop over image columns */
            for ( ; pin1<pire; ++pin1) {
               sum = jesl(0);
               pkv = pkv1;
               jkr = (int)pip->jkr0;
               pin2 = pin1 - pip->lbkup;
               /* Loop over the kernel */
               for (ikr=0; ikr<pip->nky; ++ikr) {
                  /* Locate start of input row (this is to
                  *  handle kernels that are not rectangular) */
                  pin3 = pin2 + *pkv++;
                  pie3 = pin3 + *pkv++;
                  for ( ; pin3<pie3; ++pin3) {
                     int jk = jkr >> 1;
                     si32 tval = abs32(*pin3);
                     sum = amlssw(sum, *pkv, tval, 0);
                     /* Reuse kernel value for each color */
                     pkv += jk;
                     jkr = (jkr + jk + jki) & 3;
                     } /* End one row of kernel */
                  pin2 += pip->upr.tvx3;
                  } /* End loop over kernel values */
               /* Divide, round, and store one output point */
               if (o2off) {
                  newval = (jdswq(sum, *pkv++) + S23) >> 24;
                  newval = abs32(newval);
                  if (newval > 255) newval = 255;
                  if (qsw(sum) < 0) {
                     pou[0] = 0;
                     pou[o2off] = (byte)newval;
                     }
                  else {
                     pou[0] = (byte)newval;
                     pou[o2off] = 0;
                     }
                  } /* End PP_2SYMM symmetry expansion */
               else {
                  if (qsw(sum) < 0)
                     pou[0] = 0;
                  else {
                     newval = (jdswq(sum, *pkv++) + S23) >> 24;
                     if (newval > 255) newval = 255;
                     pou[0] = (byte)newval;
                     }
                  } /* End normal processed image storage */
               ++pou;
               } /* End one image row */
            /* Skip over unused borders of input */
            pin1 += pip->upr.lrowskip;
            } /* End one output block (= one kernel) */
         pkv1 = pkv;       /* Now use next kernel */
         } /* End kernel loop */
      } /* End else not UPROG */

/* End two levels of indenting suppressed */

         } /* End preprocessor loop */
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
      if (rc) d3exit(VIDEO_ERR, "VVGRAB", rc);
      }
#endif

   return grc;
   } /* End d3imgt() */
