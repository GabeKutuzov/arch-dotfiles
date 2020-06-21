/* (c) Copyright 1991-2012, Neurosciences Research Foundation, Inc. */
/* $Id: celltype.h 52 2012-06-01 19:54:05Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                        CELLTYPE Header File                          *
*                                                                      *
*  V5C, 11/12/91, GNR - Add phimeth, simeth, new save/restore, D1      *
*  V5E, 07/20/92, GNR - Add lsp stride, lspt, KINDB, and remove mxrep  *
*  Rev, 08/25/92, GNR - Add individual cpr, cpt                        *
*  V5F, 12/08/92, GNR - Remove ctwk.rplt, add OPTSJ, OPTPJ bits        *
*  V6A, 03/26/93, GNR - Add KAMAQ (adjust MTI via sbar)                *
*  Rev, 05/20/93, GNR - Make opt unsigned, add OPTPL for pers. limit   *
*  Rev, 08/14/93, GNR - Add node1,etc. for 2**n indep. and load optim. *
*  V6D, 01/29/94, GNR - Add DELAY, spike params                        *
*  V7A, 05/01/94, GNR - Delete ps1, add NMOD delay                     *
*  V7B, 07/20/94, GNR - Add rsd, amp rule L, ptscll, htscll, regen     *
*  V8A, 04/08/95, GNR - Add MODALITY and stat cntrl at celltype level, *
*                       oas, ht to S8, CDCY, delete "LTP",nhit[pn],    *
*                       fhits[pn], revise refractory period control,   *
*                       spike stat,move group[xyn] to celldata,lijarg  *
*  Rev, 10/05/96, GNR - Add a few conntype defaults for use in d3tree  *
*  Rev, 09/28/97, GNR - Independent dynamic allocations per conntype   *
*  Rev, 01/08/98, GNR - Dynamic allocation of distribution statistics  *
*  Rev, 02/17/98, GNR - Add resting potential, afterhyperpolarization  *
*  Rev, 10/28/00, GNR - Add allocation for history statistics          *
*  V8B, 01/06/01, GNR - Add detail print controls, pnct, ctclid        *
*  V8C, 02/25/03, GNR - Cell responses in millivolts, add conductances *
*  V8D, 11/11/06, GNR - Make D,P,T,Q,K options same in rep,cell blocks *
*  Rev, 04/07/07, GNR - Remove NMOD params to a MODBY struct           *
*  ==>, 12/29/07, GNR - Last mod before committing to svn repository   *
*  Rev, 04/21/08, GNR - Add ssck (sum sign check) mechanism            *
*  V8E, 01/14/09, GNR - Add IZHIKEVICH response function, ksipcu       *
*  Rev, 02/12/09, GNR - Move phase parameters to PHASEDEF block        *
*  V8F, 04/15/10, GNR - Move CTDFLT here from misc.h, add KRPGP        *
*  V8G, 08/12/10, GNR - Add AUTOSCALE                                  *
*  Rev, 05/08/12, GNR - Add AUTOSCALE 'T' option                       *
***********************************************************************/

#ifndef CLTYPE                /* Eliminate undefined type warnings */
#define CLTYPE void
#endif
#ifndef EFFTYPE
#define EFFTYPE void
#endif
#ifndef RKFILE
#define RKFILE void
#endif

/*---------------------------------------------------------------------*
*                               CLSTAT                                 *
*   Statistical section:  The order of variables matters in the case   *
*   of the parallel version of CNS.  When forming global statistics    *
*   one needs to know which variables are summed and which a global    *
*   max should be calculated for.  Order must match COPDEFs in d3gsta. *
*   Also order of some related items is assumed in d3stat().           *
*   Note: loascl,hiascl here instead of in AUTSCL to simplify d3gsta.  *
*---------------------------------------------------------------------*/

