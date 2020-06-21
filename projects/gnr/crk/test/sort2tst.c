/***********************************************************************
*                                                                      *
*                             SORT2 TEST                               *
*                                                                      *
***********************************************************************/

#define MAIN
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include "sysdef.h"
#include "rkarith.h"
#include "rocks.h"

#define NDATA 101
#define NCHKY  11

int main(void) {

   /* Data structures to be sorted */
   struct char_rec {
      struct char_rec *pnxt;
      long junk;
      char key[NCHKY];
      } *pch,*pch0,*pche,**ppch;
   struct fix_rec {
      struct fix_rec *pnxt;
      si32 key;
      si32 other;
      } *pfx,*pfx0,*pfxe,**ppfx;
   struct float_rec {
      struct float_rec *pnxt;
      float key;
      } *pfl,*pfl0,*pfle,**ppfl;
   struct double_rec {
      struct double_rec *pnxt;
      double key;
      } *pdb,*pdb0,*pdbe,**ppdb;
   union all_recs {
      struct char_rec   uchr;
      struct fix_rec    ufxr;
      struct float_rec  uflr;
      struct double_rec udbr;
      } *puar0;

   si32 seed = 776655;
   int i;
   int errct = 0;

/* Allocate data space and work space */

   puar0 = (union all_recs *)calloc(NDATA, sizeof(union all_recs));
   if (!puar0) abexit(559);

   void *work = malloc(512*sizeof(void *));
   if (!work) abexit(559);

/* Test random-length character string */

   /* Construct the linked list */
   pch = (struct char_rec *)puar0;
   ppch = &pch0, pche = pch + NDATA;
   for ( ; pch<pche; ++pch) {
      *ppch = pch, ppch = &pch->pnxt;
      for (i=0; i<NCHKY; ++i)
         pch->key[i] = (char)udev(&seed);
      } /* End linked list construction */
   pch[-1].pnxt = NULL;
   /* Do the sort */
   pche = pch0;
   pch0 = sort2(pch0, work, sizeof(void *)+sizeof(long),
      NCHKY, KST_CHAR);
   /* Check that data are now ascending */
   for (pch=pch0; pch; pch=pch->pnxt) {
      if (!pch->pnxt) break;
      if (memcmp(pch->pnxt->key, pch->key, NCHKY) < 0) {
         printf("Char sort error at item %ld\n", pch-pche);
         ++errct;
         }
      }

/* Test random fixed-point numbers (descending) */

   /* Construct the linked list */
   pfx = (struct fix_rec *)puar0;
   ppfx = &pfx0, pfxe = pfx + NDATA;
   for ( ; pfx<pfxe; ++pfx) {
      *ppfx = pfx, ppfx = &pfx->pnxt;
      /* Make some negative */
      pfx->key = udev(&seed) - (1<<29);
      } /* End linked list construction */
   pch[-1].pnxt = NULL;
   /* Do the sort */
   pfxe = pfx0;
   pfx0 = sort2(pfx0, work, sizeof(void *),
      sizeof(si32), KST_DECR);
   /* Check that data are now descending */
   for (pfx=pfx0; pfx; pfx=pfx->pnxt) {
      if (!pfx->pnxt) break;
      if (pfx->pnxt->key > pfx->key) {
         printf("Fixpt sort error at item %ld\n", pfx-pfxe);
         ++errct;
         }
      }

/* Test all-positive floats */

   /* Construct the linked list */
   pfl = (struct float_rec *)puar0;
   ppfl = &pfl0, pfle = pfl + NDATA;
   for ( ; pfl<pfle; ++pfl) {
      *ppfl = pfl, ppfl = &pfl->pnxt;
      pfl->key = (float)udev(&seed)/(float)(1 << 28);
      } /* End linked list construction */
   pch[-1].pnxt = NULL;
   /* Do the sort */
   pfle = pfl0;
   pfl0 = sort2(pfl0, work, sizeof(void *),
      sizeof(float), KST_APOS|KST_FLOAT);
   /* Check that data are now ascending */
   for (pfl=pfl0; pfl; pfl=pfl->pnxt) {
      if (!pfl->pnxt) break;
      if (pfl->pnxt->key < pfl->key) {
         printf("Float sort error at item %ld\n", pfl-pfle);
         ++errct;
         }
      }

/* Test mixed-sign doubles */

   /* Construct the linked list */
   pdb = (struct double_rec *)puar0;
   ppdb = &pdb0, pdbe = pdb + NDATA;
   for ( ; pdb<pdbe; ++pdb) {
      *ppdb = pdb, ppdb = &pdb->pnxt;
      pdb->key = (double)(udev(&seed)-(1<<29)) *
         (double)udev(&seed) * 1E-20;
      } /* End linked list construction */
   pch[-1].pnxt = NULL;
   /* Do the sort */
   pdbe = pdb0;
   /* Note:  Setting okeys = sizeof(double) here is a bit of a
   *  kludge:  in a 64-bit system, this is the same as the size
   *  of the pnxt pointer that is realy there; in a 32-bit system,
   *  this will account for the padding inserted between the
   *  pointer and the double.  */
   pdb0 = sort2(pdb0, work, sizeof(double),
      sizeof(double), KST_FLOAT);
   /* Check that data are now ascending */
   for (pdb=pdb0; pdb; pdb=pdb->pnxt) {
      if (!pdb->pnxt) break;
      if (pdb->pnxt->key < pdb->key) {
         printf("Double sort error at item %ld\n", pdb-pdbe);
         ++errct;
         }
      }

/* All done, print error count and clean up */

   printf("Error count for sort2 test is %d\n", errct);
   free(puar0);
   free(work);
   return 0;
   } /* End SORTTEST */


