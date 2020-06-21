/* (c) Copyright 1991-2012 Neurosciences Research Foundation, Inc. */
/* $Id: conntype.h 52 2012-06-01 19:54:05Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                        CONNTYPE Header File                          *
*                                                                      *
*  V5C, 11/12/91, GNR - Add new save/restore, D1, d3elij               *
*  V5E, 07/25/92, GNR - Add RHO, DVI, KINDB for KJF amplif             *
*  Rev, 10/11/92, GNR - Add variables for new KGEN=H option            *
*  Rev, 10/22/92, ABP,GNR - Separate CNSTAT for nxdr, rearrange unions *
*  Rev, 03/01/93, GNR - Add subarborization                            *
*  V6C, 09/17/93, GNR - DAS stats changed to double to avoid overflow  *
*  V6D, 01/29/94, GNR - Add DELAY params                               *
*  V7B, 06/22/94, GNR - Add avg Cij option, cijlenx for subarbors,     *
*                       provide eight explicit amplification scales,   *
*                       boundary conditions, self-avoidance, regen,    *
*                       bilinear Cij maps                              *
*  V7C, 12/20/94, GNR - Add axraw, vdsam, KGEN=Q                       *
*  V8A, 05/06/95, GNR - Add psmdlt, o<ex|in>as, ax[NEXIN], VDSAMULT,   *
*                       SP, nampc, delete axraw, "LTP"                 *
*  Rev, 09/28/97, GNR - Independent dynamic allocations per conntype   *
*  Rev, 12/23/97, GNR - Add ADECAY, apers, PPF, ocndist, oFdist        *
*  Rev, 11/24/00, GNR - Separate RADIUS parameter, HV in IA coords     *
*  V8B, 12/28/00, GNR - Move to new memory management routines         *
*  V8C, 06/08/03, GNR - Cell responses in millivolts, add conductances *
*  ==>, 12/28/07, GNR - Last mod before committing to svn repository   *
*  Rev, 03/22/08, GNR - Move DFLTS struct to misc.h, rename CNDFLT     *
*  Rev, 04/21/08, GNR - Add ssck (sum sign check) mechanism            *
*  Rev, 05/03/08, GNR - Restore CNOPT=D and sim for GCONN, MOD conns   *
*  V8F, 04/15/10, GNR - Move CNDFLT here from misc.h, add KGEN=2       *
*  Rev, 06/16/10, GNR - Pull cnflgs, saopt out of cnopt                *
*  V8H, 10/20/10, GNR - Add xn (once per conntype) variable allocation *
*  Rev, 02/24/11, GNR - Add KAM=C for categorization models            *
*  Rev, 04/21/11, GNR - Add phifn control, remove KAMAP                *
*  Rev, 03/30/12, GNR - Add distribution of Sj, DeltaCij statistics    *
*  Rev, 04/23/12, GNR - Add wsmxax, change mnax, mxax to si32          *
*  Rev, 05/23/12, GNR - Add CNOPT=C combine +/- Aij before mxax test   *
***********************************************************************/

#ifndef LIJTYPE               /* Eliminate undefined type warnings */
#define LIJTYPE void
#endif
#ifndef RKFILE
#define RKFILE void
#endif

/*---------------------------------------------------------------------*
*                               LMINPUT                                *
*  Parameters for KGEN=L ("lambda" or Cij gradient) option             *
*---------------------------------------------------------------------*/

struct LMINPUT {
   /* (Next 4 items must be in order given) */
   si32 c00;                  /* Cij value at x=0, y=0 (S24) */
   si32 c01;                  /* Cij value at x=0, y=1 (S24) */
   si32 c11;                  /* Cij value at x=1, y=1 (S24) */
   si32 c10;                  /* Cij value at x=1, y=0 (S24) */
   si32 csg;                  /* Sigma (S28) */
   ui16 flags;                /* Control bits */
/* Flag bits for ucij.l.flags:  Cij gradient rotation options
*  (N.B.: * ==> d3genr uses arith value of bit--do not redefine) */
#define LOPIV        0x01L    /* * Initial position vertical  (LOP3V) */
#define LOPMI        0x02L    /* * Initial map is inverted    (LOP3I) */
#define LOPPH        0x04L    /*   Initial position horizont. (LOP3H) */
#define LOPRR        0x08L    /* * Rotate to right (clockwise)(LOP3R) */
#define LOPAL        0x10L    /* * Alternate                  (LOP3A) */
#define LOPRL        0x20L    /*   Rotate to left (counterclockwise)
                                                              (LOP3L) */
#define LOPNG        0x40L    /*   Rotate at every new group  (LOP3G) */
#define LOPNC        0x80L    /*   Rotate at every new cell   (LOP3C) */
#define LOPMF       0x100L    /*   Map orientation is frozen  (LOP2F) */
#define LOPSC       0x400L    /*   Use source coords (dflt)   ( new ) */
#define LOPTC       0x800L    /*   Use target coords          ( new ) */
#define LOPXM      0x1000L    /*   Add mirror perp. to X      ( new ) */
#define LOPYM      0x2000L    /*   Add mirror perp. to Y      ( new ) */
/* Go no higher than 0x8000:  flags is an unsigned short! */
   };

