/* (c) Copyright 1989-2008, The Rockefeller University *21115* */
/* $Id: glu.h 59 2017-01-13 20:34:16Z  $ */
/***********************************************************************
*                                GLU.H                                 *
*             ROCKS-to-METAFILE plot glue routines header              *
*                                                                      *
*  N.B. This file is obsolete and will be removed when the new plot    *
*     library is completed.  At this time, a revised copy of glu.h     *
*     stored in ~/src/oldplot is the one actually in use.              *
*  N.B.  Enhancements to oldplot not yet implemented in new plot lib:  *
*     (1) Extended length of frame number in new frame ('[') record.   *
*     (2) Check all error codes vs errtab.                             *
*     (3) abexit when metafile exceeds 2GB except in 64-bit code.      *
*     (4) Separate control of mfdraw/metafile per frame.               *
*     (5) Bitmap 24-bit color and scaled bitmaps.                      *
*     (6) Printing text of error messages received from mfdraw.        *
*     (7) Allow early mfsynch direct from application                  *
************************************************************************
* V1A, 04/20/89, JWT                                                   *
* V2A, 12/12/98, GNR - Complete rewrite to support binary metafiles    *
* Rev, 08/11/02, GNR - Merge in contents of gmf.h                      *
* ==>, 02/26/08, GNR - Last date before committing to svn repository   *
***********************************************************************/

#include <math.h>
#include <stddef.h>
#include <string.h>
#include "sysdef.h"
#include "plotdefs.h"

/* Avoid bringing in rfdef.h just to define RFdef */
#ifndef RFDTYPE
#define RFDTYPE void
#endif
/* Avoid bringing in rkhash.h just to define struct htbl */
#ifndef HTBLTYPE
#define HTBLTYPE void
#endif

/*---------------------------------------------------------------------*
*                  Internal plot routine prototypes                    *
*---------------------------------------------------------------------*/

void mfbitpk(unsigned long item, int len);
int bpause(void);
#ifdef PAR
long cmdpack(char *pcmd, unsigned long lcmd);
#define CMDPACK(mc,len) cmdpack(mc,len)
#else /* Serial */
long cmdpack(char *pcmd);
#define CMDPACK(mc,len) cmdpack(mc)
#endif
void cmxpack(long x);
void cmypack(long y);

/*---------------------------------------------------------------------*
*                              Constants                               *
*---------------------------------------------------------------------*/

#define LocalHostIP "127.0.0.1"

#define PI     3.141592654 /* Greek constant */
#define TORADS 0.017453293 /* Degrees to radians */

#define ASPECT   0.857143  /* Aspect ratio of standard symbols */
#define AXLBLH   1.333333  /* Axis label ht as multiple of tick ht */
#define EXPTIME  2.0       /* Minimum time for one movie exposure */
#define LABELOFF 0.333333  /* Axis label offset as multiple of ht */
#define MINARROW 0.02      /* Minimum arrow length */
#define MINORHT  0.10      /* Standard height for minor tick marks */
#define MAJORHT  0.15      /* Standard height for major tick marks */
#define POLARHT  0.0625    /* Half-length of polar plot tick marks */
#define POLARLS  0.10      /* Extra radius of polar long spokes */
#define MXTVLEN 12         /* Max tick value length */
#define SUPEROFF 0.333333  /* Allowance for height of superscript */
#define VALUGARD 0.05      /* Spacing of value labels from ticks */

