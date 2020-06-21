/* (c) Copyright 2011, The Rockefeller University *11113* */
/* $Id: norbmexset.c 1 2012-01-13 20:05:02Z  $ */
/***********************************************************************
*                             norbmexset                               *
************************************************************************
*  This is the setup routine for a Matlab mex-function package to read *
*  images from the "small NORB dataset" by Fu Jie Huang and Yann LeCun,*
*  Courant Institute, New York University.  See:  Y. LeCun, F.J. Huang,*
*  and L. Bottou, Learning methods for generic object recognition with *
*  invariance to pose and lighting, IEEE Computer Society Conference   *
*  on Computer Vision and Pattern Recognition (CVPR), 2004.            *
*                                                                      *
*  The mex-function package is based on norbmix.c, the CNS NORB-image- *
*  reading plugin for CNS also written by GNR.                         *
*                                                                      *
*  This is the ultimate NORB image reader, designed to select objects  *
*  at random based on some identification criteria, and then to cycle  *
*  systematically or at random through all or some images that meet    *
*  other criteria.  This may be useful for training runs in which e.g. *
*  different rotations of an object should be viewed sequentially, but *
*  the objects should be otherwise random.  This version also allows   *
*  the user to specify more than one set of selection criteria, each   *
*  to be used with some given probability, and to allow changing the   *
*  selection criteria within a single run.  It also allows reading     *
*  images from both the "training" and "test" files in the same run    *
*  (in spite of their different internal arrangement).                 *
*                                                                      *
*  Multiple functions are performed by norbmexset according to the     *
*  arguments received.  For convenience, documentation for this and    *
*  all functions contained in other source files is contained here.    *
*----------------------------------------------------------------------*
*  Initialization call:                                                *
*     NorbMexComm = norbmexset(ufile, nix, niy, nixo, niyo, udata)     *
*  Revise sequence call (make only after init call succeeds):          *
*     norbmexset(NorbMexComm, udata)                                   *
*  Reset call (make only after init call succeeds):                    *
*     norbmexset(NorbMexComm, 'reset')                                 *
*  Closeout call (make only after init call succeeds):                 *
*     norbmexset(NorbMexComm, 'close')                                          *
*                                                                      *
*  Actions:  The initialization call must be made first.  It prepares  *
*     the package for image acquisition according to a specified se-   *
*     quence of random and systematic selections.  This sequence can   *
*     be changed later by calling norbmexset with the two arguments    *
*     'NorbMexComm' and a new 'udata'.  If 'udata' is literally the    *
*     word 'reset' (any capitalization), the image sequence is not     *
*     changed but is restarted from the beginning.  In the present     *
*     design, this amounts to resetting the random number seed and     *
*     initial state of each systematic selection loop.  All files      *
*     are closed and all allocated memory is released by a call with   *
*     the two arguments 'NorbMexComm' and the literal word 'close',    *
*     or, for compatibility with earlier versions, just the one        *
*     argument 'NorbMexComm'.                                          *
*                                                                      *
*  Arguments:                                                          *
*     ufile    A Matlab string giving a partial or full path to the    *
*              six "-dat.mat", "-cat.mat", and "-info.mat" files of    *
               the NORB dataset.  This may refer to either the test    *
*              or training data set--both sets must be in the same     *
*              directory.  Only the directory information is used--    *
*              the file names are supplied internally.                 *
*     nix      Size of the image data to be returned along x.          *
*     niy      Size of the image data to be returned along y.          *
*     nixo     Offset of the data to be returned along x from the      *
*              left edge of the source images, nix + nixo <= 96.       *
*     niyo     Offset of the data to be returned along y from the      *
*              top edge of the source images, niy + niyo <= 96.        *
*     udata    Matlab string giving the image data selectors, cate-    *
*              gorization mode, and random number seed.  This will     *
*              generally be derived from user input.                   *
*     NorbMexComm    See below.  Once the initial norbmexset call has  *
*              been completed, all other calls must pass as argument   *
*              the array returned by the initialization call.          *
*                                                                      *
*  Contents of the udata string:  One or more selector specifications  *
*     separated by semicolons.  Any or all of the selector specs may   *
*     be assigned a probability ('P' code, see below).  The assigned   *
*     P values must add to 1.0 (within a small error) or less if any   *
*     selector specs do not have P values assigned.  The difference    *
*     between 1.0 and the sum of the assigned P values is divided      *
*     equally among all selector specs that do not have an assigned    *
*     P value.  On each norbmexget call that is not part of a selec-   *
*     tion sequence, one of the specs is chosen at random with the     *
*     given probability, and a stimulus is then chosen according to    *
*     that spec.  Any specified cycle is then completed on subsequent  *
*     norbmexget calls.  A new selector spec is chosen on the next     *
*     call after the cycle is completed.                               *
*        A selector spec comprises up to six selector strings in any   *
*     order.  Intermixed among these (but normally following them) in  *
*     any order may be a probability code, a random number seed code,  *
*     and/or a batch number code.  All these codes are separated by    *
*     commas, except the comma after one selector string may be        *
*     replaced with a slash or a plus sign.                            *
*        Only those images will be returned whose properties match     *
*     selector codes.  Selectors to the left of the '/' or '+' will    *
*     be used in random order.                                         *
*        If a '/' is entered, selections from specs to the right of    *
*     the '/' will be cycled through in the order given.  If a '+' is  *
*     entered, selection from specs to the right of the '+' are also   *
*     randomized.  In both cases, the number of selections before      *
*     picking a new object from the selection group to the left of     *
*     the '/' or '+', is the number given by the batch number code     *
*     'N', or if no 'N' is given, then the batch count is the total    *
*     number of images in the right-hand selection spec.  If no '/'    *
*     or '+' is given, all selectors are randomized.                   *
*        Each selector string comprises a single letter followed by a  *
*     numeric selector list enclosed in parentheses.  The initial let- *
*     ter determines which variable that list applies to, abbreviated  *
*     as:  V (camera view), L (lighting), C (category), I (instance),  *
*     E (elevation), and A (azimuth).  Each numeric selector list is   *
*     a set of integers and/or ranges separated by commas. An integer  *
*     selects an allowed value of an image property.  A range is a     *
*     starting value followed by a minus sign and an ending value.     *
*     The allowed selectors for each variable are:  (V) camera         *
*     (0 to 1), (L) light condition (0 to 5), (C) object category      *
*     (0 to 4), (I) object instance (0 to 9), (E) elevation (0 to 8,   *
*     encoding 30 to 70 deg. in 5 deg. intervals), and (A) azimuth     *
*     (0 to 17, encoding 0 to 340 deg. in 20 deg. intervals.  If a     *
*     list is absent, all values of that property will be accepted.    *
*        If the object instance is 0,1,2,3, or 5, the object will be   *
*     read from the "testing" file.  If the object instance is 4,6,7,  *
*     8, or 9, the object will be read from the "training" file.       *
*                                                                      *
*        A batch number code is the letter 'N' followed by an integer  *
*     in parentheses.                                                  *
*                                                                      *
*        A probability code is the letter 'P' followed by a fraction   *
*     between 0 and 1 in parentheses.  As explained above, any selec-  *
*     tor specs that are not assigned a probability will receive as    *
*     default an equal share of the difference between 1.0 and the sum *
*     of the probabilites that are assigned to other specs.            *
*                                                                      *
*        A random number seed code is the letter 'S' followed by a     *
*     value for the random number seed (0 < seed < 2**31-1) in         *
*     parentheses.                                                     *
*                                                                      *
*  Return value:  The setup call returns a Matlab array 'NorbMexComm'. *
*     This array contains information that must remain available for   *
*     access by the other routines in the package.  Its contents are   *
*     of no concern to the Matlab user and should never be modified.   *
*     NorbMexComm may be cleared by a call to NorbMexClr().            *
*                                                                      *
*  Error conditions:  The arguments are screened for possible errors.  *
*     All error conditions lead to a call to mexErrMsgTxt, which will  *
*     display an error message and numeric code and terminate the      *
*     calling script.                                                  *
*----------------------------------------------------------------------*
*  Image acquisition call (make only after init call succeeds):        *
*     [image, isid] = norbmexget(NorbMexComm)                          *
*  Call to obtain just the isid information for next scheduled image:  *
*     isid = norbmexget(NorbMexComm)                                   *
*                                                                      *
*  Action:  Both calls return the 'isid' information detailed below    *
*     for the next image in the sequence specified by the selectors    *
*     given in the last norbmexset call.  If an 'image' argument is    *
      also present, the actual image is also returned.                 *
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
*----------------------------------------------------------------------*
*  Design notes:  The NorbMexComm data are passed explicitly between   *
*  the various functions rather than being placed in global because    *
*  Matlab insists on copying global data if you want to change them.   *
************************************************************************
*  V1A, 03/10/11, GNR - New program, based on norbmix.c                *
*  V1B, 03/23/11, GNR - Add norbmexget call for isid info only.        *
*  V1C, 05/06/11, GNR - Add '+' and 'N' codes                          *
*  V2A, 12/02/11, GNR - Return transpose of image, 16 bit isids        *
*  ==>, 12/02/11, GNR - Last mod before committing to svn repository   *
***********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "mex.h"
#include "sysdef.h"
#include "rksubs.h"
#include "rkarith.h"
#include "norbmex.h"

/*=====================================================================*
*                         SOURCE-FILE GLOBALS                          *
*=====================================================================*/

