/***********************************************************************
*                              hashtest                                *
*                                                                      *
*  This program tests the hashtbl package by creating 30 entries in a  *
*  table made to hold only 10, thereby assuring that some chains must  *
*  be formed and that the table-expansion code is tested. One in five  *
*  of these entries is deleted, and then they are all looked up.  The  *
*  whole operation is then repeated with pointers to string keys.      *
***********************************************************************/

#define MAIN
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "sysdef.h"
#include "rocks.h"
#include "rksubs.h"
#include "rkhash.h"

#define NMAKE 10
#define NTEST 30
#define LTKEY 10

struct td {
   struct td *ptdn;
   long key;
   long junk;
   char *pstrkey;
   void *plink;
   } tda[NTEST];

/*---------------------------------------------------------------------*
*  Trivial hash function that returns the key as the hash              *
*---------------------------------------------------------------------*/

unsigned long myhash1(void *ptd) {
   return *(long *)ptd;
   }

/*---------------------------------------------------------------------*
*  Slightly less trivial hash function for string keys                 *
*---------------------------------------------------------------------*/
unsigned long myhash2(void *ptxt) {
   char *ptx = (char *)ptxt;
   unsigned long h = 0;
   int s = 0;
   while (*ptx) {
      h ^= *ptx++ << s;
      s = (s + 3) & 15;
      }
   return h;
   }

/*---------------------------------------------------------------------*
*  Main program                                                        *
*---------------------------------------------------------------------*/

int main(void) {

   struct htbl *ph;
   struct td *presult;
   int i;
   int ierr = FALSE;
   char ttk[LTKEY];
   char tdk[NTEST][LTKEY];

   /* Create the keys */
   for (i=0; i<NTEST; i++) {
      ssprintf(tdk[i], "Tkey %4d", i);
      tda[i].key = i*i*i;
      tda[i].junk = i;
      tda[i].pstrkey = tdk[i];
      }

/*---------------------------------------------------------------------*
*  First set of tests--long int keys                                   *
*---------------------------------------------------------------------*/

   /* Create the first hash table */
   ph = hashinit(myhash1, NMAKE, sizeof(long),
      (char *)&((struct td *)0)->key - (char *)0,
      (char *)&((struct td *)0)->plink - (char *)0);

   /* Fill it in, erasing second of every five */
   for (i=0; i<NTEST; i++)
      hashadd(ph, tda+i);
   for (i=2; i<NTEST; i+=5)
      hashdel(ph, tda+i);

   /* Now try looking them up */
   for (i=0; i<NTEST; i++) {
      long key = i*i*i;
      presult = (struct td *)hashlkup(ph, &key);
      if (i % 5 == 2) {
         if (presult) {
            convrt("(P1,'0Lookup 1 returned a value "
               "when NULL was expected for i = ',J0I6)", &i, NULL);
            ierr = TRUE; }
         }
      else {
         if (presult->junk != i) {
            convrt("(P1,'0Lookup 1 for i = ',J0I6,' returned ptr to "
               "data for i = ',J0IL8)", &i, &presult->junk, NULL);
            ierr = TRUE; }
         else if (presult->key != key) {
            convrt("(P1,'0Lookup 1 for i = ',J0I6,' returned ptr with "
               "mismatched key = ',J0IL8,' for data with i = ',J0IL8)",
               &i, &presult->key, &presult->junk, NULL);
            ierr = TRUE; }
         }

      } /* End test loop */

   /* Try to release the thing */
   hashrlse(ph);

/*---------------------------------------------------------------------*
*  Second set of tests--                                               *
*  Data structs have ptrs to NULL-terminated string keys               *
*---------------------------------------------------------------------*/

   /* Create the second hash table */
   ph = hashinit(myhash2, NMAKE, -1,
      (char *)&((struct td *)0)->pstrkey - (char *)0,
      (char *)&((struct td *)0)->plink - (char *)0);

   /* Fill it in, erasing third of every five */
   for (i=0; i<NTEST; i++)
      hashadd(ph, tda+i);
   for (i=3; i<NTEST; i+=5)
      hashdel(ph, tda+i);

   /* Now try looking them up */
   for (i=0; i<NTEST; i++) {
      ssprintf(ttk, "Tkey %4d", i);
      presult = (struct td *)hashlkup(ph, ttk);
      if (i % 5 == 3) {
         if (presult) {
            convrt("(P1,'0Lookup 2 returned a value "
               "when NULL was expected for i = ',J0I6)", &i, NULL);
            ierr = TRUE; }
         }
      else {
         if (presult->junk != i) {
            convrt("(P1,'0Lookup 2 for i = ',J0I6,' returned ptr to "
               "data for i = ',J0IL8)", &i, &presult->junk, NULL);
            ierr = TRUE; }
         else if (strcmp(presult->pstrkey, ttk)) {
            convrt("(P1,'0Lookup 2 for i = ',J1I6,'returned data with "
               "mismatched key = ',A10,', should be ',A10)",
               &i, &presult->pstrkey, ttk, NULL);
            ierr = TRUE; }
         }

      } /* End test loop */

   /* Try to release the thing */
   hashrlse(ph);

   /* Print final word */
   if (ierr)
      cryout(RK_P1, "0Got at least one error.", RK_LN2+RK_FLUSH, NULL);
   else
      cryout(RK_P1, "0All tests passed OK.", RK_LN2+RK_FLUSH, NULL);

   return 0;
   } /* End hashtest() */

