/* (c) Copyright 1990-2018, The Rockefeller University *21115* */
/* $Id: rpdef.h 79 2018-08-21 19:26:36Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                          RPDEF Header File                           *
*                                                                      *
*  Define global CNS parameters shared on all nodes.  N.B.  Struct     *
*  must be same on PAR0,PARn nodes for membcst.                        *
*                                                                      *
************************************************************************
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
*  V8H, 05/26/12, GNR - Add lAffD64                                    *
*  V8I, 11/26/12, GNR - New overflow count mechanism, add novec        *
*  Rev, 12/07/12, GNR - Convert ns[xy], ku[xy] to ui16, nst to ui32    *
*  Rev, 02/17/13, GNR - Begin converting AffData to si32, start w/pers *
*  Rev, 04/23/13, GNR - Add knoup                                      *
*  Rev, 10/14/13, GNR - Add gsimax,gsimin                              *
*  Rev, 10/25/13, GNR - Eliminate 'L' constants                        *
*  R66, 02/17/16, GNR - Change long random seeds to typedef si32 orns  *
*  R67, 04/27/16, GNR - Remove Darwin 1 support, 09/15/16 add rlvlval  *
*  R70, 01/16/17, GNR - Change outnow states to bits,                  *
*                       Add OUT_ERROR_EXIT, OUT_GRACEFUL_EXIT          *
*  R70, 01/25/17, GNR - Add nctws, nhlsi, phlsi for high/low stats     *
*  R76, 11/06/17, GNR - Remove vdopt=C, OLDVDC, add OLDAMP, OLDPERS    *
*  R77, 02/17/18, GNR - Remove cpdia, 05/12/18, add nhlssv             *
*  R78, 06/21/18, GNR - Add OLDVDOPS so VDOPT=S is Vm, not Si          *
*  R78, 07/05/18, GNR - Add INTEGRATION and first case EULER           *
***********************************************************************/

typedef short outnow_type;    /* To get nxdr table for outnow */

/*---------------------------------------------------------------------*
*                CPDEF (CNS cycle parameter structure)                 *
*     (One of these is embedded in RPDEF for separate broadcast--      *
*                This struct should have no pointers)                  *
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
#define RP_KSDSG    0x00000020   /* Save current segment */
#define RP_OKPLOT   0x00000040   /* Plots allowed */
#define RP_OKPRIN   0x00000080   /* Print allowed */
#define RP_RVALL    0x00000100   /* Reset value on all series */
#define RP_TMRSET   0x00000200   /* Timed resets will occur */
#define RP_DOINT    0x00000800   /* Doing dV/dt integration */
#define RP_NOINPT   0x00002000   /* Not using input array */
#define RP_NOMETA   0x00004000   /* Not using metafile */
#define RP_NOXG     0x00008000   /* Not using X graphics */
#define RP_ONLINE   0x00010000   /* Run is online */
#define RP_REPLAY   0x00020000   /* Run is a replay */
#define RP_TEST     0x00040000   /* This is a test mode run */
#define RP_NOSTAT   0x00100000   /* Stats to be omitted (d3parm) */
#define RP_NOCYST   0x00200000   /* No stats this cycle group */
#define RP_ALLCYC   0x00400000   /* Collect stats on all cycles */
#define RP_SSTAT    0x00800000   /* Spout statistics */
#define RP_REGENR   0x01000000   /* One or more cell types has OPT=R */
#define RP_REPPR    0x02000000   /* One or more cell types has KCTP=E */
#define RP_OPTZZ    0x04000000   /* OPTIMIZE STORAGE */
#define RP_OPTOTD   0x08000000   /* OPTIMIZE OMIT-TYPE-DETAIL */
   volatile ui32 dbgflags;    /* Debug flags read from DEBUG parm */
#define DBG_CNS     0x0001       /* Wait at start of main */
#define DBG_MEMD    0x0002       /* Dump memory blocks after bcst */
#define DBG_ENDSET  0x0004       /* Wait after setup complete */
#define DBG_PROG    0x0008       /* Print progress in main */
#define DBG_PPKERN  0x0010       /* Plot preproc kernels */
#define DBG_AUTSC   0x0020       /* Print autoscale info */
#define DBG_GBETA   0x0040       /* Print revised GCONN betas */
#define DBG_VTOUCH  0x0080       /* Print touch pad info */
   volatile ui32 mfddbgflags; /* Debug flags for mfdraw */
   volatile ui32 mpidbgflags; /* Debug flags for mpitools */
   ulng effcyc;               /* Effective cycles since start of run */
   long rpirec;               /* Current record in replay file */
   ui32 runno;                /* Run number */
   ui32 trial;                /* Trials since start of series */
   ui32 trialsr;              /* Trials since last reset */
   ui32 ser;                  /* Current trial series number */
   int  rlnow;                /* Reset level to be executed */
