/* (c) Copyright 2010-2013, The Rockefeller University *11115* */
/* $Id: bbdcmlog.c 16 2017-01-13 20:36:40Z  $ */
/***********************************************************************
*               BBD (Client Side) mex-Function Package                 *
*                             bbdcmlog.c                               *
*                                                                      *
*  This routine runs as a main program in a child process and just     *
*  copies anything it receives from CNS via the port init socket to    *
*  the log (if there is one), otherwise discards it.  If it receives   *
*  a message that CNS has terminated, it calls exit() and returns the  *
*  CNS abexit code as its return code (the copy in BBDcd of course     *
*  does not help, we are in a different address space).  The parent    *
*  can detect this at appropriate times, notify the BBD user, and      *
*  terminate.  Going the other way, when the parent terminates, it     *
*  just has to send a SIGTERM to the bbdcmlog child process so it      *
*  can cleanly close the log file.  The code hopefully handles races   *
*  between signal and input appropriately.                             *
*                                                                      *
*  There is a complication that one read() can read parts of multiple  *
*  lines.  It is not convenient to use fgets() with select().  So the  *
*  code to look for a CNS termination has to deal with fragments.      *
*                                                                      *
*  This routine is linked as a separate main program rather than as a  *
*  subroutine of bbdcinit as in the C-client version, because it must  *
*  use its own abexit(), etc., not the mex-function versions.          *
************************************************************************
*  V1A, 10/29/10, Based on bbdcmlog subroutine in C client bbdcinit    *
*  ==>, 11/04/10, GNR - Last mod before committing to svn repository   *
*  Rev, 11/05/10, GNR - Better error checking for bad child procs.     *
*  Rev, 05/02/13, GNR - Restart select() on EINTR interruption         *
***********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <semaphore.h>

#define I_AM_MEX

#include "sysdef.h"
#include "bbd.h"
#include "rfdef.h"
#include "rksubs.h"

static struct {               /* Global data for this process */
   int cnssock;                  /* File descriptor from CNS */
   int log;                      /* Log file descriptor */
   int abexloop;                 /* Abexit loop preventer */
   volatile int DoStop;          /* SIGTERM signal indication */
   volatile int Mlogrc;          /* Kill client--server abended */
   } MLOG_GLBL;

/*=====================================================================*
*                             bbdcstopsig                              *
*                                                                      *
*  Routine to handle a SIGTERM received by this process                *
*=====================================================================*/

static void bbdcstopsig(int signum) {
   MLOG_GLBL.DoStop = YES;
   }


/*=====================================================================*
*                          fatal error exits                           *
*                                                                      *
*  N.B.  All functions in the bbdcmlog program are expected to report  *
*  terminal errors by calling one of these routines.  The names are    *
*  the same as the names of the standard rocks library exit routines   *
*  so that any abexit calls from libraries also get caught here        *
*  (although in the current design there should not be any such).      *
*  These routines copy error messages to the log output stream if it   *
*  exists rather than to stderr.  There is no use checking for write   *
*  errors here, as we are already doing an error termination.          *
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
*  N.B.  Do not call ssprintf before printing msg argument, because    *
*  it will often be a static string returned by a call to ssprintf().  *
*=====================================================================*/

void abexit(int rc) {
   if (MLOG_GLBL.log > 0 && MLOG_GLBL.abexloop < 2) {
      char *ptxt = ssprintf(NULL, "\n***Program terminated"
         " with abend code %d\n", rc);
      MLOG_GLBL.abexloop = 2;
      write(MLOG_GLBL.log, ptxt, strlen(ptxt));
      close(MLOG_GLBL.log);
      close(MLOG_GLBL.cnssock);
      }
   exit(rc);
   } /* End abexit() */

void abexitm(int rc, char *msg) {
   if (MLOG_GLBL.log > 0 && MLOG_GLBL.abexloop < 1) {
      MLOG_GLBL.abexloop = 1;
      write(MLOG_GLBL.log, "\n***", 4);
      write(MLOG_GLBL.log, msg, strlen(msg));
      write(MLOG_GLBL.log, "\n", 1);
      }
   abexit(rc);
   } /* End abexitm() */

void abexitme(int rc, char *msg) {
   if (MLOG_GLBL.log > 0 && MLOG_GLBL.abexloop < 1) {
      char *ptxt;
      int  lerrno = errno;    /* Save errno across write calls */
      MLOG_GLBL.abexloop = 1;
      write(MLOG_GLBL.log, "\n***", 4);
      write(MLOG_GLBL.log, msg, strlen(msg));
      write(MLOG_GLBL.log, "\n", 1);
      ptxt = ssprintf(NULL,"***The system error code is %d\n",lerrno);
      write(MLOG_GLBL.log, ptxt, strlen(ptxt));
      }
   abexit(rc);
   } /* End abexitme() */


/*=====================================================================*
*                        bbdcmlog main program                         *
*                                                                      *
*  Arguments (passed as ASCII strings from bbdcinit) are:              *
*  cnssock  Open file descriptor for socket to talk to CNS.            *
*  log      Open file descriptor for log file.                         *
*=====================================================================*/

