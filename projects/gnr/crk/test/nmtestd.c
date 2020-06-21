/* (c) Copyright 2005, George N. Reeke Jr. */
/***********************************************************************
*              Test program for Nelder-Mead optimization               *
*           (double-precision, without simulated annealing)            *
***********************************************************************/

#define MAIN

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include "sysdef.h"
#include "rocks.h"
#include "rkxtra.h"
#include "rksubs.h"
#include "rkarith.h"
#include "nelmeadd.h"

#define Ndim   3        /* Dimensionality of the test problem */
#define X      0
#define Y      1
#define Z      2

/* Here's a little cost function--just quadratic */
#define Sing  0.01
#define Xctr  0.1
#define Yctr  0.2
#define Zctr  0.3

nmxyd myfn(nmxyd *v, void *usrd) {
   if (fabs(v[X] - 0.5) < Sing &&
       fabs(v[Y] - 0.5) < Sing &&
      fabs(v[Z] - 0.5) < Sing) return NM_SING;
   return (v[X] - Xctr)*(v[X] - Xctr) +
          (v[Y] - Yctr)*(v[Y] - Yctr) +
          (v[Z] - Zctr)*(v[Z] - Zctr);
   } /* End cost function */

/*--------------------------------------------------------------------*/

int main(void) {

   struct nmglbld *nmG;
   struct nmhistd *phist;
   nmxyd *pxbest,ybest;
   nmxyd *v0,*v;
   ui32 niter,*pstat;
   int nm;

   cryout(RK_P3, "0Begin Nelder-Mead (double) optimization test",
      RK_LN2, NULL);

   nmG = nmallod(Ndim, 0, myfn);
   v = v0 = nmgetxd(nmG);

   /* Build initial simplex */
   v[X] = 0.0, v[Y] = 0.0, v[Z] = 0.0;
   v += Ndim;
   v[X] = 1.0, v[Y] = 1.0, v[Z] = 0.0;
   v += Ndim;
   v[X] = 1.0, v[Y] = 0.0, v[Z] = 1.0;
   v += Ndim;
   v[X] = 0.0, v[Y] = 1.0, v[Z] = 1.0;

   ybest = nminitd(nmG, NULL, "nelmead test",
      NMKA_MOVE | NMKP_EVRY | NMKP_DETAIL);

   convrt("(P3,'0The best initial function value is ',J0D12.6)",
      &ybest, NULL);

   nm = nmitrnad(nmG, NULL, &pxbest, &ybest, &niter,
      1E-4, 1E-4, 1000);

   convrt("(P3,'0Refinement returned code ',J0I6)",
      &nm, NULL);
   convrt("(P3,' Solution found was at x = ',J0Q12.6,' y = ',J0Q12.6,"
      "' z = ',J0Q12.6)", &pxbest[X], &pxbest[Y], &pxbest[Z], NULL);
   convrt("(P3,' Function value at solution was ',J0D12.6)",
      &ybest, NULL);
   if ((phist = nmgethd(nmG)) == NULL)
      cryout(RK_P3, " No history was returned, as requested",
         RK_LN1, NULL);
   else
      convrt("(P3,' History was erroneously returned at ', J0ZL16)",
         &phist, NULL);

   pstat = nmgetsd(nmG);
   cryout(RK_P3, "0Statistics are as follows:", RK_LN2, NULL);
   convrt("(P3,'   NMIAV  ',IJ8,'   NMIAVM  ',IJ8)",
      &pstat[NMS_NMIAV], &pstat[NMS_NMIAVM], NULL);
   convrt("(P3,'   REFAV  ',IJ8,'   REFACC  ',IJ8,'   REFBEST  ',IJ8)",
      &pstat[NMS_REFAV], &pstat[NMS_REFACC], &pstat[NMS_REFBEST], NULL);
   convrt("(P3,'   EXPAV  ',IJ8,'   EXPACC  ',IJ8,'   EXPBEST  ',IJ8)",
      &pstat[NMS_EXPAV], &pstat[NMS_EXPACC], &pstat[NMS_EXPBEST], NULL);
   convrt("(P3,'   OSCAV  ',IJ8,'   OSCACC  ',IJ8,'   OSCBEST  ',IJ8)",
      &pstat[NMS_OSCAV], &pstat[NMS_OSCACC], &pstat[NMS_OSCBEST], NULL);
   convrt("(P3,'   INSAV  ',IJ8,'   INSACC  ',IJ8,'   INSBEST  ',IJ8)",
      &pstat[NMS_INSAV], &pstat[NMS_INSACC], &pstat[NMS_INSBEST], NULL);
   convrt("(P3,'   SHRAV  ',IJ8,'   SHRAVM  ',IJ8,'   SHRBEST  ',IJ8)",
      &pstat[NMS_SHRAV], &pstat[NMS_SHRAVM], &pstat[NMS_SHRBEST], NULL);
   convrt("(P3,'   SHRINK ',IJ8,'   POINT   ',IJ8,'   CONVG    ',IJ8W)",
      &pstat[NMS_SHRINK], &pstat[NMS_POINT], &pstat[NMS_CONVG], NULL);

   return 0;
   } /* End nmtestf program */
