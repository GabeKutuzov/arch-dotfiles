/* (c) Copyright 2011, The Rockefeller University *11113* */
/* $Id: norfmexget.c 1 2012-01-13 20:05:02Z  $ */
/***********************************************************************
*                             norfmexget                               *
************************************************************************
*  This is the feature response reading routine for a Matlab mex-      *
*  function package to read prestored responses of MIT 'C2' features   *
*  to images from the "small NORB dataset" by Fu Jie Huang and Yann    *
*  LeCun, Courant Institute, New York University.                      *
*  See:  Y. LeCun, F.J. Huang, and L. Bottou, Learning methods for     *
*  generic object recognition with invariance to pose and lighting,    *
*  IEEE Computer Society Conference on Computer Vision and Pattern     *
*  Recognition (CVPR), 2004.                                           *
*                                                                      *
*  For convenience, documentation for the entire set of routines is    *
*  contained in the norfmexset source file.  This file only contains   *
*  information for norfmexget.                                         *
*----------------------------------------------------------------------*
*  Image acquisition call (make only after norfmexset call succeeds):  *
*     [featVec, isid] = norfmexget(NorfMexComm)                        *
*  Call to obtain just the isid information for next scheduled image:  *
*     isid = norfmexget(NorfMexComm)                                   *
*                                                                      *
*  Action:  Both calls return the 'isid' information detailed below    *
*     for the next image in the sequence specified by the selectors    *
*     given in the last norfmexset call.  If a 'featVec' argument is   *
*     also present, the responses to that image are also returned.     *
*                                                                      *
*  Arguments:                                                          *
*     NorfMexComm    Data array returned by initial norfmexset call.   *
*                                                                      *
*  Return value:                                                       *
*     featVec  Matlab vector of nfeats (max 4075) uint16 responses to  *
*              the selected image.  Values are binary scale S14 and    *
*              unknown values are coded as -32767.                     *
*     isid     Matlab uint16 vector of 6 elements containing stimulus  *
*              identification info as follows (all count from 0)       *
*              isid(1) = Image group (category) number (0-4)           *
*              isid(2) = Image instance number (0-9)                   *
*              isid(3) = Azimuth (0-17)                                *
*              isid(4) = Elevation (0-8)                               *
*              isid(5) = Lighting (0-5)                                *
*              isid(6) = Camera (0 only with existing data file)       *
*                                                                      *
*  Error conditions:  The script is terminated with an error message   *
*     on all error conditions.                                         *
************************************************************************
*  V1A, 04/15/11, GNR - New program, based on norbmexget.c             *
*  V1B, 04/19/11, GNR - Add byte swapping                              *
*  V1C, 05/02/11, GNR - Add '+' and 'N' codes                          *
*  V2A, 12/02/11, GNR - Return 16-bit isids                            *
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
#include "norfmex.h"

/* Location of common data array (for error exits) */
static const mxArray *pNorfMexComm;


/*=====================================================================*
*                             nmgFreeAll                               *
*                                                                      *
*  This routine closes all files and frees all storage.  It performs   *
*  the same functions as nmFreeAll, which is registered as mexAtExit   *
*  routine by norfmexset.  Checking sigck should be unnecessary, but   *
*  is here JIC.  nmgFreeAll does not attempt mxDestroyArray because it *
*  has read-only access to NorfMexComm--the MATLAB automatic disposal  *
*  mechanism should work on NorfMexComm once the child allocations     *
*  have been freed by this routine.                                    *
*=====================================================================*/

