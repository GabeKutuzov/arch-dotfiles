/* (c) Copyright 1991-2018, The Rockefeller University *21115* */
/* $Id: conntype.h 78 2018-08-02 18:36:58Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                        CONNTYPE Header File                          *
*                                                                      *
************************************************************************
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
*  Rev, 06/07/12, GNR - Add adefer per conntype, snapshot Cij dist.    *
*  Rev, 06/16/12, GNR - Move KAMBY to cnopt, add BCM, P amplif. rules  *
*  Rev, 08/28/12, GNR - Add KAM=L                                      *
*  Rev, 12/24/12, GNR - New overflow checking                          *
*  Rev, 03/29/13, GNR - Add CNOPT=M (conntype yields max(Cij*Sj))      *
*  Rev, 04/23/13, GNR - Add mxsi                                       *
*  Rev, 10/25/13, GNR - Eliminate 'L' constants                        *
*  R63, 11/04/15, GNR - Move volt-dep options to vdopt, add 'A' code   *
*  R65, 01/02/16, GNR - n[gq][xyg] to ui16, ngrp to ui32, ncpg to ubig *
*  R66, 02/17/16, GNR - Change long random seeds to typedef si32 orns  *
*  R72, 02/05/17, GNR - Add Np, KGEN=K ("Kernel"), more longs to si32  *
*  R73, 04/18/17, GNR - Convert KGNAN to circular or ellipsoidal areas *
*  R74, 07/11/17, GNR - Add vdopt=N and new vdtt vdt test value, add   *
*                       LOPTW Cij generating method                    *
*  R75, 09/21/17, GNR - Move tcwid out of LMINPUT, wc[0] -> f[st]ainc  *
*  R75, 09/30/17, GNR - Add TERNARY struct and control card            *
*  R76, 10/30/17, GNR - Add vdopt=S to use s(i,t-1) for volt-dep mult. *
*                       Eliminate early phase convolution              *
*  R76, 11/06/17, GNR - Remove vdopt=C, pctt, NORM_SUMS w/cons tables  *
*  R78, 04/03/18, GNR - Add FADE-IN facility                           *
*  R78, 06/20/18, GNR - Add VDOPT=V so S means s(i) and V means Vm     *
***********************************************************************/

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
#define LOPIV       0x0001    /* * Initial position vertical  (LOP3V) */
#define LOPMI       0x0002    /* * Initial map is inverted    (LOP3I) */
#define LOPPH       0x0004    /*   Initial position horizont. (LOP3H) */
#define LOPRR       0x0008    /* * Rotate to right (clockwise)(LOP3R) */
#define LOPAL       0x0010    /* * Alternate                  (LOP3A) */
#define LOPRL       0x0020    /*   Rotate to left (counterclockwise)
                                                              (LOP3L) */
#define LOPNG       0x0040    /*   Rotate at every new group  (LOP3G) */
#define LOPNC       0x0080    /*   Rotate at every new cell   (LOP3C) */
#define LOPMF       0x0100    /*   Map orientation is frozen  (LOP2F) */
#define LOPSC       0x0400    /*   Use source coords (dflt)   ( new ) */
#define LOPTC       0x0800    /*   Use target coords          ( new ) */
#define LOPXM       0x1000    /*   Add mirror perp. to X      ( new ) */
#define LOPYM       0x2000    /*   Add mirror perp. to Y      ( new ) */
#define LOPTW       0x4000    /*   Cij from angle diff/width  ( new ) */
/* Go no higher than 0x8000:  flags is an unsigned short! */
   };

/*---------------------------------------------------------------------*
*                               TERNARY                                *
*  Parameters for TERNARY control card (input modifies Cij)            *
*                                                                      *
*  Note:  Initially this is included to implement the local modulation *
*  in the VP PNAS study, but the name is chosen to encourage future    *
*  generalization.                                                     *
*---------------------------------------------------------------------*/

