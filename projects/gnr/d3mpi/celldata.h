/* (c) Copyright 1994-2018, The Rockefeller University *11115* */
/* $Id: celldata.h 77 2018-03-15 21:08:14Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                             celldata.h                               *
*                                                                      *
*  CELLDATA structure:  Defines data used in calculation of responses  *
*                       of individual cells.                           *
*  CONNDATA structure:  Defines data used in calculations relating to  *
*                       individual synapses, Cij amplification, etc.   *
*                                                                      *
*  These structures are used for communication between d3nset, d3go,   *
*  d3dprt, d3inhb, and their relatives, including dynamically-linked   *
*  user-defined routines.  The one actual instance of CELLDATA occurs  *
*  in d3go() and is static. Items having event flags (noise, probeval) *
*  are valid only when the corresponding flag is set. Data in these    *
*  structures are NOT broadcast at Group III time, therefore, items    *
*  that need to be preserved across broadcasts can go here.            *
*                                                                      *
************************************************************************
*  V6D, 01/10/94, GNR - Newly defined based on DETPRT structure        *
*  V7B, 06/23/94, GNR - Add CONNDATA structure                         *
*  V8A, 09/11/95, GNR - Introduce new mechanism for tracking afferents,*
*                       add group[nxy]                                 *
*  V8C, 03/13/03, GNR - Cell responses in millivolts, add conductances *
*  V8D, 04/28/07, GNR - Add SGM_NoiseMod -- in dprt, stats, not sums   *
*  ==>, 12/28/07, GNR - Last mod before committing to svn repository   *
*  V8E, 01/31/09, GNR - Add Izhikevich u variable, a,b,c,d parms       *
*  V8F, 07/01/10, GNR - Add psws                                       *
*  V8H, 04/01/11, GNR - Add curr_cyc1                                  *
*  Rev, 05/26/12, GNR - Add pAffD64, remove pglway                     *
*  Rev, 08/29/12, GNR - Add psi, 10/13/12, add dcyh                    *
*  V8I, 11/26/12, GNR - New overflow count mechanism, povec, pdecay    *
*  Rev, 02/17/13, GNR - Begin converting AffData to si32, start w/pers *
*  Rev, 05/21/13, GNR - Add AS{Hyp,Dep}Tables for autoscale OPT=H,W    *
*  Rev, 07/20/13, GNR - New kaff scheme encodes self/nonself conntypes *
*  Rev, 10/15/13, GNR - Add rspmax, rspmin                             *
*  Rev, 08/23/14, GNR - Remove dcyh, add CLEAREDSI                     *
*  R67, 10/07/16, GNR - Add DPRT=T amplification info to CONNDATA      *
*  R76, 11/06/17, GNR - Remove vdopt=C, pctt, NORM_SUMS w/cons tables  *
*  R79, 08/13/18, GNR - New storage for KSTGP|KSTMG|KSTHR stats        *
***********************************************************************/

/*=====================================================================*
*                              WARNING!                                *
*=====================================================================*/
/* The CELLDATA structure down to 'flags' and afferent distributions
*  it points to are transmitted from comp nodes to host for detailed
*  print by special ad hoc code in d3pdpz() and d3pdpn().  This code
*  must be modified if the current setup is changed from that given in
*  the comments below.  See comments in d3pdpn.c for further info.  */

/* Definition of 'event' flag bits (lifetime--process one cell): */
#define REFRAC     0x01    /* Cell in absolute refractory period */
#define DECAYED    0x02    /* Persistence term is valid */
#define PROBED     0x08    /* Probe current was applied */
#define SPIKED     0x10    /* Cell made a spike */

/* Definition of 'flags' flag bits (lifetime--one cycle): */
#define CLEAREDSI  0x80    /* Before this cycle set all s(i) to 0 */

