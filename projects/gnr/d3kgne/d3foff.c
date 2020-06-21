/* (c) Copyright 1995-2010 Neurosciences Research Foundation, Inc. */
/* $Id: d3foff.c 48 2012-04-23 18:40:15Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                               d3foff                                 *
*                                                                      *
*      Routines implementing inhibitory surround falloff in CNS        *
*                                                                      *
*  To determine the number of bytes needed to be allocated in a        *
*  falloff table for a particular INHIBBLK (kmax = FALSE to get        *
*  actual value, kmax = TRUE to get maximum value):                    *
*  long lfoff = lfofftbl(struct INHIBBLK *ib, int kmax)                *
*                                                                      *
*  To initialize a falloff table for a particular INHIBBLK             *
*  (the table must have previously been allocated by d3nset):          *
*  void mfofftbl(struct INHIBBLK *ib)                                  *
*                                                                      *
*  To print the falloff tables for all INHIBBLKs that have them:       *
*  void prtfoff(void)                                                  *
*                                                                      *
*  To obtain a pointer to the routine needed to calculate falloff      *
*  values for a particular INHIBBLK (optimized for lookup vs calc):    *
*  FoffFn qfofffnc(struct INHIBBLK *ib)                                *
*                                                                      *
*  To calculate falloff value for an (x,y) coordinate pair:            *
*  long d = foffxxxx(struct INHIBBLK *ib, long x, long y)              *
*  (where foffxxxx is a function pointer returned by qfofffnc)         *
*                                                                      *
*  V8A, 09/02/95, GNR - Broken out from d3inhb for better modularity   *
*                       and a true distance metric implemented         *
*  ==>, 02/17/05, GNR - Last mod before committing to svn repository   *
*  V8F, 07/04/10, GNR - Move temp falloff table for print to CDAT.psws *
***********************************************************************/

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <math.h>
#include "sysdef.h"
#include "d3global.h"
#include "celldata.h"
#ifndef PARn
#include "rocks.h"
#endif

/*---------------------------------------------------------------------*
*                              lfofftbl                                *
*                                                                      *
*  N.B.  If the IBOPTT bit is not set, falloff is calculated when      *
*  needed, not from a table lookup, and no tables are ever allocated.  *
*  However, routine does not check whether falloff exists (IBOPFO),    *
*  because it may be used to reserve space for possible future use.    *
*                                                                      *
*  N.B.  If the kmax flag is TRUE, calculate the maximum space that    *
*  could be used regardless of which falloff metric is chosen, thus    *
*  allowing user to change the metric at d3chng() time.  Otherwise,    *
*  calculate the exact size needed under the present settings.  The    *
*  R metric will always require the largest table:  Assuming x > y,    *
*  (x + y/2)^2 = x^2 + xy + y^2/4 > x^2 + y^2 + y^2/4 > x^2 + y^2,     *
*  therefore, (x+ y/2) > sqrt(x^2 + y^2).  With R,S metrics, values    *
*  with fraction exactly 1/2 are rounded down, not up.                 *
*                                                                      *
*  N.B.  Cell coordinates are translated for falloff calculation to    *
*  an origin at the center of the source repertoire.  If size of       *
*  repertoire is an odd number, coords are integers (0 at center).     *
*  If size is an even number, coords are half-integers.                *
*                                                                      *
*  Just a thought:  The space wasted if one were always to allocate    *
*  the max for falloff is quite small (using IBOPTT to determine in    *
*  advance which inhibblks would need tables at any time in the run).  *
*  Presumably, if IBOPTT is set, the table will eventually be needed.  *
*  So, one could put falloff and norm tables into some permanent space *
*  and eliminate run failure when metric changed at Group III time.    *
*---------------------------------------------------------------------*/

