/***********************************************************************
*                              getptest                                *
*                                                                      *
*  Compute a few key values of getprime and compare them with          *
*  correct results obtained from a table of prime numbers.             *
*                                                                      *
************************************************************************
*  V1A, 08/13/98, GNR - Initial version                                *
***********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "sysdef.h"
#include "rkarith.h"

struct gpt {
   unsigned long arg;
   unsigned long correct;
   unsigned long result;
   } ;

void main(void) {

   struct gpt tests[] = {
      {        0,        0, },
      {        1,        1, },
      {        2,        2, },
      {        3,        3, },
      {        4,        5, },
      {        5,        5, },
      {        6,        7, },
      {        7,        7, },
      {        8,       11, },
      {        9,       11, },
      {       10,       11, },
      {       11,       11, },
      {       12,       13, },
      {       13,       13, },
      {       14,       17, },
      {       32,       37, },
      {       63,       67, },
      {      110,      113, },
      {      144,      149, },
      {      191,      191, },
      {      242,      251, },
      {      340,      347, },
      {      443,      443, },
      {      524,      541, },
      {      655,      659, },
      {      770,      773, },
      {      912,      919, },
      {     1000,     1009, },
      {     1200,     1201, },
      {     1350,     1361, },
      {     1641,     1657, },
      {     2000,     2003, },
      {     7518,     7523, }};

   unsigned long abserr, maxerr = 0, numerrs = 0;
   int it,nt = sizeof(tests)/sizeof(struct gpt);

   printf("Results of getprime test:\n"
      "   (All tests are correct except the following:)\n");

   for (it=0; it<nt; it++) {
      tests[it].result = getprime(tests[it].arg);
      abserr = labs(tests[it].result - tests[it].correct);
      if (abserr > maxerr) maxerr = abserr;
      if (abserr > 0) {
         numerrs += 1;
         printf("    For arg %lu, returned %lu, correct is %lu\n",
            tests[it].arg, tests[it].result, tests[it].correct);
         }
      }

   printf("The largest absolute error was %lu\n", maxerr);

   } /* End getptest() */