#define S14     16384.0    /* Scale for 14 fraction bits */
#define S16     65536.0    /* Scale for 16 fraction bits */
#define ANGMSK  0x00000fff /* Zero bits in short form angle */
#define ANGRSS     12      /* Right shift for short angle */
#define ANGMSK0 0x0000001f /* Bits stored for short angle */
#define ANGMSK1 0x0001ffff /* Bits stored for long angle */
#define ANGTYP1 0x00020000 /* Type code for long angle */
#define GRSS       14      /* Right shift for short general num */
#define GMSK0   0xffffbfff /* Zero bits in type 0 general num */
#define GMSK10  0x00007fff /* Bits stored for type 10 general num */
#define GMSK110 0x001fffff /* Bits stored for type 110 general num */
#define GTST10  0x00004000 /* Test for type 10 general num */
#define GTST110 0x00100000 /* Test for type 110 general num */
#define GTYP10  0x00010000 /* Code for type 10 general num */
#define GTYP110 0x00c00000 /* Code for type 110 general num */
#define GTYP111 0xe0000000 /* Code for type 111 general num */
#define IMSK00  0x0000003f /* Bits stored for type 00 integer */
#define IMSK01  0x00003fff /* Bits stored for type 01 integer */
#define IMSK10  0x003fffff /* Bits stored for type 10 integer */
#define ITST00  0x00000020 /* Test for type 00 signed integer */
#define ITST01  0x00002000 /* Test for type 01 signed integer */
#define ITST10  0x00200000 /* Test for type 10 signed integer */
#define ITYP01  0x00004000 /* Code for type 01 integer */
#define ITYP10  0x00800000 /* Code for type 10 integer */
#define ITYP11  0xc0000000 /* Code for type 11 integer */
#define INITCSYM   16      /* Initial number of color synonyms */
#define MAXSTRING  32      /* Maximum label size */

/*---------------------------------------------------------------------*
*          Operation codes and lengths for metafile commands           *
* N.B. Code 0x00 is reserved for use by Tsp1 and must not be assigned. *
*---------------------------------------------------------------------*/

#define OpStart  0x1B      /* Start of plot */
#define OpArc    0x01      /* Arc */
#define OpBMap   0x02      /* Bit map */
#define OpCirc   0x03      /* Circle */
#define OpDraw   0x04      /* Draw */
#define OpElps   0x05      /* Ellipse */
#define OpFont   0x06      /* Font */
#define OpStar   0x07      /* Polygon/Star */
#define OpThick  0x08      /* Retrace/Line thickness */
#define OpColor  0x09      /* Color index */
#define OpSCol   0x29      /* Synonym color index */
#define OpCtLd   0x0A      /* Color table load */
#define OpPCol   0x0B      /* Pen color/color name */
#define OpCtsyn  0x2B      /* Color table synonym */
#define OpLine   0x0C      /* Line */
#define OpTLine  0x2C      /* Thin line (hairline) */
#define OpMove   0x0D      /* Move */
#define OpObjid  0x0F      /* Object identifier */
#define OpPoly   0x10      /* Polyline */
#define OpSqre   0x11      /* Square */
#define OpRect   0x12      /* Rectangle */
#define OpSymb   0x13      /* Symbol */
#define OpSymc   0x33      /* Continued symbol */
#define OpPTyp   0x14      /* Pen type */
#define OpFrame  0x16      /* Frame/viewpoint */
#define OpGMode  0x18      /* Drawing mode */
#define OpNOP    0x1C      /* No operation/align */
#define OpEnd    0x1D      /* End of metafile */
/* Frame suboperations */
#define FOpSet   0x00      /* Set frame number */
#define FOpDel   0x01      /* Delete frame */
#define FOpCon   0x02      /* Define constant frame */
#define FOpBox   0x03      /* Define boxed (movable) frame */

/* Type codes for the packing interpreter.  These are #defines
*  rather than an enum so storage can be forced to byte size.
*  Comments give arguments expected by each in next bytes. The
*  Tsp1 code must not be the same as any plot operation code.  */
#define Tsp1       0       /* Skip pass 1 (scan) */
#define Txc        1       /* x coordinate: x index */
#define Txp        2       /* x prime--not stored: x index */
#define Tyc        3       /* y coordinate: y index */
#define Typ        4       /* y prime--not stored: y index */
#define Trad       5       /* radius: arg in r[NEW] */
#define Twd        6       /* width: arg in w[NEW] */
#define Tht        7       /* height: arg in h[NEW] */
#define Tang       8       /* angle: arg in aa */
#define Tchr       9       /* char string: gks index of length */
#define Tci       10       /* color index: c index of color */
#define Tk        11       /* unsigned int: gks index */
#define Tkn       12       /* unsigned int: gks index, length */
#define Ts        13       /* signed int: gks index */
#define Tsn       14       /* signed int: gks index, length */
#define Tg        15       /* general variable: gks index */
#define Tsp2      17       /* Skip pass 2 (pack) */
#define Tend      18       /* End command: no arg */

