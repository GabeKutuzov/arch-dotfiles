/* (c) Copyright 1988-2017, The Rockefeller University *11116* */
/* $Id: rockv.h 63 2017-10-13 18:41:33Z  $ */
/***********************************************************************
*                         ROCKV.H Header File                          *
*             Private variables and function declarations              *
*           used for communication between ROCKS functions             *
*                                                                      *
************************************************************************
*  V1A, 12/16/88, GNR - Initial version                                *
*  Rev, 11/11/91, GNR - Pad out to halfword length                     *
*  Rev, 08/20/92, GNR - Add time1 to RKC                               *
*  Rev, 01/31/94,  LC - Added accpt OL bit, prompt and plen to RKC     *
*  Rev, 06/04/94, GNR - Add spendm to support spoutm function          *
*  Rev, 08/15/97, GNR - Add new pagination and whitespace controls,    *
*                       add prnlin prototype                           *
*  Rev, 10/31/97, GNR - Add ACCPT_ACP bit                              *
*  Rev, 08/20/98, GNR - Change all filedefs to RFdefs, longer prompts  *
*                       and symbols, incorporate cdscan struct CS      *
*  Rev, 04/29/00, GNR - Move erscan to RK structure in rocks.h         *
*  Rev, 07/03/00, GNR - Add pcfd                                       *
*  Rev, 01/27/03, GNR - Expand erdmrk to unsigned long, add ACCPT_WD   *
*  Rev, 12/30/07, GNR - Add crlvalck, getvalck, ivalck, uvalck, etc.   *
*  Rev, 05/23/08, GNR - Add bssel, ACCPT_INF                           *
*  ==>, 05/23/08, GNR - Last date before committing to svn repository  *
*  Rev, 02/14/09, GNR - Add RKVAV chain, ivadj, VCK_ADJ                *
*  Rev, 05/24/09, GNR - Add RK_Inflags, ivckmsg[012], hexin, decin     *
*  Rev, 07/25/09, GNR - Add wvalck, valckmsg                           *
*  Rev, 11/07/09, GNR - Eliminate spoutm, more logical SPOUT controls  *
*  Rev, 08/14/11, GNR - Add LLNCCH, larger SBHDCH                      *
*  Rev, 12/03/11, GNR - Move bssel, wnxxxx() to rocks.h for rkprintf() *
*  Rev, 10/21/15, GNR - Increase SBHDCH from 408 to 544                *
*  Rev, 04/22/16, GNR - Add L_ENVSET                                   *
*  R63, 09/25/17, GNR - L_SYMBOL (= 7) -> L_SYMBNM  (= 15),            *
*                       L_ENVSYM (=31) -> L_SYMBVAL (=255)             *
***********************************************************************/

#include "rfdef.h"

#define SBHDCH     544        /* Number of characters for subtitles.
                              *  544 is enough for 4 full lines of 133
                              *  characters plus up to 12 extra LFs.  */
#define LLNCCH      12        /* Space to accumulate LF chars */
#define DOPRTY       4        /* Default output priority */
#define INIT_NVSA   10        /* Initial number of symbols to alloc */
#define L_PROMPT    16        /* Maximum length of prompt (=4n) */
#define L_SYMBNM    15        /* Maximum length of symbol name */
#define L_SYMBVAL  255        /* Maximum length of symbol value or
                              *  name of an environment variable */
#if L_SYMBNM > L_SYMBVAL
#error L_SYMBVAL must be large enough to hold a regular symbol name
#endif
#if DFLT_MAXLEN > L_SYMBVAL || LONG_SIZE > L_SYMBVAL
#error L_SYMBVAL must be large enough to hold a scanned value or number
#endif
#if L_SYMBVAL > BYTE_MAX
#error struct RKVS vsize must hold values up to L_SYMBVAL
#endif
#define L_ENVSET    80        /* Maximum length of a value to be
                              *  assigned to environment in cryin() */
#define BadDatSize  10        /* Max width of bad data output */
#define CkDatSize   16        /* Max length of coded test value */

/* Cryin card unit block */

   struct RKCDU {
      struct RFdef rfd;       /* RFdef for file--must be first! */
      char *pc;               /* Pointer to card buffer */
      struct RKVS *pfvs;      /* Pointer to first variable symbol */
      unsigned short nvs;     /* Number of variables stored */
      unsigned short nvsa;    /* Number of variables allocated */
      unsigned short nvse;    /* Number variables on EXEC card */
      ui16 accpt;             /* Saved copy of RKC.accpt */
      } ;

/* Cryout unit definition block */

   struct unit_def {          /* Print, spout controls */
      struct RFdef rfd;       /* Stdout or stderr */
      short pgcls;            /* Columns per line */
      short rmcls;            /* Remaining columns */
      byte type;              /* 0 = print, 1 = spout, 2 = file */
      byte trigger;           /* Subtitle trigger */
      byte flushed;           /* Line flushed when true */
      byte nopaginate;        /* TRUE to suppress pages and titles */
      } ;

