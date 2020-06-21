/* (c) Copyright 1988-2018, The Rockefeller University *11115* */
/* $Id: d3global.h 78 2018-08-02 18:36:58Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                        D3GLOBAL Header File                          *
*                                                                      *
*  This header file should precede all other CNS-specific headers:     *
*  Global values are set here which are referenced by other headers.   *
*                                                                      *
************************************************************************
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
*  Rev, 01/05/12, GNR - Move redundant defs from simdata.h, etc.       *
*  Rev, 06/27/12, GNR - Add xrmt,xstt typedefs                         *
*  Rev, 09/20/12, GNR - Use incomplete struct types to avoid void defs *
*  V8I, 10/05/12, GNR - Add offdecay proto and qdecay macro            *
*  Rev, 10/21/12, GNR - Remove addck, use crk ja[su]aem macros         *
*  Rev, 05/11/13, GNR - Remove gconn falloff                           *
*  Rev, 05/18/13, GNR - Begin adding allocation-limit error codes      *
*  Rev, 07/17/13, GNR - Add defs of autoscale overrides for [gmc]pspov *
*  Rev, 08/24/14, GNR - Add S30pow macro (renamed SodPow, 10/25/15)    *
*  R66, 02/17/16, GNR - Change long random seeds to typedef si32 orns  *
*                       Define NEWARB here rather than in .c files     *
*  R67, 04/27/16, GNR - Remove Darwin 1 support                        *
*  Rev, 08/13/16, GNR - MPI preparations, move ttloc() to mpitools.h   *
*  R70, 01/24/17, GNR - Define DST_CLOCK0, NHLSSV, kern, cploc structs *
*  R72, 03/30/17, GNR - Define clloc struct                            *
*  R76, 12/02/17, GNR - Add FBvl, OPhA, VMP, IZU, IZVr, renum errors   *
*  R77, 02/08/18, GNR - Add RAW variable at conntype level, new VSBIC  *
***********************************************************************/

#define qqv(s) qvar(s)
#define RUNNOFNM ".cnsrunno"  /* Name of file where run number kept */
#define DST_CLOCK0 0x8200     /* DST value that is clock 0 (expiry) */
#define DST_CLK0S16 ((si32)0x82000000)
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
/* L[DH]STAT must be even to avoid alignment probs--d3nset */
#define LDSTAT       16       /* Dimension of distribution stats */
#define LHSTAT        8       /* Dimension per mdlty of history stats */
#define LSTITL       62       /* Length of stitle array */
#define MXNJNT       15       /* Max number of joints an arm can have */
#define MXNTRT  ('Z'-'A'+3)   /* Max number of user-defined timers,
                              *  allowing for '0' and '1' plus 'A' - 'Z'
                              *  (includes holes in EBCDIC alphabet). */
#define NBPMCE        4       /* Bits per KGNMC matrix entry */
#define NCNThr       20       /* Space for deferred conntype threshs */
#define NCTT          4       /* Number of celltype thresholds (pt..) */
#define qNCTT        "4"      /* Quoted NCTT */
#define NKAFB         8       /* Number of bytes for kaff data:  Only 7
                              *  currently in use, but allow for expan-
                              *  sion, maybe better alignment */
#define PHASE_RES    32       /* Phase resolution--must be power of 2 */
#define qPHR        "32"      /* Quoted PHASE_RES */
#define PHASE_MASK   31       /* Mask to convert phase to unit circle */
#define PHASE_EXP     5       /* Log(2)(PHASE_RES) */
#define PLCSP_BORDER  0.10    /* Blank border in PLCSP plot (< 0.5) */
#define SCANLEN      16       /* Scanning routine token length */
#define qSCANLEN    "16"      /* Quoted SCANLEN */
#define SCANLEN1 (SCANLEN+1)  /*  (with room for string delimiter) */
#define COLORNUM     14       /* Num plot object colors */
/* N.B.  Code in d3ijp* assumes SEQNUM <= 32 and check refs to SEQN_EXP
*  if you want to make it something that is not a power of 2.  */
#define SEQNUM        8       /* Num colors in bubble plot seq */
#define SEQN_EXP      3       /* Log(2)(SEQNUM) */
/* Operation codes for some wbcdin calls */
#define VSBIC  (RK_CTST|RK_QPOS|RK_IORF|RK_NINT|3)
#define LEA2INT (RK_CTST|RK_QPOS|RK_IORF|RK_NINT|SCANLEN-1)

