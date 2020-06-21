/* (c) Copyright 2010, The Rockefeller University *11113* */
/***********************************************************************
*                               norbmix                                *
************************************************************************
*  This is a CNS user program to read selected images from the "small  *
*  NORB dataset" by Fu Jie Huang and Yann LeCun, Courant Institute,    *
*  New York University.  See:  Y.  LeCun, F.J.  Huang, and L.  Bottou, *
*  Learning methods for generic object recognition with invariance to  *
*  pose and lighting, IEEE Computer Society Conference on Computer     *
*  Vision and Pattern Recognition (CVPR), 2004.                        *
*                                                                      *
*  This is the ultimate NORB image reader, designed to select objects  *
*  at random based on some identification criteria, and then to cycle  *
*  systematically through all objects that meet those criteria.  This  *
*  may be useful for training runs in which, e.g. different rotations  *
*  of an object should be viewed sequentially, but the objects should  *
*  be otherwise random.                                                *
*                                                                      *
*  Version 2 has been enhanced to allow reading images from both the   *
*  "training" and "test" files in the same run (in spite of their      *
*  different internal arrangement), to specify more than one set of    *
*  selection criteria, each to be used with some given probability,    *
*  and to allow changing the selection criteria within a single run.   *
*                                                                      *
*  The calling arguments are as specified in the CNS documention for   *
*  a UPGM-type image source (CAMERA control card).                     *
*----------------------------------------------------------------------*
*  Initialization call:                                                *
*     int norbmset(struct UTVDEF *putv, char *cam,                     *
*        char *ufile, char *udata, enum tvinit_kcall kcall)            *
*  where:                                                              *
*     putv     is a pointer to a CNS-suppled UTVDEF structure with     *
*              additional parameters (e.g. the size of the desired     *
*              subimages), and where this program can store a pointer  *
*              to its persistent internal data structure. The UTVDEF   *
*              struct is defined in utvdef.h in the CNS source code    *
*              directory.                                              *
*     cam      Name of this camera (not used by norbmix).              *
*     ufile    Ptr to string entered by user as source file name.      *
*              This may refer to either the training or test data set. *
*     udata    Character string giving the image data selectors, cate- *
*              gorization mode, and random number seed.                *
*     kcall    TV_INIT for the initialization call.  The tvinit_kcall  *
*              enum is defined in utvdef.h.                            *
*  Contents of the ufile string:  A partial or full path name          *
*     giving the location of the six "-dat.mat", "-cat.mat", and       *
*     "-info.mat" files of the NORB dataset.  Only the directory       *
*     information is used--the file names are supplied internally.     *
*     For compatibility with previous versions of norbmix, partial     *
*     file names following the final '/' will be ignored.  All six     *
*     files must exist in the same directory.                          *
*  Contents of the udata string:  One or more selector specifications  *
*     separated by semicolons.  Any or all of the selector specs may   *
*     be assigned a probability ('P' code, see below).  The assigned   *
*     P values must add to 1.0 (within a small error) or less if any   *
*     selector specs do not have P values assigned.  The difference    *
*     between 1.0 and the sum of the assigned P values is divided      *
*     equally among all selector specs that do not have an assigned    *
*     P value.  On each norbmix call that is not part of a selection   *
*     sequence, one of the specs is chosen at random with the given    *
*     probability, and a stimulus is then chosen according to that     *
*     spec.  Any specified cycle is then completed on subsequent       *
*     norbmix calls.  A new selector spec is chosen on the next call   *
*     after the cycle is completed.                                    *
*        A selector spec comprises up to six selector strings in any   *
*     order.  Intermixed among these (but normally following them) in  *
*     any order may be a probability code, a categorization mode code, *
*     and/or a random number seed code.  All these codes are separated *
*     by commas, except the comma after one selector string may be     *
*     replaced with a slash.                                           *
*        Only those images will be returned whose properties match     *
*     selector codes.  Selectors to the left of the '/' will be used   *
*     in random order, selectors to the right of the '/' are cycled    *
*     through in the order given.  If no slash is entered, all selec-  *
*     tors are randomized.                                             *
*        Each selector string comprises a single letter followed by a  *
*     numeric selector list enclosed in parentheses.  The initial let- *
*     ter determines which variable that list applies to, abbreviated  *
*     as:  V (camera view), L (lighting), C (category), I (instance),  *
*     E (elevation), and A (azimuth).  Each numeric selector list is   *
*     a set of integers and/or ranges separated by commas.  An integer *
*     selects an allowed value of an image property.  A range is a     *
*     starting value followed by a minus sign and an ending value.     *
*     (These are a subset of the constructs allowed in a CNS iteration *
*     list.)  The allowed selectors for each variable are:  (V) camera *
*     (0 to 1), (L) light condition (0 to 5), (C) object category      *
*     (0 to 4), (I) object instance (0 to 9), (E) elevation (0 to 8,   *
*     encoding 30 to 70 deg. in 5 deg. intervals), and (A) azimuth     *
*     (0 to 17, encoding 0 to 340 deg. in 20 deg. intervals.  If a     *
*     list is absent, all values of that property will be accepted.    *
*        If the object instance is 0,1,2,3, or 5, the object will be   *
*     read from the "testing" file.  If the object instance is 4,6,7,  *
*     8, or 9, the object will be read from the "training" file.       *
*                                                                      *
*        A probability code is the letter 'P' followed by a fraction   *
*     between 0 and 1 in parentheses.  As explained above, any selec-  *
*     tor specs that are not assigned a probability will receive as    *
*     default an equal share of the difference between 1.0 and the sum *
*     of the probabilites that are assigned to other specs.            *
*                                                                      *
*        A categorization mode code is the letter 'M' followed by a    *
*     single number.  The number has the following values:             *
*     (0) The default.  ign of each stimulus is its category + 1       *
*        and isn is 10*category + instance + 1.                        *
*     (1) Method for analyzing rotational invariances.                 *
*        ign = 15*category + 3*instance + rotation/5 + 1.              *
*        isn = 3*(ign-1) + rotation mod 5 + 1                          *
*                 (best to omit 3,4,8,9,13-17).                        *
*     (2) 2nd method for analyzing rotational invariances (10 grps of  *
*         5 categories x 2 azimuths (trained, untrained) and 100 stim. *
*         (5 categories x 10 instances x 2 azimuths)).                 *
*        ign = 2*category + 1 if rotn is 3-5,9-11,15-17 + 1.           *
*        isn = 20*category + 2*instance + 1 (az%6>2) + 1.              *
*                                                                      *
*        A random number seed code is the letter 'S' followed by a     *
*     value for the random number seed (0 < seed < 2**31-1) in         *
*     parentheses.
*                                                                      *
*  Return value:                                                       *
*     0        Success.                                                *
*     1        Error parsing camera selectors.                         *
*     2        Error parsing lighting selectors.                       *
*     3        Error parsing category selectors.                       *
*     4        Error parsing instance selectors.                       *
*     5        Error parsing elevation selectors.                      *
*     6        Error parsing azimuth selectors.                        *
*     7        Unknown selector type letter.                           *
*     8        Number of selections exceeds number images in dataset.  *
*     9        More than one slash delimiter.                          *
*     10       Averaging service requested, not yet implemented.       *
*     11       Color mode requested does not match file.               *
*     12       Closing call but UTVSTATE was not initialized.          *
*     13       Undefined value for M code.                             *
*     14       Random number seed not in parens, had nondigit          *
*              characters, or invalid numeric value.                   *
*     15       Number of images that match selectors exceeds           *
*              product of number selected in each variable.            *
*     16       Specified probabilities add to more than 1.0, or do not *
*              add to near 1.0 (if all given), or do add to near 1.0   *
*              when not all are given.                                 *
*     17       Probability value not in parens, had nondigit           *
*              characters, or invalid numeric value.                   *
*     18       Duplicated selector type letter.                        *
*     19       Unable to allocate UTVSTATE or file name or selection   *
*              table structures.                                       *
*     20       Did not reach end-of-file simultaneously on info        *
*              and category files.                                     *
*     21-26    Category read from the input is outside the range       *
*              defined for camera, lighting, category, instance,       *
*              elevation, or azimuth, respectively.                    *
*     31-34    Error opening/closing training data file.               *
*     41-44    Error opening/closing training category file.           *
*     51-54    Error opening/closing training information file.        *
*     61-64    Error opening/closing testing data file.                *
*     71-74    Error opening/closing testing category file.            *
*     81-84    Error opening/closing testing information file.         *
*                                                                      *
*  Notes:  Max length of ufile and udata strings is 250 chars each,    *
*     enforced by code in d3grp1, not this program.  If duplicate      *
*     selectors are entered, the latest one will be used.  This is     *
*     not detected as an error.                                        *
*----------------------------------------------------------------------*
*  Image acquisition call (made only after norbmset call succeeds):    *
*     int norbmix(struct UTVDEF *putv, PIXEL *image)                   *
*  where:                                                              *
*     putv     is a pointer to the same UTVDEF structure used by       *
*              the setup call.                                         *
*     image    Pointer to array of size tvx x tvy where image should   *
*              be stored.                                              *
*  Return value:                                                       *
*     0        Success.                                                *
*     6        Error reading data file.                                *
*     12       UTVSTATE was not set up properly.                       *
*     14       Error skipping over unused pixels within an image.      *
*     15       Coding error - should never happen.                     *
*----------------------------------------------------------------------*
*  Reset call:                                                         *
*     int norbmset(struct UTVDEF *putv, char *cam,                     *
*        char *ufile, char *udata, enum tvinit_kcall kcall)            *
*  where:                                                              *
*     All values are as described above for the initial norbmset()     *
*     call, except kcall is TV_RESET.  (This call will not be made     *
*     from CNS if putv->tvuflgs & TV_BYPR is set).  This call restarts *
*     the image sequence from the beginning, but does not change it,   *
*     i.e. the ufile and udata arguments are ignored.  In the present  *
*     design, this amounts to resetting the random number seed and     *
*    initial state of each systematic selection loop.                  *
*----------------------------------------------------------------------*
*  Restart call:                                                       *
*     int norbmset(struct UTVDEF *putv, char *cam,                     *
*        char *ufile, char *udata, enum tvinit_kcall kcall)            *
*  where:                                                              *
*     All values are as described above for the initial norbmset()     *
*     call, except kcall is TV_RESTART.  This is basically the same    *
*     as the original norbmset() call--all settings and storage are    *
*     released and the program reads new ufile and udata information.  *
*     The possible return codes are also as listed above for the       *
*     initial norbmset() call.                                         *
*----------------------------------------------------------------------*
*  Closeout call:                                                      *
*     int norbmset(struct UTVDEF *putv, char *cam,                     *
*        char *ufile, char *udata, enum tvinit_kcall kcall)            *
*  where:                                                              *
*     putv, cam, and return values are same as above.  ufile and       *
*     udata are NULL.  kcall is TV_CLOSE.                              *
*     All files are closed and all memory is released.                 *
*----------------------------------------------------------------------*
*  Design note:  I wanted a input method as close as possible to the   *
*     standard CNS parsing rules but that would not involve loading    *
*     a whole copy of the crk cryin/cdscan/ilstread routines from a    *
*     PIC library or forking another process or accessing stdin/out    *
*     buffered in the main CNS code.  The scheme used here keeps the   *
*     parsing rules simple enough that simple C library functions      *
*     can be used for text handling.                                   *
*  Design note:  It was decided to close the info and cat files during *
*     a run and reopen and reread in the event of a restart.  This     *
*     simplifies the bookkeeping, may save a little memory during the  *
*     run, and even (in principle) allows completely different files   *
*     to be used at restart time.                                      *
************************************************************************
*  V1A, 08/05/10, GNR - New program, based on norbrand.c               *
*  V2A, 09/09/10, GNR - Add multiple selection criteria, restarts,     *
*                       and ability to use images from both files.     *
*  Rev, 09/30/10, GNR - Correct reqidf bug accessing training file     *
*                       only--this should not affect any earlier runs. *
***********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "sysdef.h"
#include "rkarith.h"
#include "utvdef.h"

/* Define maximum values for the various selector types */
#define MX_MCODE   2       /* Highest M code */
#define NSVARS     6       /* Number of selector types */
#define MX_CAT     4
#define MX_CAM     1
#define MX_LGT     5
#define MX_AZI    17
#define MX_ELV     8
#define MX_INST    9
#define MX_ALL    18       /* Highest MX_xxx + 1 */
#define MX_IMGS   24300    /* Max selectable images/file */
#define MX_PROB ((ui32)(1<<31))  /* Max probability */
#define MX_PROBERR 6       /* Max error in user's prob sum */
#define DFLT_SEED 10001    /* Default random number seed */
#define KILLIMG 0xff       /* sv value to kill image */

