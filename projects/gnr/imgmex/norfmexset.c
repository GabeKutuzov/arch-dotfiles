/* (c) Copyright 2011, The Rockefeller University *11113* */
/* $Id: norfmexset.c 1 2012-01-13 20:05:02Z  $ */
/***********************************************************************
*                             norfmexset                               *
************************************************************************
*  This is the setup routine for a Matlab mex-function package to read *
*  from a prestored file the responses of MIT 'C2' features to images  *
*  from the "small NORB dataset" by Fu Jie Huang and Yann LeCun of the *
*  Courant Institute, New York University.  See: Y. LeCun, F.J. Huang, *
*  and L. Bottou, Learning methods for generic object recognition with *
*  invariance to pose and lighting, IEEE Computer Society Conference   *
*  on Computer Vision and Pattern Recognition (CVPR), 2004.            *
*                                                                      *
*  The mex-function package is based on norbmex{get|set}.c, the parent *
*  NORB-image-reading mex file for the raw images also written by GNR. *
*                                                                      *
*  This is the ultimate NORB image reader, designed to select objects  *
*  at random based on some identification criteria, and then to cycle  *
*  systematically or at random through all or some images that meet    *
*  other criteria.  This may be useful for training runs in which e.g. *
*  different rotations of an object should be viewed sequentially, but *
*  the objects should be otherwise random.  This version also allows   *
*  the user to specify more than one set of selection criteria, each   *
*  to be used with some given probability, and to allow changing the   *
*  selection criteria within a single run.                             *
*                                                                      *
*  Multiple functions are performed by norfmexset according to the     *
*  arguments received.  For convenience, documentation for this and    *
*  all functions contained in other source files is contained here.    *
*----------------------------------------------------------------------*
*  Initialization call:                                                *
*     NorfMexComm = norfmexset(ufile, nfeat, udata)                    *
*  Revise sequence call (make only after init call succeeds):          *
*     norfmexset(NorfMexComm, udata)                                   *
*  Reset call (make only after init call succeeds):                    *
*     norfmexset(NorfMexComm, 'reset')                                 *
*  Closeout call (make only after init call succeeds):                 *
*     norfmexset(NorfMexComm)                                          *
*                                                                      *
*  Actions:  The initialization call must be made first.  It prepares  *
*     the package for response acquisition according to a specified    *
*     sequence of random and systematic selections.  This sequence can *
*     be changed later by calling norfmexset with the two arguments    *
*     'NorfMexComm' and a new 'udata'.  If 'udata' is literally the    *
*     word 'reset' (any capitalization), the image sequence is not     *
*     changed but is restarted from the beginning.  In the present     *
*     design, this amounts to resetting the random number seed and     *
*     initial state of each systematic selection loop.  All files      *
*     are closed and all allocated memory is released by a call with   *
*     just the one argument 'NorfMexComm'.                             *
*                                                                      *
*  Arguments:                                                          *
*     ufile    A Matlab string giving the full path to the prestored   *
*              image response file.  The program assumes that the      *
*              order of the response record in this file is the same   *
*              as that specified in the norfmex.h header file.         *
*     nfeat    Number of feature responses recorded for each image.    *
*     udata    Matlab string giving the image data selectors, cate-    *
*              gorization mode, and random number seed.  This will     *
*              generally be derived from user input.                   *
*     NorfMexComm    See below.  Once the initial norfmexset call has  *
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
*     P value.  On each norfmexget call that is not part of a selec-   *
*     tion sequence, one of the specs is chosen at random with the     *
*     given probability, and a stimulus is then chosen according to    *
*     that spec.  Any specified cycle is then completed on subsequent  *
*     norfmexget calls.  A new selector spec is chosen on the next     *
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
*        Note that although the NORB dataset contains stereo pairs     *
*     made with two cameras, only the images from camera 0 were used   *
*     to generate the feature response file.  A V(1) spec will yield   *
*     an error message--a new feature response file will need to be    *
*     made if camera 1 data are required.                              *
*                                                                      *
*        A batch number code is the letter 'N' followed by an integer  *
*     in parentheses.                                                  *
*                                                                      *
*        A probability code is the letter 'P' followed by a fraction   *
*     between 0 and 1 in parentheses.  As explained above, any selec-  *
*     tor specs that are not assigned a probability will receive as    *
*     default an equal share of the difference between 1.0 and the     *
*     sum of the probabilites that are assigned to other specs.        *
*                                                                      *
*        A random number seed code is the letter 'S' followed by a     *
*     value for the random number seed (0 < seed < 2**31-1) in         *
*     parentheses.                                                     *
*                                                                      *
*  Return value:  The setup call returns a Matlab array 'NorfMexComm'. *
*     This array contains information that must remain available for   *
*     access by the other routines in the package.  Its contents are   *
*     of no concern to the Matlab user and should never be modified.   *
*     NorfMexComm may be cleared by a call to NorfMexClr().            *
*                                                                      *
*  Error conditions:  The arguments are screened for possible errors.  *
*     All error conditions lead to a call to mexErrMsgTxt, which will  *
*     display an error message and numeric code and terminate the      *
*     calling script.                                                  *
*----------------------------------------------------------------------*
*  Image acquisition call (make only after init call succeeds):        *
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
*----------------------------------------------------------------------*
*  Design notes:  The NorfMexComm data are passed explicitly between   *
*  the various functions rather than being placed in global because    *
*  Matlab insists on copying global data if you want to change them.   *
************************************************************************
*  V1A, 04/15/11, GNR - New program, based on norbmexset.c             *
*  V1B, 04/19/11, GNR - Add byte swapping                              *
*  V1C, 05/02/11, GNR - Add '+' and 'N' codes                          *
*  V2A, 12/02/11, GNR - Return 16-bit isids                            *
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
#include "norfmex.h"

/*=====================================================================*
*                         SOURCE-FILE GLOBALS                          *
*=====================================================================*/

