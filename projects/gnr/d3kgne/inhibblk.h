/* (c) Copyright 1991-2012, Neurosciences Research Foundation, Inc. */
/* $Id: inhibblk.h 52 2012-06-01 19:54:05Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                        INHIBBLK Header File                          *
*                                                                      *
*  This file defines the geometric connection data block used with     *
*     both parallel and serial versions of CNS.  This file refers to   *
*     values set in d3global.h.  All references to ngx,ngy refer to    *
*     the *source* layer.                                              *
*                                                                      *
*  Rev, 02/23/90, GNR - Merge serial and parallel versions             *
*  Rev, 05/20/91,  MC - Add PHASE related variables and defines        *
*  V5E, 07/12/92, GNR - Changes needed to reenable serial version      *
*  Rev, 10/23/92, ABP - Move struct XYNORMDEF to comply with nxdr      *
*  V6D, 01/04/93, GNR - Add OPT=V (self-avoidance)                     *
*  V7A, 05/01/94, GNR - Add delay                                      *
*  V8A, 09/02/95, GNR - Add decay, true-distance falloff, oas, pdist,  *
*                       rnsums, remove ihcpc,ihcpr,etc.                *
*  Rev, 12/29/97, GNR - Add oidist, gpers                              *
*  V8C, 02/27/03, GNR - Rescale for millivolts, always apply ihsf      *
*  ==>, 11/03/07, GNR - Last mod before committing to svn repository   *
*  Rev, 04/21/08, GNR - Add ssck (sum sign check) mechanism            *
*  Rev, 05/03/08, GNR - IBOPT=K and COMPAT=K for knee response         *
*  V8E, 05/10/09, GNR - Add gjrev                                      *
*  V8G, 08/17/10, GNR - Add asbeta                                     *
*  V8H, 03/31/11, GNR - Add gdefer                                     *
*  Rev, 04/09/12, GNR - Add IBOPFS, IBOPGS, GBCM                       *
***********************************************************************/

/* Macro to determine number of cells for which decay space is needed */
#ifdef PAR                    /* Parallel */
#define NDecayCells(il, ib, cpn) \
   ((ib->ibopt & IBOPSA) ? cpn : ((ib->l1xy-1)/il->nodes+2))
#else                         /* Serial */
#define NDecayCells(il, ib, cpn) \
   ((ib->ibopt & IBOPSA) ? cpn : ib->l1xy)
#endif

struct XYNORMDEF {            /* Entry in a normalization table */
   short x;
   short y;
   };

struct INHIBBAND {            /* One of these per band */
   si32 beta;                 /* Band multiplier (S20) */
   si32 asbeta;               /* Scaled (used) beta (S20) */
   long mxib;                 /* Maximum inhibition (mV S20/27) */
#ifdef PAR
   long bandsum;              /* Sum for this band (mV S23-SF) */
#else
   long *bandsum;             /* Pointer to array of bandsums */
#endif
   };

