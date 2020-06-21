/* (c) Copyright 1988-2011, Neurosciences Research Foundation, Inc. */
/* $Id: d3global.h 46 2011-05-20 20:14:09Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                       D3GLOBAL Header File                           *
*                              6/1/88                                  *
*  This header file should precede all other CNS-specific headers:     *
*  Global values are set here which are referenced by other headers.   *
*                                                                      *
*  V4A, 06/01/88, Initial version                                      *
*  Rev, 02/23/90, GNR - Changes for parallel gconns                    *
*  Rev, 11/05/91, GNR - Add defs for phase distributions               *
*  Rev, 02/13/92, GNR - Add msgs for pause, VSBIC, remove IO           *
*  Rev, 04/03/92, GNR - Separate name lengths for reps (20), other (4) *
*  Rev, 04/30/92, GNR - Install generic alloc errors, remove specific  *
*  Rev, 07/22/92, GNR - Add msg, errors for CONNLIST, dynam alloc defs *
*  Rev, 12/01/92, ABP - Remove obsolete SYNCH_MSG, add HYB messages    *
*                       and error codes, separate development errors,  *
*                       add file hybdef.h, new error codes.            *
*  V6C, 08/14/93, GNR - Remove 2**n dependency, add FIRST_COMP_NODE    *
*  V6D, 01/31/94, GNR - Add presynaptic delay                          *
*  V7B, 05/28/94, GNR - Restore message codes moved by abp to nsitools *
*  V7C, 04/08/95, GNR - Define mdlt_type to contain modality bits      *
*  V8A, 04/24/96, GNR - Add overflow checking macros, LPCFNM           *
*  Rev, 11/27/96, GNR - Remove support for OVERLAY and NCUBE graphics  *
*  Rev, 12/07/96, GNR - Remove dynamic message type definitions        *
*  Rev, 09/28/97, GNR - Independent dynamic allocations per conntype   *
*  Rev, 01/05/98, GNR - Add PPF, s24tous8(), d3noise()                 *
*  Rev, 09/22/98, GNR - Add rlnm typedef                               *
*  Rev, 09/05/99, GNR - Remove remaining NCUBE support                 *
*  Rev, 10/28/00, GNR - Remove XRH, tabulate KRPHR over all cycles     *
*  V8B, 12/03/00, GNR - New memory management routines, trial timers   *
*  Rev, 08/11/01, GNR - Add text cache, remove most name length limits *
*  V8C, 02/22/03, GNR - Cell responses in millivolts                   *
*  V8D, 01/30/04, GNR - Add conductances and ions                      *
*  Rev, 09/27/07, GNR - Add RBAR, 12/26/07, add QBAR                   *
*  ==>, 12/30/07, GNR - Last mod before committing to svn repository   *
*  Rev, 05/30/08, GNR - Add color opponency color selectors            *
*  V8E, 01/12/09, GNR - Add Izhikevich cells, DFLT_MAXLEN->SCANLEN     *
*  V8F, 02/20/10, GNR - Remove PLATFORM, VDR support, add *_SRC codes  *
*  Rev, 04/16/10, GNR - Add image color codes, readbc, and PP_SRC      *
*  V8G, 08/15/10, GNR - Add AUTOSCALE                                  *
*  V8H, 10/20/10, GNR - Add xn (once per conntype) variable allocation *
*  Rev, 05/25/11, GNR - New scheme to control reset actions            *
*  Rev, 01/05/12, GNR - Move redundant defs from simdata.h, etc.       *
***********************************************************************/

#define MINUS0 0x80000000L    /* Sign bit of long integer */
#define LSmem         2       /* Length of s(i) in memory */
#define LSNAME        4       /* Length of a short name */
#define qLSN         "4"      /* Quoted LSNAME */
#define LXRLNM       22       /* Max length of rep-layer name string */
#define qLXN        "22"      /* Quoted LXRLNM + 1 */
#define LTEXTF       80       /* Maximum length of a text field */
#define qLTF        "80"      /* Quoted LTEXTF */
#define qLUPRGNM    "80"      /* Quoted max length of user prog name */
#define COLMAXCH     12       /* Max chars allowed in color name */
#define qCMC        "12"      /* Quoted COLMAXCH */
/* L[DH]STAT, LLPDAS must be even to avoid alignment probs--d3news */
#define LDSTAT       16       /* Dimension of distribution stats */
#define LHSTAT        8       /* Dimension per mdlty of history stats */
#define LSTITL       62       /* Length of stitle array */
#define MINBPRAD   0.01       /* Minimum bubble plot radius (inches) */
#define MXNTRT  ('Z'-'A'+3)   /* Max number of user-defined timers,
                              *  allowing for '0' and '1' plus 'A' - 'Z'
                              *  (includes holes in EBCDIC alphabet). */
