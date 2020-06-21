/* (c) Copyright 1991-2017, The Rockefeller University *21116* */
/* $Id: misc.h 77 2018-03-15 21:08:14Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                          MISC Header File                            *
*         This file contains assorted structures used in CNS           *
*                                                                      *
************************************************************************
*  Revisions:  JWT, GNR - Original version                             *
*  V5C, 11/13/91, GNR - SAVBLK removed to savblk.h, ECLREC to lijarg.h *
*  V5E, 06/06/92, GNR - Add KPL definitions for d3plot control         *
*  V7C, 08/29/94, GNR - Add DECAYDEF struct (Rev. 2/01/95, 4/11/96)    *
*  V8A, 06/14/95, GNR - Add PHASEDEF struct, move GRPDEF from rpdef.h  *
*  Rev, 01/31/97, GNR - Add PCFN codes                                 *
*  Rev, 10/19/97, GNR - Add SFHMATCH struct                            *
*  Rev, 01/12/98, GNR - Remove pers from DECAYDEF struct               *
*  V8B, 12/09/00, GNR - New memory management routines, trial timers   *
*  V8D, 03/15/07, GNR - Add VALEVENT structure                         *
*  Rev, 10/28/07, GNR - Add CTDFLT structure, eliminate PHASEDEF       *
*  Rev, 12/28/07, GNR - Add RPDFLT structure                           *
*  ==>, 12/29/07, GNR - Last mod before committing to svn repository   *
*  Rev, 03/22/08, GNR - Add CNDFLT structure from conntype.h, ksipl    *
*  Rev, 05/03/08, GNR - CNOPT=K and COMPAT=K for knee response         *
*  V8E, 01/12/09, GNR - Add IZHIDEF struct for Izhikevich cells        *
*  Rev, 02/07/09, GNR - Add new celltype color specs, probe ramps      *
*  Rev, 02/11/09, GNR - Move phase options to PHASEDEF block           *
*  Rev, 07/01/09, GNR - Add IZHIDEF uv3lo, vur, PRBDEF cyc1            *
*  Rev, 08/30/09, GNR - Add BREGDEF struct for Brette-Gerstner cells   *
*  V8F, 04/13/10, GNR - Move RPDFLT, CTDFLT, CNDFLT to respective hdrs *
*  V8H, 05/22/11, GNR - Move KAMAP to ppfopt PPFABR, upsp,taup to si32 *
*  Rev, 08/28/12, GNR - Add upspsi, zetapsi, change omega1 to L2S15    *
*  V8I, 09/09/12, GNR - Add ALPHA and DOUBLE EXPONENTIAL decay types   *
*  Rev, 08/22/14, GNR - Si only EXP decay, remove DECAYDEF from DCYDFLT*
*  R63, 10/28/15, GNR - Make hlfeff default 128 mV for mV scale        *
*  R66, 02/27/16, GNR - Longs to si32 in PRBDEF                        *
*  R72, 03/10/17, GNR - Add cploc, clloc, R74 expand EXTRCOLR          *
*  R77, 12/28/17, GNR - Redo DEFTHR to handle global->local properly   *
***********************************************************************/


/*---------------------------------------------------------------------*
*             BREGDEF (Brette-Gerstner aEIF model parameters)          *
*---------------------------------------------------------------------*/

   struct BREGDEF {
      ui32  gL;               /* Leak conductance (nS S20) */
      /* Some code assumes vPeak,vReset,vT,delT are adjacent in mem */
      si32  vPeak;            /* Peak voltage (mV S20/27) */
      si32  vReset;           /* Reset voltage (mV S20/27) */
      si32  vT;               /* Thresh potential (mV S20/27) */
      si32  delT;             /* Slope factor (mV S20/27) */
      ui32  tauW;             /* Adaptation time const (ms S20) */
      si32  bga;              /* Subthresh adaptation (nS S24) */
      si32  bgb;              /* Spike adaptation (nA S20/27) */
      /* Derived values */
      si32  ugLoCm;           /* gL/Cm (S28) */
      si32  ugLdToCm;         /* gL*delT/Cm (mV S20/27) */
      si32  u1oln2dT;         /* (1/ln2)/delT (recip mV S28) */
      ui32  u1otW;            /* 1/tauW (S28) */
      si32  ubga;             /* bga/Cm (S28) */
      si32  ubgb;             /* bgb/Cm (S20/27) */
      si32  uspthr;           /* Spike threshold on (V-vT) = delT *
                              *  log2(dS10/(gL*delT/Cm)) (S20/27) */
      ui16  iovec;            /* Offset to overflow count */
      };

