/* (c) Copyright 1996-2008, The Rockefeller University *11113* */
/***********************************************************************
*                                sgrep                                 *
*                                                                      *
*  Synopsis:  sgrep  word  filelist                                    *
*                                                                      *
*  This program is a wrapper for the standard UNIX utility "grep".     *
*  It encloses the "word" argument inside a pair of expressions of     *
*  the form "\<" and "\>" in order to prevent finding "word" when it   *
*  is a substring of a larger word.  Then it calls "grep", passing the *
*  "filelist" argument(s) through as the filename argument of "grep".  *
*                                                                      *
*  N.B.  An earlier version of this program used "[A-Za-z0-9_]" as the *
*  brackets.  This failed to match at the start or end of a line.  It  *
*  also called "egrep" instead of "grep".  The present version works   *
*  on both Linux and SunOS systems.                                    *
************************************************************************
*  New program, 03/28/96, G.N. Reeke                                   *
*  Rev, 09/27/07, GNR - Skip over args starting with '-'               *
*  Rev, 11/17/08, GNR - Bug fix:  It didn't find the target string if  *
*                       it occurred at the beginning or end of a line. *
***********************************************************************/

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <errno.h>
#include <memory.h>

int main(int argc, char *argv[]) {

   static char finder[] = "grep";
   static char wdel1[] = "\\<";
   static char wdel2[] = "\\>";

   char *new_word;
   int ipat;                  /* Position of pattern arg */
   int lnw;                   /* Length of new_word */
   int rc;                    /* A return code */

/* Print synopsis if not enough arguments */

   if (argc < 3) {
      printf("\nSynopsis: sgrep <word> <filelist>\n");
      exit(1);
      }

/* Skip over switch arguments */

   for (ipat=1; ipat<argc; ++ipat) {
      if (argv[ipat][0] != '-') break;
      }

/* Create new first argument to define word as regexp */

   lnw = strlen(argv[ipat]) + sizeof(wdel1) + sizeof(wdel2) + 1;
   new_word = (char *)malloc((size_t)lnw);
   strcpy(new_word, wdel1);
   strcat(new_word, argv[ipat]);
   strcat(new_word, wdel2);

/* Execute grep */

   argv[0] = finder;
   argv[ipat] = new_word;
   rc = fork();
   if (rc < 0) {
      printf("\nUnable to fork, errno = %d\n", errno);
      exit(2);
      }
   else if (rc == 0) {
      /* In child process.  Execute grep (never returns) */
      execvp(finder, argv);
      }
   if (wait(NULL) < 0) {
      printf("\nError %d waiting for grep to complete\n", errno);
      exit(3);
      }

/* Clean up and return */

   free(new_word);
   return 0;

   } /* End sgrep() */
