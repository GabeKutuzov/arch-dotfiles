/* (c) Copyright 1989-2018, The Rockefeller University *11115* */
/* $Id: rocks.h 65 2018-08-15 19:26:04Z  $ */
/***********************************************************************
*                         ROCKS.H Header File                          *
*       Static global ROCKS variables and function declarations        *
*                                                                      *
************************************************************************
*  V1A, 05/31/89, GNR - Initial version                                *
*  Rev, 08/18/92, GNR - Change TTCLS to 132 for spout to wide terms    *
*                       More rational distrib. between rocks/rkxtra.h  *
*  Rev, 02/02/94, GNR - Add sprmpt                                     *
*  Rev, 06/04/94, GNR - Add match and spout codes                      *
*  Rev, 05/22/96, GNR - Add RK_NTSUBTTL                                *
*  Rev, 02/09/97, GNR - Add RK_TOOMANY, RK.numcnv                      *
*  Rev, 09/05/97, GNR - Add accwac(), reallocv(), RK.rmcls             *
*  Rev, 11/09/97, GNR - Add RK.mcbits, RK.mckpm                        *
*  Rev, 02/20/98, GNR - Add rksleep()                                  *
*  Rev, 09/02/98, GNR - Add RK_ECNFERR, RK_BCNFERR, RK_EXCLERR, etc.   *
*  Rev, 09/12/98, GNR - Change all PGCLS, TTCLS to LNSIZE              *
*  Rev, 01/27/03, GNR - Add RK_UNITERR, RK_MULTERR, make erscan long   *
*  Rev, 12/13/07, GNR - Add C++ wrapper and ROCKS_HDR_INCLUDED test    *
*  ==>, 05/23/08, GNR - Last date before committing to svn repository  *
*  Rev, 02/14/09, GNR - Add RK_VANSERR                                 *
*  Rev, 11/07/09, GNR - Eliminate spoutm, more logical SPOUT controls  *
*  Rev, 08/31/11, GNR - Add RK.expwid                                  *
*  Rev, 12/03/11, GNR - Add bssel, wnxxxx() from rockv.h, OMIT_ROCKV   *
*  Rev, 01/11/12, GNR - Make initial rmlns -1 to get title on RK_LN0   *
*  Rev, 04/13/13, GNR - Add cryocls(), cryicls()                       *
*  R63, 06/01/17, GNR - Add RK_MPMRQ match code, 10/07/17 add envout   *
*  R64, 08/09/18, GNR - Add envin                                      *
***********************************************************************/

      /*--------------------------------------------------------*
      *                       IMPORTANT!!                       *
      *  The main program for any application using the ROCKS   *
      *  routines is required to include a statement            *
      *                    "#define MAIN"                       *
      *  before the line that includes this ROCKS.H header      *
      *  file.  This instantiates needed global structures.     *
      *--------------------------------------------------------*/

#ifndef ROCKS_HDR_INCLUDED
#define ROCKS_HDR_INCLUDED

#define PGLNS          60  /* Lines per page, including title */
#define L_TITL         66  /* Length of user-set part of title */
#define DFLT_MAXLEN    16  /* Default maximum scan length */
#define qDML          "16"

#ifndef MAIN               /* Define RK structure */
   extern
#endif
   struct RKdef {
      char *last;             /* Pointer to last card read */
      ui32 mcbits;            /* Bits returned by last mcodes */
      ui32 erscan;            /* Current error flags */
      short mckpm;            /* Kind of action of last mcodes */
      short highrc;           /* Highest return code */
      short iexit;            /* Cumulative error flags */
      short scancode;         /* Code returned by last scan */
      short plevel;           /* Parenthesis nesting level */
      short length;           /* Length of last field minus 1 */
      short numcnv;           /* Number of conversions performed */
      short pglns;            /* Number of lines printed per page */
      short pgcls;            /* Number of columns in print line */
      short ttcls;            /* Number of columns in terminal line */
      short rmlns;            /* Remaining lines on current page */
      short rmcls;            /* Remaining columns in current line */
      short pgno;             /* Current page number */
      byte  expwid;           /* Width of exponent in bcdout() */
      byte bssel;             /* Binary scale selector */
      } RK
