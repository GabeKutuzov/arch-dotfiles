/* (c) Copyright 1994-2012, Neurosciences Research Foundation, Inc. */
/* $Id: celldata.h 52 2012-06-01 19:54:05Z  $ */
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
*  user-defined routines.  The one actual instance of each structure   *
*  occurs in d3go() and is static.  Items having event flags (noise,   *
*  probeval) are valid only when the corresponding flag is set. Data   *
*  in these structures is NOT broadcast at Group III time, therefore,  *
*  items that need to be preserved across broadcasts can go here.      *
*                                                                      *
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
*  Rev, 05/26/12, GNR - Add pAffD64                                    *
***********************************************************************/

/*=====================================================================*
*                              WARNING!                                *
*=====================================================================*/
/* The CELLDATA structure down to 'flags' and afferent distributions
*  it points to are transmitted from comp nodes to host for detailed
*  print by special ad hoc code in d3pdpz() and d3pdpn().  This code
*  must be modified if the current setup is changed from that given in
*  the comments below.  See comments in d3pdpn.c for further info.  */

/* Definition of 'event' flag bits: */
#define REFRAC     0x01    /* Cell in absolute refractory period */
#define DECAYED    0x02    /* Persistence term is valid */
#define PROBED     0x08    /* Probe current was applied */
#define SPIKED     0x10    /* Cell made a spike */
#define UPDTDST    0x80    /* Update dst after depression calc */

/* Definition of 'flags' flag bits: */
/* Currently none used */

/* Define application modes of connections (bits in il->kaff).
*  N.B.  Some code in d3go requires that these be consecutive bits. */
#define AFF_Excit    PSP_POS
#define AFF_Hyper    PSP_NEG
#define AFF_Square      4
#define AFF_Shunt       8
#define NORM_SUMS       4  /* Bit for Excit+Hyper in SGM_VoltDep */
#define SCOND_SUMS      4  /* Bit for synapses that activate conds */
#define AFF_ALL4       15  /* All of the above bits */

/* Define generic types of connections (shifts of AFF bits)  */
#define SGM_Total       0  /* Must be zero--see d3oafs */
#define SGM_CellResp    4  /* Only the AFF_Excit slot is used */
#define SGM_NoiseMod    5  /* Noise Mod Excit, Inhib, or Squared */
#define SGM_Specific    8  /* Holds normals only if any VoltDeps */
#define SGM_VoltDep    12
#define SGM_Self       16  /* Only AFF_Excit,AFF_Hyper slots used */
#define SGM_Geometric  20
#define SGM_Modulatory 24
#define SGM_Conduct    28  /* AFF_Excit, AFF_Inhib, or SCOND_SUMS */

/* Definitions needed to set up consolidation tables */
#define MaxCons        36  /* Max terms in cons tables = 21 raw
                           *  items + 4 in finals + 10 end marks */
#define EndConsTbl     -1  /* Signal at end of consolidation table */
#define EndAllConsTbl  -2  /* Signal at end of all consol. tables */

/* Definitions used in transmitting detail print info to host */
#ifdef PAR
#define END_OF_GROUP LONG_MAX  /* End of data group delimiter */
#define lnndat ((char *)&(((struct CELLDATA *)0)->cdlayr) - (char *)0)
#endif