/*---------------------------------------------------------------------*
*                   CPLOC (Cell plot location info)                    *
*  N.B.  In regions with no groups (IA and VG) the cdx,cdy factors are *
*  stored and used so that cdx,cdy are always the ijplot box edges.    *
*  This means grid plot must use gdx,gdy for real cells, else cdx,cdy. *
*---------------------------------------------------------------------*/

   struct CPLOC {
      float xlo;              /* Constant term for x location */
      float gdx;              /* x group multiplier */
      float cdx;              /* x cell-in-its-group multiplier */
      float yhi;              /* Constant term for y location */
      float gdy;              /* y group multiplier */
      float cdy;              /* y cell-in-its-group multiplier */
      };
typedef struct CPLOC cploc;

/*---------------------------------------------------------------------*
*                     DECAYDEF (Decay parameters)                      *
*  N.B.  kdcy == 0, not omega == 0, is standard test for no decay.     *
*  "No decay" really means "no persistence", i.e. total decay.         *
*  Use of union du1 implies that when kdcy is changed, then changed    *
*  back to original type, numeric parameters must be reentered, but    *
*  why would anyone do that anyway?                                    *
*  Rev, 10/05/16, GNR - Place DCDu1 union after kdcy for ucvt routine  *
*---------------------------------------------------------------------*/

   struct DECAYDEF {
      si32 *peipv;            /* Ptr to excit/inhib previous (S20) */
      si32 omega;             /* Decay persistence parameter (S30) */
      ui16 iovec;             /* Overflow event code */
      byte kdcy;              /* Kind of decay        ndal: */
#define NODECAY      0           /* No decay (must = 0) 0   */
#define DCYEXP       1           /* Exponential decay   1   */
#define DCYLIM       2           /* Limiting decay      1   */
#define DCYSAT       3           /* Saturating decay    1   */
#define DCYALPH      4           /* Alpha function      2   */
#define DCYDBLE      5           /* Double exponential  2   */
      byte dcyopt;            /* Decay options and controls */
#define DCY_CHGOK    1           /* Parm changes OK (must = 1) */
#define DCY_NOKDE 0x40           /* Group III no kdcy entered */
#define DCY_IOVAS 0x80           /* iovec already assigned */
#define NDCM         2           /* Number of decay change modes */
      union DCDu1 {
         struct {             /* Items for DOUBLE-EXPONENTIAL */
            si32 omeg2;          /* exp(-b2*t) (S30) */
            si32 w1ovdw;         /* omeg1/(omeg1 - omeg2) (S24) */
            } ddb;
         struct {             /* Items for SATURATING */
            float hlfeff;        /* Half-effectiveness */
            float hesam;         /* Half-eff. sech arg multiplier =
                                 *  asech(0.5)/((S20/27)*hlfeff) */
#define DCYSDFLT 1.255948922E-6  /* hesam dflt = sechinv(0.5)/(2**20) */
#define DCYSMXIT 5               /* Max iterations for fast start */
#define DCYSTOL  0.05            /* Iteration tolerance (mV) */
            } dsa;
         struct {             /* Items for ALPHA */
            si32 mlnw;           /* -ln(omega) (S27) */
            } dal;
         } du1;
      byte ndal;              /* Number allocations per variable */
#define Odg          0           /* Offset to g(t) */
#define Odh          1           /* Offset to h(t) */
#define MXNDAL       2           /* Max value ndal can have */
      byte ndpsp;             /* ndal * npsps */
      };

   struct DCYDFLT {
      si32  epsilon;          /* Decay of Vm if integrating (FBod) */
      si32  omega;            /* Decay persistence parameter (FBod) */
      si32  omegau;           /* Omega used=(1-1/dt) if Euler (FBod) */
      si32  siet;             /* Threshold for self-input (mV S20) */
      si32  siscl;            /* Scale factor for self-input (S24) */
      si32  upspsi;           /* Growth factor for psi (S22) */
      ui16  omega1;           /* Persistence (reset type 2) (S15) */
      ui16  qdamp;            /* Damping factor for QBAR (S15) */
      ui16  sdamp;            /* Damping factor for SBAR (S15) */
      ui16  zetapsi;          /* Decay factor for psi (S15) */
      };