#ifdef MAIN
           = { NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0,
               PGLNS, LNSIZE, LNSIZE, -1, LNSIZE, 0, 0, 0 }
#endif
      ;

#ifdef __cplusplus
extern "C" {
#endif
void accwac(void);
void accwad(int kaccw);
int  bscompat(int kbs);
int  bscomclr(int kbs);
int  bscomset(int kbs);
void *callocv(size_t n, size_t size, char *msg);
void cdprnt(char *card);
void cdprt1(char *card);
void cdunit(char *file);
void convrt(char *format, ...);
char *crkvers(void);
char *cryin(void);
void cryicls(void);
void cryout(int sprty, ...);
void cryocls(void);
void dbgout(char *opath);
int  envin(const char *envnm);
int  envout(const char *envnm);
void ermark(ui32 mcode);
void freev(void *freeme, char *errmsg);
void inform(char *format, ...);
void *mallocv(size_t length, char *msg);
int  match(int keq, int iscn, int mask, int ipnc,
     char **keys, int nkeys);
void ndrflw(char *string);
void pxoff(void);
int  pxon(void);
int  pxq(char *string, int maxstr);
void rdagn(void);
void *reallocv(void *ptr, size_t length, char *msg);
void rksleep(int sec, int usec);
void sconvrt(char *line, char *format, ...);
double second(void);
void setcpm(int kparse);
void setpid(char *pid);
void settit(char *title);
void shsortus(unsigned short *us, int n);
void sinform(char *card, char *format, ...);
int  smatch(int keq, const char *item, char **keys, int nkeys);
void *sort(void *index, int keyoff, int n, int ktype);
void *sort2(void *pdata, void *work, int okeys, int lkeys, int ktype);
void spout(int n);
void sprmpt(char *prompt);
void strncpy0(char *d, const char *s, size_t mxl);
#ifndef __USE_GNU
size_t strnlen(const char *s, size_t mxl);
#endif
int  wnclen(ui32 cc);
int  wntlen(char **ppfmt);
ui32 wntype(char **ppfmt);
#ifdef __cplusplus
}
#endif

/* Flags for bscompat */

#define RK_BSSLASH      1  /* Use second scale if '/' code */
#define RK_BSVERTB      2  /* Use second scale if '|' code */
#define RK_BSQUEST      4  /* Use second scale if '?' code */
#define RK_BSUNITS   0x10  /* Accept units with binary scl */

/* Flags for cryout */

#define RK_P1         256  /* Print priority 1 */
#define RK_P2         512  /* Print priority 2 */
#define RK_P3         768  /* Print priority 3 */
#define RK_P4        1024  /* Print priority 4 */
#define RK_P5        1280  /* Print priority 5 */
#define RK_E1      0x1100  /* Error message, iexit code 1 */
#define RK_E2      0x2100  /* Error message, iexit code 2 */
#define RK_E4      0x4100  /* Error message, iexit code 4 */
#define RK_E8      0x8100  /* Error message, iexit code 8 */
#define RK_PF      0x0800  /* printf() output (no carriage controls) */

#define RK_CCL          0  /* Continue current line */
#define RK_SKIP       255  /* Skip this line */
#define RK_LN0       2048  /* Line count 0 */
#define RK_LN1       2304  /* Line count 1 */
#define RK_LN2       2560  /* Line count 2 */
#define RK_LN3       2816  /* Line count 3 */
#define RK_LN4       3072  /* Line count 4 */
#define RK_LN5       3328  /* Line count 5 */
#define RK_LN6       3584  /* Line count 6 */
#define RK_NEWPG     3840  /* Force new page */
#define RK_OMITLN    4096  /* Omit line if in RK_PGNONE mode */
#define RK_SUBTTL    8192  /* Set subtitle */
#define RK_NTSUBTTL 16384  /* Subtitle, no trigger (store only) */
#define RK_NFSUBTTL 24576  /* Subtitle, no new page force */
#define RK_FLUSH    32768  /* Flush print buffer */