/* Codes for types used in simdata, savblk, etc. */
#define FTYPE   0x00             /* Fixed point, integer */
#define UTYPE   0x01             /* Unsigned fixed point */
#define ETYPE   0x02             /* Real */
#define DTYPE   0x03             /* Double precision real */
#define KTYPE   0x04             /* Colored pixel data */
#define ATYPE   0x05             /* ASCII text data */
#define XTYPE   0x06             /* Hexadecimal data */
#define FMZTYPE 0x07             /* Fixed point, minus zero allowed */

/* Standard constants */
#define PI       3.141592653589793
#define TWOPI    6.283185307179586
#define PIOV2    1.5707963267948965
#define RSQRT2PI 0.398942280  /* Reciprocal of sqrt(2*pi) */
#define DEGS_TO_RADS  1.745329252E-2
#define BINS_TO_RADS  (6.283185308/(double)PHASE_RES)
#define Faraday 9.649E1       /* Faraday in Coulombs/mole * 1E-3 */

/* Incomplete structure defs.  These are used to permit pointers
*  to be constructed to structs defined only in specialized headers.  */
typedef struct CLBLK    CLTYPE;
typedef struct CLHDR    CHTYPE;
typedef struct EFFARB   EFFTYPE;
typedef struct GSDEF    GSTYPE;
typedef struct IJPLDEF  CMTYPE;
typedef struct IJPLNODE CMNTYPE;
typedef struct KERNEL   KNTYPE;
typedef struct LIJARG   LIJTYPE;
typedef struct PLBDEF   PLBTYPE;
typedef struct PLNDEF   PLNTYPE;
typedef struct PREPROC  PPTYPE;
typedef struct RFdef    RKFILE;     /* Is also type rkfd in rfdef.h */
typedef struct SDSELECT SDSTYPE;
typedef struct TVDEF    TVTYPE;
#ifdef BBDS
typedef struct BBDDev_t BBDTYPE;
#endif

/* Type definitions */
typedef float armwdw_type;          /* Arm/window position data */
typedef ui32 fdm_type;              /* Feature-detecting matrix data */
typedef long gac_type;              /* Conductance activation (S30) */
typedef si32 jdat_type;             /* si32 for broadcast */
typedef unsigned short mdlt_type;   /* Bits identifying a modality */
typedef si32 orns;                  /* Old random number seed */
typedef unsigned short txtid;       /* Text locator */
typedef unsigned char rd_type;      /* Repertoire data */
typedef unsigned char stim_mask_t;  /* Stimulus masks */
typedef unsigned char s_type;       /* Cell responses and phases */
typedef ui32 xrmt;                  /* XRM, other ui32,ui64 stats */
typedef ui64 xstt;                  /*    exchanged in d3stat() */
struct xysh_str {
   short sx,sy; };
typedef struct xysh_str xysh;       /* Short coords in a 2-D array */
struct clloc_str {
   short clnum, clndx; };              /* Cell list number and index */
typedef struct clloc_str clloc;     /* Cell list locator */
struct kern_str {
   si16 kx,ky; si16 ori; si16 Cij; };
typedef struct kern_str kern;       /* KGNKN kernel storage */
struct  rlnm_str { si16 hbcnm; si16 hjnt; /* hjnt counts from 0, */
   char rnm[LSNAME]; char lnm[LSNAME]; }; /* -1 indicates default */
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
#define AK            1       /* Afference (S20/27) */
#define IFD           2       /* Feature-detector number */
#define RAW           3       /* Raw afference (before vdep) (S20/27) */
#define XNNDV         4       /* Next even > highest conntype var */
#define xnexists(ix,var) (ix->xnoff[var] >= DVREQ)
#define xnrqst(ix,var)   (ix->xnoff[var]  = DVREQ)
/*-------------------Cell-level variables-------------------*/
#define RGH           0       /* Regeneration history */
#define SBAR          1       /* Mean s(i) */
#define DEPR          2       /* Depression (S8) */
#define DST           3       /* Delta spike threshold */
#define XRM           4       /* Cross-response matrix */
#define PHD           5       /* Phase distribution */
#define PSI           6       /* Amplification refractoriness (S15) */
#define CNDG          7       /* Gated conductance */
#define IONC          8       /* Ion concentration */
#define QBAR          9       /* Mean s(i) slower than SBAR */
#define IZU          10       /* Izhikevich u (S22) */
#define IZVr         11       /* Izhikevich Vrest (S20) */
#define IZRA         12       /* Izh. a,b,c,d incrs. (S14,S7/14) */
#define VMP          13       /* Vm(t-1) (mV IL4 S20/27) */
#define CTNDV        14       /* Next even > highest celltype var */
#define ctexists(il,var) (il->ctoff[var] >= DVREQ)
#define ctrqst(il,var)   (il->ctoff[var]  = DVREQ)
/*------------------Group-level variables-------------------*/
#define IONG          0       /* Ion concentration for group */
#define GPNDV         2       /* Next even > highest group var */
#define gpexists(il,var) (il->gpoff[var] >= DVREQ)
#define gprqst(il,var)   (il->gpoff[var]  = DVREQ)

