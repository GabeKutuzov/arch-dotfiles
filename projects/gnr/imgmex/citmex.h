/* (c) Copyright 2011-2012, The Rockefeller University *11113* */
/* $Id: citmex.h 9 2012-11-21 22:00:13Z  $ */
/***********************************************************************
*                           CitMex header                              *
************************************************************************
*  This parameter block contains internal information placed in the    *
*  CitMexComm global vector and shared between the routines in the     *
*  CitMex package.  This package reads images from the CIT 101 dataset.*
************************************************************************
*  V1A, 11/15/11, GNR - New header file                                *
*  ==>, 12/21/11, GNR - Last mod before committing to svn repository   *
*  Rev, 02/02/12, GNR - Add semaphore to prevent multiple free         *
***********************************************************************/

#ifndef CITMEX_HDR_INCLUDED
#define CITMEX_HDR_INCLUDED

#include "mex.h"
#include "sysdef.h"
#include "iterepgf.h"

typedef byte PIXEL;        /* Pixel type */

/* Configuration variables--many specific to CIT101 database */
#define DFLT_SEED 10001    /* Default random number seed */
#define LCATNM     20      /* Length of longest category name ++ */
#define LSTRERR   128      /* Size to allow for strerror_r msgs */
#define MAXGRAY   256      /* Highest gray level in an image + 1 */
#define MX_CAT    101      /* Highest category (from 1) */
#define BCK_CAT   102      /* Code for background category */
#define MX_INST   800      /* Highest instance (from 1) */
#define MX_PBORD  200.0    /* Largest allowed paint border */
#define END_INST  801      /* Code for unknown highest instance */
#define TOT_IMGS 8617      /* Total images in all files */
#define MX_WKSZ  1020      /* Enough space for max selector list
                           *  (increase to allow more duplicates) */
#define NCOLDIMS    3      /* Number dims in a colored image */
#define LPWKINCR 2040      /* Increment in IPgfgWk high-water */
#define NSVARS      6      /* Number of returnable selector types */
#define NCITS       2      /* Number of those in CIT 101 dataset */
#define FSTILOC     7      /* Location of first digit in ImgFNm */
#define FSTALOC    12      /* Location of first digit in AnnFNm */
#define NIMGNUM     4      /* Number of digits in image number */
#define ImgDir "101_ObjectCategories/" /* Dir name for images */
#define ImgFNm "/image_xxxx.jpg"       /* File name for images */
#define BckDir "BACKGROUND_Google"     /* Dir name for background imgs */
#define AnnDir "Annotations/"          /* Dir name for annotations */
#define AnnFNm "/annotation_xxxx.mat"  /* File name for annotations */
#define MX_PROB ((ui32)(1<<31))        /* Max probability */
#define MX_PROBERR  6      /* Max error in user's prob sum */

/* Definitions for communications control semaphore */
#define CMSemNm "CitMexCommCtrl"       /* Global semaphore name */
#ifdef _ISOC99_SOURCE
#define URW_Mode S_IRUSR|S_IWUSR
#else
#define URW_Mode S_IREAD|S_IWRITE
#endif

/* Define struct to hold an image category and instance
*  for comma-separated selection lists.  */
#if MX_CAT >= SHRT_MAX || MX_INST >= SHRT_MAX
#error CISpec is too small to hold category-instance info
#endif
typedef struct CISpec_t {
   si16 icat;              /* Category */
   si16 imgno;             /* Image number */
   } CISpec;

/* Define struct to hold information for one category
*  for two-tiered selection lists.  */
typedef struct TTSpec_t {
   si16 icat;              /* Category */
   ui16 limg;              /* Length of image list */
   ui32 oimg;              /* Offset to start of image list */
   } TTSpec;

/* Define order of control index variables */
enum KVar {
   KV_Cat=0,               /* Category */
   KV_Inst,                /* Instance */
   KV_Azim,                /* Azimuth (NORB only) */
   KV_Elev,                /* Elevation (NORB only) */
   KV_Light,               /* Lighting (NORB only) */
   KV_View,                /* View camera (NORB only) */
   };

/* The C101CATN structure is used to build an index of category
*  names.  N.B.  Do not subtract 1 to access pncn for nth cat. */