long lfofftbl(struct INHIBBLK *ib, int kmax) {

   long lft;

   if (!(ib->ibopt & IBOPTT))
      return 0;
   if (kmax || (ib->kfoff & FOFFRMETRIC)) {
      /* Generic prealloc or R metric (max(x,y) + min(x,y)/2) */
      if (ib->l1xn2 >= ib->l1yn2)
         lft = (ib->l1xn2 << 1) + ib->l1yn2 + 2;
      else
         lft = (ib->l1yn2 << 1) + ib->l1xn2 + 2;
      }
   else if (ib->kfoff & FOFFSMETRIC) {
      /* S metric (ring number) */
      lft = (max(ib->l1xn2, ib->l1yn2) << 1) + 2;
      }
   else {
      /* D metric (Euclidean distance) */
      long tx = ib->l1xn2 - 1;   /* Twice xmx */
      long ty = ib->l1yn2 - 1;   /* Twice ymx */
      /* To get four bytes per table entry, use twice the sqrt
      *  of the sum of squares of the doubled edges.  It is not
      *  possible for the sqrt of any integer to have fraction
      *  exactly 1/2, so we can round in the usual way, adding
      *  2/4 of an entry, plus 4/4 for the (0,0) entry.  It is
      *  not correct to add 1/4 as done for other metrics.  */
      lft = ((long)sqrt((double)(tx*tx + ty*ty)) << 1) + 6;
      }
   return (lft & ~3);
   } /* End lfofftbl() */

/*---------------------------------------------------------------------*
*                              mfofftbl                                *
*                                                                      *
*  The falloff table has one entry for each possible value of the      *
*  radius metric (d,r,s) from 0 out to the edge of the cell layer.     *
*  In the special, but common, case of the s metric with both cell     *
*  edges even numbers, value is tabulated for index+(1/2).             *
*---------------------------------------------------------------------*/

#define MLN2   -0.69314718056 /* Constant for adjusting exp arg */
#define SQRT2P1 2.41421356237 /* Constant for 1/rsq falloff */

void mfofftbl(struct INHIBBLK *ib) {

   double dfv;                /* Double-precision falloff value */
   double rad0,rad;           /* Radius for falloff */
   double dhlfrad;            /* Double-precision half radius */
   long *ftable;              /* Ptr into falloff table */
   long *ftablelim;           /* Loop limit */

   if ((ib->ibopt & (IBOPFO|IBOPTT)) != (IBOPFO|IBOPTT)) return;

   ftable = ib->falloff;
   ftablelim = (long *)((char *)ftable + lfofftbl(ib, FALSE));
   dhlfrad = (double)ib->hlfrad;
   rad0 = ((ib->kfoff & FOFFSMETRIC) && !((ib->l1x|ib->l1y) & 1)) ?
      0.5 : 0.0;

   for (rad=rad0 ; ftable<ftablelim; rad+=1.0) {

      /* Ignore metric type here */
      switch (ib->kfoff & 3) {

case FOFF1D:               /* hlfrad/(hlfrad+r) */
         dfv = dhlfrad/(dhlfrad + rad);
         break;

case FOFF1DSQ:             /* (qr0/(qr0+r))**2, where q=1+sqrt(2) */
         dfv = dhlfrad * (double)SQRT2P1;
         dfv /= (dfv + rad);
         dfv *= dfv;
         break;

case FOFFEMD:              /* exp(-r) */
         dfv = exp((rad/dhlfrad)*(double)MLN2);
         break;

case FOFFEMDSQ:            /* exp(-rsq) */
         dfv = rad/dhlfrad;
         dfv = exp(dfv*dfv*(double)MLN2);
         break;
         }  /* End of falloff switch */

/* Convert to (S28), round, store, and increment table pointer */

      *ftable++ = (long)(dfv*dS28 + 0.5);
      } /* End of loop over falloff table entries */

   } /* End mfofftbl */

#ifndef PARn
/*---------------------------------------------------------------------*
*                               prtfoff                                *
*                                                                      *
*  This routine is activated by the RP->kdprt code PRFOFF.             *
*---------------------------------------------------------------------*/