/* Variable symbol descriptor */

   struct RKVS {
      char *pval;                /* Ptr to value of the symbol */
      unsigned char vsize;       /* Length of the symbol */
      char vname[L_SYMBNM];      /* Name of the symbol */
      } ;

/* Saved variable adjustment value */

   struct RKVAV {
      struct RKVAV *pnvav;       /* Ptr to next value in list */
      double adjval;             /* Variable adjustment value */
      char vname[L_SYMBNM];      /* Name of the adjustor,NULL */
      byte vflag;                /* Flags-currently none used */
      };

/* Flags used by input routines -- not in RKC because may be used
*  in programs that do not need RKC.  */

   ui16 RK_Inflags;
#define DP_FOUND        0x0001   /* Decimal point found */
#define SIGN_FOUND      0x0002   /* Sign found */
#define MINUS_FOUND     0x0004   /* Minus sign found */
#define NONZERO_FRAC    0x0008   /* Nonzero fraction found */
#define EXP_DIG_FOUND   0x0010   /* Digit found in exponent */
#define NEG_EXP         0x0020   /* Exponent is negative */
#define IVCK_qERR       0x0080   /* Nine bits flag errors printed */

/* General communications area */

#ifndef MAIN
   extern
#endif
   struct RKCdef {
      double time1;           /* Time at start of run */
      double dvadj;           /* Input value adjustor */
      struct unit_def PU;     /* Print unit structure */
      struct unit_def SU;     /* SPOUT unit structure */
      struct RKCDU *cduptr;   /* Pointer to current card unit stack */
      struct RKCDU *cdures;   /* Pointer to reserve card unit stack */
      struct RKVAV *pfvav;    /* Ptr to first variable adjustore */
      char *cdsave;           /* Ptr to start of card being scanned */
      char *dmsg;             /* Ptr to ermark "$" error msg */
      char *fldsave;          /* Ptr to space to save last field */
      char *pc;               /* Ptr to current scanning location */
      char *pcfd;             /* Ptr to first delimiter */
      char *pfield;           /* Ptr to current user field location */
      char *psymbol;          /* Ptr to current symbol save location */
      ui32 erdmrk;            /* Flags for errors with $ marks */
      ui32 sysindex;          /* Current value of &SYSNDX symbol */
      int  lfldsave;          /* Size allocated for fldsave */
      int  qmaxlen;           /* Current max scan field length */
      int  spend;             /* Spout count: <=0 if inactive */
      ui16 accpt;             /* Reread and print control flags */
      ui16 f;                 /* Scan status flags */
      ui16 sccsf;             /* Semicolon comment save f */
      short ck_omitln;        /* = RK_OMITLN to check that bit */
      short ck_subttl;        /* = RK_SUBTTL to check that bit */
      short plen;             /* Length of online prompt */
      short lgth1sv;          /* Copy of old length for scanagn */
      short lgth2sv;          /* Copy of new length for scanagn */
      short plev1sv;          /* Copy of old plevel for scanagn */
      short plev2sv;          /* Copy of new plevel for scanagn */
      short rc1save;          /* Copy of old rc for scanagn */
      short rc2save;          /* Copy of new rc for scanagn */
      short remfld;           /* Remaining space in user field */
      short remsym;           /* Remaining space in symbol */
      byte kparse;            /* Parse mode flags */
      byte ktest;             /* Type of numeric value tests */
      byte nlplp;             /* Number lower paren level pending */
      byte oprty;             /* Current output priority */
      byte qcsflags;          /* Flags for cdscan */
      byte qscflags;          /* Flags for scan */
      char badval[BadDatSize+1]; /* Bad value for error msg */
      char cmntest[CkDatSize+1]; /* Min test value ('>' codes) */
      char cmxtest[CkDatSize+1]; /* Max test value ('<' codes) */
      char prompt[L_PROMPT+3];   /* Prompt string */
      char symbol[L_SYMBVAL+1];  /* Array to hold symbol name */
      } RKC
#ifdef MAIN
      = { 0.0, 0.0,                    /* time1, ivadj */
         { {NULL}, LNSIZE, LNSIZE, 0, 0, 1, 0}, /* Print structure */
         { {NULL}, LNSIZE, LNSIZE, 1, 0, 1, 0}, /* Spout structure */
         NULL, NULL, NULL,             /* cduptr, cdures, pfvav  */
         NULL, NULL, NULL, NULL, NULL, /* cdsave, etc. */
         NULL, NULL,                   /* pfield, psymbol */
         0,                            /* erdmrk */
         0,                            /* sysindex */
         0, 0, SPTM_LCLOFF,            /* lfldsave, etc. */
         0, 0, 0,                      /* accpt, f, sccsf */
         0,                            /* ck_omitln */
         RK_SUBTTL,                    /* ck_subttl */
         3,                            /* plen */
         0, 0, 0, 0, 0, 0,             /* lgth1sv, etc. */
         0, 0,                         /* remfld, remsym */
         0, 0, 0,                      /* kparse, ktest, nlplp */
         DOPRTY,                       /* oprty */
         0, 0,                         /* qcsflags, qscflags */
         "\0", "\0", "\0",             /* badval, cmntest, cmxtest */
         "\n\r>",                      /* prompt */
         "\0",                         /* symbol */
         }
