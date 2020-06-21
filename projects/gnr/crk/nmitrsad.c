/* (c) Copyright 2005-2009, The Rockefeller University *11116* */
/* $Id: nmitrsad.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*       Nelder-Mead Simplex Optimizer with Simulated Annealing         *
*               (This is the double-precision version)                 *
*                                                                      *
*  This function package uses the Nelder-Mead simplex algorithm, with  *
*  the addition of simulated annealing as suggested in the book        *
*  "Numerical Recipes in C" (2nd Ed.)  by WH Press, SA Teukolsky, WT   *
*  Vetterling, and BP Flannery, Cambridge University Press, 1992 with  *
*  some ideas for program organization borrowed from the fminsearch()  *
*  program (Nelder-Mead without simulated annealing) in MATLAB.        *
*                                                                      *
*  See programming notes and references in the setup file, nelmeadd.c  *
*  The requirements for the user callback cost-evaluation function,    *
*  ufn, are described in the writeup for the startup routine, nminitd. *
*                                                                      *
*  For use outside the rocks library, all convrt() calls can be        *
*  changed to printf() calls by compiling with -DUSE_PRINTF.           *
************************************************************************
*  V5A, 12/02/05, GNR - C version made from MATLAB version, new        *
*                 params, control yhist storage with options, use      *
*                 fluctuated values for drop-through tests (bug fix),  *
*                 save best-ever vertex even if not accepted, break    *
*                 out separate routine without simulated annealing.    *
*  Rev, 02/09/06, GNR - For ROCKS library, change printf() to convrt() *
*  ==>, 02/10/06, GNR - Last date before committing to svn repository  *
*  Rev, 08/29/09, GNR - Separate float vs. double versions             *
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
#include "nelmeadd.h"

/* Macro to get a double-float value in range 0-1 from udev().
*  (Shuffling per Press et al's ran1 is probably not needed here.
*  Also, a return value of 1.0 is OK here, so not tested.)  */
#define rand(seed) ((nmxyd)udev(seed)/(nmxyd)LONG_MAX)

/*=====================================================================*
*                              nmitrsad                                *
*                                                                      *
*  Perform Nelder-Mead simplex optimization with simulated annealing   *
*                                                                      *
*  Minimizes the function ufn beginning with the starting guess x.     *
*  nmallod() must be called first to allocate working memory, then     *
*  the starting guess must be stored in the array returned by a        *
*  call to nmgetxd(), then nminitd() must be called to initialize      *
*  relevant internal variables.                                        *
*                                                                      *
*  Arguments:                                                          *
*     nmG      Pointer to an nmglbld struct created by a previous      *
*              call to nmallod.                                        *
*     usrd     Ptr to any arbitrary data that is to be passed to       *
*              the user-defined function being optimized.  May be      *
*              NULL if no such data are expected.                      *
*     ppxbest  Ptr to a ptr that is filled in on return with the       *
*              location of an array containing the best solution       *
*              found in possibly a series of nmitrsad() calls since    *
*              the last call to nminitd().                             *
*              N.B.  Swapping this vector back into the simplex        *
*              before another call to nmitrsad() can in principle      *
*              produce a singularity, so don't do it (unless           *
*              rebuilding an entirely new simplex).                    *
*     pybest   Ptr to a location that is filled in on return with      *
*              the best value of the function ufn found since the      *
*              last call to nminitd().                                 *
*     pniter   Ptr to a ui32 that will be filled in on return with     *
*              the number of iterations completed since the last       *
*              call to nminitd().                                      *
*     T        Annealing temperature for this series of iterations.    *
*              N.B.  Original article adds -T*log(rand) to each y      *
*              value.  This program multiples y by (1-T*log(rand)).    *
*     ftol     Fractional tolerance in y value. When the magnitude     *
*              of the difference between the largest and smallest      *
*              values of y at vertices of the simplex becomes less     *
*              than ftol times the mean of the magnitudes of those     *
*              values of y, the optimization terminates. (Caution:     *
*              This is not necessarily a very good test.)              *
*     xtol     Absolute tolerance in radius of simplex.  After a       *
*              shrink only (to save time), when the difference of      *
*              x coords at lowest vs highest y is less than xtol,      *
*              the optimization terminates.                            *
*     mxiter   Maximum number of iterations.  When this number of      *
*              iterations is reached, the optimization terminates.     *
*              (MATLAB fminsearch() default is 200*N.)                 *
*                                                                      *
*  Returns:    A return code defined in nelmead.h as follows:          *
*              NM_MXITER     (0)    Completed the specified maximum    *
*                 number of iterations without converging.             *
*              NM_CONVERGED  (1)    Converged by the ftol test.        *
*              NM_SHRUNK     (2)    Shrunk to a figure with a          *
*                 radius less than specified by the xtol test.         *
*              NMERR_NOAVOID (3)    NMKA_MOVE option was specified     *
*                 and program failed to avoid a singular vertex.       *
*              NMERR_ALLSING (4)    All vertices became singular.      *
*                                                                      *
*              Additionally, the variables pointed to by the           *
*              ppxbest, pybest, and pniter arguments are filled        *
*              in as described above.  nmgethd() can be called to      *
*              locate history records requested by the NMKH_LAST       *
*              or NMKH_EVRY option codes.  nmgetsd() can be called     *
*              to locate counts of the operations performed.           *
*              Finally, nmgetxd() can be called to locate the          *
*              updated final state of the simplex.  This matrix        *
*              can be used to restart an optimization that was         *
*              halted prematurely, usually to change the annealing     *
*              temperature.                                            *
*=====================================================================*/