/* Scale factors */
#define  Sr2vS        4       /* Shift in scl real->virtual Sj  */
#define  Sv2mV        6       /* Shift for old sj (virt) to mV  */
#define  Sr2mV        7       /* Shift for old responses to mV  */
#define  Ss2hi       16       /* Shift short to high order bits */
#define  FBsi         7       /* Fraction bits in Vm (s values) */
#define  FBsv         8       /* Fraction bits in virtual cells */
#define  FBvl         8       /* Fraction bits in values        */
#define  FBws        10       /* Fraction bits in wayset scales */
#define  FBim        14       /* Fraction bits in image pixels  */
#define  FBCij       15       /* Fraction bits in Cij           */
#define  FBdf        15       /* Fraction bits in vdamp factors */
#define  FBxy        15       /* Fraction bits in x,y coords    */
#define  FBtm        20       /* Fraction bits for ternary modul*/
#define  FBwk        20       /* Fraction bits in response work */
#define  FBIu        22       /* Fraction bits in Izhikevich u  */
#define  FBCm        23       /* Fraction bits in Cm            */
#define  FBrs        23       /* Fraction bits in raw aij sums  */
#define  FBik        24       /* Fraction bits in image kernels */
#define  FBsc        24       /* Fraction bits in varied scales */
#define  SjVPB       24       /* Bits alloc for Sj in VP data   */
#define  FBlw        27       /* Fraction bits in log(omega)    */
#define  FBfi        28       /* Fraction bits in fade-in scales*/
#define  FBkn        28       /* Fraction bits in phase kernels */
#define  FBIab       28       /* Fraction bits in Izhikevich a,b*/
#define  FBod        30       /* Fraction bits in omega (decay) */
#define  FBgt        30       /* Fraction bits in conductances  */
#define  FBrf        31       /* Fraction bits in a random frac */
#define  IBod         2       /* Integer  bits in omega (decay) */
#define  IBkn         4       /* Integer  bits in phase kernels */
#define  IBsc         8       /* Integer  bits in varied scales */
#define  IBwk        12       /* Integer  bits in response work */
#define  IBnc        28       /* Max integer bits in nc         */
#define   S4         16
#define   S7        128
#define  dS7        128.0
#define   S8        256
#define  dS8        256.0
#define  S10       1024
#define dS10       1024.0
#define  S12       4096
#define  S13       8192
#define dS13       8192.0
#define  S14      16384
#define dS14      16384.0
#define  S15      32768
#define dS15      32768.0
#define dSxy      32768.0
#define  S16      65536
#define dS16      65536.0
#define  S20    1048576
#define dS20    1048576.0
#define  S23    8388608
#define dS23    8388608.0
#define  S24   16777216
#define dS24   16777216.0
#define  S27  134217728
#define dS27  134217728.0
#define  S28  268435456
#define dS28  268435456.0
#define  S30 1073741824
#define  Sod 1073741824       /* Scale for decay omega terms */
#define dS30 1073741824.0
#define dS31 2147483648.0
/* Scales based on named FB constants above (for SodPow) */
#define dSFBdf      32768.0
#define dSFBlw  134217728.0
#define dSFBod 1073741824.0

/* Values used to define signs of connections */
#define NEXIN 2               /* Num cases: EXCIT or INHIB */
#define NSnS  2               /* Num cases: self or nonself */
#define NASOP (NEXIN*NSnS)    /* Num cases: autoscale multipliers */
enum ConnSign { EXCIT = 0, INHIB };
/* N.B. Some code depends on numeric values of these defs,
*  modify only with great care.  */
#define PSP_POS      0x01     /* Postsynaptic potential */
#define PSP_NEG      0x02
#define CNSGN_POS    0x10     /* Connection multipler  */
#define CNSGN_NEG    0x20
#define SJSGN_POS    0x40     /* Possible signs of inputs */
#define SJSGN_NEG    0x80
#define PSP_SSHFT       4     /* Shift to get CNSGNs to PSPs */
#define OPhA            1     /* Offset of phased affs in rax[pn] */