#define NBPMCE        4       /* Bits per KGNMC matrix entry */
#define NCNThr       20       /* Space for deferred conntype threshs */
#define NCTT          4       /* Number of celltype thresholds (pt..) */
#define qNCTT        "4"      /* Quoted NCTT */
#define NTAC          5       /* Number of traditional (D3) amp cases */
#define PHASE_RES    32       /* Phase resolution--must be power of 2 */
#define qPHR        "32"      /* Quoted PHASE_RES */
#define PHASE_MASK   31       /* Mask to convert phase to unit circle */
#define PHASE_EXP     5       /* Log(2)(PHASE_RES) */
#define PLCSP_BORDER  0.10    /* Blank border in PLCSP plot (< 0.5) */
#define SCANLEN      16       /* Scanning routine token length */
#define SCANLEN1 (SCANLEN+1)  /*  (with room for string delimiter) */
#define COLORNUM     14       /* Num plot object colors */
#define SEQNUM        8       /* Num colors in bubble plot seq--if not
                              *  a power of 2, check refs to SEQN_EXP */
#define SEQN_EXP      3       /* Log(2)(SEQNUM) */
#define VSBIC  (RK_CTST+RK_QPOS+RK_IORF+3)
                              /* Virtual source BCD input code */

/* Codes for types used in simdata, savblk, etc. */
#define FTYPE   0x00             /* Fixed point, integer */
#define UTYPE   0x01             /* Unsigned fixed point */
#define ETYPE   0x02             /* Real */
#define DTYPE   0x03             /* Double precision real */
#define KTYPE   0x04             /* Colored pixel data */
#define ATYPE   0x05             /* ASCII data */
#define XTYPE   0x06             /* Hexadecimal data */

/* Standard constants */
#define TWOPI    6.283185308
#define RSQRT2PI 0.398942280  /* Reciprocal of sqrt(2*pi) */
#define DEGS_TO_RADS  1.745329252E-2
#define BINS_TO_RADS  (6.283185308/(double)PHASE_RES)
#define Faraday 9.649E1       /* Faraday in Coulombs/mole * 1E-3 */

/* Type definitions */
typedef float armwdw_type;          /* Arm/window position data */
typedef unsigned char d1s_type;     /* Darwin 1 response data */
typedef long  gac_type;             /* Conductance activation (S30) */
typedef unsigned short mdlt_type;   /* Bits identifying a modality */
typedef unsigned short txtid;       /* Text locator */
typedef unsigned char rd_type;      /* Repertoire data */
typedef unsigned char stim_mask_t;  /* Stimulus masks */
typedef unsigned char s_type;       /* Cell responses and phases */
struct  rlnm_str { si16 hbcnm; char rnm[LSNAME]; char lnm[LSNAME]; };
typedef struct rlnm_str rlnm;       /* Repertoire-layer name */
struct  dist_str { long d[LDSTAT]; };
typedef struct dist_str dist;       /* Count distribution */
struct  ddist_str { si64 d[LDSTAT]; };
typedef struct ddist_str ddist;     /* Statistical distribution */
struct  hist_str { long h[LHSTAT]; };
typedef struct hist_str hist;       /* Response history */

