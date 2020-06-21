/*--------------------------------------------------------------------*/
/*                                                                    */
/*    XX64TEST C -- 64-bit arithmetic test program                    */
/*                                                                    */
/*    This program is designed to test the 9 64-bit arithmetic        */
/*    routines contained in the xx64xx package plus the 4 newer       */
/*    "multiply and add" routines.  The "correct" answers reported    */
/*    for each test were obtained by running the REXX program         */
/*    'xx64test exec' with 24-digit decimal arithmetic enabled,       */
/*    or, in some cases, by hand calculation.                         */
/*                                                                    */
/*    Thoughts for further tests:  Multiplication by 0 and by most    */
/*    negative number (0x80000000), multiplication and division by    */
/*    1 are not currently tested.                                     */
/*                                                                    */
/*    V1A, 03/05/89, G. N. Reeke                                      */
/*    V1B, 08/25/93, GNR - Add more tests of negative values          */
/*    V1C, 07/10/99, GNR - Add tests for amssw, etc.                  */
/*    V1D, 10/29/13, GNR - Change longs to si32, ui32 as appropriate  */
/*    Rev, 04/07/14, GNR - swlo --> swlou where appropriate           */
/*--------------------------------------------------------------------*/

/* Include standard library functions */

#define MAIN
#include <stdio.h>
#include <stdlib.h>
#include "sysdef.h"
#include "rocks.h"
#include "rkarith.h"

/*---------------------------------------------------------------------*
*                                 ptr                                  *
*                   Function to print test results                     *
*                                                                      *
*  Synopsis:                                                           *
*     void ptr(char *name, long test, long corr)                       *
*     void ptru(char *name, unsigned long test, unsigned long corr)    *
*---------------------------------------------------------------------*/

static long errcnt = 0;

void ptr(char *name, si32 test, si32 corr) {

   static char *Result[2] = { "***INCORRECT***", "OK" };
   int OK = (test == corr);
   printf("   %s = %d, correct = %d  %s\n",name,
      (int)test,(int)corr,Result[OK]);
   if (!OK) ++errcnt;
   }

void ptru(char *name, ui32 test, ui32 corr) {

   static char *Result[2] = { "***INCORRECT***", "OK" };
   int OK = (test == corr);
   printf("   %s = %u, correct = %u  %s\n",name,
      (unsigned int)test,(unsigned int)corr,Result[OK]);
   if (!OK) ++errcnt;
   }

/*---------------------------------------------------------------------*
*                          main test program                           *
*---------------------------------------------------------------------*/

