/***********************************************************************
*                             MYIPTEST.C                               *
*  The purpose of this test is to display the value returned by        *
*  getmyip().  Since all errors in getmyip call abexitxx, those        *
*  routines are included here without the termination so several       *
*  error conditions can be tested in one run.  (But these tests        *
*  would require introducing deliberate errors in getmyip.c, not       *
*  included at this time.                                              *
***********************************************************************/

#define MAIN
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "sysdef.h"
#include "rocks.h"
#include "rkxtra.h"
#include "rksubs.h"

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

   int saverno = errno;
   convrt("(WP1,'0***Abexitme called with code ',J0I5,', error ',"
      "J0I5,', and text'/J0A128)", &code, &saverno, emsg, NULL);
   } /* End abexitme() */

/***********************************************************************
*                                                                      *
*                              MYIPTEST                                *
*                                                                      *
***********************************************************************/

int main() {

   char *myip = getmyip();

   printf("\nThe value returned by getmyip is %s\n", myip);

   return 0;

   } /* End myiptest() */
