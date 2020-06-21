/* (c) Copyright 1991-2011 Neurosciences Research Foundation, Inc. */
/* $Id: rpdef0.h 44 2011-03-04 22:36:14Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                        RPDEF0 Header File                            *
*                                                                      *
*          (Note: This file references values set in D3GLOBAL)         *
*                                                                      *
*     This header file isolates global CNS variables that are needed   *
*  only on host node in the parallel version, to save memory space on  *
*  comp nodes.                                                         *
*                                                                      *
*  Rev, 04/22/91, Initial version                                      *
*  V5A, 05/20/91, GNR - Add seqno for packet io                        *
*  V5C, 12/08/91, GNR - Add cumulative broadcast counts, d3g3pp stuff  *
*  Rev, 03/07/92, GNR - Remove GRAFDATA cell list to rpdef             *
*  Rev, 04/08/92, ABP - Add pointers to VDRs in RPDEF0                 *
*  V5E, 08/04/92, GNR - Add support for XG terminal                    *
*  Rev, 08/25/92, GNR - Move cpr,cpt from rpdef                        *
*  Rev, 10/14/92, GNR - Move kux,kuy from rpdef                        *
*  V6A, 11/16/92, ABP - Move fields parm1, pwdw1, ps0, pgpno, penvl    *
*                       from RPDEF to RPDEF0                           *
*  Rev, 02/27/93, GNR - Remove snapshot support, replaced by metafile  *
*  Rev, 04/26/94, GNR - Add global storage totals, user-defined senses *
*  V7B, 07/19/94, GNR - Add OUTCTL structures for print, plot, replay  *
*  V8A, 04/22/95, GNR - Move pgpno,penvl to modality,                  *
*                       delete cumntu, cumnku, add decay defaults      *
*  Rev, 11/27/96, GNR - Remove support for OVERLAY and NCUBE graphics  *
*  Rev, 08/25/97, GNR - Remove GRAFDATA support to allocated SDSELECT  *
*  V8B, 12/10/00, GNR - New memory management routines, trial timers   *
*  V8C, 02/22/03, GNR - Cell responses in millivolts                   *
*  V8D, 01/30/04, GNR - Add conductances and ions                      *
*  Rev, 03/15/07, GNR - Add support for RESET EVENT codes              *
*  Rev, 10/02/07, GNR - Add PSCALE, TANH, tiered threshold defaults    *
*  ==>, 12/28/07, GNR - Last mod before committing to svn repository   *
*  Rev, 03/22/08, GNR - Implement global and celltype conntype dflts   *
*  Rev, 04/21/08, GNR - Add ssck (sum sign check) mechanism, vcpt      *
*  Rev, 05/21/08, GNR - Add deferred conntype thresholds               *
*  V8E, 01/13/09, GNR - Add Izhikevich neurons, global phase defaults  *
*  V8F, 02/21/10, GNR - Remove PLATFORM and VDR support                *
*  Rev, 04/24/10, GNR - Add KERNEL and PREPROC support                 *
*  V8G, 08/14/10, GNR - Add AUTOSCALE                                  *
*  V8H, 05/25/11, GNR - New scheme to control reset actions            *
***********************************************************************/

#ifndef CHTYPE
#define CHTYPE void
#endif
#ifndef CMTYPE
#define CMTYPE void
#endif
#ifndef SDSTYPE
#define SDSTYPE void
#endif
#ifndef PLBTYPE
#define PLBTYPE void
#endif
#ifndef PLNTYPE
#define PLNTYPE void
#endif
#ifndef TVTYPE
#define TVTYPE void
#endif
#ifndef KNTYPE
#define KNTYPE void
#endif

/* RP0 structure */

struct RPDEF0 {

/* Pointers */