/* Positions of dynamic variables in offset arrays.
*  N.B.  d3chng cannot test existence of items at index zero.
*  N.B.  d3fchk assumes values do not change with CNS version. */
#define DVREQ         0       /* Dynamic variable is requested */
/*----------------Connection-level variables----------------*/
#define LIJ           0       /* Source of a connection */
#define CIJ           1       /* Connection strength */
#define CIJ0          2       /* Baseline connection strength */
#define MIJ           3       /* "Modifying substance" (S7/14) */
#define DIJ           4       /* Presynaptic delay */
#define PPF           5       /* Paired pulse facilitation (S12) */
#define PPFT          6       /* PPF timer */
#define RBAR          7       /* Mean s(j) (S7/14) */
#define CNNDV         8       /* Next even > highest connection var */
#define cnexists(ix,var) (ix->cnoff[var] >= DVREQ)
#define cnrqst(ix,var)   (ix->cnoff[var]  = DVREQ)
/*-------------Connection-type-level variables-------------*/
#define NUK           0       /* Number connections used */
#define AK            1       /* Afference */
#define IFD           2       /* Feature-detector number */
#define XNNDV         4       /* Next even > highest conntype var */
#define xnexists(ix,var) (ix->xnoff[var] >= DVREQ)
#define xnrqst(ix,var)   (ix->xnoff[var]  = DVREQ)
/*-------------------Cell-level variables-------------------*/
#define RGH           0       /* Regeneration history */
#define SBAR          1       /* Mean s(i) */
#define DEPR          2       /* Depression */
#define DST           3       /* Delta spike threshold */
#define XRM           4       /* Cross-response matrix */
#define PHD           5       /* Phase distribution */
/*                    6          ***AVAILABLE SLOT*** */
#define CNDG          7       /* Gated conductance */
#define IONC          8       /* Ion concentration */
#define QBAR          9       /* Mean s(i) slower than SBAR */
#define IZVU         10       /* Izhikevich v (IL4S20 mV), u (S23),
                              *  i3vr (S20) */
#define IZRA         11       /* Izh. a,b,c,d incrs. (S14,S7/14) */
#define CTNDV        12       /* Next even > highest celltype var */
#define ctexists(il,var) (il->ctoff[var] >= DVREQ)
#define ctrqst(il,var)   (il->ctoff[var]  = DVREQ)
/*------------------Group-level variables-------------------*/
#define IONG          0       /* Ion concentration for group */
#define GPNDV         2       /* Next even > highest group var */
#define gpexists(il,var) (il->gpoff[var] >= DVREQ)
#define gprqst(il,var)   (il->gpoff[var]  = DVREQ)

/* Scale factors */
#define  Sv2mV        6       /* Shift for old sj (virt) to mV  */
#define  Sr2mV        7       /* Shift for old responses to mV  */
#define  Ss2hi       16       /* Shift short to high order bits */
#define  FBsi         7       /* Fraction bits in Vm (s values) */
#define  FBsv         8       /* Fraction bits in virtual cells */
#define  FBdf        15       /* Fraction bits in vdamp factors */
#define  FBwk        20       /* Fraction bits in response work */
#define  FBIu        22       /* Fraction bits in Izhikevich u  */
#define  FBCm        23       /* Fraction bits in Cm            */
#define  FBrs        23       /* Fraction bits in raw aij sums  */
#define  FBsc        24       /* Fraction bits in varied scales */
#define  FBkn        28       /* Fraction bits in phase kernels */
#define  FBfo        28       /* Fraction bits in falloff curve */
#define  FBIab       28       /* Fraction bits in Izhikevich a,b*/
#define  FBod        30       /* Fraction bits in omega (decay) */
#define  FBgt        30       /* Fraction bits in conductances  */
#define  IBod         2       /* Integer  bits in omega (decay) */
#define  IBkn         4       /* Integer  bits in phase kernels */
#define  IBsc         8       /* Integer  bits in varied scales */
#define  IBwk        12       /* Integer  bits in response work */
#define   S4         16
#define   S7        128
#define  dS7        128.0
#define   S8        256
#define  dS8        256.0
#define dS10       1024.0
#define  S12       4096
#define  S13       8192
#define dS13       8192.0
#define  S14      16384
#define dS14      16384.0
#define  S15      32768
#define  S16      65536
#define dS16      65536.0
#define  S20    1048576
#define dS20    1048756.0
#define  S23    8388608
#define dS23    8388608.0
#define  S24   16777216
#define dS24   16777216.0
#define  S27  134217728
#define dS27  134217728.0
#define  S28  268435456
#define dS28  268435456.0
#define  S30 1073741824
#define dS30 1073741824.0

/* Values used to define signs of connections,
*  NEXIN = number of signs.  */
#define NEXIN 2
enum ConnSign { EXCIT = 0, INHIB };
/* N.B. Some code depends on numeric values of these defs,
*  modify only with great care.  */
#define PSP_POS      0x01     /* Postsynaptic potential */
#define PSP_NEG      0x02
#define CNSGN_POS    0x10     /* Connection multipler  */
#define CNSGN_NEG    0x20
#define SJSGN_POS    0x40     /* Presynaptic potential */
#define SJSGN_NEG    0x80
#define PSP_SSHFT       4     /* Shift to get CNSGNs to PSPs */

