/* (c) Copyright 2011-2012, The Rockefeller University *11113* */
/* $Id: citmexset.c 9 2012-11-21 22:00:13Z  $ */
/***********************************************************************
*                             citmexset                                *
************************************************************************
*  This is the setup routine for a Matlab mex-function package to read *
*  images from the "Caltech 101" dataset by Fei-Fei Li, Marco Andre-   *
*  etto, Marc 'Aurelio Ranzato, and Pietro Perona.  See: L. Fei-Fei,   *
*  R. Fergus, and P. Perona.  "Learning generative visual models from  *
*  few training examples:  An incremental Bayesian approach tested on  *
*  101 object categories."  IEEE CVPR 2004 Workshop on Generative-     *
*  Model Based Vision (2004).                                          *
*                                                                      *
*  This is the ultimate CIT image reader, designed to select objects   *
*  at random based on some identification criteria, and then to cycle  *
*  systematically or at random through all or some images that meet    *
*  other criteria.  This version also allows the user to specify more  *
*  than one set of selection criteria, each to be used with some given *
*  probability, and to allow changing the selection criteria within a  *
*  single run.   This is designed to be as similar as possible to the  *
*  setup for NORB images, even though the Caltech images do not have   *
*  multiple az-el, lighting, etc. and are all different sizes.         *
*                                                                      *
*  V2 adds the ability to read also the annotation file for each image *
*  and (1) crop the image to the specified containing box, and/or (2)  *
*  set all densities outside the specified contour to 0.               *
*                                                                      *
*  V3 adds the ability to add a border around the cropping boundaries  *
*  specified in the annotation data, and the ability to defer back-    *
*  ground painting to a separate call (so the caller can perform       *
*  filtering operations on the image before background painting).      *
*                                                                      *
*  Multiple functions are performed by citmexset according to the      *
*  arguments received.  For convenience, documentation for this and    *
*  all functions contained in other source files is contained here.    *
*----------------------------------------------------------------------*
*  Initialization call:                                                *
*     CitMexComm = citmexset(imgdir, mxix, mxiy, udata, kops)          *
*  Revise sequence call (make only after init call succeeds):          *
*     citmexset(CitMexComm, udata)                                     *
*  Reset call (make only after init call succeeds):                    *
*     citmexset(CitMexComm, 'reset')                                   *
*  Closeout call (make only after init call succeeds):                 *
*     citmexset(CitMexComm, 'close')                                   *
*                                                                      *
*  Actions:  The initialization call must be made first.  It prepares  *
*     the package for image acquisition according to a specified se-   *
*     quence of random and systematic selections.  This sequence can   *
*     be changed later by calling citmexset with the two arguments     *
*     'CitMexComm' and a new 'udata'.  If 'udata' is literally the     *
*     word 'reset' (any capitalization), the image sequence is not     *
*     changed but is restarted from the beginning.  In the present     *
*     design, this amounts to resetting the random number seed and     *
*     initial state of each systematic selection loop.  All files      *
*     are closed and all allocated memory is released by a call with   *
*     the two arguments 'CitMexComm' and the literal word 'close'.     *
*                                                                      *
*  Arguments:                                                          *
*     imgdir   A Matlab string giving the full path to the directory   *
*              'CIT101Images' that is created when the Caltech image   *
*              database is downloaded and installed.  The program      *
*              expects to find the directory '101_ObjectCategories'    *
*              under this at all times and the directory 'Annotations' *
*              as well if the cropping/masking options are requested.  *
*              The program expects to find 102 subdirectories in each  *
*              of these, each named for a category.  The category      *
*              directories under '101_ObjectCategories' each contain   *
*              a number of jpg images of objects in that category.     *
*              The BACKGROUND category is made accessible as category  *
*              #102 (out of alphabetical and numerical order) in case  *
*              needed.  The category directories under 'Annotations'   *
*              each contain one Matlab .mat file for each image, num-  *
*              bered the same way, and each containing the coordinates *
*              of a rectangular box surrounding the object of interest *
*              and the coordinates of a piecewise-linear contour line  *
*              that more closely surrounds the object of interest.     *
*     mxix     Maximum size of the image data to be returned along x.  *
*              Since images are of different sizes, larger images will *
*              be truncated and only mxix pixels will be returned,     *
*              centered on the full available image size along x.      *
*              If the x dimension is smaller, the full size will be    *
*              returned.  Make mxix large to get all images in full.   *
*              If mxix is a length-two vector, the second element is   *
*              the size of an extra border to be left around the       *
*              object when kops(2) cropping is performed (default: 0). *
*              The border will be cut if it would exceed mxix pixels.  *
*              If mxix is a length-three vector, the third element is  *
*              the size of an extra border to be left around the obj-  *
*              ect when kops(4|8) painting is performed (default: 0).  *
*     mxiy     Maximum size of the image data to be returned along y.  *
*              The explanation for mxix applies, mutatis mutandis.     *
*     udata    Matlab string giving the image data selectors, cate-    *
*              gorization mode, and random number seed.  This will     *
*              generally be derived from user input.                   *
*     kops     Matlab uint32 integer that is the sum of values from    *
*              the following list specifying optional services that    *
*              are wanted each time citmexget() is called:             *
*                 (1) Return colored images as such (with separate     *
*              red-green-blue values).  If this option is not speci-   *
*              fied, colored images are converted to equivalent 8-bit  *
*              graylevel levels.                                       *
*                 (2) Crop images to contain only the rectangle        *
*              specified in the corresponding 'Annotations' file       *
*              (or smaller rectangle if mxix,mxiy is smaller, larger   *
*              rectangle if mxix(2) or mxiy(2) is > 0).                *
*                 (4) Set all pixels outside (or possibly inside) the  *
*              contour specified in the 'Annotations' file to 0 (see   *
*              notes below regarding specification of drawing mode in  *
*              bits (128)-(16) of kops).                               *
*                 (8) Read and store information from the annotations  *
*              file for background painting.  Actual painting will not *
*              be performed by citmexget(), but rather by a later call *
*              to citmexpbg().  It is an error to have both 4 and 8    *
*              bits set.                                               *
*                 (128,64,32,16) For compatibility with old matlab     *
*              scripts, if background painting is requested by bits    *
*              (4) or (8), and bit (128) is zero, default drawing mode *
*              6 (zero background except for specified border around   *
*              the object) will be used.  If bit (128) is set, bits    *
*              (64),(32), and (16), shifted right to bits (4),(2),(1)  *
*              will be used to set the iterepgf mode bits--see header  *
*              file iterepgf.h for an explanation of these bits.       *
*     CitMexComm    See below.  Once the initial citmexset call has    *
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
*     P value.  On each citmexget call that is not part of a selec-    *
*     tion sequence, one of the specs is chosen at random with the     *
*     given probability, and a stimulus is then chosen according to    *
*     that spec.  Any specified cycle is then completed on subsequent  *
*     citmexget calls.  A new selector spec is chosen on the next      *
*     call after the cycle is completed.                               *
*        A selector spec comprises a category selector list followed   *
*     optionally by an image selector list, a batch size code, a pro-  *
*     bability code, and/or a random number seed code in any order.    *
*     These are separated by commas.  Instead of a comma, there may    *
*     be a plus sign, slash, or asterisk before either selection list. *
*        Only those images will be returned whose properties match     *
*     selector lists.  Selections to the right of a plus sign (or the  *
*     start of the selector spec if there is no punctuation) will be   *
*     selected at random with replacement.  Selections from selectors  *
*     to the right of a slash will be processed in the order listed.   *
*     Selections from selectors to the right of an asterisk will be    *
*     processed in a random permutation of the items listed.  If a     *
*     comma is placed between the two selectors, the same selection    *
*     method will be applied to both category and instance.            *
*        In the next two paragraphs, a range is an integer starting    *
*     value followed by a minus sign and an integer ending value.      *
*        A category selector comprises the letter 'C' followed by a    *
*     list of categories in parentheses and separated by commas.       *
*     Category selectors may be names or integers or ranges.  Cate-    *
*     gory numbers go from 1 to 101 and refer to the categories in     *
*     the dataset in alphabetical order, i.e. category #1 is           *
*     "accordion" and #101 is "yin_yang".                              *
*        Image instance selectors comprise the letter 'I' followed by  *
*     a list of image selectors in parentheses and separated by commas.*
*     Image selectors are just integers or ranges counting from 1 as   *
*     the first image in each category.  Because the number of images  *
*     is not the same in different categories, a '$' may be used to    *
*     indicate the last image in the category.  If a category has      *
*     fewer images than the number specified, all available images     *
*     will be selected.  If an image selection is absent, all images   *
*     in the selected categories will be accepted.                     *
*        A batch size code is the letter 'N' followed by an integer in *
*     parentheses. When the category and image selectors are separated *
*     by other than a comma, the number of image selections before     *
*     picking a new category is the batch size.  If no batch size is   *
*     specified, the number of objects in the image selection list is  *
*     the batch size.  When there is more than one selection spec, the *
*     batch size is also the number of images taken from one selection *
*     spec before picking another selection spec at random.  If there  *
*     is only one selection spec and a comma between the category and  *
*     image selection lists, the batch size is ignored and one image   *
*     is simply returned for each call to citmexget().                 *
*        A probability code is the letter 'P' followed by a fraction   *
*     between 0 and 1 in parentheses.  As explained above, any selec-  *
*     tor specs that are not assigned a probability will receive as    *
*     default an equal share of the difference between 1.0 and the     *
*     sum of the probabilites that are assigned to other specs.        *
*        A random number seed code is the letter 'S' followed by a     *
*     value for the random number seed (0 < seed < 2**31-1) in         *
*     parentheses.                                                     *
*                                                                      *
*        Examples:                                                     *
*     'C(1-101),I(1-$)'                                                *
*        select at random from all images.                             *
*     'C(1-80)*I(1-40),N(10)'                                          *
*        select from categories 1-80.  For each category, present 10   *
*        images selected from among the first 40 in a randomly per-    *
*        muted order (that is the same for each category).             *
*     'C(20-40)/I(1-10),P(0.5);C(60-80)/I(3,5,7,9),P(0.5)'             *
*        With probability 0.5, select a category from 20 to 40 and     *
*        present objects 1-10 from that category on successive         *
*        citmexget calls.  Otherwise, select a category from 60 to     *
*        80 and present object 3,5,7,9 in that order on successive     *
*        citmexget calls before again selecting another category.      *
*     '/C(airplanes,brain,emu,lotus)+I(1-$),N(20)'                     *
*        Cycle through the categories listed.  From each category,     *
*        pick any 20 objects with replacement before moving on to the  *
*        next category.                                                *
*                                                                      *
*  Return value:  The setup call returns a Matlab array 'CitMexComm'.  *
*     This array contains information that must remain available for   *
*     access by the other routines in the package.  Its contents are   *
*     of no concern to the Matlab user and should never be modified.   *
*                                                                      *
*  Error conditions:  The arguments are screened for possible errors.  *
*     All error conditions lead to a call to mexErrMsgTxt, which will  *
*     display an error message and numeric code and terminate the      *
*     calling script.                                                  *
*----------------------------------------------------------------------*
*  Image acquisition call (make only after init call succeeds):        *
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
*----------------------------------------------------------------------*
*  Background painting call (make only after init call with kops = 8   *
*  and image retrieval with citmexget):                                *
*     imgm = citmexpbg(CitMexComm, image)                              *
*                                                                      *
*  Action:  Pixels in the input image (assumed to be one returned by a *
*     previous call to citmexget()) that are outside an object contour *
*     retrieved from the annotation data by the same citmexget() call  *
*     are set to 0.  (More elaborate versions of this operation, for   *
*     example, using local mean background instead of 0, may be imple- *
*     mented at a future date.)                                        *
*                                                                      *
*  Arguments:                                                          *
*     CitMexComm    Data array returned by initial citmexset call.     *
*     image         Image returned by most recent call to citmexget(). *
*                   (Any desired operations, such as edge filters, may *
*                   be performed on the image before this call, but    *
*                   the size may not be changed.                       *
*                                                                      *
*  Return value:                                                       *
*     imgm          Modified image with background removed.            *
*                                                                      *
*  Error conditions:  The script is terminated with an error message   *
*     on all error conditions.                                         *
************************************************************************
*  Important Note:  As distributed, the Annotations directory has some *
*  differences from the 101_ObjectCategories directory.  Specifically, *
*  'airplanes' is replaced by 'Airplanes_Side_2', 'Faces' by 'Faces_2',*
*  'Faces_easy' by 'Faces_3', and 'Motorbikes' by 'Motorbikes_16'.  I  *
*  checked one or two of each set and the annotations appear to match  *
*  the images, so I renamed the 'Annotations' subdirs so this program  *
*  would not need to deal with the differences.  'Faces_easy' appears  *
*  to contain the same images as 'Faces' except cropped closer, so it  *
*  is probably not wise to use both categories in any performance test.*
*  Also, there is a READ_ME file in the 'Annotations' directory which  *
*  states that airplanes and bonsai annotations are not complete, but  *
*  in fact the numbers of files in each directory pair are the same.   *
************************************************************************
*  V1A, 11/15/11, GNR - New program, based on norbmexset.c             *
*  V2A, 12/06/11. GNR - Add cropping and masking options               *
*  ==>, 12/20/11, GNR - Last mod before committing to svn repository   *
*  Rev, 01/19/12, GNR - Better image number error checking             *
*  Rev, 02/02/12, GNR - Add semaphore to prevent multiple free         *
*  Rev, 03/22/12, GNR - Add error if previous memory not cleared first *
*  V3A, 11/08/12, GNR - Add cropping border and deferred painting      *
*  V3B, 11/20/12, GNR - Use iterepgf to add border around image paint  *
***********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <semaphore.h>
#include <errno.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include "mex.h"
#include "sysdef.h"
#include "rksubs.h"
#include "rkarith.h"
#include "citmex.h"

/*=====================================================================*
*                         SOURCE-FILE GLOBALS                          *
*=====================================================================*/