/* Values used to define usage overrides for conntypes */
#define USG_INH   PSP_POS     /* I Use inhibitory inputs only */
#define USG_EXC   PSP_NEG     /* E Use excitatory inputs only */
#define USG_DEP      0x04     /* D Include in depolarizing sums */
#define USG_HYP      0x08     /* H Include in hyperpolarizing sums */
#define USG_IIR      0x10     /* R Include in input rescaling */
#define USG_XIR      0x20     /* X Exclude from input rescaling */
#define USG_QIR      0x40     /* USG_IIR or KAUT_x combined */

/* Values used to specify the sources of connections
*  (CONNTYPE cnsrctyp, MODBY msrctyp, VCELL vtype, etc.).
*  (Moved here from pfglobal.h when PLATFORM support removed.) */
#define BADSRC        255     /* Error signal */
#define REPSRC          0     /* Repertoire */
#define IA_SRC          1     /* Input array */
#define VALSRC          2     /* Value */
#define VJ_SRC          3     /* Virtual joint */
#define VH_SRC          4     /* Virtual hand vision */
#define VW_SRC          5     /* Window kinesthesia */
#define VS_SRC          6     /* Window scan (kgen=S) */
#define VT_SRC          7     /* Virtual touch */
#define TV_SRC          8     /* TV input */
#define PP_SRC          9     /* Image preprocessor input */
#define ITPSRC         10     /* Image touch pad */
#define USRSRC         11     /* User-defined source */
#define NRST           11     /* Number of built-in source types */
#define DP_SRC         12     /* Special use for d3dprt Sjraw */

/* Values used to specify neuronal response functions.
*  N.B.  Some code depends on the numerical ordering of these
*  definitions -- modify only with great care.  IZH3X, IZH7X
*  versions are used only in rspmethu in d3bgis-d3go.  */
enum RespFunc { RF_NEVER_SET=0, RF_KNEE, RF_STEP, RF_SPIKE,
   RF_TANH, RF_BREG, RF_IZH3, RF_IZH7, RF_IZH3X, RF_IZH7X };

/* Values used to define color sensitivity of connections.
*  The default, when no value is entered, will be BandW (0) */
#define NColorDims  3         /* Dimensions in color space */
#define NColorSels 10         /* Number of items in the enum */
/* Supposedly ideal mix of colors to get gray */
#define Red2G    6881         /* Red to gray   0.21 (S15) */
#define Grn2G   23593         /* Green to gray 0.72 (S15) */
#define Blu2G    2294         /* Blue to gray  0.07 (S15) */
#define RGB2GScl   15         /* Scale of above constants */
/* If change order of this enum, code at getcshm must be changed */
enum ColorSens { BandW = 0, Red, Green, Blue, Opp,
   RmG = Opp, GmB, BmR, GmR, BmG, RmB };

/* Values used to define boundary condition options.
*  N.B.  Not all of these will be used in every occurrence  */
#define BC_NKEYS        8     /* Number of defined keys */
enum BoundaryCdx { BC_ZERO = 0, BC_NORM, BC_NOISE, BC_EDGE,
   BC_MIRROR, BC_TOROIDAL, BC_MEAN, BC_HELICAL };

/* Values used to define type of conntype fade-in */
enum FdiOps { FDI_None = 0,
   FDI_SBT,       /* Series-by-trial */
   FDI_SBC,       /* Series-by-cycle */
   FDI_TBC        /* Trial-by-cycle */
   };

/* Values used to define reset level categories.  lvtent = trial
*  level entered, lvtuse = trial level used, lvtdef = trial level
*  deferred during special event trial.  lvtuse and lvtdef must be
*  the highest values, as they have no input keys in d3g3rset.  */
enum ReLops { lvtent = 0, lvtimr, lvevnt, lvser, lvnow,
   lvarm, lvwin, lvval, lvtuse, lvtdef };

/* Values used to define types of CIJ/CIJ0/MIJ/PPF/RJ/SJ/ESJ plots */
enum KIJPlot { KCIJC = 0, KCIJC0, KCIJM, KCIJPPF, KCIJR, KCIJS,
   KCIJES, KCIJSEL };