struct INHIBBLK {
   struct INHIBBLK *pib;      /* Ptr to next inhibition block */
   struct CELLTYPE *pisrc;    /* Ptr to layer block for source cells */
   struct INHIBBAND *inhbnd;  /* Ptr to array of inhibition bands */
   long *falloff;             /* Ptr to falloff table (S28) */
   long *ieveff;              /* Ptr to excit veff array (S20) */
   long *iiveff;              /* Ptr to inhib veff array (S20) */
   float hlfrad;              /* Half-radius of falloff */
   si32  gbcm;                /* Beta per-cycle multiplier (S24) */
   rlnm  gsrcnm;              /* Source rep-layer name */
   short igt;                 /* Number of this connection type */
   short gjrev;               /* Sj reversal potential (mV S7/14) */
   short itt;                 /* Inhibitory threshold (mV S7/14) (was
                              *  'it'--renamed to avoid confusion) */
   short ittlo;               /* Negative itt (mV S7/14) */
   short subitt,subnitt;      /* Subtracted thresholds if IBOPKR */
   short ngb;                 /* Inhib band width (num groups/band) */
   short nib;                 /* Number of inhibitory bands */
   ui16  ogafs;               /* Offset into geometric affs data  */
   ui16  ogsei[NEXIN];        /* Offsets to excit/inhib AffSums */
   ui16  oidist;              /* Offset to inhib dist in pctddst */
   ui16  ibopt;               /* User entered options */
/* User-entered options in ibopt (and K,Q,S,F,O in mopt): */
#define IBOPTX       0x01     /* X - Extended field inhibition */
#define IBOPGS       0x02     /* G - Square group sums */
#define IBOPSA       0x04     /* V - Self-avoidance */
#define IBOPKR       0x08     /* K - Knee response (itt subtracted) */
#define IBOPTQ       0x10     /* Q - Squared inhibition */
#define IBOPTS       0x20     /* S - Shunting inhibition */
#define IBOPFS       0x40     /* F - Reset & fast start on new trial */
#define IBOPOR       0x80     /* O - Override restore */
#define IBOPTT     0x0100     /* T - Tabulate falloff curves */
/* Derived option codes in ibopt: */
#define IBOPFO     0x0200     /* Falloff option requested */
#define IBOEFV     0x4000     /* IBOPSA special calc required */
   ui16 gcdefer;              /* Cycles to defer GCONN calc */
   ui16 gdefer;               /* Cycles to defer GCONN use */
   ui16 gfscyc;               /* Cycle to perform IBOPFS reset */
   ui16 lafd;                 /* Length of AffData allocation */
   byte gssck;                /* Geometric sum sign check */
   byte ihsf;                 /* 1 + log2(cells in largest band-1) */
   byte kbc;                  /* Boundary condition option--
                              *  values defined in d3global.h */
   byte kfoff;                /* Falloff option */
/* Values taken on by kfoff: */
#define FOFF1D       0        /* 1/d */
#define FOFF1DSQ     1        /* 1/dsquared */
#define FOFFEMD      2        /* exp(-d) */
#define FOFFEMDSQ    3        /* exp(-dsquared) */
#define FOFFRMETRIC  4        /* d-->r = max + (1/2)min coord */
#define FOFFSMETRIC  8        /* d-->s = max(x,y) coord */
#define NFOFFOPTS   12        /* Number of falloff options */
   struct DECAYDEF GDCY;      /* GCONN decay parameter block */
   long border;               /* Noise border value (mV S23-SF) */
   short ihdelay;             /* Inhibitory delay */
   short l1n1;                /* = 1*(nr-1) */
   long l1area;               /* = (2*nr-1)*(2*nr-1) */
   long l1x;                  /* = 1*ngx */
   long l1y;                  /* = 1*ngy */
   long l1xn2;                /* = 1*(ngx+2*(nr-1)) */
   long l1yn2;                /* = 1*(ngy+2*(nr-1)) */
   long l1xy;                 /* = ngx*ngy */
#ifdef PAR
   long xtotal;               /* All groups total (OPT=X) (S23-SF) */
#else
   long *hstrip;              /* Ptr to horizontal summation strip */
   long *vstrip;              /* Ptr to vertical summation strip */
   long *boxsum;              /* Ptr to boxsum array */
   long *boxsum00;            /* =a(boxsum)+(nr-1)*(ngx+2(nr-1)+1) */
   long *boxsumbb;            /* =a(boxsum)+(ngy+nr-1)*(ngx+2(nr-1)) */
   struct XYNORMDEF *xynorm;  /* Ptr to normalization table */
   long l1n1x;                /* = 1*(nr-1)*ngx */
   long l1n1xn2;              /* = 1*(nr-1)*(ngx+2*(nr-1)) */
   long l1yxn2;               /* = 1*ngy*(ngx+2*(nr-1)) */
   long l1xn2yn2;             /* = 1*(ngx+2*(nr-1))*(ngy+2*(nr-1)) */
#endif
   /* Items needed to implement self-avoidance (OPT=V) */
   long rnsums[NEXIN];        /* Outer ring excit,inhib sums (S20) */
   long r0sum;                /* Ring 0 group sum (S20) */
   si32 r0scale;              /* Ring 0 beta (times falloff) (S20) */
   };

/* Typedef for routines used to calculate falloff.  Return is S28 */
typedef long (*FoffFn)(struct INHIBBLK *, long, long);
