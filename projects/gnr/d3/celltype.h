/* (c) Copyright 1991-2018, The Rockefeller University *11115* */
/* $Id: celltype.h 79 2018-08-21 19:26:36Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                        CELLTYPE Header File                          *
*                                                                      *
************************************************************************
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
*  Rev, 08/28/12, GNR - Add KAM=L, PSI variable, UPSPSI, ZETAPSI       *
*  Rev, 12/24/12, GNR - New overflow checking                          *
*  Rev, 04/23/13, GNR - Add AUTOSCALE 'D' option, 05/14/13 'H' option  *
*  Rev, 10/15/13, GNR - Add sunder stat for negative truncation        *
*  Rev, 10/25/13, GNR - Eliminate 'L' constants                        *
*  Rev, 08/21/14, GNR - Eliminate CDCY                                 *
*  R65, 01/08/16, GNR - Eliminate mxnr, add node info (serial)         *
*  R66, 02/17/16, GNR - Change long random seeds to typedef si32 orns  *
*  R67, 05/03/16, GNR - Reduce nel, cpn, nodes etc. to ints per doc    *
*  R72, 02/21/17, GNR - Add lsd1c0, nsdc[apt]n for simdata gather      *
*  R72, 03/08/17, GNR - Add Lp, lnqx, change lnq[xy], lnqyprev to ints *
*  R74, 08/05/17, GNR - Separate kctp into kctp and kstat for stats    *
*  R74, Undated,  GNR - Add siha,CTPFC,sfnel,sfoff,bexbgr,binbgr,cpcap *
*  R76, 11/06/17, GNR - Remove vdopt=C, pctt, NORM_SUMS w/cons tables  *
*  R77, 02/13/18, GNR - Enhanced d3lplt bars options, LPCLIST, kctp=H  *
*  R78, 05/09/18, GNR - Add sslope multiplier for RF_STEP|RF_KNEE      *
*  R78, 06/27/18, GNR - Add vhist stat (distribution of Vm), ibias     *
***********************************************************************/

#include "plots.h"

/*---------------------------------------------------------------------*
*                               CLSTAT                                 *
*   Statistical section:  The order of variables matters in the case   *
*   of the parallel version of CNS.  When forming global statistics    *
*   one needs to know which variables are summed and which a global    *
*   max should be calculated for.  Order must match COPDEFs in d3gsta. *
*   Also order of some related items is assumed in d3stat().           *
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
   long sover,sunder;         /* Number of signal overflows */
   long nregen;               /* Number of cells regenerated */
   long sdist[LDSTAT];        /* Distribution of s(i) */
   long hdist[LDSTAT];        /* Distribution of high afferences */
   long ldist[LDSTAT];        /* Distribution of low afferences */
   } ;

/*---------------------------------------------------------------------*
*                               HILOSI                                 *
*  Space used on parallel computers to collect high, low over nodes    *
*  of responses to all stimuli on nhlssv trials at a time.             *
*---------------------------------------------------------------------*/

struct HILOSI {
   ui32  nstim;               /* Number of times presented */
   ui32  nhits;               /* Number of hits */
   si32  hiresp;              /* High response (S20/27) */
   si32  loresp;              /* Low response (S20/27) */
   } ;

/*---------------------------------------------------------------------*
*                               GPSTAT                                 *
*  Statistics collected for KSTGP|KSTMG options.                       *
*  (Allocated for nds stimuli on serial and PAR0 nodes only.)          *
*---------------------------------------------------------------------*/

struct GPSTAT {
   ui32 nstim;                /* Number appearances of this stim */
   ui32 nhits;                /* Number above ht */
   /* Code in d3stat assumes the next four items are adjacent */
   si32 hiresp;               /* Highest response to this stim */
   si32 lohiresp;             /* Lowest across trials of highest
                              *  response across cells */
   si32 hiloresp;             /* Highest across trials of lowest
                              *  response across cells */
   si32 loresp;               /* Lowest response to this stim */
   } ;

/*---------------------------------------------------------------------*
*                               AUTSCL                                 *
*  Parameters relating to the AUTOSCALE option                         *
*---------------------------------------------------------------------*/