/* Operators used in pause control messages between CNS and
*  driver or graphics interface -- from old packet.h header.  */
enum pausemsgs {
   PSCL_PAUSE,    /* Drvr to CNS: Pause at end of trial */
   PSCL_PSING,    /* CNS to Drvr: Pausing now  */
   PSCL_RSM,      /* Drvr to CNS: Resume  */
   PSCL_RSMABT,   /* Drvr to CNS: Resume and ignore remainder of
                                    cycle card.  */
   PSCL_RSMING,   /* CNS to Drvr: Resuming now  */
   PSCL_EXIT      /* Drvr to CNS: Exit now */
   };

/* Values used to define kwsreg calls from kwscan scans */
#define KWS_RLNM       "0"    /* Call d3rlnm() */
#define KWS_DECAY      "1"    /* Call getdecay() */
#define KWS_TIMNW      "2"    /* Call gettimnw() */
#define KWS_NAPC       "3"    /* Call getnapc() */
#define KWS_PSPOV      "4"    /* Call getpspov() */
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

/* Return codes given by various Darwin3 routines: */
#define D3_NORMAL       0     /* Successful completion */

/* Define D3G_NOINCL to omit inclusions and prototypes */
#ifndef D3G_NOINCL

/* Library and system header files */
#define  NEWARB
#include "rkilst.h"
#include "mpitools.h"
#include "memshare.h"
#include "plotdefs.h"

/* Environment header file */
#include "env.h"

/* Control block header files */
#ifndef NXDR
#ifdef INTEL32
#include "d3nxd32.h"
#else
#ifdef PAR
#include "d3nxd64p.h"
#else
#include "d3nxd64s.h"
#endif
#endif
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
#ifdef BBDS
#include "bbd.h"
#endif
#ifndef PARn
#include "rpdef0.h"
#endif

/* Function call prototypes (alphabetical): */

/* Prototypes available on all node types */
si32 d3decay(struct DECAYDEF *, si32 *, si32 *, si32);
void d3exch(struct CELLTYPE *);
#ifdef GCC
void d3exit(char *mtxt, int ierr, int ival) __attribute__ ((noreturn));
#else
void d3exit(char *mtxt, int ierr, int ival);
#endif
void d3grex(char *mtxt, int ierr, int ival);
si32 d3i3vr(struct IZ03DEF *pi3, si32 b);
si32 d3lani(struct CONNTYPE *ix, si32 *plann);
si32 d3lkni(struct CONNTYPE *ix, ui32 *plkn);
void d3lplt(struct CELLTYPE *, s_type *, int, float, float);
int  d3ngph(float, float, float, float, char *, char *, char *, int);
void d3rplt(struct REPBLOCK *, int, float, float, float, float);
void d3rpsi(struct CELLTYPE *, int kd);
void d3save(int svstage);
/* Codes for expressing time-mode of save file in calls to d3save(): */
#define D3SV_ENDXEQ  0           /* Save at end of execution */
#define D3SV_FMGRP3  1           /* Save once--Group III card */
#define D3SV_NEWSER  2           /* Save at start of trial series */
#define D3SV_ALCFN   4           /* Added length of created filename */
ulng d3woff(struct CONNTYPE *ix);
void d3zval(int);
int  findcell(struct CELLTYPE *il, int cell);
char *fmtlrlnm(struct CELLTYPE *il);
char *fmturlnm(struct CELLTYPE *il);
char *fmturlcn(struct CELLTYPE *il, int icell);
si32 fsdecay(struct DECAYDEF *, si32 *, si32 *, si32);
int  getoas(struct CELLTYPE *il, int koff, byte kbit);
void initsi(struct CELLTYPE *, int ni1, int ni2);
long npsps(int ssck);
void rsdecay(struct DECAYDEF *pdcd, ubig ndcy, int nit);
void updecay(struct DECAYDEF *, si32 *, si32);

#ifndef PAR0            /* Prototypes not included on PAR0 nodes */
void d3cijx(struct CONNTYPE *ix);
void cndshft(struct CELLTYPE *il, struct CONDUCT *pcnd, int s);
void d3dijx(struct CONNTYPE *ix);
void d3kij(struct CELLTYPE *il, int kcall);
void d3kijf(struct CELLTYPE *il, struct CONNTYPE *ix, ui32 icell);
void d3kiji(struct CELLTYPE *il, ui32 icell);
void d3kijx(struct CONNTYPE *ix);
void d3lijx(struct CONNTYPE *ix);
void d3sjx(struct CONNTYPE *ix);
void inition(struct REPBLOCK *, ui16 bypass);
void resetu3(struct CELLTYPE *il, ui32 icell);
void resetu7(struct CELLTYPE *il, ui32 icell);
void resetw(struct CELLTYPE *il, ui32 icell);
void resetauw(struct CELLTYPE *il);
void sumrmod(struct CELLTYPE *il, ui16 curr_cyc1);
#endif