int main(int argc, char *argv[]) {

   fd_set rfds;               /* Set for read select */
   struct timeval msgtv;      /* Read select timeout value */
   ssize_t rlen;              /* Read length returned */
   ssize_t lmsg,llog;         /* Counts for message extraction */
   int   rc;                  /* A return code */
   char  logbuf[LNSIZE+2];
   char  msgbuf[LNSIZE+2];

   /* Check arguments */
   if (argc != 3)
      abexitm(BBDcErrLogFile, "Bad args to log process");
   if (signal(SIGTERM, bbdcstopsig) == SIG_ERR)
      abexitm(BBDcErrLogFile, "Unable to install termination signal "
         "handler for log process");

   /* If we got here at all, the execve in bbdcinit must have worked,
   *  and we can post our semaphore so the normal error checking
   *  mechanism can come into play.  */

   {  sem_t *psem = sem_open(MlogSemNm, O_CREAT, URW_Mode, 0);
      if (psem == SEM_FAILED)
         abexitme(BBDcErrSema, "bbdcminp can't open " MlogSemNm);
      if (sem_post(psem) < 0)
         abexitme(BBDcErrSema, "bbdcminp can't post " MlogSemNm);
      if (sem_close(psem) < 0)
         abexitme(BBDcErrSema, "bbdcminp can't close " MlogSemNm);
      }

   /* Extract file descriptors from arguments */
   MLOG_GLBL.cnssock = sibcdin(RK_QPOS|RK_CTST|LMxClSPAA-1, argv[1]);
   MLOG_GLBL.log = sibcdin(RK_QPOS|RK_CTST|LMxClSPAA-1, argv[2]);

   lmsg = 0;
   while (MLOG_GLBL.DoStop <= 0) {
      /* Set select() to wait for cnssock or 0.5 sec timeout */
      FD_ZERO(&rfds);
      FD_SET(MLOG_GLBL.cnssock, &rfds);
      msgtv.tv_sec = 0;
      msgtv.tv_usec = 500000;
      rc = select(MLOG_GLBL.cnssock+1, &rfds, NULL, NULL, &msgtv);
      if (rc == 0 || rc == EINTR)
         continue;
      if (rc < 0)
         abexitme(BBDcErrLogFile, "Select error reading CNS log");
      /* Now should have data available */
      if (!FD_ISSET(MLOG_GLBL.cnssock, &rfds))
         abexitm(BBDcErrLogFile, "Contradictory read log select "
            "return--contact author");
      rlen = read(MLOG_GLBL.cnssock, logbuf, LNSIZE);
      if (rlen < 0)
         abexitme(BBDcErrLogFile, "Error reading CNS output");
      if (rlen == 0) break;
      /* Write to the log only if it exists.
      *  N.B.  Errors writing to the log file should use only
      *  abexit, not abexitm[e], because the file is bad.  */
      if (MLOG_GLBL.log && write(MLOG_GLBL.log, logbuf, rlen) < 0)
         abexit(BBDcErrLogFile);
      /* Check for server death notice.  (This code is not very
      *  robust, but it is hard to do anything better and quite
      *  unlikely the abexit message text will ever change.)  */
      for (llog=0; llog<rlen; ++llog) {
         if (lmsg >= LNSIZE) lmsg = 0;    /* Just in case */
         msgbuf[lmsg] = logbuf[llog];
         if (msgbuf[lmsg++] == '\n') {
            /* Got end of line, look for termination message */
            if (lmsg > 21) {
               if (!memcmp(msgbuf, "Normal CNS terminatio", 21)) {
                  /* Got normal termination message */
                  MLOG_GLBL.Mlogrc = 101;
                  MLOG_GLBL.DoStop = YES;
                  }
               else if (!memcmp(msgbuf, "***PROGRAM TERMINATED", 21)) {
                  /* Got abnormal termination message */
                  MLOG_GLBL.Mlogrc =
                     sibcdin(RK_IORF+RK_SSCN+5, msgbuf+37);
                  if (MLOG_GLBL.Mlogrc <= 0 ||
                        MLOG_GLBL.Mlogrc >= 255)
                     MLOG_GLBL.Mlogrc = 100;
                  MLOG_GLBL.DoStop = YES;
                  }
               } /* End checking for terminal message */
            /* Not terminal.  Start accumulating new line */
            lmsg = 0;
            } /* End end of line checking */
         } /* End moving characters to msg line */
      } /* End main copy loop */

/* Close the log file, but only shutdown the control socket in the
*  read direction.  */

#if 0
{  /*** DEBUG ***/
   char *tmsg = ssprintf(NULL,
      "bbdcmlog terminating with exit code %d\n",
      MLOG_GLBL.Mlogrc);
   write(MLOG_GLBL.log, tmsg, strlen(tmsg));
   }  /* ENDDEBUG */
#endif

   if (close(MLOG_GLBL.log) < 0)
      abexitme(BBDcErrLogFile, "Error closing log file");
   exit(MLOG_GLBL.Mlogrc);
   } /* End bbdcmlog() */