struct TERNARY {
   struct CONNTYPE *qvpict;   /* Ptr to vpict conntype */
   si32 modscl;               /* Modulation scale (FBsc) */
   si32 bckscl;               /* Background scale (FBsc) */
   si32 bckgnd;               /* Background, SI32_SGN -> nmn (FBtm) */
   si32 mximage;              /* Max image, replaces GETN (FBIm) */
   si32 dennoi;               /* Denominator noise (FBtm) */
   si32 denscl;               /* Denom scale = modscl/imgmax (FBtm) */
   ui32 vpict;                /* Conntype where image comes in */
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
   long ncskip;               /* Number of connections skipped */
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
   orns dseed;                /* Seed for generating synaptic delay */
   si32 cijmine;              /* Minimum |Cij| entered (S15 + int) */
   si32 dmn;                  /* Mean norm presynaptic delay (S20) */
   si32 dsg;                  /* Sigma in norm presyn. delay (S24) */
   si32 gamma;                /* Synaptic decay (S24) */
   ui32 kam;                  /* Amp, decay control bits */
/* Flag bits for kam: */
#define  KAM2I  0x00000001       /* 'I'--2-way on s(i)        (KAM3I) */
#define  KAM2J  0x00000002       /* 'J'--2-way on s(j)        (KAM3J) */
#define  KAM3   0x00000004       /* '3'--3-way amplif         (KAM33) */
#define  KAM4   0x00000008       /* '4'--4-way amplif         (KAM34) */
#define  KAMHB  0x00000010       /* 'H'--Pure Hebb rule       (KAM3H) */
#define  KAMBCM 0x00000020       /* 'B'--Bienenstock-Cooper-Munro     */
/* The remaining bits in the low-order byte of 'kam' are reserved
*  for future (non-Hebbian) amplification options.  The following
*  mask must include relevant bits from the above list and is used
*  to check whether ANY amplification has been requested.  */
#define ANYAMASK (KAMEX+0xff)
/* KAMSA,KAMTS are incompatible with KAMUE|KAMDQ|KAMPA.  If both
*  entered, KAMTS prevails, no error.  KAMUE,KAMCG imply KAMVM.  */
#define  KAMFS  0x00000100       /* 'F'--Fast start Q,R ops   ( NEW ) */
#define  KAMMF  0x00000200       /* 'M'--Matrix freeze        (KAM2M) */
#define  KAMNC  0x00000400       /* 'N'--No sign change       (KAM2N) */
#define  KAMLP  0x00000800       /* 'L'--"Longevity" w/psi    ( NEW ) */
#define  KAMDS  0x00001000       /* 'D'--Detailed stats       (KAM1D) */
#define  KAMEX  0x00002000       /* 'E'--Explicit amp rule    (KAM1E) */
#define  KAMWM  0x00004000       /* 'W'--Way set from Mij-mtj ( NEW ) */
#define  KAMVM  0x00008000       /* 'V'--Value modification   (KAM2V) */
#define  KAMUE  0x00010000       /* 'U'--Unusual eligibility  ( NEW ) */
#define  KAMTS  0x00020000       /* 'T'--Timed slow amp       (KAM1T) */
#define  KAMSA  0x00040000       /* 'S'--Slow amp             (KAM1S) */
#define  KAMZS  0x00080000       /* 'Z'--Use sbar, not Mij    ( NEW ) */
#define  KAMDQ  0x00100000       /* 'Q'--Dynamic mti          ( NEW ) */
#define  KAMDR  0x00200000       /* 'R'--Dynamic mtj          ( NEW ) */
#define  KAMCG  0x00400000       /* 'C'--Categorization group ( NEW ) */
#define  KAMPA  0x00800000       /* 'P'--Use QBAR for mti     ( NEW ) */
#define  KAMKI  0x01000000       /* 'K'--Konstant si = 1      ( NEW ) */
#define  KAMVNC 0x02000000       /* (KAMVM|KAMUE) & !KAMCG            */
#define  KAMTNU 0x04000000       /* KAMTS & !KAMUE                    */
#define  KAMVNU 0x08000000       /* (KAMVM|KAMCG) & !KAMUE            */
#define  KAMQNB 0x10000000       /* (KAMDQ|KAMPA) & !KAMBCM           */
   si32 mdelta;               /* Modification parameter (S28) */
   si32 mnax;                 /* Minimum |afference| (mV S20/27) */
   si32 mxax;                 /* Maximum |afference| (mV S20/27) */
   si32 mximg;                /* Max image input after scale (S20/27) */
   si32 mxmp;                 /* Max destruction rate of Mij (S13|20) */
   si32 rho;                  /* Baseline efficacy decay (S24) */
   si32 scl;                  /* Scale factor for Aij (S24 except 20
                              *  w/mV scale and S14 virtual inputs */
   si32 sclmul;               /* Undocumented scl multiplier (FBsc) */
   si32 target;               /* Synaptic decay equil. value (S16) */
   /* Note:  Technically, tcwid is a network structure parameter and
   *  should be in CONNTYPE, not CNDFLT, but this allows defaulting
   *  via MASTER PARAMS or PARAMS cards.  */
   float tcwid;               /* Tuning curve width */
   si32 vdha;                 /* Voltage-dep half afferent (S20/27) */
#define VDHAMULT 1.5699362E-7 /* vdsam = halfaff * 2*arcsech(0.5)/S24 */
   si32 vdmm;                 /* Minimum volt-dep multiplier (S24) */
   si32 vdt;                  /* Voltage dependent threshold (S20) */
   si32 upsm;                 /* Growth constant for Mij (S16) */
   si32 zetam;                /* Destruct constant for Mij (S16) */
   ui16 adefer;               /* Amplification deferral cycles */
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
#define NOPNI      0x0040        /* N--Normalize image input          */
#define NOPVP      0x0080        /* P--Piech treatment of stgh inputs */
#define NOPVD      0x0100        /* V--Voltage-dependent input        */
#define NOPOR      0x0200        /* O--Override restore/no restore    */
#define NOPKR      0x0400        /* K--Knee response (et subtracted)  */
#define NOPSJR     0x0800        /* I--Input is Sj - Rj               */
#define NOPRS      0x1000        /* D--Radar HV distance scaling      */
#define NOPCX      0x2000        /* C--Combine +/- terms before axmx  */
#define NOPBY      0x4000        /* B--Bypass conntype        (KAM2B) */
#define NOPMX      0x8000        /* M--Conntype yields max(Cij*Sj)    */
   short dvi;                 /* Drift value index */
   short et,etlo;             /* Effectiveness thresholds (mV S7|S14) */
   short mnsi;                /* Minimum s(i) in amp (mV S7) */
   short mnsj;                /* Minimum s(j) in amp (mV S7|S14) */
   short mti;                 /* Mod threshold on si (mV S7) */
   short mtj;                 /* Mod threshold on sj (mV S7|S14) */
   short mtv;                 /* Mod thresh on value (old vt) (FBvl) */
   short mticks;              /* Alt decay time for modifying subst */
   short emxsj;               /* Max s(j) in Aij calc */
   short mxmij;               /* Maximum Mij in amp (mV S7|14) */
   short mxsi;                /* Maximum s(i) in amp (mV S7) */
   short nbc;                 /* Bits/connection (max 16) */
   ui16  qpull;               /* KAMDQ: QEff=Q-qpull*(mti-Q) (S15) */
   ui16  rdamp;               /* Damping factor for RBAR (S15) */
   short sjrev;               /* Sj reversal potential (mV S7|S14) */
   short vi;                  /* Value index */
#define MAX_WAYSET_CODES 8    /* Size of wayset array--don't change */
   short wayset[MAX_WAYSET_CODES+1];   /* Explicit amp scales (S10)--
                                       *  the 9th is 0 for INELG  */
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
#define NAPC_EXIN       0        /* Use scaled EXCIT|INHIB CIJ  */
#define NAPC_CIJ        1        /* Color according to Cij      */
#define NAPC_SJ         2        /* Color according to Sj       */
#define NAPC_MIJ        3        /* Color according to Mij      */
#define NAPC_SUBARB     4        /* Color according to subarbor */
#define NAPC_SOLID      5        /* Solid red to indicate error */
   byte nwayrd;               /* Number way scales read */
#define NWSCLE       0x80        /* Numeric way scales entered */
   byte phi;                  /* Fixed phase */
   byte phifn;                /* Phi function for amplification */
#define FFN_DFLT        0        /* Traditional default       */
#define FFN_UNIT        1        /* Always use 1, i.e. no phi */
#define FFN_ABSCIJ      2        /* Default with |Cij| as arg */
#define FFN_N           3        /* Number of defined phifns  */
   byte retard;               /* Phase delay on input */
   byte saopt;                /* Subarbor options */
#define SAOPTC       0x01        /* C--Subarbor clones Cij seed       */
#define SAOPTA       0x02        /* A--Subarbor avgs Cij after amp    */
#define SAOPTL       0x04        /* L--Subarbor holds same Lij        */
#define SAOPTI       0x08        /* I--Subarbors are independent      */
#define SAOPTM       0x10        /* M--Combine with max, not sum      */
#define DOSARB       0x80        /* This subarbor is in use           */
   byte vdopt;                /* Voltage-dependent connection options */
#define VDOPP        0x01        /* P--Use Persistence in multiplier  */
#define VDOPV        0x02        /* V--Use old Vm in multiplier       */
#define VDOPS        0x04        /* S--Use s(i,t-1) in multiplier     */
#define VDANY        0x07        /* Any of P or V or S                */
#define VDOIN        0x08        /* I--Use Inverse value of modifier  */
#define VDOPN        0x10        /* N--Use old test for v - vdt < 0   */
#define VDOPH        0x20        /* H--Use Hyperbolic modification    */
   };

