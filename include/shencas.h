/* (c) Copyright 2013, The Rockefeller University *11114* */
/* $Id: shencas.h 59 2017-01-13 20:34:16Z  $ */
/***********************************************************************
*                              shencas.h                               *
*                                                                      *
*  This header defines an interface to a package of routines that      *
*  performs Shen-Castan edge detection on grayscale (8 bits/pixel)     *
*  images.  The package is organized to save persistent data in a      *
*  structure that is returned to the caller of the initialization      *
*  routine and from there passed to the other routines, allowing       *
*  high-water-mark allocation of work arrays.  This minimizes OS       *
*  calls and is useful for implementation as Matlab mex functions.     *
*                                                                      *
*  Internal and public definitions are included in this header--the    *
*  user should avoid changing any internals except via the defined     *
*  interface calls.                                                    *
*                                                                      *
*  The routines were developed from a set of edge-detection codes      *
*  downloaded from CodeForge.com on 02/07/13.  These routines have     *
*  no indication of author and no licensing statements, but were       *
*  described on the web site as "open source".  The unknown author     *
*  is hereby acknowledged.                                             *
*                                                                      *
*  N.B.  All coordinates are in rasters, assumed to be equal in x,y.   *
*                                                                      *
*  The makefile should define "I_AM_MATLAB" if compiled for a MATLAB   *
*  mex-file environment, otherwise CRK library routines will be used.  *
************************************************************************
*  V1A, 02/07/13, GNR - New routines                                   *
*  ==>, 03/07/13, GNR - Last date before committing to svn repository  *
*  V1B, 03/20/13, GNR - Add ability to save >1 debug image or histo    *
***********************************************************************/

#ifndef SHENCAS_HDR_INCLUDED
#define SHENCAS_HDR_INCLUDED

#include "sysdef.h"
#include "rksubs.h"

/* Configuration parameters */
#define  BIGGRAD       1.0E8  /* Very large gradient */
#define  BINADJ         0.01  /* Bin scale adjust */
#define  eis              20  /* Scale shift for eidf */
#define  Seis        1048576  /* 2**eis */
#define  FSeis     1048576.0  /* (float)(2**eis) */
#define  DFLT_EI      262144  /* Default eidf = 0.25 (Seis) */
#define  DFLT_HFHI      0.50  /* Default old method high thresh */
#define  DFLT_HFLO      0.30  /* Default old method low thresh */
#define  DFLT_HGTF      0.50  /* Default high grad thresh frac */
#define  DFLT_NH         160  /* Default histogram size */
#define  DFLT_OL          25  /* Default outline */
#define  DFLT_WS           5  /* Default winsz */
#define  EDGEVAL         120  /* Value to mark an edge in output */
#define  EDGEVLO          60  /* Edges < wkhthi if SCB_LEVL */
#define  HISTSZ          256  /* Max allowed nhist (pre-alloc) */
#define  IMGINC         4096  /* Incremental image size */
#define  IMGSYS           16  /* Bytes to leave for system */
#define  NUMBIM            2  /* Number of byte extended images */
#define  NUMFIM            4  /* Number of floating extended images */
#define  OUTEXT            2  /* Extension within outline (all sides) */
#define  SCCKWD   0x53434348  /* Value for verifying scckwd */
#define  TRAKSZ          256  /* Size of trace buffer */

/* Unchangeable constants */
#define  NNeigh            8  /* Neighbors of a point */

/* ABEND error codes (670-679 reserved for this program) */
#define  ERR_BADPARM 670      /* Bad parameter */
#define  ERR_BADSCC  671      /* Bad SCC passed */
#define  ERR_TRAKEX  672      /* Circular trace buffer exceeded */
#define  ERR_GRADEQ  673      /* All gradient values identical or 0 */
#define  ERR_BIGCNT  674      /* Hysteresis threshold too low */

#define  ERR_MEXSHM  678      /* Shared memory usage error in mex */
#define  ERR_MEXCALL 679      /* Bad arguments in a mex call */

