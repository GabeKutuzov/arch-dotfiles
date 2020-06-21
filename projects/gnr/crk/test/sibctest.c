/***********************************************************************
*                              sibctest                                *
*                                                                      *
*              Test the input conversion routine sibcdin               *
*                                                                      *
************************************************************************
*  V1A, 11/08/97, GNR - Initial version                                *
***********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include "sysdef.h"
#include "rkxtra.h"

extern char *sibcdinf;

/*---------------------------------------------------------------------*
*    abexit, abexitm substitutes to eliminate linking cryout, etc.     *
*  These routines return to caller, rather than exiting, so multiple   *
*  errors can be tested in the same test routine.                      *
*---------------------------------------------------------------------*/

void abexit(int code) {

   static char abmsg[] = "\n***Program terminated with abend code %d";
   puts (ssprintf(NULL, abmsg, code));
   fflush(stdout);
   } /* End abexit() */

void abexitm(int code, char *msg) {

   static char efmt[] = "\n\n***%s";
   puts (ssprintf(NULL, efmt, msg));
   abexit(code);
   } /* End abexitm() */


/*---------------------------------------------------------------------*
*                            sibctest main                             *
*---------------------------------------------------------------------*/

void main(void) {

   int i,ic,mode;

   static char inp8[]  = "0 1 7 10 17 20 3456 -1 -7 -10 -3456 12a 8";
   static char inp10[] = "0 1 9 10 19 20 3456 -1 -9 -10 -3456 12a junk";
   static char inp16[] = "0 1 f 10 1F 20 3456 -1 -f -10 -3456 12g junk";

   printf("Testing sibcdin.\n"
      "Tests 1-12 should all end with abend 55.\n");

/* Loop over four modes:  scan, scan+CTST, fixed, fixed+CTST */

   for (mode=0; mode<4; mode++) {

      switch (mode) {
      case 0:
         ic = RK_SSCN;
         printf("Testing in scan mode without character test:\n");
         break;
      case 1:
         ic = RK_SSCN + RK_CTST;
         printf("Testing in scan mode with character test:\n");
         break;
      case 2:
         ic = 0;
         printf("Testing in fixed width mode without character test:\n");
         break;
      case 3:
         ic = RK_CTST;
         printf("Testing in fixed width mode with character test:\n");
         break;
         } /* End mode switch */

/* Tests 1,4,7,10:  Octal input */

      printf("\n   Test %d: Octal, input is "
         "0 1 7 10 17 20 3456 -1 -7 -10 -3456 12a 8\n", 1+3*mode);
      printf("    Results are: %lo", sibcdin(ic+RK_OCTF+3, inp8));
      for (i=1; i<14; i++)
         printf(" %lo", sibcdin(ic+RK_OCTF+5, NULL));
      printf("\n");

/* Tests 2,5,8,11:  Decimal input */

      printf("\n   Test %d: Decimal, input is "
         "0 1 9 10 19 20 3456 -1 -9 -10 -3456 12a junk\n", 2+3*mode);
      printf("    Results are: %ld", sibcdin(ic+3, inp10));
      for (i=1; i<14; i++)
         printf(" %ld", sibcdin(ic+5, NULL));
      printf("\n");

/* Tests 3,6,9,12:  Hexadecimal input */

      printf("\n   Test %d: Hexadecimal, input is "
         "0 1 f 10 1F 20 3456 -1 -f -10 -3456 12g junk\n", 3+3*mode);
      printf("    Results are: %lx", sibcdin(ic+RK_HEXF+3, inp16));
      for (i=1; i<14; i++)
         printf(" %lx", sibcdin(ic+RK_HEXF+5, NULL));
      printf("\n");

      } /* End testing four scanning modes */

/* Test 13:  An illegal zero value */

   printf("\n    Test 13: Input zero, should produce an error\n");
   printf("    Results are: %ld\n",
      sibcdin(RK_SSCN+RK_IORF+RK_ZTST+3, "0"));
   printf("    sibcdinf should be NULL, actual is %lx\n", sibcdinf);

/* Test 14:  An illegal negative value */

   printf("\n    Test 14: Input -1, should produce an error\n");
   printf("    Results are: %ld\n",
      sibcdin(RK_SSCN+RK_IORF+RK_QPOS+3, "-1"));
   printf("    sibcdinf should be NULL, actual is %lx\n", sibcdinf);

   } /* End sibctest() */
