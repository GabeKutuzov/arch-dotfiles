/* (c) Copyright 2011, The Rockefeller University *11113* */
/* $Id: norbmexget.c 1 2012-01-13 20:05:02Z  $ */
/***********************************************************************
*                             norbmexget                               *
************************************************************************
*  This is the image acquisition routine for a Matlab mex-function     *
*  package to read images from the "small NORB dataset" by Fu Jie      *
*  Huang and Yann LeCun, Courant Institute, New York University.       *
*  See:  Y. LeCun, F.J. Huang, and L. Bottou, Learning methods for     *
*  generic object recognition with invariance to pose and lighting,    *
*  IEEE Computer Society Conference on Computer Vision and Pattern     *
*  Recognition (CVPR), 2004.                                           *
*                                                                      *
*  For convenience, documentation for the entire set of routines is    *
*  contained in the norbmexset source file.  This file only contains   *
*  information for norbmexget.                                         *
*----------------------------------------------------------------------*
*  Image acquisition call (make only after norbmexset call succeeds):  *
*     [image, isid] = norbmexget(NorbMexComm)                          *
*  Call to obtain just the isid information for next scheduled image:  *
*     isid = norbmexget(NorbMexComm)                                   *
*                                                                      *
*  Action:  Both calls return the 'isid' information detailed below    *
*     for the next image in the sequence specified by the selectors    *
*     given in the last norbmexset call.  If an 'image' argument is    *
*     also present, the actual image is also returned.                 *
*                                                                      *
*  Arguments:                                                          *
*     NorbMexComm    Data array returned by initial norbmexset call.   *
*                                                                      *
*  Return value:                                                       *
*     image    Matlab array of size niy x nix uint8 gray-scale values. *
*     isid     Matlab uint16 vector of 6 elements containing stimulus  *
*              identification info as follows (all count from 0)       *
*              isid(1) = Image group (category) number (0-4)           *
*              isid(2) = Image instance number (0-9)                   *
*              isid(3) = Azimuth (0-17)                                *
*              isid(4) = Elevation (0-8)                               *
*              isid(5) = Lighting (0-5)                                *
*              isid(6) = Camera (0-1)                                  *
*                                                                      *
*  Error conditions:  The script is terminated with an error message   *
*     on all error conditions.                                         *
************************************************************************
*  V1A, 03/16/11, GNR - New program, based on norbmix.c                *
*  V1B, 03/23/11, GNR - Add norbmexget call for isid info only.        *
*  V1C, 05/06/11, GNR - Add '+' and 'N' codes                          *
*  V2A, 12/01/11, GNR - Return transpose of image, 16 bit isids        *
*  ==>, 12/02/11, GNR - Last mod before committing to svn repository   *
***********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "mex.h"
#include "sysdef.h"
#include "rkarith.h"
#include "norbmex.h"

/* Location of common data array (for error exits) */
static const mxArray *pNorbMexComm;


/*=====================================================================*
*                             nmgFreeAll                               *
*                                                                      *
*  This routine closes all files and frees all storage.  It performs   *
*  the same functions as nmFreeAll, which is registered as mexAtExit   *
*  routine by norbmexset.  Checking sigck should be unnecessary, but   *
*  is here JIC.  nmgFreeAll does not attempt mxDestroyArray because it *
*  has read-only access to NorbMexComm--the MATLAB automatic disposal  *
*  mechanism should work on NorbMexComm once the child allocations     *
*  have been freed by this routine.                                    *
*=====================================================================*/

static void nmgFreeAll(void) {

   if (pNorbMexComm) {
      struct UTVSTATE *pst;
      struct UTVSPEC *pss,*pnxtss;
      int idf;

      pst = mxGetData(pNorbMexComm);
      pNorbMexComm = NULL;
      if (!pst || pst->sigck != NM_SIGCK) return;

      for (idf=DF_TRAIN; idf<=DF_TEST; ++idf) {
         struct UTVFILE *pdf = pst->ndf + idf;
         if (pdf->fdimg > 0) close(pdf->fdimg);
         }

      for (pss=pst->pfss; pss; pss=pnxtss) {
         pnxtss = pss->pnss;     /* Save ptr to next while freeing */
         if (pss->pstbl) mxFree(pss->pstbl);
         mxFree(pss);
         }
      pst->pfss = NULL;

      if (pst->bfname) mxFree(pst->bfname);
      }

   } /* End nmgFreeAll() */