#ifndef PARn
/*---------------------------------------------------------------------*
*              DEFTHR (Deferred Threshold or Scale Data)               *
*  (There should be an entry here for every variable that is input via *
*  a KWJ_THR call to getthr() that can be read before srctyp is known. *
*  These exist only in rpdef0 and are used to store variables such as  *
*  scales or thresholds (e.g. et, mtj, etc.) if entered before their   *
*  binary scale is known (which may be srctyp dependent).  These get   *
*  copied from global set to local set when a new CELLTYPE is entered  *
*  and then to CNDFLT, PPFDATA etc. when connection type is entered    *
*  and scales are known.  This complexity is necessary to implement    *
*  documented hand-me-down from defaults to MASTER to before-CONNTYPE  *
*  to after-CONNTYPE PARAMS etc. control cards. Hopefully the struc-   *
*  ture created here will make it easy to expand this to other param   *
*  types.                                                              *
*                                                                      *
*  N.B.  Present design allows no more than 26 variables in any one    *
*  union because they are identified by letter codes in getthr calls.  *
*---------------------------------------------------------------------*/

   struct DEFTHR {
      union {                 /* scl-type scales */
         si64 ascl[1];           /* Array of possible scales */
         struct {
            si64  scl;           /* 'A' Aij to Aff scale (S20/24) */
            } s;
         } us;
      union {                 /* threshold-type mV scales */
         si16 amVthr[1];         /* Array of possible thresholds */
         struct {
            si16  et,etlo;       /* 'A,B' Sj thresholds (S7) */
            si16  mnsj;          /* 'C' Min Sj in amplif (S7) */
            si16  mtj;           /* 'D' Amplif thresh on Sj (S7) */
            si16  sjrev;         /* 'E' Sj reversal potential (S7) */
            si16  ppft;          /* 'F' PPF threshold (S7) */
            si16  emxsj;         /* 'G' Max Sj in Aij (S7) */
            } t;
         } umVt;
      union {                 /* threshold-type frac scales */
         si16 afrthr[1];         /* Array of possible thresholds */
         struct {
            si16  et,etlo;       /* 'A,B' Sj thresholds (S14) */
            si16  mnsj;          /* 'C' Min Sj in amplif (S14) */
            si16  mtj;           /* 'D' Amplif thresh on Sj (S14) */
            si16  sjrev;         /* 'E' Sj reversal potential (S14) */
            si16  ppft;          /* 'F' PPF threshold (S14) */
            si16  emxsj;         /* 'G' Max Sj in Aij (S14) */
            } t;
         } ufrt;
      };
#endif

/*---------------------------------------------------------------------*
*                              DEPRDFLT                                *
*  Depression defaults that may be entered on MASTER DEPRESSION cards  *
*---------------------------------------------------------------------*/

   struct DEPRDFLT {
      long omegad;            /* Persistence for depression (S30) */
      long upsd;              /* Depression growth factor (S27/20) */
      };

/*---------------------------------------------------------------------*
*                    DPDATA (Detailed Print Data)                      *
*  (These exist only on node 0 and are only used to store data from    *
*  d3grp3() to d3news() invocations.)                                  *
*---------------------------------------------------------------------*/

#ifndef PARn
   struct DPDATA {
      struct DPDATA *pndpd;   /* Ptr to next DPDATA block */
      ui32  dpitem;           /* Codes for items to be printed */
      clloc dpclloc;          /* Number of associated cell list */
      short jdpt;             /* Timer: 0 if none entered */
      short mckpm;            /* Kind of action of last mcodes:
                              *  -1 if none entered--see RK.mckpm */
      schr  dpdis;            /* -1 no change, 0, enable, 1 disable */
      byte  dppad;            /* Pad out to full word */
      };
#endif