/*---------------------------------------------------------------------*
*                              CONNTYPE                                *
*---------------------------------------------------------------------*/

struct CONNTYPE {
   struct CONNTYPE *pct;      /* Ptr to next CONNTYPE block */
   void *psrc;                /* Ptr to source block */
   struct MODALITY *psmdlt;   /* Ptr to source modality block */
   rd_type *psyn0;            /* Ptr to first synapse of this type */
   s_type *srcorg;            /* Ptr to source s(j) array, or, if
                              *  REP, to ps ptr array of source. */
   long *aeff;                /* Ptr to excit,inhib veff arrays (S24) */
   si32 *cijfrac;             /* Ptr to Cij fracs for NOPVP (S24) */
   si32 *fadein;              /* Ptr to list of fade-in scales (FBfi) */
   struct PPFDATA *PP;        /* Ptr to paired pulse--NULL if off */
   struct TERNARY *pvpfb;     /* Ptr to VP FBset option data */
   /* Ptrs to synaptic variable generating functions */
   int (*cijbr)(struct CONNTYPE *ix);  /* Cij generating function */
   ui32 (*dijbr)(struct CONNTYPE *ix); /* Dij generating function */
   int (*lijbr1)(struct CONNTYPE *ix); /* Lij gen function--first */
   int (*lijbr2)(struct CONNTYPE *ix); /* Lij subsequent */
   int (*lijbr3)(struct CONNTYPE *ix); /* Lij subarbor   */
   int (*sjbr)(struct CONNTYPE *ix);   /* Sj lookup function */
   si32 (*sjup)();                     /* User-written Sj function */
   int (*pjbr)(struct CONNTYPE *ix);   /* Phase lookup function */

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