void prtfoff(void) {

   struct CELLTYPE *il;
   struct INHIBBLK *ib;

   for (il=RP->pfct; il; il=il->pnct) {
      for (ib=il->pib1; ib; ib=ib->pib) {
         if ((ib->ibopt & (IBOPFO|IBOPTT)) == (IBOPFO|IBOPTT)) {
            double rad0,rad;
            long *ftable,*ftablelim;
            long tlen = lfofftbl(ib, FALSE);
#ifdef PAR0
            /* If parallel, create a temporary falloff table */
            ib->falloff = (long *)CDAT.psws;
            mfofftbl(ib);
#endif
            convrt("(L3,'0Falloff table for GCONN type ',J1IH4,3Hto "
               "J1A" qLXN ",'from (',J0A" qLSN ",H,J0A" qLSN ",2H):/"
               "' Radius  Falloff')", &ib->igt,
               fmtlrlnm(il), ib->gsrcnm.rnm, ib->gsrcnm.lnm, NULL);
            ftable = ib->falloff;
            ftablelim = (long *)((char *)ftable + tlen);
            rad0 = ((ib->kfoff & FOFFSMETRIC) &&
               !((ib->l1x|ib->l1y) & 1)) ? 0.5 : 0.0;
            for (rad=rad0; ftable<ftablelim; ftable++,rad+=1.0)
               convrt("(Q6.3,B28IL10.6)", &rad, ftable, NULL);
#ifdef PAR0
            /* JIC: Kill temporary falloff table */
            ib->falloff = NULL;
#endif
            } /* End if falloff for this gconn type */
         } /* End INHIBBLK loop */
      } /* End celltype loop */
   } /* End prtfoff() */
#endif

/*---------------------------------------------------------------------*
*                 Falloff functions for various cases                  *
*                                                                      *
*  Note that x,y coords passed as args are measured from the ULHC of   *
*  the source repertoire but may be outside that repertoire depending  *
*  on specified boundary conditions.  Coordinates are constrained to   *
*  the ranges:                                                         *
*           -(nr-1) <= x <= (ngx-1) + (nr-1),                          *
*           -(nr-1) <= y <= (ngy-1) + (nr-1).                          *
*                                                                      *
*  Note:  Algorithms are same as those in IBM Assembler version, al-   *
*  lowing for fact that the latter uses 4*(NG[XY]-1+2*(NR-1)) as max   *
*  coords, which are really 4*2=8 times the coords measured from the   *
*  center of the box.  Here tx,ty are double the coords and the code   *
*  for the various metrics doubles them again, so we are net 4*coords  *
*  rather than 8*coords.  Note that roundoff adds 1/4 for the 'R' and  *
*  'S' metrics, so that even halves are rounded down, but adds 2/4     *
*  for the 'D' metric, where all fractions other than exactly 1/2 can  *
*  occur--see lfofftbl.  No rounding in direct calculation cases.      *
*                                                                      *
*  Note:  Separate routines are provided for all the lookup cases so   *
*  all the if's are eliminated.  The calc cases are all consolidated   *
*  to reduce code memory, since speed is not of the essence there.     *
*---------------------------------------------------------------------*/

/* R metric, table lookup */
long foffrtbl(struct INHIBBLK *ib, long x, long y) {
   long tx = labs(x + x - ib->l1x + 1);      /* Twice x */
   long ty = labs(y + y - ib->l1y + 1);      /* Twice y */
   long tr = tx + ty + max(tx,ty) + 1;       /* Add 1 for roundoff */
   return ib->falloff[tr >> 2];
   } /* End foffrtbl */

/* S metric, table lookup */
long foffstbl(struct INHIBBLK *ib, long x, long y) {
   long tx = labs(x + x - ib->l1x + 1);      /* Twice x */
   long ty = labs(y + y - ib->l1y + 1);      /* Twice y */
   long maxxy = max(tx,ty);                  /* Larger of 2x,2y */
   long ts = maxxy + maxxy + 1;              /* Add 1 for roundoff */
   return ib->falloff[ts >> 2];
   } /* End foffstbl */