/*---------------------------------------------------------------------*
*               EXTRCOLR (Color extraction parameters)                 *
*  Note: tvshft, tvmask have NColorDims (3) entries corresponding to   *
*  red, green, blue in that order.  These are used to extract inputs   *
*  from BM_C8 and BM_C16 colored images for color-sensitive connec-    *
*  tions (not used with grayscale and BM_C24 or BM_C48 images).        *
*  tveoff, tvioff are the offsets in these color arrays to the first   *
*  or only excitatory, inhibitory colors.  tvrndc is added to raw      *
*  pixels for rounding when averaging to get gray from colors, with    *
*  an added amount if aggressive rounding is requested (interaction    *
*  with tviflgs operations has not been studied).  tv3cinc controls    *
*  color rotation if EXTR3C is set.  tvlpxgc gives size of a pixel in  *
*  bytes (1 or 2 if gray, 3 or 6 if colored).  gvin_fn defines a func- *
*  tion type that uses an EXTRCOLR to extract colored input data from  *
*  the IA or an image and d3gvin is a function to select which gvin_fn *
*  to use.                                                             *
*---------------------------------------------------------------------*/

   struct EXTRCOLR {
      ui16  tv3cinc;             /* 1 if EXTR3C is set, otherwise 0 */
      ui16  tvlpxgc;             /* Byte length of a pixel possibly
                                 *  after gray-from-color calc. */
      ui16  tvrndc;              /* Rounding constant for C8->gray */
      byte  tvcsel;              /* Color selector (enum ColorSens) */
/* Other bits left of ColorSens in tvcsel */
#define EXTR3C   0x80         /* Extract 3 colors alternately */
#define EXGfmC   0x40         /* Extract gray from color */
#define EXMASK   0x0f         /* Mask to extract ColorSens */
      byte  tvmode;              /* Image color mode (BM_ modes) */
      byte  tveoff;              /* Offset to first excitatory color */
      byte  tvioff;              /* Offset to first inhibitory color */
      byte  tvshft[NColorDims];  /* Shifts for three colors */
      byte  tvmask[NColorDims];  /* Masks for three colors (BM_C8) */
      };

   typedef si32 (*gvin_fn)(byte *, struct EXTRCOLR *);
   gvin_fn d3gvin(int ksrc, struct EXTRCOLR *pxc);

/*---------------------------------------------------------------------*
*         FDHEADER (Feature detector header table structure)           *
*---------------------------------------------------------------------*/

   struct FDHEADER {
      short mrx;              /* X dimension of matrix */
      short mry;              /* Y dimension of matrix */
      int npat;               /* Number of patterns (matrices)*/
      fdm_type *pfdm;         /* Pointer to matrices*/
      };

/*---------------------------------------------------------------------*
*                    GRPDEF (Data from GRPNO card)                     *
*---------------------------------------------------------------------*/

   struct GRPDEF {
      struct GRPDEF *pnxgrp;  /* Ptr to next group block */
      short *grp;             /* Array of group numbers  */
      };