/*---------------------------------------------------------------------*
*                               CNSTAT                                 *
*  Connection-type statistics other than distributions--these are      *
*  accumulated whether printed or not to avoid inner-loop tests        *
*---------------------------------------------------------------------*/

struct CNSTAT {
   si64 sdcij[2];             /* Sum pos,neg cij changes */
   long nampc;                /* Number of amplification cycles */
   long nmod[2];              /* Number of coeffs modified (+/-) */
   long cover;                /* Number of coefficient overflows */
   long cschg;                /* Number of coefficient sign changes */
   long mover;                /* Number of times Mij > mxsj */
   long mpover;               /* Number of times mxmp invoked */
   long nppfh;                /* Number of PPF hits */
   };

/*---------------------------------------------------------------------*
*                               CNDFLT                                 *
*  Connection type defaults that may be entered on MASTER cards or     *
*  normal connection control cards at celltype level                   *
*---------------------------------------------------------------------*/

struct CNDFLT {
   struct PCF *ppcf;          /* Ptr to syn. input phase conv. fn. */
   struct PCF *pamppcf;       /* Ptr to amplif. phase conv. fn. */
   struct DECAYDEF ADCY;      /* EPSP decay parameters */
   long delta;                /* Modification parameter (S28) */
   long dmn;                  /* Mean norm presynaptic delay (S20) */
   long dsg;                  /* Sigma in norm presyn. delay (S24) */
   long dseed;                /* Seed for generating synaptic delay */
   long gamma;                /* Synaptic decay (S24) */
   ui32 kam;                  /* Amp, decay control bits */
/* Flag bits for kam: */
#define  KAM2I        0x1L       /* 'I'--2-way on s(i)        (KAM3I) */
#define  KAM2J        0x2L       /* 'J'--2-way on s(j)        (KAM3J) */
#define  KAM3         0x4L       /* '3'--3-way amplif         (KAM33) */
#define  KAM4         0x8L       /* '4'--4-way amplif         (KAM34) */
#define  KAMHB       0x10L       /* 'H'--Pure Hebb rule       (KAM3H) */
/* The remaining bits in the low-order byte of 'kam' are reserved for
*  future (non-Hebbian) amplification options.  The following two masks
*  must include relevant bits from the above list.  */
/* Mask used to see if a Darwin II type amplification option is on */
#define DIIAMASK (KAMEX+KAM2I+KAM2J+KAM3+KAM4+KAMHB)
/* Mask used to check whether ANY amplification has been requested */
#define ANYAMASK (KAMEX+0xff)
/* KAMSA and KAMTS are incompatible with KAMUE or KAMDQ.  If both
*  entered, KAMTS prevails, no error.  KAMUE,KAMCG imply KAMVM.  */
#define  KAMFS      0x100L       /* 'F'--Fast start Q,R ops   ( NEW ) */
#define  KAMMF      0x200L       /* 'M'--Matrix freeze        (KAM2M) */
#define  KAMNC      0x400L       /* 'N'--No sign change       (KAM2N) */
#define  KAMBY      0x800L       /* 'B'--Bypass conntype      (KAM2B) */
#define  KAMDS     0x1000L       /* 'D'--Detailed stats       (KAM1D) */
#define  KAMEX     0x2000L       /* 'E'--Explicit amp rule    (KAM1E) */
#define  KAMWM     0x4000L       /* 'W'--Way set from Mij-mtj ( NEW ) */
#define  KAMVM     0x8000L       /* 'V'--Value modification   (KAM2V) */
#define  KAMUE    0x10000L       /* 'U'--Unusual eligibility  ( NEW ) */
#define  KAMTS    0x20000L       /* 'T'--Timed slow amp       (KAM1T) */
#define  KAMSA    0x40000L       /* 'S'--Slow amp             (KAM1S) */
#define  KAMZS    0x80000L       /* 'Z'--Use sbar, not Mij    ( NEW ) */
#define  KAMDQ   0x100000L       /* 'Q'--Dynamic mti          ( NEW ) */
#define  KAMDR   0x200000L       /* 'R'--Dynamic mtj          ( NEW ) */
#define  KAMCG   0x400000L       /* 'C'--Categorization group ( NEW ) */
#define  KAMKI  0x1000000L       /* 'K'--Konstant si = 1      ( NEW ) */
#define  KAMVNC 0x2000000L       /* (KAMVM|KAMUE) & !KAMCG            */
#define  KAMTNU 0x4000000L       /* KAMTS & !KAMUE                    */
#define  KAMVNU 0x8000000L       /* (KAMVM|KAMCG) & !KAMUE            */
   si32 mnax;                 /* Minimum afference (mV S20/27) */
   si32 mxax;                 /* Maximum afference (mV S20/27) */
   si32 rho;                  /* Baseline efficacy decay (S24) */
   si32 scl;                  /* Scale factor for Aij (S24) */
   long mxmp;                 /* Max destruction rate of Mij (S13/20) */
   long target;               /* Synaptic decay equil. value (S16) */
   long zetam;                /* Destruct constant for Mij (S16) */
   long upsm;                 /* Growth constant for Mij (S16) */
   long vdha;                 /* Voltage-dep half afferent (S20) */
#define VDHAMULT 1.5699362E-7 /* vdsam = halfaff * 2*arcsech(0.5)/S24 */
   long vdt;                  /* Voltage dependent threshold (S20) */
   ui16 cnopt;                /* Connection type options */
/* Flag bits for cnopt (low-order 4 bits are copied into phopt for
*  tests w/other bits--these bits may be modified after d3tchk(),
*  so always access in ix->phopt after that time.  */
#define PHASJ      0x0001        /* J--Assign s(j) phase to input     */
#define PHASR      0x0002        /* R--Random phasing of input        */
#define PHASU      0x0004        /* U--Uniform phasing of input       */
#define PHASC      0x0008        /* C--Assign constant phase to input */
#define NOPPHBITS  0x000F        /* Phase option bits                 */
#define NOPSQ      0x0010        /* Q--Squared inhibition     (NOP0Q) */
#define NOPSH      0x0020        /* S--Shunt inhibition       (NOP0S) */
#define NOPVP      0x0040        /* P--Volt-dep depends on Si(t-1)    */
#define NOPVC      0x0080        /* H--Hyperbolic cosine vdep         */
#define NOPVD      0x0100        /* V--Voltage-dependent input        */
#define NOPVA      0x01C0        /* Any kind of voltage-dependent     */
#define NOPOR      0x0200        /* O--Override restore/no restore    */
#define NOPKR      0x0400        /* K--Knee response (et subtracted)  */
#define NOPSJR     0x0800        /* I--Input is Sj - Rj               */
#define NOPRS      0x1000        /* D--Radar HV distance scaling      */
#define NOPCX      0x2000        /* C--Combine +/- terms before axmx  */
   short dvi;                 /* Drift value index */
   short et,etlo;             /* Effectiveness thresholds (mV S7/14) */
   short mnsi;                /* Minimum s(i) in amp (mV S7) */
   short mnsj;                /* Minimum s(j) in amp (mV S7) */
   short mti;                 /* Mod threshold on si (mV S7) */
   short mtj;                 /* Mod threshold on sj (mV S7) */
   short mtv;                 /* Mod thresh on value (old vt) (S8) */
   short mticks;              /* Alt decay time for modifying subst */
   short mxmij;               /* Maximum Mij in amp (mV S7) */
   short nbc;                 /* Bits/connection (max 16) */
   ui16  rdamp;               /* Damping factor for RBAR (S15) */
   short sjrev;               /* Sj reversal potential (mV S7/14) */
   short vi;                  /* Value index */
#define MAX_WAYSET_CODES 8    /* Size of wayset array--don't change */
   short wayset[MAX_WAYSET_CODES];  /* Explicit amp scales (S8) */
   char csc;                  /* Negative amp scale (obsolete) */
   byte kdecay;               /* Kind of Cij decay */
#define  KINDI       0x01        /* Decay 'I'--Individual mean s   */
#define  KINDB       0x02        /* Decay 'B'--Indiv. baseline Cij */
#define  KINDE       0x04        /* Decay 'E'--Exponential decay   */
   byte kdelay;               /* Kind of delay */
/* Values for kdelay
*  (DIJ is allocated if kdelay > DLY_CONST && optimize speed): */
#define DLY_NONE        0        /* No delays */
#define DLY_CONST       1        /* Constant delays */
#define DLY_UNIF        2        /* Uniform delays */
#define DLY_NORM        3        /* Normal delays */
#define DLY_USER        4        /* User-defined delays (NYI) */
   byte mxcdelay;             /* Maximum connection delay */
   byte napc;                 /* Neuroanatomy plot color options */
#define NAPC_CIJ        0        /* Color according to Cij      */
#define NAPC_SJ         1        /* Color according to Sj       */
#define NAPC_MIJ        2        /* Color according to Mij      */
#define NAPC_SUBARB     3        /* Color according to subarbor */
#define NAPC_SOLID      4        /* Solid red to indicate error */
   byte nwayrd;               /* Number way scales read */
#define NWSCLE       0x80        /* Numeric way scales entered        */
   byte phi;                  /* Fixed phase */
   byte phifn;                /* Phi function for amplification */
#define FFN_DFLT        0        /* Traditional default       */
#define FFN_UNIT        1        /* Always use 1, i.e. no phi */
#define FFN_ABSCIJ      2        /* Default with |Cij| as arg */
   byte retard;               /* Phase delay on input */
   byte saopt;                /* Subarbor options */
#define SAOPTC       0x01        /* C--Subarbor clones Cij seed       */
#define SAOPTA       0x02        /* A--Subarbor avgs Cij after amp    */
#define SAOPTL       0x04        /* L--Subarbor holds same Lij        */
#define SAOPTI       0x08        /* I--Subarbors are independent      */
#define DOSARB       0x80        /* This subarbor is in use           */
   };