/* Values for storing and checking x,y,r,w,h */
#define CUR        0       /* Current coordinate value */
#define NEW        1       /* New coordinate value */
#define END        2       /* Ending coordinate value */

/* Lengths of some items that are fixed (in bits) */
#define Lop        6       /* Length of an operation code */
#define Lcx        2       /* Length of type code in a coordinate */
#define Lixy       7       /* Length of integer part of x,y coord */
#define Lihw       6       /* Length of integer part of h,w coord */
#define Lsa        6       /* Length of a short angle code */
#define Lla       18       /* Length of a long angle code */
#define Lg0        2       /* Length of type 0 general number */
#define Lg10      16       /* Length of type 10 general number */
#define Lg110     24       /* Length of type 110 general number */
#define Lg111     32       /* Length of type 111 general number */
#define Li00       8       /* Length of type 00 integer (k or s) */
#define Li01      16       /* Length of type 01 integer (k or s) */
#define Li10      24       /* Length of type 10 integer (k or s) */
#define Li11      32       /* Length of type 11 integer (k or s) */
#define LSkipCt    4       /* Length of a skip count field */
/* See discussion in mfalign.c for derivation of this constant */
#define Lnop (((Lop+LSkipCt-3)/BITSPERBYTE+2)*BITSPERBYTE)

/*---------------------------------------------------------------------*
*                               Macros                                 *
*---------------------------------------------------------------------*/

/* Convert coordinates to fixed point */
#define fixxy(x) (_RKG.coordscale*(x) + (((x) >= 0.0) ? 0.5 : -0.5))
#define fixaa(a) (S16*(a) + (((a) >= 0.0) ? 0.5 : -0.5))
#define fixg(g)  (S14*(g) + (((g) >= 0.0) ? 0.5 : -0.5))
/* Hexadecimal character to integer */
#define hexch_to_int(c) ( ((c) <= '9') ? (c) - '0' : (c) - 'A' + 10 )
/* Generate bcdout decimal conversion mode parameter:
*     a = number of decimals, b = total number of digits */
#define RKD(a,b) ((RK_IORF+RK_SNGL+(a)*RK_D+(b)-1))
/* Generate bcdout integer conversion mode parameter:
*     a = number of digits */
#define RKDI(a)  ((RK_IORF+(a)-1))
/* Bits left in current buffer word */
#define BitsLeft BitRemainder(_RKG.MFbrem)

/*---------------------------------------------------------------------*
*                           Data structures                            *
*---------------------------------------------------------------------*/

/* Status of buttons sent by mfdraw */
struct butt {
   char movie;             /* See MM_xxx in plotdefs.h */
   char next;              /* 1 for next frame */
   char intxeq;            /* See MM_xxx in plotdefs.h */
   char snapshot;          /* 1 for snapshot--obsolete */
   } ;

