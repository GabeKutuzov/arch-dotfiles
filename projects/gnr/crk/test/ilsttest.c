/***********************************************************************
*                              ILSTTEST                                *
*                     Iteration List Test Program                      *
*                                                                      *
*  Each feature of the ilst routines is tested by reading an input     *
*  card, testing that the stored ilst is correct, then checking that   *
*  the iterator returns the correct values from that list.             *
*                                                                      *
*  V1A, 06/10/00, G.N. Reeke - New program                             *
*  V1B, 12/26/00, GNR - Add FIRST, LAST, RANDOM, SEED and ilstchk()    *
*  V1C, 01/06/01, GNR - Add ALL                                        *
*  V1D, 09/15/01, GNR - Add ilstitct() tests                           *
***********************************************************************/

/* Include standard library functions */

#define MAIN
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "sysdef.h"
#include "rocks.h"
#include "rkxtra.h"
#include "rkilst.h"

ilst *pil;     /* Global pointer to current iteration list */
long seed;     /* Random number seed */

/***********************************************************************
*                                                                      *
*                      ABEXIT, ABEXITM, ABEXITME                       *
*                                                                      *
*      Special test versions to print message but not terminate        *
*                                                                      *
***********************************************************************/

void abexit(int code) {

   convrt("(WP1,'0***Abexit called with code ',J0I5)", &code, NULL);
   } /* End abexit() */

void abexitm(int code, char *emsg) {

   convrt("(WP1,'0***Abexitm called with code ',J0I5,' and text'/"
      "J0A128)", &code, emsg, NULL);
   } /* End abexitm() */

void abexitme(int code, char *emsg) {

   cryout(RK_P1, ssprintf(NULL, "0***Abexitme called with code %d "
      "and msg", code), RK_LN2, emsg, RK_CCL, NULL);
   } /* End abexitme() */

/***********************************************************************
*                                                                      *
*                               DOTEST                                 *
*                                                                      *
*  Read a control card with ilstread(), verify that correct list is    *
*  returned.                                                           *
*                                                                      *
***********************************************************************/

static void dotest(int it, int idop, int ibase,
      int ix, ilstitem *expect) {

   ilstitem *pit;
   int i,ie;

   if (cryin() == NULL) {
      cryout(RK_P1,"0***Premature end of test input.",
         RK_LN2+RK_FLUSH, NULL);
      exit(1);
      }
   cdprt1(RK.last);
   cdscan(RK.last, 1, DFLT_MAXLEN, RK_WDSKIP);
   pil = ilstread(pil, idop, ibase, seed);

   /* If there is a comma after the right paren, scan again */
   if (!(RKC.f & TER)) pil = ilstread(pil, idop, ibase, seed);

   /* If this is test 43, 44, or 45, call ilstchk() */
   if (it >= 43 && it <= 45)
      ilstchk(pil, 10, "Test 43 or 45 list");

   if (pil) {
      ie = (int)pil->nusm1 + 1;
      pit = pil->pidl;
      }
   else {
      ie = 0;
      pit = NULL;
      }
   if (ie != ix) {
      convrt("(WP1,'0***Test ',J0I4,2H, J0I6,' items expected, ',"
         "J1I6,'found.')", &it, &ix, &ie, NULL);
      exit(2);
      }
   for (i=0; i<ie; i++) if (pit[i] != expect[i]) {
      convrt("(WP1,'0***Test ',J0I4,', item ',J0I4,2H, J1IL9,"
         "'expected, ',J1IL9,'found.')", &it, &i, expect+i,
         pit+i, NULL);
      exit(3);
      } /* End test loop */

   } /* End dotest() */

/***********************************************************************
*                        ILSTTEST MAIN PROGRAM                         *
***********************************************************************/

