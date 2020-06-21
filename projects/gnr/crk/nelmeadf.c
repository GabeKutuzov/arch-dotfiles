/* (c) Copyright 2002-2009, The Rockefeller University *11116* */
/* $Id: nelmeadf.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*       Nelder-Mead Simplex Optimizer with Simulated Annealing         *
*          Startup and termination routines (float versions)           *
*                                                                      *
*  This function package uses the Nelder-Mead simplex algorithm, with  *
*  or without the addition of simulated annealing as suggested in the  *
*  book "Numerical Recipes in C" (2nd Ed.)  by WH Press, SA Teukolsky, *
*  WT Vetterling, and BP Flannery, Cambridge University Press, 1992    *
*  with some ideas for program organization borrowed from the MATLAB   *
*  fminsearch() program (Nelder-Mead without simulated annealing).     *
*  When a new simplex is generated at each thermal step, it is built   *
*  around the best vertex in the previous simplex.  Therefore, the     *
*  "best-ever" y will be maintained automatically across steps.        *
*                                                                      *
*  The division into separate initialization and iteration routines    *
*  allows an optimization to be continued with minimal startup cost    *
*  after a temperature change or other interruption.  The sums of the  *
*  vertex coords are recalculated at nmasri (default 250) iteration    *
*  intervals to avoid accumulation of round-off errors.  This can be   *
*  changed by the user if desired.   The idea of recording the best-   *
*  ever vertex even if it is not accepted due to added noise, the      *
*  idea of avoiding singular points, the test for convergence on the   *
*  unthermalized data, the occasional recalc of the running total      *
*  xsums, the application of thermal noise as a multiplier rather      *
*  than an addition are all original with this GNR implementation.     *
*  This is the single-precision version--all the routines with names   *
*  ending in 'f' have corresponding versions with names ending in 'd'  *
*  that work in double precision.  Finally, in general, code is        *
*  repeated where it will avoid redundant 'if's.                       *
*                                                                      *
*  When ufn returns a singular situation (NM_SING code), if option     *
*  NMKA_MOVE was set, the point is moved in an expanding see-saw       *
*  along the line to another point picked at random until a valid      *
*  point is found, up to a maximum of NMKA_MXMV (default 10) moves.    *
*  Moving along lines between points (without letting the points       *
*  coalesce) maintains the rank of the simplex.                        *
*                                                                      *
*  Reference:  Jeffrey C.  Lagarias, James A.  Reeds, Margaret H.      *
*  Wright, Paul E.  Wright, "Convergence Properties of the Nelder-     *
*  Mead Simplex Method in Low Dimensions", SIAM Journal of             *
*  Optimization, 9(1):  p.112-147, 1998.                               *
*                                                                      *
*  For use outside the rocks library, all convrt() calls can be        *
*  changed to printf() calls by compiling with -DUSE_PRINTF.           *
************************************************************************
*  V1A, 05/03/02, G.N. Reeke - New implementation (MATLAB)             *
*  V3I, 10/08/02, GNR - Add nmasks.  Option to shrink around best      *
*                 unthermalized vertex and to iterate shrink until     *
*                 worst point stops improving or best point moves to   *
*                 a different vertex.  Record yhi,yb,ylo,yhi in yhist. *
*                 Catch one rare case where xb,yb were not saved.      *
*                 Write report=2,3 output to debug file.               *
*  Rev, 10/17/02, GNR - Add kreport, nmasar and singularity fixup.     *
*  Rev, 10/19/02, GNR - Never allow a singular point in the simplex.   *
*  V4A, 11/29/02, GNR - Remove multiple shrinks, ybest-->yinfo,        *
*                 accumulate yhist over subproblems.                   *
*  Rev, 01/29/03, GNR - Handle different subnet termination times.     *
*  Rev, 02/27/03, GNR - Add kavoid argument.                           *
*  Rev, 03/28/03, GNR - Add exit criterion code to niter.              *
*  Rev, 07/07/03, GNR - Define meaning of integer part of xtol.        *
*  V5A, 12/02/05, GNR - C version made from MATLAB version, new        *
*                 params, control yhist storage with option code, use  *
*                 fluctuated values for drop-through tests (bug fix),  *
*                 save best-ever vertex even if not accepted.          *
*  Rev, 02/09/06, GNR - For ROCKS library, change printf() to convrt() *
*  ==>, 02/10/06, GNR - Last date before committing to svn repository  *
*  V6A, 08/26/09, GNR - Separate float and double versions             *
***********************************************************************/