/* Values used to specify the sources of connections
*  (CONNTYPE cnsrctyp, MODBY msrctyp, VCELL vtype, etc.).
*  (Moved here from pfglobal.h when PLATFORM support removed.)  */
#define BADSRC        255     /* Error signal */
#define REPSRC          0     /* Repertoire */
#define IA_SRC          1     /* Input array */
#define VALSRC          2     /* Value */
#define VJ_SRC          3     /* Virtual joint  */
#define VW_SRC          4     /* Virtual window */
#define VT_SRC          5     /* Virtual touch  */
#define TV_SRC          6     /* TV input */
#define PP_SRC          7     /* Image preprocessor input */
#define D1_SRC          8     /* Darwin 1 input */
#define VH_SRC          9     /* Virtual hand vision */
#define ITPSRC         10     /* Image touch pad */
#define USRSRC         11     /* User-defined source */
#define NRST           11     /* Number of built-in source types */

/* Values used to specify neuronal response functions.
*  N.B.  Some code depends on the numerical ordering of these
*  definitions -- modify only with great care.  IZH3X, IZH7X
*  versions are used only in rspmethu in d3bgis-d3go.  */
enum RespFunc { RF_NEVER_SET=0, RF_KNEE, RF_STEP, RF_SPIKE,
   RF_TANH, RF_BREG, RF_IZH3, RF_IZH7, RF_IZH3X, RF_IZH7X };

/* Values used to define the color mode of IA and TV sources.
*  When used with BBDs, these definitions must match the color
*  modes defined in bbd.h.  Col_R4 is a flag for float senses.  */
enum ColorMode { Col_GS = 0, Col_C8, Col_C16, Col_C24, Col_R4 };

/* Values used to define color sensitivity of connections.
*  The default, when no value is entered, will be BandW (0) */
#define NColorDims  3         /* Dimensions in color space */
#define NColorSels 10         /* Number of items in the enum */
enum ColorSens { BandW = 0, Blue, Green, Red,
   RmG, BmR, GmB, GmR, RmB, BmG };

/* Values used to define boundary condition options.
*  N.B.  Not all of these will be used in every occurrence  */
#define BC_NKEYS        8     /* Number of defined keys */
enum BoundaryCdx { BC_ZERO = 0, BC_NORM, BC_NOISE, BC_EDGE,
   BC_MIRROR, BC_TOROIDAL, BC_MEAN, BC_HELICAL };

/* RP->kra reset type indexes.  If multiple types are requested
*  in the same trial, the highest rlev and OR of the kract bits
*  are used.  RLtuse and RLtdef must be the highest values, as
*  they have no input keys in d3g3rset.  */

#define NRLACT  6             /* No. of enterable action suboptions */
#define NRLDIM 10             /* No. of ReLops = dim of RP->kra */
enum ReLops { RLnone = 0,     /* Always handy no-operation */
   RLtent,                    /* Trial codes entered */
   RLtmr,                     /* Timer event */
   RLevnt,                    /* Environment or BBD event */
   RLval,                     /* Value event */
   RLser,                     /* New series */
   RLnow,                     /* NOW on RESET control card */
   RLtuse,                    /* Trial codes used */
   RLtdef,                    /* Trial codes deferred during special
                              *  post-event trial  */
   RLact };                   /* Action combination built here */

/* Values used to define types of CIJ/CIJ0/MIJ/PPF/RJ/SJ/ESJ plots */
enum KIJPlot { KCIJC = 0, KCIJC0, KCIJM, KCIJPPF, KCIJR, KCIJS,
   KCIJES, KCIJSEL };

/* Values used to define kwsreg calls from kwscan scans */
#define KWS_RLNM       "0"    /* Call d3rlnm() */
#define KWS_DECAY      "1"    /* Call getdecay() */
#define KWS_TIMNW      "2"    /* Call gettimnw() */
#define KWS_NAPC       "3"    /* Call getnapc() */
#define KWS_CSEL       "5"    /* Call getcsel() */
#define KWS_CSEL3      "6"    /* Call getcsel3() */
/* Values used to define kwjreg calls from kwscan scans */
#define KWJ_THR        "1"    /* Call getthr() */
#define KWJ_GARB       "2"    /* Call g1arb() */

/* Codes used in RK.iexit.  Note that code 1 is used in ROCKS library
*  and extensively in CNS for input errors, and code 2 (ENV_ERR) is
*  used in the env library.  These are not #defined here.  */
#define FILE_ERR        4     /* Error interpreting an input file */
#define CMDLINE_ERR     8     /* Error in a command-line parameter */
#define EXTDEV_ERR     16     /* Error in a BBD device */