/* Define order control index variables */
#define KV_View    0       /* View camera */
#define KV_Light   1       /* Lighting */
#define KV_Cat     2       /* Category */
#define KV_Inst    3       /* Instance */
#define KV_Elev    4       /* Elevation */
#define KV_Azim    5       /* Azimuth */

/* Define indexes for accessing the two database files */
#define NNDF       2       /* Number of NORB data files */
#define DF_TRAIN   0       /* "training" files */
#define DF_TEST    1       /* "testing" files */
#define RC_INCR   30       /* Error code increment for test files */

/* The UTVINFO structure contains the number of an eligible image
*  and its group and stimulus identifier numbers, which are stored
*  during startup and returned to the caller at norbmix() time.  */

struct UTVINFO {
   ui16  imdf;             /* Image data file (DF_TRAIN or DF_TEST) */
   ui16  imgno;            /* Image number (for fdimg lseek) */
#if MX_IMGS > UI16_MAX
#error Need larger variable than ui16 to contain image number
#endif
   ui16 isn;              /* Stimulus number */
   ui16 ign;              /* Group number */
   } ;

/* The UTVSEL structure contains information for making selections
*  over one image variable.  */

struct UTVSEL {
   off_t offmult;          /* Offset multiplier for this variable */
   int   nvar;             /* Number of selectors found */
   byte  sv[MX_ALL];       /* Space for selectors */
   } ;

