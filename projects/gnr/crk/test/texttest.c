/***********************************************************************
*                              texttest                                *
*                                                                      *
*  This program tests the rktext package by creating several hundred   *
*  text strings (enough to be sure the table expansion code is exer-   *
*  cised, storing them, then retrieving them and testing them.  Also,  *
*  at least one duplicate string is tested.                            *
***********************************************************************/

#define MAIN
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "sysdef.h"
#include "rocks.h"
#include "rkxtra.h"

#define LTEXT  20
#define NTEXT 200

int main(void) {

   char *p;
   int htxt[NTEXT];
   char tstr[LTEXT+1];
   int h,i;
   int ierr = 0;

/* Test 1:  Look up a string before anything is stored,
*  to be sure nonexistent tables don't cause trouble.  */

   h = findtxt("Test 1");
   if (h != 0) {
      convrt("(P1,'0Test 1 failed:'/' Text that was not "
         "stored returned locator ',J0I8,' 0 was expected.')",
         &h, NULL);
      ++ierr;
      }

/* Test 2:  Store the same string twice and see if the same
*  locator is returned each time.  */

   h = savetxt("Test 2");
   p = getrktxt(h);
   if (strcmp("Test 2", p)) {
      convrt("(P1,'0Test 2a failed--getrktxt() returned:'/1H A80/"
         "' \"Test 2\" was expected.')", p, NULL);
      ++ierr;
      }
   i = savetxt("Test 2");
   if (i != h) {
      convrt("(P1,'0Test 2b failed:'/' savetxt() returned "
         "locator ',J0I8,2H, J0I8,' was expected.')", &i, &h, NULL);
      ++ierr;
      }

/* Test 3:  Store lots of different strings, enough to exercise
*  the table expansion code.  Be sure all of them can be retrieved
*  correctly.  */

   for (i=0; i<NTEXT; ++i) {
      sconvrt(tstr, "('Test string',I8)", &i, NULL);
      htxt[i] = savetxt(tstr);
      }
   for (i=0; i<NTEXT; ++i) {
      sconvrt(tstr, "('Test string',I8)", &i, NULL);
      h = findtxt(tstr);
      if (h <= 0) {
         convrt("(P1,'0Test 3a failed for string ',J0I8,1H:/"
            " findtxt() returned ',J0I8,', a positive locator"
            " was expected.')", &i, &h, NULL);
         ++ierr;
         }
      p = getrktxt(h);
      if (strcmp(tstr, p)) {
         convrt("(P1,'0Test 3b failed--getrktxt() returned:'/1H A80/"
            "' \"',A80,'\" was expected.')", p, tstr, NULL);
         ++ierr;
         }
      }

   /* Print final word */
   if (ierr)
      cryout(RK_P1, "0Got at least one error.", RK_LN2+RK_FLUSH, NULL);
   else
      cryout(RK_P1, "0All tests passed OK.", RK_LN2+RK_FLUSH, NULL);

   return 0;
   } /* End hashtest() */

