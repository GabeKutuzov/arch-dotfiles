/* (c) Copyright 1990-2012, Neurosciences Research Foundation, Inc. */
/* $Id: rpdef.h 46 2011-05-20 20:14:09Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                          RPDEF Header File                           *
*                                                                      *
*  Define global CNS parameters shared on all nodes.                   *
*                                                                      *
*  Rev, 05/15/90, JWT - Features/relations removed                     *
*  Rev, 10/01/90, GNR - Add stuff for MOVIE options                    *
*  Rev, 04/21/91, GNR - Add CHANGES and VCELLs, break out RPDEF0       *
*  V5C, 10/28/91, GNR - Add D1 stuff, cpdia, cumbcst, maskpr, maskpl   *
*  Rev, 03/07/92, KJF,GNR - Allow for GRAFDATA cell list in high mem   *
*  Rev, 04/08/92, ABP - Add RP_NOVDRD, RP_NOVDRE                       *
*  V5E, 08/04/92, GNR - Add support for NOGDEV, NOMETA, COMPAT         *
*  Rev, 10/14/92, GNR - Add trialsr (trials since reset)               *
*  Rev, 10/23/92, ABP - Move GRPDEF to comply with nxdr                *
*  V6A, 11/17/92, ABP - Move fields parm1, pwdw1, ps0, pgpno, penvl    *
*                       to RPDEF0.  Add fields 'prblnm', 'prbrnm'.     *
*  Rev, 04/19/93, GNR - Add outnow code for termination from driver    *
*  Rev, 05/20/93, GNR - Add RP_STEP and RP_PERSLIM, remove KSVGS       *
*  V6D, 02/22/94, GNR - Add RP_SPIKE, RP_RESETCIJ                      *
*  V7A, 04/24/94, GNR - Move lxall to rpdef0, now local in d[13]nset   *
*  V7B, 06/22/94, GNR - Add mxsaa for subarbor Cij averaging, mxnct,   *
*  Rev, 07/20/94, GNR - Compat flags, maskrp, kregen                   *
*  V7C, 11/12/94, GNR - Divide ngr into ncs,nds                        *
*  V8A, 05/20/95, GNR - Add modality, CYCLE KP. Copy envdef flag defs. *
*                       Remove RP_PERSLIM and COMPAT=H,P,N.            *
*  Rev, 12/08/97, GNR - Add RP_TEST, trials0, ldrsrv, remove nindv     *
*  V8B, 12/09/00, GNR - New memory management routines, trial timers   *
*  Rev, 09/01/01, GNR - Move probe information to a linked list        *
*  V8C, 02/22/03, GNR - Cell responses in millivolts                   *
*  V8D, 09/04/05, GNR - Add conductances and ions, new reset options   *
*  Rev, 10/20/07, GNR - Add TANH response function, kplt1, kplt2       *
*  ==>, 12/28/07, GNR - Last mod before committing to svn repository   *
*  Rev, 03/23/08, GNR - Add ntr1, ntr2, EVTRIAL, RP_RVALL              *
*  Rev, 01/17/09, GNR - Reorganize colors into a single array          *
*  Rev, 11/05/09, GNR - Add Group I STATISTICS card                    *
*  V8F, 02/21/10, GNR - Remove PLATFORM and VDR support                *
*  Rev, 04/24/10, GNR - Add KERNEL and PREPROC support                 *
*  V8G, 08/19/10, GNR - Add AUTOSCALE and PRAUS                        *
*  V8H, 05/25/11, GNR - New scheme to control reset actions            *
*  Rev, 05/26/12, GNR - Add lAffD64                                    *
***********************************************************************/

#ifndef CLTYPE                /* Eliminate undefined type warnings */
#define CLTYPE void
#endif
#ifndef CMNTYP
#define CMNTYP void
#endif
#if defined D1 && ! defined D1TYPE
#define D1TYPE void
#endif
#ifndef TVTYPE
#define TVTYPE void
#endif
#ifndef PPTYPE
#define PPTYPE void
#endif

typedef short outnow_type;    /* To get nxdr table for outnow */