   long lct;                  /* Length of all conns of this type for
                              *  one cell--includes space for NUK     */
   ui32 nc;                   /* Number conns of this type (max 2^28) */
   ui32 nccat;                /* Number correct categorizations */
   ui32 kgen;                 /* Kind of connection generation */
/* Flag bits for kgen (IBM names at right): */
#define KGNMC    0x0000001    /* 'M' opt--Matrix Cij's        (KGN3M) */
#define KGNRC    0x0000002    /* 'R' opt--Random Cij's        (KGN3R) */
                              /* KGNLM is turned off if KGNTW is on   */
#define KGNLM    0x0000004    /* 'L' opt--Lambda motor map    (KGN3L) */
#define KGNTW    0x0000008    /* 'L' OPT=W Tuning curve (after d3tchk)*/
#define KGNUD    0x0000010    /* 'U' opt--Uniform distrib.    (KGN3U) */
#define KGNND    0x0000020    /* 'N' opt--Normal distribution (KGN3N) */
#define KGNGP    0x0000040    /* 'G' opt--Group               (KGN3G) */
#define KGNOG    0x0000080    /* 'O' opt--Other group         (KGN3O) */
#define KGNTP    0x0000100    /* 'S' or 'T' opt--Topographic  (KGN2T) */
#define KGNG1    0x0000200    /* '1' opt--First cell, same gp ( new ) */
#define KGNAJ    0x0000400    /* 'A' opt--Adjacent cells      (KGN2A) */
#define KGNBL    0x0000800    /* 'B' opt--Box                 (KGN2B) */
#define KGNCF    0x0001000    /* 'C' opt--Crow's foot         (KGN2C) */
#define KGNIN    0x0002000    /* 'I' opt--Independent         (KGN2I) */
#define KGNPT    0x0004000    /* 'P' opt--Partitioned         (KGN2P) */
#define KGNAN    0x0008000    /* 'Q' opt--Annulus             ( new ) */
#define KGNST    0x0010000    /* 'S' opt--Scanned topographic (KGN1S) */
#define KGNFU    0x0020000    /* 'F' opt--Floating uniform    (KGN1F) */
#define KGNXO    0x0040000    /* 'X' opt--Systematic offset   (KGN1X) */
#define KGNJN    0x0080000    /* 'J' opt--Joints              (KGN1J) */
#define KGNHV    0x0100000    /* 'W' opt--Hand vision         (KGN1W) */
#define KGNHG    0x0200000    /* 'H' opt--Hypergroup          ( new ) */
#define KGNYO    0x0400000    /* 'Y' opt--X w/sep grp update  ( new ) */
#define KGNBV    0x0800000    /* 'W' w/USRSRC--BBD vision     ( new ) */
#define KGNEX    0x1000000    /* 'E' opt--External connlist   (KGN3E) */
#define KGNE2    0x2000000    /* '2' opt--External/kernel Cij ( new ) */
#define KGNSA    0x4000000    /* 'V' opt--Self-avoidance      ( new ) */
#define KGNKN    0x8000000    /* 'K' opt--Kernel              ( new ) */

