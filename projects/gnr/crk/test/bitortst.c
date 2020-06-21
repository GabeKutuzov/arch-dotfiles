/***********************************************************************
*                              bitortst                                *
*                                                                      *
*  This program tests the bitor() function with various combinations   *
*  of operands designed to exercise the right-shift, left-shift, and   *
*  no-shift cases in the bitor() code.                                 *
*                                                                      *
*  V1A, 06/04/95, GNR - Initial version                                *
*  V2A, 05/10/99, GNR - Replace old rqswap routines with new versions  *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "bapkg.h"
#include "rkarith.h"
#include "swap.h"

#define Ntests 10          /* Number of tests implemented */

/* Test cases implemented here are:
*  (1) 1 bit in 1 byte, no shift
*  (2) 8 bits in 1 byte, no shift
*  (3) 1 bit, shifted left
*  (4) 1 bit, shifted right
*  (5) 4 bits, shifted right into two bytes
*  (6) 4 bits in two bytes, shifted left into one byte
*  (7) 8 bits in two bytes, no shift
*  (8) 20 bits in three bytes, left shifted
*  (9) 20 bits in three bytes, right shifted
* (10) 24 bits in four bytes, shifted to three bytes
*  Each of these is tested twice: first with an empty target,
*  then with a target containing random bits.
*/

/*---------------------------------------------------------------------*
*                              xbitor()                                *
*  This function implements bitor by a very safe algorithm involving   *
*  looping over the individual bits to be transferred.  The output     *
*  is used to test the bitor library function.  The goal here is       *
*  correctness, not speed.                                             *
*---------------------------------------------------------------------*/

void xbitor(byte *t, int jt, byte *s, int js, int ll) {

   int i;
   for (i=1; i<=ll; i++)
      if (bittst(s, js+i)) bitset(t, jt+i);
   }

/*---------------------------------------------------------------------*
*                        bitortst main program                         *
*---------------------------------------------------------------------*/

void main(void) {

   static int so[Ntests] = { 3, 0, 6, 2, 3, 6, 4,  5,  5,  4 };
   static int to[Ntests] = { 3, 0, 2, 6, 7, 2, 4,  3,  3,  0 };
   static int ll[Ntests] = { 1, 8, 1, 1, 4, 4, 8, 20, 20, 24 };

   long ls,lt1,lt,lxt;
   long seed = 1009;
   int itest;
   int nok = 0;
   byte s[4],t[4],xt[4];

   for (itest=0; itest<Ntests; itest++) {

/* Version I:  Clear the targets, set all the source bits.
*  Note:  Run xbitor first in case bitor being tested somehow
*  writes over the source array */

      memset((char *)s,0xff,4);
      memset((char *)t,0,4);
      memset((char *)xt,0,4);
      memcpy((char *)&ls,(char *)s,4);
      memcpy((char *)&lt1,(char *)t,4);

      xbitor(xt,to[itest],s,so[itest],ll[itest]);
      bitor(t,to[itest],s,so[itest],ll[itest]);

      if (memcmp((char *)xt,(char *)t,4)) {
         lxt = bemtoi4(xt);
         lt  = bemtoi4(t);
         printf("Test I-%d failed with so = %d, to = %d, len = %d\n",
            itest,so[itest],to[itest],ll[itest]);
         printf("   Target = %10lX, source = %10lX\n",lt1,ls);
         printf("   Answer = %10lX, correct= %10lX\n",lt,lxt);
         }
      else ++nok;

/* Version II:  Use random numbers for source and target  */

      ls = udev(&seed);
      lt1 = udev(&seed);
      bemfmi4(s,ls);
      bemfmi4(t,lt1);
      bemfmi4(xt,lt1);

      xbitor(xt,to[itest],s,so[itest],ll[itest]);
      bitor(t,to[itest],s,so[itest],ll[itest]);

      if (memcmp((char *)xt,(char *)t,4)) {
         lxt = bemtoi4(xt);
         lt =  bemtoi4(t);
         printf("Test II-%d failed with so = %d, to = %d, len = %d\n",
            itest,so[itest],to[itest],ll[itest]);
         printf("   Target = %10lX, source = %10lX\n",lt1,ls);
         printf("   Answer = %10lX, correct= %10lX\n",lt,lxt);
         }
      else ++nok;

      } /* End itest loop */

   printf("Completed %d tests correctly\n",nok);
   } /* End bitortst */