/* Define application modes of connections (bits in il->kaff) */
#define AFF_Excit    PSP_POS
#define AFF_Hyper    PSP_NEG
#define AFF_Square      4
#define AFF_Shunt       8
#define AFF_CellResp 0x10  /* Total response in SGM_Total */
#define AFS_Shft        4  /* Shift from AFF_xxx to AFS_xxx bits */
#define AFS_Excit    0x10  /* Self-excitatory */
#define AFS_Hyper    0x20  /* Self-hyperpolarizing */
#define AFS_Square   0x40  /* Self-squared */
#define AFS_Shunt    0x80  /* Self-shunting */

/* Define byte offsets in kaff of generic types of connections.
*  N.B.  Specific, VoltDep, Self must be adjacent for lSpecSums */
#define SGM_Total       0  /* Must be zero--see d3oafs */
#define SGM_Specific    1  /* Holds normals only if any VoltDeps */
#define SGM_VoltDep     2  /* Voltage-dependent connections */
#define SGM_Self        3  /* Autaptic AFS_Excit,AFS_Hyper */
#define SGM_Geometric   4  /* Geometric connection types */
#define SGM_Modulatory  5  /* Modulatory connection types */
#define SGM_Conduct     6  /* Conductances -- may merge w/VoltDep */

/* Definitions needed to set up consolidation tables.
*  Note:  36 raw items = 8 ((AFF+AFS) * (Excit + 3 Inhib)) x
*     4 sources (Spec, VDep, Geom, Mod) + 2 (Excit, Hyper) x
*     2 sources (Self, Conduct) -- Update when sources change.  */
#define MaxCons        44  /* Max terms in cons tables = 36 raw
                           *  items + 2 in finals + 6 end marks */
#define MaxASDH        38  /* Max terms in ASxTables + 2 end marks */
#define EndConsTbl     -1  /* Signal at end of consolidation table */
#define EndAllConsTbl  -2  /* Signal at end of all consol. tables */

/* Definitions used in transmitting detail print info to host */
#ifdef PAR
#define END_OF_GROUP SI32_MAX    /* End of data group delimiter */
#define MISSING_SJ (SI32_MAX-1)  /* Sj missing, connection skipped */
#define lnndat ((char *)&(((struct CELLDATA *)0)->cdlayr) - (char *)0)
#endif

struct CELLDATA {
/* The following fields must be transmitted with swapping: */
   ui32 cdcell;            /* Number of current cell */
   ui32 groupn;            /* Number of current group */
   si32 depr;              /* Depression(t) (S8) */
   si32 dst;               /* Increment in spike threshold (mV S20) */
   si32 izhu;              /* Izhikevich cell 'u' variable (mV S20) */
#define IZ_SPIKED 1           /* Cell spiked in previous cycle */
   si32 i3vr;              /* Izhikevich 2003 Vrest (mV S20) */
   si32 izha,izhb;         /* Izhikevich a,b (when variable) */
   si32 izhc,izhd;         /* Izhikevich c,d (when variable) */
   si32 noise;             /* Noise (mV S20) (may be for Vm or s(i) */
   si32 nmodval;           /* (nfr-noise modulation value) (S24) */
   si32 pers;              /* Persistence (mV S20/27) */
   si32 probeval;          /* Probe current (mV S20) */
   si32 sbar;              /* Time avg cell response (mV S7) */
   si32 qbar;              /* Slower time average Si (mV S1) */
   si32 ompsi;             /* 1 - Amplif. refractoriness (S15) */
   si32 old_si;            /* Si(t-1) (mV S20) */
   si32 new_si;            /* Si( t ) (mV S20) */
   si32 new_affs;          /* Total (A+G+M-Q*Q+V)/(1+D)+P (mV S20) */
   si32 new_vm;            /* Vm( t ) (mv S20) */
   ui16 groupx;            /* X coord of current group */
   ui16 groupy;            /* Y coord of current group */
/* The following fields must be transmitted with no swapping */
   byte old_phi;           /* Old phase = phase(t-1) */
   byte new_phi;           /* New phase = phase( t ) */
   byte events;            /* Cell event flags */
   byte flags;             /* Miscellaneous flags */

/* The following fields must not be transmitted to host (but the data
*  pointed to may be transmitted).  Offset of cdlayr is used to compute
*  length of data to be transmitted.  Raw sums are on scale (S23),
*  processed sums are on scale (S20).
*
*  N.B.  pAffDxx, etc. are used to store raw and processed sums as
*  well as persistence data for detail print, always in the order
*  raw, persistence, processed, first the excitatory set, then the
*  inhibitory set if both are present.  Space is allocated and data
*  are stored regardless of whether or not Omit-Type-Detail has been
*  specified, so values are available for statistics or other uses
*  and 'if' statements are not needed at points of storage.
*
*  As of 02/17/13, the process of converting the AffData from longs
*  to si32s was initiated.  CDAT.pAffD32 and RP->lAffD32 were added.
*  We now have 32-bit items in pAffD32 and 64-bit in pAffD64.  */

