/**********************************************************************
*                                                                     *
*                             SORT TEST                               *
*                                                                     *
**********************************************************************/

#define MAIN
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "sysdef.h"
#include "rocks.h"

int main(void) {

   struct rec_def {           /* These will be sorted */
      struct rec_def * nextrec;
      long key;
      long other;
      } ;

   long values[] = {          /* These will be the keys */
      18993, 467, 28739643, 11, 943, 15, 999, 65536 };

   struct rec_def *datalist;
   struct rec_def *next;
   struct rec_def *this;
   int irec;

/* Construct the linked list */

   for (irec=0; irec<8; irec++) {
      this = (struct rec_def *)malloc(sizeof(struct rec_def));
      if (irec == 0) datalist = this;
      else           next->nextrec = this;
      this->key = values[irec];
      this->other = -1;
      next = this;
      } /* End linked list construction */
   next->nextrec = NULL;

/* OK, here goes, folks */

   next = (struct rec_def *)
      sort(datalist,sizeof(void *),2*sizeof(long),0);

/* Print the results */

   printf("Results of sort on 8 values:\n");
   for (this=next; this; this=this->nextrec)
      printf("%ld\n",this->key) ;
   return 0;
   } /* End SORTTEST */


