/**********************************************************************
*                       Test the PXQ routines                         *
**********************************************************************/

#define MAIN
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "sysdef.h"
#include "rocks.h"

int main() {

   int its,itr,k;             /* Test counters */
   char string[CDSIZE+1];     /* Receives interactive commands */

   cryout(RK_P2,"0PXQ Test Begins...",RK_LN2,NULL);
   cryout(RK_P2,"0Interrupt using ctrl-brk and type in something.",
      RK_LN2,NULL);
   cryout(RK_P2,"   (You have 8 seconds to type in each trial.)",
      RK_LN1,NULL);
   cryout(RK_P2," (PXQ is inactivated after 10 trials)",RK_LN1,NULL);

   /* Fill test string with garbage to be sure it gets written over */
   memcpy(string,"ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789++++",40);
   memcpy(string+40,"ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789----",40);
   string[80] = '\0';
   k = pxon();
   convrt("(' PXON returned ',ji8)",&k,NULL);
   cryout(RK_P1,"\0",RK_LN0+RK_FLUSH+1,NULL);

   for (its=1;its<=2;its++) {

      for (itr=1;itr<=10;itr++) {
         /* rksleep(8,0); */
         si32 rand = 1001;
         int itime;
         fwrite(" Running udev timer\n", 20, 1,stdout);
         fflush(stdout);
         for (itime=0; itime<50000000; ++itime) udev(&rand);
         fwrite(" Calling pxq\n",13,1,stdout);
         fflush(stdout);
         k = pxq(string,CDSIZE);
         convrt("(5H NC =I4,10H, STRING =/H A80)",&k,string,NULL);
         cryout(RK_P1,"\0",RK_LN0+RK_FLUSH+1,NULL);
         } /* End loop over 10 trials */

      if (its == 2) {
         cryout(RK_P2,"0PXQ disabled...",RK_LN2,NULL);
         pxoff();
         }
      } /* End loop over two test conditions */
   cryout(RK_P1,"\0",RK_LN0+RK_FLUSH+1,NULL);
   return 0;
   } /* End PXQTEST */