   /* Cij generation parameters and working variables */
   union CNucij {
      struct LMINPUT l;       /* Parameters for 'L' ("lambda") option */
      struct {                /* d3snsa replaces Cij coeffs with scaled
                              *  floats.  struct l is invalid, l2 is the
                              *  structure as it is broadcast.  */
         float wc[4];            /* Cij[00,01,11,10]/(width*height) */
         si32 csg;               /* Sigma (S28) */
         si32 cldx;              /* Cij type L delta X (width) */
         si32 cldy;              /* Cij type L delta Y (height) */
         ui16 flags;             /* Cij type L control bits */
         byte clinc;             /* Cij type L angle increment */
         byte curang;            /* Cij type L current angle */
         } l2;
      struct {                /* Parameters for 'LW' (tuning curve) */
         float fainc;            /* Floating num tcwid in pi rads */
         float fsainc;           /* fainc / source nel */
         float ftainc;           /* fainc / target nel */
         } lw;
      struct {                /* Parameters for 'M' ("matrix") option */
         fdm_type *fdptr;        /* Feature detector matrix src ptr */
         fdm_type *fdtop;        /* Limit for fd matrix loop */
         fdm_type *fstmptr;      /* First matrix pointer */
         int mno;                /* Matrix number for type 'm' conns */
         int nks;                /* No. kinds of feat dets to skip */
         int nkt;                /* No. kinds of feat dets to use  */
         int fdincr;             /* Increment for fd matrix loop */
         int oFDMdist;           /* Offset to F dist in ctdist */
         } m;
      struct {                /* Parameters for 'R' ("random") option */
         si32 cmn;               /* Mean coefficient (S24) */
         si32 csg;               /* Sigma (S28) */
         si32 pp;                /* Proportion positive (S16) */
         } r;
      } ucij;

   struct CNDFLT Cn;          /* Defaults that can be on MASTER card */

   orns clseed;               /* lseed for current cell */
   /* N.B. d3echo assumes next 2 variables and variables paired
   *  on a single line are in order given */
   orns cseed;                /* Coefficient seed */
   orns lseed;                /* Connection seed */
   orns phseed;               /* Phasing seed for current cell */
   orns upphseed;             /* Updated phseed for next cycle */
   orns rseed;                /* Random-rounder seed */
   si32 loff;                 /* Offset (kgen=JWX) */
   si32 lsg;                  /* Sigma (kgen=JKWNOY) (S1) */
   ui32 nsa;                  /* Size of subarbor */
   ui32 nsax;                 /* Subarbor box size along x */
   si32 nux,nuy;              /* Number used along x,y (kgen=STUX) */
   ui32 hradius;              /* Radius (kgen=KW) (S16) */
   si32 stride;               /* Stride (kgen=AB1) */
   ui32 svitms;               /* Save items rqst: codes in simdata.h */
   ui32 suitms;               /* Save items used: codes in simdata.h */
   si32 xoff,yoff;            /* X,Y Offsets (groups) (kgen=FSTW,IA) */
   si32 cijmin;               /* Minimum |Cij| (S31) */
   si16 nhx,nhy;              /* Hole size (kgen=Q) */
   ui16 nrx,nry;              /* Field-of-view along x,y (kgen=BCKLM) */
   char cbc;                  /* Conntype boundary condition */
   byte fdiopt;               /* Fade-in option (enum FdiOps) */
   byte fdinum;               /* Number of scales in fadein array */ 
   byte kgfl;                 /* Flags related to kgen */
/*  ** The following 6 flags are defined only after d3tchk runs **    */
#define KGNTV  0x01           /* Input from TV or preprocessor        */
#define KGNIA  0x02           /* Input from input array directly      */
#define KGNXR  0x04           /* Input is not from a repertoire       */
#define KGNVG  0x08           /* Input from virtual groups, not TV    */
#define KGNPB  0x10           /* Input from piggybacked window        */
#define KGNSF  0x20           /* Input from steerable filters         */
#define KGNBP  0x40           /* Now bypassing this conntype          */

/* Work areas and derived quantities */