/* Values reset actions may take on:
*  N.B.  Change with care!  Some code depends on these values */
#define RL_DEFER    -2           /* Defer setting this action */
#define RL_NONE      0           /* No reset action */
#define RL_CYCLE     1           /* Normal new cycle action */
#define RL_OLDTRIAL  2           /* Original omega1 trial action */
#define RL_TRIAL     3           /* Normal new trial action */
#define RL_SERIES    4           /* Timer, event, new series action */
#define RL_FULL      5           /* New series + CIJ0 action */
#define RL_NEWSERIES 6           /* Force new series */
#define RL_NEWFULL   7           /* Force new series + CIJ0 action */
#define RL_MAXOP     7           /* Largest legal level value */
/* RL_ISKIP must be > RL_MAXOP and a power of two (for masking) */
#define RL_ISKIP     8           /* Skip reset on first trial */
   outnow_type outnow;        /* Mode of ending simulation */
/* Values outnow may take on.  Bit values allow OR of multiple
*  exit causes, action can reflect highest priority.
*  Values >= OUT_END_INPUT terminate entire run at once,
*  values >= OUT_USER_PAUSE terminate series at once,
*  values > 0 cause reset & continue.  */
#define OUT_EVENT_TRIAL   0x01   /* Event requested extra trial */
#define OUT_EVENT_NEWSER  0x02   /* Event requested new series */
#define OUT_USER_PAUSE    0x04   /* User pushed interrupt button */
#define OUT_USER_PSTAT    0x08   /* User pause with stats */
#define OUT_END_INPUT     0x10   /* End of input */
#define OUT_USER_QUIT     0x20   /* QUIT button, end bbd input, etc. */
#define OUT_GRACEFUL_EXIT 0x40   /* Graceful exit--can save stuff */
#define OUT_ERROR_EXIT    0x80   /* d3exit called with error */
#define OUT_QUICK_EXIT  0x0100   /* Immediate (no cleanup) exit */
   byte trtimer[BytesInXBits(MXNTRT)]; /* Trial timers: 0,1,A-Z */
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
   CMNTYPE *pijpn;            /* Ptr to IJPLNODE data */
   struct IONTYPE *pfionexch; /* Ptr to first ion needing exchange */
   float  *pionxb;            /* Ptr to ion exchange buffer area */
#endif
   struct CELLTYPE *pfct;     /* Ptr to first CELLTYPE block */
   struct FDHEADER *pfdh;     /* Ptr to first feature detector header */
   struct MODALITY *pmdlt1;   /* Ptr to first modality block */
   struct MODVAL *pmvv1;      /* Ptr to first modulation virt value */
   struct PCF *ppcf1;         /* Ptr to first convolution func */
   struct PRBDEF *pfprb;      /* Ptr to first probe definition block */
   stim_type *pstim;          /* Ptr to stimulus array (stim) */
   PPTYPE *pfip;              /* Ptr to first preprocessor */
   TVTYPE *pftv;              /* Ptr to TV camera chain */
   struct VBDEF  *pvblk;      /* Ptr to first value block */
   struct VBDEF  *pvblkr;     /* Ptr to first rep value block */
   struct VDTDEF *pvdat;      /* Ptr to value array */
   si32   *pjdat;             /* Ptr to broadcast si32 data */
   si64   *boxsum;            /* Ptr to GCONN boxsums (S20/27) */
   long   *ptrack;            /* For use with debugger */

   /* Cycle parameters */
   struct CPDEF CP;           /* Stuff broadcast on each cycle */

   /* Floats */
   /* epl coords here for use in neuroanat plots on PARn nodes */
   float eplx;                /* Environment plot x coordinate */
   float eply;                /* Environment plot y coordinate */
   float eplw;                /* Environment plot width */
   float eplh;                /* Environment plot height */
   float fvcscl;              /* Scale float vc to S14 or S7 mV */
   float mnbpr;               /* Minimum bubble plot object radius */
   float sphgt;               /* Superposition plot height */
   float spwid;               /* Superposition plot width */
   float stdlht;              /* Standard plot letter height */

   /* Memory allocation totals for shared data areas (all in bytes) */
   ui64 lishare;              /* Length of shared GCONN space */
   long cumbcst;              /* Length of general broadcast area */
   long cumnds;               /* Cumulative size of markers for MDLT_
                              *  USED | KAMC | CTFS modalities
                              *  = (sumnds+7)/8  */
   long ldrsrv;               /* Length of deferred mem reserve */
   size_t ombits;             /* Offset in pbcst to modality marks */
   ulng liindiv;              /* Length of individual GCONN space */
   long xtrasize;             /* Largest size of various extras */
   long xmask,ymask;          /* Masks for x,y coords in IA offsets */
   long xymask;               /* Mask for valid bits in IA offsets */
   long xymaskc;              /* Complement of xymask */
   si32 eudt;                 /* Euler fractional time step (FBod) */
   si32 gsimax,gsimin;        /* Global max, min 16-bit response */