int main(int argn, char *argc[]) {

   ilstitem y;
   iter ti;
   long iv,iw;
   int j;

   settit("TITLE Test ROCKS ilstread and iterator routines");
   cdunit("ilsttest.inp");

   seed = 1037;

/*---------------------------------------------------------------------*
*                       Basic interpreter tests                        *
*---------------------------------------------------------------------*/

/* Test 1:  A basic NEW list with 3 integer values.  Test that the
*           iterator works, starting at zero and at a higher value.
*           Error tests:  NEW not first in list.  */

   {  ilstitem x[] = { 7,13,22 };
      dotest(1, IL_NEW, 0, 3, x);
      if (pil->evry != 0) {
         convrt("(WP1,'0***Test 1, evry set to ',J0IL6,"
            "', should be 0')", &pil->evry, NULL);
         exit(1);
         }
      if (pil->frst != 0) {
         convrt("(WP1,'0***Test 1, frst set to ',J0IL6,"
            "', should be 0')", &pil->frst, NULL);
         exit(1);
         }
      if (pil->last != 0) {
         convrt("(WP1,'0***Test 1, last set to ',J0IL6,"
            "', should be 0')", &pil->last, NULL);
         exit(1);
         }
      if (pil->rand != 0) {
         convrt("(WP1,'0***Test 1, rand set to ',J0IL6,"
            "', should be 0')", &pil->rand, NULL);
         exit(1);
         }
      iv = (long)pil->nallo;
      iw = (long)IL_ILSZ;
      if (iv != iw) {
         convrt("(WP1,'0***Test 1, nallo set to ',J0IL6,"
            "', should be ',J0IL6)", &iv, &iw, NULL);
         exit(1);
         }
      if (ilstchk(pil, 100, "Test 1 list")) {
         cryout(RK_P1,"0***Test 1, ilstchk() returned error "
            "when list was actually OK.", RK_LN2+RK_FLUSH, NULL);
         exit(1);
         }
      cryout(RK_P1, "0Test 1, should get error, list item 22 "
         "exceeds block top 9:", RK_LN2, NULL);
      if (!ilstchk(pil,  10, "Test 1 list")) {
         cryout(RK_P1,"0***Test 1, ilstchk() returned OK "
            "when list was actually bad.", RK_LN2+RK_FLUSH, NULL);
         exit(1);
         }
      y = ilsthigh(pil);
      if (y != x[2]) {
         convrt("(WP1,'0***Test 1, ilsthigh returned ',J0IL6,"
            "', should be ',J0IL6)", &y, x+2, NULL);
         exit(1);
         }
      ilstset(&ti, pil, 0);
      if (ilsttest(pil, 5)) {
         cryout(RK_P1,"0***Test 1, 5 found in list, "
            "should not be.", RK_LN2+RK_FLUSH, NULL);
         exit(1);
         }
      if (!ilsttest(pil, 7)) {
         cryout(RK_P1,"0***Test 1, 7 not found in list, "
            "should be.", RK_LN2+RK_FLUSH, NULL);
         exit(1);
         }
      for (j=0; j<3; j++) {
         iv = ilstnow(&ti);
         if (iv != (long)x[j]) {
            convrt("(WP1,'0***Test 1, ilstnow returned ',J1IL6,"
               "'for item ',J0I4,', should be ',J0I6)", &iv,
               &j, x+j, NULL);
            exit(1);
            }
         iv = ilstiter(&ti);
         if (iv != (long)x[j]) {
            convrt("(WP1,'0***Test 1, ilstiter returned ',J1IL6,"
               "'for item ',J0I4,', should be ',J0I6)", &iv,
               &j, x+j, NULL);
            exit(1);
            }
         }
      iv = ilstiter(&ti);
      if (iv != -1) {
         convrt("(WP1,'0***Test 1, ilstiter should have finished"
            " after 3 calls, instead returned ',J0IL6)", &iv, NULL);
         exit(1);
         }

      ilstset(&ti, pil, x[1]);
      for (j=1; j<3; j++) {
         iv = ilstiter(&ti);
         if (iv != (long)x[j]) {
            convrt("(WP1,'0***Test 1A, ilstiter returned ',J1IL6,"
               "'for item ',J0I4,', should be ',J0I6)", &iv,
               &j, x+j, NULL);
            exit(1);
            }
         }
      iv = ilstiter(&ti);
      if (iv != -1) {
         convrt("(WP1,'0***Test 1A, ilstiter should have finished"
            " after 2 calls, instead returned ',J0IL6)", &iv, NULL);
         exit(1);
         }

      cryout(RK_P1, "0Test 1E1, should get error, "
         "NEW not first in list:", RK_LN2, NULL);
      y = 15;
      dotest(1, IL_NEW, 0, 1, &y);

      } /* End test 1 local scope */

/* Test 2:  Get rid of a list with the OFF keyword.  Default
*           key is DEL, but key in input file is NEW.  Error
*           tests:  Punctuation, OFF not only item in list.  */

      dotest(2, IL_DEL, 0, 0, NULL);
      if (pil) {
         cryout(RK_P1, "0Test 2, returned a list, should be NULL.",
            RK_LN2+RK_FLUSH, NULL);
         exit(1);
         }

      cryout(RK_P1, "0Test 2E1, should get error, "
         "data not allowed in OFF mode:", RK_LN2, NULL);
      dotest(2, IL_NEW, 0, 0, NULL);

      cryout(RK_P1, "0Test 2E2, should get error, "
         "bad punctuation:", RK_LN2, NULL);
      dotest(2, IL_NEW, 0, 0, NULL);

      cryout(RK_P1, "0Test 2E3, should get error, "
         "data not allowed in OFF mode:", RK_LN2, NULL);
      dotest(2, IL_OFF, 0, 0, NULL);

/* Test 3:  Rebuilt the list from test 1, then remove it
*           with a NULL list.  Error tests:  Punctuation
*           error, empty field not the first field.  */

   dotest(3, IL_NEW|IL_ELOK, 0, 0, NULL);
   if (pil) {
      cryout(RK_P1, "0Test 3, returned a list, should be NULL.",
         RK_LN2+RK_FLUSH, NULL);
      exit(1);
      }

   cryout(RK_P1, "0Test 3E1, should get 3 errors, "
      "bad nesting, unmatched paren, ilst required:", RK_LN2, NULL);
   dotest(3, IL_NEW, 0, 0, NULL);

   {  ilstitem x[] = { 7,13,22 };
      cryout(RK_P1, "0Test 3E2, should get error, "
         "required field missing:", RK_LN2, NULL);
      dotest(3, IL_NEW, 0, 3, x);
      } /* End test 3E2 local scope */

/* Test 4:  Use the MXC option to allocate space for a list
*           of 10 items, put 3 in, then check list length.
*           Error tests:  Punctuation, MXC not first field,
*           negative value entered.  */

   {  ilstitem x[] = { 7,13,22 };
      dotest(4, IL_NEW, 0, 3, x);
      iv = (long)pil->nallo;
      if (iv != 10) {
         convrt("(WP1,'0***Test 4, nallo set to ',J0IL6,"
            "', should be 10.')", &iv, NULL);
         exit(1);
         }

      cryout(RK_P1, "0Test 4E1, should get error, "
         "bad punctuation:", RK_LN2, NULL);
      dotest(4, IL_NEW, 0, 3, x);

      cryout(RK_P1, "0Test 4E2, should get error, "
         "MXC not first option in list:", RK_LN2, NULL);
      dotest(4, IL_NEW, 0, 3, x);

      cryout(RK_P1, "0Test 4E3, should get error, "
         "Bad numeric value for MXC:", RK_LN2, NULL);
      dotest(4, IL_NEW, 0, 3, x);

      } /* End test 4 local scope */

/* Test 5:  Basic ADD test:  with same list as test 1, add one
*           additional number in middle of list.  Error tests:
*           ADD is last item in list, ADD is not in parens,
*           ADD is repeated.  Test 5E3 also tests that when a
*           single item is added adjacent to an existing single
*           item, a simple range is created (same code handles
*           cases above and below--one test should suffice).  */

   {  ilstitem x[] = { 7,13,16,22 };
      ilstitem x1[] = { 7,13,22 };
      ilstitem x2[] = { 7,13,14+IL_REND,18,22 };
      dotest(5, IL_NEW, 0, 4, x);

      y = ilsthigh(pil);
      if (y != x[3]) {
         convrt("(WP1,'0***Test 5, ilsthigh returned ',J0IL6,"
            "', should be ',J0IL6)", &y, x+3, NULL);
         exit(1);
         }

      cryout(RK_P1, "0Test 5E1, should get error, "
         "required field missing:", RK_LN2, NULL);
      dotest(5, IL_NEW, 0, 3, x1);

      cryout(RK_P1, "0Test 5E2, should get error, "
         "parens required:", RK_LN2, NULL);
      dotest(5, IL_NEW, 0, 3, x1);

      cryout(RK_P1, "0Test 5E3, should get warning, "
         "mode keyword is repeated:", RK_LN2, NULL);
      dotest(5, IL_NEW, 0, 5, x2);

      } /* End test 5 local scope */

/* Test 6:  Basic DEL test:  with same list as test 1, delete
*           one value.  Error tests:  Same as for ADD.  */

   {  ilstitem x[] = { 7,22 };
      ilstitem x1[] = { 7,13,22 };
      dotest(6, IL_NEW, 0, 2, x);

      y = ilsthigh(pil);
      if (y != x[1]) {
         convrt("(WP1,'0***Test 6, ilsthigh returned ',J0IL6,"
            "', should be ',J0IL6)", &y, x+1, NULL);
         exit(1);
         }

      cryout(RK_P1, "0Test 6E1, should get error, "
         "required field missing:", RK_LN2, NULL);
      dotest(6, IL_NEW, 0, 3, x1);

      cryout(RK_P1, "0Test 6E2, should get error, "
         "parens required:", RK_LN2, NULL);
      dotest(6, IL_NEW, 0, 3, x1);

      cryout(RK_P1, "0Test 6E3, should get warning, "
         "mode keyword is repeated:", RK_LN2, NULL);
      dotest(6, IL_NEW, 0, 1, x1+1);

      } /* End test 6 local scope */

/* Test 7:  EVERY option:  Request EVERY option and be sure
*           iteration is correct.  Error tests:  EVERY has no
*           value, EVERY is not in parens, single item at top
*           of list is greater than EVERY increment, range at
*           top of list ends above EVERY increment.  */

   {  ilstitem x[] = { 7,13,22,107,113,122 };
      ilstitem x1[] = { 7,13,122 };
      ilstitem x2[] = { 7,121|IL_REND };
      dotest(7, IL_NEW, 1, 3, x);
      iw = 100L | (1L<<IL_BITS);
      if (pil->evry != iw) {
         convrt("(WP1,'0***Test 7, evry set to ',Z8,"
            "', should be ',Z8)", &pil->evry, &iw, NULL);
         exit(1);
         }
      y = ilsthigh(pil);
      ilstset(&ti, pil, 0);
      if (y != x[2]) {
         convrt("(WP1,'0***Test 7, ilsthigh returned ',J0IL6,"
            "', should be ',J0IL6)", &y, x+2, NULL);
         exit(1);
         }
      for (j=0; j<6; j++) {
         iv = ilstiter(&ti);
         if (iv != (long)x[j]) {
            convrt("(WP1,'0***Test 7, ilstiter returned ',J1IL6,"
               "'for item ',J0I4,', should be ',J0I6)", &iv,
               &j, x+j, NULL);
            exit(1);
            }
         }

      cryout(RK_P1, "0Test 7E1, should get error, "
         "required field missing:", RK_LN2, NULL);
      dotest(7, IL_NEW, 1, 3, x);

      cryout(RK_P1, "0Test 7E2, should get error, parens required:",
         RK_LN2, NULL);
      dotest(7, IL_NEW, 1, 3, x);

      cryout(RK_P1, "0Test 7E3, should get error, item in list "
         "is greater than EVERY value:", RK_LN2, NULL);
      dotest(7, IL_NEW, 1, 3, x1);

      cryout(RK_P1, "0Test 7E4, should get error, range in list "
         "ends above EVERY value:", RK_LN2, NULL);
      dotest(7, IL_NEW, 1, 2, x2);

      } /* End test 7 local scope */

/*---------------------------------------------------------------------*
*                              ADD tests                               *
*---------------------------------------------------------------------*/

/* Test 8:  ADD a simple range after between base items, using
*           default ADD mode.  Be sure the iterator can handle a
*           range of more than 2 items.  Error checks:  Negative
*           value first in list, punctuation, minus sign without
*           a value, two negs in succession, range top below
*           range bottom.  */

   {  ilstitem x[]  = { 7,13,15,18|IL_REND,22 };
      ilstitem x1[] = { 7,13,22 };
      ilstitem x2[] = { 7,13,18,22 };
      long    xit[] = { 7,13,15,16,17,18,22 };
      dotest(8, IL_ADD, 1, 5, x);
      y = ilsthigh(pil);
      if (y != x[4]) {
         convrt("(WP1,'0***Test 8, ilsthigh returned ',J0IL6,"
            "', should be ',J0IL6)", &y, x+4, NULL);
         exit(1);
         }
      ilstset(&ti, pil, 0);
      for (j=0; j<7; j++) {
         iv = ilstiter(&ti);
         if (iv != xit[j]) {
            convrt("(WP1,'0***Test 8, ilstiter returned ',J1IL6,"
               "'for item ',J0I4,', should be ',J0I6)", &iv,
               &j, xit+j, NULL);
            exit(1);
            }
         }
      iv = ilstiter(&ti);
      if (iv != -1) {
         convrt("(WP1,'0***Test 8, ilstiter should have finished"
            " after 7 calls, instead returned ',J0IL6)", &iv, NULL);
         exit(1);
         }

      cryout(RK_P1, "0Test 8E1, should get error, "
         "negative value requires a range start:", RK_LN2, NULL);
      dotest(8, IL_ADD, 1, 4, x2);

      cryout(RK_P1, "0Test 8E2, should read just 15-18 OK,"
         " caller scans 3 next:",
         RK_LN2, NULL);
      dotest(8, IL_ADD, 1, 5, x);

      cryout(RK_P1, "0Test 8E3, should get error, "
         "bad punctuation:", RK_LN2, NULL);
      dotest(8, IL_ADD, 1, 3, x1);

      cryout(RK_P1, "0Test 8E4, should get 2 errors, bad "
         "numeric value:", RK_LN2, NULL);
      dotest(8, IL_ADD, 1, 3, x1);

      cryout(RK_P1, "0Test 8E5, should get 2 errors, range "
         "has too many tops:", RK_LN2, NULL);
      dotest(8, IL_ADD, 1, 3, x1);

      cryout(RK_P1, "0Test 8E6, should get 2 errors, range "
         "bottom is greater than top:", RK_LN2, NULL);
      dotest(8, IL_ADD, 1, 3, x1);

      } /* End test 8 local scope */

/* Test 9:  ADD a range with stride between the base items.
*           Error checks:  Sim. to test 8, plus two strides
*           in one range, stride without a range end.  */

   {  ilstitem x[]  = { 7,13,16,4|IL_INCR,24|IL_REIN,32 };
      ilstitem x1[] = { 7,13,22 };
      ilstitem x2[] = { 7,13,16,22 };
      long    xit[] = { 7,13,16,20,24,32 };
      dotest(9, IL_ADD, 1, 6, x);
      y = ilsthigh(pil);
      if (y != x[5]) {
         convrt("(WP1,'0***Test 9, ilsthigh returned ',J0IL6,"
            "', should be ',J0IL6)", &y, x+5, NULL);
         exit(1);
         }
      if (ilsttest(pil, 19)) {
         cryout(RK_P1,"0***Test 9, 19 found in list, "
            "should not be.", RK_LN2+RK_FLUSH, NULL);
         exit(1);
         }
      if (!ilsttest(pil, 20)) {
         cryout(RK_P1,"0***Test 9, 20 not found in list, "
            "should be.", RK_LN2+RK_FLUSH, NULL);
         exit(1);
         }
      ilstset(&ti, pil, 0);
      for (j=0; j<6; j++) {
         iv = ilstiter(&ti);
         if (iv != xit[j]) {
            convrt("(WP1,'0***Test 9, ilstiter returned ',J1IL6,"
               "'for item ',J0I4,', should be ',J0I6)", &iv,
               &j, xit+j, NULL);
            exit(1);
            }
         }
      iv = ilstiter(&ti);
      if (iv != -1) {
         convrt("(WP1,'0***Test 9, ilstiter should have finished"
            " after 6 calls, instead returned ',J0IL6)", &iv, NULL);
         exit(1);
         }

      cryout(RK_P1, "0Test 9E1, should get 2 errors, "
         "end and stride require a range start:",
         RK_LN2, NULL);
      dotest(9, IL_ADD, 1, 4, x2);

      cryout(RK_P1, "0Test 9E2, should get 2 errors, "
         "bad punct and stride requires range start:",
         RK_LN2, NULL);
      dotest(9, IL_ADD, 1, 3, x1);

      cryout(RK_P1, "0Test 9E3, should get 2 errors, bad "
         "numeric val and stride requires range start:",
         RK_LN2, NULL);
      dotest(9, IL_ADD, 1, 3, x1);

      cryout(RK_P1, "0Test 9E4, should get error, range "
         "has too many strides:", RK_LN2, NULL);
      dotest(9, IL_ADD, 1, 3, x1);

      cryout(RK_P1, "0Test 9E5, should get error, range "
         "has no top:", RK_LN2, NULL);
      dotest(9, IL_ADD, 1, 3, x1);

      } /* End test 9 local scope */

/* Test 10: Add a range.  Check that if explicit stride is 1,
*           a simple range is created.  */

   {  ilstitem x[] = { 7,13,16,22|IL_REND };
      dotest(10, IL_NEW, 1, 4, x);
      } /* End test 10 local scope */

/* Test 11: ADD keyword used but there is no existing list.
*           Just create one as if keyword was "NEW"  */

   freeilst(pil), pil = NULL;
   {  ilstitem x[] = { 7,13,22 };
      dotest(11, IL_ADD, 1, 3, x);
      } /* End test 11 local scope */

/* Test 12: ADD a single item, a simple range, and a range
*           with stride below all existing items in a list.  */

   {  ilstitem xa[] = { 5,17,23,32 };
      ilstitem xb[] = { 5,10|IL_REND,17,23,32 };
      ilstitem xc[] = { 5,3|IL_INCR,17|IL_REIN,23,32 };
      long   xitb[] = { 7,8,9,10,17,23,32 };
      dotest(12, IL_ADD, 1, 4, xa);
      dotest(12, IL_ADD, 1, 5, xb);
      ilstset(&ti, pil, 7);
      for (j=0; j<7; j++) {
         iv = ilstiter(&ti);
         if (iv != xitb[j]) {
            convrt("(WP1,'0***Test 12, ilstiter returned ',J1IL6,"
               "'for item ',J0I4,', should be ',J0I6)", &iv,
               &j, xitb+j, NULL);
            exit(1);
            }
         }
      iv = ilstiter(&ti);
      if (iv != -1) {
         convrt("(WP1,'0***Test 12, ilstiter should have finished"
            " after 7 calls, instead returned ',J0IL6)", &iv, NULL);
         exit(1);
         }
      dotest(12, IL_ADD, 1, 5, xc);
      } /* End test 12 local scope */

/* Test 13: ADD a single item, a simple range, and a range
*           with stride above a single item in a list.  */

   {  ilstitem xa[] = { 17,23,32,55 };
      ilstitem xb[] = { 17,23,32,55,60|IL_REND };
      ilstitem xc[] = { 17,23,32,55,3|IL_INCR,64|IL_REIN };
      dotest(13, IL_ADD, 1, 4, xa);
      dotest(13, IL_ADD, 1, 5, xb);
      dotest(13, IL_ADD, 1, 6, xc);
      } /* End test 13 local scope */

/* Test 14: ADD a range adjacent to an existing single item.  */

   {  ilstitem x[] = { 17,23,26|IL_REND,32 };
      dotest(14, IL_NEW, 1, 4, x);
      } /* End test 14 local scope */

/* Test 15: ADD a range with stride adjacent to an existing
*           single item.  */

   {  ilstitem x[] = { 17,23,3|IL_INCR,32|IL_REIN,42 };
      dotest(15, IL_NEW, 1, 5, x);
      } /* End test 15 local scope */

/* Test 16: ADD a range that overlaps an existing range.  */

   {  ilstitem x[] = { 17,23,35|IL_REND };
      dotest(16, IL_NEW, 1, 3, x);
      } /* End test 16 local scope */

/* Test 17: ADD a range that overlaps an existing range with
*           stride, such that the existing range is
*           terminated just below the new range.  */

   {  ilstitem x[] = { 17,4|IL_INCR,25|IL_REIN,27,36|IL_REND };
      long   xit[] = { 17,21,25,27,28,29,30,31,32,33,34,35,36 };
      dotest(17, IL_NEW, 1, 5, x);
      ilstset(&ti, pil, 0);
      for (j=0; j<13; j++) {
         iv = ilstiter(&ti);
         if (iv != xit[j]) {
            convrt("(WP1,'0***Test 17, ilstiter returned ',J1IL6,"
               "'for item ',J0I4,', should be ',J0I6)", &iv,
               &j, xit+j, NULL);
            exit(1);
            }
         }
      iv = ilstiter(&ti);
      if (iv != -1) {
         convrt("(WP1,'0***Test 17, ilstiter should have finished"
            " after 13 calls, instead returned ',J0IL6)", &iv, NULL);
         exit(1);
         }
      } /* End test 17 local scope */

/* Test 18: ADD a simple range internal to a range with
*           stride, such that the range with stride must be
*           broken into two ranges.  */

   {  ilstitem x[] = { 7,4|IL_INCR,15|IL_REIN,16,21|IL_REND,
         23,4|IL_INCR,31|IL_REIN };
      dotest(18, IL_NEW, 1, 8, x);
      } /* End test 18 local scope */

/* Test 19: ADD a range with stride that starts inside an
*           existing simple range.  The starting value of
*           the new range should be adjusted above the old.  */

   {  ilstitem x[] = { 7,15|IL_REND,18,4|IL_INCR,22|IL_REIN };
      dotest(19, IL_NEW, 1, 5, x);
      } /* End test 19 local scope */

/* Test 20: ADD a range with stride entirely inside an
*           existing simple range.  No change in the list.  */

   {  ilstitem x[] = { 7,15|IL_REND };
      cryout(RK_P1, "0Test 20, should get warning, "
         "redundant addition ignored.", RK_LN2, NULL);
      dotest(20, IL_NEW, 1, 2, x);
      } /* End test 20 local scope */

/* Test 21: ADD a range with stride in phase with an existing
*           range with stride.  The two ranges should merge.  */

   {  ilstitem x[] = { 17,4|IL_INCR,37|IL_REIN };
      dotest(21, IL_NEW, 1, 3, x);
      } /* End test 21 local scope */

/* Test 22: ADD a range with stride above an existing range with
*           stride where the old stride is a multiple of the
*           new stride and the ranges are in phase.  The old
*           range is partially absorbed into the new one.  */

   {  ilstitem x[] = { 7,8|IL_INCR,15|IL_REIN,
         23,4|IL_INCR,39|IL_REIN };
      dotest(22, IL_NEW, 1, 6, x);
      } /* End test 22 local scope */

/* Test 23: Same as test 22 except the new range overlaps below
*           the old one.  */

   {  ilstitem x[] = { 9,4|IL_INCR,25|IL_REIN,
         33,8|IL_INCR,41|IL_REIN };
      dotest(23, IL_NEW, 1, 6, x);
      } /* End test 23 local scope */

/* Test 24: Old and new ranges with stride are incompatible.
*           The ranges should split into a series of individual
*           items.   */

   {  ilstitem x[] = { 7,10,12,14,17,18,4|IL_INCR,26|IL_REIN,27 };
      long   xit[] = { 7,10,12,14,17,18,22,26,27 };
      dotest(24, IL_NEW, 1, 9, x);
      ilstset(&ti, pil, 0);
      for (j=0; j<9; j++) {
         iv = ilstiter(&ti);
         if (iv != xit[j]) {
            convrt("(WP1,'0***Test 24, ilstiter returned ',J1IL6,"
               "'for item ',J0I4,', should be ',J0I6)", &iv,
               &j, xit+j, NULL);
            exit(1);
            }
         }
      iv = ilstiter(&ti);
      if (iv != -1) {
         convrt("(WP1,'0***Test 24, ilstiter should have finished"
            " after 9 calls, instead returned ',J0IL6)", &iv, NULL);
         exit(1);
         }
      } /* End test 24 local scope */

/*---------------------------------------------------------------------*
*                              DEL tests                               *
*---------------------------------------------------------------------*/

/* Test 25: Error test: Attempt to delete a range with stride. */

   {  ilstitem x[] = { 7,13,22 };
      cryout(RK_P1, "0Test 25E1, should get error, "
         "can't delete a range with stride:", RK_LN2, NULL);
      dotest(25, IL_NEW, 0, 3, x);
      } /* End test 25 local scope */

/* Test 26: Attempt to delete something and there is no existing
*           list.  Should generate a warning.  */

   cryout(RK_P1, "0Test 26E1, should get warning, "
      "there is nothing to delete:", RK_LN2, NULL);
   freeilst(pil), pil = NULL;
   dotest(26, IL_ADD, 1, 0, NULL);

/* Test 27: Delete a single item from the bottom of a simple
*           range.  */

   {  ilstitem x[] = { 18,22|IL_REND };
      dotest(27, IL_NEW, 1, 2, x);
      } /* End test 27 local scope */

/* Test 28: Delete a single item from the top of a simple range.  */

   {  ilstitem x[] = { 17,21|IL_REND };
      dotest(28, IL_NEW, 1, 2, x);
      } /* End test 28 local scope */

/* Test 29: Delete a range that includes a smaller range.  */

   {  ilstitem x[] = { 22 };
      dotest(29, IL_NEW, 1, 1, x);
      } /* End test 29 local scope */

/* Test 30: Same as test 29 but the existing range has stride.  */

   {  ilstitem x[] = { 22 };
      dotest(30, IL_NEW, 1, 1, x);
      } /* End test 30 local scope */

/* Test 31: The range to be deleted overlaps above an existing
*           range.  The range is truncated.  */

   {  ilstitem x[] = { 7,10,17|IL_REND };
      dotest(31, IL_NEW, 1, 3, x);
      } /* End test 31 local scope */

/* Test 32: Same as test 31 but the existing range has stride.  */

   {  ilstitem x[] = { 7,3|IL_INCR,16|IL_REIN };
      dotest(32, IL_NEW, 1, 3, x);
      } /* End test 32 local scope */

/* Test 33: The range to be deleted overlaps below an existing
*           range.  The range is truncated.  */

   {  ilstitem x[] = { 13,20|IL_REND };
      dotest(33, IL_NEW, 1, 2, x);
      } /* End test 33 local scope */

/* Test 34: Same as test 33 but the existing range has stride.  */

   {  ilstitem x[] = { 16,3|IL_INCR,19|IL_REIN };
      dotest(34, IL_NEW, 1, 3, x);
      } /* End test 34 local scope */

/* Test 35: The deletion range is entirely within a simple range.
*           The existing range is divided into two segments.  */

   {  ilstitem x[] = { 7,10,13|IL_REND,20,30|IL_REND };
      dotest(35, IL_NEW, 1, 5, x);
      } /* End test 35 local scope */

/* Test 36: Same as test 35 but the existing range has stride.  */

   {  ilstitem x[] = { 7,3|IL_INCR,13|IL_REIN,
         22,3|IL_INCR,28|IL_REIN };
      dotest(36, IL_NEW, 1, 6, x);
      } /* End test 36 local scope */

/*---------------------------------------------------------------------*
*                         Consolidation tests                          *
*---------------------------------------------------------------------*/

/* Test 37: After a deletion, a range is reduced to a single item.  */

   {  ilstitem x[] = { 7,10 };
      dotest(37, IL_NEW, 1, 2, x);
      } /* End test 37 local scope */

/* Test 38: After a deletion, a range is reduced to a single item
*           which then consolidates into a range with stride below it.
*/

   {  ilstitem x[] = { 7,3|IL_INCR,19|IL_REIN };
      dotest(38, IL_NEW, 1, 3, x);
      } /* End test 38 local scope */

/* Test 39: Same as test 38, but the range with stride is above the
*           item to be consolidated.  */

   {  ilstitem x[] = { 14,3|IL_INCR,26|IL_REIN };
      dotest(39, IL_NEW, 1, 3, x);
      } /* End test 39 local scope */

/* Test 40: After a deletion, a range with stride is reduced to
*           a single item.  */

   {  ilstitem x[] = { 17,33 };
      dotest(40, IL_NEW, 1, 2, x);
      } /* End test 40 local scope */

/* Test 41: After a deletion, there is nothing left.  */

   dotest(41, IL_NEW, 1, 0, NULL);

/* Test 42: A single item is added that is identical to a single
*           item already on the list.  Should give a warning.  */

   {  ilstitem x[] = { 7,13,23 };
      cryout(RK_P1, "0Test 42, should get warning, "
         "redundant addition ignored:", RK_LN2, NULL);
      dotest(42, IL_NEW, 1, 3, x);
      } /* End test 42 local scope */

/* Test 43: First 3 + last 2 of every 10, origin 1  */

   {  ilstitem x[] = { 1,2,3,9,10,11,12,13,19,20 };
      ilstitem x1[] = { 1, 3|IL_REND, 9, 10|IL_REND };
      dotest(43, IL_NEW, 1, 4, x1);
      iw = 10L | (1L<<IL_BITS);
      if (pil->evry != iw) {
         convrt("(WP1,'0***Test 43, evry set to ',Z8,"
            "', should be ',Z8)", &pil->evry, &iw, NULL);
         exit(1);
         }
      y = ilsthigh(pil);
      ilstset(&ti, pil, 0);
      if (y != x[4]) {
         convrt("(WP1,'0***Test 43, ilsthigh returned ',J0IL6,"
            "', should be ',J0IL6)", &y, x+4, NULL);
         exit(1);
         }
      for (j=0; j<10; j++) {
         iv = ilstiter(&ti);
         if (iv != (long)x[j]) {
            convrt("(WP1,'0***Test 43, ilstiter returned ',J1IL6,"
               "'for item ',J0I4,', should be ',J0I6)", &iv,
               &j, x+j, NULL);
            exit(1);
            }
         }
      } /* End test 43 local scope */

/* Test 44: 5 + Random 3 of every 10, seed 137, origin 0.  */

   {  ilstitem x[] = { 0,5,7,9,10,15,17,19 };
      dotest(44, IL_NEW, 0, 4, x);
      iw = 10L;
      if (pil->evry != iw) {
         convrt("(WP1,'0***Test 44, evry set to ',Z8,"
            "', should be ',Z8)", &pil->evry, &iw, NULL);
         exit(1);
         }
      y = ilsthigh(pil);
      ilstset(&ti, pil, 0);
      if (y != x[3]) {
         convrt("(WP1,'0***Test 44, ilsthigh returned ',J0IL6,"
            "', should be ',J0IL6)", &y, x+3, NULL);
         exit(1);
         }
      for (j=0; j<8; j++) {
         iv = ilstiter(&ti);
         if (iv != (long)x[j]) {
            convrt("(WP1,'0***Test 44, ilstiter returned ',J1IL6,"
               "'for item ',J0I4,', should be ',J0I6)", &iv,
               &j, x+j, NULL);
            exit(1);
            }
         }
      } /* End test 44 local scope */

/* Test 45:  ALL option.  */

   {  ilstitem x[] = { 2,3,4,5,6,7,8,9 };
      ilstitem x1[] = { 0, 9|IL_REND };
      ilstitem x2[] = { 0, 5|IL_REND };
      dotest(45, IL_NEW, 0, 2, x1);
      ilstset(&ti, pil, 2);
      for (j=0; j<8; j++) {
         iv = ilstiter(&ti);
         if (iv != (long)x[j]) {
            convrt("(WP1,'0***Test 45, ilstiter returned ',J1IL6,"
               "'for item ',J0I4,', should be ',J0I6)", &iv,
               &j, x+j, NULL);
            exit(1);
            }
         }

      cryout(RK_P1, "0Test 45E1, should get error, "
         "more than one of exclusive set:", RK_LN2, NULL);
      dotest(45, IL_NEW, 0, 2, x2);

      } /* End test 45 local scope */

/* Test 46:  ilstitct() without EVERY */

   {  ilstitem x[] = { 2,4,3|IL_INCR,16|IL_REIN,19,22|IL_REND,25 };
      dotest(46, IL_NEW, 0, 7, x);
      iv = ilstitct(pil, 25), iw = 10;
      if (iv != iw) {
         convrt("(WP1,'0***Test 46(1), ilstitct returned ',J1IL6,"
            "'should be ',J0IL6)", &iv, &iw, NULL);
         exit(1);
         }
      iv = ilstitct(pil, 13), iw = 4;
      if (iv != iw) {
         convrt("(WP1,'0***Test 46(2), ilstitct returned ',J1IL6,"
            "'should be ',J0IL6)", &iv, &iw, NULL);
         exit(1);
         }
      iv = ilstitct(pil, 17), iw = 6;
      if (iv != iw) {
         convrt("(WP1,'0***Test 46(3), ilstitct returned ',J1IL6,"
            "'should be ',J0IL6)", &iv, &iw, NULL);
         exit(1);
         }
      } /* End test 46 local scope */

/* Test 47:  ilstitct() with EVERY, base 0 */

   {  ilstitem x[] = { 2,4,3|IL_INCR,16|IL_REIN,19,22|IL_REND,25 };
      dotest(47, IL_NEW, 0, 7, x);
      iv = ilstitct(pil, 25), iw = 10;
      if (iv != iw) {
         convrt("(WP1,'0***Test 47(1), ilstitct returned ',J1IL6,"
            "'should be ',J0IL6)", &iv, &iw, NULL);
         exit(1);
         }
      iv = ilstitct(pil, 53), iw = 15;
      if (iv != iw) {
         convrt("(WP1,'0***Test 47(2), ilstitct returned ',J1IL6,"
            "'should be ',J0IL6)", &iv, &iw, NULL);
         exit(1);
         }
      iv = ilstitct(pil, 97), iw = 28;
      if (iv != iw) {
         convrt("(WP1,'0***Test 47(3), ilstitct returned ',J1IL6,"
            "'should be ',J0IL6)", &iv, &iw, NULL);
         exit(1);
         }
      } /* End test 47 local scope */

/* Test 48:  ilstitct() with EVERY, base 1 */

   {  ilstitem x[] = { 2,4,3|IL_INCR,16|IL_REIN,19,22|IL_REND,25 };
      dotest(47, IL_NEW, 1, 7, x);
      iv = ilstitct(pil, 25), iw = 10;
      if (iv != iw) {
         convrt("(WP1,'0***Test 48(1), ilstitct returned ',J1IL6,"
            "'should be ',J0IL6)", &iv, &iw, NULL);
         exit(1);
         }
      iv = ilstitct(pil, 52), iw = 22;
      if (iv != iw) {
         convrt("(WP1,'0***Test 48(2), ilstitct returned ',J1IL6,"
            "'should be ',J0IL6)", &iv, &iw, NULL);
         exit(1);
         }
      iv = ilstitct(pil, 92), iw = 39;
      if (iv != iw) {
         convrt("(WP1,'0***Test 48(3), ilstitct returned ',J1IL6,"
            "'should be ',J0IL6)", &iv, &iw, NULL);
         exit(1);
         }
      } /* End test 48 local scope */


   cryout(RK_P1, "0Normal end of tests.", RK_LN2|RK_FLUSH, NULL);
   return 0;
   } /* End ilsttest main program */