struct CLSTAT {
   si64 sumpos;               /* Sum of positive scores (S20) */
   si64 sumneg;               /* Sum of negative scores (S20) */
   si64 sumrsp;               /* Sum of responses       (S20) */
   si64 sharp;                /* Sum of responses > ht  (S20) */
   long loscor;               /* Low value of afferent voltage (S20) */
   long hiscor;               /* High value of afferent voltage (S20) */
   long nstcap;               /* Number of statistical cycles
                              *  actually performed */
   long numpos;               /* Number of positive scores */
   long hitpos;               /* Number of positive hits (s(i) > pt) */
   long numneg;               /* Number of negative scores */
   long hitneg;               /* Number of neg hits (scores < nt) */
   long nsharp;               /* Number of responses > ht */
   long nrfrc;                /* Number of refractory cells */
   long nspike;               /* Number of spiking cells */
   long sover;                /* Number of signal overflows */
   long gmover;               /* Number of GCONN/MODUL overflows */
   long nregen;               /* Number of cells regenerated */
                              /* Next 2 vars assumed in this order */
   si32 loascl;               /* Low calculated autoscale (S24) */
   si32 hiascl;               /* High calculated autoscale (S24) */

   long sdist[LDSTAT];        /* Distribution of s(i) */
   long hdist[LDSTAT];        /* Distribution of high responses */
   } ;

/*---------------------------------------------------------------------*
*                               GPSTAT                                 *
*  Statistics collected for KRPGP option (allocated for nds stimuli).  *
*---------------------------------------------------------------------*/

struct GPSTAT {
   si32 nstim;                /* Number appearances of this stim */
   si32 nhits;                /* Number above ht */
   /* Code in d3stat assumes the next three items are adjacent */
   si32 hiresp;               /* Highest response to this stim */
   si32 lohiresp;             /* Lowest across trials of highest
                              *  response across cells */
   si32 loresp;               /* Lowest response to this stim */
   } ;

/*---------------------------------------------------------------------*
*                               AUTSCL                                 *
*  Parameters relating to the AUTOSCALE option                         *
*---------------------------------------------------------------------*/

struct AUTSCL {
   si32 ngtht;                /* Desired number of cells to fire
                              *  (-1 indicates default 0.1*nelt) */
   /* Code in d3echo assumes next three items are adjacent */
   si32 asmxd;                /* Max delta in autoscale per trial */
   si32 asmnimm;              /* Min immediate scale (S24) */
   si32 asmximm;              /* Max immediate scale (S24) */
   si32 asrsd;                /* Copy of rsd for determinism */
   ui16 adamp;                /* Damping factor for sclmul (S15) */
   ui16 kaut;                 /* Type codes */
#define KAUT_S    0x0001         /* 'S' Scale specific connections */
#define KAUT_G    0x0002         /* 'G' Scale geometric connections */
#define KAUT_M    0x0004         /* 'M' Scale modulatory connections */
#define KAUT_A    0x0008         /* 'A' Scale autaptic connections */
#define KAUT_CYC  0x0010         /* 'C' Calculate every cycle */
#define KAUT_IMM  0x0020         /* 'I' Apply result immediately */
#define KAUT_FS   0x0040         /* 'F' Fast start */
#define KAUT_ET   0x0080         /* 'T' New scale every trial */
#define KAUT_NU   0x0100         /* 'N' No update */
#define KAUT_ANY  0x00af         /* Any of S,G,M,A,I,T */
#define KAUT_FSN  0x0800         /* kautu: Fast start time is now */
#define KAUT_NOW  0x1000         /* kautu: Calc new scale now */
#define KAUT_UPD  0x2000         /* kautu: Update running avg scale */
#define KAUT_APP  0x4000         /* kautu: Apply scale immediately */
   } ;

/*---------------------------------------------------------------------*
*                               CTDFLT                                 *
*  Celltype defaults that may be entered on MASTER or REGION cards.    *
*  N.B. inform/convrt calls assume pt...ht are defined in order given. *
*---------------------------------------------------------------------*/

