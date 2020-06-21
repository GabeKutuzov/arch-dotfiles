/* (c) Copyright 2008-2009, The Rockefeller University *11115* */
/* $Id: getmyip.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                               getmyip                                *
*                                                                      *
*  Synopsis:  char *getmyip(void)                                      *
*                                                                      *
*  This program obtains the (or a) IP address of the host on which it  *
*  is running.  Some research suggested using gethostname() to find    *
*  the name of the host, then gethostbyname to look up the IP address  *
*  of that name.  However, it is documented that gethostbyname tries   *
*  first named, then /etc/hosts.  It is not obvious that the required  *
*  information will be present in a machine using a DHCP address.      *
*                                                                      *
*  Accordingly, I decided to run the net tools program ifconfig and    *
*  look for the interface 'eth0', and if that is not present, 'ppp0',  *
*  and if that is not present, 'wlan0'.  The IP address can be read    *
*  off the 'inet addr:' field of the reply.                            *
*                                                                      *
*  This program is assigned abexit error codes 115-119                 *
*                                                                      *
************************************************************************
*  New program, 08/17/08, G.N. Reeke                                   *
*  ==>, 09/03/08, GNR - Last date before committing to svn repository  *
*  Rev, 12/18/09, GNR - Add wlan0, changes for SUNOS                   *
***********************************************************************/

#include <stddef.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include "sysdef.h"

#ifdef SUNOS
typedef int ssize_t;
#define NINTFACES 1        /* Number of interface names to check */
#else
#define NINTFACES    3     /* Number of interface names to check */
#endif

#define LNIPADDR    19     /* Maximum length of an IP address */
#define READ_PIPE    0
#define WRITE_PIPE   1

/*---------------------------------------------------------------------*
*                               getmyip                                *
*---------------------------------------------------------------------*/

char *getmyip(void) {

   FILE    *rF;            /* Result converted to FILE */
   char    *pip1,*pip2;    /* Ptrs to result text in ifconfig reply */
   pid_t   rc;
   int     ii,ll,wc,pdes[2];
   char    rbuf[LNSIZE];   /* Result buffer */
   static char myip[LNIPADDR+1];
#ifdef SUNOS
   static char *ifnames[NINTFACES] = { "le0" };
   static char opsstr[] = "inet ";
#else
   static char *ifnames[NINTFACES] = { "eth0", "ppp0", "wlan0" };
   static char opsstr[] = "inet addr:";
#endif

/* Zero the result because the method of getting it may not
*  return a zero-deliminted string.  */

   memset(myip, 0, LNIPADDR+1);
   pdes[READ_PIPE] = 0;

/* Loop over interfaces to be checked */

   for (ii=0; ii<NINTFACES; ++ii) {

/* Attempt to execute /sbin/ifconfig <intface> */

      if (pipe(pdes) < 0)
         abexitme(115, "Unable to create pipe");

      rc = fork();
      if (rc < 0)
         abexitme(115, "Unable to fork");
      else if (rc == 0) {
         /* In child process.  Duplicate the pipe stream so it will
         *  appear to ifconfig as stdout, then attempt to execute
         *  ifconfig.  */
         if (dup2(pdes[WRITE_PIPE], STDOUT_FILENO) < 0)
            abexitme(115, "Unable to duplicate pipe");
         if (close(pdes[WRITE_PIPE]) < 0)
            abexitme(115, "Unable to close pipe");
         execlp("/sbin/ifconfig", "ifconfig", ifnames[ii], NULL);
         /* If this call returns, ifconfig did not run */
         abexitme(115, "Unable to execute ifconfig");
         }
      if (wait(&wc) < 0)
         abexitme(116, "Error waiting for ifconfig");
      if (!WIFEXITED(wc))
         abexitm(116, "ifconfig exited abnormally");
      if (WEXITSTATUS(wc) == 0) {

/* ifconfig returned OK.  There will be an error message if the
*  selected interface does not exist, otherwise the required
*  information must be parsed from lots of other stuff.  The
*  parsing here is just the minimum necessary to find the result--
*  add more rigor if wrong answers are obtained with this code.  */

         rF = fdopen(pdes[READ_PIPE], "r");
         if (!rF)
            abexitme(117, "Error opening ifconfig o/p");
         /* Read first line */
         if (!fgets(rbuf, LNSIZE, rF))
            abexitme(117, "Error reading ifconfig o/p");
         if (strstr(rbuf, "error")) goto NotThisOne;
         /* Read second line */
         if (!fgets(rbuf, LNSIZE, rF))
            abexitme(117, "Error reading ifconfig o/p");
         pip1 = strstr(rbuf, opsstr);
         if (pip1) {
            pip1 += strlen(opsstr);
            pip2 = strchr(pip1, ' ');
            if (!pip2)
               abexitm(118, "Unexpected ifconfig o/p");
            ll = pip2-pip1;
            if (ll > LNIPADDR)
               abexitm(118, "Result too long");
            /* Got it.  Return it.  */
            memcpy(myip, pip1, ll);
            myip[ll] = '\0';
            if (fclose(rF) != 0)
               abexitme(118, "Error closing ifconfig o/p");
            return myip;
            }
NotThisOne:
         if (fclose(rF) != 0)
            abexitme(118, "Error closing ifconfig o/p");
         }

      } /* End ii loop */

   return NULL;
   } /* End getmyip() */