/* The UTVFILE structure contains information for reading images
*  from one database file.  (Although in fact the two NORB data
*  files have the same 96x96 image records, this information is
*  carried separately here to make it easier to add other
*  data base files later.)  */

struct UTVFILE {
   ui32  limg;             /* Length of one image record--
                           *  0 if this file not being used */
   ui32  l1rd;             /* Length of one read */
   off_t lskp1;            /* Bytes to skip before first read */
   off_t lskp2;            /* Bytes to skip between reads */
   int   fdimg;            /* File descriptor for image file */
   int   fdcat;            /* File descriptor for category file */
   int   fdinf;            /* File descriptor for information file */
   } ;

/* The UTVSPEC structure contains information relating to one
*  selector specification.  Note that pstbl is actually a six-
*  dimensional array, with the randomly selected dimensions
*  (nstr) slow-moving, and the systematically selected dimensions
*  (nsts) fast-moving.  */

struct UTVSPEC {
   struct UTVSPEC *pnss;      /* Ptr to next selector spec */
   struct UTVINFO *pstbl;     /* Ptr to list of available images */
   ui32  nstr;                /* First (random) pstbl dimension */
   ui32  nsts;                /* Second (systematic) pstbl dim. */
   ui32  nstbl;               /* No. entries in pstbl (= nstr*nsts) */
   ui32  prob;                /* Probability of this spec */
   si32  seed;                /* Random number seed */
   si32  seed0;               /* Original seed for reset */
   int   iso[NSVARS];         /* Input selector order */
   int   mcode;               /* Categorization mode code */
   struct UTVSEL cs[NSVARS];  /* Category selectors as read */
   } ;

