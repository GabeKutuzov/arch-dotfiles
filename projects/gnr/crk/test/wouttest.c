/***********************************************************************
*                              wouttest                                *
*                                                                      *
*  This is the test program for crk routine wbcdwt(), which is an      *
*  update of ibcdwt which provides full 64-bit fixed-point output      *
*  capability.                                                         *
*                                                                      *
*  V1A, 01/24/09, GNR - New program                                    *
*  V1B, 08/12/09, GNR - Revise hex/oct tests for new prefix codes      *
*  V1C, 09/04/11, GNR - Test new prefix codes, expect zero padding in  *
*                       P0H6.                                          *
*  V1D, 03/01/17, GNR - Add tests of RK_MZERO code                     *
*  R65, 09/26/17, GNR - Change ADH3,ADL3 to allow minus at left-field  *
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
*                           ABEXIT, ABEXITM                            *
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

   printf("***Abexit called with code %d\n", code);
   } /* End abexit() */

void abexitm(int code, char *emsg) {

   printf("***Abexitm called with code %d and msg %s\n",
      code, emsg);
   } /* End abexitm() */

/*---------------------------------------------------------------------*
*                                 ptr                                  *
*              Functions to print incorrect test results               *
*                                                                      *
*  Synopsis:                                                           *
*     void ptr(char *name, char *test, char *corr, int len)            *
*---------------------------------------------------------------------*/

static int ngood = 0, nbad = 0;

