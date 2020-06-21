/**********************************************************************
*                                                                     *
*     CONVERT ROUTINE CHECKER                                         *
*                                                                     *
*     This routine is hard-wired to read an input file called         *
*     "convtest.inp"                                                  *
*                                                                     *
*  V1A, 01/31/89, GNR - Converted from FORTRAN version                *
*  Rev, 09/23/93, GNR - Add test for rounding of negative fracs       *
*  Rev, 05/30/96, GNR - Add cnv2test for '^' and new '|' test         *
*  Rev, 01/28/97, GNR - Add tests for unsigned integers and           *
*                       V as code for underflow control               *
*  Rev, 08/25/01, GNR - Add test for T format input                   *
*  Rev, 02/15/03, GNR - Add test for metric units scaling             *
*  Rev, 02/16/03, GNR - Add test for alternative scales               *
*  Rev, 09/20/03, GNR - Add test for K format output                  *
*  Rev, 03/24/07, GHR - Add tests for data in parentheses             *
*  Rev, 12/31/07, GNR - Add tests of valck calls                      *
*  Rev, 05/05/09, GNR - Add test of J format input                    *
*  Rev, 12/30/09, GNR - Add tests of IS format input and output       *
*  Rev, 12/23/17, GNR - Add a test for binary scaling with muscan     *
**********************************************************************/

#define MAIN
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "sysdef.h"
#include "rocks.h"
#include "rkxtra.h"

/* Callout routine for testing 'J' code */
void kwjfn(char *pit, int *iv) {
   int it = pit[0] - '@';
   eqscan(iv, "I4", 3);
   *iv -= it;
   }