/* Location of common data array */
static mxArray *pCitMexComm;


/*=====================================================================*
*                              freespec                                *
*                                                                      *
*  Free all data relating to image selection.                          *
*  (This might be prior to entering a new selection, i.e. the other    *
*   information in the pst structure is not freed.)                    *
*                                                                      *
*  Argument:                                                           *
*     pst      Ptr to master C101STATE struct                          *
*=====================================================================*/

static void freespec(struct C101STATE *pst) {

   struct C101SPEC *pss,*pnxtss;

   if (!pst) return;
   if (pst->sigck != C101_SIGCK)
      mexErrMsgTxt("citmex 630: Corrupted CitMexComm data area");
   for (pss=pst->pfss; pss; pss=pnxtss) {
      pnxtss = pss->pnss;        /* Save ptr to next while freeing */
      if (pss->u1.pcis) mxFree(pss->u1.pcis);
      if (pss->pcat) mxFree(pss->pcat);
      if (pss->pimg) mxFree(pss->pimg);
      mxFree(pss);
      }
   pst->pfss = NULL;
   pst->pcss = NULL;
   pst->nspec = pst->nprob = pst->ivar = 0;

   } /* End freespec() */


/*=====================================================================*
*                              cmFreeAll                               *
*                                                                      *
*  This routine frees all storage acquired by citmexset.  It should be *
*  used as the registered mexAtExit routine and is also called before  *
*  error exits once any permanent storage has been allocated.  It has  *
*  to find the C101STATE struct via a static pointer, as the mexAtExit *
*  routine can have no arguments.  It does not attempt mxDestroyArray  *
*  because it has read-only access to CitMexComm--the MATLAB automatic *
*  disposal mechanism should work on CitMexComm once the child allo-   *
*  cations have been freed by this routine.  The semaphore is used to  *
*  prevent freeing memory after it has already been freed--apparently, *
*  Matlab replaces the nulled-out pCitMexComm with an earlier copy.    *
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
         freespec(pst);
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
*  (mexErrMsgIdAndTxt() requires the id number to start with a letter, *
*  so it was not used.)                                                *
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
*                              cmssmatch                               *
*                                                                      *
*  This is a version of crk ssmatch() customized for matching category *
*  names.  It differs in that the item to be tested can end with '\0', *
*  comma, or right paren, but the key can end only with '\0'.          *
*                                                                      *
*  Arguments:                                                          *
*     item        Ptr to item to be matched.                           *
*     key         Ptr to key item is to be matched against.            *
*     mnc         Minimum characters that must match.                  *
*=====================================================================*/