/* Return codes given by numerous Darwin3 routines: */
#define D3_NORMAL       0     /* Successful completion */
#define D3_PREM_EOF     1     /* Premature end of file encountered */
#define D3_FATAL        2     /* Fatal error */

/* Define D3G_NOINCL to omit inclusions and prototypes */
#ifndef D3G_NOINCL

/* Library and system header files */
#define  NEWPOOLS
#include "rkilst.h"
#include "nsitools.h"
#include "memshare.h"

/* Environment header file */
#include "env.h"

/* Control block header files */
#ifndef NXDR
#include "d3nxdr.h"
#endif
#include "d3memacc.h"
#include "misc.h"
#include "effarb.h"
#include "modality.h"
#include "conduct.h"
#include "conntype.h"
#include "inhibblk.h"
#include "vcdef.h"
#include "vbdef.h"
#include "modby.h"
#include "celltype.h"
#include "repblock.h"
#include "rpdef.h"
#ifdef LIJTYPE
#include "lijarg.h"
#endif
#ifndef PARn
#include "rpdef0.h"
#endif

/* Function call prototypes (alphabetical): */

/* Prototypes available on all node types */
void d3exch(struct CELLTYPE *);
#ifdef GCC
void d3exit(long, char *, long) __attribute__ ((noreturn));
#else
void d3exit(long, char *, long);
#endif
void d3lplt(struct CELLTYPE *, s_type *, int, float,
     float, float, float);
int  d3ngph(float, float, float, float, char *, char *, char *, int);
void d3rplt(struct REPBLOCK *, int, float, float, float, float);
void d3rprt(struct REPBLOCK *);
void d3rpsi(struct CELLTYPE *, int kd);
long d3woff(struct CONNTYPE *ix);
void d3zval(int);
long findcell(struct CELLTYPE *il, long cell);
char *fmtlrlnm(struct CELLTYPE *il);
char *fmturlnm(struct CELLTYPE *il);
char *fmturlcn(struct CELLTYPE *il, long icell);
int  getoas(struct CELLTYPE *il, ui32 bit);
void initsi(struct CELLTYPE *, int ni1, int ni2);
long npsps(int ssck);
void setinsi(struct CELLTYPE *, int ni1, int ni2);

#ifndef PAR0            /* Prototypes not included on PAR0 nodes */
void d3cijx(struct CONNTYPE *ix);
void cndshft(struct CELLTYPE *il, struct CONDUCT *pcnd, int s);
long d3decay(struct DECAYDEF *, long *, long, long *);
void d3dijx(struct CONNTYPE *ix);
si32 d3i3vr(struct IZ03DEF *pi3, si32 b);
void d3kij(struct CELLTYPE *il, int kcall);
void d3kiji(struct CELLTYPE *il, ui32 icell);
void d3kijx(struct CONNTYPE *ix);
void d3lijx(struct CONNTYPE *ix);
void d3sjx(struct CONNTYPE *ix);
void inition(struct REPBLOCK *, ui16 bypass);
long lfofftbl(struct INHIBBLK *ib, int kmax);
void mfofftbl(struct INHIBBLK *ib);
FoffFn qfofffnc(struct INHIBBLK *ib);
void resetu3(struct CELLTYPE *il, ui32 icell);
void resetu7(struct CELLTYPE *il, ui32 icell);
void resetw(struct CELLTYPE *il, ui32 icell);
void sumrmod(struct CELLTYPE *il, ui16 curr_cyc1);
#endif

#ifndef PARn            /* Prototypes not included on PARn nodes */
void autdflt(struct AUTSCL *paut);
void ckkam(long tkam);
void cndflt(struct CELLTYPE *il, struct CONNTYPE *ix);
void cndflt2(struct CELLTYPE *il, struct CONNTYPE *ix);
void ctdflt(struct CELLTYPE *il);
void *d3alfpcv(size_t n, size_t size, char *msg);
void *d3alfprv(void *ptr, size_t length, char *msg);
void d3alfree(void *block, char *msg);
int  d3bxvp(struct INHIBBLK *ib);
long d3cndl(struct CELLTYPE *il);
void d3cnfg(int, stim_mask_t *);
long d3ctbl(int mktbl);
void d3mark1(struct MODALITY *pmdlt, ui16 isn, ui16 ign, short val);
int  d3pxq(int);
void d3rlnm(rlnm *rlname);
void *d3uprg(int kpar);
void d3upcl(void);
int  d3vplt(struct VCELL *pvc, long itr, float xl, float yl,
     float width, float height);
