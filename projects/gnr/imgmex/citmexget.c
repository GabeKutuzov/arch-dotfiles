/* (c) Copyright 2011-2012, The Rockefeller University *11113* */
/* $Id: citmexget.c 9 2012-11-21 22:00:13Z  $ */
/***********************************************************************
*                             citmexget                                *
************************************************************************
*  This is the image acquisition routine for a Matlab mex-function     *
*  package to read images from the "Caltech 101" dataset by Fei-Fei    *
*  Li, Marco Andreetto, Marc 'Aurelio Ranzato, and Pietro Perona.      *
*  See: L. Fei-Fei, R. Fergus, and P. Perona.  "Learning generative    *
*  visual models from few training examples:  An incremental Bayesian  *
*  approach tested on 101 object categories."  IEEE CVPR 2004 Workshop *
*  on Generative-Model Based Vision (2004).                            *
*                                                                      *
*  For convenience, documentation for the entire set of routines is    *
*  contained in the citmexset source file.  This file only contains    *
*  information for citmexget.                                          *
*----------------------------------------------------------------------*
*  Image acquisition call (make only after citmexset call succeeds):   *
*     [image, isid] = citmexget(CitMexComm)                            *
*  Call to obtain just the isid information for next scheduled image:  *
*     isid = citmexget(CitMexComm)                                     *
*                                                                      *
*  Action:  Both calls return the 'isid' information detailed below    *
*     for the next image in the sequence specified by the selectors    *
*     given in the last citmexset call.  If an 'image' argument is     *
*     also present, the actual image is also returned.                 *
*                                                                      *
*  Arguments:                                                          *
*     CitMexComm    Data array returned by initial citmexset call.     *
*                                                                      *
*  Return value:                                                       *
*     image    Matlab array of size mxiy x mxix (or smaller).  If the  *
*              image is colored and the 'kops' 1-bit in the citmexset  *
*              call is set, 'image' is returned as an mxiy x mxix x 3  *
*              array of red-green-blue values.  Otherwise, 'image' is  *
*              returned as a uint8 array of grayscale value.           *
*     isid     Matlab uint16 vector of 6 elements containing stimulus  *
*              identification info as follows.  (For compatibility     *
*              with the NORB routines, the category and image instance *
*              numbers count from 0 and the 3rd through 6th elements   *
*              are returned but are always 0.)                         *
*              isid(1) = Category number (0-100)                       *
*              isid(2) = Image instance number (0-799)                 *
*              isid(3) = Azimuth (0)                                   *
*              isid(4) = Elevation (0)                                 *
*              isid(5) = Lighting (0)                                  *
*              isid(6) = Camera (0)                                    *
*                                                                      *
*  Error conditions:  The script is terminated with an error message   *
*     on all error conditions.                                         *
************************************************************************
*  V1A, 11/22/11, GNR - New program, based on norbmexget.c             *
*  V2A, 12/06/11, GNR - Add cropping and masking options               *
*  ==>, 12/22/11, GNR - Last mod before committing to svn repository   *
*  Rev, 01/19/12, GNR - Correct bug when nbatch > images in category   *
*  Rev, 02/02/12, GNR - Add semaphore to prevent multiple free         *
*  V3A, 11/08/12, GNR - Add cropping border and deferred painting      *
*  V3B, 11/20/12, GNR - Use iterepgf to add border around image paint  *
***********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <semaphore.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "sysdef.h"
#include "rkarith.h"
#include "citmex.h"

/* Location of common data array (for error exits) */
static const mxArray *pCitMexComm;


/*=====================================================================*
*                              cmFreeAll                               *
*                                                                      *
*  This routine frees all storage allocated.  It assumes no files are  *
*  open and performs the same functions as cmFreeAll in citmexset.c,   *
*  which is registered as a mexAtExit routine by citmexset.  It does   *
*  not attempt mxDestroyArray because it has read-only access to       *
*  CitMexComm--the MATLAB automatic disposal mechanism should work     *
*  on CitMexComm once the child allocations have been freed by this    *
*  routine.                                                            *
*=====================================================================*/