/*---------------------------------------------------------------------*
*                              CONNTYPE                                *
*---------------------------------------------------------------------*/

struct CONNTYPE {
   struct CONNTYPE *pct;      /* Ptr to next CONNTYPE block */
   struct CONNTYPE *pctt;     /* Ptr to next d3go-threaded CONNTYPE */
   void *psrc;                /* Ptr to source block */
   struct MODALITY *psmdlt;   /* Ptr to source modality block */
   rd_type *psyn0;            /* Ptr to first synapse of this type */
   s_type *srcorg;            /* Ptr to source s(j) array, or, if
                              *  REP, to ps ptr array of source. */
   long *aeff;                /* Ptr to excit,inhib veff arrays (S24) */
   struct PPFDATA *PP;        /* Ptr to paired pulse--NULL if off */
   /* Ptrs to synaptic variable generating functions */
   int (*cijbr)(struct CONNTYPE *ix);  /* Cij generating function */
   ui32 (*dijbr)(struct CONNTYPE *ix); /* Dij generating function */
   int (*lijbr1)(struct CONNTYPE *ix); /* Lij gen function--first */
   int (*lijbr2)(struct CONNTYPE *ix); /* Lij subsequent */
   int (*lijbr3)(struct CONNTYPE *ix); /* Lij subarbor   */
   int (*sjbr)(struct CONNTYPE *ix);   /* Sj lookup function */
   long (*sjup)();                     /* User-written Sj function */
   int (*pjbr)(struct CONNTYPE *ix);   /* Phase lookup function */