/* Global data shared among the package routines */
typedef struct shencas_t {
   byte  *pei0;            /* Ptr to extended image */
   byte  *pbli0;           /* Ptr to binary Laplacian image */
   float *pfei0;           /* Ptr to float extended image,
                           *  later replaced by gradient image */
   float *psei0;           /* Ptr to smoothed extended image,
                           *  later replaced by output image */
   float *pfiL0;           /* Ptr to left filtered image */
   float *pfiR0;           /* Ptr to right filtered image */
   si32  *phtn0;           /* Ptr to histograms & trace neighbors */
   byte  *pdbg0;           /* Ptr to saved debug images */

   float b;                /* Smoothing factor */
   float hfhi,hflo;        /* High, low hysteresis fractions */
   float hgtf;             /* High gradient threshold fraction */
   float olb,ohb;          /* Low, high thresh bin offsets */

   ui32  eidf;             /* Edge interp damp factor (S20) */
   ui32  scckwd;           /* Word to check for valid SCC */
   si32  outline;          /* Size of image extension */
   si32  winsz;            /* Window size (odd integer) */
   int   nhist;            /* Number of bins in threshold-setting
                           *  histogram */
   int   ndbgh;            /* Number of bins in debug histograms */
   ui16  scops;            /* Option flags */
#define SCB_HTOR   1          /* Toroid horizontally */
#define SCB_VTOR   2          /* Toroid vertically */
#define SCB_HOMZ   4          /* Omit low vals from histogram */
#define SCB_LEVL   8          /* 4-level output trace */
#define SCB_RDBI  16          /* Reduce debug images to input size */
#define SCB_OSCC  32          /* Use original sign correlation */
#define SCB_TDBG  64          /* Threshold debug Matlab print */
#define SCB_OKOP 127          /* OR of allowed scops bits */
   byte  kdbgi;            /* Kinds of debug image to save */
#define SCKD_EXP   1          /* Save expanded raw image */
#define SCKD_SMO   2          /* Save smoothed image */
#define SCKD_GRA   4          /* Save gradient image */
#define SCKD_LAP   8          /* Save Laplacian image */
#define SCKD_MAX  15          /* Highest legal option */
   byte  khist;            /* Kinds of histogram to save --
                           *  uses same codes as kdbgi */
   /* Derived values */
   float grhi,grlo;        /* Highest, lowest values of gradient */
   float hshi,hslo;        /* Highest, lowest values of khist image */
   float hthi,htlo;        /* High, low hysteresis thresholds */
   si32  nsx,nsy;          /* Size of input image */
   si32  nex,ney;          /* Size of extended image */
   ui32  szexi;            /* Size of pei array allocated */
   ui32  szhtn;            /* si32 size of histo & trace array */
   ui32  szdbg;            /* Size of debug images array */
   byte  ndbgi;            /* Number debug images requested */
   byte  nhisa;            /* Number histograms requested */
   byte  idbgi;            /* Index of current debug image */
   byte  ihisa;            /* Index of current histogram */
   } SCC;

/* Interface functions */
SCC  *sheninit(float ab);
void shenedge(SCC *pscc, byte *imgout, byte *imgin, int nsx, int nsy);
void shenclse(SCC *pscc);
/* Functions to modify parameters */
void shensetb(SCC *pscc, float ab);
void shenhfhl(SCC *pscc, float ahfhi, float ahflo);
void shenhgtf(SCC *pscc, float hgtf, float olb, float ohb, int nhist);
void shenolef(SCC *pscc, float aeidf, si32 aoutline);
void shenopt(SCC *pscc, int scops, int kdbgi, int khist, int nhist);
void shenwin(SCC *pscc, si32 awinsz);

/* Macro to determine a scale factor to fit a range of items into
*  a fixed number of bins, avoiding divide by zero when all the
*  items are identical and avoiding index == nbins when item is
*  equal to the max item.  Assumes float gmin,gmax and int nbins.  */
#define binscl(gmin, gmax, nbins) \
   (gmax != gmin ? ((float)nbins - BINADJ)/(gmax - gmin) : 1)

/* Things for MATLAB interfacing */
#ifdef I_AM_MATLAB
#define SmexShmNm "/ShenMexCommPtr"
#ifdef _ISOC99_SOURCE
#define URW_Mode S_IRUSR|S_IWUSR
#else
#define URW_Mode S_IREAD|S_IWRITE
#endif
#endif

#endif  /* SHENCAS_HDR_INCLUDED */