/* Flags for spout */

#define SPTM_GBLOFF    -3     /* Globally turned off for good */
#define SPTM_LCLOFF    -2     /* Locally turned off by cryin  */
#define SPTM_LCLON     -1     /* Used to override SPTM_LCLOFF */
#define SPTM_NONE       0     /* On but current count is zero */
#define SPTM_ALL  INT_MAX     /* Spout all (do not increment) */

/* Flags for ermark */

#define RK_WARNING 0x01000000    /* Warning only, don't set iexit */
#define RK_MARKDLM 0x02000000    /* Mark delimiter */
#define RK_MARKFLD 0x04000000    /* Mark data field  */
/* Codes for error messages currently must fit in low 3 bytes */
#define RK_PUNCERR 0x000001   /* Punctuation error */
#define RK_IDERR   0x000002   /* Identification error */
#define RK_LENGERR 0x000004   /* Length error */
#define RK_PNQTERR 0x000008   /* Parens or quotes not matched */
#define RK_REQFERR 0x000010   /* Required field missing */
#define RK_CARDERR 0x000020   /* Card not identified */
#define RK_NUMBERR 0x000040   /* Illegal numeric value */
#define RK_CHARERR 0x000080   /* Bad character in numeric field */
#define RK_ABBRERR 0x000100   /* Non-unique abbreviation */
#define RK_TOOMANY 0x000200   /* Too many fields (usually in parens) */
#define RK_ECNFERR 0x000400   /* Expected continuation not found */
#define RK_BCNFERR 0x000800   /* Blank continuation found */
#define RK_EXCLERR 0x001000   /* More than one of an exclusive set */
#define RK_NESTERR 0x002000   /* Parentheses nested */
#define RK_PNRQERR 0x004000   /* Parentheses required */
#define RK_EQRQERR 0x008000   /* Equals sign required */
#define RK_UNITERR 0x010000   /* Invalid units specifier */
#define RK_MULTERR 0x020000   /* Invalid units multipler */
#define RK_SYMBERR 0x040000   /* Undefined variable symbol */
#define RK_VANSERR 0x080000   /* Variable adjustment not preset */

/* Flags for match, smatch, eqscan */

#define RK_EQCHK        1  /* Check for preceding equals sign  */
#define RK_BEQCK        2  /* Check for not (blank or equals)  */
#define RK_BPMCK        4  /* Error if !equals (plus|minus OK) */
#define RK_PNCHK        8  /* Error if not in parentheses      */
#define RK_NMERR        2  /* Suppress no-match error */
#define RK_MSCAN        2  /* Call scan with no codes */
#define RK_MNEWK        3  /* Call scan with RK_NEWKEY flag */
#define RK_MDLIM        4  /* Call scan with flags RK_NEWKEY|
                           *  RK_PMDELIM|RK_ASTDELIM|RK_SLDELIM */
#define RK_MREQF        5  /* Call scan with RK_REQFLD flag */
#define RK_MPMDL        6  /* Call scan with RK_NEWKEY|RK_PMDELIM */
#define RK_MPMRQ        7  /* Call scan with RK_REQFLD|RK_PMDELIM */
#define RK_MPERR       -1  /* Punctuation error return */
#define RK_MENDC       -2  /* End-of-card error return */

/* Flags for sort2 ktype */

#define KST_CHAR     1     /* Keys are character strings */
#define KST_APOS     2     /* Keys are known all positive */
#define KST_FLOAT    4     /* Keys are floating-point numbers */
#define KST_DECR     8     /* Sort in decreasing order of keys */

/* Be sure needed structures are defined in main program */

#if defined(MAIN) && !defined(OMIT_ROCKV)
#include "rockv.h"
#endif

#endif   /* ROCKS_HDR_INCLUDED */
