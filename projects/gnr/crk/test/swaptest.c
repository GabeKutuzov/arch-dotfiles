/*--------------------------------------------------------------------*/
/*                                                                    */
/*                             Swap Test                              */
/*                                                                    */
/* V2A, 05/10/99, GNR - Test new lem-bem series of routines           */
/* Rev, 10/24/99, GNR - Add [bl]em{fm|to}[iu]8                        */
/* Rev, 10/26/13, GNR - LSIZE --> JSIZE                               */
/* Rev, 04/07/14, GNR - swlo --> swlou                                */
/*--------------------------------------------------------------------*/

/* This program tests the byte-order swapping routines.
*
*  N.B.  All test values should include bytes with sign bit
*  set in order to test that these do not propagate improperly!  */

#define  MAIN
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "rkarith.h"
#include "swap.h"

static void unpkhex(char *sout, char *sin, int l) {
   int i,j;
   static char hex[16] = "0123456789abcdef";
   for (i=j=0; i<l; i++) {
      unsigned int h = sin[i];
      h &= 0xff;
      sout[j++] = hex[h >> 4];
      sout[j++] = hex[h & 15];
      }
   sout[j++] = '\0';
   }

void main() {

   short i2;
   char i2x[2];

   long  i4;
   char i4x[4];

   si64  i8;
   ui64  u8;
   char i8x[8];

   union urr {
      float rr4;
      long ir4;
      } rr;
   char r4x[4];

   union udd {
      double rr8;
      struct { long ir8a,ir8b; } udl;
      } dd;
   char r8x[8];

   char hr[17];

   printf("Swap test:\n");

   i2 = 0x0182;
   lemfmi2(i2x, i2);
   unpkhex(hr, i2x, HSIZE);
   printf("lemfmi2 test gave 0x%s, correct is 0x8201\n", hr);

   i2 = lemtoi2(i2x);
   printf("lemtoi2 test gave %#x, correct is 0x0182\n", i2);

   i4 = 0x81820304;
   lemfmi4(i4x, i4);
   unpkhex(hr, i4x, JSIZE);
   printf("lemfmi4 test gave 0x%s, correct is 0x04038281\n", hr);

   i4 = lemtoi4(i4x);
   printf("lemtoi4 test gave %#lx, correct is 0x81820304\n", i4);

   i8 = jcsw(0xc1c2c3c4,0xc5c6c7c8);
   lemfmi8(i8x, i8);
   unpkhex(hr,i8x, WSIZE);
   printf("lemfmi8 test gave 0x%s, correct is "
      "0xc8c7c6c5c4c3c2c1\n", hr);

   i8 = lemtoi8(i8x);
   printf("lemtoi8 test gave %#lx%lx, correct is "
      "0xc1c2c3c4c5c6c7c8\n", swhi(i8), swlou(i8));

   u8 = jcuw(0x81828384,0x85868788);
   lemfmu8(i8x, u8);
   unpkhex(hr,i8x, WSIZE);
   printf("lemfmu8 test gave 0x%s, correct is "
      "0x8887868584838281\n", hr);

   u8 = lemtou8(i8x);
   printf("lemtou8 test gave %#lx%lx, correct is "
      "0x8182838485868788\n", uwhi(u8), uwlo(u8));

   rr.ir4 = 0xcacbcccd;
   lemfmr4(r4x, rr.rr4);
   unpkhex(hr, r4x, ESIZE);
   printf("lemfmr4 test gave 0x%s, correct is 0xcdcccbca\n", hr);

   rr.rr4 = lemtor4(r4x);
   printf("lemtor4 test gave %#lx, correct is 0xcacbcccd\n", rr.ir4);

#if BYTE_ORDRE < 0
   dd.udl.ir8a = 0xc5c6c7c8;
   dd.udl.ir8b = 0xc1c2c3c4;
#else
   dd.udl.ir8a = 0xc1c2c3c4;
   dd.udl.ir8b = 0xc5c6c7c8;
#endif
   lemfmr8(r8x, dd.rr8);
   unpkhex(hr, r8x, DSIZE);
   printf("lemfmr8 test gave 0x%s, correct is "
      "0xc8c7c6c5c4c3c2c1\n", hr);

   dd.rr8 = lemtor8(r8x);
   printf("lemtor8 test gave %#lx%lx, correct is "
#if BYTE_ORDRE < 0
      "0xc1c2c3c4c5c6c7c8\n", dd.udl.ir8b, dd.udl.ir8a);
#else
      "0xc1c2c3c4c5c6c7c8\n", dd.udl.ir8a, dd.udl.ir8b);
#endif

   i2 = 0x0182;
   bemfmi2(i2x, i2);
   unpkhex(hr, i2x, HSIZE);
   printf("bemfmi2 test gave 0x%s, correct is 0x0182\n", hr);

   i2 = bemtoi2(i2x);
   printf("bemtoi2 test gave %#x, correct is 0x0182\n", i2);

   i4 = 0x81820304;
   bemfmi4(i4x, i4);
   unpkhex(hr, i4x, JSIZE);
   printf("bemfmi4 test gave 0x%s, correct is 0x81820304\n", hr);

   i4 = bemtoi4(i4x);
   printf("bemtoi4 test gave %#lx, correct is 0x81820304\n", i4);

   i8 = jcsw(0xc1c2c3c4,0xc5c6c7c8);
   bemfmi8(i8x, i8);
   unpkhex(hr,i8x, WSIZE);
   printf("bemfmi8 test gave 0x%s, correct is "
      "0xc1c2c3c4c5c6c7c8\n", hr);

   i8 = bemtoi8(i8x);
   printf("bemtoi8 test gave %#lx%lx, correct is "
      "0xc1c2c3c4c5c6c7c8\n", swhi(i8), swlou(i8));

   u8 = jcuw(0x81828384,0x85868788);
   bemfmu8(i8x, u8);
   unpkhex(hr,i8x, WSIZE);
   printf("bemfmu8 test gave 0x%s, correct is "
      "0x8182838485868788\n", hr);

   u8 = bemtou8(i8x);
   printf("bemtou8 test gave %#lx%lx, correct is "
      "0x8182838485868788\n", uwhi(u8), uwlo(u8));

   rr.ir4 = 0xcacbcccd;
   bemfmr4(r4x, rr.rr4);
   unpkhex(hr, r4x, ESIZE);
   printf("bemfmr4 test gave 0x%s, correct is 0xcacbcccd\n", hr);

   rr.rr4 = bemtor4(r4x);
   printf("bemtor4 test gave %#lx, correct is 0xcacbcccd\n", rr.ir4);

#if BYTE_ORDRE < 0
   dd.udl.ir8a = 0xc5c6c7c8;
   dd.udl.ir8b = 0xc1c2c3c4;
#else
   dd.udl.ir8a = 0xc1c2c3c4;
   dd.udl.ir8b = 0xc5c6c7c8;
#endif
   bemfmr8(r8x, dd.rr8);
   unpkhex(hr, r8x, DSIZE);
   printf("bemfmr8 test gave 0x%s, correct is "
      "0xc1c2c3c4c5c6c7c8\n", hr);

   dd.rr8 = bemtor8(r8x);
   printf("bemtor8 test gave %#lx%lx, correct is "
#if BYTE_ORDRE < 0
      "0xc1c2c3c4c5c6c7c8\n", dd.udl.ir8b, dd.udl.ir8a);
#else
      "0xc1c2c3c4c5c6c7c8\n", dd.udl.ir8a, dd.udl.ir8b);
#endif

   } /* End of program */