   ENVIRONMENT     *Eptr;     /* Ptr to environment */
   struct CONDUCT  *pcndg;    /* Ptr to first global conductance */
   struct CONDSET  *pcnds;    /* Ptr to first conductance set */
   struct DPDATA   *pdpd1;    /* Ptr to first detailed print data */
   struct EFFARB   *pearb1;   /* Ptr to first EFFARB block */
   struct IONTYPE  *piong;    /* Ptr to first global ion info */
   struct RFdef    *psnf;     /* Ptr to current savenet file */
   struct SFHMATCH *psfhm;    /* Ptr to restore/rename info */
   struct TRTIMER  *ptrt1;    /* Ptr to first trial timer */
   struct VALEVENT *pvev1;    /* Ptr to first value event block */
   struct VBDEF    *pvblke;   /* Ptr to first env value block */
   struct VCELL    *pvc1;     /* Ptr to first VCELL (sensor) block */
   struct VCELL    **plvc;    /* Ptr to last VCELL--init to pvc1 */
   CHTYPE          *pclh1;    /* Ptr to first cell list header */
   CMTYPE          *pfijpl;   /* Ptr to first IJPL plot def */
   KNTYPE          *pfkn;     /* Ptr to first preprocessing kernel */
   PLBTYPE         *pplb1;    /* Ptr to first plot label block */
   PLBTYPE         **plplb;   /* Ptr to last PLBDEF--init to pplb1 */
   PLNTYPE         *ppln1;    /* Ptr to first plot line block */
   PLNTYPE         **plpln;   /* Ptr to last PLNDEF--init to ppln1 */
   SDSTYPE         *pgds;     /* Ptr to SIMDATA selection data */
   TVTYPE          **pltv;    /* Ptr to last TVDEF--init to pftv */
   float           *pjnwd;    /* Ptr to changes array */
   ARM     **pparm,*parm1;    /* Ptr to first arm */
   WINDOW  **ppwdw,*pwdw1;    /* Ptr to first window */
   byte            *pipimin;  /* Ptr to images for preprocessing */
   si32            *ptvrsh;   /* Ptr to camera rescale histogram */
   char            *pcsnfn;   /* Ptr to created SAVENET file name */
#ifdef D1
   d1s_type        *ps0;      /* Ptr to Node 0 cell array */
#endif
#ifdef BBDS
   struct VCELL    *pbbds1;   /* Ptr to first external VCELL block */
#endif

/* Space needed for memory allocation, printed by d3echo.
*  (Non-zero values of cumnvc, etc. cause the respective
*  input routines to be called.  */

   long cumcnd;               /* Cumulative size of conductance info */
   long cumnaw;               /* Cumulative size of arm/window info */
   long cumntv;               /* Cumulative size of TV images */
   long cumntvi;              /* Images for preprocessor input only */
   long cumnvbc,cumnvfc;      /* Cumulative size of byte,float senses */
   long cumval;               /* Size of all VDTDEF blocks */
#ifdef D1
   long cumnd1;               /* Cumulative size of D1 input info
                              *  (includes id1gn unless OPT=1 given) */
   long od1in;                /* Offset in pbcst to D1 input data */
#endif

/* Memory allocations estimated globally in d3allo for d3echo,
*  then recomputed locally for each node in d3nset, d1nset. */

   long lgall;                /* Bytes of group-level ion data */
   long lrall;                /* Bytes of repertoire space needed */
   long lsall;                /* Length of s(i) data for all reps */
#ifdef PAR
   long ls0all;               /* Length of s(i) data on host node */
#endif
   long ldecay;               /* Space for decaying connections */
   long ldstat;               /* Length of deferred statistics  */
   long liall;                /* Length of all GCONN space needed */
#ifdef D1
   long ld1sall;              /* Length of scores for all d1 reps */
   long ld1rall;              /* Length of all d1 repertoires, not
                              *  including scores, max for 1 node */
#endif
   long xtrasize;             /* Largest D1 size or other extras */

/* Miscellaneous parameters and derived quantities */