   si64 wsmnax;               /* Wide-scaled mnax (S23) */
   si64 wsmxax;               /* Wide-scaled mxax (S23) */

   /* Statistics and ptrs to statistics */
   si64 *pdas;                /* Pointer to detailed amp stats */
#define LDR (MAX_WAYSET_CODES+1)
#define OdasCt 0                 /* Offset to case count */
#define OdasSi LDR               /* Offset to (Si-mti) stat */
#define OdasSj (2*LDR)           /* Offset to (Sj-mtj) stat */
#define OdasDC (3*LDR)           /* Offset to DeltaCij stat */
#define OdasVM (4*LDR)           /* Offset to V or Mij stat */
#define NdasRows 4               /* Rows in dflt das stats */
   struct CNSTAT CNST;        /* Other connection stats */
   ui32 nccat;                /* Number correct categorizations */

   long lct;                  /* Length of all conns of this type for
                              *  one cell--includes space for NUK     */
   long nc;                   /* Number of conns of this type */
   ui32 kgen;                 /* Kind of connection generation */
/* Flag bits for kgen (IBM names at right): */
#define KGNMC        0x01L    /* 'M' opt--Matrix Cij's        (KGN3M) */
#define KGNRC        0x02L    /* 'R' opt--Random Cij's        (KGN3R) */
#define KGNLM        0x04L    /* 'L' opt--Lambda motor map    (KGN3L) */
#define KGNUD        0x10L    /* 'U' opt--Uniform distrib.    (KGN3U) */
#define KGNND        0x20L    /* 'N' opt--Normal distribution (KGN3N) */
#define KGNGP        0x40L    /* 'G' opt--Group               (KGN3G) */
#define KGNOG        0x80L    /* 'O' opt--Other group         (KGN3O) */
#define KGNTP       0x100L    /* 'S' or 'T' opt--Topographic  (KGN2T) */
#define KGNDG       0x200L    /* 'D' opt--Diagonal            (KGN2D) */
#define KGNAJ       0x400L    /* 'A' opt--Adjacent cells      (KGN2A) */
#define KGNBL       0x800L    /* 'B' opt--Box                 (KGN2B) */
#define KGNCF      0x1000L    /* 'C' opt--Crow's foot         (KGN2C) */
#define KGNIN      0x2000L    /* 'I' opt--Independent         (KGN2I) */
#define KGNPT      0x4000L    /* 'P' opt--Partitioned         (KGN2P) */
#define KGNAN      0x8000L    /* 'Q' opt--Annulus             ( new ) */
#define KGNST     0x10000L    /* 'S' opt--Scanned topographic (KGN1S) */
#define KGNFU     0x20000L    /* 'F' opt--Floating uniform    (KGN1F) */
#define KGNXO     0x40000L    /* 'X' opt--Systematic offset   (KGN1X) */
#define KGNJN     0x80000L    /* 'J' opt--Joints              (KGN1J) */
#define KGNHV    0x100000L    /* 'W' opt--Hand vision         (KGN1W) */
#define KGNHG    0x200000L    /* 'H' opt--Hypergroup          ( new ) */
#define KGNYO    0x400000L    /* 'Y' opt--X w/sep grp update  ( new ) */
#define KGNBV    0x800000L    /* 'W' w/USRSRC--BBD vision     ( new ) */
#define KGNEX   0x1000000L    /* 'E' opt--External connlist   (KGN3E) */
#define KGNE2   0x2000000L    /* '2' opt--External Lij & Cij  ( new ) */
#define KGNSA   0x4000000L    /* 'V' opt--Self-avoidance      ( new ) */

