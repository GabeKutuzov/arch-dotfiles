/* (c) Copyright 1972-1993, The Rockefeller University *21114* */
/***********************************************************************
*                                                                      *
*                               SYMBOL                                 *
*                                                                      *
*     Device-independent symbol plotting routine based on CALCOMP      *
*  IBM370 Assembler routine of same name.                              *
*                                                                      *
*  N.B.:  This routine assumes the character string to be plotted      *
*     is expressed in the native character set of the host system.     *
*     At present, this is EBCDIC for IBM370 and ASCII for all others.  *
*                                                                      *
************************************************************************
*                                                                      *
*  Usage: void symbol2(float xpt, float ypt, float hl,                 *
*           char *bcd, float theta, short ns)                          *
*                                                                      *
*     xpt,ypt - - X,Y coordinates where the label is to start.         *
*     hl  - - - - Height of the letters, in inches.  If negative or    *
*                 zero, use height and angle from previous call.       *
*                 If negative, concatenate to previous string.         *
*     bcd - - - - The actual label to be plotted, normally encoded as  *
*                 a quoted string in C.  Certain special characters    *
*                 are available and are encoded as values which do not *
*                 correspond to printable characters in the character  *
*                 code being used (e.g. control chars).  These may be  *
*                 system-dependent and are documented in the character *
*                 tables below.  These may be encoded in the calling   *
*                 program as an escaped hex character or as an integer *
*                 value in a byte variable.  Note that for historical  *
*                 reasons the length of the string must be encoded in  *
*                 the 'ns' parameter, and the normal C end-of-string   *
*                 indication is ignored.                               *
*     theta - - - The angle of the base line of the label above the    *
*                 horizontal, in degrees.                              *
*     ns  - - - - The length of the bcd string.  If ns is zero or      *
*                 negative, a single centered symbol is drawn.  If     *
*                 ns is 0 or -1, the pen is up when moving to the      *
*                 location of the symbol, otherwise the pen is down.   *
*                                                                      *
************************************************************************
*  V1A, 06/16/72, GNR - Modified from CALCOMP version for ORTEP.       *
*  V1B, 03/06/80, GNR - Restored to original for ROCKS.                *
*  V1C, 07/12/84, CMF - Restored original order of centered symbols.   *
*                       Made following centered symbols symmetric:     *
*                          square, octagon, triangle, diamond, rayed   *
*                          square, Z, small square.  Removed extra     *
*                          lines from '0', 'O', 'Z', and vertical bar. *
*  V2A, 05/13/90, GNR - Convert from IBM Assembler to C.  Rearrange    *
*                          character tables slightly.                  *
*  V2B, 06/15/90, GNR - Add lowercase letters, clean up code.          *
*  V2C, 06/13/92, GNR & ROZ - Add metafile                             *                                                                    *
*  Rev, 07/15/93, GNR - Use mfbchk()                                   *
***********************************************************************/

#include "glu.h"
#ifdef METAFILE
#include "rocks.h"
#include "rkxtra.h"
#include "glu.h"
#endif

/* Prototype from plots.h to avoid xucc table overflow */
void plot(float x,float y,int ipen);