/* Location of common data array */
static mxArray *pNorbMexComm;

/* Names and maximum values of the various types of selectors */
static const char *vnms[NSVARS] = {
   "category", "instance", "azimuth", "elevation", "light", "camera" };
static const int mxvars[NSVARS] = {
   MX_CAT, MX_INST, MX_AZI, MX_ELV, MX_LGT, MX_CAM };


/*=====================================================================*
*                              freespec                                *
*                                                                      *
*  Free all data relating to image selection.                          *
*  Argument:                                                           *
*     pst      Ptr to master UTVSTATE struct                           *
*=====================================================================*/

static void freespec(struct UTVSTATE *pst) {

   struct UTVSPEC *pss,*pnxtss;

   if (!pst) return;
   for (pss=pst->pfss; pss; pss=pnxtss) {
      pnxtss = pss->pnss;        /* Save ptr to next while freeing */
      if (pss->pstbl) mxFree(pss->pstbl);
      mxFree(pss);
      }
   pst->pfss = NULL;
   pst->pcss = NULL;
   pst->pran = NULL;
   pst->nspec = pst->nprob = pst->ivar = 0;

   } /* End freespec() */


/*=====================================================================*
*                              closeall                                *
*                                                                      *
*  Close any open files.  (Errors are ignored because this routine     *
*  will normally be called when processing some other error or when    *
*  the program is terminating anyway.)                                 *
*                                                                      *
*  Argument:                                                           *
*     pst      Ptr to master UTVSTATE struct                           *
*=====================================================================*/

static void closeall(struct UTVSTATE *pst) {

   int idf;

   for (idf=DF_TRAIN; idf<=DF_TEST; ++idf) {
      struct UTVFILE *pdf = pst->ndf + idf;
      if (pdf->fdimg > 0) {
         close(pdf->fdimg);
         pdf->fdimg = 0;   /* Prevent doing this twice */
         }
      if (pdf->fdcat > 0) {
         close(pdf->fdcat);
         pdf->fdcat = 0;   /* Prevent doing this twice */
         }
      if (pdf->fdinf > 0) {
         close(pdf->fdinf);
         pdf->fdinf = 0;   /* Prevent doing this twice */
         }
      if (pdf->ptrdl) {
         mxFree(pdf->ptrdl);
         pdf->ptrdl = 0;   /* Prevent doing this twice */
         }
      }

   } /* End closeall() */