   struct CELLTYPE *cdlayr;/* Ptr to current CELLTYPE */
   /* pconnd points to a shared work area allocated in d3nset() to
   *  hold a number of CONNDATA blocks needed for the conntype
   *  with largest nc.  This is used for c(ij) averaging and DETAIL
   *  PRINT=Y or T. */
   struct CONNDATA *pconnd;/* Ptr to amplification data */
   struct AVGCDATA *pavgcd;/* Ptr to sums for Cij averaging */
   struct GPSTAT   *pgstat;/* Ptr to all KRPGP|KRPMG stats */
   struct AUTSSL   *passl0;/* Ptr to autoscale sorted si list base */
#ifdef PAR
   /* phlsi array is used to find high and low responses and numbers of
   *  hits across nodes for all celltypes with stats plus nCdist HILOSI
   *  blocks for celltypes w/KSTMG|KSTGP stats, repeated within each
   *  celltype in groups of nhlssv trials to save having to do a node
   *  collect on every trial (or cycle if doing cycle stats).  */
   struct HILOSI   *phlsi; /* Ptr to high/low response storage */
#endif
   byte *passl;            /* Ptr to current place in passl list */
   si32 (*pdecay[2])(struct DECAYDEF *, si32 *, si32 *, si32);
   /* pdpers holds locations of decay persistence data for the two
   *  modes (persisting and changeable).  These are established in
   *  d3nset and may be used in d3resetn to zero them globally (not
   *  yet implemented).  */
   si32 *pdpers[NDCM];     /* Ptrs to decay persistence */
   /* pmarks points to space for stimulus marking, pmarkh for copies
   *  one trial old for KSTHR.  Each celltype has room for its lmm
   *  bytes at offsets il->iMarks, il->iMarkh.  */
   byte *pmarks;           /* Ptr to space for stimulus marking */
   byte *pmarkh;           /* Ptr to space for history one back */
   byte *pmbits;           /* Ptr to modality marks in pbcst */
   long *povec;            /* Ptr to overflow counters (e64set arg) */
   /*  psws points to a shared work area allocated in d3nset() to hold
   *   the maximum needed for any use that is confined to one function
   *   call (currently d3rstr, d3go cijavg, reset tables, KCTP=B box
   *   limits, IJplot, d3dprt, d3gfsvct, d3autsc, d3stat, c(ij)
   *   dists), UPGM cameras, tvplot.  */
   void *psws;             /* Ptr to shared work space */
   /* praffs is for items that are indexed by connection type number
   *  and saved for statistics when s(i) is known (allocated in d3nset
   *  for max space need over celltypes):  raw then final afference
   *  sums (nct,S20/27) and Sj sum accumulators for rbar (nct, S7/14),
   *  then single items and phase dists that can be reused for each
   *  connection type:  [rs]ax[pn] (S23/30, S20/27 after scaling).  */
#define NNCTS  3           /* Number of nct blocks in praffs */
   si64 *praffs;           /* Ptr to raw afference sums, etc. */
   si64 *pfaffs;           /* Ptr to final aff sums = praffs+nct */
   /* Following data are cleared on entry to each cell */
   si32 *pAffD32;          /* Ptr to 32-bit items in AffData */
   si64 *pAffD64;          /* Ptr to 64-bit items in AffData */
   si64 *pSpecSums;        /* Ptr to, in order, specific, self, and
                           *  final voltage-dependent sums */
   si64 *pGeomSums;        /* Ptr to geometric connection sums */
   si64 *pModulSums;       /* Ptr to modulatory connection sums */
   si64 *pCondSums;        /* Ptr to conductance sums */
   si64 *pTotSums;         /* Ptr to totals of above sums */
   si64 *pSqSums;          /* Ptr to totals of squared inhib sums */
   si64 *pShSums;          /* Ptr to totals of shunting inhib sums */
   si64 *pCellResp;        /* Ptr to final consolidated cell response */