   /* Cij generation parameters and working variables */
   union CNucij {
      struct LMINPUT l;       /* Parameters for 'L' ("lambda") option */
      struct {                /* Cij coeffs are replaced with scaled
                              *  floats at end of phs1.  This is the
                              *  structure as it is broadcast.  */
         float wc[4];            /* Cij[00,01,11,10]/(width*height) */
         si32 csg;               /* Sigma (S28) */
         si32 cldx;              /* Cij type L delta X (width) */
         si32 cldy;              /* Cij type L delta Y (height) */
         ui16 flags;             /* Cij type L control bits */
         byte clinc;             /* Cij type L angle increment */
         byte curang;            /* Cij type L current angle */
         } l2;
      struct {                /* Parameters for 'M' ("matrix") option */
         long *fdptr;            /* Feature detector matrix src ptr */
         long *fdtop;            /* Limit for fd matrix loop */
         long *fstmptr;          /* First matrix pointer */
         long mno;               /* Matrix number for type 'm' conns */
         long nks;               /* No. kinds of feat dets to skip */
         long nkt;               /* No. kinds of feat dets to use  */
         long fdincr;            /* Increment for fd matrix loop */
         ui16 oFdist;            /* Offset to F dist in ctdist */
         } m;
      struct {                /* Parameters for 'R' ("random") option */
         si32 cmn;               /* Mean coefficient (S24) */
         si32 csg;               /* Sigma (S28) */
         si32 pp;                /* Proportion positive (S16) */
         } r;
      } ucij;

   /* N.B. d3echo assumes next 2 variables are in order given */
   long cseed;                /* Coefficient seed */
   long lseed;                /* Connection seed */
   long clseed;               /* lseed for current cell */
   long phseed;               /* Phasing seed */
   long upphseed;             /* Updated phseed for next cell */
   long loff;                 /* Offset (kgen=JWX) */
   long lsg;                  /* Sigma (kgen=JNOY) (S1) */
   long nrx;                  /* Field-of-view along x (kgen=BCLM) */
   long nry;                  /* Field-of-view along y (kgen=BCLM) */
   long nsa;                  /* Size of subarbor */
   long nsax;                 /* Subarbor box size along x */
   long nux;                  /* Number used along x (kgen=STUX) */
   long nuy;                  /* Number used along y (kgen=STUX) */
   long pradi,pradj;          /* Print amplif detail i,j */
   long rseed;                /* Random-rounder seed */
   long stride;               /* Stride (kgen=AB1) */
   long xoff;                 /* X Offset (groups) (kgen=FSTW,IA) */
   long yoff;                 /* Y Offset (groups) */

   struct CNDFLT Cn;          /* Defaults that can be on MASTER card */

