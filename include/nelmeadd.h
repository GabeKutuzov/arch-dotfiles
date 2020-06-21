/* (c) Copyright 2005-2009, The Rockefeller University *11116* */
/* $Id: nelmeadd.h 59 2017-01-13 20:34:16Z  $ */
/***********************************************************************
*                           Header File for                            *
*       Nelder-Mead Simplex Optimizer with Simulated Annealing         *
*               (This is the double-precision version)                 *
*                                                                      *
*  ==>, 03/22/08, GNR - Last date before committing to svn repository  *
*  Rev, 09/23/08, GNR - Modify for 64-bit compilations                 *
*  Rev, 08/25/09, GNR - Separate float vs. double versions             *
***********************************************************************/
#include "sysdef.h"

typedef double nmxyd;      /* Type of simplex and solutions */

struct nmhistd {           /* Data returned in history array */
   nmxyd    y;                /* Y value at this iteration */
   nmxyd    ybest;            /* Best y in current simplex */
   nmxyd    yworst;           /* Worst y in current simplex */
   nmxyd    yglmin;           /* Best ever y value */
   int      vupd;             /* Vertex updated */
   int      method;           /* Method used in current interation */
   };

/* Define value user function should return when at a singularity
*  (Near the largest representable double in IEEE arithmetic).  */
#define NM_SING      1.0E308

/* Define option codes */
#define NMKH_LAST    1        /* Return nmhist for last iteration */
#define NMKH_EVRY    2        /* Return nmhist for every iteration */
#define NMKP_LAST    4        /* Print nmhist for last iteration */
#define NMKP_EVRY    8        /* Print nmhist for every iteration */
#define NMKP_DETAIL 16        /* Print shrink & avoidance info */
#define NMKS_THERM  32        /* Shrink around thermalized (default:
                              *  unthermalized) best vertex.  Ignored
                              *  by nmiterna().  */
#define NMKA_MOVE   64        /* Try to move vertex away from
                              *  singularity (ufn returns NM_SING) */

/* Define maximum number of singularity avoidance moves to try
*  before giving up hope (error -2) */
#define NMKA_MXMV   10

/* Define values of statistics returned by nminit and nmiter */
#define NNMST       20        /* Number of available statistics */
#define NMS_NMIAV    0        /* Number of avoidances in nminit */
#define NMS_NMIAVM   1        /* Avoidance moves in nminit */
#define NMS_REFAV    2        /* Avoidances on reflection */
#define NMS_REFACC   3        /* Times reflection was accepted */
#define NMS_REFBEST  4        /* Times reflection was best ever */
#define NMS_EXPAV    5        /* Avoidances on expansion */
#define NMS_EXPACC   6        /* Times expansion was accepted */
#define NMS_EXPBEST  7        /* Times expansion was best ever */
#define NMS_OSCAV    8        /* Avoidance on outside contraction */
#define NMS_OSCACC   9        /* Outside contraction was accepted */
#define NMS_OSCBEST 10        /* Outside contraction was best ever */
#define NMS_INSAV   11        /* Avoidance on inside contraction */
#define NMS_INSACC  12        /* Inside contraction was accepted */
#define NMS_INSBEST 13        /* Inside contraction was best ever */
#define NMS_SHRINK  14        /* Shrinkage was attempted */
#define NMS_SHRAV   15        /* Avoidances in shrink */
#define NMS_SHRAVM  16        /* Avoidance moves in shrink */
#define NMS_SHRBEST 17        /* Shrinkage was best ever */
#define NMS_POINT   18        /* Quit due to shrinkage to a point */
#define NMS_CONVG   19        /* Times convergence was reached */

/* Define codes for kinds of moves */
#define NMKM_SHRINK        0
#define NMKM_REFLECT       1
#define NMKM_EXPAND        2
#define NMKM_CONTRACT_OUT  3
#define NMKM_CONTRACT_IN   4

/* Termination codes */
#define NM_MXITER          0  /* Hit max iterations w/o convergence */
#define NM_CONVERGED       1  /* Converged by ftol test */
#define NM_SHRUNK          2  /* Shrunk to a point by xtol test */
#define NMERR_NOAVOID      3  /* Failed to avoid a singularity */
#define NMERR_ALLSING      4  /* All points became singular */

#ifdef IMNMEAD
/* "Global" data visible only inside the nelmead package */
#define NXVECS       6     /* Number of working x vectors needed */
void nmprtxd(nmxyd *x, int N); /* Error print routine */
struct nmglbld {
   struct nmhistd *phist;  /* Ptr to history array */
   nmxyd (*ufn)(nmxyd *x, void *usrd); /* Ptr to user function */
   nmxyd  *nmasx;          /* Ptr to simplex of N+1 N-dim vertices */
   nmxyd  *nmasy;          /* Ptr to vector of N+1 function values
                           *  at the vertices of x */
   nmxyd  *nmasxb;         /* Ptr to x vector at best y */
   char   *msgid;          /* Ptr to id info for warning messages */
   nmxyd  nmasyb;          /* Best value of y found so far */
   nmxyd  nmasrr;          /* Reflection ratio (normally 1.0) */
   nmxyd  nmasxr;          /* Expansion ratio (normally 2.0) */
   nmxyd  nmascr;          /* Contraction ratio (normally 0.5) */
   nmxyd  nmassr;          /* Shrinkage ratio (normally 0.5) */
   nmxyd  nmasar;          /* Avoidance ratio (0 if kavoid = 0) */
   ui32  nmstats[NNMST];   /* Statistics */
   ui32  h;                /* Iterations since last nminit */
   ui32  nhallo;           /* Number of nmhist structs allocated */
   si32  nmseed;           /* Random-number seed */
   int   N;                /* Dimension of the problem */
   int   nmasri;           /* X sums recalculation interval
                           *  (normally 250) */
   int   options;          /* User option codes (defined above) */
   };
#else
/* Internal nmglbl struct is hidden from the normal user */
struct nmglbld;
#endif

/* Define functions in the package.  Similar routines for float
*  and double versions have different names, ending in 'f' or 'd'
*  respectively, so one could use both packages in same app.  */
#ifdef __cplusplus
extern "C" {
#endif
struct nmglbld *nmallod(int N, si32 seed,
   nmxyd (*ufn)(nmxyd *x, void *usrd));
void nmparmsd(struct nmglbld *nmG, nmxyd nmasrr, nmxyd nmasxr,
   nmxyd nmascr, nmxyd nmassr, nmxyd nmasar, int nmasri);
nmxyd *nmgetxd(struct nmglbld *nmG);
nmxyd nminitd(struct nmglbld *nmG, void *usrd, char *msgid, int options);
/* Optimization with simulated annealing */
int nmitrsad(struct nmglbld *nmG, void *usrd, nmxyd **ppxbest,
   nmxyd *pybest, ui32 *pniter, nmxyd T,
   nmxyd ftol, nmxyd xtol, ui32 mxiter);
/* Optimization without simulated annealing */
int nmitrnad(struct nmglbld *nmG, void *usrd, nmxyd **ppxbest,
   nmxyd *pybest, ui32 *pniter,
   nmxyd ftol, nmxyd xtol, ui32 mxiter);
/* Locate statistics and clean up */
struct nmhistd *nmgethd(struct nmglbld *nmG);
ui32 *nmgetsd(struct nmglbld *nmG);
void nmfreed(struct nmglbld *nmG);
#ifdef __cplusplus
}
#endif
