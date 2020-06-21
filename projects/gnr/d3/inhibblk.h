/* (c) Copyright 1990-2016, The Rockefeller University *21115* */
/* $Id: inhibblk.h 70 2017-01-16 19:27:55Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                        INHIBBLK Header File                          *
*                                                                      *
*  This file defines the geometric connection data block used with     *
*     both parallel and serial versions of CNS.  This file refers to   *
*     values set in d3global.h.  All references to ngx,ngy refer to    *
*     the *source* layer.                                              *
*                                                                      *
*  N.B.  Documented max values of ngx,ngy,nib,ngb are 2^16-1, so sums  *
*     of these values can be assumed to fit in 32 bits.  Products are  *
*     typed ui32, but products with added terms, e.g. l1xn2yn2, need   *
*     to be tested for overflow when computed and stored.              *
*                                                                      *
*  N.B.  Dynamic allocation conditions are as follows:                 *
*     Original d3inhb serial algorithm: boxsum, [hv]strip: reused per  *
*        inhibblk, reserve for largest total size.  bandsums: reused   *
*        per celltype, reserve for largest sum over inhibblks for any  *
*        cell type.  xynorm: each inhibblk with BC=NORM gets its own.  *
*     Original d3gcon parallel algorithm: boxsum, [hv]strip, xynorm:   *
*        not used.  bandsums: one per band, stored in INHIBBANDs.      *
*     New universal algorithm: same as original d3inhb.                *
*                                                                      *
************************************************************************
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
*  Rev, 12/24/12, GNR - New overflow checking                          *
*  Rev, 04/20/13, GNR - Go to 64-bit sums, eliminate ihsf, falloff     *
*  R65, 01/11/16, GNR - Merge parallel into serial code                *
***********************************************************************/

/* Macro to determine number of cells for which decay space is needed */
#define NDecayCells(il, ib, cpn) \
   ((ib->ibopt & IBOPSA) ? cpn : il->higrp - il->logrp)

#define MXIBRNGS  23169       /* Max number of rings for si32 code */

struct XYNORMDEF {            /* Entry in a normalization table */
   short x;
   short y;
   };

struct INHIBBAND {            /* One of these per band */
   si64 *bandsum;             /* Pointer to array of bandsums */
   si32 barea;                /* Band area times nel */
   si32 beta;                 /* Band multiplier (S20) */
   si32 asbeta;               /* Autoscaled beta (S20) */
   si32 mxib;                 /* Maximum inhibition (mV S20/27) */
   };

struct INHIBBLK {
   struct INHIBBLK *pib;      /* Ptr to next inhibition block */
   struct CELLTYPE *pisrc;    /* Ptr to layer block for source cells */
   struct INHIBBAND *inhbnd;  /* Ptr to array of inhibition bands */
   struct XYNORMDEF *xynorm;  /* Ptr to normalization table */
   size_t *prroff;            /* Ptr to round-region offset table */
   struct DECAYDEF GDCY;      /* GCONN decay parameter block */
   /* Items needed to implement self-avoidance (OPT=V) */
   si64 border;               /* Noise border value (mV S20) * nel */
   si64 rnsums[NEXIN];        /* Outer ring excit,inhib sums (S20) */
   si64 r0sum;                /* Ring 0 group sum (S20) */
   size_t ndcya;              /* Number allocs for decay (d3nset) */
   si32 gbcm;                 /* Beta per-cycle multiplier (S24) */
   si32 gcarea;               /* = (2*nr-1)*(2*nr-1) */
   /* In the l1--- variables, 'mgy' refers to the number of rows of
   *  groups on the current node (parallel), mgy = ngy (serial).  */
   ui32 l1n1x;                /* = (nr-1)*ngx */
   ui32 l1xn2;                /* = (ngx+2*(nr-1)) */
   ui32 l1n1xn2;              /* = (nr-1)*(ngx+2*(nr-1)) */
   ui32 l1yn2;                /* = (mgy+2*(nr-1)) */
   ui32 l1xyn2;               /* = ngx*(mgy+2*(nr-1)) */
   ui32 l1yxn2;               /* = mgy*(ngx+2*(nr-1)) */
   ui32 l1xn2yn2;             /* = (ngx+2*(nr-1))*(mgy+2*(nr-1)) */
   ui32 nrro;                 /* Num entries in prroff table */
   si32 r0scale;              /* Ring 0 beta (S20) */
   rlnm gsrcnm;               /* Source rep-layer name */
   ui16 gcdefer;              /* Cycles to defer GCONN calc */
   ui16 gdefer;               /* Cycles to defer GCONN use */
   ui16 gfscyc;               /* Cycle to perform IBOPFS reset */
   si16 gjrev;                /* Sj reversal potential (mV S7/14) */
   ui16 ibopt;                /* User entered options */
/* User-entered options in ibopt (and M,K,Q,S,F,O in mopt): */
#define IBOPTX       0x01     /* X - Extended field inhibition */
#define IBOPRS       0x02     /* R - Round surround region */
#define IBOPMX       0x04     /* M - Use max instead of sums */
#define IBOPKR       0x08     /* K - Knee response (itt subtracted) */
#define IBOPTQ       0x10     /* Q - Squared total inhibition */
#define IBOPTS       0x20     /* S - Shunting inhibition */
#define IBOPFS       0x40     /* F - Reset & fast start on new trial */
#define IBOPOR       0x80     /* O - Override restore */
#define IBOPSA     0x0200     /* V - Self-avoidance */
#define IBOPCS     0x0400     /* C - Square individual cells in sums */
/* Derived option codes in ibopt: */
#define IBOEFV     0x4000     /* IBOPSA special calc required */
   si16 igt;                  /* Number of this connection type */
   ui16 ihdelay;              /* Inhibitory delay */
   ui16 iovec;                /* Offset to overflow count */
   si16 itt;                  /* Inhibitory threshold (mV S7/14) (was
                              *  'it'--renamed to avoid confusion) */
   si16 ittlo;                /* Negative itt (mV S7/14) */
   si16 effitt,effnitt;       /* Effective itt-1, ittlo+1 */
   si16 subitt,subnitt;       /* Subtracted thresholds if IBOPKR */
   ui16 lafd;                 /* Length of AffD32 allocation */
   ui16 l1n1;                 /* = (nr-1) = (ngb*(nib-1)) */
   ui16 ngb;                  /* Inhib band width (num groups/band) */
   ui16 nib;                  /* Number of inhibitory bands */
   ui16 ogafs;                /* Offset into geometric affs data  */
   ui16 ogsei[NEXIN];         /* Offsets to excit/inhib AffSums */
   ui16 oidist;               /* Offset to inhib dist in pctddst */
   byte gpspov;               /* Autoscale overrides */
   byte gssck;                /* Geometric sum sign check */
   byte jself;                /* 1 if self input */
   byte kbc;                  /* Boundary condition option--
                              *  values defined in d3global.h */
   };
