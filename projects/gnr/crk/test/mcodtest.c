/*--------------------------------------------------------------------*/
/*                                                                    */
/*    MCODES and MCODPRT test                                         */
/*                                                                    */
/*--------------------------------------------------------------------*/

/*
   This test is designed to run interactively.  The program reads a
   control card from the keyboard, which is expected to contain a
   data string followed by a key string.  Function mcodes is called
   to interpret the data and the flagword and return code are
   printed.
*/

#define MAIN
#include <stdio.h>
#include <stdlib.h>
#include "sysdef.h"
#include "rocks.h"
#include "rkxtra.h"

void main() {

   char data[33];
   char keys[33];
   char *card;
   unsigned long flagword;
   int mcoderet;

   cryout(RK_P1, "0==>At each prompt, type in a data string "
      "followed by a key string.", RK_LN4,
                 "    The program initializes the output flags "
      "to a value of 0X1009 and", RK_LN0,
                 "    then prints the modified flagword "
      "and return code from mcodes.", RK_LN0,
                 "    Type \"quit\" to quit.", RK_LN1, NULL);

   for (;;) {                 /* Read loop */
      card = cryin();
      if (!card || (ssmatch(card,"QUIT",4) > 0)) exit(0);
      cdprnt(card);
      cdscan(card,0,32,0);
      scan(data,RK_NEWKEY+RK_REQFLD);
      scan(keys,RK_NEWKEY+RK_REQFLD);
      flagword = 0x1009;      /* Marker for null action by mcodes */
      mcoderet = mcodes(data,keys,&flagword);
      lines(1);
      printf("\nmcodes returned %d with flag = %lX\n",mcoderet,flagword);
      printf("mcodprt returned %s\n",mcodprt(flagword,keys,32));
      } /* End of forever loop */
   } /* End of program */