#ifndef PARn            /* Prototypes not included on PARn nodes */
void armdflt(struct ARMDEF *pa);
void autdflt(struct AUTSCL *paut);
si32 chkscl(si64 tscl, int bscl, char *name);
void ckkam(ui32 tkam);
void cndflt(struct CELLTYPE *il, struct CONNTYPE *ix);
void cndflt2(struct CELLTYPE *il, struct CONNTYPE *ix);
void ctdflt(struct CELLTYPE *il);
void *d3alfpcv(size_t n, size_t size, char *msg);
void *d3alfprv(void *ptr, size_t length, char *msg);
void d3alfree(void *block, char *msg);
int  d3bxvp(struct INHIBBLK *ib);
long d3cndl(struct CELLTYPE *il);
void d3cnfg(int, stim_mask_t *);
int  d3cpli(cploc *pcpl, struct CELLTYPE *il, void *psi,
      float xl, float yl, float wd, float ht, int kp);
#define KP_LAYER 0      /* Layer plot */
#define KP_NANAT 1      /* Neuroanatomy plot */
#define KP_CIJPT 2      /* Target cells for Cij-type plot */
#define KP_CIJPS 3      /* Source cells for Cij-type plot */
long d3ctbl(int mktbl);
void d3lime(char *fnnm, int ec);
void d3mark1(struct MODALITY *pmdlt, ui16 isn, ui16 ign, short val);
int  d3pxq(int);
void d3rlnm(rlnm *rlname);
void *d3uprg(int kpar);
void d3upcl(void);
int  d3vplt(struct VCELL *pvc, si32 itr, float xl, float yl,
     float width, float height);
long dtgated(struct CONDUCT *pcnd, struct CELLTYPE *il);
void dupmsg(char *typnm, char *badnm, txtid hnm);
void effdflt(struct EFFARB *pea);
struct CELLTYPE *findln(rlnm *rlname);
struct MODALITY *findmdlt(char *mdltname, int iwdw);
struct PCF *findpcf(int hpcfn);
struct VCELL *findvc(char *vsrcrid, int ivcid, int ivcndx,
     char *erstring);
char *fmtsrcnm(rlnm *srcnm, int ityp);
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
void g2noise(struct NDFLT *pn);
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
void getijpl(enum KIJPlot kijpl);
void getion(struct REPBLOCK *ir, struct CELLTYPE *il);
void getitp(void);
void getnapc(byte *pnapc);
void getpcf(struct PCF *ppcf, int kpcf);
void getplab(void);
void getpline(void);
void getploc(void);
void getprobe(void);
void getpspov(byte *ppspov);
int  getrspfn(void);
void getsave(void);
struct SFHMATCH *getsfhm(int hmtype, void *path);
void getshape(struct CTDFLT *pctd);
void getsicol(struct CTDFLT *pctd);
void getscsrc(struct CTDFLT *pctd);
int  getsnsid(char *srcrid);
void getstat(void);
void getthr(char *pfmt, void *datitm);
int  gettimno(char *timlet, int k01);
void gettimnw(int *ntim);
void getway(struct CNDFLT *pdd);
void getvrest(struct CTDFLT *pctd, int ierr);
void gtkregen(void);
void gtpscale(void);
void ibdflt(struct INHIBBLK *ib, int);
void initsfhm(void);
void itpdflt(struct IMGTPAD *pitp);
void jntdflt(struct ARMDEF *pa, struct JNTDEF *pj, int kj);
void kamchk(struct CONNTYPE *ix);
void linksfhm(struct SFHMATCH *phm, int hmtype);
void mddflt(struct MODBY *pmb);
void mdltdflt(struct MODALITY *pmode);
struct GRPDEF *mkgrpdef(int nds);
void offdecay(struct DECAYDEF *pdcd);
void prbdflt(struct PRBDEF *pprb);
int  readbc(int bcmask);
int  readrlnm(rlnm *rlname, struct CELLTYPE *il);
void rpdflt(struct REPBLOCK *ir);
void setbscompat(int kbs);
void setrlnm(rlnm *rlname, char *rname, char *lname);
void vbdflt(struct VBDEF *ivb);
void vcdflt(struct VCELL *pvc, char *vsrcrid);
void terdflt(struct TERNARY *pvp);
void wdwdflt(struct WDWDEF *pw);
#endif   /* PARn */
#endif   /* D3G_NOINCL */

