/* (c) Copyright 2011-2013, The Rockefeller University *11114* */
/* $Id: turtle.h 59 2017-01-13 20:34:16Z  $ */
/***********************************************************************
*                              turtle.h                                *
*                                                                      *
*  This header defines the interface for turtle(), a program that      *
*  analyzes a given image and returns a graph structure describing     *
*  the object(s) found in that image, and for tfeavec(), a program     *
*  that extracts a vector of features from a TGRAPH suitable for       *
*  input to some further categorization network or analysis, and for   *
*  harrow(), a program that analyzes an image in strips to define      *
*  density blocks with some geometric relationship information.        *
*                                                                      *
*  The makefile should define "I_AM_MATLAB" if compiled for a MATLAB   *
*  mex-file environment, otherwise CRK library routines will be used.  *
*  System-dependent definitions from sysdef.h are assumed available.   *
*                                                                      *
*  N.B.  All coordinates are in rasters, assumed to be equal in x,y.   *
*  x is horizontal, y is vertical downwards, origin is at ULHC of      *
*  image.                                                              *
*                                                                      *
*  This is a C implementation of the TURTLE.ASSEMBLE program used in   *
*  the Darwin II model (1981 version).  (The MXLNJN and MXLJNS defs    *
*  make it possible to avoid some messy dynamic allocations/reallocs.) *
************************************************************************
*  V2A, 07/19/11, GNR - Redesigned, based on IBM Assembler version     *
*  V2B, 10/25/11, GNR - Add definitions for tfeavec()                  *
*  Rev, 10/30/11, GNR - Add agapdist, agapang, amxgaps for gap jumps   *
*  Rev, 01/03/12, GNR - Increase MXLNN1 to 32767                       *
*  ==>, 01/03/12, GNR - Last date before committing to svn repository  *
*  V2C, 02/15/12, GNR - Add interface for harrow()                     *
*  V2D, 12/07/12, GNR - Add interface for Edge Likelihood Index (ELI)  *
*                       Reduce MXLNN1 back to 10000, MXJCN to 12000    *
*  V2E, 03/22/13, GNR - Add new tfeavecs for line orientation, node    *
*                       distance and eli clustering                    *
***********************************************************************/

#ifndef TURTLE_HDR_INCLUDED
#define TURTLE_HDR_INCLUDED

/* Configuration parameters that may be used outside the package */
#define MXIMGXY  65535        /* Max image x,y dimension */
#define MXJCN    12000        /* Maximum junction number */
#define MXLNN1   10000        /* Maximum line number + 1 */

/* Sanity checking on configuration parameters */
#if MXIMGXY >= (1<<16)
#error Some code assumes x,y coords fit in a ui16 and x*y in a ui32
#endif
#if MXLNN1 >= (1<<15)
#error Must increase size of JCNODE lnnum, etc. if MXLNN1 >= 2**15
#endif

/* Hidden private data defined in turtint.h */
typedef struct turtint_t TRTL;

/* Bidirectional list pointer pair */
typedef struct bdlpp_t {
   struct bdlpp_t *pnxt;      /* Ptr to next struct in list */
   struct bdlpp_t *pprv;      /* Ptr to previous struct in list
                              *  (head pprv points to last item) */
   } BDLPP;

/* Graph representation returned by turtle() */

#define MXJNLN 8              /* Max lines at any one junction */
typedef struct jcn_t {
   struct jcn_t *pnjcn;       /* Ptr to next junction */
   float jcx,jcy;             /* Location of junction */
   ui16  jcnum;               /* Id number of this junction */
   ui16  jcsize;              /* Number of intersection lines */
   ui16  jlnnos[MXJNLN];      /* Line numbers at this junction */
   } JCNODE;