int nmitrsad(struct nmglbld *nmG, void *usrd, nmxyd **ppxbest,
      nmxyd *pybest, ui32 *pniter, nmxyd T,
      nmxyd ftol, nmxyd xtol, ui32 mxiter) {

   nmxyd (*ufn)(nmxyd *x, void *usrd) = nmG->ufn;
   nmxyd  *xsums,*xr,*xt;        /* Ptrs to x sums,temp arrays */
   nmxyd  *xhi;                  /* Ptr to vertex w/highest y */
   nmxyd  *px,*pxe;              /* An x vector and its end */
   nmxyd  *px0 = nmG->nmasx;     /* Ptr to x array */
   nmxyd  *py0 = nmG->nmasy;     /* Ptr to y array */
   ui32   *stats = nmG->nmstats; /* Ptr to stats array */
   nmxyd  hftol = 0.5*ftol;
   nmxyd  xtol2 = xtol*xtol;
   nmxyd  fNi;                   /* Value of 1/N or 1/(N+1) */
   nmxyd  ar,ar1;                /* Avoidance ratio */
   nmxyd  rr0,rr1;               /* Reflection ratio */
   nmxyd  xr0,xr1;               /* Expansion ratio */
   nmxyd  oc0,oc1;               /* Outside contraction ratio */
   nmxyd  ic0,ic1;               /* Inside contraction ratio */
   nmxyd  sr1;                   /* Shrinkage ratio */
   nmxyd  thisy;                 /* y saved for history reporting */
   nmxyd  ybot,ytop;             /* Current worst, best y */
   nmxyd  ylo,yhi,ynhi;          /* Current low, high, next high y */
   nmxyd  yr,yt;                 /* Temp y, later radius values */
   nmxyd  yrf,ytf;               /* Fluctuated y values */
   ui32   h,htest;               /* Current cycle, last cycle */
   ui32   NN = nmG->N;           /* (nmallod assures N > 2) */
   ui32   NNf = nmG->N*sizeof(nmxyd);
   int    lopt = nmG->options;
   int    nxsri = 0;             /* Cycles since x sums update */
   int    dostop = -1;           /* Signal first time through */
   int    km;                    /* Kind of movement step */
   int    iv,jq;                 /* Vertex,coordinate indexes */
   int    vbot,vlo,tlo,vhi;      /* Bottom, low, high vertices */

   /* Names of moves in same order as NMKM defines in nelmead.h */
   static char *km_names[] = { "shrink", "reflect", "expand",
      "contract_out", "contract_in" };

   /* Termination texts in same order as NMERR defines */
   static char *stop_msgs[] = {
      "max iterations exceeded",
      "convergence test passed",
      "simplex shrunk to a point",
      "unable to avoid singular vertex",
      "entire simplex was singular"
      };

#ifdef USE_PRINTF
   /* printf() format for various singularity messages */
   static char sgpfmt[] = "For %s, got singularity at vertex %d "
      "during Nelder-Mead %s operation at cycle %d";
#else
   /* convrt() format for various singularity messages */
   static char sgvfmt[] = "(P3L3,5H0For J0A80,1H,/"
      "'    got singularity at vertex ',J1I10,'during "
      "Nelder-Mead ',J1A12,'operation at cycle ',J0IJ10)";
#endif

   /* Derive useful local constants from globals */
   rr1 = nmG->nmasrr;
   xr1 = nmG->nmasxr*rr1;
   oc1 = nmG->nmascr*rr1;
   ic1 = -nmG->nmascr;
   fNi = 1.0/(nmxyd)NN;
   rr0 = fNi*(1.0 + rr1), rr1 += rr0;
   xr0 = fNi*(1.0 + xr1), xr1 += xr0;
   oc0 = fNi*(1.0 + oc1), oc1 += oc0;
   ic0 = fNi*(1.0 + ic1), ic1 += ic0;
   sr1 = nmG->nmassr;
   ar  = nmG->nmasar;
   ar1 = (1.0 + ar)/ar;    /* (nminitd() assures ar != 0) */
   h   = nmG->h;           /* Current cycle */
   htest = h + mxiter;

   /* Allocate space for history data (high-water-mark) */
   if (lopt & (NMKH_EVRY|NMKH_LAST) && htest > nmG->nhallo) {
      nmG->phist = (struct nmhistd *)reallocv(nmG->phist,
         htest*sizeof(struct nmhistd), "Optimization history");
      nmG->nhallo = htest;
      }

   /* Allocate space for x temps */
   xsums = nmG->nmasxb + NN;
   xr = xsums + NN;
   xt = xr + NN;
   pxe = px0 + NN*(NN+1);

   /* Locate best and worst vertices (unthermalized) */
   ybot = NM_SING, ytop = -NM_SING, vbot = 0;
   for (iv=0; iv<=NN; iv++) {
      if (py0[iv] < ybot) ybot = py0[iv], vbot = iv;
      if (py0[iv] > ytop) ytop = py0[iv];
      }

/* Iterate up to the specified maximum number of cycles */

   while (h < htest) {

/*  Determine which vertex is the highest (worst), next-highest, and
*   lowest (best), before and after thermal fluctuation.  The function
*   values in nmasy are left uncontaminated.  */

      ybot = py0[vlo=vbot=0];
      ytop = py0[vhi=1];
      ylo  = ynhi = ybot*(1.0 - T*log(rand(&nmG->nmseed)));
      yhi  =        ytop*(1.0 - T*log(rand(&nmG->nmseed)));
      if (ybot > ytop) {
         vbot = 1;
         yt = ytop, ytop = ybot, ybot = yt;
         }
      if (ylo > yhi) {
         vhi = 0, vlo = 1;
         ynhi = yhi, yhi = ylo, ylo = ynhi;
         }
      for (iv=2; iv<=NN; iv++) {
         yr = py0[iv];
         yt = yr*(1.0 - T*log(rand(&nmG->nmseed)));
         if (yr < ybot)
            vbot = iv, ybot = yr;
         else if (yr > ytop)
            ytop = yr;
         if (yt <= ylo)
            vlo = iv, ylo = yt;
         else if (yt > yhi)
            ynhi = yhi, vhi = iv, yhi = yt;
         else if (yt > ynhi)
            ynhi = yt;
         }

/* Except on the first time through, test for convergence.  Note that
*  if the best and worst points have opposite signs, the y test will
*  never succeed.  Unlike Press et al, I test on the unthermalized
*  values of y, in order to detect when the simplex converges close to
*  a single value, even early in the annealing process.  The test on
*  the x values (radius) is performed only after a shrink or
*  contract_in step, because it is rather expensive.  Unlike MATLAB, I
*  do the OR of the two tests, not the AND.  I also always do at least
*  one cycle before testing in order to avoid infinite call loops when
*  nothing is changed.  (The ftol test is better done here, rather
*  than at the end as in the MATLAB version, because it avoids the
*  need to update ybot and ytop twice in each cycle).  */

      if (dostop >= 0 && fabs(ytop - ybot) < hftol *
            (fabs(ytop) + fabs(ybot))) {
         stats[NMS_CONVG] += 1;
         dostop = NM_CONVERGED;
         goto EndIteration2;
         }
      if (ylo == NM_SING) {
         dostop = NMERR_ALLSING;
         goto EndIteration2;
         }
      dostop = 0;
      h += 1;

/*  If it is time to recalculate the x-sums, do it now */

      if (--nxsri <= 0) {
         memset((char *)xsums, 0, NNf);
         for (px=px0; px<pxe; )
            for (jq=0; jq<NN; jq++)
               xsums[jq] += *px++;
         nxsri = nmG->nmasri;
         }

/* In the following examination of various test points, keep in
*  mind that xsums holds the sums of all N+1 vertices, but the
*  worst one must always be subtracted to get the correct xbar.
*  This is done by adjusting the values of the ratios rr0, etc.
*  The logic is not well-posed for structured programming.  The
*  code is made clearer (IMHO) by using goto's to get to the end
*  from each point where a move is accepted.  */

      xhi = px0 + NN*vhi;  /* x at high vertex */

/* Compute the reflection point */

      for (jq=0; jq<NN; jq++)
         xr[jq] = rr0*xsums[jq] - rr1*xhi[jq];
      yr = ufn(xr, usrd);
      if (yr == NM_SING) {
         stats[NMS_REFAV] += 1;
         if (lopt & NMKP_DETAIL) {
#ifdef USE_PRINTF
            printf(sgpfmt, nmG->msgid, vhi,
               km_names[NMKM_REFLECT], h);
#else
            convrt(sgvfmt, nmG->msgid, &vhi,
               km_names[NMKM_REFLECT], &h, NULL);
#endif
            nmprtxd(xr, NN);
            }
         goto TryInsideContraction;
         }
      if (yr < nmG->nmasyb) {    /* Save the best ever */
         stats[NMS_REFBEST] += 1;
         nmG->nmasyb = yr;
         memcpy((char *)nmG->nmasxb, (char *)xr, NNf);
         }

/* Reflection is viable, check expansion and outside contraction */

      yrf = yr*(1.0 + T*log(rand(&nmG->nmseed)));
      if (yrf <= ylo) {

/*  Range 1:  yrf <= ylo, try expansion */

         for (jq=0; jq<NN; jq++)
            xt[jq] = xr0*xsums[jq] - xr1*xhi[jq];
         yt = ufn(xt, usrd);
         if (yt == NM_SING) {
            stats[NMS_EXPAV] += 1;
            if (lopt & NMKP_DETAIL) {
#ifdef USE_PRINTF
               printf(sgpfmt, nmG->msgid, vhi,
                  km_names[NMKM_EXPAND], h);
#else
               convrt(sgvfmt, nmG->msgid, &vhi,
                  km_names[NMKM_EXPAND], &h, NULL);
#endif
               nmprtxd(xt, NN);
               }
            goto AcceptReflection;
            }
         if (yt < nmG->nmasyb) {    /* Save the best ever */
            stats[NMS_EXPBEST] += 1;
            nmG->nmasyb = yt;
            memcpy((char *)nmG->nmasxb, (char *)xt, NNf);
            }
         ytf = yt*(1.0 + T*log(rand(&nmG->nmseed)));
         if (ytf < yrf) {
            /* Accept the expansion point */
            thisy = yt;
            stats[NMS_EXPACC] += 1;
            px = xt;
            km = NMKM_EXPAND;
            goto EndIteration;
            }
         else
            /* No, so use the original reflection point */
            goto AcceptReflection;
         }

      else if (yrf < ynhi) {

/* Range 2:  ylo <= yrf < ynhi <= yhi, accept reflection */

AcceptReflection:
         thisy = yr;
         stats[NMS_REFACC] += 1;
         px = xr;
         km = NMKM_REFLECT;
         goto EndIteration;
         }

      else if (yrf < yhi) {

/* Range 3:  ynhi <= yrf < yhi, try outside contraction */

         for (jq=0; jq<NN; jq++)
            xt[jq] = oc0*xsums[jq] - oc1*xhi[jq];
         yt = ufn(xt, usrd);
         if (yt == NM_SING) {
            stats[NMS_OSCAV] += 1;
            if (lopt & NMKP_DETAIL) {
#ifdef USE_PRINTF
               printf(sgpfmt, nmG->msgid, vhi,
                  km_names[NMKM_CONTRACT_OUT], h);
#else
               convrt(sgvfmt, nmG->msgid, &vhi,
                  km_names[NMKM_CONTRACT_OUT], &h, NULL);
#endif
               nmprtxd(xt, NN);
               }
            goto TryInsideContraction;
            }
         if (yt < nmG->nmasyb) {    /* Save the best ever */
            stats[NMS_OSCBEST] += 1;
            nmG->nmasyb = yt;
            memcpy((char *)nmG->nmasxb, (char *)xt, NNf);
            }
         ytf = yt*(1.0 + T*log(rand(&nmG->nmseed)));
         if (ytf <= yrf) {
            /* Accept the outside contraction point */
            thisy = yt;
            stats[NMS_OSCACC] += 1;
            px = xt;
            km = NMKM_CONTRACT_OUT;
            goto EndIteration;
            }
         }

/* Range 4:  All outside motions tested gave ytf >= yhi
*  (or reflection is singular), try inside contraction.
*  (MATLAB goes directly to shrink if inside contraction
*  is poorer than reflection.)  */

TryInsideContraction:
      for (jq=0; jq<NN; jq++)
         xt[jq] = ic0*xsums[jq] - ic1*xhi[jq];
      yt = ufn(xt, usrd);
      if (yt == NM_SING) {
         stats[NMS_INSAV] += 1;
         if (lopt & NMKP_DETAIL) {
#ifdef USE_PRINTF
            printf(sgpfmt, nmG->msgid, vhi,
               km_names[NMKM_CONTRACT_IN], h);
#else
            convrt(sgvfmt, nmG->msgid, &vhi,
               km_names[NMKM_CONTRACT_IN], &h, NULL);
#endif
            nmprtxd(xt, NN);
            }
         goto TryShrink;
         }
      if (yt < nmG->nmasyb) {    /* Save the best ever */
         stats[NMS_INSBEST] += 1;
         nmG->nmasyb = yt;
         memcpy((char *)nmG->nmasxb, (char *)xt, NNf);
         }
      ytf = yt*(1.0 + T*log(rand(&nmG->nmseed)));
      if (ytf < yhi) {
         /* Accept the inside contraction point */
         stats[NMS_INSACC] += 1;
         km = NMKM_CONTRACT_IN;
         /* Update the current worst vertex and xsums
         *  (required before radius test below).  */
         py0[vhi] = thisy = yt;
         for (jq=0; jq<NN; jq++) {
            xsums[jq] += xt[jq] - xhi[jq];
            xhi[jq] = xt[jq];
            }
         goto TestRadius;
         }

/* Shrink if no other move was accepted.  It is probably faster to
*  swap the low point into the low position, then shrink all the rest,
*  rather than have a test for iv==vlo inside the loop.  If NMKS_THERM
*  option is set, shrinks around the fluctuated lowest point rather
*  than the absolute lowest point.  The y value reported is the best
*  value (the one not replaced).  This is consistent with the other
*  cases in that it is the one unique point on the simplex that was
*  treated differently from the rest.  This value sticks out in any
*  plot of yhist vs h, marking the shrinks.  Test code in V3I that
*  shrank multiple times until certain criteria were met has been
*  removed in V4A:  Shrinking is very expensive and it seems best to
*  repeat all the vertex tests each time rather than shrink with the
*  simplified tests.  */

TryShrink:
      if (!(lopt & NMKS_THERM)) vlo = vbot;
      /* Swap x[vlo] and x[0] using xr as a temp */
      if (vlo > 0) {
         px = px0 + NN*vlo;
         memcpy((char *)xr, (char *)px, NNf);
         memcpy((char *)px, (char *)px0, NNf);
         memcpy((char *)px0, (char *)xr, NNf);
         py0[0] = py0[vlo];
         }
      /* Keep track of best y for best-ever test */
      ylo = NM_SING, tlo = 0;
      for (iv=1,px=px0+NN; iv<=NN; iv++,px+=NN) {
         for (jq=0; jq<NN; jq++)
            px[jq] = px0[jq] + sr1*(px[jq] - px0[jq]);
         yt = ufn(px, usrd);
         if (yt == NM_SING && lopt & NMKA_MOVE) {
            int   nmoves;           /* Avoidance move count */
            if (lopt & NMKP_DETAIL) {
#ifdef USE_PRINTF
               printf(sgpfmt, nmG->msgid, iv,
                  km_names[NMKM_SHRINK], h);
#else
               convrt(sgvfmt, nmG->msgid, &iv,
                  km_names[NMKM_SHRINK], &h, NULL);
#endif
               nmprtxd(px, NN);
               }
            /* xr is now a temp for the x shifts
            *  and xt holds the shrunk, but unshifted x */
            for (jq=0; jq<NN; jq++)
               xr[jq] = ar1 * (px0[jq] - (xt[jq] = px[jq]));
            for (nmoves = 0; nmoves<NMKA_MXMV && yt==NM_SING;
                  nmoves++) {
               for (jq=0; jq<NN; jq++)
                  px[jq] = xt[jq] + (xr[jq] *= ar);
               yt = ufn(px, usrd);
               } /* End vertex movement loop */
            stats[NMS_SHRAV] += 1;
            stats[NMS_SHRAVM] += nmoves;
            /* Force early exit if removal failed */
            if (yt == NM_SING) {
               dostop = NMERR_NOAVOID;
               goto EndIteration2;
               }
            } /* End singularity removal */
         if (yt < ylo) ylo = yt, tlo = iv;
         py0[iv] = yt;
         } /* End vertex loop */

      /* Update the best-ever solution */
      if (ylo < nmG->nmasyb) {
         stats[NMS_SHRBEST] += 1;
         nmG->nmasyb = ylo;
         memcpy((char *)nmG->nmasxb, (char *)(px0 + NN*tlo), NNf);
         }
      thisy = ylo;
      km = NMKM_SHRINK;
      stats[NMS_SHRINK] += 1;

      /* Update the x sums and reset the counter.  (Don't just
      *  set nxsri = 0 to force new sums on next iteration,
      *  because new sums are needed below for radius test.)  */
      memset((char *)xsums, 0, NNf);
      for (px=px0; px<pxe; )
         for (jq=0; jq<NN; jq++)
            xsums[jq] += *px++;
      nxsri = nmG->nmasri;

      /* Calculate simplex radius and set dostop if very small */
TestRadius:
      fNi = 1.0/(nmxyd)(NN+1);
      for (jq=0; jq<NN; jq++)
         xr[jq] = fNi*xsums[jq];
      yt = 0.0;
      for (px=px0; px<pxe; ) {
         yr = 0.0;
         for (jq=0; jq<NN; jq++) {
            nmxyd xrel = *px++ - xr[jq];
            yr += xrel*xrel;
            }
         if (yr > yt) yt = yr;
         } /* End vertex loop */
      if (yt < xtol2) {
         stats[NMS_POINT] += 1;
         dostop = NM_SHRUNK;
         goto EndIteration2;
         }

      /* Reportage for shrink or inside contraction step */
      if (lopt & NMKP_DETAIL) {
         nmxyd rtyt = sqrt(yt);
#ifdef USE_PRINTF
         printf("For %s, radius is %.5g after %s "
            "around vertex %d at cycle %ld,\n",
            nmG->msgid, rtyt, km_names[km], vlo, h);
#else
         convrt("(P3,5H For J1A80,'radius is ',J1VD9.6,'after ',"
            "J1A12,'around vertex ',J1I10,'at cycle ',J0IJ10)",
            nmG->msgid, &rtyt, km_names[km], &vlo, &h, NULL); }
#endif
      goto EndIteration2;

EndIteration:
      /* Update the current vertex that was replaced.
      *  (px points to updated vertex coords on entry). */
      py0[vhi] = thisy;
      for (jq=0; jq<NN; jq++) {
         xsums[jq] += px[jq] - xhi[jq];
         xhi[jq] = px[jq];
         }

EndIteration2:
      /* Save history for this iteration if requested */
      if (lopt & NMKH_EVRY) {
         struct nmhistd *ph = nmG->phist + h - 1;
         ph->y      = thisy;
         ph->ybest  = ybot;
         ph->yworst = ytop;
         ph->yglmin = nmG->nmasyb;
         ph->method = km;
         }

      /* Report history for this iteration if requested.  Note that
      *  the ybot,ytop reported here are those at the start of the
      *  cycle, which avoids recalculating them just for report.  */
      if (lopt & NMKP_EVRY)
#ifdef USE_PRINTF
         printf("At iteration %6d, vertex %5d, did %s\n   new, low, "
            "high, global best y = %8.5g, %8.5g, %8.5g, %8.5g\n",
            h, vlo, km_names[km], thisy, ybot, ytop, nmG->nmasyb);
#else
         convrt("(P3,' At iteration ',J0IJ10,', vertex ',J0I10,"
            "', did ',J0A12/'    New, low, high, global best y = ',"
            "4(VD9.6;1H,))", &h, &vlo, km_names[km],
            &thisy, &ybot, &ytop, &nmG->nmasyb, NULL);
#endif
      if (dostop > 0) break;

      } /* End h loop */

   /* Save history for last iteration if requested */
   if ((lopt & (NMKH_EVRY|NMKH_LAST)) == NMKH_LAST) {
      struct nmhistd *ph = nmG->phist;
      ph->y      = thisy;
      ph->ybest  = ybot;
      ph->yworst = ytop;
      ph->yglmin = nmG->nmasyb;
      ph->method = km;
      }

   /* Report history for last iteration if requested */
   if ((lopt & (NMKP_EVRY|NMKP_LAST)) == NMKP_LAST)
#ifdef USE_PRINTF
      printf("At iteration %6d, vertex %5d, did %s\n   New, low, "
         "high, global best y = %8.5g, %8.5g, %8.5g, %8.5g\n",
         h, vlo, km_names[km], thisy, ybot, ytop, nmG->nmasyb);
#else
      convrt("(P3,' At iteration ',J0IJ10,', vertex ',J0I10,"
         "', did ',J0A12/'    New, low, high, global best y = ',"
         "4(VD9.6;1H,))", &h, &vlo, km_names[km],
         &thisy, &ybot, &ytop, &nmG->nmasyb, NULL);
#endif

   /* Report termination reason if requested */
   if (lopt & (NMKP_LAST|NMKP_EVRY|NMKP_DETAIL))
#ifdef USE_PRINTF
      printf("For %s at iteration %6d, %s\n",
         nmG->msgid, h, stop_msgs[dostop]);
#else
      convrt("(P2,5H For J1A80,'at iteration ',J0IJ10,2H, J0A40)",
         nmG->msgid, &h, stop_msgs[dostop], NULL);
#endif

   /* Set specified pointers to return information */
   *ppxbest = nmG->nmasxb;       /* Ptr to best-ever solution */
   *pybest  = nmG->nmasyb;       /* Best-ever y value */
   *pniter  = h;                 /* Number of cycles */

   return dostop;
   } /* End nmitrsad() */
