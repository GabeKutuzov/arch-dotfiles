/***********************************************************************
*                                                                      *
*                      RKPRINTF ROUTINE CHECKER                        *
*                                                                      *
*  Note:  We assume that if printf emulation passes these tests, the   *
*  related routines fprintf, rfprintf, sprintf, snprintf will also.    *
*                                                                      *
*  Codes not tested in this routine: NOPY/ n1/n2B n1|n2B               *
*                                                                      *
*  V1A, 12/07/11, GNR - New program, based on convtest.c               *
***********************************************************************/

#define MAIN
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "sysdef.h"
#include "rocks.h"
#include "rkxtra.h"
#include "rkarith.h"

int main() {

/* Define test data */

   double dprec[5] =
      {2.7943E-31,0.00573802E0,2.78178E0,1259.42E0,3.8902E12};
   double nfrac[6] =
      {-2.4949,-2.4950,-2.4951,-2.9949,-2.9950,-2.9951};
   si64 widey = jcsw(88,248753);
   size_t szt = sizeof(si64);
   float gems[5] = {1.0632E-32,0.0649834,3.1415926,256.,5.732E13};
   long jkl[5] = {-9999,-1,0,23,65536};
   wseed wsd[2] = {{ 1234,56789 }, {0, 2147483646 }};
   ui32 hex[3] = {0X12345678,0X9ABCDEF0,0X0111};
   int eight = 8;
   int rep2 = 2;
   int rep5 = 5;
   int three = 3;
   int nout;
   si16 hint[3] = {1009,-2,2050};
   char chars[] = "Now is the time for all good men to come "
      "to the aid of their party";
   char num2[] = "num2";
   char keys[] = "KQ-RX-8F-ANBP";

/* Part 1: Check output functions */

/* Normal printf formats */
   printf("%S%11ba%11bb%11bc%11bd%11be");
   printf("%4Z%3R12.5F%12.1F%&12.4f\n%5R12.6jE\n%*R12.5Q\n%5R12.6E\n"
      "%5Z%(%6b%6li%)\n%5Z%(%6b%6lu%)\n",
      gems,&gems[4],gems,rep5,dprec,dprec,jkl,jkl);
   cryout(RK_P1, " Note:  It is correct that the first two values "
      "on the line above", RK_LN3, "    are different in 64-bit vs "
      "32-bit runs, because the variables", RK_LN0, "    are "
      "negative longs printed as unsigned.", RK_LN0, NULL);
   printf("A pointer (8 hex chars if 32-bit, 16 if 64-bit): %p\n",hex);
   printf("A 64-bit integer (should be 377957370801): %&wd\n",&widey);
   printf("A size_t value (should be 8): %zd\n",szt);
   printf("Left padding (should be 0008): %04d\n", eight);
   printf("Test concatenation with blank flag: %d% d\n",three,eight);
   printf("Shorts (ptr,promoted): %3Z%+6hi%6hi%4B6hi,%6i%6hi%4B6hi\n",
      hint,hint[0],hint[1],hint[2]);
   printf("Written %^J8d so far: %n",hint[0],&nout);
   if (nout == 21) cryout(RK_P1, "(result OK)", RK_CCL, NULL);
   else printf("(got %d, should be 21)\n", nout);
/* Test < >, big L to force new page, *R=0 */
   printf("%57L%5<%12.5f%2b%4s%*R12.6q%>\n", gems,chars,0,dprec);
/* Test '=' code for array stride and ';' termination code */
   printf("%2=%3<%12.5q%; ,,%>\n",dprec);
/* Test rounding of negative numbers */
   printf("\nQ.5%6R10.4Q\nQ.3%6R10.2Q\n", nfrac,nfrac);
/* Test Hollerith.  Note: len(chars) is 66, expect 2 left blanks
*  first time, but truncation to exact length on right second time. */
   printf("\nOld saying:%2\n%68s\n\n%17R.4s\n\n%17R2^4s\n",
      chars,chars,chars);
/* gcc converts a printf call with no conversions to a puts() call.
*  This line tests that our puts() implementation replaces the
*  library call, thereby counting lines and assuring that this
*  line is buffered in its correct position, not at the start of
*  all the test output as occurs if we do not supply puts().  */
   printf("This output has no conversons\n");
/* Test '!' and '%%' codes */
   printf("%3!Should see a percent sign here %%\n");
   if (RK.iexit == 3) printf("RK.iexit correctly set to 3\n");
   else printf("RK.iexit set to %d, should be 3\n", RK.iexit);
   RK.iexit = 0;
/* Test hexadecimal and other fancy stuff */
   printf("%3rV12jXabcdefghijkl%43T%4B8.1li%&12.Af%-6li%*RV12.3f\n",
      hex[0],hex[1],hex[2],jkl[4],&gems[3],jkl[1],rep2,gems);
   printf("Last two w/g instead of Vf: %2R12.3g\n", gems);
/* Test char and left-justify with padding */
   printf("\nA char %c, an octal %#jo, a word %^J12li, and %1J12li"
      "ends here\n", 'c',hex[2],jkl[0],jkl[3]);
/* Test the Z^ sequence ('#^' in convrt) */
   printf("\nTest%4s%*Z%*^2R12.5F  %12.5F\n",num2,three,eight,gems);
/* Test the | indent code */
   printf("\nIndent on the second line %|%6Z%(%3R10.4Q\n%)",nfrac);
/* Test K format codes */
   printf("\nK(W=4) test, expect AFKR8bbb: %8K<--End here\n",
      keys, hex[0]);
   printf("K(J=1) test, expect AFKR8b: %&1J8jK<--End here\n",
      keys, hex);
/* Skip a field, fill it in with blanks */
   printf("Skip with blanks test: %~?8i<--End here\n", 0,1009);
/* Test 'k' format code (wide random seed) */
   printf("Wseed test, expect (1234,56789) (0,2147483646): %2R1J24k",
      wsd);
   cryout(RK_P1,"0End of tests", RK_FLUSH+RK_LN2, NULL);
   return 0;
   } /* End convtest main program */