static int cmssmatch(const char *item, const char *key, int mnc) {

   register int mc = 0;       /* Match count */
   for ( ; item[mc]; mc++) {
      if (item[mc] == ',' || item[mc] == ')') break;
      if (key[mc]  == '\0') return 0;
      if (toupper((unsigned)item[mc]) != toupper((unsigned)key[mc]))
         return 0;
      } /* End of item */
   return (mc >= mnc) ? mc : 0;
   } /* End cmssmatch */


/*=====================================================================*
*                              cmsmatch                                *
*                                                                      *
*  This is a version of crk smatch() customized for matching category  *
*  names.  Here the keys are in an array of C101CATN structures and    *
*  the search begins at a location determined by looking up the first  *
*  letter in the alphabetic category index.                            *
*                                                                      *
*  Arguments:                                                          *
*     pst         Ptr to C101STATE for this instantiation.             *
*     item        Ptr to name to be matched against master list.       *
*=====================================================================*/

static int cmsmatch(struct C101STATE *pst, const char *item) {

   catnm *pkeys;              /* Ptr to keys info */
   int best_match = 0;        /* Longest cmssmatch */
   int item1 = tolower((unsigned)item[0]);
   int retval = 0;            /* Index number of best match */
   int this_match;            /* Current cmssmatch */
   char tie_match;            /* TRUE if tie match */

   pkeys = pst->ppad[item1 - (int)'a'];
   for ( ; pkeys; pkeys=pkeys->pnca) {
      if (tolower((unsigned)pkeys->cname[0]) != item1) break;
      if (!(this_match = cmssmatch(item, pkeys->cname, 1))) continue;
      /* Return at once if there is an exact match */
      if (pkeys->cname[this_match] == '\0') return (int)pkeys->icat;
      /* Otherwise, determine best match */
      if (this_match > best_match) {
         best_match = this_match;
         tie_match = FALSE;
         retval = (int)pkeys->icat; }
      else if (this_match == best_match) tie_match = TRUE;
      }
   if (best_match && !tie_match) return retval;
   return 0;
   } /* End cmsmatch */


/*=====================================================================*
*                              mkcatndx                                *
*                                                                      *
*  Read the CIT datset directory and construct numeric and alphabetic  *
*  indexes for looking up categories during selector parsing.          *
*  Note:  The "BACKGROUND_Google" category will be kept, and can be    *
*  accessed by name, but it will not be assigned a category number.    *
*  Many of these seem to be random screenshots with lots of text.      *
*  Note:  We need to know how many images are in each category.  An    *
*  inventory file was made by GNR, but it seems safer to open each     *
*  subdir and count the entries.  If this fails, the run likely will.  *
*                                                                      *
*  Argument:                                                           *
*     pst      Ptr to master C101STATE struct                          *
*=====================================================================*/