int main() {

/* Declarations: */

   si64 ssum;
   ui64 usum,umul;
   long e64err;
   si32 m1 = 123456;  si32 m2 = 456789;
   si32 n1 = 1234567; si32 n2 = 8294733;
   si32 d1 = 1000;    si32 d2 = 2983457;
   si32 bigmod = 0X7FFFFFFF;
   si32 bigm1 =  0X7FFFFFFEL;
   ui32 biglow = 0X81010000L;
   si32 q,r;
   si32 hi;
   ui32 lo;

/* Print a nice little header */

   printf("Test xx64xx Arithmetic Routines\n");

/* Test dm64nb */

   printf("\n===DM64NB===\n");

   q = dm64nb(m1,m2,d1,&r);
   ptr("Q1",q,56393342); ptr("R1",r,784);

   q = dm64nb(-m1,m2,d1,&r);
   ptr("Q2",q,-56393342); ptr("R2",r,-784);

   q = dm64nb(n1,n2,bigmod,&r);
   ptr("Q3",q,4768); ptr("R3",r,1201606715);

   q = dm64nb(bigmod,bigm1,bigmod,&r);
   ptr("Q4+++",q,bigm1); ptr("R4+++",r,0);

   q = dm64nb(bigmod,bigm1,-bigmod,&r);
   ptr("Q4++-",q,-bigm1); ptr("R4++-",r,0);

   q = dm64nb(bigmod,-bigm1,bigmod,&r);
   ptr("Q4+-+",q,-bigm1); ptr("R4+-+",r,0);

   q = dm64nb(bigmod,-bigm1,-bigmod,&r);
   ptr("Q4+--",q,bigm1); ptr("R4+--",r,0);

   q = dm64nb(-bigmod,bigm1,bigmod,&r);
   ptr("Q4-++",q,-bigm1); ptr("R4-++",r,0);

   q = dm64nb(-bigmod,bigm1,-bigmod,&r);
   ptr("Q4-+-",q,bigm1); ptr("R4-+-",r,0);

   q = dm64nb(-bigmod,-bigm1,bigmod,&r);
   ptr("Q4--+",q,bigm1); ptr("R4--+",r,0);

   q = dm64nb(-bigmod,-bigm1,-bigmod,&r);
   ptr("Q4---",q,-bigm1); ptr("R4---",r,0);

/* Test dm64nq */

   printf("\n===DM64NQ===\n");

   q = dm64nq(m1,m2,d1);
   ptr("Q1",q,56393342);

   q = dm64nq(-m1,m2,d1);
   ptr("Q2",q,-56393342);

   q = dm64nq(n1,n2,bigmod);
   ptr("Q3",q,4768);

   q = dm64nq(bigmod,bigm1,bigmod);
   ptr("Q4+++",q,bigm1);

   q = dm64nq(bigmod,bigm1,-bigmod);
   ptr("Q4++-",q,-bigm1);

   q = dm64nq(bigmod,-bigm1,bigmod);
   ptr("Q4+-+",q,-bigm1);

   q = dm64nq(bigmod,-bigm1,-bigmod);
   ptr("Q4+--",q,bigm1);

   q = dm64nq(-bigmod,bigm1,bigmod);
   ptr("Q4-++",q,-bigm1);

   q = dm64nq(-bigmod,bigm1,-bigmod);
   ptr("Q4-+-",q,bigm1);

   q = dm64nq(-bigmod,-bigm1,bigmod);
   ptr("Q4--+",q,bigm1);

   q = dm64nq(-bigmod,-bigm1,-bigmod);
   ptr("Q4---",q,-bigm1);

/* Test dm64nr */

   printf("\n===DM64NR===\n");

   r = dm64nr(m1,m2,d1);
   ptr("R1",r,784);

   r = dm64nr(-m1,m2,d1);
   ptr("R2",r,-784);

   r = dm64nr(n1,n2,bigmod);
   ptr("R3",r,1201606715);

   r = dm64nr(bigmod,bigm1,bigmod);
   ptr("R4+++",r,0);

   r = dm64nr(bigmod,bigm1,-bigmod);
   ptr("R4++-",r,0);

   r = dm64nr(bigmod,-bigm1,bigmod);
   ptr("R4+-+",r,0);

   r = dm64nr(bigmod,-bigm1,-bigmod);
   ptr("R4+--",r,0);

   r = dm64nr(-bigmod,bigm1,bigmod);
   ptr("R4-++",r,0);

   r = dm64nr(-bigmod,bigm1,-bigmod);
   ptr("R4-+-",r,0);

   r = dm64nr(-bigmod,-bigm1,bigmod);
   ptr("R4--+",r,0);

   r = dm64nr(-bigmod,-bigm1,-bigmod);
   ptr("R4---",r,0);

/* Test ds64nq */

   printf("\n===DS64NQ===\n");

   q = ds64nq(m1,(ui32)m2,3,d2);
   ptr("Q1",q,1421812301);

   q = ds64nq(m1,(ui32)m2,0,d2);
   ptr("Q2",q,177726537);

   q = ds64nq(m1,(ui32)m2,-3,d2);
   ptr("Q3",q,22215817);

   q = ds64nq(m1,biglow,3,d2);
   ptr("Q4",q,1421818103);

   q = ds64nq(-m1,biglow,-3,d2);
   ptr("Q5",q,-22215726);

   q = ds64nq(-n1,biglow,-32,d1);
   ptr("Q6",q,-1234);

   q = ds64nq(0,(ui32)m2,32,d2);
   ptr("Q7",q,657590780);

   q = ds64nq(-1,biglow,-16,1);
   ptr("Q8a",q,-32511);

   q = ds64nq(-1,(biglow+1),-16,1);
   ptr("Q8b",q,-32511);

/* Test jm64nb */

   printf("\n===JM64NB===\n");

   hi = jm64nb(n1,n2,&lo);
   ptr("HI1++",hi,2384); ptru("LO1++",lo,1201601947U);

   hi = jm64nb(n1,-n2,&lo);
   ptr("HI1+-",hi,-2385); ptru("LO1+-",lo,3093365349U);

   hi = jm64nb(-n1,n2,&lo);
   ptr("HI1-+",hi,-2385); ptru("LO1-+",lo,3093365349U);

   hi = jm64nb(-n1,-n2,&lo);
   ptr("HI1--",hi,2384); ptru("LO1--",lo,1201601947U);

   hi = jm64nb(bigmod,bigmod,&lo);
   ptr("HI2++",hi,0X3FFFFFFF); ptru("LO2++",lo,1);

   hi = jm64nb(bigmod,-bigmod,&lo);
   ptr("HI2+-",hi,-0X40000000); ptru("LO2+-",lo,0XFFFFFFFFU);

   hi = jm64nb(-bigmod,bigmod,&lo);
   ptr("HI2-+",hi,-0X40000000); ptru("LO2-+",lo,0XFFFFFFFFU);

   hi = jm64nb(-bigmod,-bigmod,&lo);
   ptr("HI2--",hi,0X3FFFFFFF); ptru("LO2--",lo,1);

/* Test jm64nh */

   printf("\n===JM64NH===\n");

   hi = jm64nh(n1,n2);
   ptr("HI1++",hi,2384);

   hi = jm64nh(n1,-n2);
   ptr("HI1+-",hi,-2385);

   hi = jm64nh(-n1,n2);
   ptr("HI1-+",hi,-2385);

   hi = jm64nh(-n1,-n2);
   ptr("HI1--",hi,2384);

   hi = jm64nh(bigmod,bigmod);
   ptr("HI2++",hi,0X3FFFFFFF);

   hi = jm64nh(bigmod,-bigmod);
   ptr("HI2+-",hi,-0X40000000);

   hi = jm64nh(-bigmod,bigmod);
   ptr("HI2-+",hi,-0X40000000);

   hi = jm64nh(-bigmod,-bigmod);
   ptr("HI2--",hi,0X3FFFFFFF);

/* Test jm64sb */

   printf("\n===JM64SB===\n");

   hi = jm64sb(n1,n2,3,&lo);
   ptr("HIn+++",hi,19074); ptru("LOn+++",lo,1022880984U);

   hi = jm64sb(n1,n2,0,&lo);
   ptr("HIn++0",hi,2384); ptru("LOn++0",lo,1201601947U);

   hi = jm64sb(n1,n2,-3,&lo);
   ptr("HIn++-",hi,298); ptru("LOn++-",lo,150200243U);

   hi = jm64sb(-n1,n2,3,&lo);
   ptr("HIn-++",hi,-19075); ptru("LOn-++",lo,3272086312U);

   hi = jm64sb(-n1,n2,0,&lo);
   ptr("HIn-+0",hi,-2385); ptru("LOn-+0",lo,3093365349U);

   hi = jm64sb(-n1,n2,-3,&lo);
   ptr("HIn-+-",hi,-299); ptru("LOn-+-",lo,4144767052U);

   hi = jm64sb(bigmod,bigm1,-36,&lo);
   ptr("HI2++-",hi,0); ptru("LO2++-",lo,67108863U);

   hi = jm64sb(bigmod,-bigm1,-36,&lo);
   ptr("HI2+--",hi,-1); ptru("LO2+--",lo,0XFC000000U);

   hi = jm64sb(m1,d1,36,&lo);
   ptr("HI3+++",hi,1975296000); ptru("LO3+++",lo,0);

   hi = jm64sb(m1,-d1,36,&lo);
   ptr("HI3+-+",hi,-1975296000); ptru("LO3+-+",lo,0);

/* Test jm64sh */

   printf("\n===JM64SH===\n");

   hi = jm64sh(n1,n2,3);
   ptr("HIn+++",hi,19074);

   hi = jm64sh(n1,n2,0);
   ptr("HIn++0",hi,2384);

   hi = jm64sh(n1,n2,-3);
   ptr("HIn++-",hi,298);

   hi = jm64sh(-n1,n2,3);
   ptr("HIn-++",hi,-19075);

   hi = jm64sh(-n1,n2,0);
   ptr("HIn-+0",hi,-2385);

   hi = jm64sh(-n1,n2,-3);
   ptr("HIn-+-",hi,-299);

   hi = jm64sh(bigmod,bigm1,-36);
   ptr("HI2++-",hi,0);

   hi = jm64sh(bigmod,-bigm1,-36);
   ptr("HI2+--",hi,-1);

   hi = jm64sh(m1,d1,36);
   ptr("HI3+++",hi,1975296000);

   hi = jm64sh(m1,-d1,36);
   ptr("HI3+-+",hi,-1975296000);

/* Test jm64sl */

   printf("\n===JM64SL===\n");

   lo = jm64sl(n1,n2,3);
   ptru("LOn+++",lo,1022880984U);

   lo = jm64sl(n1,n2,0);
   ptru("LOn++0",lo,1201601947U);

   lo = jm64sl(n1,n2,-3);
   ptru("LOn++-",lo,150200243U);

   lo = jm64sl(-n1,n2,3);
   ptru("LOn-++",lo,3272086312U);

   lo = jm64sl(-n1,n2,0);
   ptru("LOn-+0",lo,3093365349U);

   lo = jm64sl(-n1,n2,-3);
   ptru("LOn-+-",lo,4144767052U);

   lo = jm64sl(bigmod,bigm1,-36);
   ptru("LO2++-",lo,67108863U);

   lo = jm64sl(bigmod,-bigm1,-36);
   ptru("LO2+--",lo,0XFC000000U);

   lo = jm64sl(m1,d1,36);
   ptru("LO3+++",lo,0);

   lo = jm64sl(m1,-d1,36);
   ptru("LO3+-+",lo,0);

/* Test amssw--not the most thorough.
*  N.B.  No deliberate overflows in this test,
*  you would get different results from .c and .asm
*  versions, which is OK according to specs.  */

   printf("\n===AMSSW===\n");

   ssum = jcsw(3,1);
   ssum = amssw(ssum, 0xFFFF, 0x30001, 0);
   ssum = amssw(ssum, 0x7ffffffe, 0x7ffffffd, -8);
   ssum = amssw(ssum, 0x30, 0x50, 4);
   ssum = amssw(ssum, 0x31, -81, -1);
   ptr ("HIAMSSW", swhi(ssum), 0x400005);
   ptru("LOAMSSW", swlou(ssum), 0xfd7ee83fU);

/* Test amsswe.  Here we test the deliberate overflow
*  case, which now must give consistent results.  */

   printf("\n===AMSSWE===\n");

   e64set(E64_COUNT, &e64err);
   e64err = 0;
   ssum = jcsw(3,1);
   ssum = amsswe(ssum, 0xFFFF, 0x30001, 0, 0);
   /* The next step has a deliberate overflow */
   ssum = amsswe(ssum, 0x7ffffffe, 0x7ffffffd, 4, 0);
   ssum = amsswe(ssum, 0x30, 0x50, -4, 0);
   ssum = amsswe(ssum, 0x31, 0x51, -1, 0);
   ptr ("HIAMSSWE", swhi(ssum), 0x7fffffff);
   ptru("LOAMSSWE", swlou(ssum), 0xffffffffU);
   ptr ("ECAMSSWE", e64err, 4);

/* Test amsuw--not the most thorough.
*  N.B.  No deliberate overflows in this test,
*  you would get different results from .c and .asm
*  versions, which is OK according to specs.
*  But it is important to have some args and the result
*  greater than 2**63 to see if they are handled correctly.  */

   printf("\n===AMSUW===\n");

   usum = jcuw(3,1);
   usum = amsuw(usum, 0xFFFFU, 0x30001U, 0);
   usum = amsuw(usum, 0xffffffe0U, 0xffffffd0U, -8);
   usum = amsuw(usum, 0x30U, 0x50U, 4);
   usum = amsuw(usum, 0x7F10U, 16U, 44);
   ptru("HIAMSUW", uwhi(usum), 0x80100005U);
   ptru("LOAMSUW", uwlo(usum), 0xaffef006U);

/* Test amsuwe.  Here we test the deliberate overflow
*  case, which now must give consistent results.  */

   printf("\n===AMSUWE===\n");

   e64err = 0;
   usum = jcuw(3,1);
   usum = amsuwe(usum, 0xFFFFU, 0x30001U, 0, 0);
   /* The next step has a deliberate overflow */
   usum = amsuwe(usum, 0xfffffffeU, 0x7ffffffdU, 4, 0);
   usum = amsuwe(usum, 0x30U, 0x50U, -4, 0);
   usum = amsuwe(usum, 0x7F10U, 16U, 44, 0);
   ptru("HIAMSSWE", uwhi(usum), 0xffffffffU);
   ptru("LOAMSSWE", uwlo(usum), 0xffffffffU);
   ptr ("ECAMSSWE", e64err, 4);

/* Test mrsuw */

   printf("\n===MRSUW===\n");

   umul = jeul(0xFFFF0U);
   usum = mrsuw(umul, 0x30001U, 4);
   ptru("HIMSUW1", uwhi(usum), 2U);
   ptru("LOMSUW1", uwlo(usum), 0xfffdffffU);

   umul = jeul(0xFFFFFFFFU);
   usum = mrsuw(umul, 0xFFFFFFFFU, 0);
   ptru("HIMSUWSQ", uwhi(usum), 0xfffffffeU);
   ptru("LOMSUWSQ", uwlo(usum), 1U);

/* Test mrsuwe */

   printf("\n===MRSUWE===\n");

   e64err = 0;
   umul = jeul(0xFFFF0U);
   usum = mrsuwe(umul, 0x30001U, 4, 0);
   ptru("HIMSUWE1", uwhi(usum), 2U);
   ptru("LOMSUWE1", uwlo(usum), 0xfffdffffU);

   umul = jeul(0xFFFFFFFFU);
   usum = mrsuwe(umul, 0xFFFFFFFFU, 0, 0);
   ptru("HIMSUWESQ", uwhi(usum), 0xfffffffeU);
   ptru("LOMSUWESQ", uwlo(usum), 1U);
   ptr ("EMSUWE", e64err, 0);

   printf("\nEnd 64-bit arithmetic tests, got %ld errors.\n", errcnt);
   return 0;
   } /* End main program */