#ifndef SUN4
#ifndef NOGDEV
/* Configuration parameters */
#define MAXOFF 12             /* Size of coord table
                                 ( = highest offset used plus one */
#define NSPEC  13             /* Number of special characters */
#define PTFAC   0.25          /* Scale factor for dots (i,j,!,?) */
#define SSFAC   0.7           /* Scale factor for sub, superscripts */
#define SSOFF   2             /* Offset for sub, superscripts */
#define CSPACE  6             /* Character spacing (x offset) */
#define CTROFF  2             /* Offset to center of centered chars */

/* Special function codes */
/*    Caution: The code makes use of the particular hex values of */
/*             the PEN_UP and DOT codes--do not change arbitrarily */
#define PEN_UP  0xf0          /* Pen up code */
#define DOT     0xf1          /* Draw dot */
#define DO_BSP  1             /* Backspace */
#define DO_CRLF 2             /* Carriage return, line feed */
#define DO_SUP  3             /* Enter superscript state */
#define DO_SUB  4             /* Enter subscript state */
#define SMALLSQ 6             /* Small centered square */
#define SUBSC   0             /* Subscript state */
#define NORMAL  1             /* Normal text state */
#define SUPER   2             /* Superscript state */

/* Other defined constants */
#define RADS    0.01745329252 /* Degrees to radians */

/* Global declarations: */

/* Xtable and Ytable:

   xv[n] contains n*cos(theta)*step size and
   yv[n] contains n*sin(theta)*step size

   Certain offsets in these tables used for particular purposes
   are defined above.  The size of both tables is MAXOFF, which
   must be increased if any characters larger than the present
   MAXOFF (in either x or y) is added to this program.  x=15 is
   used to encode special actions, therefore no character may
   use this value as an actual x coordinate.  The tables are
   expected to be initialized to zero by the compiler. */

static float xv[MAXOFF];      /* Tabulated values of X */
static float yv[MAXOFF];      /* Tabulated values of Y */
static float stepht;          /* Height of one x,y step */
static float incc;            /* Cos(theta) */
static float incs;            /* Sin(theta) */
static char state = NORMAL;   /* Subscript/superscript state */

/***********************************************************************
*                                                                      *
*                       Character string table                         *
*                                                                      *
*     This table is constructed as a structure purely to guarantee     *
*  that the various strings are adjacent in memory.  Each byte         *
*  encodes an x,y pair of coordinates.  Code 0xf0 signifies that       *
*  the pen should be lifted to generate a discontinuous symbol.        *
*  Code 0xf1 signifies that a dot is to be drawn at current position.  *
*                                                                      *
*     Unfortunately, the lengths of the strings have to be given       *
*  explicitly to satisfy the C rules for structure initialization.     *
*  In order to save space in the character definition table, just      *
*  the offsets of the character strings are stored, rather than        *
*  full pointers to them.  For this to work, the strings must be       *
*  adjacent in memory, and it is assumed that this will be so.         *
*                                                                      *
*     The first NSPEC characters end with a PEN_UP and a move to 2,2   *
*  so that when a line is drawn to the next character, it will begin   *
*  at the center of the previous symbol, thus drawing a curve.         *
*                                                                      *
***********************************************************************/

static struct Stable {
   char s0[7];                /* Centered square */
   char s1[11];               /* Centered octagon */
   char s2[5];                /* Centered triangle */
   char s3[6];                /* Centered plus */
   char s4[7];                /* Centered X */
   char s5[7];                /* Centered diamond */
   char s8[9];                /* Centered Z */
   char s9[7];                /* Centered Y */
   char sa[13];               /* Centered rayed square */
   char sc[5];                /* Centered double bar X */
   char sd[3];                /* Centered vertical */
   char sg[2];                /* Vertical vector */
   char sj[6];                /* Equivalence */
   char sn[3];                /* Plus Minus */
   char ss[5];                /* Plus */
   char sm[8];                /* Not equal */
   char sb[4];                /* Less than or equal */
   char sx[3];                /* Greater than or equal */
   char sy[3];                /* Greater than */
   char t0[4];                /* Left bracket */
   char rb[4];                /* Right bracket */
   char t1[1];                /* L */
   char sf[2];                /* Horizontal vector */
   char t2[7];                /* E */
   char t3[3];                /* Reverse slash */
   char t4[2];                /* Slash */
   char ta[9];                /* A */
   char tb[6];                /* B */
   char td[7];                /* D */
   char tc[10];               /* C */
   char th[6];                /* H */
   char ti[6];                /* I */
   char te[11];               /* Cents */
   char tf[6];                /* Colon */
   char tg[6];                /* Period */
   char t9[4];                /* Left paren */
   char sq[2];                /* Vertical */
   char sr[10];               /* Ampersand */
   char ba[4];                /* Back apostrophe */
   char tu[1];                /* U */
   char tj[5];                /* J */
   char tk[7];                /* K */
   char tm[3];                /* M */
   char tn[2];                /* N */
   char tw[5];                /* W */
   char to[9];                /* 0 */
   char tq[12];               /* O,Q */
   char tp[10];               /* P */
   char st[4];                /* Exclamation */
   char su[10];               /* Question mark */
   char tt[2];                /* T */
   char tr[12];               /* Dollar sign */
   char tx[8];                /* Asterisk */
   char na[4];                /* Right paren */
   char nb[3];                /* Not */
   char n8[2];                /* 8 */
   char ts[14];               /* S */
   char tv[3];                /* V */
   char ty[5];                /* Y */
   char tz[4];                /* Z */
   char nc[14];               /* Per cent */
   char nd[2];                /* Dash */
   char n1[5];                /* 1 */
   char n2[8];                /* 2 */
   char n3[13];               /* 3 */
   char n6[12];               /* 6 */
   char n4[6];                /* 4 */
   char n5[7];                /* 5 */
   char n7[5];                /* 7 */
   char n9[12];               /* 9 */
   char ne[11];               /* Pound sign */
   char nf[16];               /* At sign */
   char ng[9];                /* Apostrophe, quotation */
   char no[17];               /* Alpha, infinity */
   char se[6];                /* Star */
   char t6[5];                /* Square root */
   char sh[3];                /* Down arrow */
   char si[5];                /* Up arrow */
   char sk[5];                /* Right arrow */
   char sl[3];                /* And */
   char so[2];                /* Underscore */
   char sp[3];                /* Double underscore */
   char sw[14];               /* Divide */
   char sv[5];                /* Summation */
   char sz[4];                /* DELTA */
   char t5[7];                /* Gamma */
   char t7[13];               /* Double dagger */
   char t8[5];                /* Left arrow */
   char nh[8];                /* Integral */
   char ni[5];                /* Times */
   char nj[6];                /* Implies */
   char nk[6];                /* Inclusion */
   char nl[13];               /* Tilde */
   char nm[9];                /* Right brace */
   char nn[9];                /* Left brace */
   char np[9];                /* Mu */
   char nq[6];                /* Pi */
   char nt[8];                /* Psi */
   char nr[3];                /* Phi */
   char ns[12];               /* Theta */
   char nu[7];                /* Chi */
   char u0[1];                /* Blank */
   char nv[11];               /* Omega */
   char nw[7];                /* Lambda */
   char nx[14];               /* Delta */
   char ny[9];                /* Epsilon */
   char nz[8];                /* Eta */
   char tl[2];                /* Overscore */
   char ca[3];                /* Caret */
   char la[11];               /* Lowercase a */
   char lb[10];               /* Lowercase b */
   char le[10];               /* Lowercase c,e */
   char ld[10];               /* Lowercase d */
   char lf[8];                /* Lowercase f */
   char lg[13];               /* Lowercase g */
   char lh[7];                /* Lowercase h */
   char li[6];                /* Lowercase i */
   char lj[6];                /* Lowercase j */
   char lk[6];                /* Lowercase k */
   char ll[4];                /* Lowercase l */
   char lm[10];               /* Lowercase m */
   char ln[7];                /* Lowercase n */
   char lo[9];                /* Lowercase o */
   char lp[10];               /* Lowercase p */
   char lq[10];               /* Lowercase q */
   char lr[5];                /* Lowercase r */
   char ls[10];               /* Lowercase s */
   char lt[7];                /* Lowercase t */
   char lu[7];                /* Lowercase u */
   char lv[3];                /* Lowercase v */
   char lw[9];                /* Lowercase w */
   char ly[6];                /* Lowercase y */
   char lz[4];                /* Lowercase z */
   } S = {
      {'\x04','\x00','\x40','\x44','\x04',PEN_UP,'\x22'},       /* s0 */
      {'\x14','\x03','\x01','\x10','\x30','\x41','\x43','\x34',
       '\x14',PEN_UP,'\x22'},                                   /* s1 */
      {'\x01','\x41','\x24','\x01',PEN_UP},                     /* s2 */
      {'\x22','\x24','\x20','\x22','\x02','\x42'},              /* s3 */
      {'\x22','\x04','\x40','\x22','\x00','\x44','\x22'},       /* s4 */
      {'\x02','\x20','\x42','\x24','\x02',PEN_UP,'\x22'},       /* s5 */
      {'\x12','\x32','\x22','\x44','\x04','\x44','\x00','\x40',
       '\x00'},                                                 /* s8 */
      {'\x22','\x04','\x22','\x44','\x22','\x20','\x22'},       /* s9 */
      {'\x44','\x33','\x13','\x04','\x13','\x11','\x00',
       '\x11','\x31','\x40','\x31','\x33',PEN_UP},              /* sa */
      {'\x22','\x44','\x04','\x40','\x00'},                     /* sc */
      {'\x22','\x24','\x20'},                                   /* sd */
      {'\x22','\x26'},                                          /* sg */
      {'\x27','\x67',PEN_UP,'\x65','\x25',PEN_UP},              /* sj */
      {'\x23','\x63',PEN_UP},                                   /* sn */
      {'\x44','\x48','\x46','\x26','\x66'},                     /* ss */
      {'\x24','\x64',PEN_UP,'\x66','\x26',PEN_UP,'\x33','\x57'},/* sm */
      {'\x68','\x26','\x64',PEN_UP},                            /* sb */
      {'\x63','\x23',PEN_UP},                                   /* sx */
      {'\x24','\x66','\x28'},                                   /* sy */
        {'\x69','\x49','\x42','\x62'},                            /* t0 */
      {'\x29','\x49','\x42','\x22'},                            /* rb */
        {'\x29'},                                                 /* t1 */
        {'\x22','\x62'},                                          /* sf */
        {'\x69','\x29','\x26','\x56','\x26','\x22','\x62'},       /* t2 */
        {'\x29','\x62',PEN_UP},                                   /* t3 */
      {'\x22','\x69'},                                          /* t4 */
        {'\x22','\x25','\x65','\x25','\x28','\x39','\x59','\x68',
       '\x62'},                                                 /* ta */
        {'\x63','\x65','\x56','\x26','\x56','\x67'},              /* tb */
        {'\x68','\x59','\x29','\x22','\x52','\x63','\x68'},       /* td */
        {'\x68','\x59','\x39','\x28','\x23','\x32','\x52','\x63',
       '\x65','\x55'},                                          /* tc */
        {'\x22','\x29','\x26','\x66','\x69','\x62'},              /* th */
        {'\x32','\x52','\x42','\x49','\x39','\x59'},              /* ti */
        {'\x65','\x56','\x36','\x25','\x23','\x32','\x52','\x63',
       PEN_UP,'\x41','\x47'},                                   /* te */
        {'\x35','\x36','\x46','\x45','\x35',PEN_UP},              /* tf */
        {'\x42','\x32','\x33','\x43','\x42','\x31'},              /* tg */
        {'\x69','\x58','\x53','\x62'},                            /* t9 */
        {'\x41','\x4a'},                                          /* sq */
        {'\x62','\x37','\x38','\x49','\x58','\x25','\x24','\x33',
       '\x43','\x64'},                                          /* sr */
      {'\x57','\x49','\x59','\x57'},                            /* ba */
        {'\x29'},                                                 /* tu */
        {'\x23','\x32','\x52','\x63','\x69'},                     /* tj */
                {'\x29','\x22','\x26','\x69',PEN_UP,'\x62','\x26'},       /* tk */
                {'\x22','\x29','\x45'},                                   /* tm */
                {'\x69','\x62'},                                          /* tn */
                {'\x29','\x22','\x46','\x62','\x69'},                     /* tw */
                {'\x27','\x39','\x59','\x67','\x64','\x52','\x32','\x24',
       '\x27'},                                                 /* to */
                {'\x68','\x59','\x39','\x28','\x23','\x32','\x52','\x63',
       '\x68',PEN_UP,'\x44','\x62'},                            /* tq */
                {'\x22','\x29','\x59','\x68','\x67','\x56','\x26','\x56',
       '\x65','\x62'},                                          /* tp */
                {'\x49','\x44',  DOT ,'\x42'},                            /* st */
                {'\x28','\x39','\x59','\x68','\x67','\x56','\x46','\x44',
         DOT ,'\x42'},                                          /* su */
                {'\x29','\x69'},                                          /* tt */
                {'\x49','\x42',PEN_UP,'\x23','\x53','\x64','\x65','\x56',
       '\x36','\x27','\x38','\x68'},                            /* tr */
                {'\x25','\x65','\x45','\x63','\x27','\x45','\x23','\x67'},/* tx */
                {'\x29','\x38','\x33','\x22'},                            /* na */
                {'\x25','\x75','\x74'},                                   /* nb */
                {'\x56','\x67'},                                          /* n8 */
                {'\x68','\x59','\x39','\x28','\x27','\x36','\x56','\x65',
       '\x63','\x52','\x32','\x23','\x25','\x36'},              /* ts */
                {'\x29','\x42','\x69'},                                   /* tv */
                {'\x29','\x46','\x42','\x46','\x69'},                     /* ty */
                {'\x29','\x69','\x22','\x62'},                            /* tz */
                {'\x38','\x28','\x29','\x39','\x38',PEN_UP,'\x69','\x22',
       PEN_UP,'\x53','\x63','\x62','\x52','\x53'},              /* nc */
                {'\x15','\x75'},                                          /* nd */
                {'\x38','\x49','\x42','\x32','\x52'},                     /* n1 */
                {'\x28','\x39','\x59','\x68','\x66','\x24','\x22','\x62'},/* n2 */
                {'\x28','\x39','\x59','\x68','\x67','\x56','\x36','\x56',
       '\x65','\x63','\x52','\x32','\x23'},                     /* n3 */
                {'\x25','\x36','\x56','\x65','\x63','\x52','\x32','\x23',
       '\x28','\x39','\x59','\x68'},                            /* n6 */
                {'\x64','\x24','\x59','\x52','\x42','\x62'},              /* n4 */
                {'\x23','\x32','\x52','\x63','\x65','\x56','\x26'},       /* n5 */
                {'\x29','\x69','\x68','\x43','\x42'},                     /* n7 */
                {'\x23','\x32','\x52','\x63','\x68','\x59','\x39','\x28',
       '\x26','\x35','\x55','\x66'},                            /* n9 */
                {'\x24','\x64','\x54','\x52','\x58','\x56','\x66','\x26',
       '\x36','\x38','\x32'},                                   /* ne */
                {'\x66','\x57','\x47','\x36','\x35','\x44','\x54','\x65',
       '\x67','\x58','\x38','\x27','\x23','\x32','\x52','\x63'},/* nf */
                {'\x47','\x49','\x59','\x47',PEN_UP,'\x27','\x29','\x39',
       '\x27'},                                                 /* ng */
                {'\x66','\x56','\x45','\x36','\x26','\x15','\x14','\x23',
                 '\x33','\x44','\x45','\x44','\x53','\x63','\x74','\x75',
       '\x66'},                                                 /* no */
                {'\x22','\x48','\x62','\x16','\x76','\x22'},              /* se */
                {'\x23','\x34','\x42','\x5a','\x7a'},                     /* t6 */
                {'\x42','\x34','\x54'},                                   /* sh */
                {'\x42','\x49','\x37','\x57','\x49'},                     /* si */
                {'\x15','\x75','\x56','\x54','\x75'},                     /* sk */
                {'\x22','\x46','\x62'},                                   /* sl */
                {'\x11','\x71'},                                          /* so */
                {PEN_UP,'\x10','\x70'},                                   /* sp */
                {'\x15','\x65',PEN_UP,'\x33','\x34','\x44','\x43','\x33',
       PEN_UP,'\x36','\x37','\x47','\x46','\x36'},              /* sw */
                {'\x71','\x11','\x56','\x1a','\x7a'},                     /* sv */
                {'\x12','\x47','\x72','\x12'},                            /* sz */
                {'\x28','\x39','\x48','\x42','\x48','\x59','\x68'},       /* t5 */
                {'\x42','\x44','\x34','\x54','\x44','\x48','\x38','\x58',
       '\x48','\x49','\x46','\x36','\x56'},                     /* t7 */
                {'\x15','\x36','\x34','\x15','\x75'},                     /* t8 */
                {'\x12','\x21','\x31','\x42','\x49','\x5a','\x6a','\x79'},/* nh */
                {'\x22','\x66',PEN_UP,'\x26','\x62'},                     /* ni */
                {'\x24','\x54','\x65','\x66','\x57','\x27'},              /* nj */
                {'\x64','\x34','\x25','\x26','\x37','\x67'},              /* nk */
                {'\x14','\x25','\x35','\x53','\x63','\x74',PEN_UP,'\x76',
       '\x65','\x55','\x37','\x27','\x16'},                     /* nl */
                {'\x21','\x31','\x42','\x45','\x56','\x47','\x4a','\x3b',
       '\x2b'},                                                 /* nm */
                {'\x61','\x51','\x42','\x45','\x36','\x47','\x4a','\x5b',
       '\x6b'},                                                 /* nn */
                {'\x21','\x36','\x33','\x42','\x52','\x63','\x66','\x63',
       '\x72'},                                                 /* np */
                {'\x32','\x36','\x26','\x66','\x56','\x52'},              /* nq */
                {'\x25','\x35','\x33','\x42','\x53','\x55','\x65',PEN_UP},/* nt */
                {'\x41','\x47',PEN_UP},                                   /* nr */
                {'\x36','\x25','\x23','\x32','\x52','\x63','\x65','\x56',
       '\x36',PEN_UP,'\x24','\x64'},                            /* ns */
                {'\x22','\x65',PEN_UP,'\x25','\x35','\x52','\x62'},       /* nu */
                {PEN_UP},                                                 /* u0 */
                {'\x36','\x25','\x23','\x32','\x43','\x45','\x43','\x52',
       '\x63','\x65','\x56'},                                   /* nv */
                {'\x28','\x39','\x46','\x62',PEN_UP,'\x22','\x46'},       /* nw */
                {'\x68','\x59','\x39','\x28','\x27','\x36','\x56','\x65',
       '\x63','\x52','\x32','\x23','\x25','\x36'},              /* nx */
                {'\x66','\x46','\x35','\x33','\x42','\x62',PEN_UP,'\x64',
       '\x34'},                                                 /* ny */
                {'\x25','\x36','\x45','\x33','\x44','\x56','\x65','\x51'},/* nz */
                {'\x1a','\x7a'},                                          /* tl */
      {'\x26','\x4a','\x66'},                                   /* ca */
      {'\x66','\x63','\x52','\x32','\x23','\x25','\x36','\x56',
       '\x65','\x63','\x72'},                                   /* la */
      {'\x29','\x22','\x25','\x36','\x56','\x65','\x63','\x52',
       '\x32','\x23'},                                          /* lb */
      {'\x63','\x52','\x32','\x23','\x25','\x36','\x56','\x65',
       '\x64','\x24'},                                          /* le */
      {'\x69','\x62','\x65','\x56','\x36','\x25','\x23','\x32',
       '\x52','\x63'},                                          /* ld */
      {'\x25','\x55',PEN_UP,'\x68','\x59','\x49','\x38','\x32'},/* lf */
      {'\x21','\x30','\x50','\x61','\x66','\x63','\x52','\x32',
       '\x23','\x25','\x36','\x56','\x65'},                     /* lg */
      {'\x29','\x22','\x25','\x36','\x56','\x65','\x62'},       /* lh */
      {'\x53','\x42','\x33','\x36',  DOT ,'\x38'},              /* li */
      {'\x21','\x30','\x41','\x46',  DOT ,'\x48'},              /* lj */
      {'\x29','\x22','\x25','\x58','\x25','\x62'},              /* lk */
      {'\x53','\x42','\x33','\x39'},                            /* ll */
      {'\x26','\x22','\x25','\x36','\x45','\x42','\x45','\x56',
       '\x65','\x62'},                                          /* lm */
      {'\x26','\x22','\x25','\x36','\x56','\x65','\x62'},       /* ln */
      {'\x25','\x23','\x32','\x52','\x63','\x65','\x56','\x36',
       '\x25'},                                                 /* lo */
      {'\x26','\x23','\x32','\x52','\x63','\x65','\x56','\x36',
       '\x25','\x20'},                                          /* lp */
      {'\x66','\x63','\x52','\x32','\x23','\x25','\x36','\x56',
       '\x65','\x60'},                                          /* lq */
      {'\x26','\x22','\x25','\x36','\x56'},                     /* lr */
      {'\x23','\x32','\x52','\x63','\x54','\x34','\x25','\x36',
       '\x56','\x65'},                                          /* ls */
      {'\x39','\x33','\x42','\x53',PEN_UP,'\x26','\x56'},       /* lt */
      {'\x26','\x23','\x32','\x52','\x63','\x66','\x62'},       /* lu */
                {'\x26','\x42','\x66'},                                   /* lv */
      {'\x26','\x23','\x32','\x43','\x46','\x43','\x52','\x63',
       '\x66'},                                                 /* lw */
      {'\x26','\x42',PEN_UP,'\x66','\x30','\x21'},              /* ly */
      {'\x26','\x66','\x22','\x62'},                            /* lz */

   }; /* End character string table */

/***********************************************************************
*                                                                      *
*                     Character definition tables                      *
*                                                                      *
***********************************************************************/

/* The following macro calculates the offset of the argument string
   in the Stable structure above */

#define STO(st) (((struct Stable *)0)->st - ((struct Stable *)0)->s0)

static struct Ctable {        /* Character table entry */
   short so;                     /* String offset */
   short sl;                     /* String length */
   } CT[] = {                 /* Character definitions follow:
                                 (The first NSPEC characters can be
                                 plotted as centered characters) */

#ifdef IBM370                 /* EBCDIC Character Table */

                              /* Dec. Hex. Character           */
      STO(s0),7,              /*   0   00  Center square       */
      STO(s1),11,             /*   1   01  Center octagon      */
      STO(s2),6,              /*   2   02  Center triangle     */
      STO(s3),7,              /*   3   03  Center plus         */
      STO(s4),7,              /*   4   04  Center x            */
      STO(s5),7,              /*   5   05  Center diamond      */
      STO(s0),7,              /*   6   06  Center square--this
                                           gets scaled to small dot */
      STO(s8),10,             /*   7   07  Center z            */
      STO(s9),7,              /*   8   08  Center y            */
      STO(sa),14,             /*   9   09  Center rayed square */
      STO(s3),13,             /*  10   0a  Center asterisk     */
      STO(sc),6,              /*  11   0b  Center double bar x */
      STO(sd),4,              /*  12   0c  Center vertical     */
      STO(sl),3               /*  13   0d  And                 */
      STO(se),6,              /*  14   0e  Star                */
      STO(sf),2,              /*  15   0f  Horiz vector        */
      STO(sg),2,              /*  16   10  Vertical vector     */
      NULL,-(DO_BSP),         /*  17   11  ** Backspace        */
      STO(sl),3,              /*  18   12  And                 */
      STO(sj),8,              /*  19   13  Equivelence         */
      STO(sk),5,              /*  20   14  Right arrow         */
      NULL,-(DO_CRLF),        /*  21   15  ** Carriage return  */
      STO(sm),8,              /*  22   16  Not equal           */
      STO(sn),8,              /*  23   17  Plus minus          */
      STO(so),2,              /*  24   18  Underscore          */
      STO(so),5,              /*  25   19  Double underscore   */
      STO(tl),2,              /*  26   1a  Overscore           */
      STO(nh),8,              /*  27   1b  Integral            */
      STO(nj),6,              /*  28   1c  Implies             */
      STO(nk),6,              /*  29   1d  Inclusion           */
      STO(nl),6,              /*  30   1e  Squiggle            */
      STO(nl),13,             /*  31   1f  Double squiggle     */
      STO(nm),9,              /*  32   20  Right brace         */
      STO(nn),9,              /*  33   21  Left  brace         */
      STO(np),9,              /*  34   22  Mu                  */
      STO(nq),6,              /*  35   23  Pi                  */
      STO(nr),12,             /*  36   24  Phi                 */
      STO(ns),12,             /*  37   25  Theta               */
      STO(nt),10,             /*  38   26  Psi                 */
      STO(nu),7,              /*  39   27  Chi                 */
      STO(nv),11,             /*  40   28  Omega               */
      STO(nw),7,              /*  41   29  Lambda              */
      STO(no),14,             /*  42   2a  Alpha               */
      STO(nx),14,             /*  43   2b  Delta               */
      STO(ny),9,              /*  44   2c  Epsilon             */
      STO(nz),8,              /*  45   2d  Eta                 */
      NULL,-(DO_SUP),         /*  46   2e  ** Superscript      */
      NULL,-(DO_SUB),         /*  47   2f  ** Subscript        */
      STO(sv),5,              /*  48   30  Sumation            */
      STO(sw),14,             /*  49   31  Divide              */
      STO(sb),6,              /*  50   32  Less than or equal  */
      STO(sx),6,              /*  51   33  Greater  or equal   */
      STO(sz),4,              /*  52   34  Delta               */
      STO(t0),4,              /*  53   35  Left  bracket       */
      STO(rb),4,              /*  54   36  Right bracket       */
      STO(t3),2,              /*  55   37  Reverse slash       */
      STO(t5),7,              /*  56   38  Gamma               */
      STO(t6),5,              /*  57   39  Square root         */
      STO(t7),10,             /*  58   3a  Double dagger       */
      STO(t7),13,             /*  59   3b  Triple dagger       */
      STO(t8),5,              /*  60   3c  Left arrow          */
      STO(ni),5,              /*  61   3d  Times               */
      STO(si),5,              /*  62   3e  Ip arrow            */
      STO(sh),5,              /*  63   3f  Down arrow          */
      STO(u0),1,              /*  64   40  Blank               */
      STO(ta),9,              /*  65   41  A                   */
      STO(tb),12,             /*  66   42  B                   */
      STO(tc),8,              /*  67   43  C                   */
      STO(td),7,              /*  68   44  D                   */
      STO(t2),7,              /*  69   45  E                   */
      STO(t2),6,              /*  70   46  F                   */
      STO(tc),10,             /*  71   47  G                   */
      STO(th),6,              /*  72   48  H                   */
      STO(ti),6,              /*  73   49  I                   */
      STO(te),11,             /*  74   4a  Cent                */
      STO(tg),5,              /*  75   4b  Period              */
      STO(sb),3,              /*  76   4c  Less                */
      STO(t9),4,              /*  77   4d  Left paren          */
      STO(ss),5,              /*  78   4e  Plus                */
      STO(sq),2,              /*  79   4f  Vertical            */
      STO(sr),10,             /*  80   50  Ampersand           */
      STO(tj),5,              /*  81   51  J                   */
      STO(tk),7,              /*  82   52  K                   */
      STO(t1),3,              /*  83   53  L                   */
      STO(tm),5,              /*  84   54  M                   */
      STO(tn),4,              /*  85   55  N                   */
      STO(tq),9,              /*  86   56  O                   */
      STO(tp),7,              /*  87   57  P                   */
      STO(tq),12,             /*  88   58  Q                   */
      STO(tp),10,             /*  89   59  R                   */
      STO(st),4,              /*  90   5a  Exclamation         */
      STO(tr),12,             /*  91   5b  Dollar sign         */
      STO(tx),8,              /*  92   5c  Asterisk            */
      STO(na),4,              /*  93   5d  Right parenthesis   */
      STO(tf),12,             /*  94   5e  Semi colon          */
      STO(nb),3,              /*  95   5f  Not                 */
      STO(tx),2,              /*  96   60  Minus               */
      STO(t4),2,              /*  97   61  Slash               */
      STO(ts),12,             /*  98   62  S                   */
      STO(tt),4,              /*  99   63  T                   */
      STO(tu),6,              /* 100   64  U                   */
      STO(tv),3,              /* 101   65  V                   */
      STO(tw),5,              /* 102   66  W                   */
      STO(t3),5,              /* 103   67  X                   */
      STO(ty),5,              /* 104   68  Y                   */
      STO(tz),4,              /* 105   69  Z                   */
      STO(no),17,             /* 106   6a  Infinity            */
      STO(tg),6,              /* 107   6b  Comma               */
      STO(nc),14,             /* 108   6c  Per cent            */
      STO(nd),2,              /* 109   6d  Dash                */
      STO(sy),3,              /* 110   6e  Greater than        */
      STO(su),10,             /* 111   6f  Question mark       */
      STO(to),9,              /* 112   70  0                   */
      STO(n1),5,              /* 113   71  1                   */
      STO(n2),8,              /* 114   72  2                   */
      STO(n3),13,             /* 115   73  3                   */
      STO(n4),6,              /* 116   74  4                   */
      STO(n5),9,              /* 117   75  5                   */
      STO(n6),12,             /* 118   76  6                   */
      STO(n7),5,              /* 119   77  7                   */
      STO(n8),16,             /* 120   78  8                   */
      STO(n9),12,             /* 121   79  9                   */
      STO(tf),11,             /* 122   7a  Colon               */
      STO(ne),11,             /* 123   7b  Pound               */
      STO(nf),16,             /* 124   7c  At                  */
      STO(ng),4,              /* 125   7d  Apostrophe          */
      STO(sm),5,              /* 126   7e  Equal               */
      STO(ng),9,              /* 127   7f  Quotations          */

#else                         /* ASCII character table */

/********************************************************************/
/***                                                              ***/
/***  The following arbitrary decisions have been made to map     ***/
/***  the above characters onto the ASCII set:                    ***/
/***     (1) The ASCII printing characters will map as expected.  ***/
/***     (2) The centered characters, the first 14 above, will    ***/
/***         map to codes 0-13 as in EBCDIC, as programs may      ***/
/***         invoke them via their integer values.                ***/
/***     (3) A consequence of (2) is that backspace and carriage  ***/
/***         return cannot be mapped to their expected values.    ***/
/***         These and the remaining special action codes         ***/
/***         (subscript, superscript) are mapped to alternative   ***/
/***         control characters in the range 0x10-0x1f.           ***/
/***     (4) The up,down,left,right arrows are mapped to their    ***/
/***         positions in the IBM PC character set.               ***/
/***     (5) Greek letters and other special characters are       ***/
/***         mapped to positions following character 0xff in      ***/
/***         the ASCII set.                                       ***/
/***                                                              ***/
/********************************************************************/

                              /* Dec. Hex. Character           */
      STO(s0),7,              /*   0   00  Center square       */
      STO(s1),11,             /*   1   01  Center octagon      */
      STO(s2),6,              /*   2   02  Center triangle     */
      STO(s3),7,              /*   3   03  Center plus         */
      STO(s4),7,              /*   4   04  Center x            */
      STO(s5),7,              /*   5   05  Center diamond      */
      STO(s0),7,              /*   6   06  Center square--this
                                           gets scaled to small dot */
      STO(s8),10,             /*   7   07  Center z            */
      STO(s9),7,              /*   8   08  Center y            */
      STO(sa),14,             /*   9   09  Center rayed square */
      STO(s3),13,             /*  10   0a  Center asterisk     */
      STO(sc),6,              /*  11   0b  Center double bar x */
      STO(sd),4,              /*  12   0c  Center vertical     */
      STO(sl),3,              /*  13   0d  And                 */
      STO(se),6,              /*  14   0e  Star                */
      STO(sf),2,              /*  15   0f  Horiz vector        */
      STO(sg),2,              /*  16   10  Vertical vector     */
      NULL,-(DO_BSP),         /*  17   11  ** Backspace        */
      NULL,-(DO_SUP),         /*  18   12  ** Superscript      */
      STO(sj),8,              /*  19   13  Equivelence         */
      NULL,-(DO_SUB),         /*  20   14  ** Subscript        */
      NULL,-(DO_CRLF),        /*  21   15  ** Carriage return  */
      STO(sm),8,              /*  22   16  Not equal           */
      STO(sn),8,              /*  23   17  Plus minus          */
      STO(si),5,              /*  24   18  Up arrow            */
      STO(sh),5,              /*  25   19  Down arrow          */
      STO(sk),5,              /*  26   1a  Right arrow         */
      STO(t8),5,              /*  27   1b  Left arrow          */
      STO(nj),6,              /*  28   1c  Implies             */
      STO(nk),6,              /*  29   1d  Inclusion           */
      STO(nl),6,              /*  30   1e  Squiggle            */
      STO(nl),13,             /*  31   1f  Double squiggle     */
      STO(u0),1,              /*  32   20  Blank               */
      STO(st),4,              /*  33   21  Exclamation         */
      STO(ng),9,              /*  34   22  Quotations          */
      STO(ne),11,             /*  35   23  Pound sign          */
      STO(tr),12,             /*  36   24  Dollar sign         */
      STO(nc),14,             /*  37   25  Per cent            */
      STO(sr),10,             /*  38   26  Ampersand           */
      STO(ng),4,              /*  39   27  Apostrope           */
      STO(t9),4,              /*  40   28  Left paren          */
      STO(na),4,              /*  41   29  Right parenthesis   */
      STO(tx),8,              /*  42   2a  Asterisk            */
      STO(ss),5,              /*  43   2b  Plus                */
      STO(tg),6,              /*  44   2c  Comma               */
      STO(tx),2,              /*  45   2d  Minus               */
      STO(tg),5,              /*  46   2e  Period              */
      STO(t4),2,              /*  47   2f  Slash               */
      STO(to),9,              /*  48   30  0                   */
      STO(n1),5,              /*  49   31  1                   */
      STO(n2),8,              /*  50   32  2                   */
      STO(n3),13,             /*  51   33  3                   */
      STO(n4),6,              /*  52   34  4                   */
      STO(n5),9,              /*  53   35  5                   */
      STO(n6),12,             /*  54   36  6                   */
      STO(n7),5,              /*  55   37  7                   */
      STO(n8),16,             /*  56   38  8                   */
      STO(n9),12,             /*  57   39  9                   */
      STO(tf),11,             /*  58   3a  Colon               */
      STO(tf),12,             /*  59   3b  Semi colon          */
      STO(sb),3,              /*  60   3c  Less than           */
      STO(sm),5,              /*  61   3d  Equal               */
      STO(sy),3,              /*  62   3e  Greater than        */
      STO(su),10,             /*  63   3f  Question mark       */
      STO(nf),16,             /*  64   40  At                  */
      STO(ta),9,              /*  65   41  A                   */
      STO(tb),12,             /*  66   42  B                   */
      STO(tc),8,              /*  67   43  C                   */
      STO(td),7,              /*  68   44  D                   */
      STO(t2),7,              /*  69   45  E                   */
      STO(t2),6,              /*  70   46  F                   */
      STO(tc),10,             /*  71   47  G                   */
      STO(th),6,              /*  72   48  H                   */
      STO(ti),6,              /*  73   49  I                   */
      STO(tj),5,              /*  74   4a  J                   */
      STO(tk),7,              /*  75   4b  K                   */
      STO(t1),3,              /*  76   4c  L                   */
      STO(tm),5,              /*  77   4d  M                   */
      STO(tn),4,              /*  78   4e  N                   */
      STO(tq),9,              /*  79   4f  O                   */
      STO(tp),7,              /*  80   50  P                   */
      STO(tq),12,             /*  81   51  Q                   */
      STO(tp),10,             /*  82   52  R                   */
      STO(ts),12,             /*  83   53  S                   */
      STO(tt),4,              /*  84   54  T                   */
      STO(tu),6,              /*  85   55  U                   */
      STO(tv),3,              /*  86   56  V                   */
      STO(tw),5,              /*  87   57  W                   */
      STO(t3),5,              /*  88   58  X                   */
      STO(ty),5,              /*  89   59  Y                   */
      STO(tz),4,              /*  90   5a  Z                   */
      STO(t0),4,              /*  91   5b  Left bracket        */
      STO(t3),2,              /*  92   5c  Backslash           */
      STO(rb),4,              /*  93   5d  Right bracket       */
      STO(ca),3,              /*  94   5e  Caret               */
      STO(so),2,              /*  95   5f  Underscore          */
      STO(ba),4,              /*  96   60  Back Apostrophe     */
      STO(la),11,             /*  97   61  a                   */
      STO(lb),10,             /*  98   62  b                   */
      STO(le),8,              /*  99   63  c                   */
      STO(ld),10,             /* 100   64  d                   */
      STO(le),10,             /* 101   65  e                   */
      STO(lf),8,              /* 102   66  f                   */
      STO(lg),13,             /* 103   67  g                   */
      STO(lh),7,              /* 104   68  h                   */
      STO(li),6,              /* 105   69  i                   */
      STO(lj),6,              /* 106   6a  j                   */
      STO(lk),6,              /* 107   6b  k                   */
      STO(ll),4,              /* 108   6c  l                   */
      STO(lm),10,             /* 109   6d  m                   */
      STO(ln),7,              /* 110   6e  n                   */
      STO(lo),9,              /* 111   6f  o                   */
      STO(lp),10,             /* 112   70  p                   */
      STO(lq),10,             /* 113   71  q                   */
      STO(lr),5,              /* 114   72  r                   */
      STO(ls),10,             /* 115   73  s                   */
      STO(lt),7,              /* 116   74  t                   */
      STO(lu),7,              /* 117   75  u                   */
      STO(lv),3,              /* 118   76  v                   */
      STO(lw),9,              /* 119   77  w                   */
      STO(ni),5,              /* 120   78  x                   */
      STO(ly),6,              /* 121   79  y                   */
      STO(lz),4,              /* 122   7a  z                   */
      STO(nn),9,              /* 123   7b  Left  brace         */
      STO(sq),2,              /* 124   7c  Vertical            */
      STO(nm),9,              /* 125   7d  Right brace         */
      STO(nl),6,              /* 126   7e  Tilde               */
                              /* Currently same as squiggle    */
      STO(s0),0,              /* 127   7f  Delete              */

/* Special characters mapped above the ASCII set */

      STO(so),5,              /* 128   80  Double underscore   */
      STO(tl),2,              /* 129   81  Overscore           */
      STO(nh),8,              /* 130   82  Integral            */
      STO(np),9,              /* 131   83  Mu                  */
      STO(nq),6,              /* 132   84  Pi                  */
      STO(nr),12,             /* 133   85  Phi                 */
      STO(ns),12,             /* 134   86  Theta               */
      STO(nt),10,             /* 135   87  Psi                 */
      STO(nu),7,              /* 136   88  Chi                 */
      STO(nv),11,             /* 137   89  Omega               */
      STO(nw),7,              /* 138   8a  Lambda              */
      STO(no),14,             /* 139   8b  Alpha               */
      STO(nx),14,             /* 140   8c  Delta               */
      STO(ny),9,              /* 141   8d  Epsilon             */
      STO(nz),8,              /* 142   8e  Eta                 */
      STO(sv),5,              /* 143   8f  Sumation            */
      STO(sw),14,             /* 144   90  Divide              */
      STO(sb),6,              /* 145   91  Less than or equal  */
      STO(sx),6,              /* 146   92  Greater  or equal   */
      STO(sz),4,              /* 147   93  Delta               */
      STO(t5),7,              /* 148   94  Gamma               */
      STO(t6),5,              /* 149   95  Square root         */
      STO(t7),10,             /* 150   96  Double dagger       */
      STO(t7),13,             /* 151   97  Triple dagger       */
      STO(ni),5,              /* 152   98  Times               */
      STO(te),11,             /* 153   99  Cents               */
      STO(nb),3,              /* 154   9a  Not                 */
      STO(no),17,             /* 155   9b  Infinity            */
      STO(nd),2,              /* 156   9c  Dash                */

#endif

      }; /* End character table definition */

/*---------------------------------------------------------------------*
*                                                                      *
*                               MKTABLE                                *
*                                                                      *
*     Make table of offset coordinates for plotting symbols            *
*                                                                      *
*---------------------------------------------------------------------*/

void mktable(float step) {

   register float xi = step*incc; /* X increment */
   register float yi = step*incs; /* Y increment */
   register int i;                /* Table index */

   xv[1] = xi;
   yv[1] = yi;

   for (i=2; i<MAXOFF; i++) {
      xv[i] = (xi += xv[1]);
      yv[i] = (yi += yv[1]);
      }

   } /* End mktable */
#endif
#endif

/*---------------------------------------------------------------------*
*                                                                      *
*                               SYMBOL                                 *
*                                                                      *
*---------------------------------------------------------------------*/

void symbol(float xpt, float ypt, float hl, char *bcd,
   float theta, int ns) {

   char *pbcd;                /* Keeps the value of bcd for later use */
   int nsym;                  /* Keeps the value of ns unchanged */

#ifdef METAFILE
   nsym = ns;
   pbcd = bcd;
   if (_NCG.s.MFmode) {
      mfbchk(MFS_SIZE(nsym));
      *_NCG.MFCurrPos++ = 'S';
      i2a(_NCG.MFCurrPos,(int)(xpt*1000.0),5,DECENCD);
      i2a(_NCG.MFCurrPos,(int)(ypt*1000.0),5,DECENCD);
      i2a(_NCG.MFCurrPos,(int)(hl*1000.0),5,DECENCD);
      i2a(_NCG.MFCurrPos,(int)(theta*1000.0),7,DECENCD);
      i2a(_NCG.MFCurrPos,nsym,3,DECENCD);
      memcpy((char *)_NCG.MFCurrPos,pbcd,nsym);
      _NCG.MFCurrPos += nsym;
      *_NCG.MFCurrPos++ = '\n';
      }
#endif

#ifndef SUN4
#ifndef NOGDEV
   if (_NCG.s.nc_graph) {
   int ic = 3;                /* Pen up/down control */
   int len;                   /* Length field from table */
   int ch;                    /* Current character */
   int is;                    /* Character counter */
   float xd,yd;               /* Dot x,y offset */
   float xt,yt;               /* Plotting coordinates */
   float xc,yc;               /* Coordinates of current character */
   float xo,yo;               /* Coordinates of line origin */
   float hf;                  /* Height factor */
   char *pc,*pc1;             /* Pointers to coords */
   register unsigned int ix,iy; /* Current integer seg coords */

#ifdef METAFILE
   _NCG.MFSilent = ON;
#endif

/* Determine action according to value of ns */

   if (ns > 0) hf = 7.0;      /* Normal text */
   else {                     /* Single character */
      if (ns < -1)
         plot(xpt,ypt,2);     /* Move to center to draw curve */
      ns = 1;                 /* Plot one character */
      if (*bcd <= NSPEC) {    /* Handle centered characters */
         if (*bcd == SMALLSQ)
            hf = 8.0;
         else
            hf = 4.0;
         } /* End centered characters */
      else  hf = 7.0;
      } /* End single characters */

/* Action dependent on height:  If negative or zero, a previous call
   to symbol set the height and angle, and this call is using the X
   and Y table values calculated for the previous call.  If zero,
   use previous height but new starting coords. */

   if (hl > 0.0) {            /* Make a new offset table */
      float angle = RADS*theta;
      incc = cos(angle);
      incs = sin(angle);
      state = NORMAL;         /* Enter normal text mode */
      mktable(stepht = hl/hf);
      }
   if (hl >= 0.0) {           /* Set new starting coordinates */
      xo = xc = xpt - xv[CTROFF] + yv[CTROFF];
      yo = yc = ypt - xv[CTROFF] - yv[CTROFF];
      }

/* Loop over the character string */

   for (is=0; is<ns; is++) {

      ch = *(byte *)bcd++;    /* Get next character */
#ifdef IBM370
      ch &= 0x7f;             /* If EBCDIC, fold into supported range */
#endif

/* Handle special action codes.  These are encoded as negative length
   values in the character table */

      if ((len = CT[ch].sl) < 0) {

         switch (-len) {

         case DO_BSP:         /* Backspace */
            xc -= xv[CSPACE]; /* Subtract CSPACE units from x,y */
            yc -= yv[CSPACE];
            break;

         case DO_CRLF:        /* Carriage-return line-feed */

            /* This code corrects a bug in the original program, which
               maintained a modified y but not a modified x origin when
               a carriage return was performed.  A vertical offset
               equal to two letter widths is applied. */

            xc = (xo += 2.0*yv[CSPACE]);
            yc = (yo -= 2.0*xv[CSPACE]);
            break;

         case DO_SUP:         /* Superscript */

            switch (state) {  /* Action depends on current state */

            case SUBSC:       /* In subscript, switch to normal */
               state = NORMAL;
               mktable(stepht);     /* Restore original scale */
               xc -= yv[SSOFF];     /* Adjust coords */
               yc += xv[SSOFF];
               break;

            case NORMAL:      /* Normal, switch to super */
               state = SUPER;
               xc -= yv[SSOFF];
               yc += xv[SSOFF];
               mktable(SSFAC*stepht);
                              /* Fall through to break ... */

            case SUPER:       /* Already super, do nothing */
               ;
               } /* End state switch */

            break;

         case DO_SUB:         /* Subscript */

            switch (state) {  /* Action depends on current state */

            case SUPER:       /* In superscript, switch to normal */
               state = NORMAL;
               mktable(stepht);   /* Restore original scale */
               xc += yv[SSOFF];   /* Bring y coord down */
               yc -= xv[SSOFF];
               break;

            case NORMAL:      /* Normal, switch to sub */
               state = SUBSC;
               xc += yv[SSOFF];
               yc -= xv[SSOFF];
               mktable(SSFAC*stepht);
                              /* Fall through to break ... */

            case SUBSC:       /* Already sub, do nothing */
               ;
               } /* End state switch */

            } /* End special action switch */
         } /* End if special action */

/* Loop over the coordinate pairs for this character */

      else {
         pc1 = (char *)&S + CT[ch].so;
         for (pc=pc1; pc<pc1+len; pc++) {

            ix = iy = *(byte *)pc;     /* Isolate coord pair */
            /* Check for pen up or dot code.  Dot code sets ic = 4
               to trigger plotting of a dot when next coord is read */
            if ((ix&PEN_UP) == PEN_UP) {
               ic = 3 + (ix&1); continue; }
            ix >>= 4; iy &= 0x0f;
            xt = xc + xv[ix] - yv[iy]; /* Calculate coords */
            yt = yc + yv[ix] + xv[iy];
            if (ic == 4) {             /* Plot a small dot */
               xd = PTFAC*xv[1];
               yd = PTFAC*yv[1];
               plot(xt-xd,yt-yd,3);
               plot(xt-yd,yt+xd,2);
               plot(xt+xd,yt+yd,2);
               plot(xt+yd,yt-xd,2);
               plot(xt-xd,yt-yd,2);
               } /* End dot plot */
            else plot(xt,yt,ic);       /* Plot the point */
            ic = 2;                    /* Now pen down */
            } /* End loop over coordinate pairs */

/* Advance character coordinates (xc,yc) to start of next character */

         xc += xv[CSPACE];    /* Use offset of 6 segments */
         yc += yv[CSPACE];
         } /* End normal character */

      ic = 3;              /* Pen up to move to next char */
      } /* End loop over characters */
#ifdef METAFILE
   _NCG.MFSilent = OFF;
#endif
      } /* End if nc_graph */
#endif
#endif

   } /* End symbol */