   ui32 radius;               /* Radius (kgen=W) (S16) */
   ui32 svitms;               /* Save items rqst: codes in simdata.h */
   ui32 suitms;               /* Save items used: codes in simdata.h */
   short nhx;                 /* Horiz'tl change - nrx loop (kgen=D) */
   short nhy;                 /* Horiz'tl change - nry loop (kgen=D) */
   short nvx;                 /* Vertical change - nrx loop (kgen=D) */
   short nvy;                 /* Vertical change - nry loop (kgen=D) */

   char cbc;                  /* Conntype boundary condition */
   byte kgfl;                 /* Flags related to kgen */
/*  ** The following 5 flags are defined only after d3tchk runs **    */
#define KGNTV  0x01           /* Input from TV or preprocessor        */
#define KGNIA  0x02           /* Input from input array directly      */
#define KGNXR  0x04           /* Input is not from a repertoire       */
#define KGNVG  0x08           /* Input from virtual groups, not TV    */
#define KGNBP  0x10           /* Now bypassing this conntype          */

/* Work areas and derived quantities */

   long bigc16;               /* Max positive coeff (S16) */
   long cmsk;                 /* Coefficient mask (S31) */
   unsigned long iawxsz;      /* Size of source (Rep, IA or window) */
   unsigned long iawysz;      /*    for bounds checking */
   long iaxoff;               /* xoff shifted for IA packed coords */
   unsigned long lijlim;      /* Value of Lij skip signal */
   long mdltmoff;             /* Bit offset of modality in XRM--
                              *     not used if source is a rep. */
   long osrc;                 /* Offset to virtual cells (VG) or
                              *  window (IA) in pbcst array */
   long phitab[32];           /* Table of delta*phi(c) (S28) */
   long srcngx;               /* Source ngx (valid for all cases) */
   long srcngy;               /* Source ngy */
   long srcnel;               /* Source nel */
   long srcnelt;              /* Source nelt */
   long upcij;                /* Updates per Cij per cycle */
   long uplij;                /* Updates per Lij per cell */
   long wcseed;               /* Working Cij seed */
   long wdseed;               /* Working Dij seed */

   union CNcnwk {             /* Shared space for individual pgms */
      /* No data are passed to comp nodes, so no swapping is done */
      struct {                /* Items used in d3grp2 */
         struct SFHMATCH sfhm;   /* Save file header match info */
         long mastseed;          /* Master seed for d3echo */
         } tree;
      struct {                /* Items used in d3fchk-d3rstr */
         RKFILE *fdes;           /* File descriptor for restore */
         long offprd;            /* Offset of prd in restore file */
         long rflel;             /* Restore file element length */
         short klcd;             /* TRUE to restore "lc" data */
         } phs1;
      struct {                /* Items used in d3rplt-d3lplt */
         float scdx,scdy;        /* Source-rep data cell delta X,Y */
         float sgdx,sgdy;        /* Source-rep data group delta X,Y */
         float sxlo,syhi;        /* Source-rep 1st plot coords X,Y */
         ui32 rppc;              /* Rows per plot column if PP_SRC */
         char knapc0;            /* Neuroanatomy plot color option */
         char knapns;            /* TRUE if no source for anat plot */
         } rplt;
      /* The following structure is used only in d3go.
      *  WARNING: d3rplt may be called from within d3go once per
      *  cycle--only items that are reset each cycle can go here! */
      struct {                /* Items used in d3go */
         long *paeff;            /* Ptr to decayed aeff */
         long mntsmxmp;          /* = min(zetam*mxsj,mxmp) (S20) */
         long psvrseed;          /* Saved rseed for udevskip */
         si32 vrho;              /* Value times rho (S24) */
         ui16 rdampc;            /* 1.0 (S15) - rdamp */
         si16 vmvt;              /* Value minus mtv (S8) */
         byte kamv;              /* Amplification case--value term */
#define KAC_INELG 8                 /* Connection is ineligible */
#define KAC_VLO   4                 /* V < mtv */
#define KAC_SILO  2                 /* Si < mti */
#define KAC_SJLO  1                 /* Sj or Mij < mtj--must be 1 */
         byte sms;               /* Scaling factor Hebb term */
         } go;
      struct {                /* Items used in d3resetn */
         struct CONNTYPE *pnxr;  /* Ptr to next CONNTYPE to reset */
         int  ncnvar;            /* Number conn vars to reset */
         } reset;
      struct {                /* Items used in d3stat */
         float fcskip;           /* Number of connections skipped */
         float fnppfh;           /* Normalized number of PPF hits */
         } stat;
      } cnwk;

