/* (c) Copyright 2008, The Rockefeller University *11113* */
/***********************************************************************
*                              gnrversn                                *
*                                                                      *
*  This program is intended to be run from a shell-out in a make file  *
*  to determine the subversion version number of the program being     *
*  built, whether in the working directory maintained by subversion    *
*  or in an ordinary copy made from that directory (e.g. when the      *
*  source code is copied to another user).                             *
*                                                                      *
*  It first attempts to execute svnversion, and if that succeeds,      *
*  it stores the result in a file called .gnrsvvsn in the current      *
*  directory, with the expectation that this file will be copied       *
*  along with the other files in the project to some other place.      *
*  It then deletes any trailing alphabetic suffix and returns the      *
*  number via stdout to the caller.                                    *
*  But, if svnversion cannot be executed, the program next looks       *
*  for the .gnrsvvsn file left by a previous execution, and, if it     *
*  is found, it returns that result, again trimmed to a number.        *
*  Finally, if both of the above fail, "Unknown" is returned.          *
*                                                                      *
*  This program is assigned abexit error codes 290-295                 *
************************************************************************
*  New program, 04/09/08, G.N. Reeke                                   *
*  Rev, 04/15/08, GNR - Also test for .svn folder in the cwd           *
*  Rev, 06/19/08, GNR - Changes needed to compile for SUN4             *
***********************************************************************/

#include <stddef.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>
#include "sysdef.h"

#ifdef SUN4
typedef int ssize_t;
#endif

#define LVERSION    12     /* Maximum length of a version number */
#define NGNRFILE  ".gnrsvvsn"
#define SVNSUBDIR ".svn"
#define READ_PIPE    0
#define WRITE_PIPE   1
#define NOENT_CODE 105     /* Defined in errtab */

/*---------------------------------------------------------------------*
*                          fatal error exits                           *
*                                                                      *
*  N.B.  The names of these terminal error routines are the same as    *
*  the ones in the standard rocks library so that any abexit calls     *
*  from gnr libraries, if used, will also get caught here.             *
*                                                                      *
*  (1) Generate fatal error message (code only) and terminate          *
*  void abexit(int rc)                                                 *
*  Argument:                                                           *
*     rc       error code to be returned                               *
*                                                                      *
*  (2) Generate fatal error code with message text and terminate       *
*  void abexitm(int rc, char *msg)                                     *
*  Arguments:                                                          *
*     rc       error code to be returned                               *
*     msg      text string to be printed                               *
*                                                                      *
*  (3) Generate fatal error message including a system error number    *
*  void abexitme(int rc, char *msg)                                    *
*  Arguments are same as for abexitm()                                 *
*                                                                      *
*  N.B.  Do not call ssprintf before printing txt argument, because    *
*  it will often be a static string returned by a call to ssprintf().  *
*---------------------------------------------------------------------*/

int abexloop;

void abexit(int rc) {
   if (abexloop < 2) {
      abexloop = 2;
      fprintf(stderr, "\n***Program terminated"
         " with abend code %d\n", rc);
      fflush(stderr);
      }
   exit(rc);
   } /* End abexit() */

void abexitm(int rc, char *msg) {
   if (!abexloop) {
      abexloop = 1;
      fputs("\n***", stderr);
      fputs(msg,  stderr);
      fputs("\n", stderr);
      }
   abexit(rc);
   } /* End abexitm() */

void abexitme(int rc, char *msg) {
   if (!abexloop) {
      int sverr = errno;   /* Save error across fputs call */
      abexloop = 1;
      fputs("\n***", stderr);
      fputs(msg,  stderr);
      fprintf(stderr, "\n***The system error code is %d\n", errno);
      errno = sverr;
      perror("***System error text");
      }
   abexit(rc);
   } /* End abexitme() */

/*---------------------------------------------------------------------*
*                              gnrversn                                *
*---------------------------------------------------------------------*/