/*=====================================================================*
*                              nmFreeAll                               *
*                                                                      *
*  This routine closes all files and frees all storage.  It should be  *
*  used as the registered mexAtExit routine and is also called before  *
*  error exits once any permanent storage has been allocated.  It has  *
*  to find the UTVSTATE struct via a static pointer, as the mexAtExit  *
*  routine can have no arguments.  It does not attempt mxDestroyArray  *
*  because it has read only access to NorbMexComm--the MATLAB automatic*
*  disposal mechanism should work on NorbMexComm once the child allo-  *
*  cations have been freed by this routine.  The sigck should prevent  *
*  disaster if called with a stale pointer after NorbMexComm already   *
*  destroyed.                                                          *
*=====================================================================*/

static void nmFreeAll(void) {

   if (pNorbMexComm) {
      struct UTVSTATE *pst = mxGetData(pNorbMexComm);
      if (pst->sigck == NM_SIGCK) {
         closeall(pst);
         freespec(pst);
         if (pst->bfname) mxFree(pst->bfname);
         }
      pNorbMexComm = NULL;
      }

   } /* End nmFreeAll() */


/*=====================================================================*
*                         nmMexErr, nmMexErr2                          *
*                                                                      *
*  These are wrappers for mexErrMsgTxt().  They concatenate the error  *
*  id number and one or two text strings, call nmFreeAll(), then       *
*  mexErrMsgTxt().  (mexErrMsgIdAndTxt() requires the id number to     *
*  start with a letter, so it was not used.)                           *
*=====================================================================*/

static void nmMexErr(char *pid, const char *pmsg) {

   char etxt[LNSIZE+1];
   int  lmsg = (LNSIZE - 2) - strlen(pid);

   nmFreeAll();
   strncpy(etxt, pid, LNSIZE-2);
   strcat(etxt, ": ");
   if (lmsg > 0) strncat(etxt, pmsg, lmsg);
   mexErrMsgTxt(etxt);

   } /* End nmMexErr() */

static void nmMexErr2(char *pid,
      const char *pmsg1, const char *pmsg2) {

   char etxt[LNSIZE+1];
   int  lmsg = (LNSIZE - 2) - strlen(pid);

   nmFreeAll();
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

   } /* End nmMexErr2() */


/*=====================================================================*
*                              norbpsel                                *
*  Parse a selector description.  Arguments:                           *
*     ppsel          Ptr to ptr to string being parsed (updated        *
*                    on non-error return).                             *
*     pcs            Ptr to UTVSEL where default selectors will be     *
*                    removed, rank order of selectors found in the     *
*                    input will be stored, and number of selectors     *
*                    stored in nvar.                                   *
*                    (Input order will be preserved).                  *
*     ikv            Type of index variable being parsed (from KV set) *
* Return value:  Number of selectors found.                            *
*=====================================================================*/

static int norbpsel(char **ppsel, struct UTVSEL *pcs, int ikv) {

   char *ps = *ppsel;         /* Ptr used to scan the input */
   byte  *svals = pcs->sv;    /* Locate results array */
   int   ib,ie;               /* Beginning, end of a range */
   int   rc;                  /* Return code */
   int   smax;                /* Max value of this selector type */

   if (*ps++ != '(') nmMexErr2("norbmex 607",
      "Missing '(' in selector for", vnms[ikv]);
   if (pcs->nvar > 0) nmMexErr2("norbmex 608",
      "Multiple selectors for", vnms[ikv]);

   /* Remove defaults */
   smax = mxvars[ikv];
   for (ib=0; ib<=smax; ++ib)
      svals[ib] = KILLIMG;

   rc = 0, ib = 0, ie = -1;
   for ( ; *ps; ++ps) {
      if (*ps >= '0' && *ps <= '9') {
         if (ie >= 0)
            ie = 10*ie + (*ps - '0');
         else
            ib = 10*ib + (*ps - '0');
         }
      else if (*ps == '-') {
         if (ie >= 0) break;
         ie = 0;
         }
      else if (*ps == ',' || *ps == ')') {
         if (ps[-1] == ',') break;
         if (ie < 0) ie = ib;
         if (ie < ib) break;
         if (ie > smax) break;
         for ( ; ib<=ie; ++ib) {
            svals[ib] = rc++;
            if (rc > smax) break;
            }
         if (*ps == ')') {
            *ppsel = ps+1;
            pcs->nvar = rc;
            return rc;
            }
         ib = 0, ie = -1;
         }
      else break;
      }
   nmMexErr2("norbmex 609",
      "Malformed selector spec for", vnms[ikv]);
   } /* End norbpsel() */


/*=====================================================================*
*                              norbswp4                                *
*                                                                      *
*  Do big-endian vs. little-endian byte swap on a 4-byte value.        *
*=====================================================================*/

#if BYTE_ORDRE > 0
static void norbswp4(void *pi4) {
   byte *pb = (byte *)pi4;
   pb[3] ^= pb[0]; pb[0] ^= pb[3]; pb[3] ^= pb[0];
   pb[2] ^= pb[1]; pb[1] ^= pb[2]; pb[2] ^= pb[1];
   } /* End norbswp4() */
#endif


