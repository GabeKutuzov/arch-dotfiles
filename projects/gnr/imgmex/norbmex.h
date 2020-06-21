/* (c) Copyright 2011, The Rockefeller University *11113* */
/* $Id: norbmex.h 1 2012-01-13 20:05:02Z  $ */
/***********************************************************************
*                           norbmex header                             *
************************************************************************
*  This parameter block contains internal information placed in the    *
*  NorbMexComm global vector and shared between the routines in the    *
*  norbmex package.  This package reads images from the NORB dataset.  *
************************************************************************
*  V1A, 03/09/11, GNR - New header file                                *
*  V1B, 05/06/11, GNR - Add '+' and 'N' codes                          *
*  V2A, 12/01/11, GNR - Return transpose of image                      *
*  ==>, 12/01/11, GNR - Last mod before committing to svn repository   *
***********************************************************************/

typedef byte PIXEL;        /* Pixel type */
#define MAXGRAY  256       /* Highest gray level in an image + 1 */
#define MX_NIXY   96       /* X,Y size of NORB images */

/* Define maximum values for the various selector types */
#define NSVARS     6       /* Number of selector types */
#define MX_CAT     4
#define MX_INST    9
#define MX_AZI    17
#define MX_ELV     8
#define MX_LGT     5
#define MX_CAM     1
#define MX_ALL    18       /* Highest MX_xxx + 1 */
/* Other configuration constants */
#define MX_IMGS   24300    /* Max selectable image pairs/file */
#define TOT_IMGS  97200    /* Total images in both files */
#define MX_PROB ((ui32)(1<<31))  /* Max probability */
#define MX_PROBERR 6       /* Max error in user's prob sum */
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

/* Define indexes for accessing the two database files */
#define NNDF       2       /* Number of NORB data files */
#define DF_TRAIN   0       /* "training" files */
#define DF_TEST    1       /* "testing" files */

/* The NORBHDR structure can contain a NORB .mat file header */

struct NORBHDR {
   ui32  magic;
#define CAT_MAGIC 0x1E3D4C54  /* Magic number for integer matrix */
#define DAT_MAGIC 0x1E3D4C55  /* Magic number for byte matrix */
   ui32  ndim;
   ui32  dim[4];
   } ;

/* The UTVINFO structure contains the number of an eligible image
*  and its stimulus identifier data, which are stored during
*  startup and returned to the caller at norbmexget time.  */

struct UTVINFO {
   ui16  imdf;             /* Image data file (DF_TRAIN or DF_TEST) */
   ui16  imgno;            /* Image number (for fdimg lseek) */
#if MX_IMGS > UI16_MAX
#error Need larger variable than ui16 to contain image number
#endif
   byte isid[NSVARS];      /* Stimulus id info */
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
   PIXEL *ptrdl;           /* Ptr to temp read line */
   ui32  limg;             /* Length of one image record--
                           *  0 if this file not being used */
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
   ui32  nbatch;              /* Batch selection number */
   ui32  prob;                /* Probability of this spec */
   si32  seed;                /* Random number seed */
   si32  seed0;               /* Original seed for reset */
   int   kbatch;              /* TRUE for systematic batch,
                              *  FALSE for random (default) */
   int   iso[NSVARS];         /* Input selector order */
   struct UTVSEL cs[NSVARS];  /* Category selectors as read */
   } ;

/* The UTVSTATE structure contains information that is derived by
*  the norbmexset startup process and passed to the other calls as
*  a matlab global vector.  It preserves the state of the open input
*  files and the property selectors between norbmexget calls.  */

struct UTVSTATE {
   struct UTVSPEC *pfss;      /* Ptr to first selector spec */
   struct UTVSPEC *pcss;      /* Ptr to current selector spec */
   struct UTVINFO *pran;      /* Ptr to randomly selected image */
   struct UTVFILE ndf[NNDF];  /* Database file info */
   char   *bfname,*fname;     /* Base, full file names */
   ui32   sigck;              /* Signature check */
#define NM_SIGCK 0x4E4F5242      /* Required sigck value */
   si32   dfltseed;           /* Default random number seed */
   int    ivar;               /* Position in systematic scan */
   int    lbnm,lfnm;          /* Lengths of base,file names */
   int    nspec,nprob;        /* Number of specs and probs entered */
   ui16   tvx,tvy;            /* Size of image after sampling */
   ui16   tvxo,tvyo;          /* x,y offsets of image sample */
   } ;

/* Prototype routines that use UTVSTATE struct */