int main(int argc, char *argv[]) {

   DIR   *svndir;
   pid_t   rc;
   ssize_t rn = 0;
   int   gfd;           /* .gnrsvvsn descriptor */
   int   wc,pdes[2];
   char  version[LVERSION];
   char * const SvnVsn[] = { "svnversion", NULL };

/* Zero the result because the method of getting it may not
*  return a zero-deliminted string.  */

   memset(version, 0, LVERSION);
   pdes[READ_PIPE] = 0;

/* First check whether there is a .svn folder in the cwd.
*  If there is not, it doesn't matter whether subversion
*  is installed on this machine, it is not being used for
*  this project.  */

   if (!(svndir = opendir(SVNSUBDIR)))
      goto NoSVN;
   closedir(svndir);

/* Attempt to execute svnversion.  While it would be nice to
*  just use the popen/pclose system calls, those would not
*  provide an easy way to determine that svnversion does not
*  exist, which in this program is a normal situation, not
*  an error.  So we need to make a pipe and all that.  */

   if (pipe(pdes) < 0)
      abexitme(290, "Unable to create pipe");

   rc = fork();
   if (rc < 0)
      abexitme(290, "Unable to fork");
   else if (rc == 0) {
      /* In child process.  Duplicate the pipe stream so it will
      *  appear to svnversion as stdout, then attempt to execute
      *  svnversion.  */
      if (dup2(pdes[WRITE_PIPE], STDOUT_FILENO) < 0)
         abexitme(290, "Unable to duplicate pipe descriptor");
      if (close(pdes[WRITE_PIPE]) < 0)
         abexitme(290, "Unable to close pipe descriptor");
      execvp(SvnVsn[0], SvnVsn);
      /* If this call returns, svnversion did not run.  Error
      *  ENOENT indicates the file did not exist.  Anything else
      *  we will consider fatal.  */
      if (errno == ENOENT) exit(NOENT_CODE);
      abexitme(291, "Unable to execute svnversion");
      }
   if (wait(&wc) < 0)
      abexitme(292, "Error waiting for forked svnversion to exit");
   if (!WIFEXITED(wc))
      abexitm(292, "Forked svnversion exited abnormally");
   if (WEXITSTATUS(wc) == 0) {
      if ((rn = read(pdes[READ_PIPE], version, LVERSION-1)) < 0)
         abexitme(292, "Error reading svnversion output pipe");
      /* Got it.  Strip off any nonnumeric characters from the
      *  right end before saving it.  */
      while (rn > 1 && !isdigit((int)version[rn-1])) {
         version[rn-1] = 0; --rn; }
      if ((gfd = open(NGNRFILE, O_WRONLY|O_CREAT|O_TRUNC,
            S_IRUSR|S_IWUSR|S_IRGRP)) < 0)
         abexitme(293, "Error opening file " NGNRFILE);
      if (write(gfd, version, (size_t)rn) < 0)
         abexitme(293, "Error writing file " NGNRFILE);
      if (close(gfd) < 0)
         abexitme(293, "Error closing file " NGNRFILE);
      }
   else if (WEXITSTATUS(wc) == NOENT_CODE) {
      /* svnversion was not found in the current path.  We are
      *  running in a copied source directory as explained above.  */
NoSVN:
      if ((gfd = open(NGNRFILE, O_RDONLY)) < 0) {
         if (errno == ENOENT) {
            /* .gnrsvvsn file does not exist.  Return "Unknown".  */
            strcpy(version, "Unknown");
            }
         else
            abexitme(294, "Error opening file " NGNRFILE);
         }
      else {
         if ((rn = read(gfd, version, LVERSION-1)) < 0)
            abexitm(294, "Error reading file " NGNRFILE);
         if (close(gfd) < 0)
            abexitme(294, "Error closing file " NGNRFILE);
         }
      }

/* The version number is now in the version string.
*  Write it to stdout, clean up, and return.  */

   if (write(STDOUT_FILENO, version, strlen(version)) < 0)
      abexitme(295, "Error writing to stdout");
   if (close(STDOUT_FILENO) < 0)
      abexitme(295, "Error closing stdout");
   if (pdes[READ_PIPE] > 0 && close(pdes[READ_PIPE]) < 0)
      abexitme(292, "Error closing svnversion output pipe");

   return 0;
   } /* End gnrversn() */