int main() {

/* Define test data */

   double dprec[5] =
      {2.7943E-31,0.00573802E0,2.78178E0,1259.42E0,3.8902E12};
   double nfrac[6] =
      {-2.4949,-2.4950,-2.4951,-2.9949,-2.9950,-2.9951};
   float gems[5] = {1.0632E-32,0.0649834,3.1415926,256.,5.732E13};
   float cell[6];
   float opt1;
   char *card;
   ui32 ic;
   long bsil[2];
   long jkl[5] = {-9999,-1,0,23,65536};
   long hex[3] = {0X12345678L,0X9ABCDEF0L,0X0111L};
   wseed wsd[2] = {{ 1234,56789 }, {0, 2147483646 }};
   int i,pd1,pd2;
   int eight = 8;
   int rep2 = 2;
   int rep5 = 5;
   int three = 3;
   unsigned short htxt;
   char chars[] = "Now is the time for all good men to come "
      "to the aid of their party";
   char num2[] = "num2";
   char keys[] = "KQ-RX-8F-ANBP";
   char pdat[6][8];

/* Part 1: Check output functions */

/* Normal FORTRAN formats */
   convrt("(#4,3F12.6F12.2f12.5/5E12.7/rq12.6/5d12.7/"
      "#5 5(6x,il6)/#5,5(6x,ul6))",
      gems,&gems[4],gems,&rep5,dprec,dprec,jkl,jkl,NULL);
   cryout(RK_P1, " Note:  It is correct that the first two values "
      "on the line above", RK_LN3, "    are different in 64-bit vs "
      "32-bit runs,", RK_LN0, "    because the variables are negative"
      " longs printed as unsigned.", RK_LN0, NULL);
/* Test < > L R=0 */
   convrt("(L57,5<f12.6,2x,a4,0q12.6>)",gems,chars,dprec,NULL);
/* Test rounding of negative numbers */
   convrt("(4H0Q.5,6Q10.5/4H Q.3,6Q10.3)",nfrac,nfrac,NULL);
/* Test Hollerith */
   convrt("(w12h0Old saying://h a68/h017a4/h017a4.2)",
      chars,chars,chars,NULL);
   convrt("(' This isn''t not in quotes')",NULL);
/* Test hexadecimal and other fancy stuff */
   convrt("(3*vz12,12habcdefghijklt43b4il8.2f12.aji6,2#vf12.4)",
      &hex[0],&hex[1],&hex[2],&jkl[4],&gems[3],&jkl[1],&rep2,
      gems,NULL);
/* Test left-justify with padding */
   convrt("(8h0A word j0i12,6h, and j1i12,9hends here)",
     &jkl[0],&jkl[3],NULL);
/* Test the #^ sequence (former cnv2test program) */
   convrt("('0Test'A4,#^,2F12.6,2X,F12.6)",
      num2,&three,&eight,gems,NULL);
/* Test the | indent code */
   convrt("('0Indent on the second line '|#6,(3Q10.5))",nfrac,NULL);
/* Test K format codes */
   convrt("('0K(W=8) test, expect AFKR8bbb: ',KL8,'<--End here')",
      keys, hex, NULL);
   convrt("(' K(J=1) test, expect AFKR8b: ',J1KL8,'<--End here')",
      keys, hex, NULL);
/* Test IS format code */
   convrt("(' Wseed test, expect (1234,56789) (0,2147483646): ',"
      "2J1IS24)", wsd, NULL);

/* Part 2: Test input functions
   (Expects to read an input file with one ROCKS 'CELL' card
    with 6 entries, one mixed-mode card for kwscan test, one
    card with a text string for T format test, one card with
    fixed-format and scannable numbers with units, and one
    card with test items in and not in parentheses.)
   (Code w in first position to print card now must be
    followed by a comma.)
*/

/* Input without scanning */
   cdunit("convtest.inp");
   card = cryin();
   sinform(card,"(w,6x,6f12)",cell,NULL);
   convrt("('0Input test:'/'0Cell: ',6F12.5)",cell,NULL);
/* Input with scanning */
   cell[4] = -1.0;
   /* Next produces deliberate error, skipping last two values,
   *  and should convert first value from 10.0 to 10.5  */
   sinform(card,"(s4#6,3V>10.5f12,2x,3f12)",cell,NULL);
   convrt("('0Cell: ',6F12.5)",cell,NULL);
   cell[4] = -2.0;
   /* Same, but suppress error message */
   sinform(card,"(s4#6n,3f12,2x3f2)",cell,NULL);
   convrt("('0Cell: ',6F12.5)",cell,NULL);

   for (i=0;i<6;i++) cell[i] = 0.0;
   card = cryin();
   cdprnt(card);
   /* Note that this call is testing the documented behavior that
   *  when scanning, sinform ignores the width spec after a numeric
   *  type code (here 2) and instead uses the width actually found. */
   sinform(card,"(s4f2=3*f2,2f2)",&cell[0],&cell[1],&cell[2],
      &cell[3],&cell[4],NULL);
   kwscan(&ic,"OPT1%F",&opt1,NULL);
   convrt("('0Cell: ',6F12.5)",cell,NULL);
   convrt("('0Opt1: ',F12.5)",&opt1,NULL);
   /* Same input card, test alternative binary scales */
   sinform(card,"(S4B12/6IL12)", bsil, NULL);
   convrt("('0Input with normal scale: ',B12/6IL12.6)", bsil, NULL);
   bscompat(RK_BSSLASH);
   sinform(card,"(S4B12/6IL12)", bsil+1, NULL);
   convrt("('0Input with compat scale: ',B12/6IL12.6)", bsil+1, NULL);
   if (bsil[0] != 531438) cryout(RK_P1,
      " ***Conversion failed, should be 531438.", RK_LN2, NULL);
   if (bsil[1] != 8304) cryout(RK_P1,
      " ***Conversion failed, should be 8304.", RK_LN2, NULL);
   /* Turn off bscompat mode for later tests */
   bscompat(0);

/* Test text string and IS format input */
   card = cryin();
   cdprnt(card);
   sinform(card,"(s0TH40,2IS)", &htxt, wsd, NULL);
   convrt("('0Text locator: ',JIH6/' Stored text: ',A44)",
      &htxt, getrktxt(htxt), NULL);
   convrt("(' Wseed input, expect (-1,1009) (9876,54321): ',2J1IS24)",
      wsd, NULL);

/* Test input with metric units scaling and J format code */
   accwad(ON);
   kwjreg((void (*)(char *, void *))kwjfn, 1);
   card = cryin();
   cdprnt(card);
   cell[1] = cell[2] = -1.0;
   i = -7;
   pd1 = 0;
   sinform(card,"(F8.2$-V,S8,J1C,=I6)", cell, &pd1, &i, NULL);
   kwscan(&ic, "ACC%F$-m/-sec2",cell+1,
               "BSCL%B14IJ$7mV",&pd2,NULL);
   if (i != -7) cryout(RK_P1,
      "0***Default 3nd positional value written over "
      "when keyword found.", RK_LN2, NULL);
   convrt("('0J test returned ',I6,', 30 is correct')", &pd1, NULL);
   if (pd1 != 30) cryout(RK_P1,
      "0***J format input conversion failed.", RK_LN2, NULL);
   convrt("('0Potential = ',#2F8.6,'V, "
      "acceleration = ',F8.6,'m/sec2, bscl = ',B7IJ8.5)", cell,
      &pd2, NULL);
   if (fabs(cell[0] - 0.0362) > 1E-6) cryout(RK_P1,
      "0***Potential conversion failed.", RK_LN2, NULL);
   if (fabs(cell[1] - 0.4725) > 1E-6) cryout(RK_P1,
      "0***Acceleration conversion failed.", RK_LN2, NULL);
   if (pd2 != 10291) cryout(RK_P1,
      "0***Bscl conversion failed.", RK_LN2, NULL);

/* Test handling of input in parentheses */
   /* Initialization of pdat[3] tests that sinform format A6.8
   *  does not write more than 6 characters to the argument  */
   strcpy(pdat[3], "xxxxxx?");
   strcpy(pdat[4], "Untouch");
   card = cryin();
   sinform(card,"(SW1,V(IV),V(2A6.8V),V(IV),V(4A6.8V))",
      &pd1,pdat[0],&pd2,pdat[2],NULL);
   cryout(RK_P1, "0Deliberate errors: The last 2 items in the second "
      "parens", RK_LN4, "   should be marked as \"TOO MANY\".", RK_LN0,
      "   Two items should be missing in the last parens.", RK_LN0,
      NULL);
   convrt("(' The two numeric items were read as ',2*I6)",
      &pd1, &pd2, NULL);
   cryout(RK_P1, (pd1 == 1) ?
      " Item in first parens read in correctly." :
      " Item in first parens should be \"1\".", RK_LN1, NULL);
   cryout(RK_P1, " Next two items should be \"too\" and \"many\":",
      RK_LN2, "    ", RK_LN0, pdat[0], 8, "    ", 4, pdat[1], 8, NULL);
   cryout(RK_P1, (pd2 == 2) ?
      " Third data item read in correctly." :
      " Third data item should be \"2\".", RK_LN1, NULL);
   cryout(RK_P1, " Next three items should be \"not\" and \"enough?\" "
      "and \"Untouch\":", RK_LN2, "    ", RK_LN0,
      pdat[2], 8, "    ", 4, pdat[3], 8, "    ", 4, pdat[4], 8, NULL);

   cryout(RK_P1,"0End of tests", RK_FLUSH+RK_LN2, NULL);
   return 0;
   } /* End convtest main program */
