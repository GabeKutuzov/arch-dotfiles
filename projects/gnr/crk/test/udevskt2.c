/*--------------------------------------------------------------------*/
/*                                                                    */
/*    UDEVSKIP interactive test                                       */
/*    V1A, 12/10/93, GNR                                              */
/*--------------------------------------------------------------------*/

/*
   This test is designed to run interactively.  The program reads
   two numbers from the keyboard, prints them, then calls udevskip
   with the first number as seed and the second as skip count.  The
   results are also calculated by calling udev repeatedly, and both
   are printed.  Type "QUIT" to exit.
*/

#define MAIN
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "sysdef.h"
#include "rocks.h"
#include "rkxtra.h"
#include "rkarith.h"

void main() {

   char *card;
   long seed,nskip;
   long seed1,iskip;

   cryout(RK_P1, "==>At the prompt, type two numbers, a seed", RK_LN2,
      "    and a skip count.  The resuts of calculating the", RK_LN1,
      "    new seed by udevskip and by multiple calls to udev", RK_LN1,
      "    will be printed.  Type \"quit\" to quit.", RK_LN1, NULL);

/* Do until terminated */
   for (;;) {                 /* Read loop */
      card = cryin();
      if (!card || (ssmatch(card,"QUIT",4) > 0)) exit(0);
      inform("(S0,2*IL)",&seed,&nskip,NULL);
      convrt("(W,'0For seed = ',IL12,', nskip = ',IL12',')",
         &seed,&nskip,NULL);
      seed1 = seed;
      udevskip(&seed1,nskip);
      convrt("(W,'   udevskip gives  ',IL12)",&seed1,NULL);
      for (iskip=0; iskip<nskip; iskip++) udev(&seed);
      convrt("(W,'   nskip udevs give',IL12/)",&seed,NULL);
      cryout(RK_P1,"\0",RK_LN0+RK_FLUSH+1,NULL);
      } /* End of forever loop */
   } /* End of program */