struct CELLDATA {
/* The following fields must be transmitted with swapping: */
   ui32 cdcell;            /* Number of current cell */
   ui32 groupn;            /* Number of current group */
   ui32 groupx;            /* X coord of current group */
   ui32 groupy;            /* Y coord of current group */
   si32 depr;              /* Depression(t) (S8) */
   si32 dst;               /* Increment in spike threshold (mV S20) */
   si32 izhu;              /* Izhikevich cell 'u' variable (mV S20) */
#define IZ_SPIKED 1           /* Cell spiked in previous cycle */
   si32 i3vr;              /* Izhikevich 2003 Vrest (mV S20) */
   si32 izha,izhb;         /* Izhikevich a,b (when variable) */
   si32 izhc,izhd;         /* Izhikevich c,d (when variable) */
   si32 noise;             /* Noise (mV S20) */
   si32 nmodval;           /* (nfr-noise modulation value) (S24) */
   si32 pers;              /* Persistence (mV S20) */
   si32 probeval;          /* Probe current (mV S20) */
   si32 sbar;              /* Time avg cell response (mV S7) */
   si32 qbar;              /* Slower time average Si (mV S1) */
   si32 old_si;            /* Si(t-1) (mV S20) */
   si32 new_si;            /* Si( t ) (mV S20) */
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
*  N.B.  pAffData, etc. are used to store raw and processed sums as
*  well as persistence data for detail print, always in the order
*  raw, persistence, processed, first the excitatory set, then the
*  inhibitory set if both are present.  Space is allocated and data
*  are stored regardless of whether or not Omit-Type-Detail has been
*  specified, so values are available for statistics or other uses
*  and 'if' statements are not needed at points of storage.
*
*  psws points to a shared work area allocated in d3nset() to hold the
*  maximum needed for any use that is confined to one subroutine call
*  (currently d3go cijavg, reset tables, KCTP=B box limits, IJplot
*  c(ij) averages, falloff table printing, dprt line buffers,
*  d3autsc parallel merge).  */

   struct CELLTYPE *cdlayr;/* Ptr to current CELLTYPE */
   struct GPSTAT   *pgstat;/* Ptr to all KRPGP|KRPMG stats */
   void    *psws;          /* Ptr to shared work space */
   short   *pdelcij;       /* Space for delta(Cij) detail print */
   short   *pglway;        /* Permanent space for amp scales */
   si32    *prgtht;        /* Space for finding si autoscale level */
   byte    *tmark0;        /* Temp space for modality markers */
   /* Following data are indexed by connection type number */
   si64 *rsumaij;          /* Ptr to raw afferent sums (S23) */
   si32 *psumaij;          /* Ptr to processed afferent sums (S20) */
   /* Following data are cleared on entry to each cell */
   si64 *pAffD64;          /* Ptr to 64-bit items in AffData */
   long *pAffData;         /* Ptr to head of all the following data */
   long *pNormSums;        /* Ptr to control sums for volt-dep conns. */
   long *pSpecSums;        /* Ptr to, in order, specific, self, and
                           *  final voltage-dependent sums */
   long *pGeomSums;        /* Ptr to geometric connection sums */
   long *pModulSums;       /* Ptr to modulatory connection sums */
   long *pCondSums;        /* Ptr to conductance sums */
   long *pTotSums;         /* Ptr to totals of above sums */
   long *pSqSums;          /* Ptr to totals of squared inhib sums */
   long *pShSums;          /* Ptr to totals of shunting inhib sums */
   long *pCellResp;        /* Ptr to final consolidated cell response */
   long lnsetindv;         /* Length of high-water allocation (bytes) */
   long lSpecSums;         /* Length of SpecSums */
   long lGeomSums;         /* Length of GeomSums */
   long lModulSums;        /* Length of ModulSums */
   long lCondSums;         /* Length of CondSums */
   long lTotSums;          /* Length of TotSums, including CellResp */
   long old_siS7;          /* si(t-1) (mV S7) */
   ui32 myngtht;           /* Number in prgtht on this node */
   int  ngstat;            /* Total number of all GPSTATs */
   ui16 curr_cyc1;         /* Current cycle (one-based) */
   /* Consolidation tables:  These tables are used by sumaffs() routine
   *  in d3go to control which terms get included in connection sums */
   short ConsTables[MaxCons];
   };

struct CONNDATA {
   long old_cij;           /* Cij before amplification (S31) */
   long curr_cij;          /* Current (amplified) Cij (S16) */
   long new_cij;           /* Cij after adjustment (S31) */
   long old_cij0;          /* Cij0 before any change (S31) */
   long curr_cij0;         /* Current (modified) Cij0 (S16) */
   long new_cij0;          /* Cij0 after adjustment (S31), except
                           *  (S16) when adding Cij for avg */
   long narb;              /* Number of subarbors for this conn */
   };

