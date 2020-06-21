/* (c) Copyright 2012-2016, The Rockefeller University *11113* */
/***********************************************************************
*                                semrm                                 *
*                                                                      *
*  Synopsis:  semrm <semaphore> ...                                    *
*                                                                      *
*     This program removes the named semaphores <semaphore> from the   *
*  system.  The semaphore must be owned by the user that runs this     *
*  command.                                                            *
*     It may need to be used after a matlab crash if semaphores are    *
*  left in memory.                                                     *
*                                                                      *
*  Error codes 660-664 are reserved for this program.                  *
************************************************************************
*  Initial version, 03/22/12, G.N. Reeke                               *
*  Rev, 03/29/16, GNR - Use sprintf() instead of ssprintf()            *
***********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <semaphore.h>
#include <errno.h>
#include "sysdef.h"

/*=====================================================================*
*           Non-ROCKS versions of abexit, abexitm, abexitme            *
*=====================================================================*/

int AbexLoop = 0;       /* Control to avoid looping error messages */

void abexit(int code) {

   if (AbexLoop < 2) {
      char tmsg[48];
      AbexLoop = 2;
      sprintf(tmsg, "***Program terminated with abend code %4d\n",
         code);
      fputs(tmsg, stderr);
      fflush(stderr);
      }
#ifdef UNIX
   exit(code > 255 ? 100 : code);
#else
   exit(code);
#endif
   } /* End abexit() */

void abexitm(int code, char *emsg) {

   if (!AbexLoop++) {
      fputs("***", stderr);
      fputs(emsg, stderr);
      fputs("\n", stderr);
      }
   abexit(code);
   } /* End abexitm() */

void abexitme(int code, char *emsg) {

   if (!AbexLoop++) {
#if defined(UNIX) || defined(DOS)
      int sverr = errno;   /* Save error across fputs() calls */
#endif
      fputs("***", stderr);
      fputs(emsg, stderr);
      fputs("\n", stderr);
#if defined(UNIX) || defined(DOS)
      errno = sverr;
      perror("***semrm ");
#else
      fputs("***Error details not available on this OS\n", stderr);
#endif
      }
   abexit(code);
   } /* End abexitme() */


/*=====================================================================*
*                                semrm                                 *
*=====================================================================*/

int main(int argc, char *argv[]) {

   int isem;

   if (argc < 2) abexitm(660,
      "At least one argument, a semaphore name, is required");

   for (isem=1; isem<argc; ++isem) {
      if (sem_unlink(argv[isem])) {
         int terr = errno;
         char smsg[64];
         sprintf(smsg, "Semaphore %32s could not be unlinked",
            argv[isem]);
         errno = terr;
         abexitme(661, smsg);
         }
      }

   return 0;
   } /* End semrm */
