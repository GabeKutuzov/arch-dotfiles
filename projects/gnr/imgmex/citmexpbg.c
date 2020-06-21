/* (c) Copyright 2012, The Rockefeller University *11113* */
/* $Id: citmexpbg.c 9 2012-11-21 22:00:13Z  $ */
/***********************************************************************
*                             citmexpbg                                *
************************************************************************
*  This is the routine to paint out the background of an image from    *
*  the "Caltech" image database using the object coordinates stored    *
*  in the annotation file for that image.  Pixels outside the object   *
*  polygon (possibly extended with a specified narrow border) are set  *
*  to 0.  This routine requires that citmexget() be called earlier     *
*  with kopt option bit 8 set in order to read in the image and the    *
*  annotation information.  The object coordinates are stored in the   *
*  pst->pitwk data for use by citmexpbg().                             *
*----------------------------------------------------------------------*
*  Background painting call (make only after citmexset and citmexget   *
*  calls have succeeded):                                              *
*     imgm = citmexpbg(CitMexComm, image)                              *
*                                                                      *
*  Action:  Pixels in the input image outside the object polygon       *
*     defined in the annotation file and stored by a previous call     *
*     to citmexget() are set to 0 and the modified image is returned.  *
*                                                                      *
*  Arguments:                                                          *
*     CitMexComm    Data array returned by initial citmexset() call.   *
*     image    Matlab uint8 matrix containing image to be painted.     *
*              Per Matlab convention, this image is transposed, i.e.   *
*              the vertical coordinate is fast moving.  It must be the *
*              same size as the image returned by citmexget(), but it  *
*              may have been processed prior to background painting,   *
*              e.g., by applying a median filter, Canny edge detector, *
*              etc.                                                    *
*                                                                      *
*  Return value:                                                       *
*     imgm     The modified image, returned as a uint8 array of the    *
*              same size and color mode (RGB or grayscale) as the      *
*              input image.                                            *
*                                                                      *
*  Error conditions:  The script is terminated with an error message   *
*     on all error conditions.                                         *
************************************************************************
*  V1A, 11/09/12, GNR - New program, based on code in citmexget.c      *
*  ==>, 11/09/12, GNR - Last mod before committing to svn repository   *
*  V3B, 11/21/12, GNR - Use iterepgf to add border around image paint  *
***********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <semaphore.h>
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

   char *etxt = ssprintf(NULL, "citmexpbg %d: %s.", rc, pmsg);
   cmFreeAll();
   mexErrMsgTxt(etxt);

   } /* End cmMexErr() */


/*=====================================================================*
*                              citmexpbg                               *
*=====================================================================*/

void citmexpbg(int nlhs, mxArray *plhs[],
      int nrhs, const mxArray *prhs[]) {

   IterEpgf P;             /* Polygon iterator data */
#ifdef DEBUG               /*** DEBUG ***/
   mxArray *pdbgo[1];      /* Dummy dbgimg output */
#endif
   struct C101STATE *pst;
   byte *pimgi;            /* Ptr to input image data */
   byte *pim;              /* Ptr to output image */
   byte *pwim;             /* Ptr to working image */
   xyf  *pgon;             /* Ptr to polygon coords */
   const mwSize *pimgd;    /* Ptr to image dimensions */
   mwSize ivtx,jvtx,nvtx;  /* Vertex counters */
   mwSize ndims;
   mwSize nsx, nsy;        /* x,y size of image */
   long l1img, l2img;      /* Length of image, 2*length of image */
   int  mode;              /* iterepgf mode */

/* Standard MATLAB argument checking */

   if (nlhs != 1 || nrhs != 2)
      cmMexErr(656, "Wrong number of arguments");

   if (!mxIsUint8(pCitMexComm = prhs[0]) ||
        !(pst = (struct C101STATE *)mxGetData(prhs[0])) ||
         (pst->sigck != C101_SIGCK) ||
         ((pst->cmflgs & (cmMASK|cmDEFMSK)) != cmDEFMSK))
      cmMexErr(632, "Bad CitMexComm arg");

   if (!mxIsUint8(prhs[1]) ||
        !(pimgi = (byte *)mxGetData(prhs[1])))
      cmMexErr(656, "Bad input image arg");
   ndims = mxGetNumberOfDimensions(prhs[1]);
   pimgd = mxGetDimensions(prhs[1]);
   nsx = pimgd[1], nsy = pimgd[0];
   l1img = nsx*nsy, l2img = l1img+l1img;

   plhs[0] = mxCreateNumericMatrix(nsy, nsx, mxUINT8_CLASS, mxREAL);
   pim = (byte *)mxGetData(plhs[0]);

/* Copy the input image to the output image */

#ifdef DEBUG /*** DEBUG *** Show input image */
   mexCallMATLAB(0,pdbgo,1,prhs[1],"dbgimg");
   mexPrintf("nsx=%d,nsy=%d,limg=%ld\n",nsx,nsy,l1img);
#endif

   memcpy(pim, pimgi, l1img);

/* Perform the actual paint job */

   nvtx = pst->nwvtx;
   pgon = (xyf *)(pst->pitwk + nvtx*sizeof(IEpgfWk));
   pwim = (byte *)pgon + nvtx*sizeof(xyf);
   /* For compat with old calls, default mode is 6 */
   mode = (pst->cmflgs & cmNDMODE) ?
      (int)(pst->cmflgs >> 4) : (EPGF_EXT|EPGF_RMB);
#ifdef DEBUG
   {  int ivtx;
      for (ivtx=0; ivtx<nvtx; ++ivtx)
         mexPrintf("Pbg vertex %d, x=%f, y=%f\n",
            ivtx,(double)pgon[ivtx].x,(double)pgon[ivtx].y);
      }
#endif

   InitEpgfIndexing(&P, pgon, (IEpgfWk *)pst->pitwk, pwim,
      pst->pbord, nsy, nsx, nvtx, mode);

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

#ifdef DEBUG /*** DEBUG *** Show output image */
   mexCallMATLAB(0,pdbgo,1,plhs[0],"dbgimg");
#endif

   return;

   } /* End citmexpbg() */


/* Wrapper to enable MATLAB debugging */

void mexFunction(int nlhs, mxArray *plhs[],
      int nrhs, const mxArray *prhs[]) {
   citmexpbg(nlhs, plhs, nrhs, prhs);
   } /* End mexFunction() */

