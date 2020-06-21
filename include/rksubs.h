/* (c) Copyright 1997-2016, The Rockefeller University *11115* */
/* $Id: rksubs.h 61 2017-04-13 20:31:56Z  $ */
/***********************************************************************
*                        RKSUBS.H Header File                          *
*           Prototype declarations of ROCKS subset routines            *
*                                                                      *
*  This header is intended for use in programs that use the subset     *
*  of ROCKS routines that do not utilize the card/line interface.      *
*  It contains selected declarations from rocks.h and rkxtra.h.        *
*  Others are in bapkg.h, rfdef.h, rkarith.h, swap.h, and sysdef.h.    *
************************************************************************
*  V1A, 11/22/97, GNR - Copied out from rocks.h                        *
*  Rev, 02/20/98, GNR - Add rksleep()                                  *
*  Rev, 08/09/07, GNR - Add ifdef cplusplus definitions                *
*  ==>, 03/22/08, GNR - Last date before committing to svn repository  *
*  Rev, 08/18/08, GNR - Add getmyip(), strncpy0                        *
*  Rev, 10/21/16, GNR - Add wbcdwtPn and bcdoutPn and their RK codes   *
***********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif
void bcdoutPn(ui32 ic, char *field, double arg);
void *callocv(size_t n, size_t size, char *msg);
void *mallocv(size_t length, char *msg);
void *reallocv(void *ptr, size_t length, char *msg);
void freev(void *freeme, char *errmsg);

int  cntrl(char *card);
char *getmyip(void);
int  jfind(char *card, char *key, int idspl);
int  qwhite(char *card);
void rksleep(int sec, int usec);
double second(void);
void shsortus(unsigned short *us, int n);
long sibcdin(int ic, char *field);
void *sort(void *index, int keyoff, int n, int ktype);
void *sort2(void *pdata, void *work, int okeys, int lkeys, int ktype);
int  ssmatch(const char *item, const char *key, int mnc);
void strncpy0(char *d, const char *s, size_t mxl);
#ifndef __USE_GNU
size_t strnlen(const char *s, size_t mxl);
#endif
void tstamp(char *pstamp);
void wbcdwtPn(void *pitm, char *field, ui32 ic);
#ifdef __cplusplus
}
#endif


/* Return values for cntrl */

#define NUMBRET         0  /* Value returned for numbers */
#define CNTLRET         1  /* Value returned for control data */

/* Flags for bcdoutPn, wbcdwtPn, sibcdin */

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
/**** Formats ****/
#define RK_EFMT         0  /* Exponential format */
#define RK_IORF      1536  /* Integer or floating format */
#define RK_OCTF      1024  /* Octal conversion */
#define RK_HEXF       512  /* Hexadecimal conversion */
/**** Argument types ****/
#define RK_NDFLT   0x0000  /* Default: int or double (per K&R C) */
#define RK_NBYTE   0x0040  /* Byte */
#define RK_NHALF   0x0080  /* Half-word (16-bit) fixed-point */
#define RK_NINT    0x00c0  /* Int (16- or 32-bit) fixed-point */
#define RK_NI32    0x0100  /* 32-bit fixed-point or single float */
#define RK_NLONG   0x0140  /* Long (32- or 64-bit) fixed-point */
#define RK_NI64    0x0180  /* 64-bit fixed-point */
#define RK_NI128   0x01c0  /* Reserved for 128-bit fixed-point */
/**** Options ****/
#define RK_TS           6  /* Shift to extract size code */
#define RK_PAD0        64  /* Pad to left with zeros */
#define RK_NPAD0     2048  /* Pad with zeros */
#define RK_AUTO      4096  /* Automatic decimal placement */
#define RK_QPOS      4096  /* Query positive */
#define RK_NZ0X      4096  /* Prefix hex values with '0X' */
#define RK_Oct0      4096  /* Prefix oct values with '0'  */
#define RK_UFLW      8192  /* Underflow protection */
#define RK_CTST      8192  /* Character test */
#define RK_NZTW      8192  /* Hex output to width of type */
#define RK_LFTJ     16384  /* Left justify */
#define RK_ZTST     16384  /* Zero test */
#define RK_NUNS     32768  /* Item is unsigned */
/* Code in bcdout, wbcdwt assumes next 3 bits are adjacent */
#define RK_PLUS   1048576  /* Plus sign (output) */
#define RK_LSPC   2097152  /* Left protective space (output) */
#define RK_NEGV   4194304  /* Negative unsigned (output) */
#define RK_NZLC   8388608  /* Lower-case hex (output) */
/**** Additional codes still used in bcdout, sibcdin ****/
#define RK_INT       2048  /* Integer */
#define RK_SNGL       256  /* Single-precision float */
#define RK_SSCN       256  /* Subset scan */

/* Flags for sort2 ktype */

#define KST_CHAR     1     /* Keys are character strings */
#define KST_APOS     2     /* Keys are known all positive */
#define KST_FLOAT    4     /* Keys are floating-point numbers */
#define KST_DECR     8     /* Sort in decreasing order of keys */