struct AUTSSL {               /* Autoscale sorted s list */
#ifndef PARn                  /* (List is an array on PARn nodes) */
   struct AUTSSL *pnsl;       /* Ptr to next item in list */
#endif
   si32 resp;                 /* Cell response (S20/27) */
   si32 tdepol;               /* Total depolarization */
   si32 thypol;               /* Total hyperpolarization */
   } ;

struct AUTSSLs {              /* Short version for gtht method */
#ifndef PARn                  /* (List is an array on PARn nodes) */
   struct AUTSSLs *pnsl;      /* Ptr to next item in list */
#endif
   si32 resp;                 /* Cell response */
   } ;

#ifdef PAR0
struct AUTSSLn {              /* To get sizeof PARn AUTSSL on PAR0 */
   si32 resp;                 /* Cell response (S20/27) */
   si32 tdepol;               /* Total depolarization */
   si32 thypol;               /* Total hyperpolarization */
   } ;
struct AUTSSLsn {             /* To get sizeof PARn AUTSSLs on PAR0 */
   si32 resp;                 /* Cell response */
   } ;
#endif

struct ASSTAT {               /* Stats, repeated for HTN, HPN --
                              *  allocated right after CLSTAT */
   si32 loascl;               /* Low calculated autoscale (S24) */
   si32 hiascl;               /* High calculated autoscale (S24) */
   ui64 ncaps;                /* Number scales capped */
   } ;

typedef si32 asmul_type[2];   /* For blkbcst type */

struct AUTSCL {
   asmul_type asmul;          /* HTN, HPN scales (S24) */
#define AS_GHTB      0        /* HTN or only scale */
#define AS_HYPB      1        /* HPN scale in HI mode */
#define AS_NOP       2        /* No scale adjustment */
   /* Code in d3echo assumes next three items are adjacent */
   si32 asmxd;                /* Max delta in autoscale per trial */
   si32 asmnimm;              /* Min immediate scale (S24) */
   si32 asmximm;              /* Max immediate scale (S24) */
   orns asrsd;                /* Copy of rsd for determinism */
   si32 astta;                /* 'astt' scaled for affs (S20/27) */
   si16 astt;                 /* Target threshold (S7/14) */
   /* Code in d3echo assumes next two items are adjacent */
   ui16 astf1,astfl;          /* Fractions above astt (S15) */
   ui16 adamp;                /* Damping factor for asmul (S15) */
   ui16 iovec;                /* Offset to overflow count */
   ui16 kaut;                 /* Type codes */
#define KAUT_S    0x0001         /* 'S' Scale specific connections */
#define KAUT_G    0x0002         /* 'G' Scale geometric connections */
#define KAUT_M    0x0004         /* 'M' Scale modulatory connections */
#define KAUT_A    0x0008         /* 'A' Scale autaptic connections */
#define KAUT_ANY  0x000f         /* Any of S,G,M,A */
#define KAUT_CYC  0x0010         /* 'C' Calculate every cycle */
#define KAUT_IMM  0x0020         /* 'I' Apply result immediately */
#define KAUT_FS   0x0040         /* 'F' Fast start */
#define KAUT_ET   0x0080         /* 'T' New scale every trial */
#define KAUT_NU   0x0100         /* 'N' No update */
#define KAUT_UOK  0x0100         /* kautu: Update OK, = ^KAUT_NU */
#define KAUT_DNO  0x0200         /* 'D' Scale activity down only */
#define KAUT_HYP  0x0400         /* 'H' Scale hyperpolarization */
#define KAUT_WTA  0x0800         /* 'W' HTN cyc 1, then HPN */
#define KAUT_HTN  0x1000         /* kautu: Calc new >astt scale now */
#define KAUT_HPN  0x2000         /* Kautu: Calc new 'H' scale now */
#define KAUT_UPD  0x4000         /* kautu: Update running avg scale */
#define KAUT_APP  0x8000         /* kautu: Apply scale immediately */
   si16 navg;                 /* Number of cells in averages */
   } ;

/*---------------------------------------------------------------------*
*                               CTDFLT                                 *
*  Celltype defaults that may be entered on MASTER or REGION cards.    *
*  N.B. inform/convrt calls assume pt...ht are defined in order given. *
*---------------------------------------------------------------------*/