#ifdef PAR
   ui32 nhlpt;                /* Total num HILOSI per trial or cycle */
   ui32 nhlsi;                /* Total num HILOSI for nhlssv cycles */
#endif
   ui32 lAffD32;              /* Length of si32 CELLDATA data */
   ui32 lAffD64;              /* Length of si64 CELLDATA data */
   ui32 lmarkh;               /* Length of resp history mark area */
   ui32 lmarks;               /* Length of stim mark area (all CTs) */
   ui32 lkall;                /* Length of KGEN=K pkern tables */
   ui32 mxnce;                /* Max over celltypes of nce */
   ui32 mxsaa;                /* Cij avg space = global max(nsa) */
   ui32 ngact;                /* Size allocd to cond activn tables */
   ui16 nsx;                  /* Input array size along x */
   ui16 nsy;                  /* Input array size along y */
   ui32 nst;                  /* Pixels in IA = RP->nsx*RP->nsy,
                              *  0 before d3tchk or if no IA in run */
   ui32 nts;                  /* Number of trial series */
   ui32 ntr;                  /* Trials per series (=ntr1 or ntr2) */
   ui32 ntr1,ntr2;            /* Trials per series (normal,freeze) */
   ui32 ovflow;               /* 64-bit arith overflow flags (If ever
                              *  expanded to 64 bits, see d3exit call
                              *  in darwin3.c */
   si32 snmn;                 /* Stimulus noise mean (S24) */
   si32 snsg;                 /* Stimulus noise std dev (S28) */
   si32 snfr;                 /* Stimulus noise frac affected (S24) */
   orns snseed;               /* Stimulus noise seed */
   ui32 timestep;             /* Length of one time step (us) */
   int  kx;                   /* log2(X dimension) */
   int  ky;                   /* log2(Y dimension) */
   int  ky1;                  /* = (ky+1) */
   int  nvblk;                /* Number of value blocks--is int
                              *  because used as convrt rep count */
   int  tuqnel;               /* Sum of unique nel */
   short bsdc;                /* Sr2mV if COMPAT=C, else 0 */
   ui16  compat;              /* Flags for compatibility options */
/* Values for compat flags: */
#define OLDSCALE      0x0001     /* S: Use old scale normalization */
#define OLDRANGE      0x0002     /* C: Compat with old CNS s(i) range */
#define OLDKNEE       0x0004     /* K: G,M thresholds are knee type */
#define OLDAMP        0x0008     /* A: Amplify with s(i,t-1) */
#define OLDBETA       0x0010     /* B: Beta > 0 inhib, < 0 excit */
#define OLDIARND      0x0020     /* R: Old C8 color->gray rounding */
#define OLDPERS       0x0040     /* P: Persistence based on s(i,t-1) */
#define OLDVDOPS      0x0080     /* V: VDOPT=S uses old Vm */
#define RESETCIJ      0x0100     /* RESET CIJ:  Treat reset 3 as 4 */
   short ifrz;                /* Freeze interval (-1 for init frz) */
   short iti;                 /* Iter-trial interval (cycles) */
   ui16 kevtr,kevtb,kevtu;       /* Kind of event: trial, base, used */
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
#define EVTDO         0x0400     /* 1 Perform extra trial on event */
#define EVTVAL        0x0800     /* V Read value                   */
   ui16 kplot;                /* Plotting control = kplt1 | kplt2 */