typedef struct C101CATN {
   struct C101CATN *pnca;  /* Ptr to next name (after sort) */
   struct C101CATN *pncn;  /* Ptr to nth category name */
   si16 icat;              /* Serial number of this category */
   ui16 nimgs;             /* Number of images in this category */
   char lcname[LCATNM];    /* Lower-case category name for sort */
   char cname[LCATNM];     /* Original category name (path name) */
   } catnm;

/* The C101SPEC structure contains information relating to one
*  selector specification.  */

typedef struct C101SPEC {
   struct C101SPEC *pnss;     /* Ptr to next selector spec */
   union {
      CISpec *pcis;              /* Ptr to cat-img selector list */
      TTSpec *ptts;              /* Ptr to two-tiered list */
      } u1;
   si16  *pcat;               /* Ptr to parsed category list */
   si16  *pimg;               /* Ptr to parsed image list */
   ui32  nbatch;              /* Batch size */
   ui32  icseq;               /* Position in category sequence */
   ui32  nu1;                 /* Number of entries in u1 */
   ui32  prob;                /* Probability of this spec */
   si32  seed;                /* Random number seed */
   si32  seed0;               /* Original seed for reset */
   int   ksel;                /* Selection controls */
#define KS_CSEQ   0x0001         /* Categories in sequence */
#define KS_CPERM  0x0002         /* Categories in permutation */
#define KS_ISEQ   0x0004         /* Instances in sequence */
#define KS_IPERM  0x0008         /* Instances in permutation */
#define KS_CISL   0x0010         /* u1 is a CISpec list */
#define KS_TTSL   0x0020         /* u1 is a TTSpec list */
#define KS_CLAST  0x0040         /* Category list read last */
#define KS_ILAST  0x0080         /* Instance list read last */
#define KS_CISH        8         /* Shift from CSEQ to SLBC bits */
#define KS_SLBC   0x0100         /* Slash before cat list */
#define KS_ASBC   0x0200         /* Asterisk before cat list */
#define KS_SLBI   0x0400         /* Slash before img list */
#define KS_ASBI   0x0800         /* Asterisk before img list */
#define KS_PLBC   0x1000         /* Plus sign before cat list */
#define KS_PLBI   0x2000         /* Plus sign before img list */
   } sspec;

/* The C101STATE structure contains information that is derived by
*  the citmexset startup process and passed to the other calls as
*  a Matlab global vector.  It preserves the state of the property
*  property selectors between citmexget calls.  */

struct C101STATE {
   char  *bname;              /* User-supplied base dirname */
   char  *fname;              /* Space for image file paths */
   char  *aname;              /* Space for annotation file paths */
   catnm *pcin0;              /* Ptr to category index allocation */
   catnm *pcins;              /* Ptr to category index (sorted) */
   catnm **ppad;              /* Ptr to alphabetic category index */
   sspec *pfss;               /* Ptr to first selector spec */
   sspec *pcss;               /* Ptr to current selector spec */
   byte  *pitwk;              /* Ptr to high-water iter work area */
   mwSize litwk;              /* Current size of pitwk array */
   float pbord;               /* Paint border */
   ui32  nwvtx;               /* Number of vertices in pitwk */
   ui32  sigck;               /* Signature check */
#define C101_SIGCK 0x43313031 /* Required sigck value */
   ui32  cmflgs;              /* Program flags */
#define cmCOLORED  0x01          /* Accept colored image data */
#define cmCROP     0x02          /* Crop images to annotated size */
#define cmMASK     0x04          /* Mask pixels outside contour to 0 */
#define cmDEFMSK   0x08          /* Defer masking to citmexpbg call */
#define cmEPMODE   0x70          /* Bits sent to iterepgf mode */
#define cmNDMODE   0x80          /* Use cmEPMODE bits, not default */
   si32  dfltseed;            /* Default random number seed */
   ui32  ivar;                /* Position in systematic scan */
   int   lbnm;                /* Length of base path name */
   int   nspec,nprob;         /* Number of specs and probs entered */
   ui16  ncats;               /* Number of categories */
   ui16  nimgs;               /* Number entries in pimg array */
   ui16  tvx,tvy;             /* Size of image after sampling */
   ui16  tbx,tby;             /* Crop border size along x,y */
   } ;

#endif   /* !defined CITMEX_HDR_INCLUDED */