struct CTDFLT {
   float cpr;                 /* Default circle plot radius ratio */
   si32  pt;                  /* Default celltype pt (mV S20/27) */
   si32  st;                  /* Default celltype st (mV S20/27) */
   si32  nt;                  /* Default celltype nt (mV S20/27) */
   si32  ht;                  /* Default celltype ht (mV S20/27) */
   ui32  Cm;                  /* Membrane capacitance (pF S23)   */
   si32  siha;                /* Shunting inhib half-aff (mV S20/27) */
   si32  sslope;              /* Response function slope (S24)   */
   si32  sspike;              /* Default spike amplitude (mV S20/27) */
   si32  stanh;               /* Default tanh amplitude (mV S20/27) */
   ui32  taum;                /* Default membrane time const (ms S20) */
   si32  vrest;               /* Rest pot'l (for d3echo) (mV S20/27) */
   ui16  kctp;                /* Celltype-level print/plot options */
/* Flag bits for kctp */
#define KRPEP     0x0001         /* 'E' Print every s(i)       */
#define KRPCO     0x0002         /* 'K' Kolored                */
#define CTPLV     0x0004         /* 'L' Old, replaced by 'D'   */
#define KRPNP     0x0008         /* 'N' Neuroanatomy plot      */
#define KRPTP     0x0010         /* 'T' Neuroanat detail list  */
#define KRPDR     0x0020         /* 'D' Neuroanat drive, long  */
#define KRPPJ     0x0040         /* 'P' Neuroanat drive, proj  */
#define CTPSB     0x0080         /* 'B' Subarbor blocks        */
#define KRPAS     0x0100         /* 'A' Plot autoscales        */
#define CTPOM     0x0200         /* 'O' Omit detail print '+'  */
#define CTPFC     0x0400         /* 'F' Fixed exc,inhib colors */
#define CTPAR     0x0800         /* 'R' Arrows                 */
#define CTSWP     0x1000         /* 'S' Swap exc-inh colors    */
#define CTPHP     0x2000         /* 'H' Hold plot              */
/* N.B. If KRPXR or CPTNS changed, see case kctpcode in d3chng */
#define KRPXR     0x4000         /* 'X' Cross response stats   */
#define CTPNS     0x8000         /* 'U' No statistics          */
/* OR of all kctp options that imply making a bubble plot */
#define KRPBPL (CTPLV|KRPNP|KRPTP|KRPDR|KRPPJ|CTPSB)
/* OR of all kctp options that imply making a neuroanatomy plot */
#define KRPNPL (CTPLV|KRPNP|KRPTP|KRPDR|KRPPJ|CTPSB)
   ui16  kstat;               /* Celltype-level statistics options */
/* Flag bits for kstat,urkstat */
#define KSTFD     0x0001         /* 'F' Sdist by fd matrix     */
#define KSTGP     0x0002         /* 'G' s(mx,mn) summary only  */
#define KSTMG     0x0004         /* 'M' s(mx,mn) by stim id    */
#define KSTCL     0x0008         /* 'C' Sdist by stim ident    */
#define KSTHR     0x0010         /* 'H' History of responses   */
#define KSTZC     0x0020         /* 'Z' Correlation matrix     */
#define KSTYM     0x0040         /* 'Y' Matrix cross rsponses  */
#define KSTXR     0x0080         /* 'X' Cross response stats   */
#define KSTNS     0x0100         /* 'U' No statistics          */
/* OR of all kstat options that imply stimulus marking in d3go */
#define KSTXRM (KSTXR|KSTYM|KSTZC|KSTHR|KSTCL|KSTGP|KSTMG)
   ui16  ctopt;               /* Layer-level option control bits */
#define OPTRG     0x0001         /* 'R' Regenerate             */
#define OPTDR     0x0002         /* 'D' Discontinuous response */
#define OPTMEM    0x0004         /* 'M' Optimize memory        */
#define OPTBR     0x0008         /* 'B' Bypass reset           */
#define OPTNR     0x0020         /* 'N' Noise restore          */
#define OPTOR     0x0040         /* 'O' Override or no restore */
#define OPTJS     0x0080         /* 'J' Just reset s(i)        */
#define OPTSQ     0x0100         /* 'Q' sQuared GCONN inhib    */
#define OPTSH     0x0200         /* 'S' Shunting GCONN inhib   */
#define OPTAB     0x0400         /* 'A' Abrupt refrac. period  */
#define OPTPH     0x0800         /* 'P' Phase requested        */
#define OPTCFS    0x1000         /* 'C' New class->fast r,qbar */
#define OPTNIN    0x2000         /* 'I' No initial noise       */
#define OPTFZ     0x4000         /* 'F' Freeze after 1 cycle   */
   short cpcap;               /* Circle plot cap (S7/S14 mV)   */
   short cpt;                 /* Circle plot threshold (S7/14 mV) */
   byte  jketim;              /* kctp=E print timer */
   byte  rspmeth;             /* Response method--a RespFunc enum--
                              *  not valid before d3tchk() */
   byte  ksiple;              /* Si plot shape entered--actually an
                              *  enum SiShapes, but declared this way
                              *  so an entire int is not consumed.  */
   byte  ksipce;              /* Si plot color source entered */
#define MxKSBARS       5      /* Max number of bars in bars plot */
   byte  ksbarn;              /* Number of bars in SIS_Bars plot */
   byte  kksbar[MxKSBARS];    /* Sources for bars */
   /* Named Si excit,inhib colors */
   char  sicolor[NEXIN][COLMAXCH];
   };
