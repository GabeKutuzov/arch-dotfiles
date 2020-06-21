/* (c) Copyright 1989-2012, The Rockefeller University *11115* */
/* $Id: rkxtra.h 61 2017-04-13 20:31:56Z  $ */
/***********************************************************************
*                        RKEXTRA.H Header File                         *
*                     ROCKS function declarations                      *
*                                                                      *
*  V1A, 04/19/89, GNR - Initial version                                *
*  Rev, 08/19/92, GNR - Add okmark(), several functions from rocks.h   *
*  Rev, 05/28/94, GNR - Add scanck()                                   *
*  Rev, 10/10/96, GNR - Add skip2end(), RK_ZTST                        *
*  Rev, 08/15/97, GNR - Add nopage(), qwhite()                         *
*  Rev, 10/31/97, GNR - Add thatsall, sibcdin, mcodop, mckpm codes     *
*  Rev, 08/12/01, GNR - Add text cache package                         *
*  Rev, 10/17/01, GNR - Four variants of mcodop(), revise scan codes   *
*  Rev, 01/25/03, GNR - Add RK_DSF                                     *
*  Rev, 12/13/07, GNR - Add C++ wrapper and RKXTRA_HDR_INCLUDED test   *
*  Rev, 05/18/08, GNR - Add curplev(), kwsregfn, MAX_KWSREG_N          *
*  ==>, 05/21/08, GNR - Last date before committing to svn repository  *
*  Rev, 09/27/08, GNR - Add nbcdiin, nbcdfin, nbcdiwt, nbcdfwt & codes *
*  Rev, 01/08/09, GNR - Remove nbcdiwt, nbcdfwt, add wbcdwt, RK_PMEQPM *
*  Rev, 08/11/09, GNR - Change some long types to ui32 for PCLUX64     *
*  Rev, 09/01/11, GNR - Remove RK_E, RK_ES, add RK_PLUS, RK_LSPC       *
*  Rev, 09/08/11, GNR - Reassign RK_NZNW, RK_Oct0 codes for octal out  *
*  Rev, 11/14/11, GNR - Add RK_NZLC for lower-case hex output          *
*  Rev, 01/22/12, GNR - Remove RK_NZ0x, change RK_NZNW to RK_NZTW      *
***********************************************************************/

#ifndef RKXTRA_HDR_INCLUDED
#define RKXTRA_HDR_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif
double bcdin(ui32 ic, char *field);
void bcdout(ui32 ic, char *field, double arg);
void binout(char *string, int bitlen, unsigned char *array);
void cdscan(char *card, int displ, int maxlen, int csflags);
void ckvadj(void);
int cntrl(char *data);
int cntrln(char *data, int length);
int curplev(void);
int eqscan(void *item, char *code, int ierr);
int findtxt(char *ptxt);
char *getdat(void);
char *getrktxt(int htxt);
char *gettit(void);
char *gtunit(void);
si32 ibcdin(ui32 ic, char *field);
void ibcdwt(ui32 ic, char *field, si32 iarg);
int invoke(char *routinename);
int jfind(char *card, char *key, int idspl);
int kwscan(ui32 *ic, ...);
void kwjreg(void (*kwjfn)(char *, void *), int n);
void kwsreg(void (*kwnfn)(void *), int n);
void kwjregfn(char *pfmt, void *pvar, int n);
void kwsregfn(void *pvar, int n);
#define MAX_KWSREG_N 9  /* Max n in a kwsreg, kwsregfn call */
int lines(int n);
int mcodes(char *data, char *keys, ui32 *item);
#define MXKEYLEN BITSPERUI32  /* Current maximum item bits */
void mcodopl(ui32 *item);
void mcodopi(unsigned int *item);
void mcodoph(unsigned short *item);
void mcodopc(unsigned char *item);
char *mcodprt(ui32 item, char *keys, int olen);
void nopage(int);
void okmark(int dmflag);
int qonlin(void);
int qwhite(char *card);
int savetxt(char *ptxt);
int scan(char *field, int scflags);
void scanagn(void);
int scanck(char *field, int scflags, int badpn);
int scanlen(int maxlen);
long sibcdin(int ic, char *field);
void skip2end(void);
int ssmatch(const char *item, const char *key, int mnc);
void svvadj(double value, char *aname);
void thatsall(void);
int tlines(int n);
void tstamp(char *);
ui32 ubcdin(ui32 ic, char *field);
void ubcdwt(ui32 ic, char *field, ui32 uarg);
void wbcdin(const char *field, void *pitem, ui32 ic);
void wbcdwt(void *pitm, char *field, ui32 ic);
void wseedin(wseed *pwsd);
void wseedout(wseed *pwsd, char *field, ui32 ic);
int xxpoly(float *coeffs, char *names, int len);
#ifdef __cplusplus
}
#endif

/* Flags for cdscan and scan */