/* The UTVSTATE structure contains information that is derived by
*  the tvinitfn startup process and passed to the tvgrabfn calls.
*  It preserves the state of the open input files and the property
*  selectors between grab calls.  */

struct UTVSTATE {
   struct UTVSPEC *pfss;      /* Ptr to first selector spec */
   struct UTVSPEC *pcss;      /* Ptr to current selector spec */
   struct UTVINFO *pran;      /* Ptr to randomly selected image */
   struct UTVFILE ndf[NNDF];  /* Database file info */
   si32   dfltseed;           /* Default random number seed */
   int    nspec,nprob;        /* Number of specs and probs entered */
   int    ivar;               /* Position in systematic scan */
   } ;

/* The NORBHDR structure can contain a NORB .mat file header */

struct NORBHDR {
   ui32  magic;
#define CAT_MAGIC 0x1E3D4C54  /* Magic number for integer matrix */
#define DAT_MAGIC 0x1E3D4C55  /* Magic number for byte matrix */
   ui32  ndim;
   ui32  dim[4];
   } ;

/* GLOBAL:  Maximum values of the various types of selectors */

static const int mxvars[NSVARS] = {
   MX_CAM, MX_LGT, MX_CAT, MX_INST, MX_ELV, MX_AZI };

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
*     smax           Maximum allowed value for a selector.             *
* Return value:  Number of selectors found, or -1 if syntax error.     *
*=====================================================================*/

static int norbpsel(const char **ppsel, struct UTVSEL *pcs, int smax) {

   const char  *ps = *ppsel;  /* Ptr used to scan the input */
   byte  *svals = pcs->sv;    /* Locate results array */
   int   ib,ie;               /* Beginning, end of a range */
   int   rc = 0;              /* Return code */
#define PSERR -1              /* Return code for parse error */

   if (*ps++ != '(') return PSERR;
   if (pcs->nvar > 0) return 18;

   /* Remove defaults */
   for (ib=0; ib<=smax; ++ib)
      svals[ib] = KILLIMG;

   ib = 0, ie = -1;
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
   return PSERR;
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
*                              norbr1im                                *
*                                                                      *
*  Read an image, taking into account sub-windowing.  Assumes          *
*  suitable constants have been set up in UTVSTATE.  Arguments:        *
*     putv           Ptr to UTVDEF.                                    *
*     pimi           Ptr to UTVINFO for the requested image.           *
*     pimage         Ptr to image.                                     *
*  Return value:                                                       *
*     0              Success.                                          *
*     6              Read error or (end-of-file not at start).         *
*     13             End-of-file at start of image.                    *
*     14             Error doing a skip on the image file.             *
*=====================================================================*/

static int norbr1im(struct UTVDEF *putv, struct UTVINFO *pimi,
      PIXEL *pimage) {

   struct UTVSTATE *pst = (struct UTVSTATE *)putv->ptvsrc;
   struct UTVFILE *pdf;                /* Ptr to data file info */
   off_t limgo;                        /* Local image offset */
   ui32 lrem;                          /* Length remaining */
   int  irow, nrow = (int)putv->tvy;
   int  idf = pimi->imdf;              /* Database file number */
   int  fdi;                           /* Image file descriptor */

   pdf = pst->ndf + idf;
   fdi = pdf->fdimg;                   /* Image file descriptor */
   limgo = (off_t)pimi->imgno * pdf->limg + pdf->lskp1 +
      sizeof(struct NORBHDR);
   if (lseek(fdi, limgo, SEEK_SET) < 0) return 14;
   for (irow=0; irow<nrow; ++irow) {
      if (irow > 0 && pdf->lskp2 &&
         lseek(fdi, pdf->lskp2, SEEK_CUR) < 0) return 14;
      lrem = pdf->l1rd;
      while (lrem > 0) {
         si32 lr = (si32)read(fdi, pimage, lrem);
         if (lr == 0 && irow == 0) return 13;
         if (lr <= 0) return 6;
         pimage += lr;
         lrem -= lr;
         }
      }
   return 0;
   } /* End norbr1im() */


/*=====================================================================*
*                              closeimg                                *
*                                                                      *
*  Close either or both image files.  (Caller may ignore error         *
*  return if caller is doing error termination already.)               *
*                                                                      *
*  Argument:                                                           *
*     pst      Ptr to master UTVSTATE struct                           *
*=====================================================================*/

static int closeimg(struct UTVSTATE *pst) {

   int idf;

   for (idf=DF_TRAIN; idf<=DF_TEST; ++idf) {
      struct UTVFILE *pdf = pst->ndf + idf;
      if (pdf->fdimg == 0) continue;
      if (pdf->fdimg < 0 || close(pdf->fdimg) < 0)
         return RC_INCR*idf + 34;
      pdf->fdimg = 0;   /* Prevent doing this twice */
      }

   return 0;
   } /* End closeimg() */


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
      (struct UTVSPEC *)calloc(1, sizeof(struct UTVSPEC));
   if (!pss) return pss;
   pss->pnss = pst->pfss;
   pst->pfss = pss;
   pst->nspec += 1;
   pss->seed0 = pst->dfltseed++;
   return pss;

   } /* End makespec() */


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
      if (pss->pstbl) free(pss->pstbl);
      free(pss);
      }
   pst->pfss = NULL;
   pst->nspec = pst->nprob = 0;

   } /* End freespec() */


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

