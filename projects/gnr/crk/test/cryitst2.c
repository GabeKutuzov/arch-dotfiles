/*--------------------------------------------------------------------*/
/*                                                                    */
/*                            CRYIN Test2                             */
/*                                                                    */
/*    Test printing of correct card when an error is encountered.     */
/*                                                                    */
/*--------------------------------------------------------------------*/

#define MAIN
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "rocks.h"
#include "rkxtra.h"

#define InFile "cryitst2.dat"

/* This program expects to read an input file named InFile
*  with four lines of arbitrary data.  The second line will
*  be scanned and the third field will be marked as an error.
*/

void main() {

   char data[16+2];
   char *sgot;
   int   icol;

/* Read using cryin */

   settit("TITLE Cryin Test Program");
   cryout(RK_P1, "0==>Expect the third field of the second card", RK_LN2,
      "   to be marked as an unrecognized keyword error.", RK_LN1, NULL);
   cdunit(InFile);
   sgot = cryin();
   cdprt1(sgot);
   sgot = cryin();
#if 0
   cdprt1(sgot);
#endif
   cdscan(sgot, 0, 16, 0);
   for (icol=0; icol<3; icol++) {
      scan(data, 0);
      }
   ermark(RK_IDERR);
   sgot = cryin();
   cdprt1(sgot);
   sgot = cryin();
   cdprt1(sgot);
   cryout(RK_P1, "0End of cryitst2", RK_LN2+RK_FLUSH, NULL);
   exit(0);
   } /* End cryitst2 */   