/**** Values for cdscan csflags and scan scflags ****/
#define RK_AMPNULL      1  /* Treat ampersands as nulls (cdscan only) */
#define RK_NEST         2  /* Permit nested parens (cdscan only) */
#define RK_NOCONT       4  /* Don't read continuations (cdscan only) */
#define RK_NEWKEY       1  /* Require new key (scan only) */
#define RK_REQFLD       2  /* Require this field (scan only) */
#define RK_FENCE        4  /* No NULL ends maximal field (scan only) */
#define RK_PMDELIM      8  /* Plus/minus treated as delimiters (both) */
#define RK_ASTDELIM    16  /* Asterisk treated as delimiter (both) */
#define RK_SLDELIM     32  /* Slash treated as delimiter (both) */
#define RK_SCANFNM     64  /* Scan a file name (scan only) */
#define RK_NOPMEQ      64  /* '+=', '-=' are separate delims (cdscan) */
#define RK_WDSKIP     128  /* Word skip (cdscan), colon stop (scan) */
#define RK_PMEQPM     256  /* '+=', '-=' treated as '=+', '=-' (scan) */
#define RK_PMVADJ     264  /* RK_PMDELIM ignores +/- in exponents */
/**** Punctuation codes returned by scan and in RK.scancode ****/
#define RK_BLANK        0  /* Blank is only separator */
#define RK_PMINUS       1  /* RK_PMDELIM: + or - found */
#define RK_COMMA        2  /* Comma found */
#define RK_EQUALS       4  /* Equals sign found */
#define RK_ASTERISK     8  /* RK_ASTDELIM: Asterisk found */
#define RK_SLASH       16  /* RL_SLDELIM:  Slash found */
#define RK_LFTPAREN    32  /* Left parens found */
#define RK_INPARENS    64  /* In parentheses */
#define RK_RTPAREN    128  /* Right paren found */
#define RK_COLON      256  /* Colon found */
#define RK_SEMICOLON  512  /* Semicolon commment found */
#define RK_ENDCARD   1024  /* End of card--must be highest scancode */

/* Codes for bcdout, ibcdwt, wbcdwt, bcdin, ibcdin, ubcdin, sibcdin */

/**** Internal Use ****/
#define RK_MZERO 0x80000000   /* Output -BIG as -0 */
#define RK_FORCE 0x40000000   /* Exponential format forced */
#define RK_MXWD        31  /* Maximum value of width field */
/**** Scale and Decimal Specification ****/
#define RK_BS          24  /* Binary scale shift */
#define RK_BSCL  16777216  /* Binary scale */
#define RK_DSF    8388608  /* Decimal scale forced */
#define RK_DS          16  /* Decimal shift */
#define RK_D        65536  /* Decimal */
/**** Number Types (original routines) ****/
#define RK_DBL          0  /* Double-precision */
#define RK_BYTE       128  /* Character */
#define RK_SNGL       256  /* Single-precision */
#define RK_INT       2048  /* Integer (input) */
#define RK_GFP      32768  /* General fixed-point */
/**** Formats ****/
#define RK_EFMT         0  /* Exponential format */
#define RK_IORF      1536  /* Integer or floating format */
#define RK_OCTF      1024  /* Octal conversion */
#define RK_HEXF       512  /* Hexadecimal conversion */
/**** Options ****/
#define RK_PAD0        64  /* Pad to left with zeros */
#define RK_SSCN       256  /* Subset scan (sibcdin) */
#define RK_AUTO      4096  /* Automatic decimal placement */
#define RK_QPOS      4096  /* Query positive */
#define RK_UFLW      8192  /* Underflow protection */
#define RK_CTST      8192  /* Character test */
#define RK_LFTJ     16384  /* Left justify */
#define RK_ZTST     16384  /* Zero test */
/* Code in bcdout, wbcdwt assumes next 3 bits are adjacent */
#define RK_PLUS   1048576  /* Plus sign (output) */
#define RK_LSPC   2097152  /* Left protective space (output) */
#define RK_NEGV   4194304  /* Negative unsigned (output) */
#define RK_NZLC   8388608  /* Lower-case hex (output) */

/* Codes for wbcdin, wbcdwt where different from the above */
#define RK_NUNS     32768  /* Item is unsigned */
#define RK_NZTW      8192  /* Hex output to width of type */
#define RK_NZ0X      4096  /* Prefix hex values with '0X' */
#define RK_Oct0      4096  /* Prefix oct values with '0'  */
#define RK_NPAD0     2048  /* Pad with zeros */
#define RK_TS           6  /* Shift to extract size code */
#define RK_NDFLT   0x0000  /* Default: int or double (per K&R C) */
#define RK_NBYTE   0x0040  /* Byte */
#define RK_NHALF   0x0080  /* Half-word (16-bit) fixed-point */
#define RK_NINT    0x00c0  /* Int (16- or 32-bit) fixed-point */
#define RK_NI32    0x0100  /* 32-bit fixed-point or single float */
#define RK_NLONG   0x0140  /* Long (32- or 64-bit) fixed-point */
#define RK_NI64    0x0180  /* 64-bit fixed-point */
#define RK_NI128   0x01c0  /* Reserved for 128-bit fixed-point */

/* Return values for cntrl */

#define NUMBRET         0  /* Value returned for numbers */
#define CNTLRET         1  /* Value returned for control data */

/* Values for RK.mckpm */

#define RK_MCSET        0  /* Set the flagword */
#define RK_MCOR         1  /* OR new bits into flagword */
#define RK_MCCLR        2  /* Clear specified bits in flagword */
#define RK_MCZERO       3  /* Zero the entire flagword */

/* Calling args for nopage */

#define RK_PGNORM       0  /* Normal pagination for printer */
#define RK_PGLONG       1  /* Long pages for printer plots */
#define RK_PGNONE       2  /* Suppress all pagination activity */

#endif   /* RKXTRA_HDR_INCLUDED */
