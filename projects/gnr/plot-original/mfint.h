/* (c) Copyright 2014-2015, The Rockefeller University *11115* */
/* $Id: mfint.h 5 2008-03-13 20:32:23Z  $ */
/***********************************************************************
*                     ROCKS metafile plot library                      *
*                               MFINT.H                                *
*                                                                      *
*  Header file for use internally by routines in the plot library,     *
*  including definition of the common data structure, _RKG             *
*                                                                      *
* (Enhancements from old plot library are documented in plotting.pdf)  *
************************************************************************
*  V2A, 08/29/14, GNR - New file based on oldplot library glu.h        *
***********************************************************************/

#ifndef __MFINT_INCLUDED__
#define __MFINT_INCLUDED__

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <math.h>
#include <unistd.h>
#include "sysdef.h"
#include "rocks.h"
#include "rkxtra.h"
#include "mfio.h"
#include "plotdefs.h"

/*** Define DEBUG as sum of values for desired debug outputs ***/
/* (These are compile-time tests so no time wasted in real runs) */
#define MFDBG_NWPLT  0x01  /* Debug output when new plot starts */
#define MFDBG_SYNCH  0x02  /* Debug output when a buffer is written */
#define MFDBG_CLOSE  0x04  /* Debug output from mfclose() */
#define MFDBG_FINPL  0x08  /* Debug o/p when endplt|finplt called */
#define MFDBG_WRITE  0x10  /* Debug output from mfwrite() */
#define DEBUG 0

/* Type definitions.  Incomplete structure defs are used to permit
*  pointers to be constructed to structs defined only in specialized
*  headers.  */
typedef struct RFdef RFDTYPE;
typedef struct htbl HTBLTYPE;
typedef ui32 FrameNum;

/*---------------------------------------------------------------------*
*           Prototypes for internal functions that are not             *
*           user-callable, including bit stuffing routines             *
*---------------------------------------------------------------------*/

int bpause(void);
void rkginit(void);           /* Initialize _RKG structure */
void rkgifin(void);           /* Post-setmf() initialization */
void mfalign(void);           /* Align on a byte boundary */
void mfnxtbb(void);           /* Skip to a byte boundary */
int  mfbchk(int nbits);       /* Check buffer space, write state */
int  mfhchk(int nbits);       /* Check buffer space only */
#define NEWBUFF   1              /* mf[bh]chk:  New buffer started */
#define OLDBUFF   0              /* mf[bh]chk:  Continuing current */
void mfbitpk(ui32 item, int len);/* Data to metafile */
void a2mf(float a);           /* Angle to metafile */
void cs2mf(const char *cs, int len);/* Character string to metafile */
void h2mf(float h);           /* Height to metafile */
void g2mf(float g);           /* General float to metafile */
void k2mf(ui32  k);           /* Unsigned 32-bit int to metafile */
void r2mf(float r);           /* Radius to metafile */
void s2mf(si32  s);           /* Signed 32-bit int to metafile */
void w2mf(float w);           /* Width to metafile */
void x2mf(float x);           /* X coordinate to metafile */
void xp2mf(float x);          /* X coordinate (no update) */
void y2mf(float y);           /* Y coordinate to metafile */
void yp2mf(float y);          /* Y coordinate (no update) */

/*---------------------------------------------------------------------*
*                              Constants                               *
*---------------------------------------------------------------------*/

#define DFLTBUFLEN 4092       /* Default buffer length */
#define DFLTLCI       6       /* Default integer coord bits */
#define DFLTLCF      10       /* Default fraction coord bits */
#define LPRTBUF     255       /* Length of mfprtf string buffer */
#define MAXCBITS     28       /* Maximum lci + lcf coord bits */
#define MAXSTRING    32       /* Maximum label size */
#define MAXLCHART    32       /* Maximum chart name length */
#define MAXLCOLOR    15       /* Maximum color name length */
#define MAXLFONT     40       /* Maximum length of a font name */
#define MAXLPEN      15       /* Maximum length of a pen type name */
#define NFRMALLO     16       /* Initial frames allocation < 2^16 */
#define NWINALLO     10       /* Initial windows allocation < 2^16 */

#define DEGCIRC     360       /* Degrees in a circle */
#define PI     3.141592654    /* Greek constant */
#define TORADS 0.01745329252  /* Degrees to radians */

