/*--------------------------------------------------------------------*/
/*                                                                    */
/*    BCDOUT, IBCDWT, AND UBCDWT TEST                                 */
/*                                                                    */
/*--------------------------------------------------------------------*/

/* This test is designed to run interactively.  The program reads
   commands from the keyboard to configure the argument and trans-
   lation codes, then calls the three routines BCDOUT, IBCDWT, and
   UBCDWT using the same arg (converted to fixed point for IBCDWT and
   UBCDWT) and same codes.  The results are displayed on the screen
   and the program then waits for more input.  The commands are:
      a ff     Set argument to floating point value
      c hhhh   Set command to hex code
      d dd     Set decimal code to dd
      e dd     Set expwid to dd
      g        Go
      q        Quit
      s dd     Set scale to dd
      w dd     Set width to dd
 */

#define  MAIN
#include <stdio.h>
#include <stdlib.h>
#ifdef NN
#include <stddef.h>
#endif
#ifdef PC
#include <conio.h>
#define PUTCHAR(c) putch(c)
#else
#define PUTCHAR(c) (putchar(c), fflush(stdout))
#endif
#include <ctype.h>
#include <string.h>
#include "sysdef.h"
#include "rocks.h"
#include "rkxtra.h"

/* Catch abexit calls so test can continue */
void abexit(int code) {

   printf("0***Test issued abexit code %d, continuing\n", code);

   }

int main() {

   char buf[33];
   char cmd[81];
   long ic;
   int scale;
   int expwid;
   int dec;
   unsigned int op;
   int width;
   double arg;

/* Set initial arg and ic code */
   arg = 1.0;
   scale = 0;
   expwid = 0;
   dec = 0;
   op = 0x0600;
   width = 15;
 
/* Do until terminated */
   for (;;) {
      PUTCHAR('?');
      gets(cmd);
      switch (toupper((int)cmd[0])) {
         case 'A' :
            sscanf(cmd,"%*c%le",&arg); break;
         case 'C' :
            sscanf(cmd,"%*c%x",&op); break;
         case 'D' :
            sscanf(cmd,"%*c%d",&dec); break;
         case 'E' :
            sscanf(cmd,"%*c%d",&expwid); break;
         case 'G' :
            ic = 16777216*(scale&31) + 1048576*(expwid&7) + 65536*(dec&15)
               + op + width;
            memset(buf,' ',32);
            buf[32] = '\0';
            buf[width] = '|';
#ifndef NN
            printf("Arg = %G, ic = %8.8lX\n                    %s\n",arg,ic,buf);
#else
            printf("Arg = %g, ic = %8.8lX\n                    %s\n",arg,ic,buf);
#endif
            bcdout(ic,buf,arg);
            printf("Result from bcdout: %s\n",buf);
            ibcdwt(ic,buf,(long)(arg*(double)(1UL<<scale)));
            printf("Result from ibcdwt: %s\n",buf);
            ubcdwt(ic,buf,(unsigned long)(arg*(double)(1UL<<scale)));
            printf("Result from ubcdwt: %s\n",buf);
#ifndef PC
            fflush(stdout);
#endif
            break;
         case 'Q' :
            exit(0);
         case 'S' :
            sscanf(cmd,"%*c%d",&scale); break;
         case 'W' :
            sscanf(cmd,"%*c%d",&width); break;
         default:
            puts("Illegal command");
#ifndef PC
            fflush(stdout);
#endif
         } /* End of switch */
      } /* End of forever loop */
   return 0;
   } /* End of program */