/*---------------------------------------------------------------------*
*                CPDEF (CNS cycle parameter structure)                 *
*     (One of these is embedded in RPDEF for separate broadcast)       *
*---------------------------------------------------------------------*/

struct CPDEF {
   ui64 runtime;              /* Time since start of series (us) */
   ui32 runflags;             /* Flags about the current run */
/* Values for runflags: */
#define RP_FREEZE   0x00000001   /* In a freeze series */
#define RP_NOAMP    0x00000002   /* Command-line NOAMP parm */
#define RP_EVTRF    0x00000004   /* In a freeze EVTRIAL */
#define RP_NOCHNG   0x00000007   /* Not updating Cij values */
#define RP_EVTRL    0x00000008   /* This is an EVTRIAL trial */
#define RP_OKPLOT   0x00000040   /* Plots allowed */
#define RP_OKPRIN   0x00000080   /* Print allowed */
#define RP_TMRSET   0x00000200   /* Timed resets will occur */
#define RP_NOINPT   0x00002000   /* Not using input array */
#define RP_NOMETA   0x00004000   /* Not using metafile */
#define RP_NOXG     0x00008000   /* Not using X graphics */
#define RP_ONLINE   0x00010000   /* Run is online */
#define RP_REPLAY   0x00020000   /* Run is a replay */
#define RP_TEST     0x00040000   /* This is a test mode run */
#define RP_NOUPAS   0x00080000   /* No update autoscales */
#define RP_NOSTAT   0x00100000   /* Stats to be omitted (d3parm) */
#define RP_NOCYST   0x00200000   /* No stats this cycle group */
#define RP_ALLCYC   0x00400000   /* Collect stats on all cycles */
#define RP_SSTAT    0x00800000   /* Spout statistics */
#define RP_REGENR   0x01000000   /* One or more cell types has OPT=R */
#define RP_REPPR    0x02000000   /* One or more reps has KRP=R|E */
#define RP_OPTZZ    0x04000000   /* OPTIMIZE STORAGE */
#define RP_OPTOTD   0x08000000   /* OPTIMIZE OMIT-TYPE-DETAIL */
   volatile ui32 dbgflags;    /* Debug flags read from DEBUG parm */
#define DBG_CNS     0x0001       /* Wait at start of main */
#define DBG_MFDRAW  0x0002       /* Wait at start of mfdraw */
#define DBG_ENDSET  0x0004       /* Wait after setup complete */
#define DBG_PPKERN  0x0010       /* Plot preproc kernels */
   long rpirec;               /* Current record in replay file */
   long ser;                  /* Current trial series number */
   long trial;                /* Trials since start of series */
   long trialsr;              /* Trials since last reset */
   ui32 effcyc;               /* Effective cycles since start of run */
   outnow_type outnow;        /* Signals premature end of simulation */
/* Values outnow may take on (PAUSE and QUIT from plotdefs.h):
*  N.B.  Values >= OUT_USER_QUIT terminate entire run at once,
*        values > 0 terminate series at once, < 0 reset & continue.  */
#define OUT_EVENT_NEWSER  -2     /* Event requested new series */
#define OUT_EVENT_TRIAL   -1     /* Event requested extra trial */
#define OUT_USER_PAUSE     1     /* User pushed interrupt button */
#define OUT_USER_PSTAT     2     /* User pause with stats */
#define OUT_USER_QUIT      3     /* QUIT button, END card, etc. */
#define OUT_REPLAY_RANGE   4     /* Replay out of range */
#define OUT_QUICK_EXIT     5     /* Immediate (no cleanup) exit */
   byte trtimer[BytesInXBits(MXNTRT)]; /* Trial timers: 0,1,A-Z */
   };

/*---------------------------------------------------------------------*
*                   KRACT (Kind of reset activity)                     *
*---------------------------------------------------------------------*/

