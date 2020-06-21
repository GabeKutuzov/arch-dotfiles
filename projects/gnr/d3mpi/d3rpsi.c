/* (c) Copyright 1997-2007, The Rockefeller University *11115* */
/* $Id: d3rpsi.c 70 2017-01-16 19:27:55Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                              d3rpsi.c                                *
*                                                                      *
*                      Layer print s(i) routine                        *
*      Prints all s(i) and phi(i), generally for diagnostic use        *
*                                                                      *
*   ---This function should be called in parallel from all nodes---    *
*         (Although, currently comp nodes have nothing to do)          *
*                                                                      *
*  Synopsis:   void d3rpsi(struct CELLTYPE *il, int kd)                *
*  Arguments:  'il' is a ptr to the CELLTYPE of the layer              *
*                 whose data are to be printed                         *
*              'kd' is cycle number to print only the data from        *
*                 the current cycle, negative to print data for        *
*                 all axonal delays (call from d3genr())               *
*                                                                      *
************************************************************************
*  V8A, 03/10/97, GNR - New routine based on d3rprt                    *
*  Rev, 01/11/98, GNR - Control rep print at cell type level           *
*  V8C, 03/29/03, GNR - Cell responses in millivolts, add conductances *
*  Rev, 10/12/03, GNR - If ngx*nel < CELLS_PER_LINE, use that number   *
*  V8D, 01/03/07, GNR - Add kd parameter, make switchable per layer    *
*  ==>, 01/14/07, GNR - Last mod before committing to svn repository   *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "d3global.h"
#ifndef PARn
#include "rocks.h"
#include "rkxtra.h"
#endif

#define CELLS_PER_LINE 10     /* Number of cells to print on one line */

/*=====================================================================*
*         Comp node code for d3rpsi--currently nothing to do           *
*=====================================================================*/

#ifdef PARn
void d3rpsi(struct CELLTYPE *il, int kd) {

   } /* End d3rpsi */
#endif

/*=====================================================================*
*       Serial and host code for d3rpsi--print repertoire data         *
* (These data can be found on node 0 for all layers in V8A and later)  *
*=====================================================================*/

#ifndef PARn
void d3rpsi(struct CELLTYPE *il, int kd) {

   static char *label[2] = {
      ", s(i) values",        ", s(i) and phase values"       };
   static char *hdfmt[2] = {
      "(RUC8)",               "(RUC11)"                       };
   /* Cell numbers for titles */
   static char jcell[CELLS_PER_LINE] = { 0,1,2,3,4,5,6,7,8,9 };
#if CELLS_PER_LINE != 10
#error "jcell must be initialized to 0,...,CELLS_PER_LINE-1 "
#endif

   struct REPBLOCK *ir = il->pback;    /* Ptr to parent region */
   s_type *psi;               /* Ptr to s(i) & phase data */
   char *pol,*pole;           /* Ptrs to data, end of output line */
   int  icell;                /* First cell on current line */
   int  idly;                 /* Current delay */
   int  itlen;                /* Item length */
   int  kdly;                 /* Delay switch */
   int  kphs;                 /* Phase switch */
   int  lncells;              /* Actual number of cells per line */
   int  ncells;               /* Number of cells in this line */
   int  ncr;                  /* Number of cells remaining */

   char cycln[13];
   char outln[8+11*CELLS_PER_LINE];
   char delayln[14];

   kdly   = kd < 0 ? il->mxsdelay1-1 : 0;
   kphs   = (int)il->phshft;
   itlen  = 8 + 3*kphs;
   lncells = il->nel * il->pback->ngx;
   if (lncells > CELLS_PER_LINE) lncells = CELLS_PER_LINE;
   ncells = lncells;

   /* Loop over delay times */
   for (idly = kdly; idly >= 0; idly--) {

      psi = il->pps[idly];
      ncr = il->nelt;

      /* Print title line */
      sconvrt(outln, hdfmt[kphs], &ncells, jcell, NULL);
      if (kdly)
         sconvrt(delayln, "(8H, delay J0I5)", &idly, NULL);
      if (kd > 0) {
         sconvrt(cycln, "(7H Cycle J0I5)", &kd, NULL);
         cryout(RK_P4, RP0->stitle, RK_NFSUBTTL+RK_LN4+RP0->lst, cycln,
            RK_CCL, " Region/Repertoire ", RK_LN0+19, ir->sname,
            LSNAME, ", cell type ", RK_CCL, il->lname, LSNAME,
            label[kphs], RK_CCL, delayln, kdly ? RK_CCL : RK_SKIP,
            "    Cell", RK_LN0+8, outln, itlen*ncells, NULL);
         }
      else cryout(RK_P4, "0Region/Repertoire ", RK_NFSUBTTL+RK_LN3+19,
         ir->sname, LSNAME, ", cell type ", RK_CCL, il->lname, LSNAME,
         label[kphs], RK_CCL, delayln, kdly ? RK_CCL : RK_SKIP,
         "    Cell", RK_LN0+8, outln, itlen*ncells, NULL);

      /* Print cell states */
      for (icell=0; icell<il->nelt; icell+=lncells) {
         ncells = min(lncells, ncr);
         wbcdwt(&icell, outln, RK_IORF|RK_NINT|7);
         pol = outln + 8; pole = pol + itlen*ncells;
         for ( ; pol<pole; psi+=il->lsp,pol+=itlen) {
            si16 si;    /* s(i) value aligned for printing */
            d3gtms2(si, psi);
            sconvrt(pol, "(OB7/14IH8.<4,RUC3)",
               &si, &kphs, psi+LSmem, NULL);
            }
         cryout(RK_P4, outln, RK_LN1+(pole-outln), NULL);
         ncr -= lncells;
         }

      } /* End delay time loop */

   /* Restore normal title line */
   cryout(RK_P1, RP0->stitle, RK_NFSUBTTL+RK_LN2+RP0->lst, NULL);

   } /* End d3rpsi */
#endif

