/* (c) Copyright 2016, The Rockefeller University */
/* $Id: copptest.c 17 2016-06-30 20:47:06Z  $ */
/***********************************************************************
*                       MPI tools test program                         *
*                              COPPTEST                                *
*                                                                      *
*  This is the test program for the coppack() package.  It sets up     *
*  enough data to fill several message bufferes, with various types    *
*  and combining operations, performs packing and checks the results.  *
*                                                                      *
*  N.B.  Link with crk version of printf, not libc printf().           *
*  There is no serial version of this program.                         *
*                                                                      *
*  N.B.  An attempt was made to use run-time host-vs-comp node tests,  *
*  but this idea fails because the allocpxx machinery needs separate   *
*  PAR0 vs PARn executables, so this was changed.  As a side effect,   *
*  we no longer have the whole rkprintf/cryout machinery on comp nodes.*
*  Also note that ptrs to allocated areas must be included in an area  *
*  that is the first handled by membcst so the translated pointers are *
*  available on comp nodes.                                            *
*----------------------------------------------------------------------*
*  V1A, 08/25/16, GNR - New routine                                    *
*  V1B, 09/02/16, GNR - Make separate PAR0 and PARn routines           *
***********************************************************************/

#define MAIN

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "rocks.h"
#include "rkxtra.h"
#include "rkarith.h"
#include "mpitools.h"
#include "mempools.h"
#include "collect.h"
#include "copptest.h"
#include "ctnxdr.h"

#ifndef PAR
#error Do not attempt to compile a serial version of copptest.
#endif

#define NREP   5        /* Number errors of each type to report */

int ntotal = 0;         /* Total error count */
int ndble = 0;          /* double errors reported */
int nui32 = 0;          /* ui32 errors reported */
int nflt  = 0;          /* float errors reported */
int nwide = 0;          /* long long errors reported */
int nlong = 0;          /* long errors reported */

/*=====================================================================*
*                      Result-checking routines                        *
*=====================================================================*/

#ifndef PARn
static void dchk(char *name, double val, double cor, int i) {
   if (val != cor && ++ndble <= NREP)
      printf("double %s[%d] is %q, correct is %q\n",
         name, i, val, cor);
   return;
   }

static void uchk(char *name, ui32 val, ui32 cor, int i) {
   if (val != cor && ++nui32 <= NREP)
      printf("ui32 %s[%d] is %jd, correct is %jd\n",
         name, i, val, cor);
   }

static void fchk(char *name, float val, float cor, int i) {
   if (val != cor && ++nflt <= NREP)
      printf("float %s[%d] is %f, correct is %f\n",
         name, i, val, cor);
   }

static void wchk(char *name, si64 val, si64 cor, int i) {
   if (val != cor && ++ndble <= NREP)
      printf("wide %s[%d] is %wd, correct is %wd\n",
         name, i, val, cor);
   }

static void lchk(char *name, long val, long cor, int i) {
   if (val != cor && ++ndble <= NREP)
      printf("long %s[%d] is %ld, correct is %ld\n",
         name, i, val, cor);
   }
#endif