/* N.B.  In graph theory, an edge can touch no more than 2 nodes,
*  but we are allowing 3 until further work indicates that the
*  strict definition must be followed.  */
#define NLENDS       2        /* Number of ends a line has */
#define MXLJNS       3        /* Max junctions one line can touch */
#define LRANLCOL     8        /* Length of random color spec */
typedef struct lnn_t {
   struct lnn_t *pnlnn;       /* Ptr to next line */
   struct jcn_t *pjc[MXLJNS]; /* Ptrs to contacted junctions */
   float ln1x,ln1y;           /* Coordinates of first end */
   float ln2x,ln2y;           /* Coordinates of second end */
   float lnlgth;              /* Length of line */
   float lncosor,lnsinor;     /* cos,sin(orientation) */
   float lnwidth;             /* Average width of trace boxes */
   float grad;                /* Mean magnitude of gradient */
   float kurv;                /* Mean line curvature */
   float eli;                 /* Edge likelihood index */
   ui32  lnsize;              /* Number of grid points in line */
   ui16  lnnum;               /* Id number of this line */
   ui16  ljnnos[MXLJNS];      /* Junctions touching this line */
   ui16  lnnojct;             /* Number of junctions contacted */
   byte  lnflgs;              /* Status flags: */
#define LNF_CURVED   1           /* Line is curved */
#define LNF_CLOSED   2           /* Line is closed curve */
#define LNF_WAVY     4           /* Line is wavy */
#define LNF_POINT    8           /* This is just a point */
   byte  lnjif;               /* Internal flags */
#define LNF_ONCJN    1           /* Line is on current junction */
#define LNF_DECL     2           /* Line has been declared */
   char  lncol[LRANLCOL];     /* pencol() spec for random color */
   } LNNODE;

/* Block density information returned by harrow() */
typedef struct hdn_t {
   struct hdn_t *pnhdn;       /* Ptr to next density block */
   float hdcx,hdcy;           /* Center coordinates of block */
   float hdhgt,hdwid;         /* Height and width of block */
   float hdcos,hdsin;         /* cos,sin of scan orientation,
                              *  clockwise from horizontal */
   ui32  hdden;               /* Total density in block */
   ui16  hdis;                /* Scan number at this angle (from 1) */
   byte  hdib;                /* Block number in this scan (from 1) */
   byte  isang;               /* Scan angle index (0 ... nhdang-1) */
   } HDNODE;

/* Header for one graph analysis.
*  N.B.:  When the last line, junction, or harrow node on the list
*  is removed, the ppljc, etc. pointer must be updated in order for
*  turtfree() to release the list correctly.  */
typedef struct graph_t {
   BDLPP  tgp;                /* TGRAPH double list links */
   JCNODE *pjcn,**ppljc;      /* Ptrs to junctions */
   LNNODE *plnn,**pplln;      /* Ptrs to lines */
   HDNODE *phdn,**pplhd;      /* Ptrs to harrow densities */
   ui16   njcn;               /* Number of junctions */
   ui16   hjcn;               /* Highest junction number issued */
   ui16   nlnn;               /* Number of lines */
   ui16   hlnn;               /* Highest line number issued */
   ui16   nhdn;               /* Number of harrow densities */
   ui16   nptlin;             /* Number of LNF_POINT lines */
   byte   nhda;               /* Number of harrow angles */
   byte   tgflgs;             /* Flags */
#define TGF_ELID     1           /* ELI calculation performed */
   } TGRAPH;