/*=====================================================================*
*                        nmgMexErr, nmgMexErr2                         *
*                                                                      *
*  These are wrappers for mexErrMsgTxt().  They concatenate the error  *
*  id number and one or two text strings, call nmgFreeAll(), then      *
*  mexErrMsgTxt().  (mexErrMsgIdAndTxt() requires the id number to     *
*  start with a letter, so it was not used.)                           *
*=====================================================================*/

static void nmgMexErr(char *pid, const char *pmsg) {

   char etxt[LNSIZE+1];
   int  lmsg = (LNSIZE - 2) - strlen(pid);

   nmgFreeAll();
   strncpy(etxt, pid, LNSIZE-2);
   strcat(etxt, ": ");
   if (lmsg > 0) strncat(etxt, pmsg, lmsg);
   mexErrMsgTxt(etxt);

   } /* End nmgMexErr() */

static void nmgMexErr2(char *pid,
      const char *pmsg1, const char *pmsg2) {

   char etxt[LNSIZE+1];
   int  lmsg = (LNSIZE - 2) - strlen(pid);

   nmgFreeAll();
   strncpy(etxt, pid, LNSIZE-2);
   strcat(etxt, ": ");
   if (lmsg <= 0) goto nmME2_Full;
   strncat(etxt, pmsg1, lmsg);
   if ((lmsg -= strlen(pmsg1)+2) <= 0) goto nmME2_Full;
   strcat(etxt, " ");
   strncat(etxt, pmsg2, lmsg);
   strcat(etxt, ".");
nmME2_Full:
   mexErrMsgTxt(etxt);

   } /* End nmgMexErr2() */


/*=====================================================================*
*                              norbr1im                                *
*                                                                      *
*  Read an image, taking into account sub-windowing.  Assumes          *
*  suitable constants have been set up in UTVSTATE.  Arguments:        *
*     pst            Ptr to UTVSTATE.                                  *
*     pimi           Ptr to UTVINFO for the requested image.           *
*     pimage         Ptr to image.                                     *
*  Return value:  Image in pimage argument.  All errors terminate the  *
*     script.                                                          *
*  Note:  Matlab appears to expect the first dimension (fast moving)   *
*     of an image to be the column direction, as in a normal FORTRAN   *
*     matrix.  Therefore, this code was modified to take the transpose *
*     of the stored NORB image while reading it. Because transposition *
*     is trivial in the calling Matlab code, there is no provision to  *
*     select one or the other method here. (Even if this method is not *
*     really faster than a Matlab transpose operator, it allows norb-  *
*     mexget and citmexget to be called via a function handle and give *
*     same result orientation.  There is no longer a case to read the  *
*     whole image at once.)                                            *
*=====================================================================*/

static void norbr1im(struct UTVSTATE *pst, struct UTVINFO *pimi,
      PIXEL *pimage) {

   struct UTVFILE *pdf;                /* Ptr to data file info */
   PIXEL *ptrd,*ptrde;                 /* Ptr to row temp & end */
   PIXEL *col;                         /* Ptr to column */
   PIXEL *row,*rowe;                   /* Ptr to row, row end */
   off_t limgo;                        /* Local image offset */
   size_t lrem;                        /* Length remaining */
   int  idf = pimi->imdf;              /* Database file number */
   int  fdi;                           /* Image file descriptor */

   static char *pfnm[NNDF] = { "training file", "testing file" };

   pdf = pst->ndf + idf;
   fdi = pdf->fdimg;                   /* Image file descriptor */
   limgo = (off_t)pimi->imgno * pdf->limg + pdf->lskp1 +
      sizeof(struct NORBHDR);
   if (lseek(fdi, limgo, SEEK_SET) < 0)
      nmgMexErr2("norbmex 625", "Seek error on", pfnm[idf]);
   rowe = pimage + pst->tvy;
   for (row=pimage; row<rowe; ++row) {
      if (row > pimage && pdf->lskp2 &&
         lseek(fdi, pdf->lskp2, SEEK_CUR) < 0)
            nmgMexErr2("norbmex 625", "Seek error on", pfnm[idf]);
      ptrd = pdf->ptrdl;
      lrem = (size_t)pst->tvx;
      while (lrem > 0) {
         size_t lr = read(fdi, ptrd, lrem);
         if (lr <= 0)
            nmgMexErr2("norbmex 616", "Read error on", pfnm[idf]);
         ptrd += lr;
         lrem -= lr;
         }
      /* Now transpose */
      ptrd = pdf->ptrdl;
      ptrde = ptrd + pst->tvx;
      lrem = (size_t)pst->tvy;
      for (col=row; ptrd<ptrde; ++ptrd,col+=lrem)
         *col = *ptrd;
      }
   return;
   } /* End norbr1im() */