#define ASPECT   0.857143  /* Aspect ratio of standard symbols */
#define AXLBLH   1.333333  /* Axis label ht as multiple of tick ht */
#define EXPTIME  2.0       /* Minimum time for one movie exposure */
#define LABELOFF 0.333333  /* Axis label offset as multiple of ht */
#define MINARROW 0.02      /* Minimum arrow length */
#define MINORHT  0.10      /* Standard height for minor tick marks */
#define MAJORHT  0.15      /* Standard height for major tick marks */
#define MXTVLEN 12         /* Max tick value length */
#define POLARHT  0.0625    /* Half-length of polar plot tick marks */
#define POLARLS  0.10      /* Extra radius of polar long spokes */
#define SUPEROFF 0.333333  /* Allowance for height of superscript */
#define VALUGARD 0.05      /* Spacing of value labels from ticks */

#define S4      16.0       /* Scale for 4 fraction bits */
#define S8      256.0      /* Scale for 8 fraction bits */
#define S16     65536.0    /* Scale for 16 fraction bits */
#define ANGMASK 0x0000ffff /* Angle mask for 15 bits plus sign */
#define ANGLOW  0x000007ff /* Mask for (15-4) bits of angle data */
#define ANGRES  92.0222222 /* 2^15/360 scale for angles */
#define GMSK10  0x0001ffff /* Bits stored for type 10 general num */
#define GMSK110 0x01ffffff /* Bits stored for type 110 general num */
#define GTYP10  0x00020000 /* Code for type 10 general num */
#define GTYP110 0x0c000000 /* Code for type 110 general num */
#define GTYP111 0x00e00000 /* Code for type 111 general num */
#define ITST00  0x0000003f /* Range test for type 00 integer */
#define ITST01  0x00003fff /* Range test for type 01 integer */
#define ITST10  0x003fffff /* Range test for type 10 integer */
#define ITYP01  0x00004000 /* Code for type 01 integer */
#define ITYP10  0x00800000 /* Code for type 10 integer */
#define ITYP11  0xc0000000 /* Code for type 11 integer */

/*---------------------------------------------------------------------*
*      Operation codes and lengths for binary metafile commands        *
* N.B. Code 0x00 is reserved for use by Tsp1 and must not be assigned. *
*---------------------------------------------------------------------*/

#define OpStart  0x1B      /* Start of plot */
#define OpNewWin 0x3B      /* Start new window */
#define OpArc    0x01      /* Arc */
#define OpBMap   0x02      /* Bit map (unscaled) */
#define OpBMaps  0x22      /* Bit map (scaled) */
#define OpCirc   0x03      /* Circle */
#define OpDraw   0x04      /* Draw */
#define OpEllps  0x05      /* Ellipse */
#define OpFont   0x06      /* Font or register font */
#define OpRFont  0x26      /* Use registered font */
#define OpStar   0x07      /* Polygon/Star */
#define OpThick  0x08      /* Retrace/Line thickness */
#define OpColor  0x09      /* Color index */
#define OpRCol   0x29      /* Use registered color */
#define OpPCol   0x0B      /* Pen color/color name */
#define OpCtsyn  0x2B      /* Register color */
#define OpLine   0x0C      /* Line */
#define OpTLine  0x2C      /* Thin line (hairline) */
#define OpMove   0x0D      /* Move */
#define OpObjid  0x0F      /* Object identifier */
#define OpPoly   0x10      /* Polyline */
#define OpSqre   0x11      /* Square */
#define OpRect   0x12      /* Rectangle */
#define OpSymb   0x13      /* Symbol */
#define OpSymc   0x33      /* Continued symbol */
#define OpPTyp   0x14      /* Pen type or register pen type */
#define OpRPTyp  0x34      /* Use registered pen type */
#define OpFrame  0x16      /* Frame/viewpoint */
#define OpChgFrm 0x36      /* Change current frame */
#define OpFrmCtl 0x3F      /* Draw/delete frame record */
#define OpWinCtl 0x17      /* Finish/delete window */
#define OpChgWin 0x37      /* Change window */
#define OpGMode  0x18      /* Drawing mode */
#if BITSPERBYTE <= 9
#define OpAlign  0x1C      /* No operation/align */
#define LSkipCt    4       /* Length of a skip count field */
#else
#define OpAlign  0x3c
#define LSkipCt    5       /* Length of a skip count field */
#endif
#define OpState  0x1E      /* Restore state */
#define OpEnd    0x1D      /* End of metafile */
/* Frame suboperations */
#define FOpCpy   0x01      /* Copy stored commands to window */
#define FOpUpd   0x02      /* Update with commands since bookmark */
#define FOpDel   0x03      /* Delete stored commands */
#define FOpPush  0x04      /* Copy frame to pushdown stack */
#define FOpPop   0x05      /* Pop frame pushdown stack to current */