   struct CNDFLT   GCnD;      /* Global conntype defaults */
   struct CNDFLT   LCnD;      /* Local conntype defaults */
   struct CTDFLT   GCtD;      /* Global celltype defaults */
   struct AUTSCL   GASD;      /* GLobal autoscale defaults */
   struct BREGDEF  GBGD;      /* Global Brette-Gerstner defaults */
   struct DCYDFLT  GDcD;      /* Global decay defaults */
   struct DEPRDFLT GDpD;      /* Global depression defaults */
   struct IZ03DEF  GIz3D;     /* Global Izhikevich 2003 defaults */
   struct IZ07DEF  GIz7D;     /* Global Izhikevich 2007 defaults */
   struct NDFLT    GNoD;      /* Global noise defaults */
   struct PHASEDEF GPdD;      /* Global phasing defaults */
   struct PPFDATA  GPpfD;     /* Global PPF defaults */
   struct PPFDATA  LPpfD;     /* Local PPF defaults */
   struct RPDFLT   GRpD;      /* Global region defaults */
   struct DEFTHR   dfthr[NCNThr];   /* Deferred thresholds */

   float cdcycut;             /* Conductance decay cutoff */
   float pfac;                /* Overall scale for plots */
   float efplw,efplh;         /* Environment plot width, height */
   float satdcynum;           /* Saturated decay hlfeff numerator */
   float smshrnkdflt;         /* Default for IJPLDEF smshrink */
   float splx,sply;           /* Scale plot x,y coordinates */
   float splw,splh;           /* Scale plot width, height */
#define NVCOLS 2              /* Number of value columns */
   float vplx[NVCOLS];        /* Value plot x,y coordinates */
   float vply[NVCOLS];        /* Second value plot x,y coords */
   float vplw,vplh;           /* Value plot width, height */
   float yvstrt[NVCOLS];      /* Starting y for value bar plots */
   float yvbxht;              /* Box height for value bar plots */

   long ilseed;               /* Random number seed for ilists */
   long ljnwd;                /* Length of joints and windows changes */
   long ntrsi;                /* Reset interval */
   long pktseq;               /* Outgoing packet sequence number */

   long rprecl;               /* Record length of replay file */
   long rprecp;               /* Packing factor for replay file */
   long rpirec;               /* Current record in replay file */

   ui32 trials0;              /* Trials since start of run */
   ui32 trialse;              /* Trials since last "event" */

   int evhosts;               /* OR of hosts providing events */
   int exisn;                 /* Counter of isn,ign range errors */
   int krenvev;               /* Switch for environment event reset */
   int notmaster;             /* TRUE if this card allows seeds */

   short grids;               /* Global (touch, IA, etc.) grids */
   txtid hxterm;              /* Name of X terminal to use */
   short isfhm1[SFHMNTYP];    /* SFHMATCHs by type */

   /* Globals used to keep track of running index value for arms,
   *  windows, or D1's specified by srcnm.lnm = '-'.  */
   short jarm;                /* Index of next arm to use */
   short jwdw;                /* Index of next window to use */
#ifdef D1
   short jd1;                 /* Index of next D1 array to use */
#endif
   short jsnf;                /* Number of created SAVENET file */

