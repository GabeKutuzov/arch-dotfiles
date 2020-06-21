/*--------------------------------------------------------------------*/
/*                                                                    */
/*    SSMATCH TEST                                                    */
/*                                                                    */
/*--------------------------------------------------------------------*/

/* This test is designed to run interactively.  The program reads
   lines from the keyboard containing a test item, a key, and a
   minimum match count.  Then ssmatch is called and the return code
   is displayed.  Type 'ESC' or 'Q' to quit.  That's all there is. */

#define  MAIN
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "rocks.h"
#include "rkxtra.h"

#ifndef PC
#include <ctype.h>
char *strupr(char *s) {
   char *t = s;
   while (*t) { *t = toupper(*t); ++t; }
   return s;
   } /* End strupr() */
#endif

/* abexit -- ssmatch calls it, we don't want full rocks lib here */
void abexit(int code) {
   printf("-->abexit called with code %i\n", code);
   exit(code);
   }

int main() {

   char itm[33];
   char key[33];
   char cmd[81];
   int mnc;
   int ic;

   printf("\n0==>At the prompt, enter a test item, a key,\n"
      "    and a minimum match count.  The return code from\n"
      "    ssmatch will be displayed.  Type ESC or Q to quit.\n");

/* Do until terminated */

   for (;;) {
      gets(cmd);
      strupr(cmd);
      if (cmd[0] == '\x1B' || cmd[0] == 'Q') exit (0);
      sscanf(cmd,"%s%s%d",&itm,&key,&mnc);
      ic = ssmatch(itm,key,mnc);
      printf("ssmatch returned %d\n",ic);
      } /* End of forever loop */
   return 0;
   } /* End of program */