   union CNul1 {           /* ul1--shared space for first Lij gen */
      struct {             /* --'G,H,J,N,O,U' Options-- */
         long wgrpad;         /* Working source group offset */
         long wgrptp;         /* Next wgrpad = wgrpad + srcnel1 */
         long srcnel1;        /* G,O,U: = nel in accessible part
                              *           of source layer
                              *  H:     = nel of source row
                              *  J:     = source row offset */
         long srcnel2;        /* G:     = 2*srcnel
                              *  H:     = 2*srcnel*ngridx*ngridy
                              *  O:     = 2*(srcnelt-srcnel)
                              *  U:     = 2*srcnelt */
         long gridrow;        /* H:     = srcnel*ngridx */
         long ogoff;          /* J:     = initial offset
                              *  O: Offset to other group's conn data */
         } g;
      struct {             /* --'F,S,T' (Topographic) Options-- */
         long fstngx;         /* Copy of our own ngx */
         long fstngy;         /* Copy of our own ngy */
         long sfx;            /* Skip factor along x */
         long sfy;            /* Skip factor along y */
         long fstsvx;         /* Saved x coord for 1:1 maps */
         long fstsvy;         /* Saved y coord for 1:1 maps */
         char fstnew;         /* TRUE to calculate new Lij  */
         char pad[3];
         } s;
      struct {             /* --'W' (Hand Vision) Option-- */
         float xwa;           /* Window-arm x distance */
         float ywa;           /* Window-arm y distance */
         float xwmx,ywmx;     /* xwa max, ywa max for NOPRS */
         float xrat;          /* x ratio = (nsx or wwd)/ngx */
         float yrat;          /* y ratio = (nsy or wht)/ngy */
         float radm2;         /* 1.0/(hand radius squared) */
         long sj;             /* Calculated sj (S7/14) */
         } w;
      struct {             /* --'X,Y' (Systematic) Options-- */
         long fstoff;         /* First offset on node */
         long grpinc;         /* New-group increment (KGNYO) */
         long incoff;         /* Increment added to offset */
         long cwrkoff;        /* Working offset for current cell */
         long gwrkoff;        /* Working offset for current grp */
         } x;
      } ul1;

   union CNul2 {           /* ul2--shared space for subseq. Lij gen */
      struct {             /* --'A' (Adjacent) Option-- */
         float bvv1;          /* vcmin (KGNBV only) */
         float bvxdv;         /* nux/(vcmax - vcmin) (KGNBV only) */
         float bvydv;         /* nuy/(vcmax - vcmin) (KGNBV only) */
         long incr;           /* Increment for adjacent cells  */
         long lim;            /* Source limit for adjacent cells */
         } a;
      struct {             /* --'B' (Box) Option-- */
         long rrlen;          /* = ngx (VG), nel*ngx(REP) */
         long rslen;          /* = stride*nrx */
         long rtlen;          /* = -nry*ngx (VG), -nry*nel*ngx (REP) */
         } b;
      struct {             /* --'D' (Diagonal) Option-- */
         long xincr;          /* = 2*nsy*nhx + nvx (IA only) */
         long yincr;          /* = 2*nsy*nhy + nvy (IA only) */
         long rowsz;          /* Row size = srcngx*srcnel */
         } d;
      struct {             /* --'P' (Partitioned) Option-- */
         long srcsiz;         /* Size selected from */
         long iaxmsk;         /* (nsx-1) (IA only) */
         } p;
      struct {             /* --'Q' (SQuare Annulus) Option-- */
         long qatot;          /* Area of total annulus */
         long qhalr;          /* Half-area of left-right blocks */
         long qhnrx;          /* = nrx/2 */
         long qhnry;          /* = nry/2 */
         long qhnhy;          /* = nhy/2 */
         long qsrcnel2;       /* = 2*srcnel (REP), ky+1 (IA), 0 (VG) */
         } q;
      } ul2;