static void mkcatndx(struct C101STATE *pst) {

   catnm *pcin;               /* Ptr to category index entry */
   catnm *pbin;               /* Ptr to background index entry */
   catnm **ppci;              /* Ptr to where to store ptr to catnm */
   catnm **ppai;              /* Ptr to alphabetic index */
   char  *pcnm,*pcnme;        /* Ptrs into a category name */
   DIR   *pdir;               /* Ptr to CIT top-level directory */
   DIR   *psub;               /* Ptr to CIT category-level dir */
   struct dirent *pcatdnm;    /* Ptr to a category from readdir() */
   struct dirent *psubdir;    /* Ptr to a category subdir entry */
   char  *headdir,*subdir;    /* Space for building subdir names */
   int   ncr;                 /* Number of categories read */
   char  clet;                /* Current letter in index */

   pcin = pst->pcin0 = (catnm *)mxCalloc(BCK_CAT, sizeof(catnm));
   mexMakeMemoryPersistent(pst->pcin0);
   ppci = &pst->pcins;
   /* 'z' - 'a' + 1 should work even for EBCDIC if ever needed */
   ppai = pst->ppad = (catnm **)mxCalloc('z' - 'a' + 1, PSIZE);
   mexMakeMemoryPersistent(pst->ppad);

   /* Just as a sanity check, if annotations are requested,
   *  be sure the annotation directory exists and can be read */
   if (pst->cmflgs & (cmCROP|cmMASK|cmDEFMSK)) {
      headdir = pst->aname;
      strcpy(headdir, pst->bname);
      strcat(headdir, AnnDir);
      pdir = opendir(headdir);
      if (!pdir)
         cmSysErr(645, "Error opening Annotations directory", errno);
      if (closedir(pdir) < 0)
         cmSysErr(650, "Error closing Annotations directory", errno);
      }

   headdir = pst->fname;
   strcpy(headdir, pst->bname);
   strcat(headdir, ImgDir);
   subdir = headdir + strlen(headdir);
   pdir = opendir(headdir);
   if (!pdir)
      cmSysErr(645, "Error opening CIT 101 directory", errno);

   ncr = 0;
   while ((pcatdnm = readdir(pdir))) { /* Assignment intended */
      int nim;
      char *pcx,*plcx;        /* Ptrs to name destinations */
      if (!strcmp(pcatdnm->d_name, ".") ||
          !strcmp(pcatdnm->d_name, "..")) continue;
      if (++ncr > BCK_CAT)
         cmMexErr2(648, "More than 102 categories in", pst->bname);
      /* Copy the category name to the index as it stands (for file
      *  lookup) and after making it lowercase (for sorting and
      *  searching -- searching in catpsel is case-independent */
      pcnme = pcatdnm->d_name + LCATNM;
      pcx = pcin->cname, plcx = pcin->lcname;
      for (pcnm=pcatdnm->d_name; pcnm<pcnme && *pcnm; ++pcnm)
         *pcx++ = *pcnm, *plcx++ = (char)tolower(*pcnm);
      /* Maneuver to count images */
      strncpy(subdir, pcin->cname, LCATNM);
      psub = opendir(headdir);
      if (!psub)
         cmSysErr(645, "Error opening image subdir", errno);
      nim = 0;
      while (readdir(psub)) ++nim;
      if (closedir(psub) < 0)
         cmSysErr(650, "Error closing image subdir", errno);
      nim -= 2;      /* Don't count . and .. */
      if (nim > MX_INST)
         cmMexErr2(640, "More than 800 images in", pcin->cname);
      pcin->nimgs = nim;
      /* Step to the next open catnm */
      *ppci = pcin; ppci = &pcin->pnca; ++pcin;
      }
   if (closedir(pdir) < 0)
      cmSysErr(650, "Error closing Categories directory", errno);

   /* Safety check */
   if (ncr != BCK_CAT)
      cmMexErr2(648, "Fewer than 102 categories in", pst->bname);

   /* Sort the category names */
   pst->pcins = sort(pst->pcin0, pcin->lcname - (char *)pcin,
      2*LCATNM, 1);

   /* Go through the sorted list, creating the first-letter index.
   *  At the same time, assign pncn pointers so it is easy to find
   *  categories by number, except do not assign a number to the
   *  background category.  Note:  The ppad index has 102 entries,
   *  but the 0'th is not used, so it holds exactly entries for
   *  categories 1 to MX_CAT (101).  */
   ncr = 0;
   clet = 0;
   for (pcin=pst->pcins; pcin; pcin=pcin->pnca) {
      if (pcin->lcname[0] != clet) {
         int ii;
         clet = pcin->lcname[0];
         ii = clet - 'a';
         if (ii < 0 || ii > ('z' - 'a'))  /* JIC */
            cmMexErr2(649, "Invalid category name in", pst->bname);
         ppai[ii] = pcin;
         }
      if (strcmp(pcin->cname, BckDir)) {
         /* This is a normal (not background) category.  Increment
         *  the category number and store a pointer to the catnm
         *  in the pncn slot at the current number.  */
         pst->pcin0[++ncr].pncn = pcin;
         pcin->icat = ncr;
         }
      }

   } /* End mkcatndx() */


/*=====================================================================*
*                               catpsel                                *
*  Parse a category selector list.  Arguments:                         *
*     pst         Ptr to C101STATE for this instantiation.             *
*                 (pst->pcss will be used as pointer to sspec          *
*                  that will hold the results.)                        *
*     ppsel       Ptr to ptr to string being parsed (updated           *
*                 on non-error return).                                *
*     pwk         Ptr to si16 work array large enough to hold at       *
*                 least MX_CAT+1 category numbers.                     *
*  Return value:  Number of selectors found.                           *
*                                                                      *
*  Note:  It may seem wasteful to list explicitly all the selectors in *
*  some long range, but this is necessary to simplify selection from   *
*  a mixed list at citmexget time.                                     *
*=====================================================================*/

static int catpsel(struct C101STATE *pst, char **ppsel, si16 *pwk) {

   sspec *pcs = pst->pcss;    /* Ptr to results spec */
   char  *ps = *ppsel;        /* Ptr used to scan the input */
   int   ib,ie;               /* Beginning, end of a range */
   int   nc;                  /* Number of categories found */

   if (*ps++ != '(')
      cmMexErr(637, "Missing '(' in category selector");

   nc = 0, ib = 0, ie = -1;
   for ( ; *ps; ++ps) {
      if (isalpha(*ps)) {
         /* Potentially got a name--should end with a comma or ')' */
         if (!(ib = cmsmatch(pst, ps)))
            cmMexErr(647, "Unknown category name");
         while (ps[1] != '\0' && ps[1] != ',' && ps[1] != ')') ++ps;
         }
      else if (*ps >= '0' && *ps <= '9') {
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
         if (ib < 1)
            cmMexErr(639, "Category number < 1");
         if (ie < 0) ie = ib;
         if (ie < ib) break;
         if (ie > MX_CAT)
            cmMexErr(639, "Category number > 101");
         for ( ; ib<=ie; ++ib) {
            pwk[nc++] = (si16)ib;
            if (nc >= MX_WKSZ) break;
            }
         if (*ps == ')') {
            mwSize ncbytes = SHSIZE*nc;
            *ppsel = ps+1;
            pst->ncats = nc;
            /* Now make a right-length copy from pwk */
            pcs->pcat = (ui16 *)mxMalloc(ncbytes);
            memcpy((char *)pcs->pcat, (char *)pwk, ncbytes);
            return nc;
            }
         ib = 0, ie = -1;
         }
      else break;
      }
   cmMexErr(639, "Malformed category selector");
   } /* End catpsel() */