static void cmFreeAll(void) {

   sem_t  *CMCCSem;        /* Ptr to mem control semaphore */

   /* If already called, or citmexset close was called,
   *  do nothing (to avoid seg fault in MATLAB) */
   if ((CMCCSem = sem_open(CMSemNm, 0)) == SEM_FAILED) {
      if (errno == ENOENT) return;
      abexitme(654, "Error reopening " CMSemNm);
      }
   if (sem_trywait(CMCCSem)) {
      if (errno != EAGAIN)
         abexitme(654, "Error testing " CMSemNm);
      }
   else if (pCitMexComm) {
      struct C101STATE *pst = mxGetData(pCitMexComm);
      if (pst && pst->sigck == C101_SIGCK) {
         sspec *pss,*pnxtss;
         for (pss=pst->pfss; pss; pss=pnxtss) {
            pnxtss = pss->pnss;
            if (pss->u1.pcis) mxFree(pss->u1.pcis);
            if (pss->pcat) mxFree(pss->pcat);
            if (pss->pimg) mxFree(pss->pimg);
            mxFree(pss);
            }
         if (pst->bname) mxFree(pst->bname);
         if (pst->pcin0) mxFree(pst->pcin0);
         if (pst->ppad)  mxFree(pst->ppad);
         if (pst->pitwk) mxFree(pst->pitwk);
         }
      pCitMexComm = NULL;
      }

   /* Close the semaphore */
   if (sem_close(CMCCSem))
      abexitme(654, "Error closing " CMSemNm);

   } /* End cmFreeAll() */


/*=====================================================================*
*                    cmMexErr, cmMexErr2, cmSysErr                     *
*                                                                      *
*  These are wrappers for mexErrMsgTxt().  They concatenate the error  *
*  id number and one or two text strings, call cmFreeAll(), then       *
*  mexErrMsgTxt().  The message is formatted before cmFreeAll() is     *
*  called, just in case some text from dynamic storage is included.    *
*  These are copies of the corresponding routines in citmexset.c,      *
*  which cannot be called directly because loaded dynamically at a     *
*  different time.                                                     *
*=====================================================================*/

static void cmMexErr(int rc, char *pmsg) {

   char *etxt = ssprintf(NULL, "citmex %d: %s.", rc, pmsg);
   cmFreeAll();
   mexErrMsgTxt(etxt);

   } /* End cmMexErr() */

static void cmMexErr2(int rc, char *pmsg1, char *pmsg2) {

   char *etxt = ssprintf(NULL, "citmex %d: %s %s.", rc, pmsg1, pmsg2);
   cmFreeAll();
   mexErrMsgTxt(etxt);

   } /* End cmMexErr2() */

static void cmSysErr(int rc, char *pmsg, int ierr) {

   char etxt[LSTRERR];
   char *etxtr = strerror_r(ierr, etxt, LSTRERR);
   cmMexErr2(rc, pmsg, etxtr);

   } /* End cmSysErr() */


/*=====================================================================*
*                              rd1image                                *
*                                                                      *
*  Read an image, taking into account sub-windowing.  Uses a callback  *
*  into Matlab to do the actual work to save interacting with libjpeg  *
*  directly.  Assumes suitable constants have been set up in C101STATE.*
*  Arguments:                                                          *
*     pst            Ptr to C101STATE.                                 *
*     pimi           Ptr to CISpec for the requested image.            *
*  Return value:  Pointer to image (uint8 array).                      *
*  All errors terminate the script.                                    *
*=====================================================================*/

#define NRDLA  3              /* Max number of citrdimg left args */
#define iimg   0                 /* Index of image array */
#define ibox   1                 /* Index of box_coord array */
#define iobjc  2                 /* Index of obj_contour array */