/* Location of common data array */
static mxArray *pNorfMexComm;

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
*     pst      Ptr to master NFMSTATE struct                           *
*=====================================================================*/

static void freespec(struct NFMSTATE *pst) {

   struct NFMSPEC *pss,*pnxtss;

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
*     pst      Ptr to master NFMSTATE struct                           *
*=====================================================================*/

static void closeall(struct NFMSTATE *pst) {

   if (pst->fdff > 0) {
      close(pst->fdff);
      pst->fdff = 0;    /* Prevent doing this twice */
      }

   } /* End closeall() */


/*=====================================================================*
*                              nmFreeAll                               *
*                                                                      *
*  This routine closes all files and frees all storage.  It should be  *
*  used as the registered mexAtExit routine and is also called before  *
*  error exits once any permanent storage has been allocated.  It has  *
*  to find the NFMSTATE struct via a static pointer, as the mexAtExit  *
*  routine can have no arguments.  It does not attempt mxDestroyArray  *
*  because it has read only access to NorfMexComm--the MATLAB automatic*
*  disposal mechanism should work on NorfMexComm once the child allo-  *
*  cations have been freed by this routine.  The sigck should prevent  *
*  disaster if called with a stale pointer after NorfMexComm already   *
*  destroyed.                                                          *
*=====================================================================*/

static void nmFreeAll(void) {

   if (pNorfMexComm) {
      struct NFMSTATE *pst = mxGetData(pNorfMexComm);
      if (pst->sigck == NM_SIGCK) {
         closeall(pst);
         freespec(pst);
         if (pst->fname) mxFree(pst->fname);
         }
      pNorfMexComm = NULL;
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
*     pcs            Ptr to NFMSEL where default selectors will be     *
*                    removed, rank order of selectors found in the     *
*                    input will be stored, and number of selectors     *
*                    stored in nvar.                                   *
*                    (Input order will be preserved).                  *
*     ikv            Type of index variable being parsed (from KV set) *
* Return value:  Number of selectors found.                            *
*=====================================================================*/

static int norbpsel(char **ppsel, struct NFMSEL *pcs, int ikv) {

   char *ps = *ppsel;         /* Ptr used to scan the input */
   byte  *svals = pcs->sv;    /* Locate results array */
   int   ib,ie;               /* Beginning, end of a range */
   int   rc;                  /* Return code */
   int   smax;                /* Max value of this selector type */

   if (*ps++ != '(') nmMexErr2("norfmex 607",
      "Missing '(' in selector for", vnms[ikv]);
   if (pcs->nvar > 0) nmMexErr2("norfmex 608",
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
   nmMexErr2("norfmex 609",
      "Malformed selector spec for", vnms[ikv]);
   } /* End norbpsel() */


/*=====================================================================*
*                              makespec                                *
*                                                                      *
*  Create an empty selector spec and initialize it.  The linked list   *
*  is constructed LIFO since it is simpler and the order doesn't       *
*  really matter.                                                      *
*                                                                      *
*  Argument:                                                           *
*     pst      Ptr to master NFMSTATE struct                           *
*=====================================================================*/

static struct NFMSPEC *makespec(struct NFMSTATE *pst) {

   struct NFMSPEC *pss =
      (struct NFMSPEC *)mxCalloc(1, sizeof(struct NFMSPEC));
   if (!pss) nmMexErr("norfmex 606", "No space for selector spec.");
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
*     pss         Ptr to NFMSPEC to be updated                         *
*     islash      Position in selector list where slash found or       *
*                 -1 if no slash was found.                            *
*     lastiv      Number of selectors entered in this block            *
*                 (value of iv at end of block).                       *
*=====================================================================*/

static void dfltrsel(struct NFMSPEC *pss, int islash, int lastiv) {

   int jj,jv;

   if (islash < 0) islash = lastiv;
   for (jv=0; jv<NSVARS; ++jv) {
      struct NFMSEL *pcs = &pss->cs[jv];
      if (pcs->nvar == 0) {
         if (lastiv >= NSVARS) nmMexErr("norfmex 608",
            "More selectors than variables.");
         for (jj=lastiv++; jj>islash; --jj)
            pss->iso[jj] = pss->iso[jj-1];
         pss->iso[islash++] = jv;
         for (jj=0; jj<=mxvars[jv]; ++jj)
            pcs->sv[jj] = jj;
         pss->nstr *= (pcs->nvar = mxvars[jv]);
         }
      } /* End variable type loop */

   if (pss->nstr*pss->nsts > TOT_IMGS) nmMexErr("norfmex 610",
      "More selections than images in dataset.");
   } /* End dfltrsel() */


/*=====================================================================*
*                              norfmexset                                *
*=====================================================================*/

void mexFunction(int nlhs, mxArray *plhs[],
      int nrhs, const mxArray *prhs[]) {

   struct NFMSTATE *pst;      /* Ptr to persistent local data */
   struct NFMSPEC *pss;       /* Ptr to current selector spec */
   struct NFMSEL *pcs;        /* Ptr to selector info */
   char   *udata;             /* Ptr to udata argument string */
   ui32 nel;                  /* Number of eligible images */
   ui32 dfltprob;             /* Default probability */
   ui32 sumprob;              /* Sum of probabilities */
   int  islash;               /* Position where slash code found */
   int  iv,jv;                /* Variable loop indexes */
   int  ldat;                 /* Length of data being read */
   int  lfnm;                 /* Length of file name */

   /* Clear pointers to avoid mxFree() errors */
   pst = NULL; udata = NULL;

/* Standard MATLAB argument checking (except init call) */

   if (nrhs > 0 && nrhs < 3) {
      if (nlhs != 0) nmMexErr("norfmex 627",
         "Function must have no return value.");
      /* Access NorfMexComm data */
      if (!mxIsClass(prhs[0],"uint8")) nmMexErr("norfmex 602",
         "1st arg must be NorfMexComm.");
      pst = (struct NFMSTATE *)mxGetData(prhs[0]);
      if (!pst || pst->sigck != NM_SIGCK) nmMexErr("norfmex 602",
         "Bad NorfMexComm arg.");
      }

/* Determine type of call from number of arguments */

   switch (nrhs) {

/* Close-out call */

case 1:
   nmFreeAll();
   return;

/* Reset or revised udata call */

case 2:
   if (!mxIsChar(prhs[1])) nmMexErr("norfmex 603",
      "udata arg must be a string.");
   ldat = mxGetNumberOfElements(prhs[1]);
   if (ldat <= 0 || !(udata = mxMalloc(ldat+1)) ||
         mxGetString(prhs[1], udata, ldat+1))
      nmMexErr2("norfmex 604", "Unable to copy", "udata");
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

case 3:
   if (nlhs != 1) nmMexErr("norfmex 627",
      "Function must have one return value.");
   if (!mxIsChar(prhs[0]) || !mxIsDouble(prhs[1]) || !mxIsChar(prhs[2]))
      nmMexErr("norfmex 603", "Argument(s) not of correct type(s).");
   if (mxGetNumberOfElements(prhs[1]) != 1)
      nmMexErr("norfmex 603", "Argument(s) not of correct length(s).");

/* Create the NorfMexComm common data area and initialize.
*  Note:  mxCreateNumericMatrix terminates script on failure.  */

   pNorfMexComm = plhs[0] =
      mxCreateNumericMatrix(sizeof(struct NFMSTATE), 1,
      mxUINT8_CLASS, mxREAL);
   pst = (struct NFMSTATE *)mxGetData(pNorfMexComm);
   pst->sigck = NM_SIGCK;
   pst->dfltseed = DFLT_SEED;

/* From here on, there may be stuff to clean up on exit */

   mexAtExit(nmFreeAll);

/* Read and save the arguments */

   /* Make space for the file name */
   lfnm = mxGetNumberOfElements(prhs[0]);
   if (lfnm <= 0 || !(pst->fname = mxMalloc(lfnm+1)) ||
         mxGetString(prhs[0], pst->fname, lfnm+1))
      nmMexErr2("norfmex 604", "Unable to copy", "fname");
   mexMakeMemoryPersistent(pst->fname);

   {  double tdnfeat = *mxGetPr(prhs[1]);
      if (tdnfeat <= 0 || tdnfeat > MX_FEATS)
         nmMexErr("norfmex 605", "nfeat negative or > 4075.");
      pst->nfeat = (ui32)tdnfeat;
      } /* End tdnfeat local scope */

   /* Check amd extract udata arg */
   ldat = mxGetNumberOfElements(prhs[2]);
   if (ldat <= 0 || !(udata = mxMalloc(ldat+1)) ||
         mxGetString(prhs[2], udata, ldat+1))
      nmMexErr2("norfmex 604", "Unable to copy", "udata");
   break;

default:
   nmMexErr("norfmex 627", "Wrong number of arguments.");

   } /* End nrhs switch */

/* Items that need to be cleared on INIT or RESTART call */
sumprob = 0;

/* Start of a new spec (first or semicolon) will enter here.
*  Create a selector spec and initialize it.  */

StartSpecHere:
   pss = makespec(pst);

/* Parse the selectors */

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
         if (*udata++ != '(') nmMexErr("norfmex 607",
            "Missing '(' in batch number.");
         while (*udata && *udata != ')') {
            int md = (int)(*udata++);
            if (isdigit(md))
               tbatch = 10*tbatch + (md - '0');
            else nmMexErr("norfmex 612", "Nondigit in batch number.");
            }
         if (tbatch <= 0 || tbatch == SI32_MAX)
            nmMexErr("norfmex 612", "Invalid batch number.");
         pss->nbatch = tbatch;
         udata++;
         } /* End 'N' local scope */
         break;
      case 'P': {                /* Probability */
         ui32 tprob = 0, tdec = 1;
         int  ndig = 0;
         if (*udata++ != '(') nmMexErr("norfmex 607",
            "Missing '(' in P code.");
         if (pss->prob == 0) pst->nprob += 1;
         while (*udata && *udata != ')') {
            int md = (int)(*udata++);
            if (md == '.') tdec = 1;
            else if (isdigit(md)) {
               tprob = 10*tprob + (md - '0');
               tdec *= 10;
               ndig += 1; }
            else nmMexErr("norfmex 609",
               "Illegal character in P code.");
            }
         if (ndig >= UI32_SIZE) nmMexErr("norfmex 609",
            "Too many digits in P code.");
         /* Because we don't want to load ROCKS routines here,
         *  we use 64-bit if available, otherwise double float */
#ifdef HAS_I64
         {  ui64 tnum = ((ui64)tprob << 31)/(ui64)tdec;
            if (tnum > MX_PROB) nmMexErr("norfmex 611",
               "Probability > 1.");
            pss->prob = (ui32)tnum;
            } /* End tnum local scope */
#else
         {  double dmxp = (double)MX_PROB;
            double tnum = dmxp*(double)tprob/(double)tdec;
            if (tnum > dmxp) nmMexErr("norfmex 611",
               "Probability > 1.");
            pss->prob = (ui32)tnum;
            } /* End tnum local scope */
#endif
         sumprob += pss->prob;
         if (sumprob > (MX_PROB+MX_PROBERR))
            nmMexErr("norfmex 611", "Total probabilities > 1.");
         udata++;
         } /* End 'P' local scope */
         break;
      case 'S': {                /* Random number seed */
         si32 tseed = 0;
         if (*udata++ != '(') nmMexErr("norfmex 607",
            "Missing '(' in seed.");
         while (*udata && *udata != ')') {
            int md = (int)(*udata++);
            if (isdigit(md))
               tseed = 10*tseed + (md - '0');
            else nmMexErr("norfmex 612", "Nondigit in seed.");
            }
         if (tseed <= 0 || tseed == SI32_MAX)
            nmMexErr("norfmex 612", "Invalid seed.");
         pss->seed0 = tseed;
         udata++;
         } /* End 'S' local scope */
         break;
      case '/':                  /* Systematic/random separator */
         pss->kbatch = TRUE;
         /* Drop through to '+' case ... */
      case '+':                  /* Systematic batch separator */
         if (islash >= 0) nmMexErr("norfmex 613",
            "More than one plus or slash in selector string.");
         islash = iv;
         break;
      case ';':                  /* New selection block */
         dfltrsel(pss, islash, iv);
         goto StartSpecHere;
      default:                   /* None of the above: error */
         nmMexErr("norfmex 614", "Unrecognized selector code.");
         } /* End code switch */
      if (*udata == ',') ++udata;   /* Skip over comma */
      } /* End parsing loop */
   dfltrsel(pss, islash, iv);    /* Insert default selectors */

/* Set default batch number if not entered and check for error */

   if (!pss->nbatch) pss->nbatch = pss->nsts;
   if (pss->kbatch && pss->nbatch > pss->nsts)
      nmMexErr("norfmex 610", "Batch number exceeds specs.");

/* Determine fraction of unit probability not assigned by user.
*  Divide this by number of selection blocks that did not have
*  a probability assigned to yield the default probability.  */

   if (pst->nprob >= pst->nspec) {
      /* Note that sumprob > (MX_PROB+MX_PROBERR) would already
      *  have been detected as an error earlier, so here only
      *  need to check the opposite case.  */
      if (sumprob < (MX_PROB-MX_PROBERR))
         nmMexErr("norfmex 611", "Total probabilities < 1.");
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

/* Allocate space for an empty eligibility table */

      pss->nstbl = pss->nstr*pss->nsts;
      pss->pstbl = (struct NFMINFO *)
         mxCalloc(pss->nstbl, sizeof(struct NFMINFO));
      if (!pss->pstbl) nmMexErr("norfmex 629",
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

/* Now, where norbmexset read through the entire cat and info files,
*  we instead loop over all the possible images and for each image
*  that matches one of the specified selectors in any selector block,
*  we enter its number into the table for that block.  Then norfmexget
*  can use a random number to select a table and an entry in this
*  table and read that image.  This way, there is no need to check
*  selection each time a udev is generated.
*
*  N.B.  The order of index variables and numbers of each in the
*  stored response file are assumed here.  This code must be revised
*  if a new stored response file is made with different contents.  */

   for (pss=pst->pfss; pss; pss=pss->pnss) {
      struct NFMINFO *pfri;      /* Ptr to this feat response info */
      /* The oxxx values are offsets into the pstbls.
      *  The rxxx values are record numbers in the response file.  */
      ui32 icat,ocat,rcat;       /* Category  index and offsets */
      ui32 iinst,oinst,rinst;    /* Instance  index and offsets */
      ui32 iazim,oazim,razim;    /* Azimuth   index and offsets */
      ui32 ielev,oelev,relev;    /* Elevation index and offsets */
      ui32 light,olght,rlght;    /* Lighting  index and offsets */
      byte jel;                  /* Current selector */

      /* Multipliers to be applied to isid values to find the
      *  record in the response file (in enum KVAR order) */
      static ui32 ismul[NSVARS] = { 9720, 972, 1, 18, 162, 0 };

/* Check against stored selectors.  There is no camera selector check
*  because only data for camera 0 were recorded in the response file. */

      for (icat=0; icat<=MX_CAT; ++icat) {
         pcs = &pss->cs[KV_Cat];                /* Check category */
         jel = pcs->sv[icat];
         if (jel == KILLIMG) continue;
         ocat = (ui32)jel * pcs->offmult;
         rcat = icat * ismul[KV_Cat];

         for (iinst=0; iinst<=MX_INST; ++iinst) {
            pcs = &pss->cs[KV_Inst];            /* Check instance */
            jel = pcs->sv[iinst];
            if (jel == KILLIMG) continue;
            oinst = (ui32)jel * pcs->offmult + ocat;
            rinst = iinst * ismul[KV_Inst] + rcat;

            for (iazim=0; iazim<=MX_AZI; ++iazim) {
               pcs = &pss->cs[KV_Azim];         /* Check azimuth */
               jel = pcs->sv[iazim];
               if (jel == KILLIMG) continue;
               oazim = (ui32)jel * pcs->offmult + oinst;
               razim = iazim * ismul[KV_Azim] + rinst;

               for (ielev=0; ielev<=MX_ELV; ++ielev) {
                  pcs = &pss->cs[KV_Elev];      /* Check elevation */
                  jel = pcs->sv[ielev];
                  if (jel == KILLIMG) continue;
                  oelev = (ui32)jel * pcs->offmult + oazim;
                  relev = ielev * ismul[KV_Elev] + razim;

                  for (light=0; light<=MX_LGT; ++light) {
                     pcs = &pss->cs[KV_Light];  /* Check light */
                     jel = pcs->sv[light];
                     if (jel == KILLIMG) continue;
                     olght = (ui32)jel * pcs->offmult + oelev;
                     rlght = light * ismul[KV_Light] + relev;

/* Found an acceptable image --
*  store record number and indexes in pstbl */

                     /* Just-in-case check that should never trigger */
                     if (olght >= pss->nstbl) nmMexErr("norfmex 619",
                        "Too many images match the given selectors");
                     pfri = pss->pstbl + olght;
                     pfri->imgno = rlght;
                     pfri->isid[KV_Cat]  = (byte)icat;
                     pfri->isid[KV_Inst] = (byte)iinst;
                     pfri->isid[KV_Azim] = (byte)iazim;
                     pfri->isid[KV_Elev] = (byte)ielev;
                     pfri->isid[KV_Light]= (byte)light;
                     pfri->isid[KV_View] = 0;

                     } /* End lighting loop */
                  } /* End elevation loop */
               } /* End azimuth loop */
            } /* End instance loop */
         } /* End cat loop */
      } /* End pss loop */

/* Open data file */

   pst->fdff = open(pst->fname, O_RDONLY);
   if (pst->fdff < 0) {
      mexWarnMsgTxt(strerror(errno));
      nmMexErr2("norbmex 615", "Unable to open", pst->fname);
      }

/* All's well--return */

   return;

   } /* End norfmexset() */