/* Lengths of some items that are fixed (in bits) */
#define Lop        6       /* Length of an operation code */
#define LNop (Lop+LSkipCt) /* Length of an Align/NOP command */
#define LVxx       6       /* Elements of a Vxx array */
#define Lcx        2       /* Length of type code in a coordinate */
#define Lsa        5       /* Length of a short angle code */
#define Lla       17       /* Length of a long angle code */
#define Llc        6       /* Bit length of chart name length */
#define Lg0        2       /* Length of type 0 general number */
#define Lg10      19       /* Length of type 10 general number */
#define Lg110     28       /* Length of type 110 general number */
#define Lg111     40       /* Length of type 111 general number */
#define Li00       8       /* Length of type 00 integer (k or s) */
#define Li01      16       /* Length of type 01 integer (k or s) */
#define Li10      24       /* Length of type 10 integer (k or s) */
#define Li11      32       /* Length of type 11 integer (k or s) */

/*---------------------------------------------------------------------*
*                               Macros                                 *
*---------------------------------------------------------------------*/

/* Command to metafile buffer */
#define c2mf(c) mfbitpk((c),Lop);
/* Hexadecimal character to integer */
#define hexch_to_int(c) ( ((c) <= '9') ? (c) - '0' : (c) - 'A' + 10 )
/* Generate bcdout decimal conversion mode parameter:
*     a = number of decimals, b = total number of digits */
#define RKD(a,b) ((RK_IORF+RK_SNGL+(a)*RK_D+(b)-1))
/* Generate wbcdwt integer conversion mode parameter:
*     a = number of digits */
#define RKDI(a)  ((RK_IORF+RK_NINT+(a)-1))
#define RKDB(a)  ((RK_IORF+RK_NBYTE+(a)-1))
#define RKDH(a)  ((RK_IORF+RK_NHALF+(a)-1))
#define RKD32(a) ((RK_IORF+RK_NI32+(a)-1))

/*---------------------------------------------------------------------*
*                           Data structures                            *
*---------------------------------------------------------------------*/

/* Status of buttons sent by mfdraw */
struct butt {
   byte movie_mode;        /* This field formerly movie, see MM_xxx
                           *  in plotdefs.h for possible values.  */
   char next;              /* 1 for next frame */
   char intxeq;            /* See BUT_xxx in plotdefs.h */
   char snapshot;          /* 1 for snapshot--obsolete */
   /* After traditional items above for compat in mfsr */
   ui16 gfrm,gwin;         /* Graphics frame and window */
   };

/* Information that characterizes a frame within a window.
*  Note:  Frames will be allocated as an array so n'th frame can be
*  accessed directly, but unused frames will be held in a queue.  */
enum vvij { vxx, vxy, vxc, vyx, vyy, vyc };  /* Names for vv */
struct Frmdef {
   struct Frmdef *pnfrm;   /* Ptr to next frame on chain */
   float vv[LVxx];         /* Coordinate transformation matrix */
   float fmwd,fmht;        /* Frame width and height */
   /* State variables that are now kept in mfdraw */
   si32  cx,cy;            /* Current plot position, S(lcf) */
   ui32  ch,cw,cr;         /* Current height, width, radius */
   ui32  ccolor;           /* Current color index or length(ccolnm) */
   ui32  cobj;             /* Current object */
   ui16  cfontid;          /* Current font id or length(cfontn) */
   ui16  cpenid;           /* Current pen type id or length(cpennm) */
   ui16  cthick;           /* Current line thickness */
   ui16  ifrm;             /* Global index of this frame */
   byte  cgmode;           /* Current drawing mode */
   byte  cmeths;           /* Method of specifying color, font, pen */
#define CCT_EXPL   0       /* Explicit color name */
#define CCT_RCOL   1       /* Registered color */
#define CCT_CINDX  2       /* Color index */
#define CCT_RFONT  4       /* Registered font */
#define CCT_RPEN   8       /* Registered pen id */
   byte  kreset;           /* State reset flags */
#define ResetX  0x01
#define ResetY  0x02
#define ResetR  0x04
#define ResetH  0x08
#define ResetW  0x10
#define InReset 0x80       /* TRUE if mfstate() just completed */
   /* Following only valid if relevent cmeths bit not set */
   char  ccolnm[MAXLCOLOR];/* Current named color */
   char  cfontn[MAXLFONT]; /* Current named font */
   char  cpennm[MAXLPEN];  /* Current named pen style */
   };