/*=====================================================================*
*                               imgpsel                                *
*  Parse an image instance selector list.  Arguments:                  *
*     pst         Ptr to C101STATE for this instantiation.             *
*                 (pst->pcss will be used as pointer to sspec          *
*                  that will hold the results.)                        *
*     ppsel       Ptr to ptr to string being parsed (updated           *
*                 on non-error return).                                *
*     pwk         Ptr to si16 work array large enough to hold at       *
*                 least END_INST+1 image instance numbers.             *
*  Return value:  Number of selectors found.                           *
*                                                                      *
*  Note:  Here we can store ranges as +/- pairs, as they will be       *
*  expanded in pcis/ptts list contruction.  This allows for the '$' to *
*  be interpreted according to the number of images in each category.  *
*=====================================================================*/

static int imgpsel(struct C101STATE *pst, char **ppsel, si16 *pwk) {

   sspec *pcs = pst->pcss;    /* Ptr to results spec */
   char  *ps = *ppsel;        /* Ptr used to scan the input */
   int   ib,ii;               /* An image number */
   int   kr;                  /* TRUE if in a range */
   int   ni;                  /* Number of instances found */

   if (*ps++ != '(')
      cmMexErr(637, "Missing '(' in image selector");

   ni = ib = ii = kr = 0;
   for ( ; *ps; ++ps) {
      if (*ps == '$')
         ii = END_INST;
      else if (*ps >= '0' && *ps <= '9')
         ii = 10*ii + (*ps - '0');
      else if (*ps == '-') {
         if (kr) break;
         if (ps[-1] == ',') break;
         if (ii < 1)
            cmMexErr(639, "Image number < 1");
         if (ii > END_INST)
            cmMexErr(639, "Image number too big");
         pwk[ni++] = ib = ii;
         kr = TRUE;
         ii = 0;
         }
      else if (*ps == ',' || *ps == ')') {
         if (ps[-1] == ',' || ps[-1] == '-') break;
         if (ii < 1)
            cmMexErr(639, "Image number < 1");
         if (ii > END_INST)
            cmMexErr(639, "Image number too big");
         if (kr) {
            if (ii < ib) break;
            pwk[ni++] = -ii;
            kr = 0;
            }
         else
            pwk[ni++] = ii;
         if (*ps == ')') {
            mwSize nibytes = SHSIZE*ni;
            *ppsel = ps+1;
            pst->nimgs = (ui16)ni;
            /* Now make a right-length copy from pwk */
            pcs->pimg = (si16 *)mxMalloc(nibytes);
            memcpy((char *)pcs->pimg, (char *)pwk, nibytes);
            return ni;
            }
         ib = ii = kr = 0;
         }
      else break;
      }
   cmMexErr(639, "Malformed image selector");
   } /* End imgpsel() */


/*=====================================================================*
*                              makespec                                *
*                                                                      *
*  Create an empty selector spec and initialize it.  The linked list   *
*  is constructed LIFO since it is simpler and the order doesn't       *
*  really matter.                                                      *
*                                                                      *
*  Argument:                                                           *
*     pst      Ptr to master C101STATE struct                          *
*=====================================================================*/

static sspec *makespec(struct C101STATE *pst) {

   sspec *pss = (sspec *)mxCalloc(1, sizeof(sspec));
   if (!pss) cmMexErr(636, "No space for selector spec.");
   pss->pnss = pst->pfss;
   pst->pfss = pst->pcss = pss;
   pst->nspec += 1;
   pss->seed = pss->seed0 = pst->dfltseed++;
   mexMakeMemoryPersistent(pss);
   return pss;

   } /* End makespec() */


/*=====================================================================*
*                              mku1list                                *
*                                                                      *
*  This routine processes pending category and image selections to     *
*  make one of the lists in the pss->u1 union that is used for the     *
*  actual image acquisition in citmexget().  It is called whenever     *
*  a second category list is found within a single selection list,     *
*  and when a semicolon or end of udata indicates the current list     *
*  should be closed out.                                               *
*                                                                      *
*  Design note:  There are separate in-sequence and permute flags      *
*  for the final lists and the input lists.  This is to allow easy     *
*  checking for invalid changes in compound lists.                     *
*  Design note:  I decided to prescan the lists to decide how much     *
*  persistent storage is required, as there is no reasonable way to    *
*  make a good guess.  This is once-only per run (or new udata call),  *
*  so should not slow things down very much.                           *
*                                                                      *
*  Argument:                                                           *
*     pst      Ptr to master C101STATE struct.                         *
*     kend     TRUE if this is the final sublist in a combo list.      *
*=====================================================================*/