static void nmgFreeAll(void) {

   if (pNorfMexComm) {
      struct NFMSTATE *pst;
      struct NFMSPEC *pss,*pnxtss;

      pst = mxGetData(pNorfMexComm);
      pNorfMexComm = NULL;
      if (!pst || pst->sigck != NM_SIGCK) return;

      if (pst->fdff > 0) { close(pst->fdff); pst->fdff = 0; }

      for (pss=pst->pfss; pss; pss=pnxtss) {
         pnxtss = pss->pnss;     /* Save ptr to next while freeing */
         if (pss->pstbl) mxFree(pss->pstbl);
         mxFree(pss);
         }
      pst->pfss = NULL;

      if (pst->fname) mxFree(pst->fname);
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
*                             norfmexget                               *
*=====================================================================*/

void mexFunction(int nlhs, mxArray *plhs[],
      int nrhs, const mxArray *prhs[]) {

   struct NFMSTATE *pst;
   struct NFMSPEC *pss;
   struct NFMINFO *prfi;
   si16  *pfvec;              /* Ptr to output feature vector */
   ui16  *pisid;              /* Ptr to image id info */
   int    jsid;               /* Index into isid array */

/* Standard MATLAB argument checking */

   if (nrhs != 1 || !mxIsClass(prhs[0],"uint8"))
      nmgMexErr("norfmex 602","1st and only arg must be NorfMexComm.");
   pNorfMexComm = prhs[0];
   pst = (struct NFMSTATE *)mxGetData(prhs[0]);
   if (!pst || pst->sigck != NM_SIGCK)
      nmgMexErr("norfmex 602", "Bad NorfMexComm arg.");

   pfvec = NULL;             /* Omit responses if NULL */
   switch (nlhs) {
   case 2:
      plhs[0] = mxCreateNumericMatrix((mwSize)pst->nfeat,
         (mwSize)1, mxINT16_CLASS, mxREAL);
      pfvec = (si16 *)mxGetData(plhs[0]);
      if (!pfvec) nmgMexErr("norfmex 629",
         "Unable to allocate space for response vector.");
      /* ... drop through to case 1 ... */
   case 1:
      plhs[nlhs-1] = mxCreateNumericMatrix((mwSize)NSVARS,
         (mwSize)1, mxUINT16_CLASS, mxREAL);
      pisid = (ui16 *)mxGetData(plhs[nlhs-1]);
      if (!pisid) nmgMexErr("norfmex 629",
         "No space for image id info.");
      break;
   default:
      nmgMexErr("norfmex 628", "Two return args required.");
      } /* End nlhs switch */

/* Select next image.  If ivar is 0, it is time to start scanning at a
*  random NFMSPEC and random point in its pstbl.  */

   if (pst->ivar == 0) {
      ui32 rr;
      pss = pst->pfss;
      if (pst->nspec > 1) {
         /* It is very unlikely that there will be more than 2 or 3
         *  selection specs, so a simple linked-list crawl is used
         *  here instead of a binary search.  The random number seed
         *  of the first NFMSPEC is used for simplicity.  The prob
         *  of the last NFMSPEC is larger than the largest udev, so
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
      prfi = pst->pran + pst->ivar;
   else {
      ui32 rr = (ui32)udev(&pss->seed);
      prfi = pst->pran + (rr % pss->nsts);
      }
   /* Advance ivar for next time */
   pst->ivar += 1;
   if (pst->ivar >= pss->nbatch) pst->ivar = 0;

   /*** DEBUG ***/
#if 0
   mexPrintf("Retrieving isid = %d,%d,%d,%d,%d,%d at %d\n",
      (int)prfi->isid[0], (int)prfi->isid[1], (int)prfi->isid[2],
      (int)prfi->isid[3], (int)prfi->isid[4], (int)prfi->isid[5],
      (int)prfi->imgno);
#endif

   /* Read the response vector if a pointer was provided */
   if (pfvec) {

#if BYTE_ORDRE < 0
      /* N.B.  When matlab reads or writes a binary file, the
      *  default byte order is big-endian, so our response file
      *  is big-endian and needs to be swapped when read on a
      *  little-endian machine.  */
      byte *pfve,*pfv = (byte *)pfvec;
#endif
      off_t limgo;                  /* Local record offset */
      off_t lrem;                   /* Length remaining */
      int fdi = pst->fdff;          /* Image file descriptor */

      lrem = pst->nfeat * sizeof(si16);
#if BYTE_ORDRE < 0
      pfve = pfv + lrem;
#endif
      limgo = (off_t)prfi->imgno * lrem;
      if (lseek(fdi, limgo, SEEK_SET) < 0)
         nmgMexErr2("norfmex 625", "Seek error on", pst->fname);
      while (lrem > 0) {
         si32 lr = (si32)read(fdi, pfvec, lrem);
         if (lr <= 0)
            nmgMexErr2("norfmex 616", "Read error on", pst->fname);
         pfvec += lr;
         lrem -= lr;
         }
#if BYTE_ORDRE < 0                  /* Swap byte order */
      for ( ; pfv<pfve; pfv+=sizeof(si16))
         pfv[0] ^= pfv[1], pfv[1] ^= pfv[0], pfv[0] ^= pfv[1];
#endif
      } /* End reading response vector */

/* Return image info to caller */

   for (jsid=0; jsid<NSVARS; ++jsid)
      pisid[jsid] = (ui16)prfi->isid[jsid];

/* All is well, return to caller */

   return;

   } /* End norfmexget() */