/*---------------------------------------------------------------------*
*                 IZHIDEF (Data from IZHIKEVICH card)                  *
*  N.B. The IZHICOM struct contains parameters common to the 2003 and  *
*  2007 models, including those used to generate individual a,b,c,d    *
*  values for each cell.  In the 2007 case, the b and d parameters     *
*  are those entered (and stored separately) divided by Cm. Some code  *
*  assumes the IZHICOM structs are the first thing in IZ03DEF,IZ07DEF. *
*---------------------------------------------------------------------*/

   struct IZHICOM {
      /* Some code assumes iza,izb,izc,izd are adjacent in memory */
      si32 iza;               /* a coefficient (S28) */
      si32 izb;               /* IZH3: b, IZH7: b/Cm (S28) */
      si32 izc;               /* c coefficient (mV S20/27) */
      si32 izd;               /* IZH3: d, IZH7: d/Cm (S22/29) */
      /* Parameters used to randomize individual a,b,c,d */
      si32 izseed;            /* Seed for randomizing a,b,c,d */
      ui16 izra;              /* Individual a randomizer (S14) */
      ui16 izrb;              /* IZH3: b, IZH7: b/Cm rand. (S14) */
      ui16 izrc;              /* Individual c randomizer (S7/14) */
      ui16 izrd;              /* IZH3: d, IZH7: d/Cm rand. (S7/14) */
      /* Some code assumes vpeak,vur are adjacent in memory */
      si32 vpeak;             /* Spike reset potential (mV S20/27) */
      si32 vur;               /* du = a*(b*(v-vur)-u) (mV S20/27) */
      ui16 iovec;             /* Offset to overflow count */
      };

   struct IZ03DEF {
      struct IZHICOM Z;       /* a,b,c,d generation parms */
      /* Some code assumes izcv2,izcv1,izcv0 are adjacent in memory */
      si32 izcv2;             /* Coeff of v squared (1/mV S30/23) */
      si32 izcv1;             /* Coeff of v (S24) */
      si32 izcv0;             /* Constant term (mV S20/27) */
      /* Derived values */
      si32 i3vr;              /* IZH3 rest potential (mV S20/27) */
      };

   struct IZ07DEF {
      struct IZHICOM Z;       /* a,b,c,d generation parms */
      /* Input values for IZHICOM subject to rescaling */
      si32 iizb;              /* b coefficient (S23) */
      si32 iizd;              /* d coefficient (nA S19/26) */
      ui16 iizrb;             /* Individual b randomizer (S10) */
      ui16 iizrd;             /* Individual d randomizer (S7/14) */
      si32 izk;               /* k in k*(v-vr)(v-vt) (S24/17) */
      si32 izvt;              /* Threshold voltage (mV S20/27) */
      si32 bvlo;              /* Value of b when v < vur (S23) */
      si32 umax;              /* Maximum value of 'u' (nA S19/26) */
      si32 umc;               /* Multiplies u to adjust c (S28) */
      si32 umvp;              /* Multiplies u to adjust vpeak (S28) */
      si32 uv3;               /* Multiplier of v^3 in u calc (S30) */
      si32 uv3lo;             /* Value of uv3 when v < vur (S30) */
      /* Derived values */
      si32 ukoCm;             /* k/Cm (S30/23) */
      si32 ubvlooCm;          /* bvlo/Cm (S28) */
      si32 uumaxoCm;          /* umax/Cm (S22/29) */
      si32 uumctCm;           /* umc*Cm (S24) */
      si32 uumvptCm;          /* umvp*Cm (S24) */
      si32 uuv3oCm;           /* uv3/Cm (S36) */
      si32 uuv3looCm;         /* uv3lo/Cm (S36) */
      si32 usmvtest;          /* Test on simvur for modifying b */
      };

#ifndef PAR0
/*---------------------------------------------------------------------*
*                               NAPSLIM                                *
*  Anatomy plot subarbor limits for CTPSB plots                        *
*---------------------------------------------------------------------*/

   struct NAPSLIM {
      float npxmn,npymn;      /* Low limits */
      float npxmx,npymx;      /* High limits */
      };
#endif

/*---------------------------------------------------------------------*
*                                NDFLT                                 *
*  Options that may be entered on NOISE or MASTER NOISE cards.         *
*  Note:  Although TYPE=S was never used, I have added a flag byte     *
*  here with a code for it with the idea that this will provide a      *
*  natural way to add any future noise types and their parameters.     *
*  (Note that NDO_GENN replaces former CTFHN flag.)                    *
*---------------------------------------------------------------------*/

   struct NDFLT {
      si32 nmn;               /* Noise mean (mV S20/27) */
      si32 nsg;               /* Noise sigma (mV S24/31) */
      si32 nfr;               /* Noise fraction (S24) */
      orns nsd;               /* Noise generating seed */
      byte noiflgs;           /* Noise flags entered */
      byte noikadd;           /* Internal flags for noise addition */
#define NFL_ADDR  0x01           /* 'R' Adds to s(i) (response) */
#define NFL_ADDV  0x02           /* 'V' Adds to Vm, not response */
#define NFL_GENN  0x03           /* 'V|S' Need to generate noise */
#define NFL_SHOT  0x04           /* 'S' Make shot noise */
#define NFL_INIT  0x08           /* 'I' Apply V noise initially to R */
      byte pad[2];
      };

/*---------------------------------------------------------------------*
*             PCF (Phase convolution function parameters)              *
*---------------------------------------------------------------------*/

   struct PCF {
      struct PCF *pnxpcf;     /* Ptr to next PCF structure */
      txtid hpcfnm;           /* Locator for name of this table */
      si32 bins[PHASE_RES];   /* Convolution kernel (S28) */
      };