/* Shared data broadcast from host to all other nodes on startup.
*  32-bit vals are 'long' (not 'int') so swap4 can be used safely.
*  Max lengths are put here to save repeated code on nodes.  */
struct RKGSdef {
   float base_fact;        /* Initial scale factor */
   long dbgmask;           /* Unspecified debug information */
   long MFBuffLen;         /* Length of metafile command buffer
                           *  (not including space for MFCountWord) */
   long MFmode;            /* Metafile output modes (0 if NOPLOT) */
#define METF       1          /* Metafile */
#define SGX        2          /* X graphics */
#define MFSO       4          /* Metafile written to stdout */
   short lcf;              /* Length of coordinate fractions */
   short lci;              /* Length of color indexes */
   unsigned long unitxy;   /* 1<<lcf */
   unsigned long XYMask01; /* (1<<(2+lcf)) - 1 */
   unsigned long XYType10; /* 2<<(7+lcf) */
   unsigned long XYMask10; /* (1<<(7+lcf)) - 1 */
   unsigned long HWType10; /* 2<<(6+lcf) */
   unsigned long HWMask10; /* (1<<(6+lcf)) - 1 */
   unsigned long ColMask;  /* (1<<lci) - 1 */
   short mdlxy;            /* Length of a fraction-only x,y coord */
   short mxlxy;            /* Max length of an x,y coord */
   short mxlrhw;           /* Max length of rad, ht, wd */
   short mxlStart;         /* Max length of Start record */
#ifdef PAR
   short mxlArc;           /* Max length of Arc record */
   short mxlCirc;          /* Max length of Circ record */
   short mxlElps;          /* Max length of Elps record */
   short mxlStar;          /* Max length of Star record */
   short mxlColor;         /* Max length of Color record */
   short mxlCtLd;          /* Max length of CtLd record */
   short mxlLine;          /* Max length of Line record */
   short mxlMove;          /* Max length of Move record */
   short mxlPoly;          /* Max length of Poly record */
   short mxlSqre;          /* Max length of Sqre record */
   short mxlRect;          /* Max length of Rect record */
   short mxlSymb;          /* Max length of Symb record */
   short mxlSymc;          /* Max length of Symc record */
   short mxlFrameB;        /* Max length of Frame-B record */
#endif /* PAR */
   };

/* Complete state of the graphics package */
struct RKGdef {
   unsigned char *MFFullBuff; /* Ptr to Full buffer incl'g CountWord */
   unsigned char *MFCmdBuff;  /* Ptr to Metafile command buffer */
   unsigned char *MFCurrPos;  /* Ptr to MFCmdBuff current position */
#ifndef PARn
   RFDTYPE *MFfdes;        /* Metafile file descriptor */
   char *MFfn;             /* Metafile file name */
   char *MFDdisplay;       /* X-windows display */
   char *MFDwicon;         /* X-windows window icon */
   char *MFDwtitl;         /* X-windows window title */
   int  MFDsock;           /* Socket to MFDraw */
   int  MFDpid;            /* MFDraw process id */
#endif
   HTBLTYPE *pctshash;     /* Color synonym hash table */
   char *psymbol;          /* Ptr to symbol string */
   struct butt msgbutt;    /* Button info from bpause or host */
   struct RKGSdef s;       /* Shared data */
   float coordscale;       /* 2**lcf */
   float fact;             /* Current default frame scale factor */
   long katomic;           /* Set to MFB_ATOM for atomic sequences */
   long ngcallno;          /* Plot number, 0 before first ngraph */
   long aa;                /* Angle argument (S16) */
   long gks[8];            /* General and integer arguments */
   long lang;              /* Most recent lettering angle */
   long lht;               /* Most recent letter height */
   long xo,yo;             /* Current default frame origin */
   long x[3], y[3];        /* Current and new x,y coords S(lcf) */
   long r[2], w[2], h[2];  /* Current and new radius, width, height */
   COLOR c[2];             /* Current plot color index */
   unsigned long gobjid;   /* Graphics object identifier */
   long MFbrem;            /* Bits remaining in current buffer,
                           *  less space for possible align NOP */
   long MFCountWord;       /* Count and flags for current buffer */
   unsigned int frame;     /* Current plotting frame */
   unsigned int nxtcsyn;   /* Next color synonym */
#ifdef PAR
   int mfsrid;             /* Host id */
   int multinode;          /* NONZERO if more than one node */
   int numnodes;           /* Number of nodes in parallel system */
   int mynode;             /* Number of current node */
#endif
   int hfontname;          /* Current font name locator */
   short gmode;            /* Current drawing mode */
#ifndef PARn
   short nexp;             /* Number of exposures per frame */
   char escstdout;         /* TRUE to write escape char at SOB */
   char escape;            /* Escape char for interleaved metafile */
   char movie_device;      /* Values defined in plotdefs.h */
   char movie_mode;        /* Values defined in plotdefs.h */
   char domvexp;           /* Expose this frame on the movie device */
#endif /* !PARn */
   char currPenCol[MAXLPCOL]; /* Current pen color */
   char currPenTyp[MAXLPCOL]; /* Current pen type */
   char currColType;          /* Current method of specifying color */
#define CCT_COLOR  0          /* Color index */
#define CCT_CTSCOL 1          /* Color table synonym */
#define CCT_PENCOL 2          /* Color name */
   byte curretrac;         /* Current line retrace (krt) */
   char MFencoding;        /* Metafile encoding: 'A', 'B', or 'D' */
   char MFPending;         /* TRUE if write pending to host */
   byte MFReset;           /* State reset flags */
#define ResetX  0x01
#define ResetY  0x02
#define ResetF  0x20
#define State1  0x40
#define State2  0x80
   };