   si32 sclu;                 /* Used scl after autoscale */
   short cnoff[CNNDV];        /* Offsets to dynamic variables-- */
   short xnoff[XNNDV];        /*    subscripts defined in d3global  */
   struct EXTRCOLR cnxc;      /* Parms for extracting color      */
   ui16 cnflgs;               /* Connection type internal flags */
#define NEEDLIJ    0x0001        /* Lij needed for Cij calculation    */
#define CLIJ       0x0002        /* Calculate Lij in d3go             */
#define DOCEI      0x0004        /* NOPCX is valid -- EX,IN exist     */
#define SCOND      0x0008        /* Pass to cell via conductance      */
#define NSVAK      0x0020        /* Save Ak for SIMDATA               */
#define DIMDLT     0x0040        /* Input is direct from a modality   */
#define NOMDLT     0x0080        /* No modality                       */
#define OVMDLT     0x0100        /* Modality override by user         */
#define STM11      0x0200        /* Topographic 1:1 map from IA       */
#define GETLIJ     0x0400        /* d3lijx can set lijbr to getlij    */
#define CNDOFS     0x0800        /* Time to do fast start rbar        */
#define DOFDIS     0x1000        /* Doing F distribution              */
#define SKPNAP     0x2000        /* Skip neuroanatomy plot            */
#define UPRSD      0x4000        /* Update of rseed is needed         */
   si16 effet,effnt;          /* Effective et-1, etlo+1          */
   si16 subet,subnt;          /* Subtracted thresholds if NOPKR  */
   ui16 ict;                  /* Number of this connection type  */
   ui16 lc;                   /* Length of 1 conn of this type   */
   ui16 ldas;                 /* Byte length of detailed amp stats */
   ui16 lxn;                  /* Length of xnoff data for one cell */
   ui16 nsarb;                /* Number of subarbors (default 1) */
   ui16 ocnddst;              /* Offset to dists in ctddst array */
#define OaCd 0                   /* Offset to Cij dist from ocnddst */
#define OaSd 1                   /* Offset to Sj dist from ocnddst */
#define OaVd 2                   /* Offset to Aj dist from ocnddst,
                                 *  then W, Rbar if used */
#define NCNDDST 3             /* Number of Oa.d distributions */
   ui16 oex64;                /* Offset to raw 64-bit affs data */
   ui16 oexafs;               /* Offset to excitatory affs data */
   ui16 oinafs;               /* Offset to inhibitory affs data */
   ui16 oasei[NEXIN];         /* Offsets to excit/inhib AffSums */
   rlnm srcnm;                /* Source rep-layer name */
   char ampbi;                /* Amplification switch */
/* Values for ampbi and amp_switch: */
#define NO_AMP     0             /* Assumed outside d3go w/o def. */
#define HEBBAMP    1
#define SLOWAMP    2
#define TICKAMP    3
#define ELJTAMP    4             /* Eligibility trace amplif */
#define LEIFAMP    5             /* Name for any unimplemented amp */
   char amptt;                /* Amp type (if traditional D3)--
*  Values of amptt are used to index amp_case_scales in d3go()
*  and must reflect the order of the values stored there.  */
   byte cijlen;               /* Length of Cij in bytes */
   byte cnsrctyp;             /* Type of connection source as
*  defined in d3global.h (0 = rep), but for KGNHV it is USRSRC,
*  not VH_SRC, if input is from an external sense.  */
   byte cssck;                /* Connection sum sign check */
   char dcybi;                /* Decay switch */
/* Values for dcybi decay_switch: */
#define NO_DECAY   0             /* Do not perform decay        */
#define DECAYE     1             /* Simple exponential decay    */
#define DECAYB     2             /* Decay to baseline Cij       */
#define DECAYI     3             /* Decay modified by mean s(i) */
#define SKIP_ALL   4             /* Skip over decay, amplification
                                 *        and statistics. */
   char finbi;                /* Finalization switch -- generic */
   char finbnow;              /* Finalization for current cycle */
/* Values for finbi and finbnow: */
#define NO_FIN_AMP 0
#define NULLAMP    1
#define NORMAMP    2
#define ELJTFIN    3
   byte lpax;                 /* Length of dprint data exc. pers */
   byte lpax64;               /* Length of 64-bit part of dprnt data */
   byte lrax;                 /* Length of raw sums arrays */
   byte lrax1;                /* Part of lrax used in synapse loop */
   byte lsax;                 /* Length of subarb/convolution arrays */
   char lijlen;               /* Length of Lij in bytes */
   char lijshft;              /* = 3 if input from value, 1 for input
                              *  with phase, else 0 */
   byte nuklen;               /* Length of stored nuk */
   byte phopt;                /* Phase options--
                              *  Low 4 bits copied from cnopt */
#define PHASECNV   0x10          /* Early convolution (nc < 32, !VDT) */
#define PHASDCNV   0x20          /* Deferred convolution */
#define PHASVDU    0x40          /* PHASU subject to volt-dep */
   byte upcij1s;              /* rseed updates per synapse */
   byte uplijss;              /* Updates per Lij (subseq conns) */
   byte armi;                 /* HD: arm number [0,n-1] */
   byte wdwi;                 /* HD or ST: window number [0,n-1] */
   };  /* End CONNTYPE definition */