/*=====================================================================*
*                              makespec                                *
*                                                                      *
*  Create an empty selector spec and initialize it.  The linked list   *
*  is constructed LIFO since it is simpler and the order doesn't       *
*  really matter.                                                      *
*                                                                      *
*  Argument:                                                           *
*     pst      Ptr to master UTVSTATE struct                           *
*=====================================================================*/

static struct UTVSPEC *makespec(struct UTVSTATE *pst) {

   struct UTVSPEC *pss =
      (struct UTVSPEC *)mxCalloc(1, sizeof(struct UTVSPEC));
   if (!pss) nmMexErr("norbmex 606", "No space for selector spec.");
   pss->pnss = pst->pfss;
   pst->pfss = pss;
   pst->nspec += 1;
   pss->seed0 = pst->dfltseed++;
   mexMakeMemoryPersistent(pss);
   return pss;

   } /* End makespec() */


/*=====================================================================*
*                              dfltrsel                                *
*                                                                      *
*  If any variables were defaulted (nvar == 0), now assign them        *
*  the fastest dimensions for random selection.  If no slash was       *
*  entered, pretend it was at the end (all random).  (To change        *
*  this default to all systematic, must also reverse nstr,nsts.)       *
*  Arguments:                                                          *
*     pss         Ptr to UTVSPEC to be updated                         *
*     islash      Position in selector list where slash found or       *
*                 -1 if no slash was found.                            *
*     lastiv      Number of selectors entered in this block            *
*                 (value of iv at end of block).                       *
*=====================================================================*/

static void dfltrsel(struct UTVSPEC *pss, int islash, int lastiv) {

   int jj,jv;

   if (islash < 0) islash = lastiv;
   for (jv=0; jv<NSVARS; ++jv) {
      struct UTVSEL *pcs = &pss->cs[jv];
      if (pcs->nvar == 0) {
         if (lastiv >= NSVARS) nmMexErr("norbmex 608",
            "More selectors than variables.");
         for (jj=lastiv++; jj>islash; --jj)
            pss->iso[jj] = pss->iso[jj-1];
         pss->iso[islash++] = jv;
         for (jj=0; jj<=mxvars[jv]; ++jj)
            pcs->sv[jj] = jj;
         pss->nstr *= (pcs->nvar = mxvars[jv]);
         }
      } /* End variable type loop */

   if (pss->nstr*pss->nsts > TOT_IMGS) nmMexErr("norbmex 610",
      "More selections than images in dataset.");
   } /* End dfltrsel() */


/*=====================================================================*
*                              norbmexset                                *
*=====================================================================*/