/*=====================================================================*
*                            copptest main                             *
*=====================================================================*/
int main(void) {

   struct bcstptrs *BPTRS;
   struct ctst1 *pct1;
   struct ctst2 *pct2;
   CPKD *pcpk;
   si32 seed;
   int i,j,k;

   /* COPDEFs */
   COPD dcop1[6] = {
      { DOUBLE+ADD, 50 },
      { DOUBLE+MAX, 50 },
      { DOUBLE+ADD, 50 },
      { DOUBLE+MAX, 50 },
      { DOUBLE+ADD, 50 },
      { DOUBLE+MAX, 50 } };
   COPD dcop2[3] = {
      { (2<<RepShft) + REPEAT, 3 },
      { DOUBLE+ADD, 50 },
      { DOUBLE+MAX, 50 } };
   COPD ucop1[4] = {
      { (3<<RepShft) + REPEAT, 4 },
      { UI32V+ADD, 100 },
      { UI32V+LOR, 100 },
      { UI32V+NOOP,100 } };
   COPD fcop1[3] = {
      { (2<<RepShft) + REPEAT, szfdat/3 },
      { FLOAT+ADD, 2 },
      { FLOAT+MIN, 1 } };
   COPD wcop1[4] = {
      { LLONG+ADD, 3 },
      { LLONG+MAX, 3 },
      { LLONG+MIN, 3 },
      { LLONG+LOR, 3 } };
   COPD lcop1[2] = {
      { LONG+ADD, 100 },
      { LONG+MAX, 100 } };

#if 1
   aninit(ANI_DBMSG,0,0);
#else
   aninit(ANI_DBMSG,DBG_DBGWT+0x100,0);
#endif
   nxtupush(CPTTT, CPTUT);

#ifdef PAR0
   printf("Running copptest with %d nodes\n", NC.total);

/* Use memshare system to allocate data space */

   BPTRS = (struct bcstptrs *)allocpmv(Static, IXstr_bcstptrs, "BPTRS");
   BPTRS->bctst1 = (struct ctst1 *)allocpmv(Private, IXstr_ctst1, "CTST1");
   BPTRS->bctst2 = (struct ctst2 *)allocpmv(Private, IXstr_ctst2, "CTST2");
#endif

   /* Ah, the trick that gets all the pointers translated */
   BPTRS = membcst(MPS_Static|MPS_Private);
   pct1 = BPTRS->bctst1;
   pct2 = BPTRS->bctst2;

#ifdef PAR0
   printf("memsize reports %zd bytes allocated\n",
      memsize(MPS_Private));
   memset(pct1, 0, sizeof(struct ctst1));
   memset(pct2, 0, sizeof(struct ctst2));

/* Fill data with known values */

#else
   for (i=0; i<szddat; ++i)
      pct1->ddat1[i] = (double)(i*NC.cnodes + NC.node);
   for (i=0; i<szudat; ++i)
      pct1->udat1[i] = (ui32)(i*NC.node);
   for (i=0; i<szfdat; ++i)
      pct2->fdat2[i] = (float)(i*NC.cnodes - NC.node);
   seed = NC.node;
   for (i=0; i<szwdat; ++i)
      pct2->wdat2[i] = jesl(udev(&seed));
   for (i=0; i<szldat; ++i)
      pct2->ldat2[i] = (long)(2*i*NC.cnodes + NC.node);
#endif

/* Perform statistical collection on these sets */

/* Run this with 500 and 0 as second arg of coppini() */
   pcpk = coppini(CPPACK_MSG, 0, 0);

   /* Double data:  Do 3 alt groups of 50 add, 50 max
   *  defined in one COPDEF, then 3 more pairs defined with a
   *  repeat count, then 4 more pairs in sep COPDEFs */
   coppack(pcpk, pct1->ddat1, dcop1, 6);
   coppack(pcpk, pct1->ddat1 + 300, dcop2, 3);
   for (i=600; i<szddat; i+=100) {
      coppack(pcpk, pct1->ddat1+i, dcop2+1, 1);
      coppack(pcpk, NULL, dcop2+2, 1);
      }

   /* ui32 data:  Do 4 sets of 300 operations, ADD, LOR, NOOP */
   coppack(pcpk, pct1->udat1, ucop1, 4);

   /* Float data:  Here I put an odd number of items to make
   *  sure alignment is handled correctly for the following
   *  wide data.  Otherwise, we'll just check the ADD and MIN
   *  operations in groups of 3 numbers.  */
   coppack(pcpk, pct2->fdat2, fcop1, 3);

   /* Wide data:  100 groups of ADD, MAX, MIN, LOR sequences */
   for (i=0; i<szwdat; i+=12)
      coppack(pcpk, pct2->wdat2+i, wcop1, 4);

   /* Long data:  A sequence with NULL pointers.  If we ever
   *  go back to 32-bit systems, this should handle that case.  */
   coppack(pcpk, pct2->ldat2, lcop1, 2);
   coppack(pcpk, pct2->ldat2+200, lcop1, 2);
   coppack(pcpk, NULL, lcop1, 2);
   coppack(pcpk, NULL, lcop1, 2);
   coppack(pcpk, NULL, lcop1, 2);
   coppack(pcpk, NULL, lcop1, 2);

/* Check the results on the host node */

   copcomb(pcpk);

#ifdef PAR0
      /* double data */
   {  double dcnode2 = (double)(NC.cnodes * NC.cnodes);
      double dnsum = (double)(NC.tailnode*(NC.tailnode+1) -
         NC.headnode*(NC.headnode-1))/2;
      float fcnode2 = (float)(NC.cnodes * NC.cnodes);
      float fnsum = (float)(NC.tailnode*(NC.tailnode+1) -
         NC.headnode*(NC.headnode-1))/2;
      long lcnode2 = (long)(NC.cnodes * NC.cnodes);
      long lnsum = (long)(NC.tailnode*(NC.tailnode+1) -
         NC.headnode*(NC.headnode-1))/2;
      ui32 unsum = (ui32)(NC.tailnode*(NC.tailnode+1) -
         NC.headnode*(NC.headnode-1))/2;
      for (j=0; j<szddat; j+=100) {
         for (i=j; i<j+50; i++)
            dchk("sum ddat1", pct1->ddat1[i],
               (double)i*dcnode2 + dnsum, i);
         for ( ; i<j+100; i++)
            dchk("max ddat1", pct1->ddat1[i],
               (double)i*NC.cnodes + NC.tailnode, i);
         }

      /* ui32 data */
      for (j=0; j<szudat; j+=300) {
         for (i=j; i<j+100; i++)
            uchk("sum udat1", pct1->udat1[i], i*unsum, i);
         for ( ; i<j+200; i++) {
            ui32 ulor = 0;
            for (k=NC.headnode; k<=NC.tailnode; k++) ulor |= i*k;
            uchk("lor udat1", pct1->udat1[i], ulor, i);
            }
         for ( ; i<j+300; i++)
            uchk("noop udat1", pct1->udat1[i], i*NC.headnode, i);
         }

      /* float data */
      for (j=0; j<szfdat; j+=3) {
         fchk("add fdat2", pct2->fdat2[j],
            (float)j*fcnode2 - fnsum, j);
         fchk("add fdat2", pct2->fdat2[j+1],
            (float)(j+1)*fcnode2 - fnsum, j+1);
         fchk("min fdat2", pct2->fdat2[j+2],
            (float)(j+2)*NC.cnodes - NC.tailnode, j+2);
         }

      /* wide data */
      {  si32 *seeds = (si32 *)malloc(NC.cnodes*sizeof(si32));
         for (k=0; k<NC.cnodes; k++)
            seeds[k] = NC.headnode + k;
         for (j=0; j<szwdat; j+=12) {
            for (i=j; i<j+3; i++) {
               si64 wsum = jesl(0);
               for (k=0; k<NC.cnodes; k++)
                  wsum = jasl(wsum, udev(seeds+k));
               wchk("add wdat2", pct2->wdat2[i], wsum, i);
               }
            for ( ; i<j+6; i++) {
               si32 uk, tmax = 0;
               for (k=0; k<NC.cnodes; k++) {
                  uk = udev(seeds+k);
                  if (uk > tmax) tmax = uk;
                  }
               wchk("max wdat2", pct2->wdat2[i], jesl(tmax), i);
               }
            for ( ; i<j+9; i++) {
               si32 uk, tmin = SI32_MAX;
               for (k=0; k<NC.cnodes; k++) {
                  uk = udev(seeds+k);
                  if (uk < tmin) tmin = uk;
                  }
               wchk("min wdat2", pct2->wdat2[i], jesl(tmin), i);
               }
            for ( ; i<j+12; i++) {
               si32 tlor = 0;
               for (k=0; k<NC.cnodes; k++)
                  tlor |= udev(seeds+k);
               wchk("lor wdat2", pct2->wdat2[i], jesl(tlor), i);
               }
            }  /* End j loop */
         free(seeds);
         } /* End wdat local scope */

      /* long data */
      for (j=0; j<szldat; j+=200) {
         for (i=j; i<j+100; i++)
            lchk("sum ldat2", pct2->ldat2[i],
               2*(long)i*lcnode2 + lnsum, i);
         for ( ; i<j+200; i++)
            lchk("max ldat2", pct2->ldat2[i],
               2*(long)i*NC.cnodes + NC.tailnode, i);
         }
      } /* End dcnode2 etc. local scope */

      ntotal = ndble + nui32 + nflt + nwide + nlong;
      printf("==>Total errors %d\n", ntotal);
      cryout(RK_P1, "0End copptest tests", RK_FLUSH+RK_LN2, NULL);
#endif

   copfree(pcpk);
   appexit(NULL,0,0);
   return 0;
   } /* End copptest() */