void ptr(char *name, char *test, char *corr, int len) {

   int notOK = strncmp(test, corr, len);
   if (notOK) {
      printf("   %s result = %.*s, correct = %.*s, **INCORRECT**\n",
         name, len, test, len, corr);
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

   si64 sl8[5];   /* Assigned by macros, see below */
   ui64 ul8[5];
   si32 sl4[5] = { 65535, -65535, -33554432, -1, 0x80000000 };
   ui32 ul4[5] = { 65535, 65536, 9999999, 0x80000000, 0xffffffff };
   si16 sl2[5] = { 0, 255, -1, -32768, 0xc000 };
   ui16 ul2[4] = { 0, 256, 4097, 65535 };
   schr sl1[3] = { 0, 1, -1 };
   byte ul1[3] = { 0, 63, 255 };

   char in05[10] = "==xxxxx==";
   char in07[12] = "==xxxxxxx==";
   char in15[20] = "==xxxxxxxxxxxxxxx==";
   char in25[30] = "==xxxxxxxxxxxxxxxxxxxxxxxxx==";
   char res[30];

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

   printf("Test wbcdwt decimal output routine\n");
   e64set(0, NULL);

/*---------------------------------------------------------------------*
*       Stage I.  Test widths and sizes, no decimals or binary         *
*---------------------------------------------------------------------*/

/* Set 1:  Straight-up decimal testing with byte items */

   strcpy(res, in05);
   wbcdwt(ul1+0, res+2, RK_NUNS|RK_IORF|RK_NBYTE|4);
   ptr("UDB1", res, "==    0==", 9);

   strcpy(res, in05);
   wbcdwt(ul1+1, res+2, RK_NUNS|RK_IORF|RK_NBYTE|4);
   ptr("UDB2", res, "==   63==", 9);

   strcpy(res, in05);
   wbcdwt(ul1+2, res+2, RK_NUNS|RK_IORF|RK_NBYTE|4);
   ptr("UDB3", res, "==  255==", 9);

   strcpy(res, in05);
   wbcdwt(sl1+0, res+2, RK_IORF|RK_NBYTE|4);
   ptr("SDB1", res, "==    0==", 9);

   strcpy(res, in05);
   wbcdwt(sl1+1, res+2, RK_IORF|RK_NBYTE|4);
   ptr("SDB2", res, "==    1==", 9);

   strcpy(res, in05);
   wbcdwt(sl1+2, res+2, RK_IORF|RK_NBYTE|4);
   ptr("SDB3", res, "==   -1==", 9);

/* Set 2:  Straight-up decimal testing with short items */

   strcpy(res, in15);
   wbcdwt(ul2+0, res+2, RK_NUNS|RK_IORF|RK_NHALF|14);
   ptr("UDH1", res, "==              0==", 19);

   strcpy(res, in15);
   wbcdwt(ul2+1, res+2, RK_NUNS|RK_IORF|RK_NHALF|14);
   ptr("UDH2", res, "==            256==", 19);

   strcpy(res, in15);
   wbcdwt(ul2+2, res+2, RK_NUNS|RK_IORF|RK_NHALF|14);
   ptr("UDH3", res, "==           4097==", 19);

   strcpy(res, in15);
   wbcdwt(ul2+3, res+2, RK_NUNS|RK_IORF|RK_NHALF|14);
   ptr("UDH4", res, "==          65535==", 19);

   strcpy(res, in15);
   wbcdwt(sl2+0, res+2, RK_IORF|RK_NHALF|14);
   ptr("SDH1", res, "==              0==", 19);

   strcpy(res, in15);
   wbcdwt(sl2+1, res+2, RK_IORF|RK_NHALF|14);
   ptr("SDH2", res, "==            255==", 19);

   strcpy(res, in15);
   wbcdwt(sl2+2, res+2, RK_IORF|RK_NHALF|14);
   ptr("SDH3", res, "==             -1==", 19);

   strcpy(res, in15);
   wbcdwt(sl2+3, res+2, RK_IORF|RK_NHALF|14);
   ptr("SDH4", res, "==         -32768==", 19);

/* Set 3:  Straight-up decimal testing with 32-bit items */

   strcpy(res, in15);
   wbcdwt(ul4+0, res+2, RK_NUNS|RK_IORF|RK_NI32|14);
   ptr("UDL1", res, "==          65535==", 19);

   strcpy(res, in15);
   wbcdwt(ul4+1, res+2, RK_NUNS|RK_IORF|RK_NI32|14);
   ptr("UDL2", res, "==          65536==", 19);

   strcpy(res, in15);
   wbcdwt(ul4+2, res+2, RK_NUNS|RK_IORF|RK_NI32|14);
   ptr("UDL3", res, "==        9999999==", 19);

   strcpy(res, in15);
   wbcdwt(ul4+3, res+2, RK_NUNS|RK_IORF|RK_NI32|14);
   ptr("UDL4", res, "==     2147483648==", 19);

   strcpy(res, in15);
   wbcdwt(ul4+4, res+2, RK_NUNS|RK_IORF|RK_NI32|14);
   ptr("UDL5", res, "==     4294967295==", 19);

   strcpy(res, in15);
   wbcdwt(sl4+0, res+2, RK_IORF|RK_NI32|14);
   ptr("SDL1", res, "==          65535==", 19);

   strcpy(res, in15);
   wbcdwt(sl4+1, res+2, RK_IORF|RK_NI32|14);
   ptr("SDL2", res, "==         -65535==", 19);

   strcpy(res, in15);
   wbcdwt(sl4+2, res+2, RK_IORF|RK_NI32|14);
   ptr("SDL3", res, "==      -33554432==", 19);

   strcpy(res, in15);
   wbcdwt(sl4+3, res+2, RK_IORF|RK_NI32|14);
   ptr("SDL4", res, "==             -1==", 19);

   strcpy(res, in15);
   wbcdwt(sl4+4, res+2, RK_IORF|RK_NI32|14);
   ptr("SDL5", res, "==    -2147483648==", 19);

/* Set 4:  Straight-up decimal testing with 64-bit items */

   strcpy(res, in25);
   wbcdwt(ul8+0, res+2, RK_NUNS|RK_IORF|RK_NI64|24);
   ptr("UDW1", res, "==               4294967295==", 29);

   strcpy(res, in25);
   wbcdwt(ul8+1, res+2, RK_NUNS|RK_IORF|RK_NI64|24);
   ptr("UDW2", res, "==               4294967296==", 29);

   strcpy(res, in25);
   wbcdwt(ul8+2, res+2, RK_NUNS|RK_IORF|RK_NI64|24);
   ptr("UDW3", res, "==              99999999999==", 29);

   strcpy(res, in25);
   wbcdwt(ul8+3, res+2, RK_NUNS|RK_IORF|RK_NI64|24);
   ptr("UDW4", res, "==      9223372036854775808==", 29);

   strcpy(res, in25);
   wbcdwt(ul8+4, res+2, RK_NUNS|RK_IORF|RK_NI64|24);
   ptr("UDW5", res, "==     18446744073709551615==", 29);

   strcpy(res, in25);
   wbcdwt(sl8+0, res+2, RK_IORF|RK_NI64|24);
   ptr("SDW1", res, "==              -2147483648==", 29);

   strcpy(res, in25);
   wbcdwt(sl8+1, res+2, RK_IORF|RK_NI64|24);
   ptr("SDW2", res, "==              -4294967296==", 29);

   strcpy(res, in25);
   wbcdwt(sl8+2, res+2, RK_IORF|RK_NI64|24);
   ptr("SDW3", res, "==             -99999999999==", 29);

   strcpy(res, in25);
   wbcdwt(sl8+3, res+2, RK_IORF|RK_NI64|24);
   ptr("SDW4", res, "==                       -1==", 29);

   strcpy(res, in25);
   wbcdwt(sl8+4, res+2, RK_IORF|RK_NI64|24);
   ptr("SDW5", res, "==     -9223372036854775808==", 29);

/*---------------------------------------------------------------------*
*          Stage II.  Test various binary and decimal scales           *
*---------------------------------------------------------------------*/

/* Set 5:  Scale testing with byte items */

   strcpy(res, in05);
   wbcdwt(ul1+0, res+2, 2<<RK_DS|RK_NUNS|RK_IORF|RK_NBYTE|4);
   ptr("UDSB1", res, "==    0==", 9);

   strcpy(res, in05);
   wbcdwt(ul1+1, res+2, 4<<RK_BS|3<<RK_DS|RK_NUNS|RK_IORF|RK_NBYTE|4);
   ptr("UDSB2", res, "== 3.94==", 9);

   strcpy(res, in05);
   wbcdwt(ul1+2, res+2, 5<<RK_BS|4<<RK_DS|RK_NUNS|RK_IORF|RK_NBYTE|4);
   ptr("UDSB3", res, "==7.969==", 9);

   strcpy(res, in05);
   wbcdwt(sl1+0, res+2, 3<<RK_BS|RK_IORF|RK_NBYTE|4);
   ptr("SDSB1", res, "==    0==", 9);

   strcpy(res, in05);
   wbcdwt(sl1+1, res+2, 3<<RK_BS|4<<RK_DS|RK_IORF|RK_NBYTE|4);
   ptr("SDSB2", res, "==0.125==", 9);

   strcpy(res, in05);
   wbcdwt(sl1+2, res+2, 3<<RK_BS|3<<RK_DS|RK_IORF|RK_NBYTE|4);
   ptr("SDSB3", res, "==-0.13==", 9);

/* Set 6:  Scale testing with short items */

   strcpy(res, in15);
   wbcdwt(ul2+0, res+2, 3<<RK_BS|2<<RK_DS|RK_NUNS|RK_IORF|RK_NHALF|14);
   ptr("UDSH1", res, "==              0==", 19);

   strcpy(res, in15);
   wbcdwt(ul2+1, res+2, 4<<RK_BS|3<<RK_DS|RK_NUNS|RK_IORF|RK_NHALF|14);
   ptr("UDSH2", res, "==          16.00==", 19);

   strcpy(res, in15);
   wbcdwt(ul2+2, res+2, 5<<RK_BS|5<<RK_DS|RK_NUNS|RK_IORF|RK_NHALF|14);
   ptr("UDSH3", res, "==       128.0313==", 19);

   strcpy(res, in15);
   wbcdwt(ul2+3, res+2, 7<<RK_BS|7<<RK_DS|RK_NUNS|RK_IORF|RK_NHALF|14);
   ptr("UDSH4", res, "==     511.992188==", 19);

   strcpy(res, in15);
   wbcdwt(sl2+0, res+2, 4<<RK_BS|4<<RK_DS|RK_IORF|RK_NHALF|14);
   ptr("SDSH1", res, "==              0==", 19);

   strcpy(res, in15);
   wbcdwt(sl2+1, res+2, 4<<RK_BS|4<<RK_DS|RK_IORF|RK_NHALF|14);
   ptr("SDSH2", res, "==         15.938==", 19);

   strcpy(res, in15);
   wbcdwt(sl2+2, res+2, 7<<RK_BS|8<<RK_DS|RK_IORF|RK_NHALF|14);
   ptr("SDSH3", res, "==     -0.0078125==", 19);

   strcpy(res, in15);
   wbcdwt(sl2+3, res+2, 4<<RK_BS|5<<RK_DS|RK_IORF|RK_NHALF|14);
   ptr("SDSH4", res, "==     -2048.0000==", 19);

/* Set 7:  Scale testing with 32-bit items */

   strcpy(res, in15);
   wbcdwt(ul4+0, res+2, 7<<RK_BS|9<<RK_DS|RK_NUNS|RK_IORF|RK_NI32|14);
   ptr("UDSL1", res, "==   511.99218750==", 19);

   strcpy(res, in15);
   wbcdwt(ul4+1, res+2, 8<<RK_BS|8<<RK_DS|RK_NUNS|RK_IORF|RK_NI32|14);
   ptr("UDSL2", res, "==    256.0000000==", 19);

   strcpy(res, in15);
   wbcdwt(ul4+2, res+2, RK_DSF|4<<RK_DS|RK_NUNS|RK_IORF|RK_NI32|14);
   ptr("UDSL3", res, "==       9999.999==", 19);

   strcpy(res, in15);
   wbcdwt(ul4+3, res+2, 16<<RK_BS|RK_NUNS|RK_IORF|RK_NI32|14);
   ptr("UDSL4", res, "==          32768==", 19);

   strcpy(res, in15);
   wbcdwt(ul4+4, res+2, 24<<RK_BS|12<<RK_DS|RK_NUNS|RK_IORF|RK_NI32|14);
   ptr("UDSL5", res, "==255.99999994040==", 19);

   strcpy(res, in15);
   wbcdwt(sl4+0, res+2, 8<<RK_BS|8<<RK_DS|RK_IORF|RK_NI32|14);
   ptr("SDSL1", res, "==    255.9960938==", 19);

   strcpy(res, in15);
   wbcdwt(sl4+1, res+2, 8<<RK_BS|8<<RK_DS|RK_IORF|RK_NI32|14);
   ptr("SDSL2", res, "==   -255.9960938==", 19);

   strcpy(res, in15);
   wbcdwt(sl4+2, res+2, 23<<RK_BS|9<<RK_DS|RK_IORF|RK_NI32|14);
   ptr("SDSL3", res, "==    -4.00000000==", 19);

   strcpy(res, in15);
   wbcdwt(sl4+3, res+2, 4<<RK_BS|5<<RK_DS|RK_IORF|RK_NI32|14);
   ptr("SDSL4", res, "==        -0.0625==", 19);

   strcpy(res, in15);
   wbcdwt(sl4+4, res+2, RK_IORF|RK_NI32|14);
   ptr("SDSL5", res, "==    -2147483648==", 19);

/* Set 8:  Scale testing with 64-bit items */

   strcpy(res, in25);
   wbcdwt(ul8+0, res+2, 32<<RK_BS|15<<RK_DS|RK_NUNS|RK_IORF|RK_NI64|24);
   ptr("UDSW1", res, "==         0.99999999976717==", 29);

   strcpy(res, in25);
   wbcdwt(ul8+1, res+2, 16<<RK_BS|15<<RK_DS|RK_NUNS|RK_IORF|RK_NI64|24);
   ptr("UDSW2", res, "==     65536.00000000000000==", 29);

   strcpy(res, in25);
   wbcdwt(ul8+2, res+2, 3<<RK_DS|RK_NUNS|RK_IORF|RK_NI64|24);
   ptr("UDSW3", res, "==           99999999999.00==", 29);

   strcpy(res, in25);
   wbcdwt(ul8+3, res+2, 63<<RK_BS|8<<RK_DS|RK_NUNS|RK_IORF|RK_NI64|24);
   ptr("UDSW4", res, "==                1.0000000==", 29);

   strcpy(res, in25);
   wbcdwt(ul8+4, res+2, 63<<RK_BS|15<<RK_DS|RK_NUNS|RK_IORF|RK_NI64|24);
   ptr("UDSW5", res, "==         2.00000000000000==", 29);

   strcpy(res, in25);
   wbcdwt(sl8+0, res+2, 32<<RK_BS|5<<RK_DS|RK_IORF|RK_NI64|24);
   ptr("SDSW1", res, "==                  -0.5000==", 29);

   strcpy(res, in25);
   wbcdwt(sl8+1, res+2, 34<<RK_BS|6<<RK_DS|RK_IORF|RK_NI64|24);
   ptr("SDSW2", res, "==                 -0.25000==", 29);

   strcpy(res, in25);
   wbcdwt(sl8+2, res+2, RK_DSF|5<<RK_DS|RK_IORF|RK_NI64|24);
   ptr("SDSW3", res, "==            -9999999.9999==", 29);

   strcpy(res, in25);
   wbcdwt(sl8+3, res+2, 63<<RK_BS|15<<RK_DS|RK_IORF|RK_NI64|24);
   ptr("SDSW4", res, "==        -0.00000000000000==", 29);

   strcpy(res, in25);
   wbcdwt(sl8+4, res+2, 60<<RK_BS|5<<RK_DS|RK_IORF|RK_NI64|24);
   ptr("SDSW5", res, "==                  -8.0000==", 29);

/*---------------------------------------------------------------------*
*     Stage III.  Test hexadecimal, octal, and exponential output      *
*---------------------------------------------------------------------*/

/* Set 9: Straight hexadecimal */

   strcpy(res, in05);
   wbcdwt(ul1+1, res+2, RK_NUNS|RK_HEXF|RK_NBYTE|4);
   ptr("UHB1", res, "==   3F==", 9);

   strcpy(res, in05);
   wbcdwt(sl1+2, res+2, RK_HEXF|RK_NZTW|RK_NBYTE|4);
   ptr("SHB1", res, "==  -01==", 9);

   strcpy(res, in05);
   wbcdwt(ul2+1, res+2, RK_HEXF|RK_NZTW|RK_NHALF|3);
   ptr("UHN1", res, "==0100x==", 9);

   strcpy(res, in05);
   wbcdwt(ul4+2, res+2, RK_HEXF|RK_NI32|4);
   ptr("UHN2", res, "== 1.+7==", 9);

   strcpy(res, in15);
   wbcdwt(ul2+2, res+2, RK_NUNS|RK_NZ0X|RK_NZLC|RK_HEXF|RK_NHALF|14);
   ptr("UHH1", res, "==         0x1001==", 19);

   strcpy(res, in15);
   wbcdwt(sl2+2, res+2, RK_HEXF|RK_NZ0X|RK_NZTW|RK_NHALF|14);
   ptr("SHH1", res, "==        -0X0001==", 19);

   strcpy(res, in15);
   wbcdwt(ul4+2, res+2, RK_NUNS|RK_HEXF|RK_NZTW|RK_NI32|14);
   ptr("UHL1", res, "==       0098967F==", 19);

   strcpy(res, in15);
   wbcdwt(sl4+2, res+2, RK_HEXF|RK_NI32|14);
   ptr("SHL1", res, "==       -2000000==", 19);

   strcpy(res, in25);
   wbcdwt(ul8+2, res+2, RK_NUNS|RK_NZ0X|RK_NZTW|RK_HEXF|RK_NI64|24);
   ptr("UHW1", res, "==       0X000000174876E7FF==", 29);

   strcpy(res, in25);
   wbcdwt(sl8+1, res+2, RK_HEXF|RK_NZTW|RK_NI64|24);
   ptr("SHW1", res, "==        -0000000100000000==", 29);

/* Set 10: Straight octal */

   strcpy(res, in05);
   wbcdwt(ul1+1, res+2, RK_NUNS|RK_OCTF|RK_NBYTE|4);
   ptr("UOB1", res, "==   77==", 9);

   strcpy(res, in05);
   wbcdwt(sl1+2, res+2, RK_OCTF|RK_NBYTE|4);
   ptr("SOB1", res, "==   -1==", 9);

   strcpy(res, in15);
   wbcdwt(ul2+2, res+2, RK_NUNS|RK_Oct0|RK_OCTF|RK_NHALF|14);
   ptr("UOH1", res, "==         010001==", 19);

   strcpy(res, in15);
   wbcdwt(sl2+2, res+2, RK_OCTF|RK_Oct0|RK_NHALF|14);
   ptr("SOH1", res, "==            -01==", 19);

   strcpy(res, in15);
   wbcdwt(ul4+2, res+2, RK_NUNS|RK_OCTF|RK_NI32|14);
   ptr("UOL1", res, "==       46113177==", 19);

   strcpy(res, in15);
   wbcdwt(sl4+2, res+2, RK_OCTF|RK_NI32|14);
   ptr("SOL1", res, "==     -200000000==", 19);

   strcpy(res, in25);
   wbcdwt(ul8+2, res+2, RK_NUNS|RK_Oct0|RK_OCTF|RK_NI64|24);
   ptr("UOW1", res, "==           01351035563777==", 29);

   strcpy(res, in25);
   wbcdwt(sl8+1, res+2, RK_OCTF|RK_NI64|24);
   ptr("SOW1", res, "==             -40000000000==", 29);

/* Set 11: Explicit exponential */

   strcpy(res, in07);
   wbcdwt(ul1+1, res+2, 3<<RK_DS|RK_NUNS|RK_EFMT|RK_NBYTE|6);
   ptr("UEB1", res, "== 6.30+1==", 11);

   strcpy(res, in05);
   wbcdwt(sl1+2, res+2, RK_EFMT|RK_NBYTE|4);
   ptr("SEB1", res, "==   -1==", 9);

   strcpy(res, in15);
   wbcdwt(ul2+2, res+2, 5<<RK_BS|5<<RK_DS|RK_NUNS|RK_EFMT|RK_NHALF|14);
   ptr("UEH1", res, "==       1.2803+2==", 19);

   strcpy(res, in15);
   wbcdwt(sl2+2, res+2, 4<<RK_BS|3<<RK_DS|RK_EFMT|RK_NHALF|14);
   ptr("SEH1", res, "==        -6.25-2==", 19);

   strcpy(res, in15);
   wbcdwt(ul4+2, res+2, 9<<RK_DS|RK_NUNS|RK_EFMT|RK_NI32|14);
   ptr("UEL1", res, "==   9.99999900+6==", 19);

   strcpy(res, in15);
   RK.expwid = 4;
   wbcdwt(sl4+2, res+2, 2<<RK_BS|8<<RK_DS|RK_EFMT|RK_NI32|14);
   ptr("SEL1", res, "== -8.3886080+006==", 19);
   RK.expwid = 0;

   strcpy(res, in25);
   wbcdwt(ul8+2, res+2, 5<<RK_DS|RK_NUNS|RK_EFMT|RK_NI64|24);
   ptr("UEW1", res, "==                1.0000+11==", 29);

   strcpy(res, in25);
   wbcdwt(sl8+1, res+2, 24<<RK_BS|15<<RK_DS|RK_EFMT|RK_NI64|24);
   ptr("SEW1", res, "==      -2.56000000000000+2==", 29);

/*---------------------------------------------------------------------*
*              Stage IV.  Test miscellaneous option bits               *
*---------------------------------------------------------------------*/

/* Set 12: Left justify */

   strcpy(res, in25);
   wbcdwt(ul8+3, res+2, RK_NUNS|RK_LFTJ|RK_IORF|RK_NI64|24);
   ptr("LJW1", res, "==9223372036854775808      ==", 29);

   strcpy(res, in15);
   wbcdwt(sl4+3, res+2, 4<<RK_BS|5<<RK_DS|RK_LFTJ|RK_IORF|RK_NI32|14);
   ptr("LJL2", res, "==-0.0625        ==", 19);

   strcpy(res, in15);
   wbcdwt(sl2+2, res+2, 4<<RK_BS|3<<RK_DS|RK_LFTJ|RK_EFMT|RK_NHALF|14);
   ptr("LJH3", res, "==-6.25-2        ==", 19);

/* Set 13: Pad with zeros */

   strcpy(res, in05);
   wbcdwt(ul1+1, res+2, RK_NUNS|RK_NPAD0|RK_IORF|RK_NBYTE|4);
   ptr("P0B1", res, "==00063==", 9);

   strcpy(res, in25);
   wbcdwt(sl8+4, res+2, RK_NPAD0|RK_IORF|RK_NI64|24);
   ptr("P0W2", res, "==-000009223372036854775808==", 29);

   strcpy(res, in15);
   wbcdwt(sl4+3, res+2, 4<<RK_BS|5<<RK_DS|RK_NPAD0|RK_IORF|RK_NI32|14);
   ptr("P0L3", res, "==-000000000.0625==", 19);

   strcpy(res, in15);
   wbcdwt(sl2+2, res+2, RK_HEXF|RK_NZ0X|RK_NPAD0|RK_NHALF|14);
   ptr("P0H4", res, "==-0X000000000001==", 19);

   strcpy(res, in15);
   wbcdwt(ul4+2, res+2, RK_NUNS|RK_OCTF|RK_NPAD0|RK_NI32|14);
   ptr("P0L5", res, "==000000046113177==", 19);

   strcpy(res, in15);   /* PAD0 ignored if exponential */
   wbcdwt(ul2+2, res+2, 5<<RK_BS|5<<RK_DS|RK_NUNS|RK_NPAD0|RK_EFMT|
      RK_NHALF|14);
   ptr("P0H6", res, "==00000001.2803+2==", 19);

/* Set 14: Left justify and pad to right with zeros */

   strcpy(res, in05);
   wbcdwt(ul1+1, res+2, RK_NUNS|RK_LFTJ|RK_NPAD0|RK_IORF|RK_NBYTE|4);
   ptr("PJB1", res, "==63000==", 9);

   strcpy(res, in25);
   wbcdwt(sl8+4, res+2, RK_LFTJ|RK_NPAD0|RK_IORF|RK_NI64|24);
   ptr("PJW2", res, "==-922337203685477580800000==", 29);

   strcpy(res, in15);
   wbcdwt(sl4+3, res+2, 4<<RK_BS|5<<RK_DS|RK_LFTJ|RK_NPAD0|RK_IORF|
      RK_NI32|14);
   ptr("PJL3", res, "==-0.062500000000==", 19);

   strcpy(res, in15);
   wbcdwt(sl2+2, res+2, RK_HEXF|RK_NZ0X|RK_NZTW|RK_LFTJ|RK_NPAD0|
      RK_NHALF|14);
   ptr("PJH4", res, "==-0X000100000000==", 19);

   strcpy(res, in15);
   wbcdwt(ul4+2, res+2, RK_NUNS|RK_OCTF|RK_LFTJ|RK_NPAD0|RK_NI32|14);
   ptr("PJL5", res, "==461131770000000==", 19);

/* Set 15: Automatic decimal */

   strcpy(res, in05);
   wbcdwt(ul1+2, res+2, 5<<RK_BS|RK_AUTO|RK_NUNS|RK_IORF|RK_NBYTE|4);
   ptr("ADB1", res, "== 7.97==", 9);

   strcpy(res, in15);
   wbcdwt(ul2+3, res+2, 7<<RK_BS|7<<RK_DS|RK_AUTO|RK_NUNS|RK_IORF|
      RK_NHALF|14);
   ptr("ADH2", res, "==     511.992188==", 19);

   strcpy(res, in15);
   wbcdwt(sl2+2, res+2, 7<<RK_BS|RK_AUTO|RK_IORF|RK_NHALF|14);
   ptr("ADH3", res, "==-0.007812500000==", 19);

   strcpy(res, in07);
   wbcdwt(ul4+4, res+2, 24<<RK_BS|RK_AUTO|RK_NUNS|RK_IORF|RK_NI32|6);
   ptr("ADL4", res, "== 256.00==", 11);

   strcpy(res, in15);
   wbcdwt(sl4+4, res+2, RK_AUTO|RK_IORF|RK_NI32|14);
   ptr("ADL5", res, "==-2147483648.000==", 19);

   strcpy(res, in25);
   wbcdwt(ul8+0, res+2, 32<<RK_BS|RK_AUTO|RK_NUNS|RK_IORF|RK_NI64|24);
   ptr("ADW6", res, "== 0.9999999997671693563461==", 29);

   strcpy(res, in15);
   wbcdwt(ul8+4, res+2, 63<<RK_BS|6<<RK_DS|RK_AUTO|RK_NUNS|
      RK_IORF|RK_NI64|14);
   ptr("ADW7", res, "==        2.00000==", 19);

/* Set 16: Underflow protection */

   strcpy(res, in05);   /* No underflow--should have no effect */
   wbcdwt(ul1+1, res+2, 4<<RK_BS|3<<RK_DS|RK_NUNS|RK_UFLW|RK_IORF|
      RK_NBYTE|4);
   ptr("UFB1", res, "== 3.94==", 9);

   strcpy(res, in15);
   wbcdwt(sl4+3, res+2, 17<<RK_BS|6<<RK_DS|RK_UFLW|RK_IORF|
      RK_NI32|14);
   ptr("UFL2", res, "==       -0.00001==", 19);

   strcpy(res, in15);
   wbcdwt(sl4+3, res+2, 17<<RK_BS|5<<RK_DS|RK_UFLW|RK_IORF|
      RK_NI32|14);
   ptr("UFL3", res, "== -7.629394531-6==", 19);

   strcpy(res, in07);
   wbcdwt(sl4+3, res+2, 17<<RK_BS|RK_AUTO|RK_UFLW|RK_IORF|
      RK_NI32|6);
   ptr("UFL4", res, "== -7.6-6==", 19);

   strcpy(res, in25);
   {  si64 m5 = jcsw(0xFFFFFFFF,0xFFFFFFFB);
      wbcdwt(&m5, res+2, 63<<RK_BS|13<<RK_DS|RK_UFLW|RK_IORF|
         RK_NI64|24);
      }
   ptr("UFW5", res, "==    -5.421010862427522-19==", 29);

   strcpy(res, in25);
   {  si64 p5 = jcsw(0,5);
      wbcdwt(&p5, res+2, 63<<RK_BS|13<<RK_DS|RK_UFLW|RK_IORF|
         RK_NI64|24);
      }
   ptr("UFW6", res, "==     5.421010862427522-19==", 29);

/* Set 17: Switch to exponential format on overflow */

   strcpy(res, in05);
   wbcdwt(ul2+3, res+2, RK_NUNS|RK_IORF|RK_NHALF|2);
   ptr("OFH1", res, "==***xx==", 9);

   strcpy(res, in07);
   wbcdwt(ul4+4, res+2, RK_NUNS|RK_IORF|RK_NI32|6);
   ptr("OFL2", res, "== 4.29+9==", 11);

   strcpy(res, in07);
   wbcdwt(sl4+2, res+2, RK_IORF|RK_NI32|6);
   ptr("OFL3", res, "== -3.4+7==", 11);

   strcpy(res, in15);
   wbcdwt(ul8+4, res+2, 3<<RK_BS|RK_NUNS|RK_IORF|RK_NI64|14);
   ptr("OFW4", res, "== 2.305843009+18==", 19);

   strcpy(res, in15);
   wbcdwt(ul8+2, res+2, 5<<RK_DS|RK_NUNS|RK_IORF|RK_NI64|14);
   ptr("OFW5", res, "== 1.000000000+11==", 19);

/* Set 18: Test RK_PLUS and RK_LSPC bits */

   strcpy(res, in15);
   wbcdwt(ul4+0, res+2, 7<<RK_BS|RK_PLUS|9<<RK_DS|RK_NUNS|RK_IORF|
      RK_NI32|14);
   ptr("UDP1", res, "==  +511.99218750==", 19);

   strcpy(res, in15);
   wbcdwt(sl4+1, res+2, 8<<RK_BS|RK_PLUS|8<<RK_DS|RK_IORF|RK_NI32|14);
   ptr("SDP2", res, "==   -255.9960938==", 19);

   strcpy(res, in15);
   wbcdwt(ul4+2, res+2, RK_PLUS|9<<RK_DS|RK_NUNS|RK_EFMT|RK_NI32|14);
   ptr("UEP3", res, "==  +9.99999900+6==", 19);

   strcpy(res, in15);
   wbcdwt(ul8+4, res+2, 3<<RK_BS|RK_PLUS|RK_NUNS|RK_IORF|RK_NI64|14);
   ptr("OFP4", res, "== 2.305843009+18==", 19);

   strcpy(res, in15);
   wbcdwt(ul4+0, res+2, 7<<RK_BS|RK_LSPC|RK_PLUS|9<<RK_DS|RK_NUNS|
      RK_IORF|RK_NI32|14);
   ptr("UDSP1", res, "==  +511.99218750==", 19);

   strcpy(res, in05);
   wbcdwt(ul1+2, res+2, 5<<RK_BS|RK_LSPC|4<<RK_DS|RK_NUNS|RK_IORF|
      RK_NBYTE|4);
   ptr("UDSP2", res, "== 7.97==", 9);

   strcpy(res, in25);
   wbcdwt(ul8+3, res+2, RK_LSPC|RK_NUNS|RK_LFTJ|RK_IORF|RK_NI64|24);
   ptr("UDSP3", res, "== 9223372036854775808     ==", 29);

/* Set 19: Test RK_MZERO */

   strcpy(res, in15);
   wbcdwt(sl2+3, res+2, 12<<RK_BS|4<<RK_DS|RK_MZERO|RK_IORF|RK_NHALF|14);
   ptr("SDZH1", res, "==             -0==", 19);

   strcpy(res, in15);
   wbcdwt(sl2+3, res+2, 15<<RK_BS|4<<RK_DS|RK_MZERO|RK_IORF|RK_NHALF|14);
   ptr("SDZH2", res, "==             -0==", 19);

   strcpy(res, in15);
   wbcdwt(sl2+4, res+2, 12<<RK_BS|4<<RK_DS|RK_MZERO|RK_IORF|RK_NHALF|14);
   ptr("SDZH3", res, "==         -4.000==", 19);

   strcpy(res, in15);
   wbcdwt(sl4+4, res+2, 12<<RK_BS|5<<RK_DS|RK_MZERO|RK_IORF|RK_NI32|14);
   ptr("SDZH4", res, "==             -0==", 19);

   strcpy(res, in15);
   wbcdwt(sl4+4, res+2, 15<<RK_BS|5<<RK_DS|RK_MZERO|RK_IORF|RK_NI32|14);
   ptr("SDZH5", res, "==             -0==", 19);

   strcpy(res, in15);
   wbcdwt(sl4+1, res+2, 12<<RK_BS|5<<RK_DS|RK_MZERO|RK_IORF|RK_NI32|14);
   ptr("SDZH6", res, "==       -15.9998==", 19);

/* THE END */

   printf("\nEnd wbcdwt tests, got %d correct, %d errors.\n",
      ngood, nbad);
   return 0;
   } /* End main program */