#define IMNMEAD            /* Expand hidden globals */

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "sysdef.h"
#include "rocks.h"
#include "rkarith.h"
#include "nelmeadf.h"


/*=====================================================================*
*                               nmprtxf                                *
*                                                                      *
*  This private routine prints the coordinates of a singular point     *
*  when detailed print is requested.                                   *
*                                                                      *
*  Arguments:                                                          *
*     x        Ptr to array of x coordinates of the bad point.         *
*     N        Size of x.                                              *
*=====================================================================*/

#define VXPL   10          /* Values of x per line */
#define qVXPL "10"

void nmprtxf(nmxyf *x, int N) {

#ifdef USE_PRINTF
   int i,j;
   printf("   Location of singular test vertex:");
   for (i=j=0; i<N; i++) {
      if (--j <= 0) { printf("\n      "); j = VXPL; }
      printf(" %9.5f", x[i]);
      } /* End i loop */
   printf("\n");
#else
   int  nlines = 2 + (N + VXPL - 1)/VXPL;
   convrt("(P3L,'   Location of singular test vertex:'/"
      "#,6X|(" qVXPL "VF9.6/))", &nlines, &N, x, NULL);
#endif

   } /* End nmprtxf() */


/*=====================================================================*
*                               nmallof                                *
*                                                                      *
*  Allocate a global workspace for a Nelder-Mead optimization.  This   *
*  allows multiple optimizations to be run in parallel without mutual  *
*  interference.  nmfreef must be called to free the storage when it   *
*  is no longer needed.                                                *
*                                                                      *
*  Arguments:                                                          *
*     N        Number of dimensions to the problem.                    *
*     seed     Random number seed.  If zero, a value based on the      *
*              system clock is chosen.                                 *
*     ufn      Pointer to a function, ufn(nmxyf *x, void *usrd), that  *
*              is to be minimized, where x is a vector of length N     *
*              defining a point where the function is to be evaluated  *
*              and usrd is any user data (typically a structure) that  *
*              is to be passed to ufn each time it is called.  ufn     *
*              should return NM_SING if the result is a singularity.   *
*  Returns:    A pointer to a nmglblf structure that will contain all  *
*              the data needed to maintain the state of the ongoing    *
*              optimization.  This pointer must be passed to all       *
*              the other routines in the package.  Its contents are    *
*              of no concern to the caller.                            *
*  Errors:     Generates an abexit error if required memory is not     *
*              available.                                              *
*=====================================================================*/

struct nmglblf *nmallof(int N, si32 seed,
      nmxyf (*ufn)(nmxyf *x, void *usrd)) {

   struct nmglblf *nmG;

   if (N <= 2)
      abexitm(85, "Nelder-Mead simplex dimension < 3");
   nmG = (struct nmglblf *)callocv(1, sizeof(struct nmglblf),
      "Global optimization data");
   nmG->N = N;
   nmG->ufn = ufn;
   nmG->nmasx = mallocv((N*(N+NXVECS)+1)*sizeof(nmxyf),
      "Optimization simplex data");
   nmG->nmasy = nmG->nmasx + N*(N+1);
   nmG->nmasxb = nmG->nmasy + N + 1;
   nmG->nmasrr = 1.0;
   nmG->nmasxr = 2.0;
   nmG->nmascr = 0.5;
   nmG->nmassr = 0.5;
   nmG->nmasar = -1.083333;
   nmG->nmasri = 250;
   nmG->nmseed = (seed > 0) ? seed : (si32)time(NULL);

   return nmG;
   } /* End nmallof() */


/*=====================================================================*
*                              nmparmsf                                *
*                                                                      *
*  Modifies the standard Nelder-Mead search parameters as needed       *
*  for special applications.                                           *
*                                                                      *
*  Arguments:                                                          *
*     nmG      Pointer to an nmglblf struct created by a previous      *
*              call to nmallof.                                        *
*     nmasrr   Reflection ratio (normally 1.0).                        *
*     nmasxr   Expansion ratio (normally 2.0).                         *
*     nmascr   Contraction ratio (normally 0.5).                       *
*     nmassr   Shrinkage ratio (normally 0.5).                         *
*     nmasar   Avoidance ratio (used to avoid placing a simplex        *
*              vertex in a singularity.  Sets the relative amount      *
*              of each successive movement.  Should be negative        *
*              and a little less than -1.0, normally -1.083333.        *
*              Not used if option NMKA_MOVE is not set.                *
*     nmasri   Interval at which sums of x vectors are recalculated    *
*              rather than just corrected (used to prevent roundoff    *
*              errors from huilding up indefinitely, normally 250).    *
*=====================================================================*/