/* Message types for parallel version
*  (0x3000-0x301F are reserved for CNS) */

#ifdef PAR
#define UPDATE_SI_MSG   0x3000   /* Cell states */
#define STATS_MSG       0x3001   /* Statistics */
#define GRPMSK_MSG      0x3002   /* Group masks */
#define HLSSTAT_MSG     0x3005   /* High/low afference stats */
#define OFLW_CT_MSG     0x3006   /* Overflow count statistics */
#define AUTSIG_MSG      0x3007   /* AUTOSCALE ready signal msg */
#define AUTSID_MSG      0x3008   /* AUTOSCALE s(i) response data */
#define AUTSCL_MSG      0x3009   /* AUTOSCALE scales */
#define XSTATS_MSG      0x300A   /* Cross-response stats */
#define OUTOVFPK_MSG    0x300B   /* OR outnow, ovflow to host */
#define LOOP_EXIT_MSG   0x300C   /* Check for on-line interrupt */
#define CP_CYCLE_MSG    0x300D   /* Broadcast CP struct */
#define CIJ_DIST_MSG    0x300E   /* Distribution of Cij stat */
#define PSI_STAT_MSG    0x300F   /* PSI mean, min, max stats */

#define IJPL_DATA_MSG   0x3014   /* CIJPLOT/MIJPLOT data */
#define DP_DATA_MSG     0x3016   /* Detail print data */
#define SIMDATA_MSG     0x3017   /* Graph data message */
#define SAVENET_MSG     0x3018   /* Save repertoire data */
#define RESTORE_MSG     0x3019   /* Restore repertoire data */
#define ELIJ_MSG        0x301A   /* External Lij data */
#define GREX_HDR_MSG    0x301E   /* Header info for graceful exit */
#define GREX_DATA_MSG   0x301F   /* Message text for graceful exit */
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

/* Define error codes for programmed limit check errors */
#define PLcb(ec)  (EAcb(ec << BITSPERBYTE))
#define PLC_GCWS  1        /* Workspace for serial GCONNs */
#define PLC_GCXS  2        /* Extended GCONN > LAYER SIZE */
#define PLC_RNQY  3        /* Print y groups > UI16_MAX */
#define PLC_KERN  4        /* Connection kernel size > S28 */
#define PLC_TVSZ  5        /* TV image size exceeds ZSIZE */
#define PLC_PPSZ  6        /* Preprocessor input exceeds 16 bits */
#define PLC_ALLO  7        /* General space allocation */

/* Define bits used for reporting overflows in RP->ovflow when
*  CDAT.povec or CD.cdlayr are not available.  Must agree with
*  output labels in d3perr case OVERFLOW_ERR.  Add more as needed.  */
#define OVF_GEN   0        /* Generic overflow */
#define OVF_ALLO  1        /* Overflow in memory allocation */
#define OVF_GEOM  2        /* Geometric connection overflow */
#define OVF_DCY   3        /* Overflow allocating decay memory */
#define OVF_IZHI  4        /* Overflow in Izhikevich calcs */
#define OVF_BREG  5        /* Overflow in Brette-Gerstner calcs */
#define OVF_TSCL  6        /* Overflow in ternary scale calc */
#define OVF_STAT  7        /* Overflow in statistics */
#define OVF_PROB  8        /* Overflow in probe setup */
#define OVF_FCHK  9        /* Overflow retrieving savenet file */
#define OVF_KCON 10        /* Overflow convolving preproc kernels */
#define OVT_END  11        /* One greater than highest defined value */

/* Macro to calculate standard noise function (with dummy update).
*  Result scale = S(nmn), S(nsg) = S(nmn) + 4, S(nfr) = 24.  */
#define d3noise(nmn, nsg, nfr, nsd) \
   (udev(&nsd)>>7 <= (nfr) ? ndev(&nsd, nmn, nsg) : (udev(&nsd), 0))

/* Macro to test in a standard, safe, manner whether decay has been
*  requested in the specified DECAYDEF and returns a value that is
*  guaranteed to be either 0 or 1.  */
#define qdecay(pdcd) ((pdcd)->kdcy != NODECAY)

