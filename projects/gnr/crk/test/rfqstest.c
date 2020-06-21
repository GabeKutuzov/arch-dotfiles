/***********************************************************************
*                             RFQSTEST.C                               *
*  The purpose of this test is to display the value returned by        *
*  rfqsame() when the output is to stdout or stderr, either native or  *
*  redirected to a file.  These results can hopefully be used to pro-  *
*  gram the appropriate tests in cryout() to inactivate spout when     *
*  stdout and stderr are the same.                                     *
***********************************************************************/

#define MAIN
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "rocks.h"
#include "rfdef.h"

void main() {

   struct RFdef rfd1,rfd2;
   int rqs;

   memset((char *)&rfd1, 0, sizeof rfd1);
   memset((char *)&rfd2, 0, sizeof rfd2);
   rfopen(&rfd1, NULL, WRITE, SysOut, SEQUENTIAL, TOP,
      NO_LOOKAHEAD, REWIND, RELEASE_BUFF, IGNORE, IGNORE,
      IGNORE, ABORT);         /* Open print file */
   rfopen(&rfd2, NULL, WRITE, SysErr, SEQUENTIAL, TOP,
      NO_LOOKAHEAD, REWIND, RELEASE_BUFF, IGNORE, IGNORE,
      IGNORE, ABORT);         /* Open spout file */
   rqs = rfqsame(&rfd1, &rfd2);
   printf("\nThe value returned by rfqsame is %d\n", rqs);

   } /* End rfqsame() */