static void mku1list(struct C101STATE *pst, int kend) {

   catnm *pcn;
   sspec *pss = pst->pcss;
   si16  *piml0 = NULL;       /* Ptr to new TTSpec image list */
   ui32  szimgl;              /* Size of image list */
   int   icat,jcat,ncat,iimg,jimg,aimg,limg,nimg,mximgno;
   int   qttspec;             /* TRUE if TTSpec list called for */

/* It is an error if a TTSpec list has already been processed and there
*  is now another list before a semicolon, or if a CISpec list has been
*  processed and there is an attempt to switch to a TTSpec list.  */


   if (pss->ksel & KS_TTSL)
      cmMexErr(643, "Cannot add to a two-tiered list");
   qttspec = pss->ksel & (KS_SLBI|KS_ASBI|KS_PLBI);

/* Determine the space that will be needed for the final image list.
*  This must be checked individually for each category because of the
*  different numbers of images per category.  */

   ncat = (int)pst->ncats, nimg = (int)pst->nimgs;
   szimgl = 0;
   for (icat=0; icat<ncat; ++icat) {
      jcat = (int)pss->pcat[icat];
      pcn  = pst->pcin0[jcat].pncn;
      mximgno = (int)pcn->nimgs;
      for (iimg=0; iimg<nimg; ++iimg) {
         jimg = (int)pss->pimg[iimg];
         aimg = abs(jimg);
         if (jimg > 0) {   /* Individual selection or start of range */
            if (aimg > mximgno)
               cmMexErr2(644, "1st image no. < no. in category for",
                  pcn->cname);
            limg = aimg, szimgl += 1;
            }
         else {            /* End of range */
            if (aimg > mximgno) aimg = mximgno;
            szimgl += aimg - limg;
            }
         } /* End iimg loop */
      } /* End icat loop */
   if (szimgl == 0)
      cmMexErr(644, "No images were selected");

/* Generate a TTSpec list.  This is the case where there are separate
*  ordering codes for the image list vs. the category list.  The pimg
*  pointer will be reused for the final image lists.  */

   if (qttspec) {
      /* Allocate space for TTSpec list and image lists */
      TTSpec *ptt = pss->u1.ptts =
         (TTSpec *)mxMalloc(sizeof(TTSpec)*ncat);
      ui32 oiml = 0;       /* Offset into piml0 lists */
      ui32 mxlimg = 0;     /* For nbatch default */
      piml0 = (si16 *)mxMalloc(SHSIZE*szimgl);
      mexMakeMemoryPersistent(piml0);
      mexMakeMemoryPersistent(ptt);

      /* Permute categories first if required, update flags */
      if (pss->ksel & KS_ASBC) {
         si16perm(pss->pcat, &pss->seed, ncat);
         pss->ksel |= KS_CPERM;
         }
      else if (pss->ksel & KS_SLBC)
         pss->ksel |= KS_CSEQ;
      if (pss->ksel & KS_ASBI)
         pss->ksel |= KS_IPERM;
      else if (pss->ksel & KS_SLBI)
         pss->ksel |= KS_ISEQ;

      /* Prepare the lists */
      for (icat=0; icat<ncat; ++icat,++ptt) {
         jcat = (int)pss->pcat[icat];
         pcn  = pst->pcin0[jcat].pncn;
         ptt->icat = jcat;
         ptt->oimg = oiml;
         mximgno = (int)pcn->nimgs;
         for (iimg=0; iimg<nimg; ++iimg) {
            jimg = (int)pss->pimg[iimg];
            aimg = abs(jimg);
            if (aimg > mximgno) aimg = mximgno;
            if (jimg > 0) piml0[oiml++] = limg = aimg;
            else for (++limg; limg<=aimg; ++limg)
               piml0[oiml++] = limg;
            } /* End iimg loop */
         ptt->limg = oiml - ptt->oimg;
         if (ptt->limg > mxlimg) mxlimg = ptt->limg;
         /* Permute image list if requested */
         if (pss->ksel & KS_ASBI)
            si16perm(piml0+ptt->oimg, &pss->seed, (si32)ptt->limg);
         } /* End icat loop */
      /* Set default batch number if not entered
      *  and check for error */
      if (!pss->nbatch) pss->nbatch = mxlimg;
      else if (pss->nbatch > mxlimg)
         cmMexErr(646, "Batch N exceeds images in largest category.");
      pss->nu1 = pss->icseq = (ui32)ncat;
      pss->ksel |= KS_TTSL;
      } /* End making TTSpec list */

/* Generate a new CISpec or add to an existing CISpec.  It is an error
*  if adding to an existing list and any cycling mode is specified
*  (use the code in pound-if 0 to check only for changes).  */

   else {
      CISpec *pci;
      ui32   newnu1;
      if (pss->ksel & KS_CISL) {
         if (qttspec ||
#if 0
               (pss->ksel << KS_CISH ^ pss->ksel) & (KS_SLBC|KS_ASBC)
#else
               pss->ksel & (KS_SLBC|KS_ASBC)
#endif
            )
         cmMexErr(643, "Cannot mix selection orders");
         } /* End error checking on concatenated lists */
      else {
         /* Save category parsing flags into final flags */
         if (pss->ksel & KS_ASBC)
            pss->ksel |= KS_CPERM;
         else if (pss->ksel & KS_SLBC)
            pss->ksel |= KS_CSEQ;
         }
      /* Allocate (or increase) space for a CISpec list */
      newnu1 = pss->nu1 + szimgl;
      pss->u1.pcis = (CISpec *)
         mxRealloc(pss->u1.pcis, sizeof(CISpec)*newnu1);
      pci = pss->u1.pcis + pss->nu1;
      mexMakeMemoryPersistent(pss->u1.pcis);

      /* Prepare (or extend) the list */
      for (icat=0; icat<ncat; ++icat) {
         jcat = (int)pss->pcat[icat];
         pcn  = pst->pcin0[jcat].pncn;
         mximgno = (int)pcn->nimgs;
         for (iimg=0; iimg<nimg; ++iimg) {
            jimg = (int)pss->pimg[iimg];
            aimg = abs(jimg);
            if (aimg > mximgno) aimg = mximgno;
            if (jimg > 0) limg = aimg;
            for ( ; limg<=aimg; ++limg,++pci)
               pci->icat = jcat, pci->imgno = limg;
            } /* End iimg loop */
         } /* End icat loop */

      /* Finalize this CISpec */
      pss->nu1 = newnu1;
      pss->ksel |= KS_CISL;
      if (kend) {
         /* Set default batch number if not entered
         *  and check for error */
         if (!pss->nbatch) pss->nbatch = newnu1;
         else if (pss->nbatch > newnu1)
            cmMexErr(646, "Batch number exceeds specs.");
      /* If permutation is requested, now do the permutation.  We can
      *  treat the CISpec data as single si32s for this purpose.  */
         if (pss->ksel & KS_CPERM)
            si32perm((si32 *)pss->u1.pcis, &pss->seed, (si32)newnu1);
         }
      } /* End making CISpec list */

   /* Free original cat and image lists.  If this is a TTSpec,
   *  store the pointer to the new image list over the old one. */
   mxFree(pss->pcat); pss->pcat = NULL;
   mxFree(pss->pimg); pss->pimg = piml0;

   /* Clear list-prep flags */
   pss->ksel &= ~(KS_SLBC|KS_ASBC|KS_SLBI|KS_ASBI|KS_PLBC|KS_PLBI);
   pst->ncats = pst->nimgs = 0;

   } /* End mku1list() */

/*=====================================================================*
*                              citmexset                               *
*=====================================================================*/

/* For debugging, we need the wrapper approach */