void nmparmsf(struct nmglblf *nmG, nmxyf nmasrr, nmxyf nmasxr,
      nmxyf nmascr, nmxyf nmassr, nmxyf nmasar, int nmasri) {

   nmG->nmasrr = nmasrr;
   nmG->nmasxr = nmasxr;
   nmG->nmascr = nmascr;
   nmG->nmassr = nmassr;
   nmG->nmasar = nmasar;
   nmG->nmasri = nmasri;

   } /* End nmparmsf() */


/*=====================================================================*
*                               nmgetxf                                *
*                                                                      *
*  Returns the location of the storage allocated for the simplex       *
*  by a previous call to nmallof().   This should be viewed as a       *
*  two-dimensional array of (N+1) vectors each of length N.            *
*=====================================================================*/

nmxyf *nmgetxf(struct nmglblf *nmG) {
   return nmG->nmasx;
   } /* End nmgetxf() */


/*=====================================================================*
*                               nminitf                                *
*                                                                      *
*  Initialize Nelder-Mead simplex optimization                         *
*                                                                      *
*  Calculates and stores the initial function values at the vertices   *
*  of a simplex in readiness for a round of optimization by            *
*  nmitr[ns]af.  Before calling nminitf, the user should call nmallof  *
*  to allocate a global minimization workspace, nmparmsf if desired    *
*  to change any default optimization parameters.  Then call nmgetxf   *
*  to locate the storage allocated for the simplex and generate an     *
*  initial simplex of (N+1) vertices in N-dimensional space at that    *
*  location.  Any vertex that is in a singularity may be moved by      *
*  nminitf to a safe nearby position if option NMKA_MOVE is set.       *
*  nminitf() also stores the options argument for use by nmitr[ns]af.  *
*  It should be followed by one or more calls to nmitr[ns]af to carry  *
*  out the actual optimization.  In deference to custom, the ufn       *
*  function value is minimized--ufn should return the complementary    *
*  value if maximization is desired.                                   *
*                                                                      *
*  Arguments:                                                          *
*     nmG      Pointer to an nmglblf struct created by a previous      *
*              call to nmallof.                                        *
*     usrd     Ptr to any arbitrary data that is to be passed to       *
*              the user-defined function being optimized.  May be      *
*              NULL if no such data are expected.                      *
*     msgid    Identifying text to be inserted in error messages       *
*              and reports from nmitr[ns]af.  Only the pointer, not    *
*              the text, is copied, so the value should remain valid   *
*              until the nmfreef() has been called.                    *
*     options  OR of codes defined in nelmeadf.h for various reporting *
*              and algorithmic options.                                *
*                                                                      *
*  Returns:    The best value, y, of the user-defined function at any  *
*              vertex of the initial simplex.                          *
*=====================================================================*/

