/*--------------------------------------------------------------------*/
/*                                                                    */
/*    BCDIN, IBCDIN, UBCDIN TEST                                      */
/*                                                                    */
/* Rev, 03/15/08, GNR - Use cryout for output, real ermark, but       */
/*                      sprintf for independent conversions.          */
/*--------------------------------------------------------------------*/

/* This test is designed to run interactively.  The program reads
   commands from the keyboard to configure the translation codes,
   then calls all three routines, BCDIN, IBCDIN, and UBCDIN using
   the same codes.  The results are displayed on the screen and
   the program then waits for more input.  The commands are:
      a ss     Set input (argument) string to ss
      c hhhh   Set command to hex code
      d dd     Set decimal code to dd
      e dd     Set expwid to dd
      f        Toggle RK_DSF (decimal scale force) bit
      g        Go
      q        Quit
      s dd     Set scale to dd
      w dd     Set width to dd
 */

#define MAIN
#include "sysdef.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#ifdef TURBOC
#include <conio.h>
#endif
#include "rocks.h"
#include "rkxtra.h"

int main() {

   char buf[33];
   char cmd[81];
   long ic;
   int scale;
   int expwid;
   int dec;
   int force;
   unsigned int op;
   int width;
   double arg;
   long iarg;
   short sarg;
   unsigned long uarg;
   unsigned short usarg;

/* Set initial field and ic code */
   memset(buf,' ',32); buf[32] = '\0';
   scale = 0;
   expwid = 0;
   dec = 0;
   force = 0;
   op = 0x0600;
   width = 15;

/* Do until terminated */
   nopage(RK_PGNONE);
   for (;;) {
#ifdef TURBOC
      putch('?');
#else
      putchar('?');
      fflush(stdout);
#endif
      gets(cmd);
      switch (toupper((int)cmd[0])) {
         case 'A' :
            strcpy(buf,cmd+2); break;
         case 'C' :
            sscanf(cmd,"%*c%x",&op); break;
         case 'D' :
            sscanf(cmd,"%*c%d",&dec); break;
         case 'E' :
            sscanf(cmd,"%*c%d",&expwid); break;
         case 'F' :
            force ^= 128; break;
         case 'G' :
            ic = 16777216*(scale&31) + 1048576*(expwid&7) +
               65536*(force + (dec&127)) + op + (width&31);
            sprintf(cmd, "0%s, ic = %8.8lX", buf, ic);
            cryout(RK_P1, cmd, RK_LN2, NULL);
            arg = bcdin(ic,buf);
            sprintf(cmd, " BCDIN result = %G", arg);
            cryout(RK_P1, cmd, RK_LN1, NULL);
            iarg = ibcdin(ic,buf);
            arg = (double)iarg/(double)(1UL << scale);
            sarg = (short)iarg;
            sprintf(cmd, " IBCDIN result = %G, short = %d", arg, sarg);
            cryout(RK_P1, cmd, RK_LN1, NULL);
            uarg = ubcdin(ic,buf);
            arg = (double)uarg/(double)(1UL << scale);
            usarg = (unsigned short)uarg;
            sprintf(cmd, " UBCDIN result = %G, ushort = %ld", arg,
               (unsigned long)usarg);
            cryout(RK_P1, cmd, RK_LN1|RK_FLUSH, NULL);
            break;
         case 'Q' :
            exit(0);
         case 'S' :
            sscanf(cmd,"%*c%d",&scale); break;
         case 'W' :
            sscanf(cmd,"%*c%d",&width); break;
         default:
            puts("Illegal command");
         } /* End of switch */
      } /* End of forever loop */

   return 0;
   } /* End of program */
