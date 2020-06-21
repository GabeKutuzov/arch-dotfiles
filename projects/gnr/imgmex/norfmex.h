/* (c) Copyright 2011, The Rockefeller University *11113* */
/* $Id: norfmex.h 1 2012-01-13 20:05:02Z  $ */
/***********************************************************************
*                           norfmex header                             *
************************************************************************
*  This parameter block contains internal information placed in the    *
*  NorfMexComm global vector and shared between the routines in the    *
*  norfmex package.  This package reads a feature response file.       *
************************************************************************
*  V1A, 04/15/11, GNR - New header file, based on norbmex.h            *
*  V1B, 04/19/11, GNR - Add byte swapping                              *
*  V1C, 05/02/11, GNR - Add '+' and 'N' codes                          *
*  ==>, 10/29/11, GNR - Last mod before committing to svn repository   *
***********************************************************************/

/* Define maximum values for the various selector types.  These are
*  not necessarily the max values in the original NORB dataset, but
*  the maxima that were stored in the feature response file.  */
#define NSVARS     6       /* Number of selector types */
#define MX_CAT     4
#define MX_INST    9
#define MX_AZI    17
#define MX_ELV     8
#define MX_LGT     5
#define MX_CAM     0
#define MX_ALL    18       /* Highest MX_xxx + 1 */
/* Other configuration constants */
#define TOT_IMGS  48600    /* Total images in response file */
#define MX_FEATS   4075    /* Max size of nfeats arg */
#define MX_PROB ((ui32)(1<<31))  /* Max probability */
#define MX_PROBERR    6    /* Max error in user's prob sum */
#define DFLT_SEED 10001    /* Default random number seed */
#define KILLIMG 0xff       /* sv value to kill image */

/* Define order of control index variables */
enum KVar {
   KV_Cat=0,               /* Category */
   KV_Inst,                /* Instance */
   KV_Azim,                /* Azimuth */
   KV_Elev,                /* Elevation */
   KV_Light,               /* Lighting */
   KV_View,                /* View camera */
   };

/* The NFMINFO structure contains the number of an eligible image
*  and its stimulus identifier data, which are stored during
*  startup and returned to the caller at norfmexget time.  */

struct NFMINFO {
   ui32 imgno;             /* Image number (for fdff lseek) */
   byte isid[NSVARS];      /* Stimulus id info */
   } ;

/* The NFMSEL structure contains information for making selections
*  over one image variable.  */

struct NFMSEL {
   off_t offmult;          /* Offset multiplier for this variable */
   int   nvar;             /* Number of selectors found */
   byte  sv[MX_ALL];       /* Space for selectors */
   } ;

/* The NFMSPEC structure contains information relating to one
*  selector specification.  Note that pstbl is actually a six-
*  dimensional array, with the randomly selected dimensions
*  (nstr) slow-moving, and the systematically selected dimensions
*  (nsts) fast-moving.  */

struct NFMSPEC {
   struct NFMSPEC *pnss;      /* Ptr to next selector spec */
   struct NFMINFO *pstbl;     /* Ptr to list of available images */
   ui32  nstr;                /* First (random) pstbl dimension */
   ui32  nsts;                /* Second (systematic) pstbl dim. */
   ui32  nstbl;               /* No. entries in pstbl (= nstr*nsts) */
   ui32  nbatch;              /* Batch selection number */
   ui32  prob;                /* Probability of this spec */
   si32  seed;                /* Random number seed */
   si32  seed0;               /* Original seed for reset */
   int   kbatch;              /* TRUE for systematic batch,
                              *  FALSE for random (default) */
   int   iso[NSVARS];         /* Input selector order */
   struct NFMSEL cs[NSVARS];  /* Category selectors as read */
   } ;

/* The NFMSTATE structure contains information that is derived by
*  the norfmexset startup process and passed to the other calls as
*  a matlab global vector.  It preserves the state of the open
*  input file and the property selectors between norfmexget calls.  */

struct NFMSTATE {
   struct NFMSPEC *pfss;      /* Ptr to first selector spec */
   struct NFMSPEC *pcss;      /* Ptr to current selector spec */
   struct NFMINFO *pran;      /* Ptr to randomly selected image */
   char   *fname;             /* Feature response file name */
   ui32   nfeat;              /* Number of features to be read */
   ui32   sigck;              /* Signature check */
#define NM_SIGCK 0x4E4F5246   /* Required sigck value */
   si32   dfltseed;           /* Default random number seed */
   int    fdff;               /* File descriptor for feature file */
   int    ivar;               /* Position in systematic scan */
   int    nspec,nprob;        /* Number of specs and probs entered */
   } ;