struct KRACT {
   ui16 kract;                /* Kinds of reset actions */
/* Values of kract flags:  (It would be easy to add separate codes
*  for type G and M connections, but there seems to be no point.)  */
#define KRABR         0x0001     /* B ctopt OPTBR all celltypes */
#define KRACIJ        0x0004     /* C Reset Cij to Cij0 */
#define KRAPRB        0x0008     /* P Reset probe timing */
#define KRAVAL        0x0010     /* V Reset all values to 0 */
#define KRASRM        0x0020     /* R Reset stim response marks */
#define KRACND        0x0040     /* G Reset channel conductances */
#define KRAUTV        0x0080     /* T Reset input from upgm cameras */
#define KRAION        0x0100     /* I Reset ion concentrations */
#define KRAARM        0x0200     /* A Reset arms in IA environment */
#define KRAWIN        0x0400     /* W Reset windows in IA */
#define KRAEFF        0x0800     /* E Reset efferent outputs */
#define KRANEWS       0x1000     /* N Start a new series */
#define KRUSUAL       0x0f78     /* Defaults for KRLSER level  */
   byte rlev;                 /* Reset action level (1-3) */
#define KRLNONE           0      /* 0 = no action */
#define KRLTRL            1      /* 1 = old TRIAL level */
#define KRLITI2           2      /* 2 = ITI trials, fast calc */
#define KRLITI3           3      /* 3 = ITI trials, full calc */
#define KRLSER            4      /* 4 = old SERIES level */
#define qKRLMX           "4"     /* Quoted max level */
   byte rlent;                /* TRUE if user entered rlev */
   };

/*---------------------------------------------------------------------*
*                  RPDEF (CNS root data structure )                    *
*---------------------------------------------------------------------*/

struct RPDEF {

   /* Pointers */
   struct REPBLOCK *tree;     /* Ptr to repertoire tree */
   byte *pbcst;               /* Ptr to general broadcast area
                              *  (see d3snsa for explanation) */
   gac_type *pgact;           /* Ptr to cond. activ'n tables (S30) */
   armwdw_type *paw;          /* Ptr to arm/window position data */
#define AWX                0     /* Index of X value (arm or window) */
#define AWY                1     /* Index of Y value (arm or window) */
#define WWD                2     /* Index of  width (window only) */
#define WHT                3     /* Index of height (window only) */
#define BARM_VALS          2     /* Num arm values brdcst (x,y) */
#define BWDW_VALS          4     /* Num wdw values brdcst (x,y,w,h) */
#define BARM_SIZE (BARM_VALS*sizeof(armwdw_type))
#define BWDW_SIZE (BWDW_VALS*sizeof(armwdw_type))
#ifdef PAR
   CMNTYP *pijpn;             /* Ptr to IJPLNODE data */
   struct IONTYPE *pfionexch; /* Ptr to first ion needing exchange */
   float  *pionxb;            /* Ptr to ion exchange buffer area */
#endif
#ifdef D1
   D1TYPE *pd1b1;             /* Ptr to first Darwin I block */
#endif
   struct CELLTYPE *pfct;     /* Ptr to first CELLTYPE block */
   struct FDHEADER *pfdh;     /* Ptr to first feature detector header */
   CLTYPE *pgdcl;             /* Ptr to grafdata cell block list */
   struct MODALITY *pmdlt1;   /* Ptr to first modality block */
   struct MODVAL *pmvv1;      /* Ptr to first modulation virt value */
   struct PCF *ppcf1;         /* Ptr to first convolution func */
   struct PRBDEF *pfprb;      /* Ptr to first probe definition block */
   si32   *psclmul;           /* Ptr to autoscale multipliers (S24) */
   stim_type *pstim;          /* Ptr to stimulus array (stim) */
   PPTYPE *pfip;              /* Ptr to first preprocessor */
   TVTYPE *pftv;              /* Ptr to TV camera chain */
   struct VBDEF  *pvblk;      /* Ptr to first value block */
   struct VBDEF  *pvblkr;     /* Ptr to first rep value block */
   struct VDTDEF *pvdat;      /* Ptr to value array */
   long   *ptrack;            /* For use with debugger */

   /* Cycle parameters */
   struct CPDEF CP;           /* Stuff broadcast on each cycle */
   /* The kra array gives the actions to be performed for various
   *  types of resets as indexed by enum ReLops in d3global.  */
   struct KRACT kra[NRLDIM];