/* Macro to take a power of an decay factor or similar and return
*  result as a ui32 (Sod).  Floating-point method is probably faster
*  and more accurate than ui32pow(), although without the guarantee
*  of same results on non-IEEE-floating-point (IBM 370) systems.
*  Macro assumes 0 <= omega < 1.0, so there is no need to check for
*  result overflow.  */
#ifdef _ISOC99_SOURCE
#define SodPow(omeg,nit,omsc) (omeg > 0 ? \
   (si32)(dSFBod*pow((double)(omeg)/dS##omsc, (double)nit)) : 0)
#else
#define SodPow(omeg,nit,omsc) (omeg > 0 ? \
   (si32)(dSFBod*exp((double)nit*log((double)omeg/dS##omsc))) : 0)
#endif

/* Macro to calculate memory size of s(i),phi without multiplying */
#define spsize(n,s) (((n) << (s)) + (n))

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
#define ASIZE_ERR           412   /* An array exceeded pgm max size */
#define SAVEXF_ERR          413   /* Ran out of save files */
#define LIJXF_ERR           414   /* Ran out of Lij files */
#define FILECK_ERR          415   /* Error checking input file */
#define REPLAY_ERR          416   /* Replay file structure wrong */
#define VALMAG_ERR          417   /* External value magnitude */
#define LIJBDV_ERR          418   /* CONNLIST bad value error */
#define RESETCIJ_ERR        419   /* Reset CIJ<-CIJ0 and no CIJ0 */
#define ROCKIO_ERR          420   /* Rocks exit flag set (I/O) */
#define NODENO_ERR          421   /* Single node allocated in PAR */
#define MAXISN_ERR          422   /* Number of stimuli exceeds 2**15 */
#define GCONNSZ_ERR         423   /* GCONN area exceeds layer area */
#define REPLAY_EOF_ERR      424   /* Premature EOF on replay file */
#define VMAMPL_ERR          425   /* Value-modulated amp bounds */
#define OVERFLOW_ERR        426   /* Any kind of overflow error */
#define PREPROC_ERR         427   /* User image preprocessor error */
#define GFSVRL_ERR          428   /* Simdata reclen > 32 bits */
#define VIDEO_ERR           429   /* Video driver error */
#define TV_SCALE_ERR        430   /* Image too big for scaling, etc. */
#define PXQQ_ERR            431   /* Got PXQQ when not online */
#define PAUSEMSG_ERR        432   /* Error in pause message */
                        /*  433-435  Obsolete VDR errors */
#define BADCELL_ERR         436   /* Requested cell does not exist */
                        /*  437      Obsolete bad pointer (d3tsvp) */
#define RESITM_ERR          438   /* Req'd item not in RESTORE file */
#define RESTYP_ERR          439   /* Type mismatch in RESTORE file */
#define IJPLBOX_ERR         440   /* ijplot source box bad kgen--
                                  *  reported at setup, not d3exit */
#define UPROG_ERR           441   /* User programs not supported */
#define BBD_ERR             442   /* BBD error */
#define RUNNO_ERR           443   /* Error involving runno file */
#define IMTOOSM_ERR         444   /* Image too small for preproc */
#define VFILE_ERR           445   /* Error involving version file */
#define NREAD_ERR           447   /* Node-node read error */
#define NWRITE_ERR          448   /* Node-node write error */
#define PREV_GREX_ERR       449   /* Previous d3grex error */
#define QUICKEXIT_RQST      450   /* Quick exit requested */

#define FIRST_DEVERR        480   /* Errors above this number are
                                  *  development errors--give message
                                  *  for user to contact the author.  */
#define NYIMPL_ERR          481   /* Named option not yet implemented */
#define GETTHR_ERR          482   /* Bad format string to getthr() */
#define VALDAT_ERR          484   /* Value data array is missing */
#define DPLLEN_ERR          485   /* Detail print line too long */
#define LIJBND_ERR          486   /* Lij out of bounds entering lb2b  */
#define COLOR_ERR           487   /* Bad color selector encountered */
#define CONDCT_ERR          488   /* Conductance refd nonexistent ict */
#define INITSI_ERR          491   /* Tried to init > mxsdelay si's */
#define LSRSTR_ERR          492   /* Error saving lsrstr in d3allo */
#define IJPLEND_ERR         493   /* Unexpected end of IJPLOT data */
#define IONREG_ERR          494   /* ION on CELLTYPE has no REGION */
#define LIJGEN_ERR          495   /* Unsupported Lij gen opts */
#define DPSYNC_ERR          496   /* Detail print out of synch */
#define CSELJSYN_ERR        497   /* Negative jsyn in PAR0 cmpltcsel */
                        /*  499      Obsolete library exit (now 450) */