/* Values of kplot and RP0->kplt1,kplt2 flags: */
#define PLFRZ         0x0001     /* F: Plot env in frozen cycles */
#define PLALL         0x0002     /* A: Plot env in all cycles */
#define PLGRD         0x0004     /* G: Grid plot */
#define PLSQR         0x0010     /* Q: Plot squares */
#define PLSUP         0x0020     /* S: Superposition plot */
#define PLDYN         0x0040     /* D: Dynamic config plot */
#define PLCOL         0x0080     /* K: Colored */
#define PLNST         0x0100     /* X: Omit Series and Trial */
#define PLRTR         0x0200     /* 2: Retrace env plot */
#define PLCSP         0x0400     /* C: Color scale plot */
#define PLVTS         0x0800     /* V: View time series data */
#define PLEDG         0x1000     /* E: Edges (show gconn borders) */
#define PLNOT         0x2000     /* O: Omit titles from plots */
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
#define KSDNE         0x0100     /* SAVE SIMDATA never entered (NEW)  */
#define KSDOFF        0x0400     /* SAVE SIMDATA OFF           (NEW)  */
#define KSDNOK        0x0800     /* Command-line NOSD          (NEW)  */
#define KSDHW         0x1000     /* SIMDATA header written     (NEW)  */
#define KSVNR         0x2000     /* Nogenerate in replay    (RPKSV2G) */
   ui16  mxlax;               /* Largest lrax for sums alloc */
   ui16  mxlmm;               /* Max over celltypes of lmm */
   ui16  mxnct;               /* Largest nct for any cell type */
   ui16  mxnds;               /* Largest nds for any modality */
   ui16  ncndg;               /* Total gated conductances (all types) */
   ui16  nhlssv;              /* Number phlsi trials (PAR) */
   short nfdh;                /* Number of feature detector hdrs read */
   short nijpldefs;           /* Number of IJPLDEF blocks */
   ui16  novec;               /* Number of overflow event codes */
   ui16  npcf;                /* Number of PCF's */
   ui16  nrep;                /* Number of repertoires in system */
   ui16  ntc;                 /* Cycles per stimulus (=ntc1 or ntc2) */
   ui16  ntc1,ntc2;           /* Cycles per stimulus (normal,freeze) */
   ui16  nuqnel;              /* Number of different nel in STGH */
   short nvcpl;               /* Number of virtual cell plots */
   ui16  ortvfl;              /* OR of utv.tvsflgs (active cams) */

/* User-defined colors.
*  Note:  When plot library color hashing is implemented,
*  color indexes rather than names should be stored here.  */
   char colors[COLORNUM+2*SEQNUM][COLMAXCH];
/* Indexes into color arrays (exactly COLORNUM=14 entries): */
#define CI_BOX             0     /* Boxes around plots */
#define CI_BUB             1     /* Cell response circles (bubbles) */
#define CI_EXT             2     /* External conns on anatomy plots */
#define CI_EXC             3     /* Excitatory connections */
#define CI_INH             4     /* Inhibitory connections */
#define CI_REP             5     /* Repertoire boxes in config plots */
#define CI_LAY             6     /* Layer boxes in config plots */
/* Code in d3ijpl, d3ijpz assumes next 2 defs are sequential.
*  These provide storage for colors manipulated during cycling.  */
#define CI_SPEX            7     /* Specified excitatory color */
#define CI_SPIN            8     /* Specified inhibitory color */
/* (Code in getcolor, d3lplt assumes next 4 defs are sequential) */
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
   byte kinteg;               /* Kind of integration */
#define KIEUL              1     /* First-order Euler integration */
   byte kdprt;                /* Detail print control (was KPS[4]) */
/* Values of kdprt flags: */
#define PRAUS           0x02     /* A Print autoscales        (NEW)  */
#define PRSPT           0x04     /* S Spout                  (KPS3S) */
#define PRVAL           0x08     /* V Print value each cycle  (NEW)  */
#define PRREP           0x10     /* R Report arm/wdw coords   (NEW)  */
#define PROBJ           0x20     /* O Print object coords     (NEW)  */
#define PRCHG           0x40     /* C Print changes           (NEW)  */
#define PRSNS           0x80     /* I Print sense data        (NEW)  */
   byte knoup;                /* No-update flags */
#define KNUP_AS         0x01     /* No update autoscales */
#define KNUP_RB         0x02     /* No update rbar */
   byte kpl,kpl1;             /* Immediate plot control bits */
#define KPLSUP          0x01     /* Do superposition plot      */
#define KPLMFSP         0x02     /* Do superposition metafile  */
#define KPLENV          0x04     /* Do environment plot        */
#define KPLDYN          0x08     /* Do dynamic config plot     */
#define KPLEDG          0x10     /* Do gconn box verify plot   */
#define KPLIRP          0x20     /* Do individual rep plots    */
#define KPLVCP          0x40     /* Do individual vcell plots  */
   byte kpr;                  /* Immediate print/plot control bits */
#define KPRREP          0x01     /* Do KCTP=E numeric s(i) print */
#define KPRENV          0x02     /* Do environment print */
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
#define GSTATS          0x10     /* G,M stats exist somewhere */
#define ZSTATS          0x20     /* Z stats exist somewhere */
#define ISKAMC          0x40     /* KAM=C exists somewhere */
#define ISOPTC          0x80     /* CTOPT=C exists somewhere */
/* Statistics that require marking stimulus numbers in d3go */
#define MXSTATS (XSTATS|YSTATS|ZSTATS|HSTATS|CSTATS|GSTATS)
   byte nitps;                /* Number of image touch senses */
   byte nmdlt;                /* Number of sensory modalities */
   byte narms;                /* Num arms defined in current env */
   byte nwdws;                /* Num wdws defined in current env */
   byte ntvs;                 /* Number of TV's */
   schr rlvlval;              /* Copy of rlvl[lvval] for d3resetn */
   };

#ifndef MAIN
extern
#endif
   struct RPDEF *RP;