void citmexset(int nlhs, mxArray *plhs[],
      int nrhs, const mxArray *prhs[]) {

   sem_t  *CMCCSem;           /* Ptr to memory control semaphore */
   struct C101STATE *pst;     /* Ptr to persistent local data */
   sspec  *pss;               /* Ptr to current selector spec */
   char   *prdd;              /* Ptr to data being read */
   ui16   *pwk;               /* Ptr to work area */
   char   *udata,*vdata;      /* Ptrs to udata argument string */
   double *pmx,*pmy;          /* Ptrs to data in mxix,mxiy vectors */
   double tnx,tny;            /* Temps for image size args */
   mwSize lbname,lbfnm;       /* Length of pst->bname, bname+fname */
   size_t nmix,nmiy;          /* Sizes of mxix,mxiy vectors */
   ui32   nel;                /* Number of eligible images */
   ui32   dfltprob;           /* Default probability */
   ui32   sumprob;            /* Sum of probabilities */
   int    ldat;               /* Length of data being read */

   /* Clear pointers to avoid mxFree() errors */
   pst = NULL; pwk = NULL; udata = NULL;

/* Determine type of call from number of arguments */

   switch (nrhs) {

/* Reset, close, or revised udata call */

case 2:
   if (nlhs != 0)
      cmMexErr(631, "Function must have no return value.");
   /* Access CitMexComm data */
   if (!mxIsClass(prhs[0],"uint8"))
      cmMexErr(632, "1st arg must be CitMexComm.");
   pst = (struct C101STATE *)mxGetData(prhs[0]);
   if (!pst || pst->sigck != C101_SIGCK)
      cmMexErr(632, "Bad CitMexComm arg.");
   if (!mxIsChar(prhs[1]))
      cmMexErr(633, "udata or mode arg must be a string.");
   ldat = mxGetNumberOfElements(prhs[1]);
   if (ldat <= 0 || !(udata = mxMalloc(ldat+1)) ||
         mxGetString(prhs[1], udata, ldat+1))
      cmMexErr(634, "Unable to copy udata or mode arg");
   if (cmssmatch(udata, "close", 5)) {
      /* Close case */
      cmFreeAll();
      mxFree(udata);
      return;
      }
   if (cmssmatch(udata, "reset", 5)) {
      /* Reset case */
      for (pss=pst->pfss; pss; pss=pss->pnss)
         pss->seed = pss->seed0, pss->icseq = pss->nu1;
      pst->ivar = 0;
      mxFree(udata);
      return;
      }
   else {
      /* Remove previous selectors and read new ones */
      freespec(pst);
      break;   /* Proceed as for initial call */
      }

/* Initialization call */

case 5:
   if (nlhs != 1)
      cmMexErr(631, "Function must have one return value.");
   if (!mxIsChar(prhs[0]) || !mxIsDouble(prhs[1]) ||
         !mxIsDouble(prhs[2]) || !mxIsChar(prhs[3]) ||
         !mxIsUint32(prhs[4]))
      cmMexErr(633, "Argument(s) not of correct type(s).");
   nmix = mxGetNumberOfElements(prhs[1]);
   nmiy = mxGetNumberOfElements(prhs[2]);
   if (nmix < 1 || nmix > 3 || nmiy < 1 || nmiy > 2 ||
         mxGetNumberOfElements(prhs[4]) != 1)
      cmMexErr(633, "Argument(s) not of correct length(s).");

/* Create the CitMexComm common data area and initialize.
*  Note:  mxCreateNumericMatrix terminates script on failure.  */

   pCitMexComm = plhs[0] =
      mxCreateNumericMatrix(sizeof(struct C101STATE), 1,
      mxUINT8_CLASS, mxREAL);
   pst = (struct C101STATE *)mxGetData(pCitMexComm);
   pst->sigck = C101_SIGCK;
   pst->dfltseed = DFLT_SEED;

/* Establish a semaphore that cmFreeAll can use to avoid freeing
*  memory that has already been freed.  This avoids relying on
*  anything Matlab might mess with.  Give an abexit if this memory
*  already exists and has not been cleared (semaphore > 0), because
*  the mexAtExit would need a more complicated setup to deal with
*  more than one CitMexComm block.  */

   {  int   qerrcl;           /* True if memory was not cleared */
      int   qerrno;           /* errno for this error */
      if ((CMCCSem =
            sem_open(CMSemNm, O_CREAT, URW_Mode, 0)) == SEM_FAILED)
         abexitme(654, "Could not open " CMSemNm);
      qerrcl = sem_trywait(CMCCSem); qerrno = errno;
      /* Post before exiting so get same error again if user
      *  does not do a 'mex clear' first.  */
      if (sem_post(CMCCSem) < 0)
         abexitme(654, "Error posting " CMSemNm);
      if (qerrcl == 0) {
         mexPrintf("Previous sem.CitMexCommCtrl was not cleared\n"
            "---exit Matlab and do \"semrm CitMexCommCtrl\"\n");
         abexitm(655, "Previous CitMexComm was not cleared");
         }
      else if ((errno = qerrno) != EAGAIN)
         abexitme(654, "Error testing " CMSemNm);
      } /* End qerrclr,qerrno local scope */

/* From here on, there may be stuff to clean up on exit */

   mexAtExit(cmFreeAll);

/* Read and save the arguments */

   /* Read the kops flags first--used below */
   pst->cmflgs = *(ui32 *)mxGetPr(prhs[4]);
   if ((pst->cmflgs & (cmMASK|cmDEFMSK)) == (cmMASK|cmDEFMSK))
      cmMexErr(633, "Incompatible kopt bits");

   /* Read the imgdir and be sure it ends with '/' */
   ldat = mxGetNumberOfElements(prhs[0]);
   lbname = lbfnm =
      2*ldat + strlen(ImgDir) + strlen(ImgFNm) + LCATNM + 4;
   if (pst->cmflgs & (cmCROP|cmMASK|cmDEFMSK)) lbname +=
      ldat + strlen(AnnDir) + strlen(AnnFNm) + LCATNM + 1;
   if (ldat <= 0 || !(pst->bname = mxMalloc(lbname)) ||
         mxGetString(prhs[0], pst->bname, ldat+1))
      cmMexErr(634, "Unable to copy imgdir");
   mexMakeMemoryPersistent(pst->bname);
   if (pst->bname[ldat-1] != '/') {
      pst->bname[ldat++] = '/';
      pst->bname[ldat] = '\0';
      }
   pst->lbnm = ldat;
   pst->fname = pst->bname + ldat + 1;
   if (pst->cmflgs & (cmCROP|cmMASK|cmDEFMSK))
      pst->aname = pst->bname + lbfnm;

   /* Read the max image sizes */
   tnx = *(pmx = mxGetPr(prhs[1]));
   tny = *(pmy = mxGetPr(prhs[2]));
   if (tnx <= 0 || tnx > UI16_MAX || tny <= 0 || tny > UI16_MAX)
      cmMexErr(635, "mxix or mxiy <= 0 or absurdly large.");
   pst->tvx = (ui16)tnx;
   pst->tvy = (ui16)tny;
   if (nmix >= 2) {
      tnx = pmx[1];
      if (tnx < 0 || tnx > UI16_MAX)
         cmMexErr(635, "x border < 0 or absurdly large.");
      pst->tbx = (ui16)tnx;
      if (nmix == 3) {
         tnx = pmx[2];
         if (tnx < 0 || tnx > MX_PBORD)
            cmMexErr(635, "Paint border < 0 or absurdly large.");
         pst->pbord = (float)tnx;
         }
      }
   if (nmiy == 2) {
      tny = pmy[1];
      if (tny < 0 || tny > UI16_MAX)
         cmMexErr(635, "y border < 0 or absurdly large.");
      pst->tby = (ui16)tny;
      }

   /* Check amd extract udata arg */
   ldat = mxGetNumberOfElements(prhs[3]);
   if (ldat <= 0 || !(udata = mxMalloc(ldat+1)) ||
         mxGetString(prhs[3], udata, ldat+1))
      cmMexErr(634, "Unable to copy udata");

   /* Read the imgdir directory and construct the alphabetic
   *  and numeric category indexes  */
   mkcatndx(pst);
   break;

default:
   cmMexErr(601, "Wrong number of arguments.");

   } /* End nrhs switch */

/* Initialize for INIT or RESTART call */

   sumprob = 0;
   vdata = udata;
   pwk = (ui16 *)mxMalloc(BCK_CAT*SHSIZE);

/* Start of a new spec (first or semicolon) will enter here.
*  Create a selector spec and initialize it.  */

StartSpecHere:
   pss = makespec(pst);
   while (*vdata) {           /* Parse the udata string */
      switch (toupper(*vdata++)) {
      case 'C':                  /* Category selectors */
         if (pss->ksel & KS_CLAST)
            cmMexErr(638, "Multiple category selectors");
         pss->ksel = (pss->ksel & ~KS_ILAST) | KS_CLAST;
         if (pss->nu1)           /* Compound list */
            mku1list(pst, FALSE);
         catpsel(pst, &vdata, pwk);
         break;
      case 'I':                  /* Instance selectors */
         if (pss->ksel & KS_ILAST)
            cmMexErr(638, "Multiple image selectors");
         pss->ksel = (pss->ksel & ~KS_CLAST) | KS_ILAST;
         imgpsel(pst, &vdata, pwk);
         break;
      case 'N': {                /* Batch number code */
         si32 tbatch = 0;
         if (*vdata++ != '(')
            cmMexErr(642, "Missing '(' in batch number.");
         while (*vdata && *vdata != ')') {
            int md = (int)(*vdata++);
            if (isdigit(md))
               tbatch = 10*tbatch + (md - '0');
            else cmMexErr(642, "Nondigit in batch number.");
            }
         if (tbatch <= 0)        /* Overflow? */
            cmMexErr(642, "Invalid batch number.");
         pss->nbatch = tbatch;
         vdata++;
         } /* End 'N' local scope */
         break;
      case 'P': {                /* Probability */
         ui32 tprob = 0, tdec = 1;
         int  ndig = 0;
         if (*vdata++ != '(')
            cmMexErr(642, "Missing '(' in P code.");
         if (pss->prob == 0) pst->nprob += 1;
         while (*vdata && *vdata != ')') {
            int md = (int)(*vdata++);
            if (md == '.') tdec = 1;
            else if (isdigit(md)) {
               tprob = 10*tprob + (md - '0');
               tdec *= 10;
               ndig += 1; }
            else cmMexErr(642, "Illegal character in P code.");
            }
         if (ndig >= UI32_SIZE)
            cmMexErr(642, "Too many digits in P code.");
         /* Because we don't want to load ROCKS routines here,
         *  we use 64-bit if available, otherwise double float */
#ifdef HAS_I64
         {  ui64 tnum = ((ui64)tprob << 31)/(ui64)tdec;
            if (tnum > MX_PROB)
               cmMexErr(641, "Probability > 1.");
            pss->prob = (ui32)tnum;
            } /* End tnum local scope */
#else
         {  double dmxp = (double)MX_PROB;
            double tnum = dmxp*(double)tprob/(double)tdec;
            if (tnum > dmxp)
               cmMexErr(641, "Probability > 1.");
            pss->prob = (ui32)tnum;
            } /* End tnum local scope */
#endif
         sumprob += pss->prob;
         if (sumprob > (MX_PROB+MX_PROBERR))
            cmMexErr(641, "Total probabilities > 1.");
         vdata++;
         } /* End 'P' local scope */
         break;
      case 'S': {                /* Random number seed */
         si32 tseed = 0;
         if (*vdata++ != '(')
            cmMexErr(642, "Missing '(' in seed.");
         while (*vdata && *vdata != ')') {
            int md = (int)(*vdata++);
            if (isdigit(md))
               tseed = 10*tseed + (md - '0');
            else cmMexErr(642, "Nondigit in seed.");
            }
         if (tseed <= 0 || tseed == SI32_MAX)
            cmMexErr(642, "Invalid seed.");
         pss->seed0 = tseed;
         vdata++;
         } /* End 'S' local scope */
         break;
      case '/':                  /* Systematic/random separator */
         if (*vdata == 'C') {
            if (pss->ksel & KS_SLBC)
               cmMexErr(643, ">1 '/' before cat list");
            pss->ksel |= KS_SLBC;
            }
         else if (*vdata == 'I') {
            if (pss->ksel & KS_SLBI)
               cmMexErr(643, ">1 '/' before img list");
            pss->ksel |= KS_SLBI;
            }
         else
            cmMexErr(643, "Misplaced slash");
         break;
      case '*':                  /* Permuted batch separator */
         if (*vdata == 'C') {
            if (pss->ksel & KS_ASBC)
               cmMexErr(643, ">1 '*' before cat list");
            pss->ksel |= KS_ASBC;
            }
         else if (*vdata == 'I') {
            if (pss->ksel & KS_ASBI)
               cmMexErr(643, ">1 '*' before img list");
            pss->ksel |= KS_ASBI;
            }
         else
            cmMexErr(643, "Misplaced asterisk");
         break;
      case '+':                  /* Systematic batch separator */
         if (*vdata == 'C') {
            if (pss->ksel & KS_PLBC)
               cmMexErr(643, ">1 '+' before cat list");
            pss->ksel |= KS_PLBC;
            }
         else if (*vdata == 'I') {
            if (pss->ksel & KS_PLBI)
               cmMexErr(643, ">1 '+' before img list");
            pss->ksel |= KS_PLBI;
            }
         else
            cmMexErr(643, "Misplaced plus sign");
         break;
      case ';':                  /* New selection block */
         mku1list(pst, TRUE);       /* Process pending selectors */
         goto StartSpecHere;
      default:                   /* None of the above: error */
         cmMexErr(614, "Unrecognized selector code.");
         } /* End code switch */
      if (*vdata == ',') ++vdata;   /* Skip over comma */
      } /* End parsing loop */
   mku1list(pst, TRUE);             /* Process pending selectors */

/* Determine fraction of unit probability not assigned by user.
*  Divide this by number of selection blocks that did not have
*  a probability assigned to yield the default probability.  */

   if (pst->nprob >= pst->nspec) {
      /* Note that sumprob > (MX_PROB+MX_PROBERR) would already
      *  have been detected as an error earlier, so here only
      *  need to check the opposite case.  */
      if (sumprob < (MX_PROB-MX_PROBERR))
         cmMexErr(611, "Total probabilities < 1.");
      dfltprob = 0;
      }
   else
      dfltprob = (MX_PROB - sumprob)/(pst->nspec - pst->nprob);

/* Do final checking and setup relating to input selectors */

   sumprob = 0;
   for (pss=pst->pfss; pss; pss=pss->pnss) {
      /* Restart seed is value after any permutations done */
      pss->seed0 = pss->seed;
      /* Assign cumulative probability */
      if (pss->prob == 0) pss->prob = dfltprob;
      sumprob += pss->prob;
      /* Last block should have cumulative probability 1.0 */
      pss->prob = pss->pnss ? sumprob : MX_PROB;
      }

/* All's well--free work area and return */

   mxFree(udata);
   mxFree(pwk);
   return;

   } /* End citmexset() */

void mexFunction(int nlhs, mxArray *plhs[],
      int nrhs, const mxArray *prhs[]) {
   citmexset(nlhs, plhs, nrhs, prhs);
   } /* End mexFunction() */