long dtgated(struct CONDUCT *pcnd, struct CELLTYPE *il);
void dupmsg(char *typnm, char *badnm, txtid hnm);
void effdflt(struct EFFARB *pea);
struct CELLTYPE *findln(rlnm *rlname);
struct MODALITY *findmdlt(char *mdltname, int iwdw);
struct PCF *findpcf(int hpcfn);
struct VCELL *findvc(char *vsrcrid, int ivcid, int ivcndx,
   char *erstring);
void getitp(void);
void itpdflt(struct IMGTPAD *pitp);
char *fmtrlnm(rlnm *rlname);
char *fmtcsel(int csel);
char *fmtvcn(struct VCELL *pvc);
char *fmtvcns(struct VCELL *pvc);
void g1arb(char *type, struct EFFARB *pea);
#ifdef BBDS
void g1bbdn(txtid *phost, txtid *pname);
#endif
void g2amplif(struct CNDFLT *pdd);
void g2autsc(struct AUTSCL *paut);
void g2breg(struct BREGDEF *pbg);
void g2decay(struct CELLTYPE *il, struct DCYDFLT *pcd,
     struct CNDFLT *pdd);
void g2delay(struct CNDFLT *pdd);
void g2dprsn(struct DEPRDFLT *pdpr);
void g2iz03(struct IZ03DEF *piz3);
void g2iz07(struct IZ07DEF *piz7);
void g2param(struct CONNTYPE *ix, struct CNDFLT *pdd, int iamthr);
void g2phase(struct PHASEDEF *ppd, struct DCYDFLT *pdd);
void g2ppf(struct CNDFLT *pdd, struct PPFDATA *ppfd);
void getcolor(void);
void getcond(struct CELLTYPE *il, struct CONNTYPE *ix);
void getcsel(struct EXTRCOLR *pxtc);
void getcsel3(struct EXTRCOLR *pxtc);
void getcset(struct CELLTYPE *il);
void getcshm(struct EXTRCOLR *pxtc, char *emsg);
void getdecay(struct DECAYDEF *pdd);
int  getffn(void);
void getijpl(enum KIJPlot kijpl);
void getion(struct REPBLOCK *ir, struct CELLTYPE *il);
void getnapc(byte *pnapc);
void getpcf(struct PCF *ppcf, int kpcf);
void getplab(void);
void getpline(void);
void getploc(void);
void getprobe(void);
int  getrspfn(void);
void getsave(void);
struct SFHMATCH *getsfhm(int hmtype, void *path);
int  getshape(void);
void getsicol(struct CTDFLT *pctd);
int  getsnsid(char *srcrid);
void getstat(void);
void getthr(char *pfmt, short *thresh);
int  gettimno(char *timlet, int k01);
void gettimnw(int *ntim);
void getway(struct CNDFLT *pdd);
void getvrest(struct CTDFLT *pctd, int ierr);
void gtkregen(void);
void gtpscale(void);
void ibdflt(struct INHIBBLK *ib, int);
void initsfhm(void);
void kamchk(struct CONNTYPE *ix);
void linksfhm(struct SFHMATCH *phm, int hmtype);
void mddflt(struct MODBY *pmb);
void mdltdflt(struct MODALITY *pmode);
struct GRPDEF *mkgrpdef(int nds);
void prbdflt(struct PRBDEF *pprb);
int  readbc(int bcmask);
int  readlid(rlnm *idtyp, short *itemnum, int limit, char *name);
int  readrlnm(rlnm *rlname, struct CELLTYPE *il);
void rpdflt(struct REPBLOCK *ir);
void setbscompat(byte kbs);
void setrlnm(rlnm *rlname, char *rname, char *lname);
void vbdflt(struct VBDEF *ivb);
void vcdflt(struct VCELL *pvc, char *vsrcrid);
#endif   /* PARn */
#endif   /* D3G_NOINCL */

/* Message types for parallel version
*  (0x3000-0x301F are reserved for CNS) */

#ifdef PAR
#define UPDATE_SI_MSG   0x3005   /* Cell states */
#define STATS_MSG       0x3006   /* Statistics */
#define GRPMSK_MSG      0x3007   /* Group masks */
#define AUTSID_MSG      0x3008   /* AUTOSCALE s(i) response data */
#define AUTSCL_MSG      0x3009   /* AUTOSCALE scales */
#define XSTATS_MSG      0x300A   /* Cross-response stats */
#define INTERRUPT_MSG   0x300B   /* Check for group3 interrupt */
#define LOOP_EXIT_MSG   0x300C   /* Check for on-line interrupt */
#define CP_CYCLE_MSG    0x300D   /* Broadcast CP struct */

