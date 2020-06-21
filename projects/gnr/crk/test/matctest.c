/*--------------------------------------------------------------------*/
/*                                                                    */
/*    MATCH and SMATCH test                                           */
/*                                                                    */
/*--------------------------------------------------------------------*/

/*
   This test executes match with each possible value of iscn.
   Since match calls smatch, this program is also a test of smatch.
*/

#define MAIN
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "rocks.h"
#include "rkxtra.h"

void main() {

   char card[] = { "X,XYZ,XY=ABC,JUNK" };
   char *keys[4] = { "XYZ","X","ABCDEF","NOISE" };
   int ic;                    /* scan code */

   cdscan(card,0,DFLT_MAXLEN,0);

/* Test 1: A single letter which is an exact match */
   ic = match(0,2,~RK_COMMA,0,keys,4);
   printf("Test 1: Exact single letter match\n"
      "match returned %d, 2 is correct\n",ic);

/* Test 2: A longer string which is an exact match */
   ic = match(0,2,~RK_COMMA,0,keys,4);
   printf("Test 2: Exact three letter match\n"
      "match returned %d, 1 is correct\n",ic);

/* Test 3: Match 2 out of 3 characters */
   ic = match(0,3,~0,RK_EQUALS,keys,4);
   printf("Test 3: Two out of three match with equals sign\n"
      "match returned %d, 1 is correct\n",ic);

/* Test 4: Equals check */
   ic = match(RK_EQCHK,2,~RK_COMMA,0,keys,4);
   printf("Test 4: Match item after equals sign\n"
      "match returned %d, 3 is correct\n",ic);

/* Test 5: An item that is not matched */
   ic = match(RK_NMERR,2,~RK_COMMA,0,keys,4);
   printf("Test 5: An item that is not matched\n"
      "match returned %d, 0 is correct\n",ic);

/* Test 6: A missing item */
   ic = match(0,5,~RK_COMMA,0,keys,4);
   printf("Test 6: Nothing there to scan\n"
      "match returned %d, -2 is correct\n",ic);

   } /* End of program */