/*---------------------------------------------------------------------*
*             Here is where all the plotting defaults go               *
*---------------------------------------------------------------------*/

#ifdef I_AM_NGRAPH
struct RKGdef _RKG = {
   NULL,                   /* MFFullBuff */
   NULL,                   /* MFCmdBuff */
   NULL,                   /* MFCurrPos */
#ifndef PARn
   NULL,                   /* MFfdes */
   NULL,                   /* MFfn */
   NULL,                   /* MFDdisplay */
   NULL,                   /* MFDwicon */
   NULL,                   /* MFDwtitl */
   0,                      /* MFDsock */
   0,                      /* MFDpid */
#endif
   NULL,                   /* pctshash */
   NULL,                   /* psymbol */
   {                       /* Begin msgbutt struct */
      MM_MOVIE-1,             /* movie mode = movie */
      0,                      /* next frame */
      0,                      /* intxeq     */
      0,                      /* snapshot   */
      },
   {                       /* Begin s struct */
      1.0,                    /* base_fact */
      0,                      /* dbgmask */
      2048,                   /* MFBuffLen */
      0,                      /* MFmode */
      10,                     /* lcf */
      8,                      /* lci */
      },                      /* (Rest init'd to 0's) */
   1024.0,                 /* coordscale */
   1.0,                    /* fact */
   0,                      /* katomic */
   0,                      /* ngcallno */
   0,                      /* aa */
   { 0,0,0,0,0,0,0,0 },    /* gks */
   0, 0,                   /* lang, lht */
   0, 0,                   /* xo, yo */
   { 0, 0, 0 },            /* x */
   { 0, 0, 0 },            /* y */
   { 0, 0 },               /* r */
   { 0, 0 },               /* w */
   { 0, 0 },               /* h */
   { 0, 0 },               /* c */
   0,                      /* gobjid */
   0,                      /* MFbrem */
   0,                      /* MFCountWord */
   0,                      /* frame */
   0,                      /* nxtcsyn */
#ifdef PAR
   0,                      /* mfsrid */
   0,                      /* multinode */
   1,                      /* numnodes */
   0,                      /* mynode */
#endif
   0,                      /* hfontname */
   GM_SET,                 /* gmode */
#ifndef PARn
   0,                      /* nexp */
   0, 0,                   /* escstdout, escape */
   MD_MATRIX,              /* movie_device */
   MM_MOVIE-1,             /* movie_mode   */
   0,                      /* domvexp */
#endif
   "BLACK",                /* currPenCol */
   "DEFAULT",              /* currPenType */
   CCT_PENCOL,             /* currColType */
   0,                      /* curretrac */
   'B',                    /* MFencoding */
   0,                      /* MFPending */
   0                       /* MFReset */
   };
#else
extern struct RKGdef _RKG;
#endif

