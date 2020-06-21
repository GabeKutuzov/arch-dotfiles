/*--------------------------------------------------------------------*/
/*                                                                    */
/*    SYMBOL tests                                                    */
/*                                                                    */
/* Rev, 10/10/11, GNR - Change ngraph call to newplt                  */
/*--------------------------------------------------------------------*/

/* Note that as a temporary measure, the machine-independent version
   of 'symbol' is named 'symbol2' to distinguish it from 'symbol',
   which is in the crympltm.c source file and which is built for the
   MATROX specifically. */

#define MAIN
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "rocks.h"
#include "plots.h"

/* Prototype for this little guy */
void symbol2(float x, float y, float ht,
   char *text, float angle, short int n);

void main() {

   float xc= 1.25,yc=0.625;
   char *str1 = "LABEL GENERATED WITH SYMBOL";
   short lstr = strlen(str1);
   int i,j;                   /* Array counters */
   byte ch;                   /* Actual character */

/* Start the graphics package */

   newplt(14.0, 14.0, 0, 0, "DEFAULT", "RED","DEFAULT", 0);

/* Plot some labels around the edge */

   symbol2(0.05,0.05,0.25,str1,0.0,lstr);
   symbol2(13.95,0.05,0.15,str1,90.0,lstr);
   symbol2(13.95,10.70,0.25,str1,180.0,lstr);
   symbol2(0.20,10.70,0.15,str1,270.0,lstr);

/* Test carriage return, backspace, subscript, superscript */

   symbol2(0.05,5.0,0.15,"BSPO\x11" "-\x15" "A\x14" "B\x12"
      "C\x12" "D",0.0,14);

/* Plot a little graph at the right */

   ch = 0;
   symbol2(13.5,1.0,0.125,&ch,0.0,0);
   ch = 1;
   for (i=1; i<=12; i++,ch++)
      symbol2(13.5,1.0+0.5*i,0.125,&ch,0.0,-2);

/* Plot the entire character set */

   ch = 0;
   for (i=0; i<13; i++) {
      xc = 1.25;
      for (j=0; j<12; j++) {
         if (ch != 17 && ch != 18 && ch != 20 && ch != 21)
            symbol2(xc,yc,0.5,&ch,0.0,1);
         xc += 1.0;
         ch++;
         }
      yc += 0.75;
      }

   } /* End symbtest */