   size_t omxpix;             /* Offset to max pix in RP->pjdat */
   size_t cnosfmx;            /* Offset to NOPVP max over angle */
   long iaxoff;               /* xoff shifted for IA packed coords */
   ulng lijlim;               /* Value of Lij skip signal */
   /* N.B.  d3go adds osrc to srcorg if !(REPSRC|IA_SRC|VALSRC|VS_SRC).
   *  For VS_SRC, it is set from d3woff, added to Lij where needed. */
   ulng osrc;                 /* Offset to virtual cells (VG) or
                              *  window (IA) in pbcst array */
   long ocdat;                /* Offset to ict data in CDAT.pconnd */

   union CNcnwk {             /* Shared space for individual pgms */
      /* No data are passed to comp nodes, so no swapping is done */
      struct {                /* Items used in d3grp2 */
         struct SFHMATCH sfhm;   /* Save file header match info */
         orns mastseed;          /* Master seed for d3echo */
         } tree;
      struct {                /* Items used in d3fchk-d3rstr */
         RKFILE *fdes;           /* File descriptor for restore */
         ui64 offprd;            /* Offset of prd in restore file */
         ui64 rflel;             /* Restore file element length */
         short klcd;             /* TRUE to restore "lc" data */
         } phs1;
      /* The following structure is used only in d3go.
      *  WARNING: If routines that have other cnwk structs are called
      *  from within d3go, any overlap must be avoided.  */
      struct {                /* Items used in d3go */
         si32 *paeff;            /* Ptr to decayed aeff */
         orns psvrseed;          /* Saved rseed for udevskip */
         si32 mntsmxmp;          /* = min(zetam*mxsj,mxmp) (S20) */
         si32 vrho;              /* Value times rho (S24) */
         si16 vmvt;              /* Value minus mtv (S8) */
         byte kamv;              /* Amplification case--value term */
#define KAC_INELG 8                 /* Connection is ineligible */
#define KAC_VLO   4                 /* V < mtv */
#define KAC_SILO  2                 /* Si < mti */
#define KAC_SJLO  1                 /* Sj or Mij < mtj--must be 1 */
         } go;
      struct {                /* Items used in d3resetn */
         struct CONNTYPE *pnxr;  /* Ptr to next CONNTYPE to reset */
         int  ncnvar;            /* Number conn vars to reset */
         } reset;
      struct {                /* Items used in d3stat */
         long totcskip;          /* Number of connections skipped */
#define NCnCijStat (LDSTAT+1)    /* Number longs in Cij vecollect */
#define OCnSkipTot (LDSTAT+0)    /* Offset to number not generated */
         } stat;
      } cnwk;