#define EXCH_D1_MSG     0x3011   /* Exchange D1 scores */
#define AMPL_D1_MSG     0x3012   /* Amplify D1 repertoires */
#define IJPL_DATA_MSG   0x3014   /* CIJPLOT/MIJPLOT data */
#define GSTA_MSG        0x3015   /* Statistics to host */
#define DP_DATA_MSG     0x3016   /* Detail print data */
#define SIMDATA_MSG     0x3017   /* Graph data message */
#define SAVENET_MSG     0x3018   /* Save repertoire data */
#define RESTORE_MSG     0x3019   /* Restore repertoire data */
#define ELIJ_MSG        0x301A   /* External Lij data */
#endif

/* Message types for host communication.
*  N.B.  These messages may be used on serial and parallel systems.
*  The definitions must agree with corresponding message types
*  defined in packet.h on UNIX and OS9 computers.  (ABP code that
*  dynamically adjusted the numeric codes for these message types
*  was removed in V8A, 12/07/96, GNR.)  */

#define PAUSECTL_MSG    0x5005   /* Pause control */

/* Macros for assigning fixed point values */
#define LFIXP(A,B) \
   ((long)(((A)*(float)(1UL<<(B))) + ((A)<0 ? -0.5 : 0.5)))
#define JFIXP(A,B) \
   ((si32)(((A)*(float)(1UL<<(B))) + ((A)<0 ? -0.5 : 0.5)))
#define SFIXP(A,B) \
   ((si16)(((A)*(float)(1UL<<(B))) + ((A)<0 ? -0.5 : 0.5)))
#define ULFIXP(A,B) \
   ((unsigned long)(((A)*(float)(1UL<<(B))) + 0.5))
#define UJFIXP(A,B) \
   ((ui32)(((A)*(float)(1UL<<(B))) + 0.5))
#define USFIXP(A,B) \
   ((ui16)(((A)*(float)(1UL<<(B))) + 0.5))

/* Macros for checking long fixed point addition for overflow--
*  Can be replaced with in-line assembler code where applicable.
*  (Error codes are defined as bit shifts so a hardware overflow
*  bit can be shifted and stored without need for an 'if' test.
*  Not all of these macros and bits are necessarily used in
*  in current code.)
*/
#define addck(it,ia,bit)            /* Args and sum any sign */ \
   { register long t = it + (ia); \
   if (((t ^ (ia)) & ~(it ^ (ia))) < 0) RP->ovflow |= (1L<<bit); \
   it = t; }
#define addckp(it,ia,bit)           /* Args and sum positive */ \
   { if ((it += (ia)) < 0) RP->ovflow |= (1L<<bit); }
#define addckn(it,ia,bit)           /* Args and sum negative */ \
   { if ((it += (ia)) > 0) RP->ovflow |= (1<<bit); }
#define addcks(it,ia,bit)           /* Args and sum same sign */ \
   { if (((it += (ia)) ^ (ia)) < 0) RP->ovflow |= (1<<bit); }
/* Define overflow error bits for reporting in RP->ovflow */
#define OVF_GEN   0        /* Generic overflow */
#define OVF_RFN   1        /* Response function overflow */
#define OVF_SPEC  2        /* Specific connection overflow */
#define OVF_GEOM  3        /* Geometric connection overflow */
#define OVF_MOD   4        /* Modulatory connection overflow */
#define OVF_PHAS  5        /* Overflow in phase calculation */
#define OVF_DCY   6        /* Overflow in decay calculation */
#define OVF_VDEP  7        /* Overflow in volt-dep scaling */
#define OVF_AMP   8        /* Overflow in amplification */
#define OVF_VAL   9        /* Overflow in value calculation */
#define OVF_RSET 10        /* Overflow in reset calculation */
#define OVF_IZHI 11        /* Overflow in Izhikevich calcs */
#define OVF_BREG 12        /* Overflow in Brette-Gerstner calcs */
#define OVF_AUTS 13        /* Overflow in Autoscale calcs */
#define OVF_ALLO 14        /* Overflow in memory allocation */
#define OVF_D1   15        /* Overflow in Darwin 1 calcs
                           *  (always highest OVF value) */

