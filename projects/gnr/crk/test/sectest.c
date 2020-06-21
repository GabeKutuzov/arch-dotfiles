/*--------------------------------------------------------------------*/
/*                                                                    */
/*    SECTEST C -- Test timer routine                                 */
/*                                                                    */
/*    V1A, 03/16/89, G. N. Reeke                                      */
/*    Rev, 04/08/14, GNR - Return int, ran & seed long --> si32       */
/*--------------------------------------------------------------------*/

/* Include standard library functions */

#define MAIN
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "sysdef.h"
#include "rocks.h"
#include "rkarith.h"

#define NREP 10000

int main(void) {

/* Declarations: */

   double tret;
   si32 ran[10000];
   si32 seed;
   int irep;

/* Start clock and print initial time */

   tret = second();
   printf("Initial time %f\n",tret);
   seed = 1009;

/* Calculate 10000 random numbers to waste some time,
   then read clock and print again */

   for (irep=0; irep<NREP; ++irep) 
      rannum(ran,10000,&seed,0);
   tret = second();
   printf("Time after 10,000 * 10,000 rannums %f\n",tret);

/* Do it again */

   for (irep=0; irep<NREP; ++irep) 
      rannum(ran,10000,&seed,0);
   tret = second();
   printf("Time after 2 * 10,000 * 10,000 rannums %f\n",tret);

   return 0;
   } /* End sectest */