/* Codes defining normalization options: */
#define PCFN_NONE    0           /* None */
#define PCFN_MAX     1           /* Maximum table value set to 1.0 */
#define PCFN_MEAN    2           /* Mean table value set to 1.0 */
#define PCFN_SUM     3           /* Sum of table values set to 1.0 */

/*---------------------------------------------------------------------*
*                  PHASEDEF (Parameters for phasing)                   *
*---------------------------------------------------------------------*/

   struct PHASEDEF {
      struct PCF *pselfpcf;   /* Ptr to self-input convolution */
      si32 pht;               /* Phasing input threshold (mV S20/27) */
      orns phiseed;           /* Seed for PHIMETH=RANDOM */
      si32 fixsi;             /* A constant s(i) (mV S20/27) */
      ui16 iovec;             /* Offset to overflow count */
      byte fixpha;            /* A constant phase */
      byte phops;             /* Miscellaneous phase options */
/* Values that may be taken on by phops: */
#define PHOPD        0x01        /* D: Discontinuous (STEP) */
#define PHOPM        0x02        /* M: Max phase bin norm to 1 */
#define PHOPC        0x10        /* Compatible mode */
      byte phimeth;           /* Method of calculating phase */
/* Values that may be taken on by phimeth: */
#define PHI_CENTROID    1        /* CENTROID */
#define PHI_RANDOM      2        /* RANDOM */
#define PHI_FIRST       3        /* FIRST */
#define PHI_MODE        4        /* MODE */
#define PHI_CONST       5        /* CONSTANT */
      byte simeth;            /* Method of calculating s(i) */
/* Values that may be taken on by simeth: */
#define SI_CENTROID     1        /* CENTROID */
#define SI_HEIGHT       2        /* HEIGHT */
#define SI_CONST        3        /* CONST (ex-SPIKE) */
#define SI_SUMPOS       4        /* SUMPOS */
#define SI_MEAN         5        /* MEAN */
      };

/*---------------------------------------------------------------------*
*      PPFDATA (Parameters for PPF (Paired Pulse Facilitation))        *
*---------------------------------------------------------------------*/

   struct PPFDATA {
      si32  upsp;             /* Growth constant for PPF (S28) */
      si32  taup;             /* Decay constant for PPF (S28) */
      short ppflim;           /* Limiting PPF (S12) */
      short ppft;             /* Threshold for PPF (mV S7/14) */
      ui16  htup;             /* Half-time for rise to mxppf (S1) */
      ui16  htdn;             /* Half-time for drop to baseline (S1) */
      byte  ppfopt;           /* Options */
#define PPFABR          1        /* Abrupt */
      };

/*---------------------------------------------------------------------*
*                   PRBDEF (Probe definition block)                    *
*  Rev, 11/07/09, GNR - Change cppnow to cppe                          *
*  R67, 11/06/16, GNR - Add pnctp so celltype can have > 1 probe       *
*---------------------------------------------------------------------*/

   struct PRBDEF {
      struct PRBDEF *pnprb;   /* Ptr to next probe globally */
      struct PRBDEF *pnctp;   /* Ptr to next celltype probe */
      struct CELLTYPE *psct;  /* Ptr to selected cell type or NULL */
      struct CLBLK *pclb;     /* Ptr to associated cell list */
      float  (*pupsprb)(long iseq, long icell, long series,
         long trial, long cycle, float sprb); /* User pgm for sprb */
      si32   pptr;            /* Probes per trial */
      si32   sprb;            /* Activation level of probe (mV S20) */
      si32   ramp;            /* Increase in sprb per cycle */
      clloc  prbclloc;        /* Cell list locator */
      ui16   cpp;             /* Cycles per probe (in each trial) */
      ui16   cyc1;            /* First cycle to apply probe */
      byte   jprbt;           /* Timer */
      byte   prbopt;          /* User probe options */
#define PRBMARK  1               /* 'V' Mark probes in rep plots */
      byte   prbdis;          /* Probe disable control */
#define PRBDUSR   1              /* Disabled by user */
#define PRBDSYS   2              /* Disabled by system */
      /* Derived quantities */
      iter   prbit;           /* Cell list iterator */
      si32   ipptr;           /* Probes per trial on this node */
      si32   lpptr;           /* Working copy of ipptr */
      si32   iprbcyc;         /* Current trial in probe cycle */
      si32   lprbcyc;         /* Length of full probe cycle */
      si32   prob1st;         /* Cell to probe first in next cycle--
                              *  During input, flags options found */
      si32   probnxt;         /* Cell to probe first in next trial */
#ifdef PAR
      si32   myipc1;          /* Value of iprbcyc where node starts */
      si32   myipt1;          /* Value of ipptr where node starts */
#endif
      si32   sprbcyc;         /* Value of sprb in current cycle */
      ui16   cyce;            /* Ending cycle (0 to skip trial,
                              *  may exceed ntca)  */
      ui16   ictp;            /* Probe number on this cell type */
      };