/* Macro to calculate standard noise function (with dummy update).
*  Result scale = S(nmn), S(nsg) = S(nmn) + 4, S(nfr) = 24.  */
#define d3noise(nmn, nsg, nfr, nsd) \
   (udev(&nsd)>>7 <= (nfr) ? ndev(&nsd, nmn, nsg) : (udev(&nsd), 0))

/* Macro to calculate memory size of s(i),phi without multiplying */
#define spsize(n,s) (((n) << (s)) + (n))

/* Macro to locate a translation table entry */
#define ttloc(ix) (NXDRTT+(ix&MBTMask))

/*---------------------------------------------------------------------*
*                    Terminal error codes for CNS.                     *
*---------------------------------------------------------------------*/

/* See /home/cdr/src/crk/errtab for global list of error codes.
*  Codes in the range 410-499 are globally reserved for CNS.
*  Codes in range 300-399 were used in IBM version but not C version.
*  No ifdef PAR because some of these are used in SER version.
*  N.B.  UNIX only recognizes codes modulo 256, too bad.  */

#define ABEXIT_ERR          410   /* Unadorned abexit() or abexitm()
                                  *  call (from library) */
#define LIMITS_ERR          411   /* Objects in input exceeded size
                                  *  of counter for that object type */
#define SAVEXF_ERR          413   /* Ran out of save files */
#define LIJXF_ERR           414   /* Ran out of Lij files */
#define FILECK_ERR          415   /* Error checking input file */
#define REPLAY_ERR          416   /* Replay file structure wrong */
#define VALMAG_ERR          417   /* External value magnitude */
#define LIJBDV_ERR          418   /* CONNLIST bad value error */
#define RESETCIJ_ERR        419   /* Reset CIJ<-CIJ0 and no CIJ0 */
#define ROCKIO_ERR          420   /* Rocks exit flag set (I/O) */
#define NODENO_ERR          421   /* Single node allocated in PAR */
#define D1WBLO_ERR          424   /* D1 wobble-nbpe overflow error */
#define VMAMPL_ERR          425   /* Value-modulated amp bounds */
#define OVERFLOW_ERR        426   /* Any kind of overflow error */
#define PREPROC_ERR         427   /* Image preprocessor error */
                        /*  428      Obsolete phasing overflow */
#define VIDEO_ERR           429   /* Video error */
                        /*  430      Obsolete NOMAD platform error */
#define PXQQ_ERR            431   /* Got PXQQ when not online */
#define PAUSEMSG_ERR        432   /* Error in pause message */
                        /*  433-435  Obsolete VDR errors */
#define BADCELL_ERR         436   /* Requested cell does not exist */
#define BADPTR_ERR          437   /* Bad pointer detected (d3tsvp) */
#define EXCH_LEN_ERR        438   /* Bad segment length in d3exch */
#define UPROG_ERR           441   /* User programs not supported */
#define BBD_ERR             442   /* BBD error */
#define NREAD_ERR           448   /* Node-node read error */
#define NWRITE_ERR          449   /* Node-node write error */
#define QUICKEXIT_RQST      450   /* Quick exit requested */

#define FIRST_DEVERR        480   /* Errors above this number are
                                  *  development errors--give message
                                  *  for user to contact the author.  */
#define GETTHR_ERR          482   /* Bad format string to getthr() */
#define EFFLNK_ERR          483   /* Bad effector linkage to CELLTYPE */
#define VALDAT_ERR          484   /* Value data array is missing */
#define DPLLEN_ERR          485   /* Detail print line too long */
#define LIJBND_ERR          486   /* Lij out of bounds entering lb2b  */
#define COLOR_ERR           487   /* Bad color selector encountered */
#define CONDCT_ERR          488   /* Conductance refd nonexistent ict */
#define RESITM_ERR          489   /* Req'd item not in RESTORE file */
#define RESTYP_ERR          490   /* Type mismatch in RESTORE file */
#define INITSI_ERR          491   /* Tried to init > mxsdelay si's */
#define LSRSTR_ERR          492   /* Error saving lsrstr in d3allo */
#define IJPLEND_ERR         493   /* Unexpected end of IJPLOT data */
#define IONREG_ERR          494   /* ION on CELLTYPE has no REGION */
#define LIJGEN_ERR          495   /* Unsupported Lij gen opts */
#define DPSYNC_ERR          496   /* Detail print out of synch */
#define LEIFAMP_ERR         497   /* No Leif amplification */
#define NOCLST_ERR          498   /* No cell list for saved items */
                        /*  499      Obsolete library exit (now 450) */