typedef struct Frmdef Frame;

/* Information that may be different per window.
*  N.B.  Initially, no Windefs will be allocated.  When newwin() is
*  called, it will initiate, or increase via reallocv() as needed, a
*  batch of Windefs that can be accessed via _RKG.pW0 and the window
*  number (no list creep needed). The only ptrs into Windefs are the
*  pnwin ptrs to unused Windefs, and this chain will be empty and
*  regenerated any time a realloc occurs.  At the end of the run,
*  all the Windefs can be freed as a block with origin at pW0.  */
struct Windef {
   struct Windef *pnwin;   /* Ptr to next window on unused chain */
   struct Frmdef *pfr1;    /* Ptr to first frame on this window */
   byte *MFFullBuff;       /* Ptr to Full buffer incl'g CountWord */
   byte *MFCmdBuff;        /* Ptr to Metafile command buffer */
   byte *MFCP;             /* Ptr to MFCmdBuff current position */
   FrameNum XGFrame;       /* XG plot number, 0 before first newplt */
   FrameNum MFFrame;       /* Metafile plot number */
   float xsiz,ysiz;        /* Window size */
   si32  MFBrem;           /* Bits remaining in current buffer,
                           *  less space for possible align NOP */
   si32  MFBrem0;          /* Copy of _RKG.s.MFBrem0 */
   ui32  MFCountWord;      /* Count and flags for current buffer */
   ui32  gobjid;           /* Object id */
   ui16  iwin;             /* Global index of this window */
   ui16  cfrm;             /* Current drawing frame */
   ui16  MFActive;         /* MFMode filtered by skip bits */
   ui16  WinFlags;         /* Status flags, initially 0 */
#define WFF_HasBuff  0x01  /* TRUE if window has buffer allocated */
#define WFF_Atomic   0x02  /* TRUE if writing atomic sequence */
#define WFF_FrmPend  0x04  /* TRUE if a frame is pending */
#define WFF_DoMvExp  0x10  /* Do movie exposure set in newplt() */
#define WFF_COMPLT   0x20  /* TRUE if MBF_COMPLT sent to server,
                           *  but ack not yet received */
#define WFF_UClosed  0x40  /* Closed by user--still write metafile */
#define WFF_SClosed  0x80  /* Closed by system--end of metafile */
#ifndef PARn
   ui16  nexpose;          /* Number of exposures per frame */
   byte  movie_device;     /* Values defined in plotdefs.h */
   byte  movie_mode;       /* Values defined in plotdefs.h */
#endif /* !PARn */
   };
typedef struct Windef Win;