   /* Floats */
   float cpdia;               /* Circle plot absolute diameter */
   /* epl coords here for use in neuroanat plots on PARn nodes */
   float eplx;                /* Environment plot x coordinate */
   float eply;                /* Environment plot y coordinate */
   float eplw;                /* Environment plot width */
   float eplh;                /* Environment plot height */
   float fvcscl;              /* Scale float vc to S14 or S7 mV */
   float sphgt;               /* Superposition plot height */
   float spwid;               /* Superposition plot width */
   float stdlht;              /* Standard plot letter height */

   /* Memory allocation totals for shared data areas (all in bytes) */
   long cumbcst;              /* Length of general broadcast area */
   long cumnds;               /* Cumulative size of markers for MDLT_
                              *  USED | KAMC | CTFS modalities
                              *  = (sumnds+7)/8  */
   long ldrsrv;               /* Length of deferred mem reserve */
#ifndef PAR
   long lishare;              /* Length of shared GCONN space */
   long lifixed;              /* Length of fixed, unshared GCONN */
#endif
   si32 mxngtht;              /* Max ngtht over autoscale types */
   long mxsaa;                /* Max subarbor Cij averaging space */
   long ngact;                /* Size allocd to cond activn tables */
   long nsx;                  /* Input array size along x */
   long nsy;                  /* Input array size along y */
   long nst;                  /* Pixels in IA = RP->nsx*RP->nsy,
                              *  0 before d3tchk or if no IA in run */
   ui32 lAffData;             /* Length of needed CELLDATA data */
   ui32 lAffD64;              /* Length of si64 CELLDATA data */
   ui32 nts;                  /* Number of trial series */
   ui32 ntr;                  /* Trials per series (=ntr1 or ntr2) */
   ui32 ntr1,ntr2;            /* Trials per series (normal,freeze) */
   ui32 ombits;               /* Offset in pbcst to modality marks */
   ui32 ovflow;               /* 64-bit arith overflow flags */
   si32 snmn;                 /* Stimulus noise mean (S24) */
   si32 snsg;                 /* Stimulus noise std dev (S28) */
   si32 snfr;                 /* Stimulus noise frac affected (S24) */
   long snseed;               /* Stimulus noise seed */
   ui32 timestep;             /* Length of one time step (us) */
   long xmask,ymask;          /* Masks for x,y coords in IA offsets */
   long xymask;               /* Mask for valid bits in IA offsets */
   long xymaskc;              /* Complement of xymask */
   int  kx;                   /* log2(X dimension) */
   int  ky;                   /* log2(Y dimension) */
   int  ky1;                  /* = (ky+1) */
   int  nvblk;                /* Number of value blocks--is int
                              *  because used as convrt rep count */
   ui16  adefer;              /* Amplification deferral cycles */
   short bsdc;                /* Sr2mV if COMPAT=C, else 0 */
   ui16  compat;              /* Flags for compatibility options */
/* Values for compat flags: */
#define OLDSCALE      0x0001     /* S: Use old scale normalization */
#define OLDRANGE      0x0002     /* C: Compat with old CNS s(i) range */
#define OLDKNEE       0x0004     /* K: G,M thresholds are knee type */
#define OLDVDC        0x0008     /* V: Omit G,M in volt-dep calc */
   short ifrz;                /* Freeze interval (-1 for default) */
   short iti;                 /* Iter-trial interval (cycles) */
   ui16 kevtr,kevtb,kevtu;    /* Kind of event: trial, base, used */
/* Values of kevtr flags:  */
#define EVTENV          0x01     /* I Update virtual environment   */
#define EVTFRZ          0x02     /* F Freeze amplif on event trial */
#define EVTPRB          0x04     /* P Apply probes normally        */
#define EVTWSD          0x08     /* W Write SIMDATA                */
/* EVTBTV,EVTVTV,EVTUTV must match TV_BBD,TV_VVTV,TV_UPGM, bzw.    */
#define EVTVTV          0x10     /* G Read trial VVTV grabber data */
#define EVTBTV          0x20     /* T Read trial BBD TV data       */
#define EVTUTV          0x40     /* U Read trial UPGM TV data      */
#define EVTSNS          0x80     /* S Read sensors                 */
#define EVTEFF        0x0100     /* E Send effector data           */
#define EVTD1         0x0200     /* D Read Darwin 1 data           */
#define EVTDO         0x0400     /* 1 Perform extra trial on event */
#define EVTVAL        0x0800     /* V Read value                   */
   ui16 kplot;                /* Plotting control = kplt1 | kplt2 */
/* Values of kplot and RP0->kplt1,kplt2 flags: */
#define PLFRZ           0x1L     /* F: Plot env in frozen cycles */
#define PLALL           0x2L     /* A: Plot env in all cycles */
#define PLGRD           0x4L     /* G: Grid plot */
#define PLSQR          0x10L     /* Q: Plot squares */
#define PLSUP          0x20L     /* S: Superposition plot */
#define PLDYN          0x40L     /* D: Dynamic config plot */
#define PLCOL          0x80L     /* K: Colored */
#define PLNST         0x100L     /* X: Omit Series and Trial */
#define PLRTR         0x200L     /* 2: Retrace env plot */
#define PLCSP         0x400L     /* C: Color scale plot */
#define PLVTS         0x800L     /* V: View time series data */
#define PLEDG        0x1000L     /* E: Edges (show gconn borders) */
#define PLNOT        0x2000L     /* O: Omit titles from plots */
   ui16 ksv;                  /* Save and misc option switches */
/* Values for ksv bits: */
#define KSVSR         0x0001     /* Save repertoires        (RPKSV3S) */
#define KSVRR         0x0002     /* Restore from saved reps (RPKSV3R) */
#define KSVS3         0x0004     /* GRP3 SAVENET            (RPKSV3V) */
#define KSVOK         0x0008     /* No command-line NOSAVE     (NEW)  */
#define KSVNS         0x0010     /* Save on every new series   (NEW)  */
#define KSVCFN        0x0020     /* Create new file names      (NEW)  */
#define KSVRFN        0x0040     /* Rotate file names          (NEW)  */
#define KSDNC         0x0080     /* Save internal cycles       (NEW)  */
#define KSDNE         0x0100     /* SAVE SIMDATA never netered (NEW)  */
#define KSDSG         0x0200     /* Save current segment       (NEW)  */
#define KSDOFF        0x0400     /* SAVE SIMDATA OFF           (NEW)  */
#define KSDNOK        0x0800     /* Command-line NOSD          (NEW)  */
#define KSDHW         0x1000     /* SIMDATA header written     (NEW)  */
#define KSVNR         0x2000     /* Nogenerate in replay    (RPKSV2G) */
#ifdef D1
   short md1byt;              /* Max over D1BLKs of nbyt */
#endif
   ui16  mxlax;               /* Largest lrax+lsax for sums alloc */
   ui16  mxlmm;               /* Max over celltypes of lmm */
   ui16  mxnct;               /* Largest nct for any cell type */
   ui16  mxnds;               /* Largest nds for any modality */
   short nijpldefs;           /* Number of IJPLDEF blocks */
   ui16  ncndg;               /* Total gated conductances (all types) */
   short nfdh;                /* Number of feature detector hdrs read */
   short npcf;                /* Number of PCF's */
   short nrep;                /* Number of repertoires in system */
   ui16  ntc;                 /* Cycles per stimulus (=ntc1 or ntc2) */
   ui16  ntc1,ntc2;           /* Cycles per stimulus (normal,freeze) */
   short nvcpl;               /* Number of virtual cell plots */
   ui16  ortvfl;              /* OR of utv.tvsflgs (active cams) */

/* User-defined colors.
*  Note:  When plot library color hashing is implemented,
*  color indexes rather than names should be stored here.  */
   char colors[COLORNUM+2*SEQNUM][COLMAXCH];
/* Indexes into color arrays: */
#define CI_BOX             0     /* Boxes around plots */
#define CI_BUB             1     /* Cell response circles (bubbles) */
#define CI_EXT             2     /* External conns on anatomy plots */
#define CI_EXC             3     /* Excitatory connections */
#define CI_INH             4     /* Inhibitory connections */
#define CI_REP             5     /* Repertoire boxes in config plots */
#define CI_LAY             6     /* Layer boxes in config plots */
#define CI_SPEC            7     /* Bubble color specified per celltype */
#define CI_BAR             8     /* Darwin I bars */
/* (Code in d3lplt assumes next 4 defs are sequential) */
#define CI_HPP             9     /* History plus-plus */
#define CI_HPM            10     /* History plus-minus */
#define CI_HMP            11     /* History minus-plus */
#define CI_HMM            12     /* History minus-minus */
/*                        13        Spare */
#define CI_SEQ      COLORNUM     /* Offset to bubble color sequence */
#define CI_PHS (COLORNUM+SEQNUM) /* Offset to phase color sequence */
   byte jdcptim;              /* Dynamic config plot timer */
   byte jeptim;               /* Individual environment plot timer */
   byte jmvtim;               /* Movie timer */
   byte jrsettm;              /* Reset timer */
   byte jsdsvtm;              /* SIMDATA save timer */
   byte jsptim;               /* Superposition plot timer */
   byte jspmftim;             /* Superpos plot metafile timer */
   byte jvcptim;              /* Virtual cell plot timer */
   char kcolor;               /* Environment color mode, see env.h */
   byte kdprt;                /* Detail print control (was KPS[4]) */
/* Values of kdprt flags: */
#define PRFOFF          0x01     /* F Print falloff tables    (REV)  */
#define PRAUS           0x02     /* A Print autoscales        (NEW)  */
#define PRSPT           0x04     /* S Spout                  (KPS3S) */
#define PRVAL           0x08     /* V Print value each cycle  (NEW)  */
#define PRREP           0x10     /* R Report arm/wdw coords   (NEW)  */
#define PROBJ           0x20     /* O Print object coords     (NEW)  */
#define PRCHG           0x40     /* C Print changes           (NEW)  */
#define PRSNS           0x80     /* I Print sense data        (NEW)  */
   byte kpl,kpl1;             /* Immediate plot control bits */
#define KPLSUP          0x01     /* Do superposition plot      */
#define KPLMFSP         0x02     /* Do superposition metafile  */
#define KPLENV          0x04     /* Do environment plot        */
#define KPLDYN          0x08     /* Do dynamic config plot     */
#define KPLEDG          0x10     /* Do gconn box verify plot   */
#define KPLIRP          0x20     /* Do individual rep plots    */
#define KPLVCP          0x40     /* Do individual vcell plots  */
   byte kpr;                  /* Immediate print control bits */
#define KPRREP             1     /* Do region print */
#define KPRENV             2     /* Do environment print */
   byte kpsta;                /* CYCLE KP statistics print options */
#define KPSUN              1     /* U No stats */
#define KPSQS              2     /* Q Print stats on mfdraw quit */
   char kregen;               /* Regeneration control */
   byte kxr;                  /* Cross-response switches */
/* Values of kxr flags: */
#define XSTATS          0x01     /* X stats exist somewhere */
#define YSTATS          0x02     /* Y stats exist somewhere */
#define HSTATS          0x04     /* History stats exist somewhere */
#define CSTATS          0x08     /* C stats exist somewhere */
#define GSTATS          0x10     /* G stats exist somewhere */
#define ZSTATS          0x20     /* Z stats exist somewhere */
#define ISKAMC          0x40     /* KAM=C exists somewhere */
#define ISOPTC          0x80     /* CTOPT=C exists somewhere */
/* Statistics that require marking stimulus numbers in d3go */
#define MXSTATS (XSTATS|YSTATS|ZSTATS|HSTATS|CSTATS|GSTATS)
   byte nmdlt;                /* Number of sensory modalities */
   byte narms;                /* Num arms defined in current env */
   byte nwdws;                /* Num wdws defined in current env */
   byte ntvs;                 /* Number of TV's */
   byte nd1s;                 /* Number of Darwin 1 arrays */
   };

#ifndef MAIN
extern
#endif
   struct RPDEF *RP;