struct CTDFLT {
   float cpr;                 /* Default circle plot radius ratio */
   long  pt;                  /* Default celltype pt (mV S20/27) */
   long  st;                  /* Default celltype st (mV S20/27) */
   long  nt;                  /* Default celltype nt (mV S20/27) */
   long  ht;                  /* Default celltype ht (mV S20/27) */
   ui32  Cm;                  /* Membrane capacitance (pF S23)   */
   si32  sspike;              /* Default spike amplitude (mV S20/27) */
   si32  stanh;               /* Default tanh amplitude (mV S20/27) */
   ui32  taum;                /* Default membrane time const (ms S20) */
   si32  vrest;               /* Rest pot'l (for d3echo) (mV S20/27) */
   ui32  kctp;                /* Celltype-level print/plot options */
/* Flag bits for kctp,urkctp */
#define KRPSQ     0x0001L        /* 'Q' Use square for circle (KRP1Q) */
#define KRPCO     0x0002L        /* 'K' Kolored               (KRP1K) */
#define KRP3B     0x0004L        /* '3' 3 bars, Si, N, NMod   ( NEW ) */
#define CTPNS     0x0008L        /* 'U' No statistics         (OPT3U) */
#define KRPNP     0x0010L        /* 'N' Neuroanatomy plot     (KRP3N) */
#define KRPTP     0x0020L        /* 'T' Neuroanat detail list (KRP3D) */
#define KRPDR     0x0040L        /* 'D' Neuroanat drive, long (OPT3D) */
#define KRPPJ     0x0080L        /* 'P' Neuroanat drive, proj ( NEW ) */
#define KRPFD     0x0100L        /* 'F' Sdist by fd matrix    ( NEW ) */
#define KRPGP     0x0200L        /* 'G' s(mx,mn) by stim grp  ( NEW ) */
#define KRPMG     0x0400L        /* 'M' s(mx,mn) summary only ( NEW ) */
#define KRPCL     0x0800L        /* 'C' Sdist by stim ident   ( NEW ) */
#define KRPZC     0x1000L        /* 'Z' Correlation matrix    ( NEW ) */
#define KRPYM     0x2000L        /* 'Y' Matrix cross rsponses (KRP2M) */
#define KRPHR     0x4000L        /* 'H' History of responses  (KRP2H) */
#define KRPXR     0x8000L        /* 'X' Cross response stats  (KRP2X) */
#define CTPSB   0x010000L        /* 'B' Subarbor blocks       ( NEW ) */
#define CTPAR   0x020000L        /* 'A' Arrows                (OPT3A) */
#define CTPOM   0x040000L        /* 'O' Omit detail print '+' ( NEW ) */
#define KRPEP   0x080000L        /* 'E' Print every s(i)      ( NEW ) */
#define CTPLV   0x100000L        /* 'L' Old, replaced by 'D'  (OPT3D) */
/* OR of all kctp options that imply making a bubble plot */
#define KRPBPL (KRP3B|CTPLV|KRPNP|KRPTP|KRPDR|KRPPJ)
/* OR of all kctp options that imply stimulus marking in d3go */
#define KRPXRM (KRPXR|KRPYM|KRPZC|KRPHR|KRPCL|KRPGP)
   ui16  ctopt;               /* Layer-level option control bits */
#define OPTRG     0x0001         /* 'R' Regenerate            ( NEW ) */
#define OPTDR     0x0002         /* 'D' Discontinuous response(OPT1D) */
#define OPTMEM    0x0004         /* 'M' Optimize memory       ( NEW ) */
#define OPTBR     0x0008         /* 'B' Bypass reset          (OPT1B) */
#define OPTNR     0x0020         /* 'N' Noise restore         (OPT0N) */
#define OPTOR     0x0040         /* 'O' Override or no restore(OPT0O) */
#define OPTJS     0x0080         /* 'J' Just reset s(i)       ( NEW ) */
#define OPTSQ     0x0100         /* 'Q' sQuared GCONN inhib   (OPT0Q) */
#define OPTSH     0x0200         /* 'S' Shunting GCONN inhib  (OPT0S) */
#define OPTAB     0x0400         /* 'A' Abrupt refrac. period (OPT0A) */
#define OPTPH     0x0800         /* 'P' Phase requested       ( NEW ) */
#define OPTCFS    0x1000         /* 'C' New class, fast r,qbar( NEW ) */
#define OPTNIN    0x2000         /* 'I' No initial noise      ( NEW ) */
#define OPTFZ     0x4000         /* 'F' Freeze after 1 cycle  (KAMFZ) */
   short cpt;                 /* Circle plot threshold (S7/14 mV)     */
   byte  jketim;              /* kctp=E print timer */
   byte  rspmeth;             /* Response method--a RespFunc enum */
   byte  ksiple;              /* Si plot shape entered--actually an
                              *  enum SiShapes, but declared this way
                              *  so an entire int is not consumed.  */
   byte  ksipce;              /* Si plot color source */
/* Values ksipce may take on (ksipcu has enum SiColors only)
*  (d3chng needs to know what was entered) */
#define SIC_MASK    0x0f         /* Bits for an SiColors enum */
#define SIC_SOPE    0x10         /* SOLID or OPEN entered */
#define SIC_COLE    0x20         /* Color name entered */
   char  sicolor[COLMAXCH];   /* Named Si color or 0 if none */
   };