#define NRDRA  4              /* Max number of citrdimg right args */
#define iFNm   0                 /* Index of image path arg */
#define iMxXY  1                 /* Index of mxix,mxiy arg */
#define nMxXY  4                 /* Size of mxxy vector */
#define  jmxX   0                   /* Offset of X max */
#define  jcbX   1                   /* Offset of crop border x */
#define  jmxY   2                   /* Offset of Y max */
#define  jcbY   3                   /* Offset of crop border y */
#define iKopt  2                 /* Index of kopt arg */
#define nKopt  3                 /* Size of kopt vector */
#define  kcolr  0                   /* Offset of color switch */
#define  kcrop  1                   /* Offset of crop switch */
#define  kbgdp  2                   /* Background paint */
#define iaFNm  3                 /* Index of annotation path arg */


static mxArray *rd1image(struct C101STATE *pst, CISpec *pimi) {

   mxArray *prla[NRDLA];      /* Ptrs to citrdimg left arguments */
   mxArray *prra[NRDRA];      /* Ptrs to citrdimg right arguments */
#ifdef DEBUG  /*** DEBUG ***/
   mxArray *pdbg[1],*pdbgo[1];
#endif
   mxArray *pimage = NULL;    /* Ptr to image */
   char    *fnm = pst->fname; /* Ptr to image path construction area */
   catnm   *pcn;              /* Ptr to category name index entry */
   double  *pdmxxy;           /* Ptr to mxix,mxiy citrdimg args */
   mxLogical *pkopt;          /* Ptr to kopt citrdimg args */
   int     imgn,jfn;          /* Control for writing image number */
   int     nlargs;            /* Number of left args to citrdimg */
   static char imgFNm[] = ImgFNm;
   static char annFNm[] = AnnFNm;

/* Generate the required image file name from category and instance.
*  Note:  pst->bname is guaranteed to end in a '/'.  */

   pcn = pst->pcin0[pimi->icat].pncn;
   imgn = pimi->imgno;
   for (jfn=NIMGNUM-1; jfn>=0; --jfn) {
      div_t qr = div(imgn, 10);
      annFNm[jfn+FSTALOC] = imgFNm[jfn+FSTILOC] = qr.rem + '0';
      imgn = qr.quot;
      }
   strcpy(fnm, pst->bname);   /* Build image path */
   strcat(fnm, ImgDir);       /* Append image subdir name */
   strcat(fnm, pcn->cname);   /* Append category name */
   strcat(fnm, imgFNm);       /* Append image file name */

/* Generate arguments and call Matlab function "citrdimg" to actually
*  read (and possibly downsize and convert to grayscale) the image.  */

   if (!(prra[iFNm] = mxCreateString(fnm)))
      cmMexErr(634, "Unable to copy filename to Matlab string");

   prra[iMxXY] = mxCreateDoubleMatrix(1, nMxXY, mxREAL);
   if (!(pdmxxy = mxGetPr(prra[iMxXY])))
      cmMexErr2(636, "Unable to allocate", "1x2 Matlab array");
   pdmxxy[jmxX] = (double)pst->tvx;
   pdmxxy[jcbX] = (double)pst->tbx;
   pdmxxy[jmxY] = (double)pst->tvy;
   pdmxxy[jcbY] = (double)pst->tby;

   prra[iKopt] = mxCreateLogicalMatrix(nKopt, 1);
   if (!(pkopt = (mxLogical *)mxGetData(prra[iKopt])))
      cmMexErr2(636, "Unable to allocate","4x1 MxLogical vector");
   pkopt[kcolr] = (mxLogical)((pst->cmflgs & cmCOLORED) != 0);
   pkopt[kcrop] = (mxLogical)((pst->cmflgs & cmCROP) != 0);
   pkopt[kbgdp] = (mxLogical)((pst->cmflgs & (cmMASK|cmDEFMSK)) != 0);

/* Cropping and/or masking */

   if (pst->cmflgs & (cmCROP|cmMASK|cmDEFMSK)) {
      char *anm = pst->aname;    /* Ptr to annotation path */
      strcpy(anm, pst->bname);   /* Build annotation path */
      strcat(anm, AnnDir);       /* Append annotation subdir name */
      strcat(anm, pcn->cname);   /* Append category name */
      strcat(anm, annFNm);       /* Append annotation file name */
      if (!(prra[iaFNm] = mxCreateString(anm)))
         cmMexErr(634, "Unable to copy filename to Matlab string");
      nlargs = pst->cmflgs & (cmMASK|cmDEFMSK) ? NRDLA : 1;
      if (mexCallMATLAB(nlargs, prla, NRDRA, prra, "citrdimg"))
         cmMexErr(651, "Call to citrdimg failed");
      pimage = prla[iimg];
#ifdef DEBUG /*** DEBUG *** Fig 1 */
      pdbg[0] = pimage;
      mexCallMATLAB(0,pdbgo,1,pdbg,"dbgimg");
#endif
      if (pst->cmflgs & (cmMASK|cmDEFMSK)) {
         /* Store object coords for masking--citrdimg does cropping.
         *  N.B.  Matlab returns images as the transpose (y fast
         *  moving).  iterepgf should not care, so instead of
         *  transposing, we just feed it the reversed dimensions.
         *  In the following, 'x' and 'y' refer horiz,vert, bzw. */
         const mwSize *pnvtx = mxGetDimensions(prla[iobjc]);
         const mwSize *pimgd = mxGetDimensions(pimage);
         double *pbox  = mxGetPr(prla[ibox]);
         double *pobj  = mxGetPr(prla[iobjc]);
         xyf    *pgon;           /* Ptr to polygon list */
         double x0,y0;
         mwSize lepgwk,nlitwk;   /* Length of IEpgfWks, new litwk */
         mwSize ivtx,jvtx,nvtx = pnvtx[1];
         mwSize nsx = pimgd[1], nsy = pimgd[0];

         /* If doing immediate masking, temporary vertex coordinate
         *  list is stored over the coords returned in obj_coords,
         *  but if masking is deferred, space is made for this list
         *  following the IPgfgWk work areas.  Either way, space is
         *  also allocated here for iterepgf's work image.
         *  Use high-water-mark allocation.  */
         lepgwk = nvtx*sizeof(IEpgfWk);
         nlitwk = lepgwk + nsx*BytesInXBits(nsy);
         if (pst->cmflgs & cmDEFMSK) nlitwk += nvtx*sizeof(xyf);
         if (nlitwk > pst->litwk) {
            pst->litwk = nlitwk + LPWKINCR;  /* Allow some extra */
            pst->pitwk = (byte *)mxRealloc(pst->pitwk, pst->litwk);
            mexMakeMemoryPersistent(pst->pitwk);
            }
         if (pst->cmflgs & cmDEFMSK) {       /* Deferred painting */
            pgon = (xyf *)(pst->pitwk + lepgwk);
            pst->nwvtx = nvtx; }
         else                                /* Immediate painting */
            pgon = (xyf *)pobj;

         /* Because we have to increment the contour coords by the box
         *  coords if cropping was not done, it seems reasonable also
         *  to use this opportunity to reverse the x and y axes and
         *  to convert the doubles to floats rather than make a double-
         *  precision iterepgf.  */
         x0 = pbox[2], y0 = pbox[0];
#ifdef DEBUG
         mexPrintf("Cat %d, Image %d, x0 = %f, y0 = %f\n",
            pimi->icat, pimi->imgno, x0, y0);
#endif
         for (ivtx=jvtx=0; ivtx<nvtx; ++ivtx) {
            pgon[ivtx].y = (float)(pobj[jvtx++] + x0);
            pgon[ivtx].x = (float)(pobj[jvtx++] + y0);
#ifdef DEBUG
            mexPrintf("Vertex %d, x=%f, y=%f\n",
               ivtx,(double)pgon[ivtx].x,(double)pgon[ivtx].y);
#endif
            }

         /* Now actually perform the masking if not deferred */
         if (pst->cmflgs & cmMASK) {
            IterEpgf P;             /* Polygon iterator data */
            byte   *pim  = (byte *)mxGetData(pimage);
            mwSize ndims = mxGetNumberOfDimensions(pimage);
            long   l1img = nsx*nsy, l2img = l1img+l1img;
            /* For compat with old calls, default mode is 6 */
            int    mode  = (pst->cmflgs & cmNDMODE) ?
               (int)(pst->cmflgs >> 4) : (EPGF_EXT|EPGF_RMB);

#ifdef DEBUG /*** DEBUG ***/
            mexPrintf("nsx=%d,nsy=%d,limg=%ld\n",nsx,nsy,l1img);
#endif
            InitEpgfIndexing(&P, pgon, (IEpgfWk *)pst->pitwk,
               pst->pitwk+lepgwk, pst->pbord, nsy, nsx, nvtx, mode);
#ifdef DEBUG
            {  IEpgfWk *ped;
               double ds = P.um1 + 1;
               for (ped=P.wkel; ped; ped=ped->pne) {
                  double dlx = (double)ped->slx/ds;
                  double dhx = (double)ped->shx/ds;
                  double dly = (double)ped->sly/ds;
                  double dhy = (double)ped->shy/ds;
                  double drx = (double)ped->rix/ds;
                  double dsl = (double)ped->slope/ds;
                  mexPrintf("Edge (%f,%f)--(%f,%f), rix=%f, slope=%f\n",
                     dlx,dly,dhx,dhy,drx,dsl);
                  }
               }
#endif
#ifdef DEBUG /*** DEBUG *** Fig. 2 */
            pdbg[0] = pimage;
            mexCallMATLAB(0,pdbgo,1,pdbg,"dbgimg");
#endif
            if (ndims == NCOLDIMS) while (GetNextPointInEpgf(&P)) {
#ifdef DEBUG
               if (P.ioff >= l1img)
                  mexPrintf("Point at %ld, %ld has ioff = %ld\n",
                     P.ix,P.iy,P.ioff);
#endif
               pim[P.ioff] = pim[P.ioff+l1img] = pim[P.ioff+l2img] = 0;
               }
            else while (GetNextPointInEpgf(&P)) {
#ifdef DEBUG
               if (P.ioff >= l1img)
                  mexPrintf("Point at %ld, %ld has ioff = %ld\n",
                     P.ix,P.iy,P.ioff);
#endif
               pim[P.ioff] = 0;
               }
#ifdef DEBUG /*** DEBUG *** Fig. 3 */
            pdbg[0] = pimage;
            mexCallMATLAB(0,pdbgo,1,pdbg,"dbgimg");
#endif
            } /* End masking */

         mxDestroyArray(prla[ibox]);
         mxDestroyArray(prla[iobjc]);
         } /* End if cmMASK | cmDEFMSK */

      mxDestroyArray(prra[iaFNm]);
      }

/* No cropping or masking */

   else {
      if (mexCallMATLAB(1, &pimage, NRDRA-1, prra, "citrdimg"))
         cmMexErr(651, "Call to citrdimg failed");
      }

   mxDestroyArray(prra[iFNm]);
   mxDestroyArray(prra[iMxXY]);
   mxDestroyArray(prra[iKopt]);

   return pimage;

   } /* End rd1image() */