nmxyf nminitf(struct nmglblf *nmG, void *usrd, char *msgid,
      int options) {

   nmxyf *qx,*vx;          /* Ptrs to x vectors */
   nmxyf *wkxv,*wkdx;      /* Temp x vectors */
   nmxyf ar,ar1;           /* Avoidance ratios */
   nmxyf yt;               /* ufn value at x[iv] */
   nmxyf ybest = NM_SING;  /* Best y value found */
   int   NN = nmG->N;      /* Local copy of N */
   int   NNf = nmG->N*sizeof(nmxyf);
   int   iv,jq,iq;         /* Vertex,coordinate indexes */
   int   nmoves;           /* Advoidance move count */
   int   vlo = 0;          /* iv at ybest */

   /* Store control parameters */
   nmG->msgid   = msgid;
   nmG->options = options;
   if (nmG->nmasar >= 0.0) nmG->nmasar = -1.083333;
   ar = nmG->nmasar, ar1 = (ar + 1.0)/ar;

   /* Clear statistics */
   memset((char *)nmG->nmstats, 0, NNMST*sizeof(ui32));
   nmG->h = 0;

   wkxv = nmG->nmasxb + NN;
   wkdx = wkxv + NN;
   /* Compute cost function at all vertices */
   for (vx=nmG->nmasx,iv=0; iv<=NN; vx+=NN,iv++) {
      yt = nmG->ufn(vx, usrd);
      if (yt == NM_SING && options & NMKA_MOVE) {
         /* Got a singularity--Pick an arbitrary vertex qx
         *  to move toward or away from.  If recovery is not
         *  successful, the vertex is nonetheless left in the
         *  simplex, as a normal move may later recover it.
         *  If recovery fails in nmitr[ns]af(), error -2 is issued.  */
         iq = (int)(udev(&nmG->nmseed) % NN);
         if (iq == iv) iq = NN;
         qx = nmG->nmasx + NN*iq;
         for (jq=0; jq<NN; jq++)
            wkdx[jq] = ar1 * (qx[jq] - vx[jq]);
         for (nmoves=0; nmoves<NMKA_MXMV && yt==NM_SING; nmoves++) {
            for (jq=0; jq<NN; jq++)
               wkxv[jq] = vx[jq] + (wkdx[jq] *= ar);
            yt = nmG->ufn(wkxv, usrd);
            } /* End vertex movement loop */
         nmG->nmstats[NMS_NMIAV] += 1;
         nmG->nmstats[NMS_NMIAVM] += nmoves;
         if (options & NMKP_DETAIL) {
#ifdef USE_PRINTF
            printf("For %s, got singularity at vertex %d during "
               "Nelder-Mead initialization,\n", msgid, iv);
            nmprtxf(vx, NN);
            if (yt == NM_SING)
               printf("   not recovered after %d moves.\n", nmoves);
            else
               printf("   recovered to y = %.5g after %d moves.\n",
                  yt, nmoves);
#else
            convrt("(P3L3,5H0For J0A" qCDSIZE ",1H,/'    got "
               "singularity at vertex ',J1I10,'during Nelder-Mead "
               "initialization')", msgid, &iv, NULL);
            nmprtxf(vx, NN);
            if (yt == NM_SING)
               convrt("(P3,'    not recovered after ',J1I10,'moves.')",
                  &nmoves, NULL);
            else
               convrt("(P3,'    recovered to y = ',J1E10.6,'after ',"
                  "J1I10,'moves.')", &yt, &nmoves, NULL);
#endif
            }
         memcpy((char *)vx, (char *)wkxv, NNf);
         } /* End singularity removal */
      if (yt < ybest) ybest = yt, vlo = iv;
      nmG->nmasy[iv] = yt;
      } /* End iv loop */

   memcpy((char *)nmG->nmasxb, (char *)(nmG->nmasx+NN*vlo), NNf);
   nmG->nmasyb = ybest;

   return ybest;
   } /* End nminitf() */


/*=====================================================================*
*                               nmgethf                                *
*                                                                      *
*  Returns the location of the storage allocated for the history       *
*  record kept by a previous call to nmitr[ns]af().  The result        *
*  will point to one nmhistf struct if the NMKH_LAST options bit       *
*  was set, or to an array containing one nmhistf struct for each      *
*  iteration since the last nminitf() call if the NMKH_EVRY options    *
*  bit was set, or NULL if neither bit was set.                        *
*=====================================================================*/

struct nmhistf *nmgethf(struct nmglblf *nmG) {

   /* Return NULL if no history requested in last nminitf() call, even
   *  though there might be a non-NULL nmG->phist from earlier.  */
   return (nmG->options & (NMKH_LAST|NMKH_EVRY)) ? nmG->phist : NULL;
   } /* End nmgethf() */


/*=====================================================================*
*                               nmgetsf                                *
*                                                                      *
*  Returns the location of the storage allocated for the optimization  *
*  statistics by a previous call to nmallof().  This array contains    *
*  NNMST counts with individual meanings as defined in nelmeadf.h.     *
*=====================================================================*/

ui32 *nmgetsf(struct nmglblf *nmG) {
   return nmG->nmstats;
   } /* End nmgetsf() */


/*=====================================================================*
*                               nmfreef                                *
*                                                                      *
*  Free global workspace storage allocated by nmallof.  After this     *
*  call, the nmG pointer is invalid and no more calls to routines      *
*  in this package should be made using this pointer.                  *
*                                                                      *
*  Argument:                                                           *
*     nmG      Pointer to an nmglblf struct created by a previous      *
*              call to nmallof.                                        *
*=====================================================================*/

void nmfreef(struct nmglblf *nmG) {

   if (nmG->nhallo) freev(nmG->phist, "Optimization history");
   freev(nmG->nmasx, "Optimization simplex data");
   freev(nmG, "Global optimization data");

   } /* End nmfreef() */