/* Values that can be taken on by ksipl (SIS_Circle is default): */
enum SiShapes { SIS_None = 0, SIS_Circle, SIS_Square, SIS_DownTR,
   SIS_LeftTR, SIS_UpTR, SIS_RightTR, SIS_PhNdl, SIS_OrNdl,
   SIS_Bars, SIS_GMxBars };
/* Values that can be taken on by kksbar bar identifiers
*  (ix->ict is in high nibble for conntype afference plot)  */
enum SiBarId { SIB_None = 0, SIB_Si, SIB_Vm, SIB_Pers, SIB_Depr,
   SIB_Noise, SIB_Izhu, SIB_Aff };
/* Values that can be taken on by ksipce,ksipcu (color source).
*  SIC_None means "None entered" and implies the default SIC_Si */
enum SiColSrc { SIC_None = 0, SIC_Si, SIC_SBar, SIC_QBar, SIC_Phase,
   SIC_Hist, SIC_Izhu, SIC_GAvg, SIC_GMax };
/* Subscripts for ctclloc: */
#define NCLIDS 4              /* Number of items in following enum */
enum Clids { CTCL_DPRINT = 0, CTCL_SIMDAT, CTCL_NANAT, CTCL_LPLT };

/*---------------------------------------------------------------------*
*                              CELLTYPE                                *
*---------------------------------------------------------------------*/

struct CELLTYPE {
   struct CELLTYPE *pnct;     /* Ptr to next CELLTYPE globally */
   struct CELLTYPE *play;     /* Ptr to next CELLTYPE in region */
   rd_type         *prd;      /* Ptr to actual rep (cell) data */
   rd_type         *prdbak[MxKSBARS];  /* Ptrs to SIB_Aff AK data */
   rd_type         *prdraw[MxKSBARS];  /* Ptrs to SIB_Aff RAW data */
   s_type          **pps;     /* Ptr to ptrs to s(i) lookup tables
                              *  (ptrs are indexed by delay time) */
   s_type          *ps2;      /* Ptr to s(i) table for node contrib */
   struct REPBLOCK *pback;    /* Ptr back to governing REPBLOCK */
   struct CELLTYPE *pusrc;    /* Ptr to upwith CELLTYPE */
   struct CONDUCT  *pcnd1;    /* Ptr to first conductance */
   struct IONTYPE  *pion1;    /* Ptr to first ion type */
   struct CONNTYPE *pct1;     /* Ptr to first CONNTYPE */
   struct INHIBBLK *pib1;     /* Ptr to first INHIBBLK */
   struct MODBY    *pmby1;    /* Ptr to first MODBY */
   struct MODVAL   *pmrv1;    /* Ptr to first rep-type MODVAL */
   struct AUTSCL   *pauts;    /* Ptr to AUTOSCALE info */
   struct PHASEDEF *pctpd;    /* Ptr to PHASEDEF if phase */
   struct PRBDEF   *pctprb;   /* Ptr to probe defs for this layer */
   struct RFRCDATA *prfrc;    /* Ptr to refractory period params */
   struct EFFARB   *pfea;     /* Ptr to first associated EFFARB */
   struct CLSTAT   *CTST;     /* Ptr to statistics block */
   void            *prfi;     /* Ptr to response function info:
                              *  BREGDEF, IZ03DEF, or IZ07DEF */
   struct CLBLK    *dpclb;    /* Ptr to detail print cell list blk */
   struct CLBLK    *psdclb;   /* Ptr to SIMDATA cell list block */
   struct CLBLK    *ptclb;    /* Ptr to neuranat cell list block */
   struct CLBLK    *plpclb;   /* Ptr to layer plot cell list block */
   ddist           *pctddst;  /* Ptr to celltype distribution stats */
   dist            *pctdist;  /* Ptr to celltype count statistics */
   hist            *phrs;     /* Ptr to history stats (1 per mdlty) */

