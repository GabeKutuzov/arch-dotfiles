/*--------------------------------------------------------------------*/
/*                                                                    */
/*    SU64TEST C -- 64-bit arithmetic test program                    */
/*                                                                    */
/*    This program is designed to test the basic 64-bit arithmetic    */
/*    routines with names of the form 'j.?[su]w*.c', the multiply-    */
/*    and-shift routines 'm[lr]s[su][lw]j?[de]?', the multiply-round- */
/*    and-shift routines 'mrsr[su][lw]j?[de]?', the multiply, divide, */
/*    and round routines, dmsrwwq[de], the divide and round routine,  */
/*    'drswq', and the shift-round-and-divide routines, 'dsr[su]wq'.  */
/*    The "correct" answers reported for each test were obtained by   */
/*    hand calculation or use of 'dc'.                                */
/*                                                                    */
/*    V1A, 06/27/99, G. N. Reeke - New program, based on xx64tests    */
/*    Rev, 03/22/03, GNR - Add tests for ja[su]l, ja[su]le, dsr[su]wq */
/*    Rev, 04/19/03, GNR - Add tests for mssw, msswe, msuw, msuwe     */
/*    Rev, 11/23/08, GNR - Add more stringent tests for msuwe,msswe   */
/*    Rev, 12/06/08, GNR - Add tests for msrswe, msruwe               */
/*    Rev, 12/28/08, GNR - Add tests for mssle, msrsle                */
/*    Rev, 01/11/09, GNR - Change jduwb to return 64-bit quotient     */
/*    Rev, 01/17/09, GNR - Add tests for jmuwb                        */
/*    Rev, 10/15/09, GNR - Add tests for msrule, msule                */
/*    Rev. 08/20/12, GNR - Add tests for jmuwwe                       */
/*    Rev, 10/20/12, GNR - Add tests for j[ar][su]aem                 */
/*    Rev, 12/01/12, GNR - Add tests for dsr[su]wqe, jdswqe, jdswbe,  */
/*                        jduwqe, msrsl, msrul                        */
/*    Rev, 05/26/13, GNR - Add tests for dsrswwqe, dsruwwqe           */
/*    Rev, 06/02/13, GNR - Add test for dmrswjwe, dsrsjqe             */
/*    Rev, 08/26/13, GNR - Add test for jmuwje                        */
/*    Rev, 05/25/14, GNR - Add tests for jrsle and jrule              */
/*    Rev, 09/05/14, GNR - Add tests for msswj, remove msrsl, msrul   */
/*    Rev, 09/30/14, GNR - Add tests for msr[su]wj                    */
/*    Rev, 10/08/14, GNR - Add mrs[rsu]x family (assume s >= 0) and   */
/*                        ms[rsu]x macros that call those with |s|.   */
/*    Rev, 10/11/14, GNR - jsl[su]w jsr[su]w, jsrr[su] families       */
/*    Rev, 12/27/16, GNR - Add drswq                                  */
/*    Rev, 08/28/17, GNR - Add dmrswwqe                               */
/*    Rev, 09/01/18, GNR - Add dsrswjqe                               */
/*--------------------------------------------------------------------*/

/* Include standard library functions */

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "sysdef.h"
#include "rkarith.h"

/***********************************************************************
*                                                                      *
*                 ABEXIT, ABEXITQ, ABEXITM, ABEXITMQ                   *
*                                                                      *
*      Special test versions to print message but not terminate        *
*                                                                      *
*  N.B.  Note that abexit() and abexitm() are needed because called    *
*  from e64set() and we are not providing cryout(), etc.  With -O2     *
*  compilation, apparently the return code is not even generated,      *
*  so a segfault occurs when abexitm() is called.  This is repaired    *
*  for our deliberate errors by using e64set() to have e64act() call   *
*  abexitr() instead of abexitm() on errors.  We have also modified    *
*  all the math routines that call abexit() or abexitm() (e.g. for     *
*  divide by 0 or shift out-of-range) to call abexitq() or abexitmq()  *
*  instead so that a return is expected.  The call to e64stest() will  *
*  be tested here in our printf() versions of those routines.  There   *
*  is no fix for abexit() and abexitm(), except to provide them here   *
*  with calls to printf() instead of cryout(), but hopefully those     *
*  calls from e64set() will never occur.                               *
***********************************************************************/

void abexit(int code) {

   printf("***Abexit called with code %d\n", code);
   } /* End abexit() */

void abexitq(int code) {

   printf("***Abexitq called with code %d\n", code);
   } /* End abexitq() */

void abexitm(int code, char *emsg) {

   printf("***Abexitm called with code %d and msg %s\n",
      code, emsg);
   } /* End abexitm() */

void abexitmq(int code, char *emsg) {

   printf("***Abexitmq called with code %d and msg %s\n",
      code, emsg);
   } /* End abexitmq() */

void abexitr(char *emsg, int code) {

   printf("***Abexitr called with code %d and msg %s\n",
      code, emsg);
   } /* End abexitr() */

/*---------------------------------------------------------------------*
*                                 ptr                                  *
*                   Functions to print test results                    *
*                                                                      *
*  Synopsis:                                                           *
*     void ptrsw(char *name, si64 test, si64 corr)                     *
*     void ptruw(char *name, ui64 test, ui64 corr)                     *
*     void ptrxw(char *name, si64 test, si64 ex64, si64 ex32)          *
*     void ptrsl(char *name, si32 test, si32 corr)                     *
*     void ptrul(char *name, ui32 test, ui32 corr)                     *
*---------------------------------------------------------------------*/

static long errcnt = 0;
static char *Result[2] = { "***INCORRECT***", "OK" };

void ptrsw(char *name, si64 test, si64 corr) {

#ifdef HAS_I64
   int OK = (test == corr);
#if LSIZE == 8
   printf("   %s = %#.18lx, correct = %#.18lx  %s\n",
      name, test, corr, Result[OK]);
#else
   printf("   %s = %#.18llx, correct = %#.18llx  %s\n",
      name, test, corr, Result[OK]);
#endif
#else
   int OK = (test.hi == corr.hi && test.lo == corr.lo);
   printf("   %s = %#.10lx%.8lx, correct = %#.10lx%.8lx  %s\n",
      name, test.hi, test.lo, corr.hi, corr.lo, Result[OK]);
#endif
   if (!OK) ++errcnt;
   }

void ptruw(char *name, ui64 test, ui64 corr) {

#ifdef HAS_I64
   int OK = (test == corr);
#if LSIZE == 8
   printf("   %s = %#.18lx, correct = %#.18lx  %s\n",
      name, test, corr, Result[OK]);
#else
   printf("   %s = %#.18llx, correct = %#.18llx  %s\n",
      name, test, corr, Result[OK]);
#endif
#else
   int OK = (test.hi == corr.hi && test.lo == corr.lo);
   printf("   %s = %#.10lx%.8lx, correct = %#.10lx%.8lx  %s\n",
      name, test.hi, test.lo, corr.hi, corr.lo, Result[OK]);
#endif
   if (!OK) ++errcnt;
   }

/* This one is for obsolete routines that unavoidably return
*  different results with HAS_I64 (that is why they are obsolete).
*  ex64 = expected result with HAS_I64,
*  ex32 = expected result without HAS_I64.
*  The ex64 result usually involves propagation during scaling of
*     an overflow into the sign bit, making the result negative.
*/
void ptrxw(char *name, si64 test, si64 ex64, si64 ex32) {

#ifdef HAS_I64
   int OK = (test == ex64);
#if LSIZE == 8
   printf("   %s, expected: %#.18lx (64-bit), %#.18lx (32-bit)\n"
      "   Result = %#.18lx, expected = %#.18lx  %s\n",
      name, ex64, ex32, test, ex64, Result[OK]);
#else
   printf("   %s, expected: %#.18llx (64-bit), %#.18llx (32-bit)\n"
      "   Result = %#.18llx, expected = %#.18llx  %s\n",
      name, ex64, ex32, test, ex64, Result[OK]);
#endif
#else
   int OK = (test.hi == ex32.hi && test.lo == ex32.lo);
   printf("   %s, expected: %#.10lx%.8lx (64-bit), %#.10lx%.8lx (32-bit)\n"
      "   Result = %#.10lx%.8lx, expected = %#.10lx%.8lx  %s\n",
      name, ex64.hi, ex64.lo, ex32.hi, ex32.lo,
      test.hi, test.lo, ex32.hi, ex32.lo, Result[OK]);
#endif
   if (!OK) ++errcnt;
   }

void ptrsl(char *name, long test, long corr) {

   int OK = (test == corr);
   printf("   %s = %#.10lx, correct = %#.10lx  %s\n",
      name, test, corr, Result[OK]);
   if (!OK) ++errcnt;
   }

void ptrul(char *name, unsigned long test, unsigned long corr) {

   int OK = (test == corr);
   printf("   %s = %#.10lx, correct = %#.10lx  %s\n",
      name, test, corr, Result[OK]);
   if (!OK) ++errcnt;
   }

/*---------------------------------------------------------------------*
*                          main test program                           *
*---------------------------------------------------------------------*/