/* Values that can be taken on by ksipl (SIS_Circle is default): */
enum SiShapes { SIS_None = 0, SIS_Circle, SIS_Square, SIS_3Bar,
   SIS_2Bar, SIS_DownTR, SIS_LeftTR, SIS_UpTR, SIS_RightTR };
/* Values that can be taken on by ksipce */
enum SiColors { SIC_None = 0, SIC_Si, SIC_SBar, SIC_QBar, SIC_Phase,
   SIC_Hist, SIC_Izhu, SIC_Fixed };

/*---------------------------------------------------------------------*
*                              CELLTYPE                                *
*---------------------------------------------------------------------*/

struct CELLTYPE {
   struct CELLTYPE *pnct;     /* Ptr to next CELLTYPE globally */
   struct CELLTYPE *play;     /* Ptr to next CELLTYPE in region */
   rd_type         *prd;      /* Ptr to actual rep (cell) data */
   s_type          **pps;     /* Ptr to ptrs to s(i) lookup tables
                              *  (ptrs are indexed by delay time) */
   s_type          *ps2;      /* Ptr to second s(i) table */
   struct REPBLOCK *pback;    /* Ptr back to governing REPBLOCK */
   struct CELLTYPE *pusrc;    /* Ptr to upwith CELLTYPE */
   struct CONDUCT  *pcnd1;    /* Ptr to first conductance */
   struct IONTYPE  *pion1;    /* Ptr to first ion type */
   struct CONNTYPE *pct1;     /* Ptr to first CONNTYPE */
   struct CONNTYPE *pctt1;    /* Ptr to first d3go-ordered CONNTYPE */
   struct INHIBBLK *pib1;     /* Ptr to first INHIBBLK */
   struct MODBY    *pmby1;    /* Ptr to first MODBY */
   struct MODVAL   *pmrv1;    /* Ptr to first rep-type MODVAL */
   struct AUTSCL   *pauts;    /* Ptr to AUTOSCALE info */
   struct PHASEDEF *pctpd;    /* Ptr to PHASEDEF if phase */
   struct PRBDEF   *pctprb;   /* Ptr to probe def for this layer */
   struct RFRCDATA *prfrc;    /* Ptr to refractory period params */
   struct CLSTAT   *CTST;     /* Ptr to statistics block */
   void            *prfi;     /* Ptr to response function info:
                              *  BREGDEF, IZ03DEF, or IZ07DEF */
   CLTYPE          *dpclb;    /* Ptr to detail print cell list blk */
   ddist           *pctddst;  /* Ptr to celltype distribution stats */
   dist            *pctdist;  /* Ptr to celltype count statistics */
   hist            *phrs;     /* Ptr to history stats (1 per mdlty) */
   EFFTYPE         *pfea;     /* Ptr to first associated EFFARB */

