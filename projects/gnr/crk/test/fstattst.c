/***********************************************************************
*                             FSTATTST.C                               *
*  The purpose of this test is to display certain values returned by   *
*  fstat() when the output is to stdout or stderr, either native or    *
*  redirected to a file.  These results can hopefully be used to pro-  *
*  gram the appropriate tests in cryout() to inactivate spout when     *
*  stdout and stderr are the same.                                     *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

void main() {

   struct stat statbuf;
   int i;

   for (i=1; i<=2; i++) {

      if (fstat(i, &statbuf) < 0) {
         printf("Unable to fstat %d\n", i);
         exit(i); }
      printf("\nFor file handle %d:\n", i);
      printf("   st_dev = %d\n", statbuf.st_dev);
      printf("   st_ino = %d\n", statbuf.st_ino);
      printf("   st_mode= %x\n", statbuf.st_mode);
      printf("   st_rdev= %d\n", statbuf.st_rdev);
      } /* End loop over two output handles */
   } /* End fstattst() */