static int dfltrsel(struct UTVSPEC *pss, int islash, int lastiv) {

   int jj,jv;

   if (islash < 0) islash = lastiv;
   for (jv=0; jv<NSVARS; ++jv) {
      struct UTVSEL *pcs = &pss->cs[jv];
      if (pcs->nvar == 0) {
         if (lastiv >= NSVARS) return 18;
         for (jj=lastiv++; jj>islash; --jj)
            pss->iso[jj] = pss->iso[jj-1];
         pss->iso[islash++] = jv;
         for (jj=0; jj<=mxvars[jv]; ++jj)
            pcs->sv[jj] = jj;
         pss->nstr *= (pcs->nvar = mxvars[jv]);
         }
      } /* End variable type loop */

   if (pss->nstr*pss->nsts > MX_IMGS) return 8;
   return 0;
   } /* End dfltrsel() */


/*=====================================================================*
*                              norbmset                                *
*=====================================================================*/

int norbmset(struct UTVDEF *putv, char *camnm, const char *ufile,
   const char *udata, enum tvinit_kcall kcall) {

   struct UTVSTATE *pst;      /* Ptr to persistent local data */
   struct UTVSPEC *pss;       /* Ptr to current selector spec */
   struct UTVSEL *pcs;        /* Ptr to selector info */
   char *bfname,*fname;       /* Base file name, file name */
   char *prdd;                /* Ptr to data being read */
   struct NORBHDR nhdr;       /* Headers */
   ui32 nel;                  /* Number of eligible images */
   ui32 dfltprob;             /* Default probability */
   ui32 sumprob;              /* Sum of probabilities */
   int  idf;                  /* Data file index */
   int  islash;               /* Position where slash code found */
   int  iv,jv;                /* Variable loop indexes */
   int  lbnm,lfnm;            /* Lengths of base,file names */
   int  ldat;                 /* Length of data being read */
   int  needidf[NNDF];        /* TRUE if need file idf */
   int  rc;                   /* Return code */

   /* Which file each category instance is in */
   static const byte reqidf[MX_INST+1] = {
      DF_TEST, DF_TEST,  DF_TEST,  DF_TEST,  DF_TRAIN,
      DF_TEST, DF_TRAIN, DF_TRAIN, DF_TRAIN, DF_TRAIN  };

   /* The base parts of the NORB file names */
   static const char *ndsfnm[NNDF] = {
      "smallnorb-5x46789x9x18x6x2x96x96-training",
      "smallnorb-5x01235x9x18x6x2x96x96-testing" };
   static char mtpath[] = "./";

/* Clear pointers to avoid free() errors */

   pst = NULL;
   bfname = NULL;

/* Switch according to type of call */

   /* Items that need to be cleared on INIT or RESTART call */
   needidf[DF_TRAIN] = needidf[DF_TEST] = FALSE;
   sumprob = 0;
   switch (kcall) {

/* Close-out call */

   case TV_CLOSE:
      pst = (struct UTVSTATE *)putv->ptvsrc;
      if (udata != NULL || pst == NULL) return 12;
      rc = closeimg(pst);
      goto SetupErrorExit;

/* Restart call */

   case TV_RESTART:
      pst = (struct UTVSTATE *)putv->ptvsrc;
      if (!pst) return 12;
      rc = closeimg(pst);
      if (rc) goto SetupErrorExit;
      freespec(pst);
      pst->ivar = 0;
      break;

/* Reset call */

   case TV_RESET:
      pst = (struct UTVSTATE *)putv->ptvsrc;
      if (!pst) return 12;
      for (pss=pst->pfss; pss; pss=pss->pnss)
         pss->seed = pss->seed0;
      pst->ivar = 0;
      return 0;

/* Initialization call */

   case TV_INIT:

      /* Sanity checks */
      if (putv->tvsa != 1 || putv->tvta != 1) return 10;
      if (putv->tvkicol != TVC_GS) return 11;
      /* Allocate a UTVSTATE block */
      pst = (struct UTVSTATE *)calloc(1, sizeof(struct UTVSTATE));
      if (!pst) return 9;
      pst->dfltseed = DFLT_SEED;
      break;
      } /* End kcall switch */

/* Start of a new spec (first or semicolon) will enter here.
*  Create a selector spec and initialize it.  */

StartSpecHere:
   pss = makespec(pst);
   if (!pss) return 9;

/* Parse the selectors first so files are not opened if there is
*  an error.  */

   if (*udata == '/')
      islash = 0, udata += 1;
   else
      islash = -1;
   iv = 0;
   pss->nstr = pss->nsts = 1;
   while (*udata) {
      /* One of the documented letter codes is expected here */
      switch (toupper(*udata++)) {
      case 'V':                  /* Camera selectors */
         nel = norbpsel(&udata, &pss->cs[KV_View], MX_CAM);
         if (nel == PSERR) { rc = 1; goto SetupErrorExit; }
         if (islash >= 0) pss->nsts *= nel;
         else             pss->nstr *= nel;
         pss->iso[iv++] = KV_View;
         break;
      case 'L':                  /* Lighting selectors */
         nel = norbpsel(&udata, &pss->cs[KV_Light], MX_LGT);
         if (nel == PSERR) { rc = 2; goto SetupErrorExit; }
         if (islash >= 0) pss->nsts *= nel;
         else             pss->nstr *= nel;
         pss->iso[iv++] = KV_Light;
         break;
      case 'C':                  /* Category selectors */
         nel = norbpsel(&udata, &pss->cs[KV_Cat], MX_CAT);
         if (nel == PSERR) { rc = 3; goto SetupErrorExit; }
         if (islash >= 0) pss->nsts *= nel;
         else             pss->nstr *= nel;
         pss->iso[iv++] = KV_Cat;
         break;
      case 'I':                  /* Instance selectors */
         nel = norbpsel(&udata, &pss->cs[KV_Inst], MX_INST);
         if (nel == PSERR) { rc = 4; goto SetupErrorExit; }
         if (islash >= 0) pss->nsts *= nel;
         else             pss->nstr *= nel;
         pss->iso[iv++] = KV_Inst;
         break;
      case 'E':                  /* Elevation selectors */
         nel = norbpsel(&udata, &pss->cs[KV_Elev], MX_ELV);
         if (nel == PSERR) { rc = 5; goto SetupErrorExit; }
         if (islash >= 0) pss->nsts *= nel;
         else             pss->nstr *= nel;
         pss->iso[iv++] = KV_Elev;
         break;
      case 'A':                  /* Azimuth selectors */
         nel = norbpsel(&udata, &pss->cs[KV_Azim], MX_AZI);
         if (nel == PSERR) { rc = 6; goto SetupErrorExit; }
         if (islash >= 0) pss->nsts *= nel;
         else             pss->nstr *= nel;
         pss->iso[iv++] = KV_Azim;
         break;
      case 'M': {                /* Categorization mode */
         int md = (int)(*udata++);
         pss->mcode = isdigit(md) ? md - '0' : -1;
         if (pss->mcode < 0 || pss->mcode > MX_MCODE)
            { rc = 13; goto SetupErrorExit; }
         } /* End 'M' local scope */
         break;
      case 'P': {                /* Probability */
         ui32 tprob = 0, tdec = 1;
         int  ndig = 0;
         if (*udata++ != '(')
            { rc = 17; goto SetupErrorExit; }
         if (pss->prob == 0) pst->nprob += 1;
         while (*udata && *udata != ')') {
            int md = (int)(*udata++);
            if (md == '.') tdec = 1;
            else if (isdigit(md)) {
               tprob = 10*tprob + (md - '0');
               tdec *= 10;
               ndig += 1; }
            else { rc = 17; goto SetupErrorExit; }
            }
         if (ndig >= UI32_SIZE)
            { rc = 17; goto SetupErrorExit; }
         /* Because we don't want to load ROCKS routines here,
         *  we use 64-bit if available, otherwise double float */
#ifdef HAS_I64
         {  ui64 tnum = ((ui64)tprob << 31)/(ui64)tdec;
            if (tnum > MX_PROB)
               { rc = 17; goto SetupErrorExit; }
            pss->prob = (ui32)tnum;
            } /* End tnum local scope */
#else
         {  double dmxp = (double)MX_PROB;
            double tnum = dmxp*(double)tprob/(double)tdec;
            if (tnum > dmxp)
               { rc = 17; goto SetupErrorExit; }
            pss->prob = (ui32)tnum;
            } /* End tnum local scope */
#endif
         sumprob += pss->prob;
         if (sumprob > (MX_PROB+MX_PROBERR))
            { rc = 16; goto SetupErrorExit; }
         udata++;
         } /* End 'P' local scope */
         break;
      case 'S': {                /* Random number seed */
         si32 tseed = 0;
         if (*udata++ != '(')
            { rc = 14; goto SetupErrorExit; }
         while (*udata && *udata != ')') {
            int md = (int)(*udata++);
            if (isdigit(md))
               tseed = 10*tseed + (md - '0');
            else { rc = 14; goto SetupErrorExit; }
            }
         if (tseed <= 0 || tseed == SI32_MAX)
            { rc = 14; goto SetupErrorExit; }
         pss->seed0 = tseed;
         udata++;
         } /* End 'S' local scope */
         break;
      case '/':                  /* Systematic/random separator */
         if (islash >= 0)
            { rc = 9; goto SetupErrorExit; }
         islash = iv;
         break;
      case ';':                  /* New selection block */
         rc = dfltrsel(pss, islash, iv);
         if (rc) goto SetupErrorExit;
         goto StartSpecHere;
      default:                   /* None of the above: error */
         rc = 7; goto SetupErrorExit;
         } /* End code switch */
      if (*udata == ',') ++udata;   /* Skip over comma */
      } /* End parsing loop */
   rc = dfltrsel(pss, islash, iv);  /* Insert default selectors */
   if (rc) goto SetupErrorExit;

/* Determine fraction of unit probability not assigned by user.
*  Divide this by number of selection blocks that did not have
*  a probability assigned to yield the default probability.  */

   if (pst->nprob >= pst->nspec) {
      /* Note that sumprob > (MX_PROB+MX_PROBERR) would already
      *  have been detected as an error earlier, so here only
      *  need to check the opposite case.  */
      if (sumprob < (MX_PROB-MX_PROBERR))
         { rc = 16; goto SetupErrorExit; }
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
      pss->pstbl =
         (struct UTVINFO *)calloc(pss->nstbl, sizeof(struct UTVINFO));
      if (!pss->pstbl) {
         rc = 19; goto SetupErrorExit; }

/* Calculate offsets into eligibility table by variable type */

      nel = 1;
      for (jv=NSVARS-1; jv>=0; --jv) {
         pcs = &pss->cs[pss->iso[jv]];
         pcs->offmult = nel;
         nel *= pcs->nvar;
         }
      } /* End loop over selection blocks */

/* Make space to construct the three (or six) file names.
*  (As documented, file names are not preserved across restarts,
*  therefore, this is local storage.  Either or both of training
*  and test sets are accessed only if called for by the selectors.)
*/

   if (!(ufile && ufile[0])) ufile = mtpath;
   prdd = strrchr(ufile, '/');
   lbnm = prdd ? prdd - ufile : strlen(ufile);
   lfnm = lbnm + strlen(ndsfnm[DF_TRAIN]) + 12;
   bfname = malloc(lfnm << 1);
   if (!bfname) {
      rc = 19; goto SetupErrorExit; }
   fname = bfname + lfnm;
   memcpy(bfname, ufile, lbnm);
   bfname[lbnm] = '/';

/* Open data files and check headers */

   for (idf=DF_TRAIN; idf<=DF_TEST; ++idf) {

      struct UTVFILE *pdf;       /* Ptr to data file info */
      int irc;                   /* Return code file increment */

      if (!needidf[idf]) continue;

      irc = RC_INCR*idf;
      strcpy(bfname+lbnm+1, ndsfnm[idf]);
      strcpy(fname, bfname);
      strcat(fname, "-dat.mat");

      pdf = pst->ndf + idf;
      pdf->fdimg = open(fname, O_RDONLY);
      if (pdf->fdimg < 0) {
         rc = irc+31; goto SetupErrorExit; }
      ldat = sizeof(struct NORBHDR);
      prdd = (char *)&nhdr;
      while (ldat > 0) {
         int lr = (int)read(pdf->fdimg, prdd, ldat);
         if (lr <= 0) {
            rc = irc+32; goto SetupErrorExit; }
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
            nhdr.dim[0] != MX_IMGS || nhdr.dim[1] != 2) {
         rc = irc+33; goto SetupErrorExit; }

      /* Set up variables needed for reading subimages (d3grp1 will
      *  check later that offsets + tvx,tvy do not exceed raw image
      *  size).  These images are known to be grayscale, so pixel
      *  size is implicitly assumed to be 1 byte.  */
      putv->tvix = nhdr.dim[2], putv->tviy = nhdr.dim[3];
      pdf->limg = putv->tvix * putv->tviy;
      if (putv->tvx == putv->tvix && putv->tvy == putv->tviy) {
         pdf->l1rd = pdf->limg;
         pdf->lskp1 = pdf->lskp2 = 0;
         }
      else {
         pdf->l1rd = putv->tvx;
         pdf->lskp1 = putv->tvyo*putv->tvix + putv->tvxo;
         pdf->lskp2 = putv->tvix - putv->tvx;
         }

   /* Open category file and check header */

      strcpy(fname, bfname);
      strcat(fname, "-cat.mat");
      pdf->fdcat = open(fname, O_RDONLY);
      if (pdf->fdcat < 0) {
         rc = irc+41; goto SetupErrorExit; }
      ldat = sizeof(struct NORBHDR) - sizeof(ui32);
      prdd = (char *)&nhdr;
      while (ldat > 0) {
         int lr = (int)read(pdf->fdcat, prdd, ldat);
         if (lr <= 0) {
            rc = irc+42; goto SetupErrorExit; }
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
            nhdr.dim[2] != 1) {
         rc = irc+43; goto SetupErrorExit; }

/* Open info file and check header */

      strcpy(fname, bfname);
      strcat(fname, "-info.mat");
      pdf->fdinf = open(fname, O_RDONLY);
      if (pdf->fdinf < 0) {
         rc = irc+51; goto SetupErrorExit; }
      ldat = sizeof(struct NORBHDR) - sizeof(ui32);
      prdd = (char *)&nhdr;
      while (ldat > 0) {
         int lr = (int)read(pdf->fdinf, prdd, ldat);
         if (lr <= 0) {
            rc = irc+52; goto SetupErrorExit; }
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
            nhdr.dim[2] != 1) {
         rc = irc+53; goto SetupErrorExit; }

/* Now read through the entire cat and info files and for each image
*  that matches one of the specified selectors in any selector block,
*  just enter its number into the table for that block.  Then norbmix
*  can use a random number to select a table and an entry in this
*  table and read that image.  This way, there is no need to check
*  selection each time a udev is generated.  */

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
               rc = 7; goto SetupErrorExit; }
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
               rc = 8; goto SetupErrorExit; }
            prdd += lr, ldat -= lr;
            }