   struct CTDFLT   Ct;        /* MASTER CELLTYPE parameters */
   struct DCYDFLT  Dc;        /* MASTER DECAY parameters */
   struct DEPRDFLT Dp;        /* MASTER DEPRESSION parameters */
   struct NDFLT    No;        /* MASTER NOISE parameters */

   long nsd;                  /* Noise generating seed */
   ui32 orkam;                /* OR of kam  bits for all conntypes */
   ui32 orkgen;               /* OR of kgen bits for all conntypes */
   si32 rsd;                  /* Random-rounding seed for decay */
   ui32 urkctp;               /* Original settings of kctp */
   ui32 svitms;               /* Save items rqst: codes in simdata.h */
   ui32 suitms;               /* Save items used: codes in simdata.h */

   short jijpl;               /* IJPlot conntype selector */
   short mncij;               /* Min Cij for N plot (S8) */
   short mxcij;               /* Max Cij for N plot (S8) */
   ui16 dpitem;               /* Detail print conn level item codes */
/* Flag bits for dpitem: */
#define DPCIJ     0x01           /* C: Print Cij */
#define DPLIJ     0x02           /* L: Print Lij */
#define DPMIJ     0x04           /* M: Print Mij */
#define DPSJ      0x08           /* S: Print Sj  */
#define DPRIJ     0x10           /* R: Print <Sj> */
#define DPCIJ0    0x20           /* B: Print baseline Cij */
#define DPDIJ     0x40           /* D: Print Dij */
#define DPPPF     0x80           /* P: Print PPF and PPFT */
#define DPALL   0x0100           /* A: Print Lij,Dij all trials */
#define DPC80   0x0200           /* N: Narrow printing (80 cols) */
#define DPDCIJ  0x0400           /* Y: Print delta Cij */
#define DPOOB   0x0800           /* O: Omit KGNST out-of-bounds */
#define DPVERB  0x1000           /* V: Verbose--print all cycles */
#define DPABCD  0x2000           /* Z: Print Izhikevich a,b,c,d */
#define DPDIDL  0x8000           /* Set when Lij have been printed */

   char lname[LSNAME];        /* Name of this cell type */

/* Work areas and derived quantities */

   union CTctwk {
      struct {                /* Items used in d3grp2-d3echo only */
         struct MODBY *pnmby;    /* Ptr to noise modulation block */
         long  mastseed;         /* Master seed for d3echo */
         rlnm  usrcnm;           /* UPWITH rep-layer name */
         byte  orkdecay;         /* OR of kdecay for all conntypes */
         byte  krmop;            /* Bits for rspmeth options, etc. */
#define KRM_RFENT 0x80              /* Response function entered */
#define KRM_SPENT 0x04              /* SPIKE value entered */
#define KRM_THENT 0x02              /* THAMP value entered */
         } tree;
      struct {                /* Items used in d3fchk-d3rstr */
         RKFILE *fdes;           /* File descriptor for restore */
         long  offsi;            /* Offset of s(i) in restore file */
         long  offprd;           /* Offset of "ls" in restore file */
         long  rflel;            /* Restore file element length */
         short rfphshft;         /* Phase shift in restore file */
         short rfdelay;          /* Delay cycles in restore file */
         char  hmwarn[SFHMNTYP]; /* Header match warning flags */
#define HMWM1 1                  /* Matched at least one header */
#define HMWWI 2                  /* Warning issued */
         } phs1;
      struct {                /* Items used in d3go */
         float fstanh;           /* = (float)stanh */
         si32  ntca;             /* Number cycles layer is active - 1 */
         ui16  qdampc;           /* = 1.0 - qdamp (S15) */
         ui16  sdampc;           /* = 1.0 - sdamp (S15) */
         mdlt_type delmk;        /* Stimulus change bits by modality  */
         char  masking;          /* TRUE if H|MXSTATS and doing stats */
         } go;
      struct {                /* Items used in d3rprt */
         s_type *icp;            /* Initial cell on page */
         } rprt;
      } ctwk;

