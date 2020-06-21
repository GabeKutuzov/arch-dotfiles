/* (c) Copyright 20xx, The Rockefeller University *nnnnn* */
/***********************************************************************
*                      PLOTOPS for ROCKS plot library                  *
*                                PLOTOPS.H                             *
*                                                                      *
*  Header file containing definitions for operation codes used by      *
*  the new plot library, dumpmf and mfdraw.                            *
*                                                                      *
************************************************************************
*  Version 1, 00/00/19, GMK                                            *
***********************************************************************/

#ifndef __PLOTOPS__
#define __PLOTOPS__

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
#define AHIV          0       /* Additional header info. version */

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
#define OpNNode  0x1F      /* Number of nodes */
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

#endif /* ifndef __PLOTOPS */