void mexFunction(int nlhs, mxArray *plhs[],
      int nrhs, const mxArray *prhs[]) {

   struct UTVSTATE *pst;      /* Ptr to persistent local data */
   struct UTVSPEC *pss;       /* Ptr to current selector spec */
   struct UTVSEL *pcs;        /* Ptr to selector info */
   char *prdd;                /* Ptr to data being read */
   char *udata;               /* Ptr to udata argument string */
   struct NORBHDR nhdr;       /* Headers */
   double tnxy,txyo;          /* Temps for nix,niy,nixo,niyo */
   ui32 nel;                  /* Number of eligible images */
   ui32 dfltprob;             /* Default probability */
   ui32 sumprob;              /* Sum of probabilities */
   int  idf;                  /* Data file index */
   int  islash;               /* Position where slash code found */
   int  iv,jv;                /* Variable loop indexes */
   int  ldat;                 /* Length of data being read */
   int  needidf[NNDF];        /* TRUE if need file idf */

   /* Which file each category instance is in */
   static const byte reqidf[MX_INST+1] = {
      DF_TEST, DF_TEST,  DF_TEST,  DF_TEST,  DF_TRAIN,
      DF_TEST, DF_TRAIN, DF_TRAIN, DF_TRAIN, DF_TRAIN  };

   /* The base parts of the NORB file names */
   static const char *ndsfnm[NNDF] = {
      "smallnorb-5x46789x9x18x6x2x96x96-training",
      "smallnorb-5x01235x9x18x6x2x96x96-testing" };

   /* Clear pointers to avoid mxFree() errors */
   pst = NULL; udata = NULL;

/* Standard MATLAB argument checking (except init call) */

   if (nrhs > 0 && nrhs < 3) {
      if (nlhs != 0) nmMexErr("norbmex 601",
         "Function must have no return value.");
      /* Access NorbMexComm data */
      if (!mxIsClass(prhs[0],"uint8")) nmMexErr("norbmex 602",
         "1st arg must be NorbMexComm.");
      pst = (struct UTVSTATE *)mxGetData(prhs[0]);
      if (!pst || pst->sigck != NM_SIGCK) nmMexErr("norbmex 602",
         "Bad NorbMexComm arg.");
      }

/* Determine type of call from number of arguments */

   switch (nrhs) {

/* Close-out call */

case 1:
   nmFreeAll();
   return;

/* Reset or revised udata call */

case 2:
   if (!mxIsChar(prhs[1])) nmMexErr("norbmex 603",
      "udata arg must be a string.");
   ldat = mxGetNumberOfElements(prhs[1]);
   if (ldat <= 0 || !(udata = mxMalloc(ldat+1)) ||
         mxGetString(prhs[1], udata, ldat+1))
      nmMexErr2("norbmex 604", "Unable to copy", "udata");
   if (ssmatch(udata, "close", 5)) {
      /* Close case -- same as with no second arg */
      nmFreeAll();
      mxFree(udata);
      return;
      }
   if (ssmatch(udata, "reset", 5)) {
      /* Reset case */
      for (pss=pst->pfss; pss; pss=pss->pnss)
         pss->seed = pss->seed0;
      pst->ivar = 0;
      return;
      }
   else {
      /* Remove previous selectors and read new ones */
      closeall(pst);
      freespec(pst);
      break;   /* Proceed as for initial call */
      }

/* Initialization call */

case 6:
   if (nlhs != 1) nmMexErr("norbmex 601",
      "Function must have one return value.");
   if (!mxIsChar(prhs[0]) || !mxIsDouble(prhs[1]) ||
         !mxIsDouble(prhs[2]) || !mxIsDouble(prhs[3]) ||
         !mxIsDouble(prhs[4]) || !mxIsChar(prhs[5]))
      nmMexErr("norbmex 603",
         "Argument(s) not of correct type(s).");
   if (mxGetNumberOfElements(prhs[1]) != 1 ||
         mxGetNumberOfElements(prhs[2]) != 1 ||
         mxGetNumberOfElements(prhs[3]) != 1 ||
         mxGetNumberOfElements(prhs[4]) != 1)
      nmMexErr("norbmex 603",
         "Argument(s) not of correct length(s).");

/* Create the NorbMexComm common data area and initialize.
*  Note:  mxCreateNumericMatrix terminates script on failure.  */

   pNorbMexComm = plhs[0] =
      mxCreateNumericMatrix(sizeof(struct UTVSTATE), 1,
      mxUINT8_CLASS, mxREAL);
   pst = (struct UTVSTATE *)mxGetData(pNorbMexComm);
   pst->sigck = NM_SIGCK;
   pst->dfltseed = DFLT_SEED;

/* From here on, there may be stuff to clean up on exit */

   mexAtExit(nmFreeAll);

/* Read and save the arguments */

   /* Make space to construct the three (or six) file names.  Either
   *  or both of training and test sets are accessed only if called
   *  for by the selectors.  */
   pst->lbnm = mxGetNumberOfElements(prhs[0]);
   pst->lfnm = pst->lbnm + strlen(ndsfnm[DF_TRAIN]) + 12;
   if (pst->lbnm <= 0 || !(pst->bfname = mxMalloc(pst->lfnm << 1)) ||
         mxGetString(prhs[0], pst->bfname, pst->lbnm+1))
      nmMexErr2("norbmex 604", "Unable to copy", "fname");
   mexMakeMemoryPersistent(pst->bfname);
   prdd = strrchr(pst->bfname, '/');
   if (prdd) pst->lbnm = prdd - pst->bfname;
   else pst->bfname[pst->lbnm] = '/';
   pst->fname = pst->bfname + pst->lfnm;

   tnxy = *mxGetPr(prhs[1]);
   txyo = *mxGetPr(prhs[3]);
   if (tnxy <= 0 || txyo < 0 || tnxy + txyo > MX_NIXY)
      nmMexErr("norbmex 605", "nix + nixo negative or > 96.");
   pst->tvx = (ui16)tnxy;
   pst->tvxo = (ui16)txyo;

   tnxy = *mxGetPr(prhs[2]);
   txyo = *mxGetPr(prhs[4]);
   if (tnxy <= 0 || txyo < 0 || tnxy + txyo > MX_NIXY)
      nmMexErr("norbmex 605", "niy + niyo negative or > 96.");
   pst->tvy = (ui16)tnxy;
   pst->tvyo = (ui16)txyo;

   /* Check amd extract udata arg */
   ldat = mxGetNumberOfElements(prhs[5]);
   if (ldat <= 0 || !(udata = mxMalloc(ldat+1)) ||
         mxGetString(prhs[5], udata, ldat+1))
      nmMexErr2("norbmex 604", "Unable to copy", "udata");
   break;

default:
   nmMexErr("norbmex 601", "Wrong number of arguments.");

   } /* End nrhs switch */

/* Items that need to be cleared on INIT or RESTART call */
needidf[DF_TRAIN] = needidf[DF_TEST] = FALSE;
sumprob = 0;

/* Start of a new spec (first or semicolon) will enter here.
*  Create a selector spec and initialize it.  */

StartSpecHere:
   pss = makespec(pst);

/* Parse the selectors first so files are not opened if there is
*  an error.  */

   if (*udata == '/')
      pss->kbatch = TRUE, islash = 0, udata += 1;
   else if (*udata == '+')
      islash = 0, udata += 1;
   else
      islash = -1;
   iv = 0;
   pss->nstr = pss->nsts = 1;
   while (*udata) {
      /* One of the documented letter codes is expected here */
      switch (toupper(*udata++)) {
      case 'C':                  /* Category selectors */
         nel = norbpsel(&udata, &pss->cs[KV_Cat], KV_Cat);
         if (islash >= 0) pss->nsts *= nel;
         else             pss->nstr *= nel;
         pss->iso[iv++] = KV_Cat;
         break;
      case 'I':                  /* Instance selectors */
         nel = norbpsel(&udata, &pss->cs[KV_Inst], KV_Inst);
         if (islash >= 0) pss->nsts *= nel;
         else             pss->nstr *= nel;
         pss->iso[iv++] = KV_Inst;
         break;
      case 'A':                  /* Azimuth selectors */
         nel = norbpsel(&udata, &pss->cs[KV_Azim], KV_Azim);
         if (islash >= 0) pss->nsts *= nel;
         else             pss->nstr *= nel;
         pss->iso[iv++] = KV_Azim;
         break;
      case 'E':                  /* Elevation selectors */
         nel = norbpsel(&udata, &pss->cs[KV_Elev], KV_Elev);
         if (islash >= 0) pss->nsts *= nel;
         else             pss->nstr *= nel;
         pss->iso[iv++] = KV_Elev;
         break;
      case 'L':                  /* Lighting selectors */
         nel = norbpsel(&udata, &pss->cs[KV_Light], KV_Light);
         if (islash >= 0) pss->nsts *= nel;
         else             pss->nstr *= nel;
         pss->iso[iv++] = KV_Light;
         break;
      case 'V':                  /* Camera selectors */
         nel = norbpsel(&udata, &pss->cs[KV_View], KV_View);
         if (islash >= 0) pss->nsts *= nel;
         else             pss->nstr *= nel;
         pss->iso[iv++] = KV_View;
         break;
      case 'N': {                /* Batch number code */
         si32 tbatch = 0;
         if (*udata++ != '(') nmMexErr("norbmex 607",
            "Missing '(' in batch number.");
         while (*udata && *udata != ')') {
            int md = (int)(*udata++);
            if (isdigit(md))
               tbatch = 10*tbatch + (md - '0');
            else nmMexErr("norbmex 612", "Nondigit in batch number.");
            }
         if (tbatch <= 0 || tbatch == SI32_MAX)
            nmMexErr("norbmex 612", "Invalid batch number.");
         pss->nbatch = tbatch;
         udata++;
         } /* End 'N' local scope */
         break;
      case 'P': {                /* Probability */
         ui32 tprob = 0, tdec = 1;
         int  ndig = 0;
         if (*udata++ != '(') nmMexErr("norbmex 607",
            "Missing '(' in P code.");
         if (pss->prob == 0) pst->nprob += 1;
         while (*udata && *udata != ')') {
            int md = (int)(*udata++);
            if (md == '.') tdec = 1;
            else if (isdigit(md)) {
               tprob = 10*tprob + (md - '0');
               tdec *= 10;
               ndig += 1; }
            else nmMexErr("norbmex 609",
               "Illegal character in P code.");
            }
         if (ndig >= UI32_SIZE) nmMexErr("norbmex 609",
            "Too many digits in P code.");
         /* Because we don't want to load ROCKS routines here,
         *  we use 64-bit if available, otherwise double float */
#ifdef HAS_I64
         {  ui64 tnum = ((ui64)tprob << 31)/(ui64)tdec;
            if (tnum > MX_PROB) nmMexErr("norbmex 611",
               "Probability > 1.");
            pss->prob = (ui32)tnum;
            } /* End tnum local scope */
#else
         {  double dmxp = (double)MX_PROB;
            double tnum = dmxp*(double)tprob/(double)tdec;
            if (tnum > dmxp) nmMexErr("norbmex 611",
               "Probability > 1.");
            pss->prob = (ui32)tnum;
            } /* End tnum local scope */
#endif
         sumprob += pss->prob;
         if (sumprob > (MX_PROB+MX_PROBERR))
            nmMexErr("norbmex 611", "Total probabilities > 1.");
         udata++;
         } /* End 'P' local scope */
         break;
      case 'S': {                /* Random number seed */
         si32 tseed = 0;
         if (*udata++ != '(') nmMexErr("norbmex 607",
            "Missing '(' in seed.");
         while (*udata && *udata != ')') {
            int md = (int)(*udata++);
            if (isdigit(md))
               tseed = 10*tseed + (md - '0');
            else nmMexErr("norbmex 612", "Nondigit in seed.");
            }
         if (tseed <= 0 || tseed == SI32_MAX)
            nmMexErr("norbmex 612", "Invalid seed.");
         pss->seed0 = tseed;
         udata++;
         } /* End 'S' local scope */
         break;
      case '/':                  /* Systematic/random separator */
         pss->kbatch = TRUE;
         /* Drop through to '+' case ... */
      case '+':                  /* Systematic batch separator */
         if (islash >= 0) nmMexErr("norbmex 613",
            "More than one plus or slash in selector string.");
         islash = iv;
         break;
      case ';':                  /* New selection block */
         dfltrsel(pss, islash, iv);
         goto StartSpecHere;
      default:                   /* None of the above: error */
         nmMexErr("norbmex 614", "Unrecognized selector code.");
         } /* End code switch */
      if (*udata == ',') ++udata;   /* Skip over comma */
      } /* End parsing loop */
   dfltrsel(pss, islash, iv);    /* Insert default selectors */

/* Set default batch number if not entered and check for error */

   if (!pss->nbatch) pss->nbatch = pss->nsts;
   if (pss->kbatch && pss->nbatch > pss->nsts)
      nmMexErr("norbmex 610", "Batch number exceeds specs.");

/* Determine fraction of unit probability not assigned by user.
*  Divide this by number of selection blocks that did not have
*  a probability assigned to yield the default probability.  */

   if (pst->nprob >= pst->nspec) {
      /* Note that sumprob > (MX_PROB+MX_PROBERR) would already
      *  have been detected as an error earlier, so here only
      *  need to check the opposite case.  */
      if (sumprob < (MX_PROB-MX_PROBERR))
         nmMexErr("norbmex 611", "Total probabilities < 1.");
      dfltprob = 0;
      }
   else
      dfltprob = (MX_PROB - sumprob)/(pst->nspec - pst->nprob);

/* Do final checking and setup relating to input selectors */

   sumprob = 0;
   for (pss=pst->pfss; pss; pss=pss->pnss) {

/* Set starting seed and assign cumulative probability */

      pss->seed = pss->seed0;
      if (pss->prob == 0) pss->prob = dfltprob;
      sumprob += pss->prob;
      /* Last block should have cumulative probability 1.0 */
      pss->prob = pss->pnss ? sumprob : MX_PROB;

/* Make note of which data files will be needed according to
*  which object instances have been requested.  */

      pcs = &pss->cs[KV_Inst];
      for (jv=0; jv<=mxvars[KV_Inst]; ++jv) {
         if (pcs->sv[jv] != KILLIMG)
            needidf[reqidf[jv]] = TRUE;
         }

/* Allocate space for an empty eligibility table */

      pss->nstbl = pss->nstr*pss->nsts;
      pss->pstbl = (struct UTVINFO *)
         mxCalloc(pss->nstbl, sizeof(struct UTVINFO));
      if (!pss->pstbl) nmMexErr("norbmex 606",
         "Unable to allocate selection table.");
      mexMakeMemoryPersistent(pss->pstbl);

/* Calculate offsets into eligibility table by variable type */

      nel = 1;
      for (jv=NSVARS-1; jv>=0; --jv) {
         pcs = &pss->cs[pss->iso[jv]];
         pcs->offmult = nel;
         nel *= pcs->nvar;
         }
      } /* End loop over selection blocks */

/* Open data files and check headers */

   for (idf=DF_TRAIN; idf<=DF_TEST; ++idf) {

      struct UTVFILE *pdf;       /* Ptr to data file info */
      int trd;                   /* Temp for read to test eof */

      if (!needidf[idf]) continue;

      strcpy(pst->bfname+pst->lbnm+1, ndsfnm[idf]);
      strcpy(pst->fname, pst->bfname);
      strcat(pst->fname, "-dat.mat");

      pdf = pst->ndf + idf;
      pdf->fdimg = open(pst->fname, O_RDONLY);
      if (pdf->fdimg < 0) {
         mexWarnMsgTxt(strerror(errno));
         nmMexErr2("norbmex 615", "Unable to open", pst->fname);
         }
      ldat = sizeof(struct NORBHDR);
      prdd = (char *)&nhdr;
      while (ldat > 0) {
         int lr = (int)read(pdf->fdimg, prdd, ldat);
         if (lr <= 0) {
            mexWarnMsgTxt(strerror(errno));
            nmMexErr2("norbmex 616", "Read error on", pst->fname);
            }
         prdd += lr, ldat -= lr;
         }
#if BYTE_ORDRE > 0
      norbswp4(&nhdr.magic);
      norbswp4(&nhdr.ndim);
      norbswp4(&nhdr.dim[0]);
      norbswp4(&nhdr.dim[1]);
      norbswp4(&nhdr.dim[2]);
      norbswp4(&nhdr.dim[3]);
#endif
      if (nhdr.magic != DAT_MAGIC || nhdr.ndim  != 4 ||
            nhdr.dim[0] != MX_IMGS || nhdr.dim[1] != 2 ||
            nhdr.dim[2] != MX_NIXY || nhdr.dim[3] != MX_NIXY)
         nmMexErr2("norbmex 617", "Bad header on", pst->fname);

      /* Set up variables needed for reading subimages.
      *  These images are known to be grayscale, so pixel size
      *  is implicitly assumed to be 1 byte.  Now that image is
      *  being transposed, earlier code to read whole image at
      *  once has been removed.  */
      pdf->limg = nhdr.dim[2] * nhdr.dim[3];
      pdf->lskp1 = pst->tvyo*nhdr.dim[2] + pst->tvxo;
      pdf->lskp2 = nhdr.dim[2] - pst->tvx;
      /* Space for making transpose */
      if (!pdf->ptrdl) {      /* JIC */
         pdf->ptrdl = (PIXEL *)mxMalloc((mwSize)pst->tvx);
         mexMakeMemoryPersistent(pdf->ptrdl);
         }

   /* Open category file and check header */

      strcpy(pst->fname, pst->bfname);
      strcat(pst->fname, "-cat.mat");
      pdf->fdcat = open(pst->fname, O_RDONLY);
      if (pdf->fdcat < 0) {
         mexWarnMsgTxt(strerror(errno));
         nmMexErr2("norbmex 615", "Unable to open", pst->fname);
         }
      ldat = sizeof(struct NORBHDR) - sizeof(ui32);
      prdd = (char *)&nhdr;
      while (ldat > 0) {
         int lr = (int)read(pdf->fdcat, prdd, ldat);
         if (lr <= 0) {
            mexWarnMsgTxt(strerror(errno));
            nmMexErr2("norbmex 616", "Read error on", pst->fname);
            }
         prdd += lr, ldat -= lr;
         }
#if BYTE_ORDRE > 0
      norbswp4(&nhdr.magic);
      norbswp4(&nhdr.ndim);
      norbswp4(&nhdr.dim[0]);
      norbswp4(&nhdr.dim[1]);
      norbswp4(&nhdr.dim[2]);
#endif
      if (nhdr.magic != CAT_MAGIC || nhdr.ndim  != 1 ||
            nhdr.dim[0] != MX_IMGS || nhdr.dim[1] != 1 ||
            nhdr.dim[2] != 1)
         nmMexErr2("norbmex 617", "Bad header on", pst->fname);

/* Open info file and check header */

      strcpy(pst->fname, pst->bfname);
      strcat(pst->fname, "-info.mat");
      pdf->fdinf = open(pst->fname, O_RDONLY);
      if (pdf->fdinf < 0) {
         mexWarnMsgTxt(strerror(errno));
         nmMexErr2("norbmex 615", "Unable to open", pst->fname);
         }
      ldat = sizeof(struct NORBHDR) - sizeof(ui32);
      prdd = (char *)&nhdr;
      while (ldat > 0) {
         int lr = (int)read(pdf->fdinf, prdd, ldat);
         if (lr <= 0) {
            mexWarnMsgTxt(strerror(errno));
            nmMexErr2("norbmex 616", "Read error on", pst->fname);
            }
         prdd += lr, ldat -= lr;
         }
#if BYTE_ORDRE > 0
      norbswp4(&nhdr.magic);
      norbswp4(&nhdr.ndim);
      norbswp4(&nhdr.dim[0]);
      norbswp4(&nhdr.dim[1]);
      norbswp4(&nhdr.dim[2]);
#endif
      if (nhdr.magic != CAT_MAGIC || nhdr.ndim  != 2 ||
            nhdr.dim[0] != MX_IMGS || nhdr.dim[1] != 4 ||
            nhdr.dim[2] != 1)
         nmMexErr2("norbmex 617", "Bad header on", pst->fname);

/* Now read through the entire cat and info files and for each image
*  that matches one of the specified selectors in any selector block,
*  just enter its number into the table for that block.  Then
*  norbmexget can use a random number to select a table and an entry
*  in this table and read that image.  This way, there is no need to
*  check selection each time a udev is generated.  */

      for (nel=0; nel<MX_IMGS; ++nel) {
         struct {                /* Current image info */
            int   iinst;            /* Instance */
            int   ielev;            /* Elevation */
            int   iazim;            /* Azimuth */
            int   light;            /* Light */
            } iinf;
         int  icat;              /* Current image category */
         int  iaz;               /* Category selector being tested */

         ldat = sizeof(int);     /* Read category from cat file */
         prdd = (char *)&icat;
         while (ldat > 0) {
            int lr = (int)read(pdf->fdcat, prdd, ldat);
            if (lr <= 0) {
               mexWarnMsgTxt(strerror(errno));
               nmMexErr("norbmex 616", "Read error on cat file");
               }
            prdd += lr, ldat -= lr;
            }
#if BYTE_ORDRE > 0
         norbswp4(&icat);
#endif

         ldat = sizeof(iinf);    /* Read other info from info file */
         prdd = (char *)&iinf;
         while (ldat > 0) {
            int lr = (int)read(pdf->fdinf, prdd, ldat);
            if (lr <= 0) {
               mexWarnMsgTxt(strerror(errno));
               nmMexErr("norbmex 616", "Read error on info file");
               }
            prdd += lr, ldat -= lr;
            }
#if BYTE_ORDRE > 0
         norbswp4(&iinf.iinst);
         norbswp4(&iinf.ielev);
         norbswp4(&iinf.iazim);
         norbswp4(&iinf.light);
#endif

/* Sanity checks */

         if (iinf.light < 0 || iinf.light > MX_LGT) nmMexErr2(
            "norbmex 618", "Info file has bad", vnms[KV_Light]);
         if (icat < 0 || icat > MX_CAT)
            nmMexErr("norbmex 618", "Bad category in cat file");
         if (iinf.iinst < 0 || iinf.iinst > MX_INST) nmMexErr2(
            "norbmex 618", "Info file has bad", vnms[KV_Inst]);
         if (iinf.ielev < 0 || iinf.ielev > MX_ELV) nmMexErr2(
            "norbmex 618", "Info file has bad", vnms[KV_Elev]);
         if (iinf.iazim < 0 || iinf.iazim & 1 ||
               iinf.iazim > 2*MX_AZI) nmMexErr2(
            "norbmex 618", "Info file has bad", vnms[KV_Azim]);
         iaz = iinf.iazim >> 1;

/* Check against stored selectors */

         for (pss=pst->pfss; pss; pss=pss->pnss) {
            ui32 iel,jel;              /* Location in pstbl */

            iel = 0;
            pcs = &pss->cs[KV_Azim];   /* Check azimuth */
            jel = pcs->sv[iaz];
            if (jel == KILLIMG) continue;
            iel += jel * pcs->offmult;

            pcs = &pss->cs[KV_Elev];   /* Check elevation */
            jel = pcs->sv[iinf.ielev];
            if (jel == KILLIMG) continue;
            iel += jel * pcs->offmult;

            pcs = &pss->cs[KV_Inst];   /* Check instance */
            jel = pcs->sv[iinf.iinst];
            if (jel == KILLIMG) continue;
            iel += jel * pcs->offmult;

            pcs = &pss->cs[KV_Cat];    /* Check category */
            jel = pcs->sv[icat];
            if (jel == KILLIMG) continue;
            iel += jel * pcs->offmult;

            pcs = &pss->cs[KV_Light];  /* Check light */
            jel = pcs->sv[iinf.light];
            if (jel == KILLIMG) continue;
            iel += jel * pcs->offmult;

/* Found an acceptable image -- store indexes (except camera)
*  in table */

            /* Just-in-case check that should never trigger */
            if (iel >= pss->nstbl) nmMexErr("norbmex 619",
               "Too many images match the given selectors");

            pcs = &pss->cs[KV_View];
            for (jv=0; jv<=MX_CAM; ++jv) {
               struct UTVINFO *pimi;   /* Ptr to current image info */
               jel = pcs->sv[jv];
               if (jel == KILLIMG) continue;
               pimi = pss->pstbl + iel + jel*pcs->offmult;
               pimi->imdf = idf;
               pimi->imgno = nel + nel + jv;
               pimi->isid[KV_Cat]  = (byte)icat;
               pimi->isid[KV_Inst] = (byte)iinf.iinst;
               pimi->isid[KV_Azim] = (byte)iaz;
               pimi->isid[KV_Elev] = (byte)iinf.ielev;
               pimi->isid[KV_Light]= (byte)iinf.light;
               pimi->isid[KV_View] = (byte)jv;
               }

            } /* End pss loop */
         } /* End nel loop */

/* Now just check that the two files reached end-of-file */

      if (read(pdf->fdcat, &trd, 1) || read(pdf->fdinf, &trd, 1))
         nmMexErr("norbmex 620",
            "Inf & Cat files did not end together");
      if (close(pdf->fdcat) < 0)
         nmMexErr("norbmex 621", "Error closing cat file");
      pdf->fdcat = 0;   /* JIC */
      if (close(pdf->fdinf) < 0)
         nmMexErr("norbmex 621", "Error closing inf file");
      pdf->fdinf = 0;   /* JIC */

      } /* End loop over the two database sections */

/* All's well--return */

   return;

   } /* End norbmexset() */
