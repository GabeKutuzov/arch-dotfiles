/**********************************************************************
*                                                                     *
*                             RANDTEST.C                              *
*                                                                     *
**********************************************************************/

/* A little quick-and-dirty test of a problem with Lij in Cns */

#define MAIN
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "sysdef.h"
#include "rocks.h"
#include "rkarith.h"

#define NUM    10

int main() {

   long Lij;
   long seed = 289985;
   long nsrc = 512;
   int i;

   cryout(RK_P2,"0UDEV Lij results:",RK_SUBTTL+RK_LN2,NULL);

   for (i=0; i<NUM; i++) {
      Lij = jm64nh(udev(&seed),2*nsrc);
      convrt("(IL9)",&Lij,NULL); }
   cryout(RK_P2,"\0",1+RK_FLUSH,NULL);

   return 0;
   } /* End RANDTEST */