#if BYTE_ORDRE > 0
         norbswp4(&iinf.iinst);
         norbswp4(&iinf.ielev);
         norbswp4(&iinf.iazim);
         norbswp4(&iinf.light);
#endif

/* Sanity checks */

         if (iinf.light < 0 || iinf.light > MX_LGT) {
            rc = 22; goto SetupErrorExit; }
         if (icat < 0 || icat > MX_CAT) {
            rc = 23; goto SetupErrorExit; }
         if (iinf.iinst < 0 || iinf.iinst > MX_INST) {
            rc = 24; goto SetupErrorExit; }
         if (iinf.ielev < 0 || iinf.ielev > MX_ELV) {
            rc = 25; goto SetupErrorExit; }
         if (iinf.iazim < 0 || iinf.iazim & 1 ||
               iinf.iazim > 2*MX_AZI) {
            rc = 26; goto SetupErrorExit; }
         iaz = iinf.iazim >> 1;

/* Check against stored selectors */

         for (pss=pst->pfss; pss; pss=pss->pnss) {
            ui32 iel,jel;              /* Location in pstbl */
            int  jgn,jsn;              /* Temps for ign,isn */

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

/* Found an acceptable image -- store index and categorization
*  according to mcode in table for one or two cameras.  */

            /* Just-in-case check that should never trigger */
            if (iel >= pss->nstbl) {
               rc = 15; goto SetupErrorExit; }

            switch (pss->mcode) {
            case 0:                    /* Default */
               jgn = icat + 1;
               jsn = 10*icat + iinf.iinst + 1;
               break;
            case 1: {                  /* Rotational invariance tester */
               /* Note that this mcode allows three rotational categories
               *  for each object, but adds iazim%5.  It was assumed that
               *  only three azimuths would be selected in the udata.  */
               int ib = 15*icat + 3*iinf.iinst + iaz/5;
               jgn = ib + 1;
               jsn = 3*ib + iaz%5 + 1;
               } /* End case 1 local scope */
               break;
            case 2: {                  /* 7/20/10 rot. inv. tester */
               int ia = iaz % 6 > 2;
               jgn = 2*icat + ia + 1;
               jsn = 20*icat + 2*iinf.iinst + ia + 1;
               } /* End case 2 local scope */
               break;
               /* Other cases can be added here as needed */
               } /* End mcode switch */

            /* Now make entries for either or both cameras */
            pcs = &pss->cs[KV_View];
            for (jv=0; jv<=MX_CAM; ++jv) {
               struct UTVINFO *pimi;   /* Ptr to current image info */
               jel = pcs->sv[jv];
               if (jel == KILLIMG) continue;
               pimi = pss->pstbl + iel + jel*pcs->offmult;
               pimi->imdf = idf;
               pimi->imgno = nel + nel + jv;
               pimi->isn = jsn;
               pimi->ign = jgn;
               }

            } /* End pss loop */
         } /* End nel loop */