   ui32 ctf;                  /* Miscellaneous cell type flags */
/* Values that may be taken on by ctf (low 6 bits shared): */
#define DIDSCALC  0x0001L  /* d3go: Set when s(i) calculated          */
#define DIDSTATS  0x0002L  /* d3go: Set when statistics done          */
#define OLDSILIM  0x0008L  /* d3go: Limit s(i) to COMPAT=C range      */
#define CTFSTENB  0x0010L  /* d3go: Statistics enabled in this cycle  */
#define CTFDPENB  0x0020L  /* d3go: Detail print exists & is enabled  */
#define CTFULCHK  0x0001L  /* d3tchk: Used to find update loops       */
#define CTFALIGN  0x0002L  /* d3allo: Align cell data on long boundary*/
#define RSTOLDSI  0x0004L  /* d3fchk: Restore from file with L=1 si   */
#define CTFAE     0x0010L  /* d3schk: Drives effector (excit) (OPTAE) */
#define CTFAI     0x0020L  /* d3schk: Drives effector (inhib) (OPTAI) */

#define CTDPDIS   0x0080L  /* Disable detailed print                  */
#define INCLPERS  0x0100L  /* Include persistence in NormSums         */
#define CTFASFS   0x0200L  /* Autoscale fast start time               */
#define DIDIMAW2  0x0400L  /* Did immediate amp-omega1 warning        */
#define DIDIMAW3  0x0800L  /* Did immediate amp-CDCY.omega warning    */
#define CTFI3IVR  0x1000L  /* IZH3 cells have individual Vrest in mem */
#define CTFI3IVC  0x2000L  /* IZH3 cells must calc have indiv Vrest   */
#define CTFRANOS  0x4000L  /* Indiv. Izhi coeffs and OPTIMIZE STORAGE */
#define CTFDOFS   0x8000L  /* Time to do fast start sbar              */
#define CTFPD 0x00010000L  /* Save phase distribution for SIMDATA     */
#define CTFNP 0x00020000L  /* No probes allowed on this cell type     */
#define CTFgD 0x00040000L  /* Celltype has at least one cond w/gDCY   */
#define CTFXD 0x00080000L  /* Celltype drives one with XYZHCG stats   */
#define CTFGV 0x00100000L  /* At least one inhibblk has IBOPT=V       */
#define CTFNC 0x00200000L  /* Phase exists and is not constant        */
#define CTFHN 0x00400000L  /* d3news/d3go: Cell has noise             */
#define CTFMN 0x00800000L  /* NMOD card entered               (OPT1Q) */
#define CTFDN 0x01000000L  /* DEPRESSION card entered         (OPT1D) */
#define CTFMT 0x02000000L  /* MODULATE card entered           (OPT1M) */
#define CTFDR 0x04000000L  /* Step: OPT=D or RF=STEP                  */
#define CTFCN 0x08000000L  /* Centroid needed for phimeth or simeth   */
#define CTFDU 0x10000000L  /* Deferred update                 (OPT1U) */
#define CTFVP 0x20000000L  /* At least one conntype is voltage-dep P  */
#define CTFVD 0x40000000L  /* At least one conntype is voltage-dep V  */
   ui32 kaff;                 /* Afferent type bits, assigned as
                              *  (AFF type << SGM type) */
   long lel;                  /* Length of one element =
                              *  ls + sum(lct) */
   long llt;                  /* Length of layer data (total) =
                              *  nelt*lel (PAR: for this node) */
   long locell;               /* First cell on this processor */
   long logrp;                /* First group on this processor */
#ifdef PAR0
   long lsd1c;                /* Length of node simdata (one cell) */
#endif
   long lspt;                 /* Length of si,phase data (total) =
                              *  nelt*lsp */
   long mcells;               /* Number of cells from layer
                              *  assigned to this processor */
   long nce;                  /* No. of connections/element */
   long nel;                  /* No. of elements (cells) per group */
   long nelt;                 /* No. of elements (cells) in layer */
#ifdef PAR
   /* node1=1,nodes=NC.totalm1 until load optimizer is written */
   long node1;                /* First node  used by this cell type */
   long nodes;                /* Total nodes used by this cell type */
   long cpn;                  /* Cells per node */
   long crn;                  /* Cell remainder */
   long cut;                  /* First cell on node (node1+crn) */
#endif
   ui32 RCm;                  /* 1/Cm (S34) */
   si32 sisclu;               /* siscl used (S24) */
   mdlt_type amdlts;          /* All modalities celltype responds to */
   mdlt_type rmdlts;          /* XSTATS subset of amdlts */