   union CNul1 {           /* ul1--shared space for first Lij gen */
      struct {             /* --'G,1,H,J,N,O,U' Options-- */
         long wgrpad;         /* Working source group offset */
         long wgrptp;         /* Next wgrpad = wgrpad + srcnel1 */
         long srcnel1;        /* G,1,O,U: = nel in accessible part
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
         long fstsvx;         /* Saved x coord for 1:1 maps */
         long fstsvy;         /* Saved y coord for 1:1 maps */
         si32 sfx;            /* Skip factor along x */
         si32 sfy;            /* Skip factor along y */
         ui16 fstngx;         /* Copy of our own ngx */
         ui16 fstngy;         /* Copy of our own ngy */
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
         /* N.B.  If IA src, fstoffx, grpincx, incoffx are *= 2*nsy.
         *  Otherwise, fstoffy etc. are *= srcnel*srcngx.  */
         long fstoffx;        /* First x offset on node */
         long fstoffy;        /* First y offset on node */
         long grpincx;        /* New-group x increment (KGNYO) */
         long grpincy;        /* New-group y increment (KGNYO) */
         long incoffx;        /* Increment added to x offset */
         long incoffy;        /* Increment added to y offset */
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
      struct {             /* --'K' (Kernel) Option-- */
         kern **ptori;        /* Ptr to target orientation table */
         kern *pkern;         /* Ptr to kernel geometry info -- is ptr
                              *  to unique kernel list before d3genr */
         kern *rkern;         /* Ptr to 1st pkern row entry for current
                              *  tgt cell orientation, set in d3kiji */
         ui32 lkern;          /* Length of this kernel */
         } k;
      struct {             /* --'P' (Partitioned) Option-- */
         long srcsiz;         /* Size selected from */
         long iaxmsk;         /* (nsx-1) (IA only) */
         } p;
      struct {             /* --'Q' (Concentric annuli) Option-- */
         xysh *paxy;          /* Ptr to list of x,y pairs -- is ptr to
                              *  unique annulus list before d3genr */
         si32 lannt;          /* Length of grid table table (# xysh) */
         } q;
      } ul2;