/*---------------------------------------------------------------------*
*                              RFRCDATA                                *
*    (Parameters for refractory period and afterhyperpolarization)     *
*---------------------------------------------------------------------*/

   struct RFRCDATA {
      si32 ahp;               /* Afterhyperpolarization (mV S20) */
      si32 omegadst;          /* Persistence for dst (S30) */
      si32 psdst;             /* Post-spike delta st (mV S20) */
      si32 upsahp;            /* Growth coefficient for ahp (S28) */
      si32 upsdst;            /* Growth factor for dst (S16) */
      unsigned short refrac;  /* Refractory period (cycles) */
      };

/*---------------------------------------------------------------------*
*          SFHMATCH (Save/restore header match information)            *
*---------------------------------------------------------------------*/

   struct SFHMATCH {
      void *path;             /* Ptr to affected type header--
                              *  CELLTYPE, CONNTYPE, INHIBBLK, MODBY */
      struct CELLTYPE *ppl;   /* Ptr to parent layer (for err msgs) */
      short nxthm;            /* Index of next SFHMATCH in list--not
                              *  a pointer because of list realloc */
      short fno;              /* File number to restore from */
      short rct;              /* Connection type to restore from:
                              *  -ict for default, +rct if explicit */
      short ict;              /* Conntype number (for err msgs) */
      rlnm  rrennm;           /* Restore rename rep-layer name */
      };
/* Codes defining types affected by SFHMATCH */
#define SFHMCELL 0               /* Cell type restore */
#define SFHMCONN 1               /* Specific conntype restore */
#define SFHMGCON 2               /* Geometric conntype restore */
#define SFHMMODU 3               /* Modulatory conntype restore */
#define SFHMNTYP 4               /* Number of header match types */

#ifndef PARn
/*---------------------------------------------------------------------*
*                 TRTIMER (Trial Timer--Node 0 Info)                   *
*---------------------------------------------------------------------*/

   struct TRTIMER {
      struct TRTIMER *pntrt;  /* Ptr to next trial timer */
      ilst *psil;             /* Ptr to stim  iteration list */
      ilst *ptil;             /* Ptr to trial iteration list */
      mdlt_type smdlts;       /* Controlling stimulus modalities */
      short jtno;             /* Number of this timer */
      byte  tmrop;            /* Timer options */
#define TRTEVON     1            /* ON for special event trials */
#define TRTEVOFF    2            /* OFF for special event trials */
#define TRTREL      4            /* Trial numbers are relative
                                 *  to last special event trial */
      stim_mask_t timsm[1];   /* Timer stimulus mask--actual size
                              *  is determined from RP->mxnds */
      };
#define TRTN0    1            /* Timer number for timer "0" */
#define TRTN1    2            /* Timer number for timer "1" */
#define TRTNA    3            /* Timer number for timer "A" */
#define PRTRT   "O"           /* Timer for PRINT compatibility */
#define PLTRT   "P"           /* Timer for PLOT compatibility */

/*---------------------------------------------------------------------*
*                 VALEVENT (Event triggered by value)                  *
*---------------------------------------------------------------------*/

   struct VALEVENT {
      struct VALEVENT *pnve;  /* Ptr to next value event */
      si32  evtrig;           /* Trigger value (S8) */
      short ivdt;             /* Value number */
      short veops;            /* Option flags */
#define VEV_GE    1              /* Greater-than */
      };
#endif