/*=====================================================================*
*                             norbmexget                               *
*=====================================================================*/

void mexFunction(int nlhs, mxArray *plhs[],
      int nrhs, const mxArray *prhs[]) {

   struct UTVSTATE *pst;
   struct UTVSPEC *pss;
   struct UTVINFO *pimi;
   PIXEL *pimage;             /* Ptr to output image */
   ui16  *pisid;              /* Ptr to image id info */
   int    jsid;               /* Index into isid array */

/* Standard MATLAB argument checking */

   if (nrhs != 1 || !mxIsClass(prhs[0],"uint8"))
      nmgMexErr("norbmex 602","1st and only arg must be NorbMexComm.");
   pNorbMexComm = prhs[0];
   pst = (struct UTVSTATE *)mxGetData(prhs[0]);
   if (!pst || pst->sigck != NM_SIGCK)
      nmgMexErr("norbmex 602", "Bad NorbMexComm arg.");

   pimage = NULL;             /* Omit image if NULL */
   switch (nlhs) {
   case 2:
      plhs[0] = mxCreateNumericMatrix((mwSize)pst->tvy,
         (mwSize)pst->tvx, mxUINT8_CLASS, mxREAL);
      pimage = (PIXEL *)mxGetData(plhs[0]);
      if (!pimage) nmgMexErr("norbmex 606",
         "Unable to allocate space for image.");
      /* ... drop through to case 1 ... */
   case 1:
      plhs[nlhs-1] = mxCreateNumericMatrix((mwSize)NSVARS,
         (mwSize)1, mxUINT16_CLASS, mxREAL);
      pisid = (ui16 *)mxGetData(plhs[nlhs-1]);
      if (!pisid) nmgMexErr("norbmex 606",
         "No space for image id info.");
      break;
   default:
      nmgMexErr("norbmex 626", "Two return args required.");
      } /* End nlhs switch */

/* Select next image.  If ivar is 0, it is time to start scanning at a
*  random UTVSPEC and random point in its pstbl.  */

   if (pst->ivar == 0) {
      ui32 rr;
      pss = pst->pfss;
      if (pst->nspec > 1) {
         /* It is very unlikely that there will be more than 2 or 3
         *  selection specs, so a simple linked-list crawl is used
         *  here instead of a binary search.  The random number seed
         *  of the first UTVSPEC is used for simplicity.  The prob
         *  of the last UTVSPEC is larger than the largest udev, so
         *  the loop should never drop through with pss == NULL.  */
         rr = (ui32)udev(&pss->seed);
         for ( ; pss; pss=pss->pnss) {
            if (rr <= pss->prob) break;
            }
         }
      /* Got a spec, now pick an image */
      rr = (ui32)udev(&pss->seed);
      pst->pcss = pss;
      pst->pran = pss->pstbl + (rr % pss->nstr)*pss->nsts;
      }
   else
      pss = pst->pcss;
   /* Now pick next sequential or random image from nsts list */
   if (pss->kbatch)
      pimi = pst->pran + pst->ivar;
   else {
      ui32 rr = (ui32)udev(&pss->seed);
      pimi = pst->pran + (rr % pss->nsts);
      }
   /* Advance ivar for next time */
   pst->ivar += 1;
   if (pst->ivar >= pss->nbatch) pst->ivar = 0;

   /* Read the image if a pointer was provided */
   if (pimage) norbr1im(pst, pimi, pimage);

/* Return image info to caller */

   for (jsid=0; jsid<NSVARS; ++jsid)
      pisid[jsid] = (ui16)pimi->isid[jsid];

/* All is well, return to caller */

   return;

   } /* End norbmexget() */