int main() {

/* Declarations: */

   si64 sr,sc;
   ui64 ur,uc;
   si32 b15    = 0X0000FFFF;
   si32 b31    = 0X7FFFFFFF;
   si32 b31m1  = 0X7FFFFFFE;
   si32 m1     = 0XFFFFFFFF;
   si32 m2     = 0XFFFFFFFE;
   si32 bm     = 0X80000000;
   ui32 ub15   = 0X0000FFFFU;
   ui32 ub31   = 0X7FFFFFFFU;
   ui32 ub31m1 = 0X7FFFFFFEU;
   ui32 um1    = 0XFFFFFFFFU;
   ui32 um2    = 0XFFFFFFFEU;
   ui32 um3    = 0XFFFFFFFDU;
   ui32 u6u    = 0XFF6666FFU;
   ui32 ubm    = 0X80000000U;
   ui32 uq,uz;
   si32 q,r;

/* Print a nice little header */

   printf("Test si64-ui64 Arithmetic Routines\n");
   e64set(E64_CALL, abexitr);
   e64stest(TRUE);

/* Test jasw */

   printf("\n===jasw===\n");

   sr = jasw(jcsw(0,0), jcsw(0,0));
   sc = jcsw(0,0);
   ptrsw("AS1", sr, sc);

   sr = jasw(jcsw(100,200), jcsw(87,89));
   sc = jcsw(187,289);
   ptrsw("AS2", sr, sc);

   sr = jasw(jcsw(100,ub31), jcsw(87,um1));
   sc = jcsw(188,ub31m1);
   ptrsw("AS3", sr, sc);

   printf("The next test has a deliberate overflow--"
      "not detected by design.\n");
   sr = jasw(jcsw(b31,2), jcsw(b31,2));
   sc = jcsw(m2,4);
   ptrsw("AS4", sr, sc);

   sr = jasw(jcsw(m1,um2), jcsw(0,2));
   sc = jcsw(0,0);
   ptrsw("AS5", sr, sc);

   sr = jasw(jcsw(m1,0XFFFFFFFAL), jcsw(87,89));
   sc = jcsw(87,83);
   ptrsw("AS6", sr, sc);

   sr = jasw(jcsw(m2,89), jcsw(-1,ub15));
   sc = jcsw(-3,ub15+89);
   ptrsw("AS7", sr, sc);

   sr = jasw(jcsw(m2,89), jcsw(-1,um1));
   sc = jcsw(m2,88);
   ptrsw("AS8", sr, sc);

/* Test jaswe */

   printf("\n===jaswe===\n");

   sr = jaswe(jcsw(0,0), jcsw(0,0), 1);
   sc = jcsw(0,0);
   ptrsw("ASE1", sr, sc);

   sr = jaswe(jcsw(100,200), jcsw(87,89), 2);
   sc = jcsw(187,289);
   ptrsw("ASE2", sr, sc);

   sr = jaswe(jcsw(100,ub31), jcsw(87,um1) ,3);
   sc = jcsw(188,ub31m1);
   ptrsw("ASE3", sr, sc);

   printf("The next test has a deliberate overflow.\n");
   sr = jaswe(jcsw(b31,2), jcsw(b31,2), 4);
   sc = jcsw(b31,um1);
   ptrsw("ASE4", sr, sc);

   sr = jaswe(jcsw(m1,um2), jcsw(0,2), 5);
   sc = jcsw(0,0);
   ptrsw("ASE5", sr, sc);

   sr = jaswe(jcsw(m1,0XFFFFFFFAL), jcsw(87,89), 6);
   sc = jcsw(87,83);
   ptrsw("ASE6", sr, sc);

   sr = jaswe(jcsw(m2,89), jcsw(-1,ub15), 7);
   sc = jcsw(-3,ub15+89);
   ptrsw("ASE7", sr, sc);

   sr = jaswe(jcsw(m2,89), jcsw(-1,um1), 8);
   sc = jcsw(m2,88);
   ptrsw("ASE8", sr, sc);

/* Test jasjem */
/* Note:  jasiem, jaslem have same code except type in
*  overflow replacement, so these are not tested separately.  */

   printf("\n===jasjem===\n");

   jasjem(q, b15, b15, 1);
   r = b15+b15;
   ptrsl("ASA1", q, r);

   jasjem(q, b31, -b15, 2);
   r = b31-b15;
   ptrsl("ASA2", q, r);

   jasjem(q, bm, b15, 3);
   r = bm+b15;
   ptrsl("ASA3", q, r);

   jasjem(q, -b15, -b15, 4);
   r = -(b15+b15);
   ptrsl("ASA4", q, r);

   printf("The next 2 tests have deliberate overflows.\n");

   jasjem(q, b31, b15, 5);
   ptrsl("ASA5", q, b31);

   jasjem(q, bm, m2, 6);
   r = 0-b31;
   ptrsl("ASA6", q, r);

/* Test jauw */

   printf("\n===jauw===\n");

   ur = jauw(jcuw(0,0), jcuw(0,0));
   uc = jcuw(0,0);
   ptruw("AU1", ur, uc);

   ur = jauw(jcuw(100,200), jcuw(87,89));
   uc = jcuw(187,289);
   ptruw("AU2", ur, uc);

   ur = jauw(jcuw(100,ub31), jcuw(87,um1));
   uc = jcuw(188,ub31m1);
   ptruw("AU3", ur, uc);

   ur = jauw(jcuw(ub31,2), jcuw(ub31,2));
   uc = jcuw(um2,4);
   ptruw("AU4", ur, uc);

   printf("The next test has a deliberate overflow--"
      "not detected by design.\n");
   ur = jauw(jcuw(um1,um2), jcuw(0,2));
   uc = jcuw(0,0);
   ptruw("AU5", ur, uc);

   printf("The next test has a deliberate overflow--"
      "not detected by design.\n");
   ur = jauw(jcuw(um1,0XFFFFFFFAUL), jcuw(87,89));
   uc = jcuw(87,83);
   ptruw("AU6", ur, uc);

   printf("The next test has a deliberate overflow--"
      "not detected by design.\n");
   ur = jauw(jcuw(um2,89), jcuw(um1,ub15));
   uc = jcuw(0xfffffffdul,ub15+89);
   ptruw("AU7", ur, uc);

   printf("The next test has a deliberate overflow--"
      "not detected by design.\n");
   ur = jauw(jcuw(um2,89), jcuw(um1,um1));
   uc = jcuw(um2,88);
   ptruw("AU8", ur, uc);

/* Test jauwe */

   printf("\n===jauwe===\n");

   ur = jauwe(jcuw(0,0), jcuw(0,0), 1);
   uc = jcuw(0,0);
   ptruw("AUE1", ur, uc);

   ur = jauwe(jcuw(100,200), jcuw(87,89), 2);
   uc = jcuw(187,289);
   ptruw("AUE2", ur, uc);

   ur = jauwe(jcuw(100,ub31), jcuw(87,um1) ,3);
   uc = jcuw(188,ub31m1);
   ptruw("AUE3", ur, uc);

   ur = jauwe(jcuw(ub31,2), jcuw(ub31,2), 4);
   uc = jcuw(um2,4);
   ptruw("AUE4", ur, uc);

   printf("The next test has a deliberate overflow.\n");
   ur = jauwe(jcuw(um1,um2), jcuw(0,2), 5);
   uc = jcuw(um1,um1);
   ptruw("AUE5", ur, uc);

   printf("The next test has a deliberate overflow.\n");
   ur = jauwe(jcuw(um1,0XFFFFFFFAUL), jcuw(87,89), 6);
   uc = jcuw(um1,um1);
   ptruw("AUE6", ur, uc);

   printf("The next test has a deliberate overflow.\n");
   ur = jauwe(jcuw(um2,89), jcuw(um1,ub15), 7);
   uc = jcuw(um1,um1);
   ptruw("AUE7", ur, uc);

   printf("The next test has a deliberate overflow.\n");
   ur = jauwe(jcuw(um2,89), jcuw(um1,um1), 8);
   uc = jcuw(um1,um1);
   ptruw("AUE8", ur, uc);

/* Test jauaem */

   printf("\n===jauaem===\n");

   jauaem(uq, ub15, ub15, 1);
   uz = ub15+ub15;
   ptrul("AUA1", uq, uz);

   printf("The next test has a deliberate overflow.\n");

   jauaem(uq, um1, ub15, 2);
   ptrul("AUA2", uq, um1);

/* Test jasl */

   printf("\n===jasl===\n");

   sr = jasl(jcsw(0,0), 0);
   sc = jcsw(0,0);
   ptrsw("AS1", sr, sc);

   sr = jasl(jcsw(100,200), 89);
   sc = jcsw(100,289);
   ptrsw("AS2", sr, sc);

   sr = jasl(jcsw(100,ub31), m1);
   sc = jcsw(100,ub31m1);
   ptrsw("AS3", sr, sc);

   printf("The next test has a deliberate overflow--"
      "not detected by design.\n");
   sr = jasl(jcsw(b31,um2), b31);
   sc = jcsw(bm,0x7FFFFFFDL);
   ptrsw("AS4", sr, sc);

   sr = jasl(jcsw(m1,um2), 2);
   sc = jcsw(0,0);
   ptrsw("AS5", sr, sc);

   sr = jasl(jcsw(m1,0XFFFFFFFAL), 89);
   sc = jcsw(0,83);
   ptrsw("AS6", sr, sc);

   sr = jasl(jcsw(100,um2), b15);
   sc = jcsw(101,ub15-2);
   ptrsw("AS7", sr, sc);

   sr = jasl(jcsw(m2,um2), m1);
   sc = jcsw(m2,um3);
   ptrsw("AS8", sr, sc);

/* Test jasle */

   printf("\n===jasle===\n");

   sr = jasle(jcsw(0,0), 0, 1);
   sc = jcsw(0,0);
   ptrsw("ASE1", sr, sc);

   sr = jasle(jcsw(100,200), 89, 2);
   sc = jcsw(100,289);
   ptrsw("ASE2", sr, sc);

   sr = jasle(jcsw(100,ub31), m1 ,3);
   sc = jcsw(100,ub31m1);
   ptrsw("ASE3", sr, sc);

   printf("The next test has a deliberate overflow.\n");
   sr = jasle(jcsw(b31,um2), b31, 4);
   sc = jcsw(b31,um1);
   ptrsw("ASE4", sr, sc);

   sr = jasle(jcsw(m1,um2), 2, 5);
   sc = jcsw(0,0);
   ptrsw("ASE5", sr, sc);

   sr = jasle(jcsw(m1,0XFFFFFFFAL), 89, 6);
   sc = jcsw(0,83);
   ptrsw("ASE6", sr, sc);

   sr = jasle(jcsw(100,um2), b15, 7);
   sc = jcsw(101,ub15-2);
   ptrsw("ASE7", sr, sc);

   sr = jasle(jcsw(m2,um2), m1, 8);
   sc = jcsw(m2,um3);
   ptrsw("ASE8", sr, sc);

/* Test jaul */

   printf("\n===jaul===\n");

   ur = jaul(jcuw(0,0), 0);
   uc = jcuw(0,0);
   ptruw("AU1", ur, uc);

   ur = jaul(jcuw(100,200), 89);
   uc = jcuw(100,289);
   ptruw("AU2", ur, uc);

   ur = jaul(jcuw(100,ub31), um1);
   uc = jcuw(101,ub31m1);
   ptruw("AU3", ur, uc);

   ur = jaul(jcuw(ub31,2), ub31);
   uc = jcuw(ub31,0X80000001UL);
   ptruw("AU4", ur, uc);

   printf("The next test has a deliberate overflow--"
      "not detected by design.\n");
   ur = jaul(jcuw(um1,um2), 2);
   uc = jcuw(0,0);
   ptruw("AU5", ur, uc);

   printf("The next test has a deliberate overflow--"
      "not detected by design.\n");
   ur = jaul(jcuw(um1,0XFFFFFFFAUL), 89);
   uc = jcuw(0,83);
   ptruw("AU6", ur, uc);

   ur = jaul(jcuw(um2,89), ub15);
   uc = jcuw(um2,ub15+89);
   ptruw("AU7", ur, uc);

/* Test jaule */

   printf("\n===jaule===\n");

   ur = jaule(jcuw(0,0), 0, 1);
   uc = jcuw(0,0);
   ptruw("AUE1", ur, uc);

   ur = jaule(jcuw(100,200), 89, 2);
   uc = jcuw(100,289);
   ptruw("AUE2", ur, uc);

   ur = jaule(jcuw(100,ub31), um1 ,3);
   uc = jcuw(101,ub31m1);
   ptruw("AUE3", ur, uc);

   ur = jaule(jcuw(ub31,2), ub31, 4);
   uc = jcuw(ub31,0X80000001UL);
   ptruw("AUE4", ur, uc);

   printf("The next test has a deliberate overflow.\n");
   ur = jaule(jcuw(um1,um2), 2, 5);
   uc = jcuw(um1,um1);
   ptruw("AUE5", ur, uc);

   printf("The next test has a deliberate overflow.\n");
   ur = jaule(jcuw(um1,0XFFFFFFFAUL), 89, 6);
   uc = jcuw(um1,um1);
   ptruw("AUE6", ur, uc);

   ur = jaule(jcuw(um2,89), ub15, 7);
   uc = jcuw(um2,ub15+89);
   ptruw("AUE7", ur, uc);

/* Test jnsw */

   printf("\n===jnsw===\n");

   sr = jnsw(jcsw(0,0));
   sc = jcsw(0,0);
   ptrsw("NS1", sr, sc);

   sr = jnsw(jcsw(100,200));
   sc = jcsw(-101,-200);
   ptrsw("NS2", sr, sc);

   sr = jnsw(jcsw(m1,um1));
   sc = jcsw(0,1);
   ptrsw("NS3", sr, sc);

   printf("The next test has a deliberate overflow--"
      "not detected by design.\n");
   sr = jnsw(jcsw(bm,0));
   sc = jcsw(bm,0);
   ptrsw("NS4", sr, sc);

/* Test jnswe */

   printf("\n===jnswe===\n");

   sr = jnswe(jcsw(0,0), 1);
   sc = jcsw(0,0);
   ptrsw("NSE1", sr, sc);

   sr = jnswe(jcsw(100,200), 2);
   sc = jcsw(-101,-200);
   ptrsw("NSE2", sr, sc);

   sr = jnswe(jcsw(m1,um1), 3);
   sc = jcsw(0,1);
   ptrsw("NSE3", sr, sc);

   printf("The next test has a deliberate overflow.\n");
   sr = jnswe(jcsw(bm,0), 4);
   sc = jcsw(bm,0);
   ptrsw("NSE4", sr, sc);

/* Test jrsw */

   printf("\n===jrsw===\n");

   sr = jrsw(jcsw(b31,2), jcsw(b31,2));
   sc = jcsw(0,0);
   ptrsw("RS1", sr, sc);

   sr = jrsw(jcsw(100,200), jcsw(87,89));
   sc = jcsw(13,111);
   ptrsw("RS2", sr, sc);

   sr = jrsw(jcsw(100,ub31), jcsw(87,um1));
   sc = jcsw(12,ubm);
   ptrsw("RS3", sr, sc);

   printf("The next test has a deliberate overflow--"
      "not detected by design.\n");
   sr = jrsw(jcsw(b31,2), jcsw(bm,4));
   sc = jcsw(m2,m2);
   ptrsw("RS4", sr, sc);

   sr = jrsw(jcsw(131,133), jcsw(m1,0XFFFFFFFAL));
   sc = jcsw(131,139);
   ptrsw("RS5", sr, sc);

   sr = jrsw(jcsw(m2,89), jcsw(1,ub15));
   sc = jcsw(-4,0XFFFF005AL);
   ptrsw("RS6", sr, sc);

   sr = jrsw(jcsw(m2,89), jcsw(-1,um1));
   sc = jcsw(-2,90);
   ptrsw("RS7", sr, sc);

   sr = jrsw(jcsw(m1,um1), jcsw(b31,um1));
   sc = jcsw(bm,0);
   ptrsw("RS8", sr, sc);

/* Test jrswe */

   printf("\n===jrswe===\n");

   sr = jrswe(jcsw(b31,2), jcsw(b31,2), 1);
   sc = jcsw(0,0);
   ptrsw("RSE1", sr, sc);

   sr = jrswe(jcsw(100,200), jcsw(87,89), 2);
   sc = jcsw(13,111);
   ptrsw("RSE2", sr, sc);

   sr = jrswe(jcsw(100,ub31), jcsw(87,um1) ,3);
   sc = jcsw(12,ubm);
   ptrsw("RSE3", sr, sc);

   printf("The next test has a deliberate overflow.\n");
   sr = jrswe(jcsw(b31,2), jcsw(bm,4), 4);
   sc = jcsw(b31,um1);
   ptrsw("RSE4", sr, sc);

   sr = jrswe(jcsw(131,133), jcsw(m1,0XFFFFFFFAL), 5);
   sc = jcsw(131,139);
   ptrsw("RSE5", sr, sc);

   sr = jrswe(jcsw(m2,89), jcsw(1,ub15), 6);
   sc = jcsw(-4,0XFFFF005AL);
   ptrsw("RSE6", sr, sc);

   sr = jrswe(jcsw(m2,89), jcsw(-1,um1), 7);
   sc = jcsw(-2,90);
   ptrsw("RSE7", sr, sc);

   sr = jrswe(jcsw(m1,um1), jcsw(b31,um1), 8);
   sc = jcsw(bm,0);
   ptrsw("RSE8", sr, sc);

/* Test jrsjem */
/* Note:  jrsiem, jrslem have same code except type in
*  overflow replacement, so these are not tested separately.  */

   printf("\n===jrsjem===\n");

   jrsjem(q, b15, b15, 1);
   r = 0;
   ptrsl("RSA1", q, r);

   jrsjem(q, b15, m2, 2);
   r = b15-m2;
   ptrsl("RSA2", q, r);

   jrsjem(q, m2, b15, 3);
   r = m2-b15;
   ptrsl("RSA3", q, r);

   jrsjem(q, bm, m2, 4);
   r = bm-m2;
   ptrsl("RSA4", q, r);

   printf("The next 2 tests have deliberate overflows.\n");

   jrsjem(q, b31, m2, 5);
   ptrsl("RSA5", q, b31);

   jrsjem(q, bm, b15, 6);
   r = 0-b31;
   ptrsl("RSA6", q, r);

/* Test jruw */

   printf("\n===jruw===\n");

   ur = jruw(jcuw(ub31,2), jcuw(ub31,2));
   uc = jcuw(0,0);
   ptruw("RU1", ur, uc);

   ur = jruw(jcuw(100,200), jcuw(87,89));
   uc = jcuw(13,111);
   ptruw("RU2", ur, uc);

   ur = jruw(jcuw(100,ub31), jcuw(87,um1));
   uc = jcuw(12,ubm);
   ptruw("RU3", ur, uc);

   printf("The next test has a deliberate underflow--"
      "not detected by design.\n");
   ur = jruw(jcuw(ub31,2), jcuw(ubm,4));
   uc = jcuw(um2,um2);
   ptruw("RU4", ur, uc);

   printf("The next test has a deliberate underflow--"
      "not detected by design.\n");
   ur = jruw(jcuw(131,133), jcuw(um1,0XFFFFFFFAUL));
   uc = jcuw(131,139);
   ptruw("RU5", ur, uc);

   ur = jruw(jcuw(um2,89), jcuw(1,ub15));
   uc = jcuw(0XFFFFFFFCUL,0XFFFF005AUL);
   ptruw("RU6", ur, uc);

   printf("The next test has a deliberate overflow--"
      "not detected by design.\n");
   ur = jruw(jcuw(um2,89), jcuw(um1,um1));
   uc = jcuw(um2,90);
   ptruw("RU7", ur, uc);

   ur = jruw(jcuw(um1,um1), jcuw(ub31,um1));
   uc = jcuw(ubm,0);
   ptruw("RU8", ur, uc);

/* Test jruwe */

   printf("\n===jruwe===\n");

   ur = jruwe(jcuw(ub31,2), jcuw(ub31,2), 1);
   uc = jcuw(0,0);
   ptruw("RUE1", ur, uc);

   ur = jruwe(jcuw(100,200), jcuw(87,89), 2);
   uc = jcuw(13,111);
   ptruw("RUE2", ur, uc);

   ur = jruwe(jcuw(100,ub31), jcuw(87,um1) ,3);
   uc = jcuw(12,ubm);
   ptruw("RUE3", ur, uc);

   printf("The next test has a deliberate underflow.\n");
   ur = jruwe(jcuw(ub31,2), jcuw(ubm,4), 4);
   uc = jcuw(0,0);
   ptruw("RUE4", ur, uc);

   printf("The next test has a deliberate underflow.\n");
   ur = jruwe(jcuw(131,133), jcuw(um1,0XFFFFFFFAUL), 5);
   uc = jcuw(0,0);
   ptruw("RUE5", ur, uc);

   ur = jruwe(jcuw(um2,89), jcuw(1,ub15), 6);
   uc = jcuw(-4,0XFFFF005AUL);
   ptruw("RUE6", ur, uc);

   printf("The next test has a deliberate overflow.\n");
   ur = jruwe(jcuw(um2,89), jcuw(um1,um1), 7);
   uc = jcuw(0,0);
   ptruw("RUE7", ur, uc);

   ur = jruwe(jcuw(um1,um1), jcuw(ub31,um1), 8);
   uc = jcuw(ubm,0);
   ptruw("RUE8", ur, uc);

/* Test jruaem */

   printf("\n===jruaem===\n");

   jruaem(uq, ub31, ub15, 1);
   uz = ub31-ub15;
   ptrul("RUA1", uq, uz);

   jruaem(uq, ub15, ub15, 2);
   uz = 0;
   ptrul("RUA2", uq, uz);

   printf("The next test has a deliberate overflow.\n");

   jruaem(uq, ub15, ub15+1, 3);
   ptrul("RUA3", uq, 0);

/* Test jrsl */

   printf("\n===jrsl===\n");

   sr = jrsl(jcsw(0,0), 0);
   sc = jcsw(0,0);
   ptrsw("RLS1", sr, sc);

   sr = jrsl(jcsw(100,200), 89);
   sc = jcsw(100,111);
   ptrsw("RLS2", sr, sc);

   sr = jrsl(jcsw(100,ub31), m1);
   sc = jcsw(100,ubm);
   ptrsw("RLS3", sr, sc);

   printf("The next test has a deliberate overflow--"
      "not detected by design.\n");
   sr = jrsl(jcsw(b31,um2), m2);
   sc = jcsw(bm,0);
   ptrsw("RLS4", sr, sc);

   sr = jrsl(jcsw(m1,um2), m2);
   sc = jcsw(0,0);
   ptrsw("RLS5", sr, sc);

   sr = jrsl(jcsw(m1,0XFFFFFFFAU), -89);
   sc = jcsw(0,83);
   ptrsw("RLS6", sr, sc);

   sr = jrsl(jcsw(100,um2), -b15);
   sc = jcsw(101,ub15-2);
   ptrsw("RLS7", sr, sc);

   sr = jrsl(jcsw(m2,um2), 1);
   sc = jcsw(m2,um3);
   ptrsw("RLS8", sr, sc);

/* Test jrsle */

   printf("\n===jrsle===\n");

   sr = jrsle(jcsw(0,0), 0, 1);
   sc = jcsw(0,0);
   ptrsw("RLSE1", sr, sc);

   sr = jrsle(jcsw(100,200), 89, 2);
   sc = jcsw(100,111);
   ptrsw("RLSE2", sr, sc);

   sr = jrsle(jcsw(100,ub31), m1 ,3);
   sc = jcsw(100,ubm);
   ptrsw("RLSE3", sr, sc);

   printf("The next test has a deliberate overflow.\n");
   sr = jrsle(jcsw(b31,um2), um2, 4);
   sc = jcsw(b31,um1);
   ptrsw("RLSE4", sr, sc);

   sr = jrsle(jcsw(m1,um2), m2, 5);
   sc = jcsw(0,0);
   ptrsw("RLSE5", sr, sc);

   sr = jrsle(jcsw(m1,0XFFFFFFFAU), -89, 6);
   sc = jcsw(0,83);
   ptrsw("RLSE6", sr, sc);

   sr = jrsle(jcsw(100,um2), -b15, 7);
   sc = jcsw(101,ub15-2);
   ptrsw("RLSE7", sr, sc);

   sr = jrsle(jcsw(m2,um2), 1, 8);
   sc = jcsw(m2,um3);
   ptrsw("RLSE8", sr, sc);

/* Test jrul */

   printf("\n===jrul===\n");

   ur = jrul(jcuw(0,0), 0);
   uc = jcuw(0,0);
   ptruw("RLU1", ur, uc);

   ur = jrul(jcuw(100,200), 89);
   uc = jcuw(100,111);
   ptruw("RLU2", ur, uc);

   ur = jrul(jcuw(100,ub31), um1);
   uc = jcuw(99,ubm);
   ptruw("RLU3", ur, uc);

   ur = jrul(jcuw(ub31,2), ub31);
   uc = jcuw(ub31m1,0X80000003U);
   ptruw("RLU4", ur, uc);

   printf("The next test has a deliberate underflow--"
      "not detected by design.\n");
   ur = jrul(jcuw(0,1), 2);
   uc = jcuw(um1,um1);
   ptruw("RLU5", ur, uc);

   printf("The next test has a deliberate underflow--"
      "not detected by design.\n");
   ur = jrul(jcuw(0,0XFFFFFFFAU), um1);
   uc = jcuw(um1,0xFFFFFFFBU);
   ptruw("RLU6", ur, uc);

   ur = jrul(jcuw(um2,89), ub15);
   uc = jcuw(um3,0xFFFF005AU);
   ptruw("RLU7", ur, uc);

/* Test jrule */

   printf("\n===jrule===\n");

   ur = jrule(jcuw(0,0), 0, 1);
   uc = jcuw(0,0);
   ptruw("RLUE1", ur, uc);

   ur = jrule(jcuw(100,200), 89, 2);
   uc = jcuw(100,111);
   ptruw("RLUE2", ur, uc);

   ur = jrule(jcuw(100,ub31), um1 ,3);
   uc = jcuw(99,ubm);
   ptruw("RLUE3", ur, uc);

   ur = jrule(jcuw(ub31,2), ub31, 4);
   uc = jcuw(ub31m1,0X80000003U);
   ptruw("RLUE4", ur, uc);

   printf("The next test has a deliberate overflow.\n");
   ur = jrule(jcuw(0,1), 2, 5);
   uc = jcuw(0,0);
   ptruw("RLUE5", ur, uc);

   printf("The next test has a deliberate overflow.\n");
   ur = jrule(jcuw(0,0XFFFFFFFAU), um1, 6);
   uc = jcuw(0,0);
   ptruw("RLUE6", ur, uc);

   ur = jrule(jcuw(um2,89), ub15, 7);
   uc = jcuw(um3,0xFFFF005AU);
   ptruw("RLUE7", ur, uc);

/* Test jdswb */

   printf("\n===jdswb===\n");

   q = jdswb(jcsw(7,0), 65536, &r);
   ptrsl("DS1Q", q, 458752);
   ptrsl("DS1R", r, 0);

   q = jdswb(jcsw(7,111), b15, &r);
   ptrsl("DS2Q", q, 458759);
   ptrsl("DS2R", r, 118);

   printf("Test DS3 has a deliberate overflow.\n");
   printf("Returned values are irrelevant.\n");
   jdswb(jcsw(32767,ubm), b15, &r);

   q = jdswb(jcsw(-7,0), b15, &r);
   ptrsl("DS4Q", q, -458759);
   ptrsl("DS4R", r, -7);

   q = jdswb(jcsw(7,0), -b15, &r);
   ptrsl("DS5Q", q, -458759);
   ptrsl("DS5R", r, 7);

   q = jdswb(jcsw(-7,0), -b15, &r);
   ptrsl("DS6Q", q, 458759);
   ptrsl("DS6R", r, -7);

   q = jdswb(jcsw(0X3FFFFFFFL,0), b31, &r);
   ptrsl("DS7Q", q, b31m1);
   ptrsl("DS7R", r, b31m1);

/* Test jdswbe */

   printf("\n===jdswbe===\n");

   q = jdswbe(jcsw(7,0), 65536, 1, &r);
   ptrsl("ES1Q", q, 458752);
   ptrsl("ES1R", r, 0);

   q = jdswbe(jcsw(7,111), b15, 2, &r);
   ptrsl("ES2Q", q, 458759);
   ptrsl("ES2R", r, 118);

   printf("Test ES3 has a deliberate overflow.\n");
   q = jdswbe(jcsw(32767,ubm), b15, 3, &r);
   ptrsl("ES3Q", q, SI32_MAX);
   ptrsl("ES3R", r, 0);

   q = jdswbe(jcsw(-7,0), b15, 4, &r);
   ptrsl("ES4Q", q, -458759);
   ptrsl("ES4R", r, -7);

   q = jdswbe(jcsw(7,0), -b15, 5, &r);
   ptrsl("ES5Q", q, -458759);
   ptrsl("ES5R", r, 7);

   q = jdswbe(jcsw(-7,0), -b15, 6, &r);
   ptrsl("ES6Q", q, 458759);
   ptrsl("ES6R", r, -7);

   q = jdswbe(jcsw(0X3FFFFFFFL,0), b31, 7, &r);
   ptrsl("ES7Q", q, b31m1);
   ptrsl("ES7R", r, b31m1);

/* Test jdswq */

   printf("\n===jdswq===\n");

   q = jdswq(jcsw(7,0), 65536);
   ptrsl("DS1Q", q, 458752);

   q = jdswq(jcsw(7,111), b15);
   ptrsl("DS2Q", q, 458759);

   printf("Test DS3 has a deliberate overflow.\n");
   printf("Returned values are irrelevant.\n");
   q = jdswq(jcsw(32767,ubm), b15);

   q = jdswq(jcsw(-7,0), b15);
   ptrsl("DS4Q", q, -458759);

   q = jdswq(jcsw(7,0), -b15);
   ptrsl("DS5Q", q, -458759);

   q = jdswq(jcsw(-7,0), -b15);
   ptrsl("DS6Q", q, 458759);

   q = jdswq(jcsw(0X3FFFFFFFL,0), b31);
   ptrsl("DS7Q", q, b31m1);

/* Test jdswqe */

   printf("\n===jdswqe===\n");

   q = jdswqe(jcsw(7,0), 65536, 1);
   ptrsl("ES1Q", q, 458752);

   q = jdswqe(jcsw(7,111), b15, 2);
   ptrsl("ES2Q", q, 458759);

   printf("Test ES3 has a deliberate overflow.\n");
   q = jdswqe(jcsw(32767,ubm), b15, 3);
   ptrsl("ES2Q", q, SI32_MAX);

   q = jdswqe(jcsw(-7,0), b15, 4);
   ptrsl("ES4Q", q, -458759);

   q = jdswqe(jcsw(7,0), -b15, 5);
   ptrsl("ES5Q", q, -458759);

   q = jdswqe(jcsw(-7,0), -b15, 6);
   ptrsl("ES6Q", q, 458759);

   q = jdswqe(jcsw(0X3FFFFFFFL,0), b31, 7);
   ptrsl("ES7Q", q, b31m1);

/* Test jduwb */

   printf("\n===jduwb===\n");

   ur = jduwb(jcuw(7,0), 65536, &uz);
   uc = jcuw(0,458752);
   ptruw("DU1Q", ur, uc);
   ptrul("DU1R", uz, 0);

   ur = jduwb(jcuw(7,111), ub15, &uz);
   uc = jcuw(0,458759);
   ptruw("DU2Q", ur, uc);
   ptrul("DU2R", uz, 118);

   printf("Test DU3 has a deliberate divide check.\n");
   printf("Returned values are irrelevant.\n");
   jduwb(jcuw(65535,0), 0, &uz);

   ur = jduwb(jcuw(0,0), 111, &uz);
   uc = jcuw(0,0);
   ptruw("DU4Q", ur, uc);
   ptrul("DU4R", uz, 0);

   ur = jduwb(jcuw(um2,um1), um1, &uz);
   uc = jcuw(0,um1);
   ptruw("DU5Q", ur, uc);
   ptrul("DU5R", uz, um2);

   /* Tests that return quotients > 2^32 w/full, halfword divisors */
   ur = jduwb(jcuw(um2,um1), 0x35ffff, &uz);
   uc = jcuw(0x4bd,0xa145dd0f);
   ptruw("DU6Q", ur, uc);
   ptrul("DU6R", uz, 0x1bdd0e);

   ur = jduwb(jcuw(ub15,u6u), 10, &uz);
   uc = jcuw(0x1999,0x998a3d7f);
   ptruw("DU7Q", ur, uc);
   ptrul("DU7R", uz, 9);

/* Test jduwq */

   printf("\n===jduwq===\n");

   uq = jduwq(jcuw(7,0), 65536);
   ptrul("DU1Q", uq, 458752);

   uq = jduwq(jcuw(7,111), ub15);
   ptrul("DU2Q", uq, 458759);

   printf("Test DU3 has a deliberate overflow.\n");
   printf("Returned values are irrelevant.\n");
   uq = jduwq(jcuw(65535,0), ub15);

   uq = jduwq(jcuw(0,0), ub15);
   ptrul("DU4Q", uq, 0);

   uq = jduwq(jcuw(um2,um1), um1);
   ptrul("DU5Q", uq, um1);

/* Test jduwqe */

   printf("\n===jduwqe===\n");

   uq = jduwqe(jcuw(7,0), 65536, 1);
   ptrul("DU1QE", uq, 458752);

   uq = jduwqe(jcuw(7,111), ub15, 2);
   ptrul("DU2QE", uq, 458759);

   printf("Test DU3QE has a deliberate overflow.\n");
   uq = jduwqe(jcuw(65535,0), ub15, 3);
   ptrul("DU3QE", uq, UI32_MAX);

   uq = jduwqe(jcuw(0,0), ub15, 4);
   ptrul("DU4QE", uq, 0);

   uq = jduwqe(jcuw(um2,um1), um1, 5);
   ptrul("DU5QE", uq, um1);

/* Test jmsw */

   printf("\n===jmsw===\n");

   sr = jmsw(-5,89);
   sc = jcsw(m1,-5*89);
   ptrsw("MS1", sr, sc);

   sr = jmsw(b15,b15);
   sc = jcsw(0,ub15*ub15);
   ptrsw("MS2", sr, sc);

   sr = jmsw(b15, 65536);
   sc = jcsw(0,0xFFFF0000UL);
   ptrsw("MS3", sr, sc);

   sr = jmsw(65536, 65536);
   sc = jcsw(1,0);
   ptrsw("MS4", sr, sc);

   sr = jmsw(5*65536, 7*65536);
   sc = jcsw(35,0);
   ptrsw("MS5", sr, sc);

   sr = jmsw(5*65536, -7*65536);
   sc = jcsw(-35,0);
   ptrsw("MS6", sr, sc);

   sr = jmsw(-5*65536, -7*65536);
   sc = jcsw(35,0);
   ptrsw("MS7", sr, sc);

   sr = jmsw(b31,b31);
   sc = jcsw(0X3FFFFFFF,1);
   ptrsw("MS8", sr, sc);

/* Test jmuw */

   printf("\n===jmuw===\n");

   ur = jmuw(0XFFFFFFFBUL,89);
   uc = jcuw(88,0XFFFFFE43UL);
   ptruw("MU1", ur, uc);

   ur = jmuw(ub15,ub15);
   uc = jcuw(0,ub15*ub15);
   ptruw("MU2", ur, uc);

   ur = jmuw(ub15, 65536);
   uc = jcuw(0,0xFFFF0000UL);
   ptruw("MU3", ur, uc);

   ur = jmuw(65536, 65536);
   uc = jcuw(1,0);
   ptruw("MU4", ur, uc);

   ur = jmuw(5*65536, 7*65536);
   uc = jcuw(35,0);
   ptruw("MU5", ur, uc);

   ur = jmuw(5*65536, 0XFFF90000UL);
   uc = jcuw(0X0004FFDDUL,0);
   ptruw("MU6", ur, uc);

   ur = jmuw(0XFFFB0000UL, 0XFFF90000UL);
   uc = jcuw(4294180899UL,0);
   ptruw("MU7", ur, uc);

   ur = jmuw(um1,um1);
   uc = jcuw(um2,1);
   ptruw("MU8", ur, uc);

/* Test jmuwb */

   printf("\n===jmuwb===\n");

   ur = jmuwb(jcuw(3,5),47,&uz);
   uc = jcuw(141,235);
   ptruw("JMUW1", ur, uc);
   ptrsl("JMUH1", uz, 0);

   ur = jmuwb(jcuw(ub15,u6u),ub15,&uz);
   uc = jcuw(0xFFFEFF66UL,0x67989901UL);
   ptruw("JMUW2", ur, uc);
   ptrsl("JMUH2", uz, 0);

   ur = jmuwb(jcuw(0xFFFF0000L,0x00999901UL),u6u,&uz);
   uc = jcuw(0x999A3CD8UL,0xC15BCDFFUL);
   ptruw("JMUW3", ur, uc);
   ptrsl("JMUH3", uz, 0xFF656798);

   ur = jmuwb(jcuw(0x7FFF0000L,0x00999901UL),0x10000,&uz);
   uc = jcuw(0x99,0x99010000);
   ptruw("JMUW4", ur, uc);
   ptrsl("JMUH4", uz, 0x7FFF  );

   ur = jmuwb(jcuw(bm,0),ub15,&uz);
   uc = jcuw(bm,0);
   ptruw("JMUW5", ur, uc);
   ptrsl("JMUH5", uz, 0x7FFF);

   ur = jmuwb(jcuw(0x7FFF0000UL,0x00999901UL),ub15,&uz);
   uc = jcuw(0x80010099UL,0x986766FFUL);
   ptruw("JMUW6", ur, uc);
   ptrsl("JMUH6", uz, 0x7FFE);

   ur = jmuwb(jcuw(ub31,um1),ub31,&uz);
   uc = jcuw(0x7FFFFFFFUL,0x80000001);
   ptruw("JMUW7", ur, uc);
   ptrsl("JMUH7", uz, 0x3FFFFFFF);

   ur = jmuwb(jcuw(bm,0),bm,&uz);
   uc = jcuw(0,0);
   ptruw("JMUW8", ur, uc);
   ptrsl("JMUH8", uz, 0x40000000);

   ur = jmuwb(jcuw(bm,0),1,&uz);
   uc = jcuw(bm,0);
   ptruw("JMUW9", ur, uc);
   ptrsl("JMUH9", uz, 0);

/* Test jmuwje */

   printf("\n===jmuwje===\n");

   ur = jmuwje(jcuw(0,0XFFFFFFFBU), 89, 1);
   uc = jcuw(88,0XFFFFFE43U);
   ptruw("MWJ1", ur, uc);

   ur = jmuwje(jcuw(0,ub15), 65536, 2);
   uc = jcuw(0,0xFFFF0000U);
   ptruw("MWJ2", ur, uc);

   ur = jmuwje(jcuw(0,5*65536), 7*65536, 3);
   uc = jcuw(35,0);
   ptruw("MWJ3", ur, uc);

   ur = jmuwje(jcuw(66,908068875), 56932, 4);
   uc = jcuw(3769548,3950816844U);
   ptruw("MWJ4", ur, uc);

   ur = jmuwje(jcuw(0,0XFFFB0000U), 0XFFFFFFFFU, 5);
   uc = jcuw(0XFFFAFFFFU,0X00050000U);
   ptruw("MWJ5", ur, uc);

   /* Deliberate overflow error -- smallest possible */
   printf("Test MWJ6 has a deliberate overflow.\n");
   ur = jmuwje(jcuw(2,0), 0X80000000U, 6);
   uc = jcuw(um1,um1);
   ptruw("MWJ6", ur, uc);

   printf("Test MWJ7 has a deliberate overflow.\n");
   ur = jmuwje(jcuw(31,2645496195U), 1454280888U, 7);
   uc = UI64_MAX;
   ptruw("MWJ7", ur, uc);

/* Test jmuwwe */

   printf("\n===jmuwwe===\n");

   ur = jmuwwe(jcuw(0,0XFFFFFFFBU), jcuw(0,89), 1);
   uc = jcuw(88,0XFFFFFE43U);
   ptruw("MW1", ur, uc);

   ur = jmuwwe(jcuw(0,ub15), jcuw(0,65536), 2);
   uc = jcuw(0,0xFFFF0000U);
   ptruw("MW2", ur, uc);

   ur = jmuwwe(jcuw(0,5*65536), jcuw(0,7*65536), 3);
   uc = jcuw(35,0);
   ptruw("MW3", ur, uc);

   ur = jmuwwe(jcuw(66,908068875), jcuw(0,56932), 4);
   uc = jcuw(3769548,3950816844U);
   ptruw("MW4", ur, uc);

   ur = jmuwwe(jcuw(0,0XFFFB0000U), jcuw(1,0), 5);
   uc = jcuw(0XFFFB0000U,0);
   ptruw("MW5", ur, uc);

   /* Deliberate overflow error -- smallest possible */
   printf("Test MW6 has a deliberate overflow.\n");
   ur = jmuwwe(jcuw(1,0),jcuw(1,0),6);
   uc = jcuw(um1,um1);
   ptruw("MW6", ur, uc);

   printf("Test MW7 has a deliberate overflow.\n");
   ur = jmuwwe(jcuw(31,2645496195U),jcuw(2097,1454280888U), 7);
   uc = UI64_MAX;
   ptruw("MW7", ur, uc);

/* Test jssw */

   printf("\n===jssw===\n");

   sr = jssw(jcsw(0X123L,ub31), -4);
   sc = jcsw(0X12L,0X37FFFFFFL);
   ptrsw("SS1", sr, sc);

   sr = jssw(jcsw(0X123L,ub31), -34);
   sc = jcsw(0,0X48L);
   ptrsw("SS2", sr, sc);

   sr = jssw(jcsw(m2,ub31), -4);
   sc = jcsw(m1,0XE7FFFFFFL);
   ptrsw("SS3", sr, sc);

   sr = jssw(jcsw(0XFFFFFEDCL,0X456789abL), -4);
   sc = jcsw(0XFFFFFFED,0XC456789AL);
   ptrsw("SS4", sr, sc);

   sr = jssw(jcsw(3,ub31), 4);
   sc = jcsw(0x37,0XFFFFFFF0L);
   ptrsw("SS5", sr, sc);

   sr = jssw(jcsw(0,0X00123456L), 36);
   sc = jcsw(0X01234560L,0);
   ptrsw("SS6", sr, sc);

   printf("The next test has a deliberate overflow--"
      "not detected by design.\n");
   sr = jssw(jcsw(0X01234560,ub31), 8);
   sc = jcsw(0X2345607FL,0XFFFFFF00L);
   ptrsw("SS7", sr, sc);

   printf("The next test has a deliberate overflow--"
      "not detected by design.\n");
   sr = jssw(jcsw(3,ub31), 36);
   sc = jcsw(0XFFFFFFF0L,0L);
   ptrsw("SS8", sr, sc);

   sr = jssw(jcsw(~3,ub31), 4);
   sc = jcsw(0XFFFFFFC7,0XFFFFFFF0L);
   ptrsw("SS9", sr, sc);

   sr = jssw(jcsw(m1,0XFFEDCBA9L), 36);
   sc = jcsw(0XFEDCBA90L,0L);
   ptrsw("SS10", sr, sc);

   printf("The next test has a deliberate overflow--"
      "not detected by design.\n");
   sr = jssw(jcsw(0XFEDCBA9FL,ub31), 8);
   sc = jcsw(0XDCBA9F7F,0XFFFFFF00L);
   ptrsw("SS11", sr, sc);

   printf("The next test has a deliberate overflow--"
      "not detected by design.\n");
   sr = jssw(jcsw(~3,ub31), 36);
   sc = jcsw(0XFFFFFFF0L,0);
   ptrsw("SS12", sr, sc);

/* Test jsswe */

   printf("\n===jsswe===\n");

   sr = jsswe(jcsw(0X123L,ub31), -4, 1);
   sc = jcsw(0X12L,0X37FFFFFFL);
   ptrsw("SSE1", sr, sc);

   sr = jsswe(jcsw(0X123L,ub31), -34, 2);
   sc = jcsw(0,0X48L);
   ptrsw("SSE2", sr, sc);

   sr = jsswe(jcsw(m2,ub31), -4, 3);
   sc = jcsw(m1,0XE7FFFFFFL);
   ptrsw("SSE3", sr, sc);

   sr = jsswe(jcsw(0XFFFFFEDCL,0X456789abL), -4, 4);
   sc = jcsw(0XFFFFFFED,0XC456789AL);
   ptrsw("SSE4", sr, sc);

   sr = jsswe(jcsw(3,ub31), 4, 5);
   sc = jcsw(0x37,0XFFFFFFF0L);
   ptrsw("SSE5", sr, sc);

   sr = jsswe(jcsw(0,0X00123456L), 36, 6);
   sc = jcsw(0X01234560L,0);
   ptrsw("SSE6", sr, sc);

   printf("The next test has a deliberate overflow.\n");
   sr = jsswe(jcsw(0X01234560,ub31), 8, 7);
   sc = jcsw(b31,um1);
   ptrsw("SSE7", sr, sc);

   printf("The next test has a deliberate overflow.\n");
   sr = jsswe(jcsw(3,ub31), 36, 8);
   sc = jcsw(b31,um1);
   ptrsw("SSE8", sr, sc);

   sr = jsswe(jcsw(~3,ub31), 4, 9);
   sc = jcsw(0XFFFFFFC7,0XFFFFFFF0L);
   ptrsw("SSE9", sr, sc);

   sr = jsswe(jcsw(m1,0XFFEDCBA9L), 36, 10);
   sc = jcsw(0XFEDCBA90L,0L);
   ptrsw("SSE10", sr, sc);

   printf("The next test has a deliberate overflow.\n");
   sr = jsswe(jcsw(0XFEDCBA9FL,ub31), 8, 11);
   sc = jcsw(bm,0);
   ptrsw("SSE11", sr, sc);

   printf("The next test has a deliberate overflow.\n");
   sr = jsswe(jcsw(~3,ub31), 36, 12);
   sc = jcsw(bm,0);
   ptrsw("SSE12", sr, sc);

/* Test jslswe -- Similar to jsswe except only left shifts */

   printf("\n===jslswe===\n");

   sr = jslswe(jcsw(3,ub31), 4, 1);
   sc = jcsw(0x37,0XFFFFFFF0L);
   ptrsw("JSLSWE1", sr, sc);

   sr = jslswe(jcsw(0,0X00123456L), 36, 2);
   sc = jcsw(0X01234560L,0);
   ptrsw("JSLSWE2", sr, sc);

   printf("The next test has a deliberate overflow.\n");
   sr = jslswe(jcsw(0X01234560,ub31), 7, 3);
   sc = jcsw(b31,um1);
   ptrsw("JSLSWE3", sr, sc);

   printf("The next test has a deliberate overflow.\n");
   sr = jslswe(jcsw(3,ub31), 36, 4);
   sc = jcsw(b31,um1);
   ptrsw("JSLSWE4", sr, sc);

   /* Test overflow or lack thereof with negative args */
   sr = jslswe(jcsw(~3,ub31), 4, 5);
   sc = jcsw(0XFFFFFFC7,0XFFFFFFF0L);
   ptrsw("JSLSWE5", sr, sc);

   sr = jslswe(jcsw(m1,0XFFEDCBA9L), 36, 6);
   sc = jcsw(0XFEDCBA90L,0L);
   ptrsw("JSLSWE6", sr, sc);

   printf("The next test has a deliberate overflow.\n");
   sr = jslswe(jcsw(0XFEDCBA9FL,ub31), 8, 7);
   sc = jcsw(bm,0);
   ptrsw("JSLSWE7", sr, sc);

   /* Test a shift of exactly 32 */
   sr = jslswe(jcsw(m1,u6u), 32, 8);
   sc = jcsw(u6u,0);
   ptrsw("JSLSWE8", sr, sc);

/* Test jsrsw -- Similar to jssw except only right shifts */

   printf("\n===jsrsw===\n");

   sr = jsrsw(jcsw(0X123L,ub31), 4);
   sc = jcsw(0X12L,0X37FFFFFFL);
   ptrsw("JSRSW1", sr, sc);

   sr = jsrsw(jcsw(0X123L,ub31), 34);
   sc = jcsw(0,0X48L);
   ptrsw("JSRSW2", sr, sc);

   sr = jsrsw(jcsw(m2,ub31), 4);
   sc = jcsw(m1,0XE7FFFFFFL);
   ptrsw("JSRSW3", sr, sc);

   sr = jsrsw(jcsw(0XFFFFFEDCL,0X456789abL), 4);
   sc = jcsw(0XFFFFFFED,0XC456789AL);
   ptrsw("JSRSW4", sr, sc);

   sr = jsrsw(jcsw(bm,0), 4);
   sc = jcsw(0XF8000000,0);
   ptrsw("JSRSW5", sr, sc);

/* Test jsrrsw */

   printf("\n===jsrrsw===\n");

   sr = jsrrsw(jcsw(0X123L,ub31), 4);
   sc = jcsw(0X12L,0X38000000L);
   ptrsw("JSRRSW1", sr, sc);

   sr = jsrrsw(jcsw(0X123L,ub31), 32);
   sc = jcsw(0,0X123L);
   ptrsw("JSRRSW2", sr, sc);

   sr = jsrrsw(jcsw(0X123L,ub31), 34);
   sc = jcsw(0,0X49L);
   ptrsw("JSRRSW3", sr, sc);

   sr = jsrrsw(jcsw(b31,um1), 1);
   sc = jcsw(0X40000000,0);
   ptrsw("JSRRSW4", sr, sc);

   sr = jsrrsw(jcsw(b31,um1), 2);
   sc = jcsw(0X20000000,0);
   ptrsw("JSRRSW5", sr, sc);

   sr = jsrrsw(jcsw(m2,ub31), 4);
   sc = jcsw(m1,0XE8000000L);
   ptrsw("JSRRSW6", sr, sc);

   sr = jsrrsw(jcsw(0XFFFFFEDCL,0X456789ABL), 4);
   sc = jcsw(0XFFFFFFED,0XC456789BL);
   ptrsw("JSRRSW7", sr, sc);

   sr = jsrrsw(jcsw(bm,0), 4);
   sc = jcsw(0XF8000000,0);
   ptrsw("JSRRSW8", sr, sc);

   sr = jsrrsw(jcsw(m1,um1), 1);
   sc = jcsw(0,0);
   ptrsw("JSRRSW9", sr, sc);

   sr = jsrrsw(jcsw(m1,um1), 7);
   sc = jcsw(0,0);
   ptrsw("JSRRSW10", sr, sc);

/* Test jsuw */

   printf("\n===jsuw===\n");

   ur = jsuw(jcuw(0X123UL,ub31), -4);
   uc = jcuw(0X12UL,0X37FFFFFFUL);
   ptruw("SU1", ur, uc);

   ur = jsuw(jcuw(0X123UL,ub31), -34);
   uc = jcuw(0,0X48UL);
   ptruw("SU2", ur, uc);

   ur = jsuw(jcuw(um2,ub31), -4);
   uc = jcuw(0X0FFFFFFFUL,0XE7FFFFFFUL);
   ptruw("SU3", ur, uc);

   ur = jsuw(jcuw(0XFFFFFEDCUL,0X456789ABUL), -4);
   uc = jcuw(0X0FFFFFEDUL,0XC456789AUL);
   ptruw("SU4", ur, uc);

   ur = jsuw(jcuw(3,ub31), 4);
   uc = jcuw(0x37,0XFFFFFFF0UL);
   ptruw("SU5", ur, uc);

   ur = jsuw(jcuw(0,0X00123456UL), 36);
   uc = jcuw(0X01234560UL,0);
   ptruw("SU6", ur, uc);

   printf("The next test has a deliberate overflow--"
      "not detected by design.\n");
   ur = jsuw(jcuw(0X01234560,ub31), 8);
   uc = jcuw(0X2345607FUL,0XFFFFFF00UL);
   ptruw("SU7", ur, uc);

   printf("The next test has a deliberate overflow--"
      "not detected by design.\n");
   ur = jsuw(jcuw(3,ub31), 36);
   uc = jcuw(0XFFFFFFF0UL,0UL);
   ptruw("SU8", ur, uc);

   printf("The next test has a deliberate overflow--"
      "not detected by design.\n");
   ur = jsuw(jcuw(0XFEDCBA9FUL,ub31), 8);
   uc = jcuw(0XDCBA9F7FUL,0XFFFFFF00UL);
   ptruw("SU9", ur, uc);

   printf("The next test has a deliberate overflow--"
      "not detected by design.\n");
   ur = jsuw(jcuw(~3ul,ub31), 36);
   uc = jcuw(0XFFFFFFF0UL,0);
   ptruw("SU10", ur, uc);

/* Test jsuwe */

   printf("\n===jsuwe===\n");

   ur = jsuwe(jcuw(0X123UL,ub31), -4, 1);
   uc = jcuw(0X12UL,0X37FFFFFFUL);
   ptruw("SUE1", ur, uc);

   ur = jsuwe(jcuw(0X123UL,ub31), -34, 2);
   uc = jcuw(0,0X48UL);
   ptruw("SUE2", ur, uc);

   ur = jsuwe(jcuw(um2,ub31), -4, 3);
   uc = jcuw(0X0FFFFFFFUL,0XE7FFFFFFUL);
   ptruw("SUE3", ur, uc);

   ur = jsuwe(jcuw(0XFFFFFEDCUL,0X456789abUL), -4, 4);
   uc = jcuw(0X0FFFFFEDUL,0XC456789AUL);
   ptruw("SUE4", ur, uc);

   ur = jsuwe(jcuw(3,ub31), 4, 5);
   uc = jcuw(0x37,0XFFFFFFF0UL);
   ptruw("SUE5", ur, uc);

   ur = jsuwe(jcuw(0,0X00123456UL), 36, 6);
   uc = jcuw(0X01234560UL,0);
   ptruw("SUE6", ur, uc);

   printf("The next test has a deliberate overflow.\n");
   ur = jsuwe(jcuw(0X01234560UL,ub31), 8, 7);
   uc = jcuw(um1,um1);
   ptruw("SUE7", ur, uc);

   printf("The next test has a deliberate overflow.\n");
   ur = jsuwe(jcuw(3,ub31), 36, 8);
   uc = jcuw(um1,um1);
   ptruw("SUE8", ur, uc);

   printf("The next test has a deliberate overflow.\n");
   ur = jsuwe(jcuw(0XFEDCBA9FUL,ub31), 8, 9);
   uc = jcuw(um1,um1);
   ptruw("SUE9", ur, uc);

   printf("The next test has a deliberate overflow.\n");
   ur = jsuwe(jcuw(~3ul,ub31), 36, 10);
   uc = jcuw(um1,um1);
   ptruw("SUE10", ur, uc);

/* Test jsluwe -- Similar to jsuwe except only left shifts */

   printf("\n===jsluwe===\n");

   ur = jsluwe(jcuw(3,ub31), 4, 1);
   uc = jcuw(0x37,0XFFFFFFF0UL);
   ptruw("JSLUWE1", ur, uc);

   ur = jsluwe(jcuw(0,0X00123456UL), 36, 2);
   uc = jcuw(0X01234560UL,0);
   ptruw("JSLUWE2", ur, uc);

   /* Shift into sign bit OK with unsigned args */
   ur = jsluwe(jcuw(0X01234560UL,ub31), 7, 3);
   uc = jcuw(0X91A2B03F,0XFFFFFF80);
   ptruw("JSLUWE3", ur, uc);

   printf("The next test has a deliberate overflow.\n");
   ur = jsluwe(jcuw(0X01234560UL,ub31), 8, 4);
   uc = jcuw(um1,um1);
   ptruw("JSLUWE4", ur, uc);

   printf("The next test has a deliberate overflow.\n");
   ur = jsluwe(jcuw(3,ub31), 36, 5);
   uc = jcuw(um1,um1);
   ptruw("JSLUWE5", ur, uc);

   printf("The next test has a deliberate overflow.\n");
   ur = jsluwe(jcuw(0XFEDCBA9FUL,ub31), 8, 6);
   uc = jcuw(um1,um1);
   ptruw("JSLUWE6", ur, uc);

   /* Test a shift of exactly 32 */
   ur = jsluwe(jcuw(0,ub31), 32, 7);
   uc = jcuw(ub31,0);
   ptruw("JSLUWE7", ur, uc);

/* Test jsruw -- Similar to jsuw except only right shifts */

   printf("\n===jsruw===\n");

   ur = jsruw(jcuw(0X123UL,ub31), 4);
   uc = jcuw(0X12UL,0X37FFFFFFUL);
   ptruw("JSRUW1", ur, uc);

   ur = jsruw(jcuw(0X123UL,ub31), 34);
   uc = jcuw(0,0X48UL);
   ptruw("JSRUW2", ur, uc);

   ur = jsruw(jcuw(um2,ub31), 4);
   uc = jcuw(0X0FFFFFFFUL,0XE7FFFFFFUL);
   ptruw("JSRUW3", ur, uc);

   ur = jsruw(jcuw(0XFFFFFEDCUL,0X456789ABUL), 4);
   uc = jcuw(0X0FFFFFEDUL,0XC456789AUL);
   ptruw("JSRUW4", ur, uc);

   ur = jsruw(jcuw(ubm,0), 8);
   uc = jcuw(0X00800000UL,0);
   ptruw("JSRUW5", ur, uc);

/* Test jsrruw */

   printf("\n===jsrruw===\n");

   ur = jsrruw(jcuw(0X123L,ub31), 4);
   uc = jcuw(0X12L,0X38000000L);
   ptruw("JSRRUW1", ur, uc);

   ur = jsrruw(jcuw(0X123L,ub31), 32);
   uc = jcuw(0,0X123L);
   ptruw("JSRRUW2", ur, uc);

   ur = jsrruw(jcuw(0X123L,ub31), 34);
   uc = jcuw(0,0X49L);
   ptruw("JSRRUW3", ur, uc);

   ur = jsrruw(jcuw(b31,um1), 1);
   uc = jcuw(0X40000000,0);
   ptruw("JSRRUW4", ur, uc);

   ur = jsrruw(jcuw(b31,um1), 2);
   uc = jcuw(0X20000000,0);
   ptruw("JSRRUW5", ur, uc);

/* Test drswq */

   printf("\n===drswq===\n");

   q = drswq(jcsw(7,0xFFF80000), b15);
   ptrsl("DRSW1", q, 524288);

   q = drswq(jcsw(7,0xFFF90000), b15);
   ptrsl("DRSW2", q, 524289);

   q = drswq(jcsw(7,0xFFF80000), -b15);
   ptrsl("DRSW1", q, -524288);

   q = drswq(jcsw(7,0xFFF90000), -b15);
   ptrsl("DRSW2", q, -524289);

/* Test dmrswjwe */

   printf("\n===dmrswjwe===\n");

   sr = dmrswjwe(jcsw(7,0), 1, 65536, 0, 1);
   ptrsw("DMRW1", sr, jesl(458752));

   sr = dmrswjwe(jcsw(7,40000), 1, 65536, 0, 2);
   ptrsw("DMRW2", sr, jesl(458753));

   sr = dmrswjwe(jcsw(7,873482197), 1024, 250000, 2, 3);
   ptrsw("DMRW3", sr, jesl(31680771));

   sr = dmrswjwe(jcsw(7,873484299), 1023, 250001, 2, 4);
   ptrsw("DMRW4", sr, jesl(31649709));

   sr = dmrswjwe(jcsw(2147483647,4294967295U), 2147483646,
      2147483647, 0, 5);
   ptrsw("DMRW5", sr, jcsw(2147483646,4294967293U));

   sr = dmrswjwe(jcsw(131072,0), 16384, 262144, 1, 6);
   ptrsw("DMRW6", sr, jcsw(4096,0));

   sr = dmrswjwe(jcsw(700,854400000), 1100001, 257, 32, 7);
   ptrsw("DMRW7", sr, jesl(2996963));

   printf("Test DMRW8: deliberate shift too large.\n");
   printf("Returned values are irrelevant.\n");
   sr = dmrswjwe(jcsw(255,873482197), 255, 250000, 64, 8);

   /* Repeat one of the above with negative dividend */
   sr = dmrswjwe(jcsw(~7,-873484299), 1023, 250001, 2, 4);
   ptrsw("DMRW9", sr, jesl(-31649709));

   /* And with negative divisor */
   sr = dmrswjwe(jcsw(7,873484299), 1023, -250001, 2, 4);
   ptrsw("DMRW10", sr, jesl(-31649709));

   /* And with all three negative */
   sr = dmrswjwe(jcsw(~7,-873484299), -1023, -250001, 2, 4);
   ptrsw("DMRW11", sr, jesl(-31649709));

/* Test dmrswwqe(si64 x, si64 y, si32 m, int ec) */

   printf("\n===dmrswwqe===\n");

   /* Cases in which multiplication does not exceed 64 bits */
   sr = dmrswwqe(jcsw(7,0), 65536, 2999, 1);
   ptrsw("DMWW1", sr, jesl(1375797248));

   sr = dmrswwqe(jcsw(7,40000), 327680, 65536, 2);
   ptrsw("DMWW2", sr, jcsw(1,1717994918));

   /* Multiplication does exceed 64 bits (vdivl used) */
   sr = dmrswwqe(jcsw(19,873482197), 2061946, 250000, 3);
   ptrsw("DMWW3", sr, jcsw(2,1410067932));

   /* Should round up */
   sr = dmrswwqe(jcsw(19,873482197), 2061951, 250000, 4);
   ptrsw("DMWW4", sr, jcsw(2,1410043684));

   sr = dmrswwqe(jcsw(2147483647,4294967295U), 2147483646,
      2147483645, 5);
   ptrsw("DMWW5", sr, jcsw(2147483646,4294967291));

   sr = dmrswwqe(jcsw(131072,0), 16384, 262144, 6);
   ptrsw("DMWW6", sr, jcsw(2097152,0));

   sr = dmrswwqe(jcsw(700,854400000), 1100001, 257, 7);
   ptrsw("DMWW7", sr, jesl(702621359));

   printf("Test DMWW8: deliberate quotient > 2^64.\n");
   sr = dmrswwqe(jcsw(2147483647,4294967295U), 1024, 2147483647, 8);
   ptrsw("DMWW8", sr, jcsw(SI32_MAX,~0));

   /* Repeat one of the above with negative dividend */
   sr = dmrswwqe(jcsw(~19,-873482197), 2061951, 250000, 9);
   ptrsw("DMWW9", sr, jcsw(~2,-1410043684));

   /* And with negative divisor */
   sr = dmrswwqe(jcsw(131072,0), -16384, 262144, 10);
   ptrsw("DMWW10", sr, jcsw(-2097152,0));

   /* And with all three negative */
   sr = dmrswwqe(jcsw(-131072,0), -16384, -262144, 11);
   ptrsw("DMWW11", sr, jcsw(-2097152,0));

/* Test dsrsjqe */

   printf("\n===dsrsjqe===\n");

   q = dsrsjqe(7, 32, 65536, 1);
   ptrsl("DSRJ1", q, 458752);

   q = dsrsjqe(498752, 16, 65535, 2);
   ptrsl("DSRJ2", q, 498760);

   q = dsrsjqe(873482197, 10, 250000, 3);
   ptrsl("DSRJ3", q, 3577783);

   /* This one should round up one from the above result */
   q = dsrsjqe(873482377, 10, 250000, 4);
   ptrsl("DSRJ4", q, 3577784);

   printf("Test DSRS5:  deliberate overflow into the sign bit.\n");
   q = dsrsjqe(200000, 46, 250000, 5);
   ptrsl("DSRJ5", q, SI32_MAX);

   printf("Test DSRS6: deliberate divide check.\n");
   q = dsrsjqe(1379400000, 16, 10000, 6);
   ptrsl("DSRJ6", q, SI32_MAX);

   q = dsrsjqe(1558734822, -8, 2500, 7);
   ptrsl("DSRJ7", q, 2436);

   /* Repeat one of the above with negative dividend */
   q = dsrsjqe(-873482197, 10, 250000, 8);
   ptrsl("DSRJ8", q, -3577783);

   /* And with negative divisor */
   q = dsrsjqe(873482197, 10, -250000, 9);
   ptrsl("DSRJ9", q, -3577783);

/* Test dsrswq */

   printf("\n===dsrswq===\n");

   q = dsrswq(jcsw(7,0), 0, 65536);
   ptrsl("DSRS1", q, 458752);

   q = dsrswq(jcsw(7,40000), 0, 65536);
   ptrsl("DSRS2", q, 458753);

   q = dsrswq(jcsw(7,873482197), 10, 250000);
   ptrsl("DSRS3", q, 126723085);

   /* This one should round up one from the above result */
   q = dsrswq(jcsw(7,873482257), 10, 250000);
   ptrsl("DSRS4", q, 126723086);

   q = dsrswq(jcsw(7,873482397), 10, 250000);
   ptrsl("DSRS5", q, 126723086);

   printf("Test DSRS6:  deliberate overflow into the sign bit.\n");
   printf("Returned values are irrelevant.\n");
   dsrswq(jcsw(200000,0), 14, 250000);

   printf("Test DSRS7 has a deliberate divide check.\n");
   printf("Returned values are irrelevant.\n");
   dsrswq(jcsw(7,400000), 16, 100000);

   q = dsrswq(jcsw(255,873482197), -8, 250000);
   ptrsl("DSRS8", q, 17126);

   /* Repeat one of the above with negative dividend */
   q = dsrswq(jcsw(~7,-873482257), 10, 250000);
   ptrsl("DSRS9", q, -126723086);

   /* And with negative divisor */
   q = dsrswq(jcsw(7,873482257), 10, -250000);
   ptrsl("DSRS10", q, -126723086);

/* Test dsrswqe */

   printf("\n===dsrswqe===\n");

   q = dsrswqe(jcsw(7,0), 0, 65536, 1);
   ptrsl("ESRS1", q, 458752);

   q = dsrswqe(jcsw(7,40000), 0, 65536, 2);
   ptrsl("ESRS2", q, 458753);

   q = dsrswqe(jcsw(7,873482197), 10, 250000, 3);
   ptrsl("ESRS3", q, 126723085);

   /* This one should round up one from the above result */
   q = dsrswqe(jcsw(7,873482257), 10, 250000, 4);
   ptrsl("ESRS4", q, 126723086);

   q = dsrswqe(jcsw(7,873482397), 10, 250000, 5);
   ptrsl("ESRS5", q, 126723086);

   printf("Test DSRS6:  deliberate overflow into the sign bit.\n");
   q = dsrswqe(jcsw(200000,0), 14, 250000, 6);
   ptrsl("ESRS6", q, SI32_MAX);

   printf("Test DSRS7: deliberate divide check.\n");
   q = dsrswqe(jcsw(7,400000), 16, 100000, 7);
   ptrsl("ESRS7", q, SI32_MAX);

   q = dsrswqe(jcsw(255,873482197), -8, 250000, 8);
   ptrsl("ESRS8", q, 17126);

   /* Repeat one of the above with negative dividend */
   q = dsrswqe(jcsw(~7,-873482257), 10, 250000, 9);
   ptrsl("ESRS9", q, -126723086);

   /* And with negative divisor */
   q = dsrswqe(jcsw(7,873482257), 10, -250000, 10);
   ptrsl("ESRS10", q, -126723086);

/* Test dsrswwqe */

   printf("\n===dsrswwqe===\n");

   sr = dsrswwqe(jcsw(7,0), jesl(65536), 0, 1);
   ptrsw("EDSW1", sr, jesl(458752));

   sr = dsrswwqe(jcsw(7,40000), jesl(65536), 0, 2);
   ptrsw("EDSW2", sr, jesl(458753));

   sr = dsrswwqe(jcsw(7,873482197), jesl(250000), 10, 3);
   ptrsw("EDSW3", sr, jesl(126723085));

   /* This one should round up one from the above result */
   sr = dsrswwqe(jcsw(7,873482257), jesl(250000), 10, 4);
   ptrsw("EDSW4", sr, jesl(126723086));

   sr = dsrswwqe(jcsw(7,873482397), jesl(250000), 10, 5);
   ptrsw("EDSW5", sr, jesl(126723086));

   sr = dsrswwqe(jcsw(200000,0), jesl(250000), 14, 6);
   ptrsw("EDSW6", sr, jcsw(13107,858993459));

   sr = dsrswwqe(jcsw(700,854400000), jcsw(1,100000), 16, 7);
   ptrsw("EDSW7", sr, jesl(45887169));

   printf("Test EDSW8: deliberate shift too large.\n");
   printf("Returned values are irrelevant.\n");
   sr = dsrswwqe(jcsw(255,873482197), jesl(250000), 36, 8);

   /* Repeat one of the above with negative dividend */
   sr = dsrswwqe(jcsw(~7,-873482257), jesl(250000), 10, 9);
   ptrsw("EDSW9", sr, jesl(-126723086));

   /* And with negative divisor */
   sr = dsrswwqe(jcsw(7,873482257), jesl(-250000), 10, 10);
   ptrsw("EDSW10", sr, jesl(-126723086));

   /* And with both negative */
   sr = dsrswwqe(jcsw(~7,-873482257), jesl(-250000), 10, 11);
   ptrsw("EDSW11", sr, jesl(126723086));

/* Test dsrswjqe */

   printf("\n===dsrswjqe===\n");

   sr = dsrswjqe(jcsw(7,0), 0, 65536, 1);
   ptrsw("DWJQ1", sr, jesl(458752));

   sr = dsrswjqe(jcsw(7,40000), 0, 65536, 2);
   ptrsw("DWJQ2", sr, jesl(458753));

   sr = dsrswjqe(jcsw(7,873482197), 10, 250000, 3);
   ptrsw("DWJQ3", sr, jesl(126723085));

   /* This one should round up one from the above result */
   sr = dsrswjqe(jcsw(7,873482257), 10, 250000, 4);
   ptrsw("DWJQ4", sr, jesl(126723086));

   sr = dsrswjqe(jcsw(7,873482397), 10, 262143, 5);
   ptrsw("DWJQ5", sr, jesl(120853014));

   sr = dsrswjqe(jcsw(200000,0), 13, 250000, 6);
   ptrsw("DWJQ6", sr, jcsw(6553,2576980378));

   sr = dsrswjqe(jcsw(700,854400000), 16, 100000, 7);
   ptrsw("DWJQ7", sr, jcsw(458,3789754991));

   printf("Test DWJQ8: deliberate shift too large.\n");
   printf("Returned values are irrelevant.\n");
   sr = dsrswjqe(jcsw(255,873482197), 36, 250000, 8);

   /* Repeat one of the above with negative dividend */
   sr = dsrswjqe(jcsw(~7,-873482257), 10, 250000, 9);
   ptrsw("DWJQ9", sr, jesl(-126723086));

   /* And with negative divisor */
   sr = dsrswjqe(jcsw(7,873482257), 10, -250000, 10);
   ptrsw("DWJQ10", sr, jesl(-126723086));

   /* And with both negative */
   sr = dsrswjqe(jcsw(~7,-873482257), 10, -250000, 11);
   ptrsw("DWJQ11", sr, jesl(126723086));

/* Test dsruwq */

   printf("\n===dsruwq===\n");

   uq = dsruwq(jcuw(7,0), 0, 65536);
   ptrul("DSRU1", uq, 458752);

   uq = dsruwq(jcuw(7,40000), 0, 65536);
   ptrul("DSRU2", uq, 458753);

   uq = dsruwq(jcuw(7,873482197), 10, 250000);
   ptrul("DSRU3", uq, 126723085);

   /* This one should round up one from the above result */
   uq = dsruwq(jcuw(7,873482257), 10, 250000);
   ptrul("DSRU4", uq, 126723086);

   uq = dsruwq(jcuw(7,873482397), 10, 250000);
   ptrul("DSRU5", uq, 126723086);

   printf("Test DSRU6: deliberate overflow past the sign bit.\n");
   printf("Returned values are irrelevant.\n");
   dsruwq(jcuw(200000,0), 15, 250000);

   printf("Test DSRU7 has a deliberate divide check.\n");
   printf("Returned values are irrelevant.\n");
   dsruwq(jcuw(7,400000), 16, 100000);

   uq = dsruwq(jcuw(255,873482197), -8, 250000);
   ptrul("DSRU8", uq, 17126);

   uq = dsruwq(jcuw(1,1), 31, um1);
   ptrul("DSRU9", uq, 2147483649UL);

   printf("Test DSRU10 has a subtle kind of divide check.\n");
   printf("Returned values are irrelevant.\n");
   dsruwq(jcuw(1,0), 31, ub31);

/* Test dsruwqe */

   printf("\n===dsruwqe===\n");

   uq = dsruwqe(jcuw(7,0), 0, 65536, 1);
   ptrul("ESRU1", uq, 458752);

   uq = dsruwqe(jcuw(7,40000), 0, 65536, 2);
   ptrul("ESRU2", uq, 458753);

   uq = dsruwqe(jcuw(7,873482197), 10, 250000, 3);
   ptrul("ESRU3", uq, 126723085);

   /* This one should round up one from the above result */
   uq = dsruwqe(jcuw(7,873482257), 10, 250000, 4);
   ptrul("ESRU4", uq, 126723086);

   uq = dsruwqe(jcuw(7,873482397), 10, 250000, 5);
   ptrul("ESRU5", uq, 126723086);

   printf("Test DSRU6: deliberate overflow past the sign bit.\n");
   uq = dsruwqe(jcuw(200000,0), 15, 250000, 6);
   ptrul("ESRU6", uq, UI32_MAX);

   printf("Test DSRU7: deliberate divide check.\n");
   uq = dsruwqe(jcuw(7,400000), 16, 100000, 7);
   ptrul("ESRU7", uq, UI32_MAX);

   uq = dsruwqe(jcuw(255,873482197), -8, 250000, 8);
   ptrul("ESRU8", uq, 17126);

   uq = dsruwqe(jcuw(1,1), 31, um1, 9);
   ptrul("ESRU9", uq, 2147483649UL);

   printf("Test DSRU10 has a subtle kind of divide check.\n");
   uq = dsruwqe(jcuw(1,0), 31, ub31, 10);
   ptrul("ESRU10", uq, UI32_MAX);

/* Test dsruwwqe */

   printf("\n===dsruwwqe===\n");

   ur = dsruwwqe(jcuw(7,0), jeul(65536), 0, 1);
   ptruw("EDUW1", ur, jeul(458752));

   ur = dsruwwqe(jcuw(7,40000), jeul(65536), 0, 2);
   ptruw("EDUW2", ur, jeul(458753));

   ur = dsruwwqe(jcuw(7,873482197), jeul(250000), 10, 3);
   ptruw("EDUW3", ur, jeul(126723085));

   /* This one should round up one from the above result */
   ur = dsruwwqe(jcuw(7,873482257), jeul(250000), 10,4);
   ptruw("EDUW4", ur, jeul(126723086));

   ur = dsruwwqe(jcuw(7,873482397), jeul(250000), 10, 5);
   ptruw("EDUW5", ur, jeul(126723086));

   ur = dsruwwqe(jcuw(200000,0), jeul(250000), 15, 6);
   ptruw("EDUW6", ur, jcuw(26214,1717986918));

   ur = dsruwwqe(jcuw(700,854400000), jcuw(21,100000), 16, 7);
   ptruw("EDUW7", ur, jeul(2185152));

   printf("Test EDUW8: deliberate negative shift.\n");
   printf("Returned values are irrelevant.\n");
   ur = dsruwwqe(jcuw(255,873482197), jeul(250000), -8, 8);

   ur = dsruwwqe(jcuw(1,1), jeul(um1), 31, 9);
   ptruw("EDUW9", ur, jeul(2147483649U));

   printf("Test DSRU10 has a subtle kind of divide check.\n");
   ur = dsruwwqe(jcuw(ubm,0), jeul(1073741824), 31, 10);
   ptruw("EDUW10", ur, jcuw(um1,um1));

/* Test mrssw */

   printf("\n===mrssw===\n");
   printf("***Note:  This routine is obsolete and should be replaced\n"
      "***with mrsswe in all applications--many dangerous overflows\n"
      "***are not detected (by design), and in those cases, results\n"
      "***may differ between 32-bit and 64-bit compilations.  Also,\n"
      "***negative results differ by one bit between 64- and 32-bit\n"
      "***versions due to shift neg. vs. shift pos, then negate.\n");
   sr = mrssw(jcsw(2,5),47,2);
   sc = jcsw(23,0x8000003aUL);
   ptrsw("MRSS1", sr, sc);

   sr = mrssw(jcsw(b15,u6u),b15,8);
   ptrxw("MRSS2", sr, jcsw(0xFFFFFEFFL,0x66679899UL),
      jcsw(0x00FFFEFFL,0x66679899UL));

   sr = mrssw(jcsw(b15,u6u),b15,40);
   ptrxw("MRSS3", sr, jcsw(m1,0xFFFFFEFFUL), jcsw(0,0x00FFFEFFUL));

   sr = mrssw(jcsw(b15,u6u),b15,0);
   sc = jcsw(0xFFFEFF66L,0x67989901UL);
   ptrsw("MRSS4", sr, sc);

   sr = mrssw(jcsw(b15,u6u),b15,8);
   ptrxw("MRSS5", sr, jcsw(0xFFFFFEFFL,0x66679899UL),
      jcsw(0xFFFEFFL,0x66679899UL));

   sr = mrssw(jcsw(0xFFFF0000L,0x00999901UL),b15,8);
   ptrxw("MRSS6", sr, jcsw(0x00000100L,0x99986766UL),
      jcsw(0xFF000100L,0x99986767UL));

   printf("The next test has a deliberate overflow--"
      "not detected by design.\n");
   sr = mrssw(jcsw(0x7FFF0000L,0x00999901UL),b15,8);
   ptrxw("MRSS7", sr, jcsw(0xFF800100L,0x99986766UL),
      jcsw(0x00800100L,0x99986766UL));

   printf("The next test has a deliberate overflow--"
      "not detected by design.\n");
   sr = mrssw(jcsw(bm,0),b15,12);
   sc = jcsw(0xFFF80000,0);
   ptrsw("MRSS8", sr, sc);

/* Test mlsswje */

   printf("\n===mlsswje===\n");

   /* Testing different sign combinations */
   sr = mlsswje(12345,6789,2,1);
   sc = jesl(335240820);
   ptrsw("MLSSWJE1", sr, sc);

   sr = mlsswje(-4096,28902,4,2);
   sc = jesl(-1894121472);
   ptrsw("MLSSWJE2", sr, sc);

   sr = mlsswje(-4096,-28902,4,3);
   sc = jesl(1894121472);
   ptrsw("MLSSWJE3", sr, sc);

   /* Shift brings low product to high word */
   sr = mlsswje(4096,28902,11,4);
   sc = jcsw(0x38,0x73000000);
   ptrsw("MLSSWJE4", sr, sc);

   /* Product > 32 bits */
   sr = mlsswje(0x33FFFF8,0x1248888,0,5);
   sc = jcsw(0x3B6BB,0xB0DBBBC0);
   ptrsw("MLSSWJE5", sr, sc);

   sr = mlsswje(0x33FFFF8,-0x1248888,4,6);
   sc = jnsw(jcsw(0x3B6BBB,0x0DBBBC00));
   ptrsw("MLSSWJE6", sr, sc);

   /* Test shifts of 32 and >32, both signs */
   sr = mlsswje(b15,0x7FFF,32,7);
   sc = jcsw(0x7FFE8001,0);
   ptrsw("MLSSWJE7", sr, sc);

   sr = mlsswje(0xFF,0xFFF,40,8);
   sc = jcsw(0x0FEF0100,0);
   ptrsw("MLSSWJE8", sr, sc);

   sr = mlsswje(-0xFF,0xFFF,40,9);
   sc = jnsw(sc);
   ptrsw("MLSSWJE9", sr, sc);

   /* Multiplications involving the biggest positive and
   *  negative numbers */
   sr = mlsswje(b31,b31,1,10);
   sc = jcsw(0x7FFFFFFE,0x00000002);
   ptrsw("MLSSWJE10", sr, sc);

   sr = mlsswje(bm,b15,2,11);
   sc = jnsw(jcsw(0x1FFFE,0));
   ptrsw("MLSSWJE11", sr, sc);

   sr = mlsswje(bm,bm,0,12);
   sc = jcsw(0x40000000,0);
   ptrsw("MLSSWJE12", sr, sc);

   printf("The next test has a deliberate overflow.\n");
   sr = mlsswje(bm,bm,1,13);
   sc = jcsw(b31,m1);
   ptrsw("MLSSWJE13", sr, sc);

   printf("The next test has a deliberate overflow.\n");
   sr = mlsswje(-1234500,6789000,25,14);
   sc = jcsw(bm,0);
   ptrsw("MLSSWJE14", sr, sc);

/* Test mrsswj, similar to mrssl but 64-bit answers */

   printf("\n===mrsswj===\n");

   /* Testing different sign combinations */
   sr = mrsswj(18458,28902,2);
   sc = jesl(133368279);
   ptrsw("MRSSWJ1", sr, sc);

   sr = mrsswj(-18458,28902,2);
   sc = jesl(-133368279);
   ptrsw("MRSSWJ2", sr, sc);

   sr = mrsswj(-18458,-28902,2);
   sc = jesl(133368279);
   ptrsw("MRSSWJ3", sr, sc);

   /* Result > 32 bits */
   sr = mrsswj(0x33FFFF,0x1248888,2);
   sc = jcsw(0xEDA,0xEE9EDDDE);
   ptrsw("MRSSWJ4", sr, sc);

   sr = mrsswj(0x33FFFF,-0x1248888,2);
   sc = jnsw(sc);
   ptrsw("MRSSWJ5", sr, sc);

   /* Test shifts of 0, 32, and >32 */
   sr = mrsswj(b31,b31m1,0);
   sc = jcsw(0x3FFFFFFE,0x80000002);
   ptrsw("MRSSWJ6", sr, sc);

   sr = mrsswj(b31,b31m1,32);
   sc = jesl(0x3FFFFFFE);
   ptrsw("MRSSWJ7", sr, sc);

   sr = mrsswj(b31,b31m1,33);
   sc = jesl(0x1FFFFFFF);
   ptrsw("MRSSWJ8", sr, sc);

   /* Multiplications involving the biggest positive and
   *  negative numbers */
   sr = mrsswj(b31,bm,2);
   sc = jnsw(jcsw(0x0FFFFFFF,0xE0000000));
   ptrsw("MRSSWJ9", sr, sc);

   sr = mrsswj(bm,b15,2);
   sc = jnsw(jcsw(0x1FFF,0xE0000000));
   ptrsw("MRSSWJ10", sr, sc);

   sr = mrsswj(bm,bm,36);
   sc = jesl(0x04000000);
   ptrsw("MRSSWJ11", sr, sc);

/* Test mrssle.
*  New, 12/30/08 - This routine is similar to mrsswe except both
*  multiplicands are 32-bit.  */

   printf("\n===mrssle===\n");
   printf("***Note:  Shifts of negative results modified 09/13/14\n"
      "   to give same results as hdwr arithmetic-right-shift.\n");

   /* Testing different sign combinations */
   r = mssle(18458,28902,-2,1);
   q = 133368279;
   ptrsl("MRSSLE1", r, q);

   r = mrssle(-18458,28902,2,2);
   q = -133368279;
   ptrsl("MRSSLE2", r, q);

   r = mrssle(-18458,-28902,2,3);
   q = 133368279;
   ptrsl("MRSSLE3", r, q);

   /* > 32 bits, but shift brings it back--no overflow */
   r = mrssle(0x33FFFF,b15,8,4);
   q = 0x33FFCB00;
   ptrsl("MRSSLE4", r, q);

   r = mrssle(0x33FFFF,-b15,8,5);
   q = -q-1;      /* -1 for shift change */
   ptrsl("MRSSLE5", r, q);

   /* Test zero shift and positive shift now taken as right shift */
   r = mrssle(0x33FF,0x20,2,6);
   q = 0x19FF8;
   ptrsl("MRSSLE6", r, q);

   r = mrssle(-18458,28902,0,7);
   q = -533473116;
   ptrsl("MRSSLE7", r, q);

   /* Multiplications involving the biggest positive and
   *  negative numbers */
   r = mssle(b31,b15,-16,8);
   q = 0x7FFF7FFF;
   ptrsl("MRSSLE8", r, q);

   r = mrssle(bm,b15,16,9);
   q = 0x80008000;
   ptrsl("MRSSLE9", r, q);

   r = mrssle(bm,256,8,10);
   q = 0x80000000;
   ptrsl("MRSSLE10", r, q);

   printf("The next test has a deliberate overflow.\n");
   r = mrssle(bm,bm,31,11);
   q = b31;
   ptrsl("MRSSLE11", r, q);

   printf("The next test has a deliberate overflow.\n");
   r = mrssle(0x7FFF0000,b15,8,12);
   q = b31;
   ptrsl("MRSSLE12", r, q);

   printf("The next test has a deliberate overflow.\n");
   r = mrssle(65536,32768,0,13);
   q = b31;
   ptrsl("MRSSLE13", r, q);

   printf("The next test has a deliberate overflow.\n");
   r = mrssle(b31,b31m1,8,14);
   q = b31;
   ptrsl("MRSSLE14", r, q);

/* Test mrsswe.
*  Rev, 11/23/08, GNR - This routine now supports a 96-bit intermediate
*  product, therefore tests have been added to exercise this feature,
*  including multiplying the largest negative number by +1 or -1  */

   printf("\n===mrsswe===\n");
   printf("***Note:  Shifts of negative results modified 09/20/14\n"
      "   to give same results as hdwr arithmetic-right-shift.\n");

   sr = mrsswe(jcsw(2,5),47,2,1);
   sc = jcsw(23,0x8000003aUL);
   ptrsw("MRSSE1", sr, sc);

   sr = mrsswe(jcsw(b15,u6u),b15,8,2);
   sc = jcsw(0xFFFEFFL,0x66679899UL);
   ptrsw("MRSSE2", sr, sc);

   sr = mrsswe(jcsw(b15,u6u),b15,40,3);
   sc = jcsw(0,0xFFFEFFL);
   ptrsw("MRSSE3", sr, sc);

   /* In this test, the multiplication results in the
   *  sign bit being set, but the shift moves it to
   *  the right, so an overflow should not be detected.  */
   sr = mrsswe(jcsw(b15,u6u),b15,4,4);
   sc = jcsw(0x0FFFEFF6L,0x66798990UL);
   ptrsw("MRSSE4", sr, sc);

   sr = msswe(jcsw(b15,u6u),b15,-8,5);
   sc = jcsw(0xFFFEFFL,0x66679899UL);
   ptrsw("MRSSE5", sr, sc);

   sr = mrsswe(jcsw(0xFFFF0000L,0x00999901UL),b15,8,6);
   sc = jcsw(0xFF000100L,0x99986766UL);
   ptrsw("MRSSE6", sr, sc);

   printf("The next test has a deliberate overflow.\n");
   sr = mrsswe(jcsw(0x7FFF0000L,0x00999901UL),b15,8,7);
   sc = jcsw(b31,um1);
   ptrsw("MRSSE7", sr, sc);

   printf("The next test has a deliberate overflow.\n");
   sr = mrsswe(jcsw(bm,0),b15,12,8);
   sc = jcsw(bm,0);
   ptrsw("MRSSE8", sr, sc);

   /* Following are tests added for mrsswe V2A */
   sr = mrsswe(jcsw(0x7FFF0000L,0x00999901UL),b15,16,9);
   sc = jcsw(0x7FFE8001L,0x00999867UL);
   ptrsw("MRSSE9", sr, sc);

   sr = mrsswe(jcsw(b31,um1),b31,32,10);
   sc = jcsw(0x3FFFFFFFL,ub31);
   ptrsw("MRSSE10",sr,sc);

   sr = mrsswe(jcsw(bm,0),bm,33,11);
   sc = jcsw(0x20000000L,0);
   ptrsw("MRSSE11",sr,sc);

   sr = mrsswe(jcsw(bm,0),1,32,12);
   sc = jcsw(m1,bm);
   ptrsw("MRSSE12",sr,sc);

/* Test mrsrsle.
*  New, 12/30/08 - This routine is similar to mrsrswe except both
*  multiplicands are 32-bit.  */

   printf("\n===mrsrsle===\n");
   printf("***Note:  Shifts of negative results modified 09/29/14\n"
      "   to give same results as hdwr arithmetic-right-shift.\n");

   /* Testing different sign combinations */
   r = mrsrsle(18459,28902,2,1);
   q = 133375505;
   ptrsl("MRSRSLE1", r, q);

   r = mrsrsle(-18459,28902,2,2);
   q = -133375504;
   ptrsl("MRSRSLE2", r, q);

   r = mrsrsle(-18459,-28902,2,3);
   q = 133375505;
   ptrsl("MRSRSLE3", r, q);

   /* > 32 bits, but shift brings it back--no overflow */
   r = mrsrsle(0x33FFFF,b15,8,4);
   q = 0x33FFCB00;
   ptrsl("MRSRSLE4", r, q);

   r = mrsrsle(0x33FFFF,-b15,8,5);
   q = -q;
   ptrsl("MRSRSLE5", r, q);

   /* Test negative and zero shift */
   r = msrsle(0x33FFFF,-b15,-8,6);
   ptrsl("MRSRSLE6", r, q);

   r = mrsrsle(-18459,28902,0,7);
   q = -533502018;
   ptrsl("MRSRSLE7", r, q);

   /* Multiplications involving the biggest positive and
   *  negative numbers */
   r = mrsrsle(b31,b15,16,8);
   q = 0x7FFF7FFF;
   ptrsl("MRSRSLE8", r, q);

   r = mrsrsle(bm,b15,16,9);
   q = 0x80008000;
   ptrsl("MRSRSLE9", r, q);

   r = mrsrsle(bm,256,8,10);
   q = 0x80000000;
   ptrsl("MRSRSLE10", r, q);

   printf("The next test has a deliberate overflow.\n");
   r = mrsrsle(bm,bm,31,11);
   q = b31;
   ptrsl("MRSRSLE11", r, q);

   printf("The next test has a deliberate overflow.\n");
   r = mrsrsle(0x7FFF0000,b15,8,12);
   q = b31;
   ptrsl("MRSRSLE12", r, q);

   printf("The next test has a deliberate overflow.\n");
   r = mrsrsle(65536,32768,0,13);
   q = b31;
   ptrsl("MRSRSLE13", r, q);

   /* Make sure there is no false carry when low order is 0 */
   r = mrsrsle(65536,1048576,32,14);
   q = 16;
   ptrsl("MRSRSLE14", r, q);

   r = mrsrsle(65536,1048576,34,15);
   q = 4;
   ptrsl("MRSRSLE15", r, q);

/* Test mrsrswj, similar to mrsrsl but 64-bit answers.
*  Overflow is not possible, so overflow checking versions
*  (mrsrswjd, mrsrswje) do not exist and we test mrsrswj.  */

   printf("\n===mrsrswj===\n");

   /* Testing different sign combinations */
   sr = mrsrswj(18458,28902,2);
   sc = jesl(133368279);
   ptrsw("MRSRSWJ1", sr, sc);

   sr = mrsrswj(-18458,28902,2);
   sc = jesl(-133368279);
   ptrsw("MRSRSWJ2", sr, sc);

   sr = mrsrswj(-18458,-28902,2);
   sc = jesl(133368279);
   ptrsw("MRSRSWJ3", sr, sc);

   /* Result > 32 bits */
   sr = mrsrswj(0x33FFFF,0x1248888,2);
   sc = jcsw(0xEDA,0xEE9EDDDE);
   ptrsw("MRSRSWJ4", sr, sc);

   sr = mrsrswj(0x33FFFF,-0x1248888,2);
   sc = jnsw(jcsw(0xEDA,0xEE9EDDDE));
   ptrsw("MRSRSWJ5", sr, sc);

   /* Test shifts of 0, 32, and >32 */
   sr = mrsrswj(b31,b31m1,0);
   sc = jcsw(0x3FFFFFFE,0x80000002);
   ptrsw("MRSRSWJ6", sr, sc);

   sr = mrsrswj(b31,b31m1,32);
   sc = jesl(0x3FFFFFFF);
   ptrsw("MRSRSWJ7", sr, sc);

   sr = mrsrswj(b31,b31m1,33);
   sc = jesl(0x1FFFFFFF);
   ptrsw("MRSRSWJ8", sr, sc);

   /* Multiplications involving the biggest positive and
   *  negative numbers */
   sr = mrsrswj(b31,bm,2);
   sc = jnsw(jcsw(0x0FFFFFFF,0xE0000000));
   ptrsw("MRSRSWJ9", sr, sc);

   sr = mrsrswj(bm,b15,2);
   sc = jnsw(jcsw(0x1FFF,0xE0000000));
   ptrsw("MRSRSWJ10", sr, sc);

   sr = mrsrswj(bm,bm,36);
   sc = jesl(0x04000000);
   ptrsw("MRSRSWJ11", sr, sc);

   /* Make sure there is no false carry when low order is 0 */
   sr = mrsrswj(65536,1048576,32);
   sc = jesl(16);
   ptrsw("MRSRSWJ12", sr, sc);

   sr = mrsrswj(65536,1048576,34);
   sc = jesl(4);
   ptrsw("MRSRSWJ13", sr, sc);

/* Test mrsrswe */

   printf("\n===mrsrswe===\n");

   sr = mrsrswe(jcsw(2,5),47,2,1);
   sc = jcsw(23,0x8000003bUL);
   ptrsw("MRSRSE1", sr, sc);

   sr = mrsrswe(jcsw(b15,u6u),b15,8,2);
   sc = jcsw(0xFFFEFFL,0x66679899UL);
   ptrsw("MRSRSE2", sr, sc);

   sr = mrsrswe(jcsw(b15,u6u),b15,39,3);
   sc = jcsw(0,0x1FFFDFFL);
   ptrsw("MRSRSE3", sr, sc);

   sr = mrsrswe(jcsw(b15,u6u),b15,4,4);
   sc = jcsw(0x0FFFEFF6L,0x66798990UL);
   ptrsw("MRSRSE4", sr, sc);

   sr = mrsrswe(jcsw(b31,um1),1,4,5);
   sc = jcsw(0x08000000L,0UL);
   ptrsw("MRSRSE5", sr, sc);

   sr = mrsrswe(jcsw(0xFFFF0000L,0x00999901UL),b15,12,6);
   sc = jcsw(0xFFF00010L,0x09998676UL);
   ptrsw("MRSRSE6", sr, sc);

   printf("The next test has a deliberate overflow.\n");
   sr = mrsrswe(jcsw(0x7FFF0000L,0x00999901UL),b15,8,7);
   sc = jcsw(b31,um1);
   ptrsw("MRSRSE7", sr, sc);

   printf("The next test has a deliberate overflow.\n");
   sr = mrsrswe(jcsw(bm,0),b15,12,8);
   sc = jcsw(bm,0);
   ptrsw("MRSRSE8", sr, sc);

   sr = mrsrswe(jcsw(0x7FFF0000L,0x00999901UL),b15,16,9);
   sc = jcsw(0x7FFE8001L,0x00999867UL);
   ptrsw("MRSRSE9", sr, sc);

   sr = mrsrswe(jcsw(b31,um1),b31,32,10);
   sc = jcsw(0x3FFFFFFFL,ubm);
   ptrsw("MRSRSE10",sr,sc);

   sr = mrsrswe(jcsw(bm,0),bm,33,11);
   sc = jcsw(0x20000000L,0);
   ptrsw("MRSRSE11",sr,sc);

   sr = mrsrswe(jcsw(bm,0),1,32,12);
   sc = jcsw(m1,bm);
   ptrsw("MRSRSE12",sr,sc);

   /* Test carry of round into high 32 */
   sr = mrsrswe(jcsw(b31,um1),256,9,13);
   sc = jcsw(0x40000000L,0UL);
   ptrsw("MRSRSE13", sr, sc);

   /* Make sure there is no false carry when low order is 0 */
   sr = mrsrswe(jesl(65536),1048576,32,14);
   sc = jesl(16);
   ptrsw("MRSRSWE14", sr, sc);

   sr = mrsrswe(jesl(65536),1048576,34,15);
   sc = jesl(4);
   ptrsw("MRSRSWE15", sr, sc);

/* Test mrsuw */

   printf("\n===mrsuw===\n");

   ur = mrsuw(jcuw(2,5),47,2);
   uc = jcuw(23,0x8000003aUL);
   ptruw("MRSU1", ur, uc);

   ur = mrsuw(jcuw(b15,u6u),b15,8);
   uc = jcuw(0xFFFEFFL,0x66679899UL);
   ptruw("MRSU2", ur, uc);

   ur = mrsuw(jcuw(b15,u6u),b15,40);
   uc = jcuw(0,0xFFFEFFL);
   ptruw("MRSU3", ur, uc);

   ur = mrsuw(jcuw(b15,u6u),b15,0);
   uc = jcuw(0xFFFEFF66L,0x67989901UL);
   ptruw("MRSU4", ur, uc);

   ur = mrsuw(jcuw(b15,u6u),b15,8);
   uc = jcuw(0xFFFEFFL,0x66679899UL);
   ptruw("MRSU5", ur, uc);

   printf("The next test has a deliberate overflow--"
      "not detected by design.\n");
   ur = mrsuw(jcuw(0xFFFF0000L,0x00999901UL),b15,8);
   uc = jcuw(0x00000100L,0x99986766UL);
   ptruw("MRSU6", ur, uc);

   printf("The next test has a deliberate overflow--"
      "not detected by design.\n");
   ur = mrsuw(jcuw(0x7FFF0000L,0x00999901UL),b15,8);
   uc = jcuw(0x00800100L,0x99986766UL);
   ptruw("MRSU7", ur, uc);

   printf("The next test has a deliberate overflow--"
      "not detected by design.\n");
   ur = mrsuw(jcuw(bm,0),b15,12);
   uc = jcuw(0x00080000,0);
   ptruw("MRSU8", ur, uc);

/* Test mlsuwje */

   printf("\n===mlsuwje===\n");

   ur = mlsuwje(12345,6789,2,1);
   uc = jeul(335240820);
   ptruw("MLSUWJE1", ur, uc);

   /* Shift brings low product to high word */
   ur = mlsuwje(4096,28902,11,2);
   uc = jcuw(0x38,0x73000000);
   ptruw("MLSUWJE2", ur, uc);

   /* Product > 32 bits */
   ur = mlsuwje(0x33FFFF8,0x1248888,0,3);
   uc = jcuw(0x3B6BB,0xB0DBBBC0);
   ptruw("MLSUWJE3", ur, uc);

   ur = mlsuwje(0x33FFFF8,0x1248888,4,4);
   uc = jcuw(0x3B6BBB,0x0DBBBC00);
   ptruw("MLSUWJE4", ur, uc);

   /* Test shifts of 32 and >32 */
   ur = mlsuwje(b15,0x7FFF,32,5);
   uc = jcuw(0x7FFE8001,0);
   ptruw("MLSUWJE5", ur, uc);

   ur = mlsuwje(0xFF,0xFFF,40,6);
   uc = jcuw(0x0FEF0100,0);
   ptruw("MLSUWJE6", ur, uc);

   /* Multiplications involving the biggest unsigned number */
   ur = mlsuwje(ub31,ub31,1,7);
   uc = jcuw(0x7FFFFFFE,0x00000002);
   ptruw("MLSUWJE7", ur, uc);

   ur = mlsuwje(um1,um1,0,8);
   uc = jcuw(um2,1);
   ptruw("MLSUWJE8", ur, uc);

   ur = mlsuwje(ubm,ubm,1,9);
   uc = jcuw(0x80000000,0);
   ptruw("MLSUWJE9", ur, uc);

   printf("The next test has a deliberate overflow.\n");
   ur = mlsuwje(ubm,ubm,2,10);
   uc = jcuw(um1,um1);
   ptruw("MLSUWJE10", ur, uc);

   printf("The next test has a deliberate overflow.\n");
   ur = mlsuwje(1234500,6789000,25,11);
   uc = jcuw(um1,um1);
   ptruw("MLSUWJE11", ur, uc);

/* Test mrsuwj, similar to mrsul but 64-bit answers.
*  Overflow is not possible, so overflow checking versions
*  (mrsuwjd, mrsuwje) do not exist and we test mrsuwj.  */

   printf("\n===mrsuwj===\n");

   ur = mrsuwj(18458,28902,2);
   uc = jeul(133368279);
   ptruw("MRSUWJ1", ur, uc);

   /* > 32 bits, but shift brings it back  */
   ur = mrsuwj(0x33FFFF,ub15,8);
   uc = jeul(0x33FFCB00);
   ptruw("MRSUWJ2", ur, uc);

   ur = mrsuwj(0xF033FFFF,ub31m1,40);
   uc = jeul(0x007819FF);
   ptruw("MRSUWJ3", ur, uc);

   /* Test 32 and zero shifts */
   ur = mrsuwj(0xF033FFFF,ub31m1,32);
   uc = jeul(0x7819FFFD);
   ptruw("MRSUWJ4", ur, uc);

   ur = mrsuwj(18458,28902,0);
   uc = jeul(533473116);
   ptruw("MRSUWJ5", ur, uc);

   /* Multiplications involving the biggest positive number */
   ur = mrsuwj(um1,ub15,1);
   uc = jcuw(0x7FFF,0x7FFF8000);
   ptruw("MRSUWJ6", ur, uc);

   ur = mrsuwj(ubm,ub15,16);
   uc = jeul(0x7FFF8000);
   ptruw("MRSUWJ7", ur, uc);

   ur = mrsuwj(ubm,256,8);
   uc = jeul(0x80000000);
   ptruw("MRSUWJ8", ur, uc);

   ur = mrsuwj(ubm,ubm,10);
   uc = jcuw(0x100000,0);
   ptruw("MRSUWJ9", ur, uc);

   ur = mrsuwj(0x7FFF0000,ub15,8);
   uc = jcuw(0x7F,0xFE800100);
   ptruw("MRSUWJ10", ur, uc);

   ur = mrsuwj(65536,32768,0);
   uc = jeul(ubm);
   ptruw("MRSUWJ11", ur, uc);

   ur = mrsuwj(131072,32768,0);
   uc = jcuw(1,0);
   ptruw("MRSUWJ12", ur, uc);

/* Test mrsule.
*  New, 10/15/09 - This routine is similar to mrsuwe except both
*  multiplicands are 32-bit.  All shifts are right shifts.  */

   printf("\n===mrsule===\n");

   uz = mrsule(18458,28902,2,1);
   uq = 133368279;
   ptrul("MRSULE1", uz, uq);

   /* > 32 bits, but shift brings it back--no overflow */
   uz = mrsule(0x33FFFF,ub15,8,2);
   uq = 0x33FFCB00;
   ptrul("MRSULE2", uz, uq);

   uz = mrsule(0xF033FFFF,ub31m1,40,3);
   uq = 0x007819FF;
   ptrul("MRSULE3", uz, uq);

   /* Test 32 and zero shifts */
   uz = mrsule(0xF033FFFF,ub31m1,32,4);
   uq = 0x7819FFFD;
   ptrul("MRSULE4", uz, uq);

   uz = mrsule(18458,28902,0,5);
   uq = 533473116;
   ptrul("MRSULE5", uz, uq);

   /* Multiplications involving the biggest positive number */
   uz = mrsule(um1,ub15,16,6);
   uq = 0xFFFEFFFF;
   ptrul("MRSULE6", uz, uq);

   uz = mrsule(ubm,ub15,16,7);
   uq = 0x7FFF8000;
   ptrul("MRSULE7", uz, uq);

   uz = mrsule(ubm,256,8,8);
   uq = 0x80000000;
   ptrul("MRSULE8", uz, uq);

   uz = mrsule(ubm,ubm,31,9);
   uq = ubm;
   ptrul("MRSULE9", uz, uq);

   printf("The next test has a deliberate overflow.\n");
   uz = mrsule(0x7FFF0000,ub15,8,10);
   uq = um1;
   ptrul("MRSULE10", uz, uq);

   uz = mrsule(65536,32768,0,11);
   uq = ubm;
   ptrul("MRSULE11", uz, uq);

   printf("The next test has a deliberate overflow.\n");
   uz = mrsule(131072,32768,0,12);
   uq = um1;
   ptrul("MRSULE12", uz, uq);

/* Test mrsuwe */

   printf("\n===mrsuwe===\n");

   ur = mrsuwe(jcuw(2,5),47,2,1);
   uc = jcuw(23,0x8000003aUL);
   ptruw("MRSUE1", ur, uc);

   ur = mrsuwe(jcuw(b15,u6u),b15,8,2);
   uc = jcuw(0xFFFEFFL,0x66679899UL);
   ptruw("MRSUE2", ur, uc);

   ur = mrsuwe(jcuw(b15,u6u),b15,40,3);
   uc = jcuw(0,0xFFFEFFL);
   ptruw("MRSUE3", ur, uc);

   ur = mrsuwe(jcuw(b15,u6u),b15,0,4);
   uc = jcuw(0xFFFEFF66L,0x67989901UL);
   ptruw("MRSUE4", ur, uc);

   ur = msuwe(jcuw(b15,u6u),b15,-8,5);
   uc = jcuw(0xFFFEFFL,0x66679899UL);
   ptruw("MRSUE5", ur, uc);

   printf("The next test has a deliberate overflow.\n");
   ur = mrsuwe(jcuw(0xFFFF0000L,0x00999901UL),b15,8,6);
   uc = jcuw(um1,um1);
   ptruw("MRSUE6", ur, uc);

   printf("The next test has a deliberate overflow.\n");
   ur = mrsuwe(jcuw(0x7FFF0000L,0x00999901UL),b15,8,7);
   uc = jcuw(um1,um1);
   ptruw("MRSUE7", ur, uc);

   printf("The next test has a deliberate overflow.\n");
   ur = mrsuwe(jcuw(bm,0),b15,12,8);
   uc = jcuw(um1,um1);
   ptruw("MRSUE8", ur, uc);

   /* Following are tests added for mrsuwe V2A */
   ur = mrsuwe(jcuw(0x7FFF0000UL,0x00999901UL),ub15,16,9);
   uc = jcuw(0x7FFE8001UL,0x00999867UL);
   ptruw("MRSUE9", ur, uc);

   ur = mrsuwe(jcuw(ub31,um1),ub31,32,10);
   uc = jcuw(0x3FFFFFFFUL,ub31);
   ptruw("MRSUE10",ur,uc);

   ur = mrsuwe(jcuw(bm,0),bm,33,11);
   uc = jcuw(0x20000000L,0);
   ptruw("MRSUE11",ur,uc);

   ur = mrsuwe(jcuw(bm,0),1,32,12);
   uc = jcuw(0,bm);
   ptruw("MRSUE12",ur,uc);

/* Test mrsrule.
*  New, 10/15/09 - This routine is similar to mrsule except
*  with rounding added.  */

   printf("\n===mrsrule===\n");

   uz = msrule(18458,28902,-2,1);
   uq = 133368279;
   ptrul("MRSRULE1", uz, uq);

   /* > 32 bits, but shift brings it back--no overflow */
   uz = mrsrule(0x33FFFF,ub15,8,2);
   uq = 0x33FFCB00;
   ptrul("MRSRULE2", uz, uq);

   uz = mrsrule(0xF033FFFF,ub31m1,40,3);
   uq = 0x00781A00;
   ptrul("MRSRULE3", uz, uq);

   /* Test 32 and zero shifts */
   uz = mrsrule(0xF033FFFF,ub31m1,32,4);
   uq = 0x7819FFFE;
   ptrul("MRSRULE4", uz, uq);

   uz = mrsrule(18458,28902,0,5);
   uq = 533473116;
   ptrul("MRSRULE5", uz, uq);

   /* Multiplications involving the biggest positive number */
   uz = mrsrule(um1,ub15,16,6);
   uq = 0xFFFEFFFF;
   ptrul("MRSRULE6", uz, uq);

   uz = mrsrule(ubm,u6u,36,7);
   uq = 0x7FB3338;
   ptrul("MRSRULE7", uz, uq);

   uz = mrsrule(ubm,256,8,8);
   uq = 0x80000000;
   ptrul("MRSRULE8", uz, uq);

   uz = mrsrule(ubm,ubm,31,9);
   uq = ubm;
   ptrul("MRSRULE9", uz, uq);

   printf("The next test has a deliberate overflow.\n");
   uz = mrsrule(0x7FFF0000,ub15,8,10);
   uq = um1;
   ptrul("MRSRULE10", uz, uq);

   uz = mrsrule(65536,32768,0,11);
   uq = ubm;
   ptrul("MRSRULE11", uz, uq);

   printf("The next test has a deliberate overflow.\n");
   uz = mrsrule(131072,32768,0,12);
   uq = um1;
   ptrul("MRSRULE12", uz, uq);

   /* Make sure there is no false carry when low order is 0 */
   uz = mrsrule(65536,1048576,32,13);
   uq = 16;
   ptrul("MRSRULE13", uz, uq);

   uz = mrsrule(65536,1048576,34,14);
   uq = 4;
   ptrul("MRSRULE14", uz, uq);

/* Test mrsruwj, similar to mrsrul but 64-bit answers.
*  Overflow is not possible, so overflow checking versions
*  (mrsruwjd, mrsruwje) do not exist and we test mrsruwj.  */

   printf("\n===mrsruwj===\n");

   ur = mrsruwj(18458,28902,2);
   uc = jeul(133368279);
   ptruw("MRSRUWJ1", ur, uc);

   /* > 32 bits, but shift brings it back  */
   ur = mrsruwj(0x33FFFF,ub15,8);
   uc = jeul(0x33FFCB00);
   ptruw("MRSRUWJ2", ur, uc);

   ur = mrsruwj(0xF033FFFF,ub31m1,40);
   uc = jeul(0x00781A00);
   ptruw("MRSRUWJ3", ur, uc);

   /* Test 32 and zero shifts */
   ur = mrsruwj(0xF033FFFF,ub31m1,32);
   uc = jeul(0x7819FFFE);
   ptruw("MRSRUWJ4", ur, uc);

   ur = mrsruwj(18458,28902,0);
   uc = jeul(533473116);
   ptruw("MRSRUWJ5", ur, uc);

   /* Multiplications involving the biggest positive number */
   ur = mrsruwj(um1,ub15,1);
   uc = jcuw(0x7FFF,0x7FFF8001);
   ptruw("MRSRUWJ6", ur, uc);

   ur = mrsruwj(ubm,ub15,16);
   uc = jeul(0x7FFF8000);
   ptruw("MRSRUWJ7", ur, uc);

   ur = mrsruwj(ubm,256,8);
   uc = jeul(0x80000000);
   ptruw("MRSRUWJ8", ur, uc);

   ur = mrsruwj(ubm,ubm,10);
   uc = jcuw(0x100000,0);
   ptruw("MRSRUWJ9", ur, uc);

   ur = mrsruwj(0x7FFF0000,ub15,8);
   uc = jcuw(0x7F,0xFE800100);
   ptruw("MRSRUWJ10", ur, uc);

   ur = mrsruwj(65536,32768,0);
   uc = jeul(ubm);
   ptruw("MRSRUWJ11", ur, uc);

   ur = mrsruwj(131072,32768,0);
   uc = jcuw(1,0);
   ptruw("MRSRUWJ12", ur, uc);

   /* Make sure there is no false carry when low order is 0 */
   ur = mrsruwj(65536,1048576,32);
   uc = jeul(16);
   ptruw("MRSRUWJ13", ur, uc);

   ur = mrsruwj(65536,1048576,34);
   uc = jeul(4);
   ptruw("MRSRUWJ14", ur, uc);

/* Test mrsruwe */

   printf("\n===mrsruwe===\n");

   ur = mrsruwe(jcuw(2,5),47,2,1);
   uc = jcuw(23,0x8000003bUL);
   ptruw("MRSRUE1", ur, uc);

   ur = mrsruwe(jcuw(b15,u6u),b15,8,2);
   uc = jcuw(0xFFFEFFUL,0x66679899UL);
   ptruw("MRSRUE2", ur, uc);

   ur = mrsruwe(jcuw(b15,u6u),b15,39,3);
   uc = jcuw(0,0x1FFFDFFUL);
   ptruw("MRSRUE3", ur, uc);

   ur = mrsruwe(jcuw(b15,u6u),b15,4,4);
   uc = jcuw(0xFFFEFF6UL,0x66798990UL);
   ptruw("MRSRUE4", ur, uc);

   ur = mrsruwe(jcuw(b31,um1),1,4,5);
   uc = jcuw(0x08000000UL,0UL);
   ptruw("MRSRUE5", ur, uc);

   printf("The next test has a deliberate overflow.\n");
   ur = mrsruwe(jcuw(0xFFFF0000L,0x00999901UL),b15,8,6);
   uc = jcuw(um1,um1);
   ptruw("MRSRUE6", ur, uc);

   printf("The next test has a deliberate overflow.\n");
   ur = mrsruwe(jcuw(0x7FFF0000L,0x00999901UL),b15,8,7);
   uc = jcuw(um1,um1);
   ptruw("MRSRUE7", ur, uc);

   printf("The next test has a deliberate overflow.\n");
   ur = mrsruwe(jcuw(bm,0),b15,12,8);
   uc = jcuw(um1,um1);
   ptruw("MRSRUE8", ur, uc);

   ur = mrsruwe(jcuw(0x7FFF0000UL,0x00999901UL),ub15,16,9);
   uc = jcuw(0x7FFE8001UL,0x00999867UL);
   ptruw("MRSRUE9", ur, uc);

   ur = mrsruwe(jcuw(ub31,um1),ub31,32,10);
   uc = jcuw(0x3FFFFFFFUL,ubm);
   ptruw("MRSRUE10",ur,uc);

   ur = mrsruwe(jcuw(bm,0),bm,33,11);
   uc = jcuw(0x20000000L,0);
   ptruw("MRSRUE11",ur,uc);

   ur = mrsruwe(jcuw(bm,0),1,32,12);
   uc = jcuw(0,bm);
   ptruw("MRSRUE12",ur,uc);

   /* Test carry of round into high 32 */
   ur = mrsruwe(jcuw(ub31,um1),256,9,13);
   uc = jcuw(0x40000000UL,0UL);
   ptruw("MRSRUE13", ur, uc);

   /* Make sure there is no false carry when low order is 0 */
   ur = mrsruwe(jeul(65536),1048576,32,14);
   uc = jeul(16);
   ptruw("MRSRUWE14", ur, uc);

   ur = mrsruwe(jeul(65536),1048576,34,15);
   uc = jeul(4);
   ptruw("MRSRUWE15", ur, uc);

   printf("\nEnd 64-bit arithmetic tests, got %ld errors.\n", errcnt);
   return 0;
   } /* End main program */