/* Now just check that the two files reached end-of-file */

      if ((int)read(pdf->fdcat, &rc, 1) != 0) {
         rc = 20; goto SetupErrorExit; }
      if ((int)read(pdf->fdinf, &rc, 1) != 0) {
         rc = 20; goto SetupErrorExit; }
      if (close(pdf->fdcat) < 0) {
         rc = irc+44; goto SetupErrorExit; }
      pdf->fdcat = 0;   /* JIC */
      if (close(pdf->fdinf) < 0) {
         rc = irc+54; goto SetupErrorExit; }
      pdf->fdinf = 0;   /* JIC */

      } /* End loop over the two database sections */

/* All's well--link the UTVSTATE to the UTVDEF and return 0 */

   if (bfname) free(bfname);
   putv->ptvsrc = pst;
   return 0;

/* Here on all read errors during setup */

SetupErrorExit:
   if (bfname) free(bfname);
   if (pst) { freespec(pst); free(pst); }
   return rc;

   } /* End norbmset() */


/*=====================================================================*
*                               norbmix                                *
*=====================================================================*/

int norbmix(struct UTVDEF *putv, PIXEL *pimage) {

   struct UTVSTATE *pst = (struct UTVSTATE *)putv->ptvsrc;
   struct UTVSPEC *pss;
   struct UTVINFO *pimi;
   int    rc;                 /* Return code */

   if (!pst || !pst->pfss) return 12;

/* Read image.  If ivar is 0, it is time to start scanning at a
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
   pimi = pst->pran + pst->ivar;
   /* Advance ivar for next time */
   pst->ivar += 1;
   if (pst->ivar >= pss->nsts) pst->ivar = 0;
   rc = norbr1im(putv, pimi, pimage);
   if (rc) goto ReadImageError;

/* Return category and instance numbers to caller */

   putv->isn = pimi->isn;
   putv->ign = pimi->ign;

/* All is well, return to caller with code 0 */

   return 0;

/* Here on all read image errors to return rc code */

ReadImageError:
   /* Ignore close errors, return original rc */
   closeimg(pst);
   freespec(pst);
   free(pst);
   return rc;
   } /* End norbmix() */
