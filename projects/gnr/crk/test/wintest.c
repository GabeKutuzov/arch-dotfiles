/***********************************************************************
*                               wintest                                *
*                                                                      *
*  This is the test program for crk routine wbcdin(), which is an      *
*  update of ibcdin which provides full 64-bit fixed-point input       *
*  capability.                                                         *
*                                                                      *
*  V1A, 07/04/09, GNR - Based on wouttest                              *
***********************************************************************/

/* Include standard library functions */

#define MAIN

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "rocks.h"
#include "rkxtra.h"
#include "rkarith.h"

/***********************************************************************
*                                                                      *
*                      ABEXIT, ABEXITM, ABEXITME                       *
*                                                                      *
*      Special test versions to print message but not terminate        *
*    (Note that abexit() is needed because called from e64set().)      *
*                                                                      *
************************************************************************
*  N.B.  Compiler warnings that these functions return when they have  *
*  been declared "no-return" (sysdef.h) may be ignored.  Versions here *
*  do return in order to allow multiple errors in one test.            *
***********************************************************************/

void abexit(int code) {

   cryout(RK_P1, ssprintf(NULL, "0***Abexit called with code %d",
      code), RK_LN2, NULL);
   } /* End abexit() */

void abexitm(int code, char *emsg) {

   cryout(RK_P1, ssprintf(NULL, "0***Abexitm called with code %d "
      "and msg", code), RK_LN2, emsg, RK_CCL, NULL);
   } /* End abexitm() */

void abexitme(int code, char *emsg) {

   cryout(RK_P1, ssprintf(NULL, "0***Abexitme called with code %d "
      "and msg", code), RK_LN2, emsg, RK_CCL, NULL);
   } /* End abexitme() */


/*---------------------------------------------------------------------*
*                                 ptr                                  *
*              Functions to print incorrect test results               *
*                                                                      *
*  Synopsis:                                                           *
*  void ptr(char *name, char *inpt, byte *test, byte *corr, ui32 ic)   *
*                                                                      *
*  'name'   is the name of the test                                    *
*  'inpt'   is the input string                                        *
*  'test'   is the result of calling wbcdin                            *
*  'corr'   is the correct answer                                      *
*  'ic'     is the function code from the wbcdin call                  *
*---------------------------------------------------------------------*/

static int ngood = 0, nbad = 0;

void ptr(char *name, const char *inpt, byte *test, void *corr,
      ui32 ic) {

   static int len0[8] = { sizeof(int), sizeof(byte), sizeof(short),
      sizeof(int), sizeof(ui32), sizeof(long), sizeof(ui64), 0 };

   int len = len0[ic >> 6 & 7];
   int len2,notOK;
   if (len == 0) {
      abexit(40);
      len = 1; }
   notOK = memcmp((char *)test, (char *)corr, len);
   len2 = 2*len;
   if (notOK) {
      char ctest[18],ccorr[18];
      wbcdwt(test, ctest, RK_NUNS|RK_HEXF|ic & RK_NI128|len2-1);
      ctest[len2] = '\0';
      wbcdwt(corr, ccorr, RK_NUNS|RK_HEXF|ic & RK_NI128|len2-1);
      ccorr[len2] = '\0';
      cryout(RK_P1,"    ", RK_LN1, name, RK_CCL, " input = ", RK_CCL,
         inpt, RK_CCL, ", result = ", RK_CCL, ctest, len2,
         ", correct = ", RK_CCL, ccorr, len2,
         " **INCORRECT**", RK_CCL, NULL);
      ++nbad;
      }
   else
      ++ngood;
   }

/*---------------------------------------------------------------------*
*                          main test program                           *
*---------------------------------------------------------------------*/