#endif
      ;

/* RKC.accpt flags */
#define ACCPT_RR   0x0001  /* Reread last control card */
#define ACCPT_AGN  0x0002  /* scanagn() has been called */
#define ACCPT_CC   0x0004  /* Card was a comment */
#define ACCPT_CE   0x0008  /* Continuation expected */
#define ACCPT_CEP  0x0010  /* Card ever printed--print continuations */
#define ACCPT_CLP  0x0020  /* Card last printed--erprnt skips print */
#define ACCPT_CS   0x0040  /* Card being scanned--$ mark is OK */
#define ACCPT_OL   0x0080  /* Cards being read on-line */
#define ACCPT_INF  0x0100  /* Running (s)inform() */
#define ACCPT_NKP  0x0200  /* NEWKEY error pending */
#define ACCPT_ACP  0x0400  /* Last thing printed is a control card--
                           *  cdprt1 single-spaces on next call */
#define ACCPT_NOR  0x0800  /* Unit will not be reused--free buffers */
#define ACCPT_ERP  0x1000  /* Interrupt cryout for error print */
#define ACCPT_EF   0x2000  /* End-of-file */
#define ACCPT_MKOK 0x4000  /* Error marking forced OK */
#define ACCPT_MKNO 0x8000  /* Error marking forced OFF */
/* RKC.f scanning status flags */
#define SQT          0x01  /* In single quotes */
#define DQT          0x02  /* In double quotes */
#define TER          0x04  /* Terminate on next call */
#define IN           0x08  /* Initial in field */
#define AMP          0x10  /* Ampersand found--processing vsym */
#define CEPA         0x20  /* Comma, equals sign, parens, asterisk,
                           *  slash, plus/minus ended previous field */
#define HSCC         0x40  /* Holding semicolon comment for CrkParse */
#define BTE          0x80  /* Blank found; tentative end */
#define PLEQ       0x0100  /* '+=' found: insert '+' in next field */
#define MIEQ       0x0200  /* '-=' found: insert '-' in next field */
/* Error codes for ivckmsg[012] */
#define IVCK_FORMAT     1  /* Value is not a well formed number */
#define IVCK_HEXFMT     2  /* Value is not a valid hex/octal number */
#define IVCK_BADEXP     3  /* Bad exponent */
#define IVCK_APPGE0     4  /* Application requires value >= 0 */
#define IVCK_APPNZ      5  /* Application requires value != 0 */
#define IVCK_INTEGER    6  /* Application requires an integer */
#define IVCK_OVERFLOW   7  /* Value overflows on conversion */
#define IVCK_UNDRFLOW   8  /* Value underflows on conversion */
#define IVCK_HIGHCODE   8  /* Highest legal ivckmsg error code */
/* RKC.kparse parse mode flags */
#define RKKP_CRKP    0x01  /* Running in special CrkParse mode */
#define RKKP_DOEX    0x02  /* Process EXEC cards */
#define RKKP_NOVS    0x04  /* Omit variable substitutions */
/* And a few other bits that can be tucked away here */
#define RKKP_EDMPRQ  0x10  /* Error dump requested */
#define RKKP_WC      0x20  /* Accept whitespace as comment */
#define RKKP_WD      0x40  /* Accept whitespace as data card */
/* RKC.ktest test mode flags */
#define VCK_GTHAN    0x01  /* Greater than */
#define VCK_GTEQ     0x02  /* Greater than or equal */
#define VCK_LTHAN    0x04  /* Less than */
#define VCK_LTEQ     0x08  /* Less than or equal */
#define VCK_NZERO    0x10  /* Not zero */
#define VCK_WARN     0x20  /* Just a warning */
#define VCK_ADJ      0x80  /* An adjustment is present */

/* Prototypes for routines private to ROCKS programs */
#ifdef __cplusplus
extern "C" {
#endif
void clrvalck(void);
double decin(ui32 ic, char *field);
double dvalck(double value, ui32 ccode);
void erprnt(void);
void freepfvs(struct RKCDU *pncu);
float fvalck(float value, ui32 ccode);
char *getvalck(char *fptr);
void hexin(ui32 ic, char *field, byte *value, int maxlen);
int  iscontnu(char *card);
si32 ivalck(si32 value, ui32 ccode);
void ivckmsg0(const char *udat, int ierr);
void ivckmsg1(const char *udat, char *vlim);
void ivckmsg2(const char *udat, int iint);
void muscan(char **ppfmt, char *pdat, ui32 *pcc);
void prnlin(struct unit_def *ublk, char *pd, int width, short lcode);
ui32 uvalck(ui32 value, ui32 ccode);
void valckmsg(char *ttype, char *goodval);
char *vslookup(char *symbol, int ct);
void wvalck(void *val, ui32 ccode);
#ifdef __cplusplus
}
#endif