/*=====================================================================*
*                              citmexget                               *
*=====================================================================*/

void citmexget(int nlhs, mxArray *plhs[],
      int nrhs, const mxArray *prhs[]) {

   struct C101STATE *pst;
   mxArray *pimage;           /* Ptr to output image */
   CISpec  *pimi;             /* Ptr to image info */
   ui16    *pisid;            /* Ptr to image id info */
   sspec   *pss;              /* Ptr to selection spec */
   ui32    rr;                /* Random number */
   CISpec  simi;              /* Selected TTSpec image info */

/* Standard MATLAB argument checking */

   if (nrhs != 1 || !mxIsClass(prhs[0],"uint8"))
      cmMexErr(632, "1st and only arg must be CitMexComm");
   pCitMexComm = prhs[0];
   pst = (struct C101STATE *)mxGetData(prhs[0]);
   if (!pst || pst->sigck != C101_SIGCK)
      cmMexErr(630, "Bad CitMexComm arg.");

   if (nlhs == 0 || nlhs > 2)
      cmMexErr(652, "One or two return values required");

   plhs[nlhs-1] = mxCreateNumericMatrix((mwSize)NSVARS,
      (mwSize)1, mxUINT16_CLASS, mxREAL);
   pisid = (ui16 *)mxGetData(plhs[nlhs-1]);
   if (!pisid)
      cmMexErr(636, "No space for image id info");

/* Select next image.  If ivar is 0, it is time to start scanning at a
*  random sspec and, if a TTSL, pick starting category.  */

   if (pst->ivar == 0) {
      pss = pst->pfss;
      if (pst->nspec > 1) {
         /* It is very unlikely that there will be more than 2 or 3
         *  selection specs, so a simple linked-list crawl is used
         *  here instead of a binary search.  The random number seed
         *  of the first C101SPEC is used for simplicity.  The prob
         *  of the last C101SPEC is larger than the largest udev, so
         *  the loop should never drop through with pss == NULL.  */
         rr = (ui32)udev(&pss->seed);
         for ( ; pss; pss=pss->pnss) {
            if (rr <= pss->prob) break;
            }
         pst->pcss = pss;
         }
      /* Got a spec, now if TTSpec, pick a category */
      if (pss->ksel & KS_TTSL) {
         if (pss->ksel & (KS_CSEQ|KS_CPERM)) {
            pss->icseq += 1;
            if (pss->icseq >= pss->nu1) pss->icseq = 0;
            }
         else {
            rr = (ui32)udev(&pss->seed);
            pss->icseq = rr % pss->nu1;
            }
         }
      }
   else
      pss = pst->pcss;

/* Select category and image if CISL, image if TTSL */

   if (pss->ksel & KS_TTSL) {    /* TTSL */
      TTSpec *ptt = pss->u1.ptts + pss->icseq;
      simi.icat = ptt->icat;
      if (pss->ksel & (KS_ISEQ|KS_IPERM)) {
         ui32 jvar = pst->ivar % ptt->limg;
         simi.imgno = pss->pimg[ptt->oimg + jvar];
         }
      else {
         rr = (ui32)udev(&pss->seed) % ptt->limg;
         simi.imgno = pss->pimg[ptt->oimg + rr];
         }
      pimi = &simi;
      }

   else {                        /* CISL */
      if (pss->ksel & (KS_CSEQ|KS_CPERM))
         pimi = pss->u1.pcis + pst->ivar;
      else {
         rr = (ui32)udev(&pss->seed) % pss->nu1;
         pimi = pss->u1.pcis + rr;
         }
      }

   /* Advance ivar for next time */
   pst->ivar += 1;
   if (pst->ivar >= pss->nbatch) pst->ivar = 0;

   /* Return category and image instance info */
   pisid[KV_Cat]  = pimi->icat - 1;
   pisid[KV_Inst] = pimi->imgno - 1;
   pisid[KV_Azim] = pisid[KV_Elev] =
      pisid[KV_Light] = pisid[KV_View] = 0;

   /* Read the image if a pointer was provided */
   if (nlhs == 2) {
      mxArray *prdimg = rd1image(pst, pimi);
#ifdef DEBUG  /*** DEBUG ***/
      mxArray *pllh[1],*prrh[1];
      prrh[0] = prdimg;
      mexCallMATLAB(0, pllh, 1, prrh, "dbgimg");
#endif
      plhs[0] = prdimg;
      }

   return;

   } /* End citmexget() */

void mexFunction(int nlhs, mxArray *plhs[],
      int nrhs, const mxArray *prhs[]) {
   citmexget(nlhs, plhs, nrhs, prhs);
   } /* End mexFunction() */