   ui64 lnsetindv;         /* Length of high-water allocation (bytes) */
   size_t ldpers[NDCM];    /* Lengths of pdpers arrays (bytes, !PAR0) */
   ui32 lSpecSums;         /* Length of SpecSums */
   ui32 lGeomSums;         /* Length of GeomSums */
   ui32 lModulSums;        /* Length of ModulSums */
   ui32 lCondSums;         /* Length of CondSums */
   ui32 lTotSums;          /* Length of TotSums, including CellResp */
   ui32 ngstat;            /* Total number of all GPSTATs */
   si32 old_siS7;          /* si(t-1) (mV S7) */
   si32 rspmax,rspmin;     /* Max, min 16-bit response */
#ifdef PAR
   int  ihlsi;             /* Index of trial/cycle saved in qhlsi */
#endif
   ui16 curr_cyc1;         /* Current cycle (one-based) */
   /* Consolidation tables:  These tables are used by sumaffs() routine
   *  in d3go and by d3assv() routine in d3autsc to control which terms
   *  get included in connection sums.  Total size of 2 ASxTables is
   *  MaxASDH, so let AsHypTable run down from top of AsDepTable.  */
   short ConsTables[MaxCons];
   short ASDepTable[MaxASDH];
#define ASHypTable ASDepTable+MaxASDH-1
   byte  kasop[NASOP];     /* Autoscale codes from AS_xxx set for the
                           *  4 cases:  EXCIT nonself, self,
                           *            INHIB nonself, self */
   byte  qasov;            /* USG_QIR when autoscale is on, else 0 */
   };

/* Connection data used for amplification and detail print.
*  These are reused for each cell and one is allocated for each
*  connection in the max number of connections per cell (mxnce).
*  curr_cij, curr_cij0 are values before rounding and truncating.
*  N.B.  vmvt is redundant copy of value in ix->cnwk.go.vmvt.  */
struct CONNDATA {
   si32 old_cij;           /* Cij before amplification (S31) */
   si32 curr_cij;          /* Current (amplified) Cij (S16) */
   si32 new_cij;           /* Cij after adjustment (S31) */
   si32 old_cij0;          /* Cij0 before any change (S31) */
   si32 curr_cij0;         /* Current (modified) Cij0 (S16) */
   si32 new_cij0;          /* Cij0 after adjustment (S31) */
   si32 simmti;            /* (Si - mti) (mV S7) */
   si32 sjmmtj;            /* (Sj - mtj) (mV S7/14) */
   si32 mjmmtj;            /* (Mj - mtj) (mV S7/14) */
   si32 vmvt;              /* (Vk - mtk) (FBvl=8) */
   si32 wayscl;            /* wayset[kamj] (S10) */
   si32 delCij;            /* Total Cij change incl. decay (S15) */
   si32 Cijprod;           /* Hebbian product (S16) */
   si32 Cijterm;           /* Final change in Cij (S16) */
   si32 narb;              /* 1 or number of Cij in average */
   };

/* Data used for Cij averaging.  The field sum_cij is si64 to avoid
*  possible overflow with old method of rescaling Cij to S16 for sums.
*  AVGCDATA is indexed by subarbor and allocated for the max number of
*  subarbors in any cell type (mxnsarb).  */
struct AVGCDATA {
   si64 sum_cij;           /* Sum(Cij) for subarbor averaging */
   si32 num_cij;           /* Number of Cij in average */
   };
