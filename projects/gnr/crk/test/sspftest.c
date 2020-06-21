/***********************************************************************
*                              sspftest                                *
*                                                                      *
*                Test the formatting routine ssprintf                  *
*                                                                      *
*  (N.B.  The library routine puts() adds a newline to each output.)   *
************************************************************************
*  V1A, 11/23/97, GNR - Initial version                                *
*  V1B, 03/29/98, GNR - Add hexadecimal test                           *
*  V2B, 07/09/98, GNR - Add consecutive '%' codes test                 *
*  Rev, 09/22/98, GNR - Add test of pointer conversion                 *
*  Rev, 09/22/08, GNR - Add test of long integers                      *
*  Rev, 10/04/15, GNR - Add test of integer padding                    *
***********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include "sysdef.h"
#include "rksubs.h"

/*---------------------------------------------------------------------*
*    abexit, abexitm substitutes to eliminate linking cryout, etc.     *
*---------------------------------------------------------------------*/

void abexit(int code) {

   static char abmsg[] = "0***Program terminated with abend code %10d";
   char emsg[sizeof(abmsg) + LONG_SIZE];
   ssprintf(emsg, abmsg, code);
   puts(emsg);
   exit(code);
   } /* End abexit() */

void abexitm(int code, char *emsg) {

   static char efmt[] = "***%128s";
   char emsg2[133];
   ssprintf(emsg2, efmt, emsg);
   puts("  ");
   puts(emsg2);
   abexit(code);
   } /* End abexitm() */



/*---------------------------------------------------------------------*
*                            sspftest main                             *
*---------------------------------------------------------------------*/

void main(void) {

   unsigned char mstr[5] = { 0x12, 0x34, 0xab, 0xcd, 0xef };
   long t11;                  /* Number for test 11 */
   char tstmp[12];            /* Space for time stamp output */
   char pout[133];            /* Output string */

/* Test 1:  A simple string */

   ssprintf(pout, "Test 1: A simple string");
   puts(pout);

/* Test 2:  A string that includes another string */

   ssprintf(pout, "Test 2: This string includes %24s", "this string");
   puts(pout);

/* Test 3:  A string that includes a decimal number */

   ssprintf(pout, "Test 3: This string has %4d words", 7);
   puts(pout);

/* Test 4:  A negative number */

   ssprintf(pout, "Test 4: Negative seven is %4d", -7);
   puts(pout);

/* Test 5:  A NULL pointer to a string */

   ssprintf(pout, "Test 5: Following is a null string %24s", NULL);
   puts(pout);

/* Test 6:  A lower-case hexadecimal number */

   ssprintf(pout, "Test 6: Should see abcd: %4x", 0xabcd);
   puts(pout);

/* Test 7:  An upper-case negative hexadecimal number */

   ssprintf(pout, "Test 7: Should see %8s: %8X",
      sizeof(int) == 2 ? "FFFF" : "FFFFFFFF", -1);
   puts(pout);

/* Test 8:  A pointer */

   ssprintf(pout, "Test 8: Should see a pointer: %p", pout);
   puts(pout);

/* Test 9:  A variable-width memory dump */

   ssprintf(pout, "Test 9: Should see 1234abcdef: %*m", 5, mstr);
   puts(pout);

/* Test 10:  Two consecutive format codes */

   ssprintf(pout, "Test 10: Two consecutive numbers %4d%4d", 1, 2);
   puts(pout);

/* Test 11:  A long integer */

#if LSIZE == 8
   t11 = 123456789012345;
   ssprintf(pout, "Test 11: Should see 123456789012345: %16ld", t11);
#else
   t11 = 1234567890;
   ssprintf(pout, "Test 11: Should see 1234567890: %16ld", t11);
#endif
   puts(pout);

/* Test 12: Integer with padding */

   ssprintf(pout, "Test 12:  Should see \"  12\": %4ed", 12);
   puts(pout);

/* Test 13:  tstamp tested here for convenience */

   tstamp(tstmp);
   ssprintf(pout, "Test 13:  Should see time stamp: %12s", tstmp);
   puts(pout);

/* Test 14:  Oops, forgot the width */

   puts("Now should generate abexit 58:");
   ssprintf(pout, "Test 14: Should not print this %s");
   puts(pout);

   } /* End sspftest() */