/* Shared data broadcast from host to all other nodes on startup.
*  Max lengths are put here to save repeated code on nodes.  */
struct RKGSdef {
   float base_fact;        /* Initial scale factor */
   float fcsc;             /* Coordinate scale = 2**lcf */
   ui32  dbgmask;
   ui32  MFBuffLen;        /* Length of metafile command buffer
                           *  (not including space for MFCountWord) */
   si32  MFBrem0;          /* Value of MFBrem at start of buffer */
   ui32  xymask;           /* Mask to avoid coordinate overflow */
   ui32  sxymask;          /* xymask with sign bit */
   ui32  sfrmask;          /* Coord fraction mask */
   ui32  unitxy;           /* 1<<lcf */
   ui16  MFMode;           /* Metafile output modes (0 if NOPLOT) */
#define METF    0x04          /* Metafile */
#define SGX     0x08          /* X graphics */
   short lci;              /* Length of coordinate integers */
   short lcf;              /* Length of coordinate fractions */
   short lct;              /* lct = lci + lcf */
   short mxlxy;            /* Max length of a cded x,y coord */
   short mxlrhw;           /* Max length of coded radius, wid, ht */
   int   numnodes;         /* Number of nodes in parallel system,
                           *  0 if serial (so NC not needed) */
/* Max lengths of various command records.  (Record types with
*  char strings will add the exact length to the value given here.
*  mxls with no lci,lcf dependence are compile-time constants.)  */
#define mxlStart  (Lop + 3*Li10 + 2*Li11 + 2*Lg111 + 2*Li01 + 6)
   short mxlArc;           /* Max length of Arc record */
   short mxlBmap;          /* Fixed part of Bitmap record */
   short mxlSBmap;         /* Fixed part of scaled Bitmap record */
   short mxlCirc;          /* Max length of Circ record */
   short mxlDraw;          /* Max length of Draw record */
   short mxlEllps;         /* Max length of Elps record */
#define  mxlFont   (Lop + Li01 + 6)
#define  mxlRFont  (Lop + Li01)
   short mxlStar;          /* Max length of Star record */
#define  mxlRetrc  (Lop + 4)
#define  mxlColor  (Lop + Li10)
#define  mxlRCol   (Lop + Li10)
#define  mxlPenCol (Lop + 4)
   short mxlLine;          /* Max length of Line record */
   short mxlMove;          /* Max length of Move record */
#define  mxlOId    (Lop + Li11)
#define  mxlPoly   (Lop + 3 + Li11)
   short mxlSqre;          /* Max length of Sqre record */
   short mxlRect;          /* Max length of Rect record */
   short mxlSymb;          /* Fixed part of Symbol record */
   short mxlCSym;          /* Fixed part of continued Symbol */
#define  mxlPenTyp (Lop + Li01 + 4)
#define  mxlRPen   (Lop + Li01)
   short mxlFrmNR;         /* Max length of frame def w/o Vxx */
   short mxlFrmWVxx;       /* Max length of frame def w/Vxx */
#define  mxlVPort  (Lop + Li01)
#define  mxlVPCtl  (Lop + 3 + Li01)
#define  mxlWinCtl (Lop + 2 + Li01)
#define  mxlWinChg (Lop + Li01)
#define  mxlGMode  (Lop + 4)
#define  mxlState  (Lop + 12 + 5*Li01)
   };

/* Complete state of the graphics package */
struct RKGdef {
   Frame *pF0;             /* Ptr to array of Frames */
   Frame *pafr;            /* Ptr to list of available frames */
   Frame *pcf;             /* Ptr to currently active frame */
   Win   *pW0;             /* Ptr to array of Windefs */
   Win   *pawn;            /* Ptr to list of available windows */
   Win   *pcw;             /* Ptr to currently active window */
#ifndef PARn
   RFDTYPE *MFfdes;        /* Metafile file descriptor */
   char  *MFfn;            /* Metafile file name */
   char  *MFDdisplay;      /* X-windows display */
   char  *MFDwicon;        /* X-windows window icon */
   char  *MFDwtitl;        /* X-windows window title */
#ifndef PAR                /* Totcount is on mfsr if PAR */
   unsigned long totcount; /* Total metafile length */
#endif
   pid_t MFDpid;           /* MFDraw process id */
   int   MFDsock;          /* Socket to MFDraw */
#endif
   struct RKGSdef s;       /* Shared data */
   ui16  cfrm;             /* Current frame */
   ui16  cwin;             /* Current window */
   ui16  nfallo,nwallo;    /* Number frames, windows allocated */
   ui16  nfused,nwused;    /* Number frames, windows used */
   ui16  nw2allo;          /* Number windows to allocate next */
   ui32  nxtcsyn;          /* Next color synonym */
   ui16  nxtfontid;        /* Next font id */
   ui16  nxtpenid;         /* Next pen id */
   byte  MFFlags;          /* Status flags, initially 0 */
#define MFF_HdrSent     4  /* TRUE if metafile header sent */
#define MFF_DidSetup    8  /* TRUE if rkginit() setup was done */
#define MFF_MFDrawX    16  /* TRUE if mfdraw already dead */
#define MFF_FirstBuff  32  /* TRUE if this is first buffer in plot */
#define MFF_DidIfin    64  /* TRUE if rkgifin() setup was done */
#define MFF_MEnded    128  /* TRUE if endplt() was executed */
   schr  MFPending;        /* Count of sgx frames sent, not ack'ed */
   /* For compatibility with old startup code, nexpose, movie_mode,
   *  and movie_device define the default movie mode.  In V2, each
   *  window starts with these defaults, but setmovie() calls or GUI
   *  action by user can vary them, as stored in Win structure.  */
   ui16  nexpose;          /* Number of exposures per frame */
   byte  movie_device;     /* Values defined in plotdefs.h */
   byte  movie_mode;       /* Default movie mode for new windows */
   struct butt msgbutt;    /* Button info from bpause or host */
   };

#ifndef I_AM_SETMF
extern
#endif
   struct RKGdef _RKG;

#endif   /* __MFINT_INCLUDED__ */