   struct CTDFLT   Ct;        /* MASTER CELLTYPE parameters */
   struct DCYDFLT  Dc;        /* MASTER DECAY parameters */
   struct DEPRDFLT Dp;        /* MASTER DEPRESSION parameters */
   struct NDFLT    No;        /* MASTER NOISE parameters */

   orns nsd0;                 /* Value of nsd at start of cycle */
   orns rsd;                  /* Random-rounding seed for decay */
   ui32 urkstat;              /* Original settings of kstat */
   ui32 svitms;               /* Save items rqst: codes in simdata.h */
   ui32 suitms;               /* Save items used: codes in simdata.h */

   si16 ibias;                /* Initial bias activity (S7/14) */
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
#define DPTERM  0x4000           /* T: Print amplification terms */
#define DPDIDL  0x8000           /* Set when Lij have been printed */

   byte apspov;               /* Autaptic PSP usage override */
   byte jdptim;               /* Detailed print timer */
   char lname[LSNAME];        /* Name of this cell type */

/* Work areas and derived quantities */

   union CTctwk {
      struct {                /* Items used in d3grp2-d3echo only */
         struct MODBY *pnmby;    /* Ptr to noise modulation block */
         orns  mastseed;         /* Master seed for d3echo */
         rlnm  usrcnm;           /* UPWITH rep-layer name */
         byte  orkdecay;         /* OR of kdecay for all conntypes */
         byte  krmop;            /* Bits for rspmeth options, etc. */
#define KRM_RFENT 0x80              /* Response function entered */
#define KRM_PIEXT 0x08              /* Print Izhikevich extensions */
#define KRM_MODND 0x04              /* Modality needed for OPT=C */
#define KRM_SPENT 0x02              /* SPIKE value entered */
#define KRM_THENT 0x01              /* THAMP value entered */
         } tree;
      struct {                /* Items used in d3fchk-d3rstr */
         RKFILE *fdes;           /* File descriptor for restore */
         size_t  offsi;          /* Offset of s(i) in restore file */
         ui64  offprd;           /* Offset of "ls" in restore file */
         ui64  rflel;            /* Restore file element length */
         short rfphshft;         /* Phase shift in restore file */
         short rfdelay;          /* Delay cycles in restore file */
         char  hmwarn[SFHMNTYP]; /* Header match warning flags */
#define HMWM1 1                  /* Matched at least one header */
#define HMWWI 2                  /* Warning issued */
         } phs1;
      struct {                /* Items used in d3go */
         float fstanh;           /* = (float)stanh */
         float fvmscl;           /* = (float)(1/(S20 or S27)) */
         si32  ntca;             /* Number cycles layer is active - 1 */
         mdlt_type delmk;        /* Stimulus change bits by modality  */
         char  marking;          /* TRUE if marking is being done */
         char  masking;          /* TRUE if H|MXSTATS and doing stats */
         } go;
      } ctwk;

   ui64 liboxes;              /* Length of my GCONN boxes */
   ui64 lel;                  /* Length of one element =
                              *  ls + sum(lct) */
   ui64 llt;                  /* Length of layer data (total) =
                              *  nelt*lel (PAR: for this node) */
   ui64 wspt;                 /* Length of si,phase data (total) = */
   size_t lspt;               /* nelt*lsp */
   size_t mspt;               /* Length of mcells si.phase data */