   struct CPLOC Np;           /* Neuroanatomy plot coord params */
   si32 bigc16;               /* Max positive coeff (S16) */
   si32 cmsk;                 /* Coefficient mask (S31) */
   ui32 cnflgs;               /* Connection type internal flags */
#define NEEDLIJ    0x0001        /* Lij needed for Cij calculation    */
#define CLIJ       0x0002        /* Calculate Lij in d3go             */
#define DOCEI      0x0004        /* NOPCX is valid -- EX,IN exist     */
#define SCOND      0x0008        /* Pass to cell via conductance      */
#define CNVKR4     0x0010        /* Copy of VCELL vcflags VKR4 bit    */
#define DIMDLT     0x0040        /* Input is direct from a modality   */
#define NOMDLT     0x0080        /* No modality                       */
#define OVMDLT     0x0100        /* Modality override by user         */
#define STM11      0x0200        /* Topographic 1:1 map from IA       */
#define GETLIJ     0x0400        /* d3lijx can set lijbr to getlij    */
#define CNDOFS     0x0800        /* Time to do fast start rbar        */
#define DOFDIS     0x1000        /* Doing F distribution              */
#define SKPNAP     0x2000        /* Skip neuroanatomy plot            */
#define UPRSD      0x4000        /* Update of rseed is needed         */
#define UNIKQ      0x8000        /* Conntype has unique K or Q tables */
#define NSVAK  0x00010000        /* Save Ak for SIMDATA               */
#define NSVRAW 0x00020000        /* Save raw afference for SIMDATA    */
   ui32 iawxsz;               /* Size of source (Rep, IA, window, or */
   ui32 iawysz;               /*    VT, for bounds checking */
   ui32 mdltmoff;             /* Byte offset of modality in XRM--
                              *     not used if source is a rep. */
   ui32 nsarb;                /* Number of subarbors (default 1) */
   ui32 oPPsct;               /* Offset into STGH sin/cos table */
   si32 phitab[32];           /* Table of mdelta*phi(c) (S28) */
   ui32 rppc;                 /* Rows per plot column (PP_SRC) =
                              *  tvy*((nker+dcols-1)/dcols) */
   si32 sjcur;                /* Current value of Sj (S7/14) */
   si32 srccpr;               /* Source cells/row = nel*ngx */
   ui32 srcnel;               /* Source nel */
   ui32 srcnelt;              /* Source nelt */
   ui32 srcnimg;              /* Number shorts in preproc image */
   ui32 upcij;                /* Updates per Cij per cycle */
   ui32 uplij;                /* Updates per Lij per cell */
   orns wcseed;               /* Working Cij seed */
   orns wdseed;               /* Working Dij seed */
   si32 sclu[NEXIN];          /* Used scales after autoscale (S24) */
   short cnoff[CNNDV];        /* Offsets to dynamic variables-- */
   short xnoff[XNNDV];        /*    subscripts defined in d3global  */
   struct EXTRCOLR cnxc;      /* Parms for extracting color      */
   si16 effet,effnt;          /* Effective et-1, etlo+1          */
   si16 subet,subnt;          /* Subtracted thresholds if NOPKR  */
   ui16 ict;                  /* Number of this connection type  */
   ui16 iovec;                /* Offset to overflow counts       */
#define CNO_SUMS  0              /* Sums of afferent terms */
#define CNO_RED   1              /* Scale and reduce sums to 32-bit */
#define CNO_AMP   2              /* Addition for amplification */
#define CNO_CDCY  3              /* Addition for Cij decay */
#define CNO_VDEP  4              /* Voltage-dependent modification */
#define CNO_TER   5              /* Ternary connection calcs */
#define CNO_NOFF  6              /* Number of iovec additions */
   ui16 lc;                   /* Length of 1 conn of this type   */
   ui16 ldas;                 /* Byte length of detailed amp stats */
   ui16 lsd1n,lsd1n0;         /* Length of d3gfsv data 1 connection */
   ui16 lxn;                  /* Length of xnoff data for one cell */
   ui16 ocnddst;              /* Offset to dists in ctddst array */
#define OaSd 0                   /* Offset to Sj dist from ocnddst */
#define OaVd 1                   /* Offset to Aj dist from ocnddst,
                                 *  then V, W, Rbar if used */
#define NCNDDST 2             /* Number fixed Oa.d distributions */
   ui16 odpafs;               /* Offset to detail print affs data */
   ui16 oasei[NEXIN];         /* Offsets to excit/inhib AffSums */
   ui16 srcngx;               /* Source ngx (valid for all cases) */
   ui16 srcngy;               /* Source ngy */
   short olvty;               /* Offset to virtual group y */
   rlnm srcnm;                /* Source rep-layer name */
   char ampbi;                /* Amplification switch */
/* Values for ampbi and amp_switch: */
#define NO_AMP     0             /* Assumed outside d3go w/o def. */
#define HEBBAMP    1
#define SLOWAMP    2
#define TICKAMP    3
#define ELJTAMP    4             /* Eligibility trace amplif */
   byte cijlen;               /* Length of Cij in bytes */
   byte cnsrctyp;             /* Type of connection source as
*  defined in d3global.h (0 = rep), but for KGNHV it is USRSRC,
*  not VH_SRC, if input is from an external sense.  */
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
#define BCMAFIN    4
   byte jself;                /* 1 if self input */
   byte knapc0;               /* Neuroanatomy plot color option */
   byte knopt;                /* Options for kernel connections */
#define KNO_EXCIT  0x01          /* Connections are excitatory */
#define KNO_INHIB  0x02          /* Connections are inhibitory */
#define KNO_PRINT  0x04          /* Print the kernel */
   byte lrax;                 /* Length of raw, phased sums arrays */
   byte lrax1;                /* Length [rs]ax[pn] for PHASU|PHASC */
   byte lsax;                 /* Length of subarb, PHASCNV wkspace */
   char lijlen;               /* Length of Lij in bytes */
   char lijshft;              /* = 1 for input with phase, else 0 */
   byte npafraw;              /* Words of raw dprint data = lrax1 */
   byte npaffin;              /* Words of final dprnt data */
   byte npaftot;              /* npafraw + npaffin + ndal if decay */
   byte nuklen;               /* Length of stored nuk */
   byte phopt;                /* Phase options--If there is phasing,
                              *  d3tchk() assures that one of the low
                              *  4 cnopt bits is on and copied here.  */
#define PHOPANY    0x0f          /* Any phase option */
#define PHASCNV    0x20          /* Phase convolution */
#define PHASVDU    0x40          /* PHASU subject to volt-dep */
   byte qaffmods;             /* 1 if affs changed by vdep,mxax */
   byte sclb;                 /* Frac bits in scl scale factor */
   byte scls;                 /* Shift needed to calc or apply scl */
   byte sjbs;                 /* Sj binary scale (7 or 14) */
   byte sms;                  /* Scaling factor for Hebb term */
   byte spspov;               /* Specific PSP usage override */
   byte sssck,sssck0;         /* Connection sum sign check */
   byte upcij1s;              /* rseed updates per synapse */
   byte uplijss;              /* Updates per Lij (subseq conns) */
   byte armi;                 /* HD: arm number [0,n-1] */
   byte wdwi;                 /* HD or ST: window number [0,n-1] */
   };  /* End CONNTYPE definition */