   ui16  kplt1,kplt2;         /* Plot options from PLIM, CYCLE cards--
                              *  bits defined in rpdef.h for kplot */
   short lbsnfn;              /* Length of base SAVENET file name */
   short lst;                 /* Length of stitle string */
   short mmdev;               /* Movie device */
   short mmode;               /* Movie mode */
   short mpx;                 /* Max cells printed per line in 1 grp */
   short mxnib;               /* Max value of nib over all gconns */
   ui16  n0flags;             /* Miscellaneous node 0 flags */
#define N0F_MPPFE    0x0001   /* A MASTER PPF card was entered */
#define N0F_CTPPFE   0x0002   /* A CELLTYPE-level PPF was entered */
#define N0F_ANYAMP   0x0010   /* Set if any conntype is amplified */
#define N0F_ANYKCG   0x0020   /* Set if any conntype has KAMCG */
#define N0F_TVRSCL   0x0080   /* Some camera has rescale option */
#define N0F_MKTBLS   0x0100   /* Make activation tables in d3ctbl() */
#define N0F_SPMFTM   0x0200   /* SPMFTIMER option entered */
#define N0F_SVCIJ0   0x0400   /* Save CIJ0 for later reset code C */
#define N0F_MPARME   0x0800   /* Movie mode parameter entered */
#define N0F_DPCTRL   0x1000   /* Detail print under PRINT control */
#define N0F_VTCHSU   0x2000   /* A virtual touch sense is in use */
#define N0F_TSVP     0x4000   /* Tree structure verify print */
   ui16  nautsc;              /* Number of cell types with AUTOSCALE */
   ui16  ncs;                 /* Default number of stimulus classes */
   ui16  nds;                 /* Default number of distinct stimuli */
   short nexp;                /* Number of exposures per movie frame */
   short nextclid;            /* Next internal cell list id */
   short totnlyr;             /* Total number of celltypes */
   short totconn;             /* Total number of specific conntypes */
   short totgcon;             /* Total number of geometric conntypes */
   short totmodu;             /* Total number of modulation types */
   short vcpt;                /* Vcell circle plot threshold (S14) */
#ifdef BBDS
   ui16  vmsgwho;             /* Clients who get version messages */
#endif
#ifdef D1
   short nd1f;                /* Number of DIGINPUT files */
#endif

   char stitle[LSTITL];       /* Series and trial title */
   byte bssel;                /* COMPAT=C or not for bscompat call */
   byte ccgrp;                /* Current control card group */
#define CCGRP1  1                /* These defs allow OR tests */
#define CCGRP2  2
#define CCGRP3  4
   byte kiarr;                /* IA array print control */
#define PIAFRZ      0x01         /* F: Print IA frozen cycles */
#define PIAALL      0x02         /* A: Print IA all cycles */
#define PIASPT      0x04         /* S: Spout = ENV_PRSPT */
#define PIANOI      0x08         /* N: Print with noise = ENV_PRNOI */
   byte ksrctyp;              /* Source type code for getthr */
   byte ndfth;                /* Number of deferred thresholds */
   byte needevnt;             /* Reasons for needing event detector */
#define NEV_CAM     0x01         /* An event camera is in use */
#define NEV_RSET    0x02         /* Event reset is requested */
#define NEV_TIMR    0x04         /* A timer looks at events */
#define NEV_EVTRL   0x08         /* An event trial is requested */
   byte orcdfl;               /* OR of all conductance flags */
   byte rssck;                /* ssck bits for real cell inputs */

/* The following items hold command-line parameters from the start
*  of the run that must be applied after d3grp1() has run.  */

   ui32  ORrfl;               /* Bits to OR to runflags */
   ui32  ANDrfl;              /* Bits to AND to runflags */
   ui16  ORksv;               /* Bits to OR to ksv */
   ui16  ANDksv;              /* Bits to AND to ksv */

/* The following items are used as holding places on the way to
*  being passed to the environment package. */

   float epr;                 /* Environment plot radius */
   int   ept;                 /* Environment plot threshold (S8) */
   short mxobj;               /* Max number of simultaneous objects--
                              *  Obsolete: remove w/old-style replay */
   short kux;                 /* Stimulus pixels used - x */
   short kuy;                 /* Stimulus pixels used - y */
   char ecolgrd[ECOLMAXCH];   /* Environment grid color */
   char ecolstm[ECOLMAXCH];   /* Environment stimulus color */
   char ecolwin[ECOLMAXCH];   /* Environment window color */
   char ecolarm[ECOLMAXCH];   /* Environment arm color */
   stim_type eilo;            /* Value assigned to background pixels */
   stim_type eihi;            /* Value assigned to stimulus pixels */
   };

#ifndef MAIN
extern
#endif
   struct RPDEF0 *RP0;