   struct CPLOC Lp;           /* Layer plot parameters */
   float xlal,ylal;           /* xla,yla of last lplt (for NANAT) */
   ui32 ctf;                  /* Miscellaneous cell type flags */
/* Values that may be taken on by ctf (low 5 bits shared): */
#define DIDSCALC 0x00001   /* d3go: Set when s(i) calculated          */
#define DIDSTATS 0x00002   /* d3go->d3stat: Set when statistics done  */
#define CTFSTENB 0x00004   /* d3go: Statistics enabled in this cycle  */
#define CTFDPENB 0x00008   /* d3go: Detail print exists & is enabled  */
#define CTFULCHK 0x00001   /* d3tchk: Used to find update loops       */
#define CTFALIGN 0x00002   /* d3allo: Align cell data on si32 boundary*/
#define CTFAE    0x00004   /* d3schk->d3allo: Drives effector (excit) */
#define CTFAI    0x00008   /* d3schk->d3allo: Drives effector (inhib) */
#define RSTOLDSI 0x00010   /* d3fchk->d3rstr: Restore from L1 si file */

#define CTDPDIS  0x00080   /* Disable detailed print                  */
#define CTFAS1T  0x00100   /* Autoscale uses astf1                    */
#define CTFASFS  0x00200   /* Autoscale fast start time               */
#define DIDIMAW2 0x00400   /* Did immediate amp-omega1 warning        */
#define DIDIMAW3 0x00800   /* Did immediate amp-CDCY.omega warning    */
#define CTFI3IVR 0x01000   /* IZH3 cells have individual Vrest in mem */
#define CTFI3IVC 0x02000   /* IZH3 cells must calc have indiv Vrest   */
#define CTFRANOS 0x04000   /* Indiv. Izhi coeffs and OPTIMIZE STORAGE */
#define CTFDOFS  0x08000   /* Time to do fast start sbar              */
#define CTFPG 0x00020000   /* Plot group: SIC_GAvg|GMax|SIS_GMx|OrNdl */
#define CTFgD 0x00040000   /* Celltype has at least one cond w/gDCY   */
#define CTFXD 0x00080000   /* Celltype drives one with XYZHCGM stats  */
#define CTFGV 0x00100000   /* At least one inhibblk has IBOPT=V       */
#define CTFNC 0x00200000   /* Phase exists and is not constant        */
#define CTFMN 0x00800000   /* NMOD card entered               (OPT1Q) */
#define CTFDN 0x01000000   /* DEPRESSION card entered         (OPT1D) */
#define CTFMT 0x02000000   /* MODULATE card entered           (OPT1M) */
#define CTFDR 0x04000000   /* Step: OPT=D or RF=STEP                  */
#define CTFCN 0x08000000   /* Centroid needed for phimeth or simeth   */
#define CTFDU 0x10000000   /* Deferred update                 (OPT1U) */
#define CTFPD 0x20000000   /* Save phase distribution for SIMDATA     */
   ui32 iGstat;               /* Index in CDAT.pgstat of my G stats */
   ui32 iHLsi;                /* Index in CDAT.phlsi of my HiLo si  */
   ui32 iMarkh;               /* Index in CDAT.pmarkh of my history */
   ui32 iMarks;               /* Index in CDAT.pmarks of my marks */
   ui32 logrp;                /* First group on this node */
   ui32 lsd1c;                /* Length of node simdata (one cell) */
   ui32 lsd1c0;               /* Length of segment 0 node simdata */
   ui32 higrp;                /* Last group on this node + 1 */
   ui32 mgy;                  /* Groups along y on current node */
   ui32 nce;                  /* No. of connections/element */
   ui32 oGstat;               /* Offset in CDAT.pgstat of my G stats */
   ui32 orkam;                /* OR of kam  bits for all conntypes */
   ui32 RCm;                  /* 1/Cm (S34) */
   si32 sisclu;               /* siscl used (S24) */
   int  cpn;                  /* Cells per node */
   int  crn;                  /* Cell remainder */
   int  cut;                  /* First cell on node (node1+crn) */
   int  lnqx;                 /* Layer nqx (cells/grp horizontal) */
   int  lnqy;                 /* Layer nqy = (nel - 1)/nqx + 1 */
   int  lnqyprev;             /* Sum (lnqy) of previous layers */
   int  locell,hicell;        /* First,end cell on this processor */
   int  mcells;               /* Number of cells from layer
                              *  assigned to this processor */
   int  nsdcan;               /* Number simdata cells, all nodes */
   int  nsdcpn;               /* Number simdata cells, previous nodes */
   int  nsdctn;               /* Number simdata cells, this node */
   int  nel;                  /* No. of elements (cells) per group */
   int  nelt;                 /* No. of elements (cells) in layer */
   int  node1;                /* First node  used by this cell type */
   int  nodes;                /* Total nodes used by this cell type */
   int  sfnel,sfoff;          /* Steerable filter unique nel info */
   mdlt_type amdlts;          /* All modalities celltype responds to */
   mdlt_type rmdlts;          /* XSTATS subset of amdlts */