/* D metric, table lookup */
long foffdtbl(struct INHIBBLK *ib, long x, long y) {
   long tx = labs(x + x - ib->l1x + 1);      /* Twice x */
   long ty = labs(y + y - ib->l1y + 1);      /* Twice y */
   /* Use system sqrt() rather than writing our own integer sqrt()--
   *  may be supported by hardware */
   long td = (long)sqrt((double)((tx*tx + ty*ty)<<2)) + 2;
   return ib->falloff[td >> 2];
   } /* End foffdtbl */

/* All metrics, direct calculation */
long foffcalc(struct INHIBBLK *ib, long x, long y) {
   double dfm,dfv;                           /* Falloff metric,value */
   double dhr = (double)ib->hlfrad;          /* Half-radius */
   long tx = labs(x + x - ib->l1x + 1);      /* Twice x */
   long ty = labs(y + y - ib->l1y + 1);      /* Twice y */
   long maxxy = max(tx,ty);                  /* Larger of 2x,2y */
   /* Determine distance from center according to metric */
   if (ib->kfoff & FOFFRMETRIC)              /* 'R' metric */
      dfm = 0.25*(double)(tx + ty + maxxy);
   else if (ib->kfoff & FOFFSMETRIC)         /* 'S' metric */
      dfm = 0.5*(double)(maxxy);
   else                                      /* 'D' metric */
      dfm = 0.5*sqrt((double)(tx*tx + ty*ty));

   /* Calculate appropriate falloff function at this radius */
   switch (ib->kfoff & 3) {

case FOFF1D:               /* hlfrad/(hlfrad+r) */
      dfv = dhr/(dhr + dfm);
      break;

case FOFF1DSQ:             /* (qr0/(qr0+r))**2, where q=1+sqrt(2) */
      dfv = dhr * (double)SQRT2P1;
      dfv /= (dfv + dfm);
      dfv *= dfv;
      break;

case FOFFEMD:              /* exp(-r) */
      dfv = exp((dfm/dhr)*(double)MLN2);
      break;

case FOFFEMDSQ:            /* exp(-rsq) */
      dfv = dfm/dhr;
      dfv = exp(dfv*dfv*(double)MLN2);
      break;
      } /* End of falloff switch */

   return (long)(dfv*dS28 + 0.5);
   } /* End foffcalc */

/* No falloff */
long foffnone(struct INHIBBLK *ib, long x, long y) {
   return S28;
   } /* End foffnone */

/*---------------------------------------------------------------------*
*                              qfofffnc                                *
*                                                                      *
*  There are 16 possible ways to calculate falloff:  12 defined modes, *
*  3 table lookup cases (where we need to calculate the correct metric *
*  but the falloff function itself is embodied in the table, and none. *
*  Depending on how much we want to optimize things, these 16 cases    *
*  can be provided in various combinations by one or more functions.   *
*  This routine passes to the caller the address of the appropriate    *
*  function, whose body is defined above, and the caller need not be   *
*  aware of whether or not a table is being used.                      *
*---------------------------------------------------------------------*/

FoffFn qfofffnc(struct INHIBBLK *ib) {

   if (ib->ibopt & IBOPFO) {           /* There is falloff */
      if (ib->ibopt & IBOPTT) {           /* Table lookup */
         if (ib->kfoff & FOFFRMETRIC)        /* 'R' metric */
            return foffrtbl;
         else if (ib->kfoff & FOFFSMETRIC)   /* 'S' metric */
            return foffstbl;
         else                                /* 'D' metric */
            return foffdtbl;
         }
      else                                /* Direct calculation */
         return foffcalc;
      } /* End falloff */

   else {                              /* No falloff */
      return foffnone;
      } /* End no falloff */

   } /* End qfofffnc */