   short ctclid[2];           /* Cell list identifiers for: */
#define CTCL_DPRINT 0            /* Detail print cell list offset */
#define CTCL_PRBSL  1            /* Probe definition index */
   short ctoff[CTNDV];        /* Offsets to dynamic cell variables:
                              *  subscripts defined in d3global.h */
   ui16  kautu;               /* Autoscale type codes actually used.
                              *  (S,G,M,A bits off if IMM or ET) */
   ui16  lcndg;               /* Length of CNDG data/cell */
   ui16  lmm;                 /* Length of all modality masks or
                              *  zero if globally no H|MXSTATS */
   ui16  lnqy;                /* Layer nqy = (nel - 1)/nqx + 1 */
   ui16  lnqyprev;            /* Sum (lnqy) of previous layers */
   ui16  ls;                  /* Length of cell-as-a-whole data */
   ui16  lsp;                 /* Length of si,phase data */
   ui16  lsrstr;              /* Length of ls data to restore */
   ui16  mxnr;                /* Max nr (d3allo-d3nset only) */
   ui16  mxsdelay1;           /* Maximum source delay + 1 */
   ui16  nAff32;              /* Number of afferent 32-bit data */
   ui16  nAff64;              /* Number of afferent 64-bit data */
   ui16  nCdist;              /* Number of C distributions--
                              *  whether or not currently turned on */
   ui16  ncnd;                /* Number of conductances */
   ui16  nct;                 /* No. of specific connection types */
   ui16  nctdist;             /* Number of dist  stats allocated */
   ui16  nctddst;             /* Number of ddist stats allocated */
   ui16  ngct;                /* No. of geometric connection types */
   ui16  nmct;                /* No. of modulatory connection types
                              *  (including NMOD if any) */
   ui16  nmdlts;              /* Number of modalities in rmdlts */
   ui16  oautsc;              /* Offset to sclmul in psclmul */
                              /* Offset in pctdist or pctddst of: */
   ui16  oCdist;              /* C (stimulus class) distributions */
   ui16  oFdist;              /* F (phase, phase change) dists */
#define OcFd  0                  /* Offset to phase dist in oFdist */
#define OcDFd 1                  /* Offset to delta phase dist */
   ui16  oGstat;              /* Offset in CDAT.pgstat of my G stats */
   ui16  ossei[NEXIN];        /* Offsets to self-input AffSums */
   ui16  seqvmo;              /* Celltype seq */
   ui16  upseq;               /* Update seq */

   byte jdptim;               /* Detailed print timer */
   byte ksipcu;               /* Si plot color actually used */
   byte ksiplu;               /* Si plot shape actually used */
   byte ksqbup;               /* How sbar,qbar are updated */
#define KSQUP_GO  1              /* Do in d3go */
#define KSQUP_AS  2              /* Do in d3autsc */
   byte nionc;                /* Number of ions (I+E) at cell level */
   byte nizvar;               /* Number Izhi abcd that vary per cell */
   byte nizvab;               /* Number ab that vary per cell */
   byte nizvcd;               /* Number cd that vary per cell */
   byte rspmethu;             /* Response method actually used */
   char pdshft;               /* Phase dist shift = PHASE_EXP or 0 */
   char phshft;               /* 1 if cell has phase, otherwise 0  */
   };
