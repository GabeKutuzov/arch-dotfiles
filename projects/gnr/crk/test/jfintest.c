/*--------------------------------------------------------------------*/
/*                                                                    */
/*    JFIND test                                                      */
/*                                                                    */
/*--------------------------------------------------------------------*/

/*
   This test is designed to run interactively.  The program reads a
   control card from the keyboard, then calls jfind to search for the
   word 'key' and prints the value returned.  That's all folks.
*/

#define MAIN
#include <stdio.h>
#include <stdlib.h>
#include "sysdef.h"
#include "rocks.h"
#include "rkxtra.h"

void main() {

   char key[] = {"KEY"};
   char *card;
   int rc;

   cryout(RK_P1, "0==>Type in any input at the prompt.", RK_LN2,
      "   The program should print the location of the word", RK_LN1,
      "   \"KEY\" if found (uppercase) in the data, otherwise 0.",
      RK_LN1, "   Type \"quit\" to quit.", RK_LN1, NULL);

/* Do until terminated */
   for (;;) {                 /* Read loop */
      card = cryin();
      if (!card || (ssmatch(card,"QUIT",4) > 0)) exit(0);
      cdprnt(card);
      rc = jfind(card, key, 1);
      convrt("('0jfind returns ',I6)", &rc, NULL);
      } /* End of forever loop */
   } /* End of program */