int main() {

/* Declarations: */

   const char *pin;  /* Input string */
   byte res[8];      /* Result */
   ui32 ic;          /* Conversion code */
   si32 tt;          /* Temps for unusual expected results */
   short tth;

   si64 zl8,sl8[5];  /* Assigned by macros, see below */
   ui64 ul8[5];
   si32 sl4[5] = { 65535, -65535, -33554432, -1, 0x80000000 };
   ui32 ul4[5] = { 65535, 65536, 9999999, 0x80000000, 0xffffffff };
   si16 sl2[4] = { 0, 255, -1, -32768 };
   ui16 ul2[4] = { 0, 256, 4097, 65535 };
   schr sl1[3] = { 0, 1, -1 };
   byte ul1[3] = { 0, 63, 255 };

   sl8[0] = jcsw(0xFFFFFFFF,0x80000000);
   sl8[1] = jcsw(0xFFFFFFFF,0);
   sl8[2] = jcsw(0xFFFFFFE8,0xB7891801);
   sl8[3] = jcsw(0xFFFFFFFF,0xFFFFFFFF);
   sl8[4] = jcsw(0x80000000,0);
   ul8[0] = jcuw(0,0xFFFFFFFF);
   ul8[1] = jcuw(1,0);
   ul8[2] = jcuw(0x17,0x4876E7FF);
   ul8[3] = jcuw(0x80000000,0);
   ul8[4] = jcuw(0xFFFFFFFF,0xFFFFFFFF);

/* Print a nice little header */

   cryout(RK_P1, "0Test wbcdin decimal input routine", RK_LN2, NULL);
   e64set(0, NULL);

/*---------------------------------------------------------------------*
*       Stage I.  Test widths and sizes, no decimals or binary         *
*---------------------------------------------------------------------*/

/* Set 1:  Straight-up decimal testing with byte items */

   pin = "0";
   ic = RK_NUNS|RK_IORF|RK_NBYTE|4;
   wbcdin(pin, res, ic);
   ptr("UDB1", pin, res, ul1+0, ic);

   pin = "63";
   ic = RK_NUNS|RK_IORF|RK_NBYTE|4;
   wbcdin(pin, res, ic);
   ptr("UDB2", pin, res, ul1+1, ic);

   pin = "255";
   ic = RK_NUNS|RK_IORF|RK_NBYTE|4;
   wbcdin(pin, res, ic);
   ptr("UDB3", pin, res, ul1+2, ic);

   pin = "0";
   ic = RK_IORF|RK_NBYTE|4;
   wbcdin(pin, res, ic);
   ptr("SDB1", pin, res, sl1+0, ic);

   pin = "1";
   ic = RK_IORF|RK_NBYTE|4;
   wbcdin(pin, res, ic);
   ptr("SDB2", pin, res, sl1+1, ic);

   pin = "-1";
   ic = RK_IORF|RK_NBYTE|4;
   wbcdin(pin, res, ic);
   ptr("SDB3", pin, res, sl1+2, ic);

/* Set 2:  Straight-up decimal testing with short items */

   pin = "0";
   ic = RK_NUNS|RK_IORF|RK_NHALF|8;
   wbcdin(pin, res, ic);
   ptr("UDH1", pin, res, ul2+0, ic);

   pin = "256";
   ic = RK_NUNS|RK_IORF|RK_NHALF|8;
   wbcdin(pin, res, ic);
   ptr("UDH2", pin, res, ul2+1, ic);

   pin = "4097";
   ic = RK_NUNS|RK_IORF|RK_NHALF|8;
   wbcdin(pin, res, ic);
   ptr("UDH3", pin, res, ul2+2, ic);

   pin = "65535";
   ic = RK_NUNS|RK_IORF|RK_NHALF|8;
   wbcdin(pin, res, ic);
   ptr("UDH4", pin, res, ul2+3, ic);

   pin = "0";
   ic = RK_IORF|RK_NHALF|8;
   wbcdin(pin, res, ic);
   ptr("SDH1", pin, res, sl2+0, ic);

   pin = "255";
   ic = RK_IORF|RK_NHALF|8;
   wbcdin(pin, res, ic);
   ptr("SDH2", pin, res, sl2+1, ic);

   pin = "-1";
   ic = RK_IORF|RK_NHALF|8;
   wbcdin(pin, res, ic);
   ptr("SDH3", pin, res, sl2+2, ic);

   pin = " -32768";
   ic = RK_IORF|RK_NHALF|8;
   wbcdin(pin, res, ic);
   ptr("SDH4", pin, res, sl2+3, ic);

/* Set 3:  Straight-up decimal testing with 32-bit items */

   pin = " 65535";
   ic = RK_NUNS|RK_IORF|RK_NI32|14;
   wbcdin(pin, res, ic);
   ptr("UDL1", pin, res, ul4+0, ic);

   pin = "  65536";
   ic = RK_NUNS|RK_IORF|RK_NI32|14;
   wbcdin(pin, res, ic);
   ptr("UDL2", pin, res, ul4+1, ic);

   pin = " 9999999";
   ic = RK_NUNS|RK_IORF|RK_NI32|14;
   wbcdin(pin, res, ic);
   ptr("UDL3", pin, res, ul4+2, ic);

   pin = " 2147483648";
   ic = RK_NUNS|RK_IORF|RK_NI32|14;
   wbcdin(pin, res, ic);
   ptr("UDL4", pin, res, ul4+3, ic);

   pin = " 4294967295";
   ic = RK_NUNS|RK_IORF|RK_NI32|14;
   wbcdin(pin, res, ic);
   ptr("UDL5", pin, res, ul4+4, ic);

   pin = " 65535";
   ic = RK_IORF|RK_NI32|14;
   wbcdin(pin, res, ic);
   ptr("SDL1", pin, res, sl4+0, ic);

   pin = " -65535";
   ic = RK_IORF|RK_NI32|14;
   wbcdin(pin, res, ic);
   ptr("SDL2", pin, res, sl4+1, ic);

   pin = "  -33554432";
   ic = RK_IORF|RK_NI32|14;
   wbcdin(pin, res, ic);
   ptr("SDL3", pin, res, sl4+2, ic);

   pin = " -1";
   ic = RK_IORF|RK_NI32|14;
   wbcdin(pin, res, ic);
   ptr("SDL4", pin, res, sl4+3, ic);

   pin = "  -2147483648";
   ic = RK_IORF|RK_NI32|14;
   wbcdin(pin, res, ic);
   ptr("SDL5", pin, res, sl4+4, ic);

/* Set 4:  Straight-up decimal testing with 64-bit items */

   pin = " 4294967295";
   ic = RK_NUNS|RK_IORF|RK_NI64|24;
   wbcdin(pin, res, ic);
   ptr("UDW1", pin, res, ul8+0, ic);

   pin = " 4294967296";
   ic = RK_NUNS|RK_IORF|RK_NI64|24;
   wbcdin(pin, res, ic);
   ptr("UDW2", pin, res, ul8+1, ic);

   pin = " 99999999999";
   ic = RK_NUNS|RK_IORF|RK_NI64|24;
   wbcdin(pin, res, ic);
   ptr("UDW3", pin, res, ul8+2, ic);

   pin = " 9223372036854775808";
   ic = RK_NUNS|RK_IORF|RK_NI64|24;
   wbcdin(pin, res, ic);
   ptr("UDW4", pin, res, ul8+3, ic);

   pin = "  18446744073709551615";
   ic = RK_NUNS|RK_IORF|RK_NI64|24;
   wbcdin(pin, res, ic);
   ptr("UDW5", pin, res, ul8+4, ic);

   pin = "  -2147483648";
   ic = RK_IORF|RK_NI64|24;
   wbcdin(pin, res, ic);
   ptr("SDW1", pin, res, sl8+0, ic);

   pin = "  -4294967296";
   ic = RK_IORF|RK_NI64|24;
   wbcdin(pin, res, ic);
   ptr("SDW2", pin, res, sl8+1, ic);

   pin = " -99999999999";
   ic = RK_IORF|RK_NI64|24;
   wbcdin(pin, res, ic);
   ptr("SDW3", pin, res, sl8+2, ic);

   pin = "        -1";
   ic = RK_IORF|RK_NI64|24;
   wbcdin(pin, res, ic);
   ptr("SDW4", pin, res, sl8+3, ic);

   pin = "  -9223372036854775808";
   ic = RK_IORF|RK_NI64|24;
   wbcdin(pin, res, ic);
   ptr("SDW5", pin, res, sl8+4, ic);

/*---------------------------------------------------------------------*
*          Stage II.  Test various binary and decimal scales           *
*---------------------------------------------------------------------*/

/* Set 5:  Scale testing with byte items */

   pin = "0";
   ic = 2<<RK_DS|RK_NUNS|RK_IORF|RK_NBYTE|4;
   wbcdin(pin, res, ic);
   ptr("UDSB1", pin, res, ul1+0, ic);

   pin = "3.94";
   ic = 4<<RK_BS|3<<RK_DS|RK_NUNS|RK_IORF|RK_NBYTE|4;
   wbcdin(pin, res, ic);
   ptr("UDSB2", pin, res, ul1+1, ic);

   pin = "7.969";
   ic = 5<<RK_BS|4<<RK_DS|RK_NUNS|RK_IORF|RK_NBYTE|4;
   wbcdin(pin, res, ic);
   ptr("UDSB3", pin, res, ul1+2, ic);

   pin = " 0";
   ic = 3<<RK_BS|RK_IORF|RK_NBYTE|4;
   wbcdin(pin, res, ic);
   ptr("SDSB1", pin, res, sl1+0, ic);

   pin = "0.125";
   ic = 3<<RK_BS|4<<RK_DS|RK_IORF|RK_NBYTE|4;
   wbcdin(pin, res, ic);
   ptr("SDSB2", pin, res, sl1+1, ic);

   pin = "-0.13";
   ic = 3<<RK_BS|3<<RK_DS|RK_IORF|RK_NBYTE|4;
   wbcdin(pin, res, ic);
   ptr("SDSB3", pin, res, sl1+2, ic);

/* Set 6:  Scale testing with short items */

   pin = " 0";
   ic = 3<<RK_BS|2<<RK_DS|RK_NUNS|RK_IORF|RK_NHALF|14;
   wbcdin(pin, res, ic);
   ptr("UDSH1", pin, res, ul2+0, ic);

   pin = " 16.00";
   ic = 4<<RK_BS|3<<RK_DS|RK_NUNS|RK_IORF|RK_NHALF|14;
   wbcdin(pin, res, ic);
   ptr("UDSH2", pin, res, ul2+1, ic);

   pin = "  128.0313";
   ic = 5<<RK_BS|5<<RK_DS|RK_NUNS|RK_IORF|RK_NHALF|14;
   wbcdin(pin, res, ic);
   ptr("UDSH3", pin, res, ul2+2, ic);

   pin = "  511.992188";
   ic = 7<<RK_BS|7<<RK_DS|RK_NUNS|RK_IORF|RK_NHALF|14;
   wbcdin(pin, res, ic);
   ptr("UDSH4", pin, res, ul2+3, ic);

   pin = "  0";
   ic = 4<<RK_BS|4<<RK_DS|RK_IORF|RK_NHALF|14;
   wbcdin(pin, res, ic);
   ptr("SDSH1", pin, res, sl2+0, ic);

   pin = "  15.938";
   ic = 4<<RK_BS|4<<RK_DS|RK_IORF|RK_NHALF|14;
   wbcdin(pin, res, ic);
   ptr("SDSH2", pin, res, sl2+1, ic);

   pin = "   -0.0078125";
   ic = 7<<RK_BS|8<<RK_DS|RK_IORF|RK_NHALF|14;
   wbcdin(pin, res, ic);
   ptr("SDSH3", pin, res, sl2+2, ic);

   pin = "   -2048.0000";
   ic = 4<<RK_BS|5<<RK_DS|RK_IORF|RK_NHALF|14;
   wbcdin(pin, res, ic);
   ptr("SDSH4", pin, res, sl2+3, ic);

/* Set 7:  Scale testing with 32-bit items */

   pin = " 511.99218748";  /* Needs to round up */
   ic = 7<<RK_BS|9<<RK_DS|RK_NUNS|RK_IORF|RK_NI32|14;
   wbcdin(pin, res, ic);
   ptr("UDSL1", pin, res, ul4+0, ic);

   pin = "  256.0000000";
   ic = 8<<RK_BS|8<<RK_DS|RK_NUNS|RK_IORF|RK_NI32|14;
   wbcdin(pin, res, ic);
   ptr("UDSL2", pin, res, ul4+1, ic);

   pin = "   9999.999";
   ic = RK_DSF|3<<RK_DS|RK_NUNS|RK_IORF|RK_NI32|14;
   wbcdin(pin, res, ic);
   ptr("UDSL3", pin, res, res, ic);

   pin = "    32768";
   ic = 16<<RK_BS|RK_NUNS|RK_IORF|RK_NI32|14;
   wbcdin(pin, res, ic);
   ptr("UDSL4", pin, res, ul4+3, ic);

   pin = "255.99999994040";
   ic = 24<<RK_BS|12<<RK_DS|RK_NUNS|RK_IORF|RK_NI32|15;
   wbcdin(pin, res, ic);
   ptr("UDSL5", pin, res, ul4+4, ic);

   pin = "  255.9960938";
   ic = 8<<RK_BS|8<<RK_DS|RK_IORF|RK_NI32|14;
   wbcdin(pin, res, ic);
   ptr("SDSL1", pin, res, sl4+0, ic);

   pin = "  -255.9960938";
   ic = 8<<RK_BS|8<<RK_DS|RK_IORF|RK_NI32|14;
   wbcdin(pin, res, ic);
   ptr("SDSL2", pin, res, sl4+1, ic);

   pin = "  -4.00000000";
   ic = 23<<RK_BS|9<<RK_DS|RK_IORF|RK_NI32|14;
   wbcdin(pin, res, ic);
   ptr("SDSL3", pin, res, sl4+2, ic);

   pin = "  -0.0625";
   ic = 4<<RK_BS|5<<RK_DS|RK_IORF|RK_NI32|14;
   wbcdin(pin, res, ic);
   ptr("SDSL4", pin, res, sl4+3, ic);

   pin = "  -2147483648.00";
   ic = RK_IORF|RK_NI32|16;
   wbcdin(pin, res, ic);
   ptr("SDSL5", pin, res, sl4+4, ic);

/* Set 8:  Scale testing with 64-bit items */

   pin = "  0.99999999976717";
   ic = 32<<RK_BS|15<<RK_DS|RK_NUNS|RK_IORF|RK_NI64|24;
   wbcdin(pin, res, ic);
   ptr("UDSW1", pin, res, ul8+0, ic);

   pin = "65536.0000000000000";
   ic = 16<<RK_BS|15<<RK_DS|RK_NUNS|RK_IORF|RK_NI64|24;
   wbcdin(pin, res, ic);
   ptr("UDSW2", pin, res, ul8+1, ic);

   pin = " 99999999998.77";   /* This needs to round up */
   ic = 3<<RK_DS|RK_NUNS|RK_IORF|RK_NI64|24;
   wbcdin(pin, res, ic);
   ptr("UDSW3", pin, res, ul8+2, ic);

   pin = "    1.0000000";
   ic = 63<<RK_BS|8<<RK_DS|RK_NUNS|RK_IORF|RK_NI64|24;
   wbcdin(pin, res, ic);
   ptr("UDSW4", pin, res, ul8+3, ic);

   cryout(RK_P1, "0Next test should produce a magnitude error",
      RK_LN2, NULL);
   pin = " 2.00000000000000";
   ic = 63<<RK_BS|15<<RK_DS|RK_NUNS|RK_IORF|RK_NI64|24;
   wbcdin(pin, res, ic);
   ptr("UDSW5", pin, res, ul8+4, ic);

   pin = "   -0.5000";
   ic = 32<<RK_BS|5<<RK_DS|RK_IORF|RK_NI64|24;
   wbcdin(pin, res, ic);
   ptr("SDSW1", pin, res, sl8+0, ic);

   pin = "  -0.25000";
   ic = 34<<RK_BS|6<<RK_DS|RK_IORF|RK_NI64|24;
   wbcdin(pin, res, ic);
   ptr("SDSW2", pin, res, sl8+1, ic);

   pin = "  -9999999.9999";
   ic = RK_DSF|124<<RK_DS|RK_IORF|RK_NI64|24;
   wbcdin(pin, res, ic);
   ptr("SDSW3", pin, res, sl8+2, ic);

   zl8 = jesl(0);
   pin = "  -0.00000000000000";
   ic = 63<<RK_BS|15<<RK_DS|RK_IORF|RK_NI64|24;
   wbcdin(pin, res, ic);
   ptr("SDSW4", pin, res, &zl8, ic);

   pin = "  -8.0000";
   ic = 60<<RK_BS|5<<RK_DS|RK_IORF|RK_NI64|24;
   wbcdin(pin, res, ic);
   ptr("SDSW5", pin, res, sl8+4, ic);

/*---------------------------------------------------------------------*
*     Stage III.  Test hexadecimal, octal, and exponential input       *
*---------------------------------------------------------------------*/

/* Set 9: Straight hexadecimal */

   pin = "  3F";
   ic = RK_NUNS|RK_HEXF|RK_NBYTE|4;
   wbcdin(pin, res, ic);
   ptr("UHB1", pin, res, ul1+1, ic);

   pin = " -01";
   ic = RK_HEXF|RK_NBYTE|4;
   wbcdin(pin, res, ic);
   ptr("SHB1", pin, res, sl1+2, ic);

   pin = "  0x1001";
   ic = RK_NUNS|RK_HEXF|RK_NHALF|14;
   wbcdin(pin, res, ic);
   ptr("UHH1", pin, res, ul2+2, ic);

   pin = " -0X0001";
   ic = RK_HEXF|RK_NHALF|14;
   wbcdin(pin, res, ic);
   ptr("SHH1", pin, res, sl2+2, ic);

   pin = "   0098967F";
   ic = RK_NUNS|RK_HEXF|RK_NI32|14;
   wbcdin(pin, res, ic);
   ptr("UHL1", pin, res, ul4+2, ic);

   pin = " -02000000";
   ic = RK_HEXF|RK_NI32|14;
   wbcdin(pin, res, ic);
   ptr("SHL1", pin, res, sl4+2, ic);

   pin = "  0x000000174876E7FF";
   ic = RK_NUNS|RK_HEXF|RK_NI64|24;
   wbcdin(pin, res, ic);
   ptr("UHW1", pin, res, ul8+2, ic);

   pin = " -0000000100000000";
   ic = RK_HEXF|RK_NI64|24;
   wbcdin(pin, res, ic);
   ptr("SHW1", pin, res, sl8+1, ic);

/* Set 10: Straight octal */

   pin = "  77";
   ic = RK_NUNS|RK_OCTF|RK_NBYTE|4;
   wbcdin(pin, res, ic);
   ptr("UOB1", pin, res, ul1+1, ic);

   pin = "   -1";
   ic = RK_OCTF|RK_NBYTE|4;
   wbcdin(pin, res, ic);
   ptr("SOB1", pin, res, sl1+2, ic);

   pin = " 010001";
   ic = RK_NUNS|RK_OCTF|RK_NHALF|14;
   wbcdin(pin, res, ic);
   ptr("UOH1", pin, res, ul2+2, ic);

   pin = " -01";
   ic = RK_OCTF|RK_NHALF|14;
   wbcdin(pin, res, ic);
   ptr("SOH1", pin, res, sl2+2, ic);

   pin = " 46113177";
   ic = RK_NUNS|RK_OCTF|RK_NI32|14;
   wbcdin(pin, res, ic);
   ptr("UOL1", pin, res, ul4+2, ic);

   pin = " -200000000";
   ic = RK_OCTF|RK_NI32|14;
   wbcdin(pin, res, ic);
   ptr("SOL1", pin, res, sl4+2, ic);

   pin = "  01351035563777";
   ic = RK_NUNS|RK_OCTF|RK_NI64|24;
   wbcdin(pin, res, ic);
   ptr("UOW1", pin, res, ul8+2, ic);

   pin = " -40000000000";
   ic = RK_OCTF|RK_NI64|24;
   wbcdin(pin, res, ic);
   ptr("SOW1", pin, res, sl8+1, ic);

/* Set 11: Explicit or implicit exponential--same thing on input */

   pin = " 6.30+1";
   ic = 3<<RK_DS|RK_NUNS|RK_EFMT|RK_NBYTE|6;
   wbcdin(pin, res, ic);
   ptr("UEB1", pin, res, ul1+1, ic);

   pin = " -1";
   ic = RK_IORF|RK_NBYTE|4;
   wbcdin(pin, res, ic);
   ptr("SEB1", pin, res, sl1+2, ic);

   pin = "  1.2803+2";
   ic = 5<<RK_BS|5<<RK_DS|RK_NUNS|RK_EFMT|RK_NHALF|14;
   wbcdin(pin, res, ic);
   ptr("UEH1", pin, res, ul2+2, ic);

   pin = "   -6.25-2";
   ic = 4<<RK_BS|3<<RK_DS|RK_IORF|RK_NHALF|14;
   wbcdin(pin, res, ic);
   ptr("SEH1", pin, res, sl2+2, ic);

   pin = "  9.99999900+6";
   ic = 9<<RK_DS|RK_NUNS|RK_EFMT|RK_NI32|14;
   wbcdin(pin, res, ic);
   ptr("UEL1", pin, res, ul4+2, ic);

   pin = " -8.3886080E+006";
   RK.expwid = 4;
   ic = 2<<RK_BS|8<<RK_DS|RK_IORF|RK_NI32|16;
   wbcdin(pin, res, ic);
   ptr("SEL1", pin, res, sl4+2, ic);
   RK.expwid = 0;

   zl8 = jcsw(0x17,0x4876E800);
   pin = "   1.0000+11";
   ic = 5<<RK_DS|RK_NUNS|RK_EFMT|RK_NI64|24;
   wbcdin(pin, res, ic);
   ptr("UEW1", pin, res, &zl8, ic);

   pin = "  -2.56000000000000+2";
   ic = 24<<RK_BS|15<<RK_DS|RK_IORF|RK_NI64|24;
   wbcdin(pin, res, ic);
   ptr("SEW1", pin, res, sl8+1, ic);

/*---------------------------------------------------------------------*
*              Stage IV.  Test miscellaneous option bits               *
*---------------------------------------------------------------------*/

/* Set 12: DSF bit with a negative or positive decimal */

   tt  = -10000;
   pin = "-0.0625";  /* 124 = -4 mod 128 */
   ic = 4<<RK_BS|RK_DSF|124<<RK_DS|RK_IORF|RK_NI32|14;
   wbcdin(pin, res, ic);
   ptr("DSF1", pin, res, (byte *)&tt, ic);

   tt  = -100000000;
   pin = "-625";
   ic = 4<<RK_BS|RK_DSF|124<<RK_DS|RK_IORF|RK_NI32|14;
   wbcdin(pin, res, ic);
   ptr("DSF2", pin, res, (byte *)&tt, ic);

   tt = 41;
   pin = "0.62";
   ic = 16<<RK_BS|RK_DSF|3<<RK_DS|RK_IORF|RK_NI32|8;
   wbcdin(pin, res, ic);
   ptr("DSF3", pin, res, (byte*)&tt, ic);

   tt = 4063;
   pin = "62";
   ic = 16<<RK_BS|RK_DSF|3<<RK_DS|RK_IORF|RK_NI32|8;
   wbcdin(pin, res, ic);
   ptr("DSF4", pin, res, (byte*)&tt, ic);

   tth = -66;
   pin = "-6.25-2";
   ic = 20<<RK_BS|RK_DSF|3<<RK_DS|RK_EFMT|RK_NHALF|14;
   wbcdin(pin, res, ic);
   ptr("DSF5", pin, res, (byte *)&tth, ic);

/* Set 13: Zero test */

   /* This should not produce an error */
   pin = "-9223372036854775808";
   ic = RK_ZTST|RK_IORF|RK_NI64|24;
   wbcdin(pin, res, ic);
   ptr("ZTST1", pin, res, sl8+4, ic);

   cryout(RK_P1, "0Next three tests should produce ZTST errors",
      RK_LN2, NULL);
   tt = 1;
   pin = "0";
   ic = 4<<RK_BS|3<<RK_DS|RK_ZTST|RK_IORF|RK_NI32|8;
   wbcdin(pin, res, ic);
   ptr("ZTST2", pin, res, (byte *)&tt, ic);

   pin = " 1.2803-6";
   ic = 5<<RK_BS|5<<RK_DS|RK_NUNS|RK_ZTST|RK_EFMT|RK_NI32|14;
   wbcdin(pin, res, ic);
   ptr("ZTST3", pin, res, (byte *)&tt, ic);

   tt = -1;
   pin = "-0.00001";
   ic = RK_IORF|2<<RK_DS|RK_DSF|RK_ZTST|RK_NI32|14;
   wbcdin(pin, res, ic);
   ptr("ZTST4", pin, res, (byte *)&tt, ic);

/* Set 14: Invalid character test */

   /* This should not produce an error */
   pin = " 7.97";
   ic = 5<<RK_BS|RK_CTST|RK_NUNS|RK_IORF|RK_NBYTE|4;
   wbcdin(pin, res, ic);
   ptr("CTST2", pin, res, ul1+2, ic);

   cryout(RK_P1, "0Next five tests should produce CTST errors"
      " and one magnitude error", RK_LN2, NULL);
   pin = "  C511.992188";
   ic = 7<<RK_BS|7<<RK_DS|RK_CTST|RK_NUNS|RK_IORF|RK_NHALF|14;
   wbcdin(pin, res, ic);
   ptr("CTST2", pin, res, ul2+3, ic);

   pin = "256.00b%";
   ic = 24<<RK_BS|RK_CTST|RK_NUNS|RK_IORF|RK_NI32|6;
   wbcdin(pin, res, ic);
   ptr("CTST3", pin, res, ul4+4, ic);

   pin = "   0098967FG";
   ic = RK_NUNS|RK_HEXF|RK_CTST|RK_NI32|14;
   wbcdin(pin, res, ic);
   ptr("CTST4", pin, res, ul4+2, ic);

   pin = " 461131877";
   ic = RK_NUNS|RK_OCTF|RK_CTST|RK_NI32|14;
   wbcdin(pin, res, ic);
   ptr("CTST5", pin, res, ul4+2, ic);

   pin = " 46113177A";
   ic = RK_NUNS|RK_OCTF|RK_CTST|RK_NI32|14;
   wbcdin(pin, res, ic);
   ptr("CTST6", pin, res, ul4+2, ic);

/* Set 15: Negative value test */

   /* This should not produce an error */
   pin = " 3.94";
   ic = 4<<RK_BS|3<<RK_DS|RK_NUNS|RK_QPOS|RK_IORF|RK_NBYTE|4;
   wbcdin(pin, res, ic);
   ptr("QPOS1", pin, res, ul1+1, ic);

   cryout(RK_P1, "0Next four tests should produce QPOS errors",
      RK_LN2, NULL);
   tt = 0;
   pin = "  -255.9960938";
   ic = 8<<RK_BS|8<<RK_DS|RK_QPOS|RK_IORF|RK_NI32|14;
   wbcdin(pin, res, ic);
   ptr("QPOS2", pin, res, (byte *)&tt, ic);

   pin = "  -4.00000000";
   ic = 23<<RK_BS|9<<RK_DS|RK_QPOS|RK_IORF|RK_NI32|14;
   wbcdin(pin, res, ic);
   ptr("QPOS3", pin, res, (byte *)&tt, ic);

   sl8[0] = jcsw(0,0);
   pin = "   -0.5000";
   ic = 32<<RK_BS|5<<RK_DS|RK_QPOS|RK_IORF|RK_NI64|24;
   wbcdin(pin, res, ic);
   ptr("QPOS4", pin, res, sl8+0, ic);

   pin = "  -9999999.9999";
   ic = RK_DSF|5<<RK_DS|RK_QPOS|RK_IORF|RK_NI64|24;
   wbcdin(pin, res, ic);
   ptr("QPOS5", pin, res, sl8+0, ic);

/* Set 16: Test that a value must be an integer--
*  The result is not adjusted */

   /* This test should not produce an error */
   tt = -2776;
   pin = " -3.47+2";
   ic = 3<<RK_BS|RK_INT|RK_IORF|RK_NI32|8;
   wbcdin(pin, res, ic);
   ptr("INT1", pin, res, (char *)&tt, ic);

   cryout(RK_P1, "0Next four tests should produce not-an-integer "
      "errors", RK_LN2, NULL);
   tt = 4;
   pin = " 4.29";
   ic = RK_INT|RK_IORF|RK_NI32|6;
   wbcdin(pin, res, ic);
   ptr("INT2", pin, res, (char *)&tt, ic);

   tt = -278;
   pin = " -3.47+1";
   ic = 3<<RK_BS|RK_INT|RK_IORF|RK_NI32|8;
   wbcdin(pin, res, ic);
   ptr("INT3", pin, res, (char *)&tt, ic);

   sl8[0] = jcsw(0,18);
   pin = " 2.3076";
   ic = 3<<RK_BS|RK_INT|RK_IORF|RK_NI64|14;
   wbcdin(pin, res, ic);
   ptr("INT4", pin, res, sl8+0, ic);

   /* This is incidentally an integer, but it looks like it is not.
   *  Program gives an error here by design--this is not an error
   *  in the test.  */
   sl8[0] = jcsw(0,32000);
   pin = " 1.0000000+3";
   ic = 5<<RK_BS|RK_INT|RK_IORF|RK_NI64|14;
   wbcdin(pin, res, ic);
   ptr("INT5", pin, res, sl8+0, ic);

/* Set 17: Detection of ill-formed inputs that do not require
*  a specific test bit set  */

   cryout(RK_P1, "0Input with two decimal points", RK_LN2, NULL);
   pin = "  -255.9960.938";
   ic = 8<<RK_BS|8<<RK_DS|RK_IORF|RK_NI32|16;
   wbcdin(pin, res, ic);
   ptr("SDSL2", pin, res, sl4+1, ic);

   cryout(RK_P1, "0Input with plus-minus", RK_LN2, NULL);
   pin = "  +-255.9960938";
   ic = 8<<RK_BS|8<<RK_DS|RK_IORF|RK_NI32|16;
   wbcdin(pin, res, ic);
   ptr("SDSL2", pin, res, sl4+1, ic);

   cryout(RK_P1, "0Input with minus-plus", RK_LN2, NULL);
   pin = "  -+255.9960938";
   ic = 8<<RK_BS|8<<RK_DS|RK_IORF|RK_NI32|16;
   wbcdin(pin, res, ic);
   ptr("SDSL3", pin, res, sl4+1, ic);

   cryout(RK_P1, "0Input with two E's", RK_LN2, NULL);
   pin = " -8.3886080EE+006";
   RK.expwid = 4;
   ic = 2<<RK_BS|8<<RK_DS|RK_IORF|RK_NI32|16;
   wbcdin(pin, res, ic);
   ptr("SEL1", pin, res, sl4+2, ic);

   cryout(RK_P1, "0Input with two exponents", RK_LN2, NULL);
   pin = " -8.3886080+6E-3";
   ic = 2<<RK_BS|8<<RK_DS|RK_IORF|RK_NI32|16;
   wbcdin(pin, res, ic);
   ptr("SEL1", pin, res, sl4+2, ic);

   cryout(RK_P1, "0Exponent with no digits", RK_LN2, NULL);
   tt = 0xffffffde;
   pin = " -8.3886080E";
   ic = 2<<RK_BS|8<<RK_DS|RK_IORF|RK_NI32|16;
   wbcdin(pin, res, ic);
   ptr("SEL1", pin, res, &tt, ic);
   RK.expwid = 0;

   cryout(RK_P1, "0Hexadecimal with an exponent", RK_LN2, NULL);
   pin = "   0098967F+3";
   ic = RK_NUNS|RK_HEXF|RK_NI32|14;
   wbcdin(pin, res, ic);
   ptr("UHL1", pin, res, ul4+2, ic);

   cryout(RK_P1, "0No digits at all", RK_LN2, NULL);
   pin = "CD";
   ic = RK_NUNS|RK_IORF|RK_NHALF|8;
   wbcdin(pin, res, ic);
   ptr("UDH1", pin, res, ul2+0, ic);

   cryout(RK_P1, "0Huge exponent", RK_LN2, NULL);
   tt = 0x7fffffff;
   pin = "  255.996+938";
   ic = 8<<RK_BS|8<<RK_DS|RK_IORF|RK_NI32|16;
   wbcdin(pin, res, ic);
   ptr("SDSL2", pin, res, (char *)&tt, ic);

/*---------------------------------------------------------------------*
*                   Stage V.  Test value adjustment                    *
*  (Adjustment values are set up directly in RKC so svvadj(),ckvadj()  *
*  are not tested here.)                                               *
*---------------------------------------------------------------------*/

   /* Positive adjustment to a positive input--2 sizes */
   RKC.dvadj = 0.375;
   RKC.ktest |= VCK_ADJ;
   tt = 1820164;
   pin = " 27.3985";
   ic = 16<<RK_BS|RK_IORF|RK_NI32|10;
   wbcdin(pin, res, ic);
   ptr("ADJ1", pin, res, (char *)&tt, ic);

   zl8 = jcsw(84,449683076);
   pin = " 83.7297";
   ic = 32<<RK_BS|RK_IORF|RK_NI64|10;
   wbcdin(pin, res, ic);
   ptr("ADJ2", pin, res, (char *)&zl8, ic);

   /* Positive adjustment to a negative input */
   RKC.dvadj = 12387.088;
   tt = -62287524;
   pin = "-27594.003  ";
   ic = 12<<RK_BS|RK_IORF|RK_NI32|15;
   wbcdin(pin, res, ic);
   ptr("ADJ3", pin, res, (char *)&tt, ic);

   cryout(RK_P1, "0Expect no \"MUST BE >= 0\" msg here", RK_LN2, NULL);
   /* Note:  If both numbers are carried to higher than S12 precision
   *  before adjusting, the correct rounded answer is 16224603, however,
   *  if both numbers are first rounded to S12, the answer given here
   *  is correct.  */
   tt = 16224602;
   pin = "-8426.00337";
   ic = 12<<RK_BS|RK_QPOS|RK_IORF|RK_NI32|15;
   wbcdin(pin, res, ic);
   ptr("ADJ4", pin, res, (char *)&tt, ic);

   cryout(RK_P1, "0Now expect 2 \"AFTER ADJUSTMENT\" msgs", RK_LN2, NULL);
   tt = 0;
   pin = "-50426.00337";
   ic = 12<<RK_BS|RK_QPOS|RK_IORF|RK_NI32|15;
   wbcdin(pin, res, ic);
   ptr("ADJ5", pin, res, (char *)&tt, ic);

   /* Negative adjustment to a positive input */
   RKC.dvadj = -0.3754;
   tt = 1770934;
   pin = " 27.3977";
   ic = 16<<RK_BS|RK_IORF|RK_NI32|10;
   wbcdin(pin, res, ic);
   ptr("ADJ6", pin, res, (char *)&tt, ic);

   zl8 = jcsw(83,1521706913);
   pin = " 83.7297";
   ic = 32<<RK_BS|RK_IORF|RK_NI64|10;
   wbcdin(pin, res, ic);
   ptr("ADJ7", pin, res, (char *)&zl8, ic);

   RKC.dvadj = -12387.088;
   tt = 0;
   pin = "426.00337";
   ic = 12<<RK_BS|RK_QPOS|RK_IORF|RK_NI32|15;
   wbcdin(pin, res, ic);
   ptr("ADJ8", pin, res, (char *)&tt, ic);

   /* Negative adjustment to a negative input */
   /* Note:  If both numbers are carried to higher than S12 precision
   *  before adjusting, the correct rounded answer is -163762549,
   *  however, if both numbers are first rounded to S12, the answer
   *  given here is correct.  */
   tt = -163762548;
   pin = "-27594.003  ";
   ic = 12<<RK_BS|RK_IORF|RK_NI32|15;
   wbcdin(pin, res, ic);
   ptr("ADJ9", pin, res, (char *)&tt, ic);

   zl8 = jcsw(12470,3512203064);
   zl8 = jnsw(zl8);
   pin = " -83.7297485";
   ic = 32<<RK_BS|RK_IORF|RK_NI64|14;
   wbcdin(pin, res, ic);
   ptr("ADJ10", pin, res, (char *)&zl8, ic);

/* THE END */

   cryout(RK_P1, ssprintf(NULL, "0End wbcdin tests, got %d correct, "
      "%d errors.", ngood, nbad), RK_LN2+RK_FLUSH, NULL);
   return 0;
   } /* End main program */