   clloc ctclloc[NCLIDS];     /* Cell list locators */
   short ctoff[CTNDV];        /* Offsets to dynamic cell variables:
                              *  subscripts defined in d3global.h */
   ui16  iovec;               /* Offset to overflow counts */
#define CTO_RFN   0              /* Addition for response function */
#define CTO_RSET  1              /* Addition for reset */
#define CTO_DCY   2              /* s, q, dpr, dst etc. decays */
#define CTO_PSI   3              /* Addition for psi */
#define CTO_AFF   4              /* Addition for afference terms */
#define CTO_ALLO  5              /* Memory allocation calculations */
#define CTO_RGEN  6              /* Cell regeneration */
#define CTO_NOFF  7              /* Number of iovec additions */
   bgrv  bexbgr;              /* Bubble plot excit high BGR colors */
   bgrv  binbgr;              /* Bubble plot inhib high BGR colors */
   ui16  kautu;               /* Autoscale type codes actually used.
                              *  (S,G,M,A bits off if IMM or ET) */
   ui16  lcndg;               /* Length of CNDG data/cell */
   ui16  lmm;                 /* Length of all modality masks or
                              *  zero if globally no H|MXSTATS */
   ui16  lnir;                /* Number of layer in its region */
   ui16  logrpy;              /* Low groupy on this node */
   ui16  higrpy;              /* High group y on this node + 1 */
   ui16  ls;                  /* Length of cell-as-a-whole data */
   ui16  lsp;                 /* Length of si,phase data */
   ui16  lsrstr;              /* Length of ls data to restore */
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
                              /* Offset in pctdist or pctddst of: */
   ui16  oCdist;              /* C (stimulus class) distributions */
   ui16  oFdist;              /* F (phase, phase change) dists */
#define OcFd  0                  /* Offset to phase dist in oFdist */
#define OcDFd 1                  /* Offset to delta phase dist */
   ui16  ossei[NEXIN];        /* Offsets to self-input AffSums */
   ui16  seqvmo;              /* Celltype seq */
   ui16  upseq;               /* Update seq */

   byte kaff[NKAFB];          /* Afferent type bits */
   byte ksipcu;               /* Si plot color source actually used */
   byte ksiplu;               /* Si plot shape actually used */
   byte sicop;                /* Si options derived from color */
#define SICECSQ  0x01         /* Cell plot excit color is SEQUENCE */
#define SICICSQ  0x02         /* Cell plot inhib color is SEQUENCE */
#define SICECOM  0x04         /* Omit plotting excit Si */
#define SICICOM  0x08         /* Omit plotting inhib Si */
   byte nauts;                /* Number of autoscales that exist--
                              *  whether or not updated this cycle */
   byte nionc;                /* Number of ions (I+E) at cell level */
   byte nizvar;               /* Number Izhi abcd that vary per cell */
   byte nizvab;               /* Number ab that vary per cell */
   byte nizvcd;               /* Number cd that vary per cell */
   byte orphopt;              /* OR of phopt bits for all conntypes */
   byte orvdopt;              /* OR of vdopt bits for all conntypes */
   byte rspmethu;             /* Response method actually used */
   char pdshft;               /* Phase dist shift = PHASE_EXP or 0 */
   char phshft;               /* 1 if cell has phase, otherwise 0  */
   };