/* "Public" function prototypes */
#ifdef __cplusplus
extern "C" {
#endif
TRTL *turtinit(
   float awdthfac,            /* Width as a multiple of delta */
   float athangle,            /* Trace half fwd search angle (deg) */
   float amnwdth,             /* Min line (turtle base) width */
   float afbratio,            /* Front to back ratio for trace */
   float agapdist,            /* Distance a line gap can jump */
   float agapang,             /* Trace angle where gaps allowed */
   float astepfac,            /* Step as a multiple of width */
   float amncurv,             /* Min frac pixels off mean-width line
                              *  to define a line as curved or wavy */
   float amxbend,             /* Max bend before line is split */
   float ajcrad,              /* Junction search radius */
   float ajpsrad,             /* Parallel search radius */
   float ajpsdr,              /* Parallel search distance range */
   ui32  acutoff,             /* Cutoff density (initial) */
   ui32  amxgaps,             /* Max gaps in one trace box */
#define MGC_OPSHFT 16            /* Shift to hide newops in mxgaps */
   ui32  anix,                /* Image size along x */
   ui32  aniy,                /* Image size along y */
   ui32  anlmn0,              /* Minimum points to start a line */
   ui32  anlmin,              /* Minimum points to end a line */
   ui32  akprt                /* Print and plot switches */
#define TKP_ASSBDA 0x00000001    /* Assignments before disambig */
#define TKP_ASSADA 0x00000002    /* Assignments after disambig */
#define TKP_TRCPT1 0x00000004    /* Location of start of trace */
#define TKP_TRCPTn 0x00000008    /* Coords at start of new step */
#define TKP_NEWLPT 0x00000010    /* Address of new LNNODE */
#define TKP_NEWJPT 0x00000020    /* Address of new JCNODE */
#define TKP_NEWGPT 0x00000040    /* Address of new TGRAPH */
#define TKP_NEWXYT 0x00000080    /* Address of new XYCL */
#define TKP_TRSUMS 0x00000100    /* Trace sums */
#define TKP_TRWSTP 0x00000200    /* Trace width, front, and step */
#define TKP_TRCOSB 0x00000400    /* Cos(bend) when bend detected */
#define TKP_TBOXES 0x00000800    /* Corner coords of turtle box */
#define TKP_SWINGA 0x00001000    /* Swing angle for turtle step */
#define TKP_NEWTPT 0x00002000    /* New point in turtle */
#define TKP_ASSCLN 0x00004000    /* Assignments at line close */
#define TKP_AMBIGS 0x00008000    /* Ambiguous points list */
#define TKP_LNELIM 0x00010000    /* Eliminated lines */
#define TKP_PTDISA 0x00020000    /* Points disambiguated */
#define TKP_JNSLOC 0x00040000    /* Junction start location */
#define TKP_ADDL2J 0x00080000    /* Add line to junction */

#define TPL_TRCBEG 0x00100000    /* Plot start of trace */
#define TPL_TRCSWG 0x00200000    /* Plot trace swing search */
#define TPL_TRCEND 0x00400000    /* Plot when line ended */
#define TPL_TRACES 0x00800000    /* Plot after each trace step */
#define TPL_PTSREM 0x01000000    /* Plot after points removed */
#define TPL_PTDISA 0x02000000    /* Plot after disambiguation */
#define TPL_PJCTNS 0x04000000    /* Plot junctions */
#define TPL_LNNUMS 0x08000000    /* Plot numbers on lines */
#define TPL_PGRIDS 0x10000000    /* Add grid lines to plots */
#define TPL_COLLEG 0x20000000    /* Add color legend to plots */
#define TPL_MKMETA 0x40000000    /* Make metafile */
#define TPL_ELICOL 0x80000000    /* Color by eli value */
#define TPL_ALLPLT 0xfff00000    /* Mask for all plot bits */
   );
TGRAPH *turtle(TRTL *ptc, byte *image, char *lbl);
void tagraph(TRTL *ptc);
void tfeavec(TGRAPH *tg, float *fvec, ui32 kgf
#define TFL_CURVED 0x00000001    /* Frac. of curved edges */
#define TFL_WAVY   0x00000002    /* Frac. of wavy edges */
#define TFL_CLOSED 0x00000004    /* Frac. of closed edges */
#define TFL_POINTS 0x00000008    /* Frac. of edges that are points */
#define TFL_ELEN12 0x00000010    /* Frac. edge lengths in 1st 1/2 */
#define TFL_ELEN22 0x00000020    /* Frac. edge lengths in 2nd 1/2 */
#define TFL_ELEN13 0x00000040    /* Frac. edge lengths in 1st 1/3 */
#define TFL_ELEN23 0x00000080    /* Frac. edge lengths in 2nd 1/3 */
#define TFL_ELEN33 0x00000100    /* Frac. edge lengths in 3rd 1/3 */
#define TFL_EORHOR 0x00000200    /* Frac. edges in 1st +/- 30 deg.*/
#define TFL_EORDIA 0x00000400    /* Frac. edges in 2nd +/- 30 deg.*/
#define TFL_EORVER 0x00000800    /* Frac. edges in 3rd +/- 30 deg.*/
#define TFL_ENODE0 0x00001000    /* Frac. edges with 0 nodes */
#define TFL_ENODE1 0x00002000    /* Frac. edges with 1 nodes */
#define TFL_ENODE2 0x00004000    /* Frac. edges with 2 nodes */
#define TFL_ENODE3 0x00008000    /* Frac. edges with 3 nodes */
#define TFL_NODES2 0x00010000    /* Frac. nodes with 2 edges */
#define TFL_NODES3 0x00020000    /* Frac. nodes with 3 edges */
#define TFL_NODES4 0x00040000    /* Frac. nodes with >=4 edges */
#define TFL_EORVL1 0x00080000    /* Frac. edges-longest 1st 30 deg */
#define TFL_TOTNOD 0x00100000    /* 1/(total nodes) */
#define TFL_TOTEDG 0x00200000    /* 1/(total edges) */
#define TFL_EORVL2 0x00400000    /* Frac. edges-longest 2nd 30 deg */
#define TFL_EORVL3 0x00800000    /* Frac. edges-longest 3rd 30 deg */
#define TFL_NNDIS1 0x01000000    /* Node-node dist in 1st quartile */
#define TFL_NNDIS2 0x02000000    /* Node-node dist in 2nd quartile */
#define TFL_NNDIS3 0x04000000    /* Node-node dist in 3rd quartile */
#define TFL_NNDIS4 0x08000000    /* Node-node dist in 4th quartile */
#define TFL_ELIQ1  0x10000000    /* ELI 1st quartile cutoff */
#define TFL_ELIQ2  0x20000000    /* ELI 2nd quartile cutoff */
#define TFL_ELIQ3  0x40000000    /* ELI 3rd quartile cutoff */
   );
void eliinit(TRTL *p,         /* Ptr to existing TRTL block */
   float amnkurv,             /* Minimum curvature */
   float app,                 /* Exponent of gradient in ELI */
   float aqq,                 /* Exponent of length in ELI */
   float arr                  /* Exponent of curvature in ELI */
   );
float elianal(TRTL *p,        /* Ptr to existing TRTL block */
   byte   *rawimg,            /* Ptr to raw image from which p->img
                              *  was derived (same size, x,y order) */
   float  *pqr                /* Ptr to an optional array to receive
                              *  lnnum & normalized p,q,r components
                              *  of eli for each line (NULL to omit) */
   );
void harrinit(TRTL *p,        /* Ptr to existing TRTL block */
   float ahdfbx,              /* Forward box resolution (pixels) */
   int   anhdang,             /* Number of scan angles */
   int   anhddiv,             /* Number of divisions per angle */
   int   ahdcut,              /* Cutoff density */
   int   amnnac,              /* Min points to terminate a gap */
   ui32  ahdkp                /* Print and plot options */
#define HDP_NEWHDN 0x00000010    /* Address of new HDNODE */
#define HDP_NEWGPT TKP_NEWGPT    /* Address of new TGRAPH (0x0040) */
#define HPL_ALLBOX 0x00000100    /* Plot all boxes at end */
#define HPL_INCIMG 0x00000200    /* Include image in plot */
/* High bits TPL_PGRIDS, TPL_MKMETA are also accepted */
   );
TGRAPH *harrow(TRTL *p, TGRAPH *pwg, byte *image, char *lbl);
void hfeavec(TGRAPH *tg, float *fvec, ui32 khf
#define HFL_SCANS1 0x00000001    /* Frac. scans with 1 box */
#define HFL_SCANS2 0x00000002    /* Frac. scans with 2 boxes */
#define HFL_SCANSM 0x00000004    /* Frac. scans with >2 boxes */
#define HFL_RHTG13 0x00000008    /* Frac. box ht in glbl 1st 1/3 */
#define HFL_RHTG23 0x00000010    /* Frac. box ht in glbl 2nd 1/3 */
#define HFL_RHTG33 0x00000020    /* Frac. box ht in glbl 3rd 1/3 */
#define HFL_RHTA13 0x00000040    /* Frac. box ht in angle 1st 1/3 */
#define HFL_RHTA23 0x00000080    /* Frac. box ht in angle 2nd 1/3 */
#define HFL_RHTA33 0x00000100    /* Frac. box ht in angle 3rd 1/3 */
#define HFL_DENS13 0x00000200    /* Frac. w/density in 1st 1/3 */
#define HFL_DENS23 0x00000400    /* Frac. w/density in 2nd 1/3 */
#define HFL_DENS33 0x00000800    /* Frac. w/density in 3rd 1/3 */
#define HFL_CENR13 0x00001000    /* Frac. w/center r in 1st 1/3 */
#define HFL_CENR23 0x00002000    /* Frac. w/center r in 2nd 1/3 */
#define HFL_CENR33 0x00004000    /* Frac. w/center r in 3rd 1/3 */
#define HFL_CENS13 0x00008000    /* Frac. w/center s in 1st 1/3 */
#define HFL_CENS23 0x00010000    /* Frac. w/center s in 2nd 1/3 */
#define HFL_CENS33 0x00020000    /* Frac. w/center s in 3rd 1/3 */
#define HFL_FBANDS 0x00040000    /* Frac. bands at this angle */
#define HFL_FBOXES 0x00080000    /* Frac. boxes at this angle */
#define HFL_GAPS11 0x00100000    /* Frac. gaps > largest box */
#define HFL_GAPS12 0x00200000    /* Frac. gaps > 1/2 largest box */
#define HFL_GAPS14 0x00400000    /* Frac. gaps > 1/4 largest box */
#define HFL_GAPS18 0x00800000    /* Frac. gaps > 1/8 largest box */
#define HFL_LHHTRL 0x01000000    /* Low/high height (local) */
#define HFL_LHHTRG 0x02000000    /* Low/high height (global) */
#define HFL_MHHTRL 0x04000000    /* Mean/high height (local) */
#define HFL_MHHTRG 0x08000000    /* Mean/high height (global) */
#define HFL_MXHTTA 0x10000000    /* Max height is at this angle */
#define HFL_FGAPTA 0x20000000    /* Fraction of gaps this angle */
#define HFL_GAPCTR 0x40000000    /* Center of gravity of gaps */
#define HFL_GAPSTD 0x80000000    /* Wtd. std. dev. of gap centers */
   );
void tglist(TGRAPH *tg, char *lbl, int kpcol);
int  turtldel(TRTL *p, TGRAPH *pwg, int lnno);
void turtplot(TRTL *p, xyf *pe, ui32 kplt, int jass0);
void turtrnum(TRTL *p, TGRAPH *pwg);
void turtfdxy(TRTL *p);
void turtfree(TRTL *p, TGRAPH *pgold);
void turtclse(TRTL *p);
void newimgsz(TRTL *p, ui32 nnix, ui32 nniy);
void newmxwid(TRTL *p, float amxwdth);
#ifdef __cplusplus
   }
#endif

#endif  /* TURTLE_HDR_INCLUDED */
