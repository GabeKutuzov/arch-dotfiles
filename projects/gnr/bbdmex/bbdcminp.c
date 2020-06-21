/* (c) Copyright 2010-2015, The Rockefeller University *11115* */
/* $Id: bbdcminp.c 16 2017-01-13 20:36:40Z  $ */
/***********************************************************************
*               BBD (Client Side) mex-Function Package                 *
*                             bbdcminp.c                               *
*                                                                      *
*  This routine runs as a main program in a child process and copies   *
*  the CNS input file to CNS via the main socket, allowing the main    *
*  (MATLAB) program to deal with the configuration and neural data.    *
*  It is left for future work to provide a mechanism whereby the       *
*  client can interrupt CNS, issue Group III controls via this         *
*  process, and restart with a new CYCLE card.                         *
*                                                                      *
*  This routine is linked as a separate main program rather than as a  *
*  subroutine of bbdcinit as in the C-client version, because it must  *
*  link against the standard ROCKS library abexit(), etc., not the     *
*  mex-function versions.                                              *
************************************************************************
*  V1A, 10/29/10, Based on bbdcminp subroutine in C client bbdcinit    *
*  ==>, 11/04/10, GNR - Last mod before committing to svn repository   *
*  Rev, 11/05/10, GNR - Better error checking for bad child procs.     *
*  V1B, 01/12/11, GNR - Pass file descriptor for errors from bbdcinit  *
*  V1C, 01/16/11, GNR - Add optional arg to pass extra info to server  *
*  R13, 11/18/15, GNR - Add 'vardefs' mechanism.  This is an enhanced  *
*                       version of the BBDCEXSE mechanism, which is    *
*                       retained for compatibility.                    *
***********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <syslog.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <semaphore.h>

#define I_AM_MEX

#include "sysdef.h"
#include "bbd.h"
#include "rksubs.h"

/* Define positions of the input parameters */
enum minpargs { mnpnm, mnpsock, mnpinfd, mnpchnm, mnpvcli, mnpport,
   mnpxtra, mnpvard };

static struct {
   int msgfd;                    /* Message file descriptor */
   int abexloop;                 /* Abexit loop eliminator */
   volatile int DoStop;          /* SIGTERM signal indication */
   } MINP_GLBL;

/*=====================================================================*
*                             bbdcstopsig                              *
*                                                                      *
*  Routine to handle a SIGTERM received by this process.               *
*=====================================================================*/

static void bbdcstopsig(int signum) {
   MINP_GLBL.DoStop = YES;
   }


/*=====================================================================*
*                          fatal error exits                           *
*                                                                      *
*  N.B.  All functions in the bbdcminp program are expected to report  *
*  terminal errors by calling one of these routines.  The names are    *
*  the same as the names of the standard rocks library exit routines   *
*  so that any abexit calls from libraries also get caught here        *
*  (although in the current design there should not be any such).      *
*  These routines write error messages to a file descriptor passed     *
*  from bbdcinit if it exists, otherwise to stderr for compatibility   *
*  with earlier versions, but there may be no way for user to see      *
*  output written to stderr.  There is no use checking for write       *
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
   if (MINP_GLBL.abexloop < 2) {
      char *msg1 = ssprintf(NULL,
         "\n***bbdcminp terminated with abend code %d\n", rc);
      write(MINP_GLBL.msgfd, msg1, strlen(msg1));
      close(MINP_GLBL.msgfd);
      MINP_GLBL.abexloop = 2;
      }
   exit(rc);
   } /* End abexit() */

void abexitm(int rc, char *msg) {
   if (!MINP_GLBL.abexloop) {
      char *msg2 = ssprintf(NULL, "\n***%s\n", msg);
      write(MINP_GLBL.msgfd, msg2, strlen(msg2));
      MINP_GLBL.abexloop = 1;
      }
   abexit(rc);
   } /* End abexitm() */

void abexitme(int rc, char *msg) {
   if (!MINP_GLBL.abexloop) {
      char *msg3;
      int sverr = errno;
      msg3 = ssprintf(NULL, "\n***%s\n", msg);
      write(MINP_GLBL.msgfd, msg3, strlen(msg3));
      msg3 = ssprintf(NULL,
         "***The system error code is %d\n", sverr);
      write(MINP_GLBL.msgfd, msg3, strlen(msg3));
      MINP_GLBL.abexloop = 1;
      }
   abexit(rc);
   } /* End abexitme() */


/*=====================================================================*
*                               tstamp                                 *
*                                                                      *
*  Simplified version of crk tstamp using ssprintf instead of wbcdwt.  *
*  The argument is a pointer to a string of at least 18 chars that     *
*  will be filled with 'yy/mm/dd hh:mm:ss' with a final NULL.          *
*=====================================================================*/

void tstamp(char *tmstmp) {

   struct tm *ptm;
   time_t tt;
   int tyr,tmo;

   time(&tt);
   ptm = localtime(&tt);
   tyr = ptm->tm_year%100;
   tmo = ptm->tm_mon+1;
   ssprintf(tmstmp, "%2d/%2d/%2d %2d:%2d:%2d", tyr, tmo,
      ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
   } /* End tstamp() */


/*=====================================================================*
*                        bbdcminp main program                         *
*                                                                      *
*  Arguments (passed as ASCII strings from bbdcinit) are:              *
*  cnssock  Open file descriptor for socket to talk to CNS.  May be    *
*           followed by a blank and file descriptor for messages.      *
*  CNSin    Open file descriptor for CNS control file.                 *
*  chnm     Client host name or IP.                                    *
*  Vclients Bits indicating which clients should receive version info. *
*  DataPort Number of port for data exchange.                          *
*  Xtra     Optional string of data to pass at start of control file.  *
*  Vardefs  Records containing variable definitions to be passed to    *
*           CNS via a manufactured EXECUTE card.                       *
*=====================================================================*/

int main(int argc, char *argv[]) {

   char   *pcc,*phn;             /* Ptr into ccbuf, host name */
   FILE   *CNSinf;               /* CNS input file */
   int    cnssock,CNSin;         /* File descriptor arguments */
   int    DataPort;              /* Data port from parent BBDcd */
   int    Vclients;              /* Vclients from parent BBDcd */
   int    lhn;                   /* Length of host name */
   int    lhp;                   /* Length of ":port" */
   int    lcc,ltc;               /* Lengths of ccbuf chunks */
   char   ccbuf[CDSIZE+1];       /* Space for a control card */
   char   tmstmp[18];            /* Space for a time stamp */

#ifdef DBG_MINP
   char   dbgmsg[120];
#endif

   /* The very first thing we must do, before any abexits can
   *  happen, is get the file descriptor for error messages
   *  from the parent and write a startup message there.  */
   MINP_GLBL.msgfd = STDERR_FILENO; /* Just in case */
   pcc = strrchr(argv[mnpsock], ' ');
   if (pcc) {
      MINP_GLBL.msgfd = sibcdin(RK_SSCN|RK_QPOS|RK_CTST+4, pcc+1);
      tstamp(tmstmp);
      write(MINP_GLBL.msgfd, tmstmp, strlen(tmstmp));
      write(MINP_GLBL.msgfd, " bbdcminp started\n", 18);
      fsync(MINP_GLBL.msgfd);
      }

   /* If we got here at all, the execve in bbdcinit must have worked,
   *  and we can post our semaphore so the normal error checking
   *  mechanism can come into play.  */

   {  sem_t *psem = sem_open(MinpSemNm, O_CREAT, URW_Mode, 0);
      if (psem == SEM_FAILED)
         abexitme(BBDcErrSema, "bbdcminp can't open " MinpSemNm);
      if (sem_post(psem) < 0)
         abexitme(BBDcErrSema, "bbdcminp can't post " MinpSemNm);
      if (sem_close(psem) < 0)
         abexitme(BBDcErrSema, "bbdcminp can't close " MinpSemNm);
      }

#ifdef DBG_MINP
   strcpy(dbgmsg, MinpSemNm " semaphore opened successfully.\n");
   write(MINP_GLBL.msgfd, dbgmsg, strlen(dbgmsg));
   fsync(MINP_GLBL.msgfd);
#endif

   if (argc < mnpxtra)
      abexitm(BBDcErrControl, "Bad args to bbdcminp process");
   if (signal(SIGTERM, bbdcstopsig) == SIG_ERR)
      abexitm(BBDcErrControl, "Unable to install termination signal "
         "handler for bbdcminp process");

   /* Extract arguments */
   cnssock = sibcdin(RK_SSCN|RK_QPOS|RK_CTST|LMxClSPAA-1,
      argv[mnpsock]);
   CNSin   = sibcdin(RK_QPOS|RK_CTST|LMxClSPAA-1, argv[mnpinfd]);
   phn     = argv[mnpchnm];
   Vclients = sibcdin(RK_QPOS|RK_CTST|LMxClSPAA-1, argv[mnpvcli]);
   DataPort = sibcdin(RK_QPOS|RK_CTST|LMxClSPAA-1, argv[mnpport]);

/* Insert an undocumented BBDHOST card into the input stream so CNS
*  will know how to call back for confirmation and data exchange.
*  This code will generate correct ROCKS syntax for names of any
*  length, although extra space is used to avoid some length checks.
*  The syntax is (lhn = length of "hostname:port" data, hosts =
*  OR of host numbers that want to receive version info from CNS):
*  BBDHOST [VMSG hosts] lhn hostname:port
*/

#ifdef DBG_MINP
   strcpy(dbgmsg, "Contructing BBDHOST card for Cns.\n");
   write(MINP_GLBL.msgfd, dbgmsg, strlen(dbgmsg));
   fsync(MINP_GLBL.msgfd);
#endif

   lhp = 7;                      /* Max poss. length of ":port\n" */
   lhn = strlen(phn);
   if (lhn >= 1000)
      abexit(BBDcErrControl);    /* So unlikely, no message */
   if (Vclients)
      ssprintf(ccbuf, "BBDHOST VMSG 1 " qMxClSPAfmt "d ", lhn+lhp);
   else
      ssprintf(ccbuf, "BBDHOST " qMxClSPAfmt "d ", lhn+lhp);
   pcc = ccbuf + (ltc = strlen(ccbuf));
   /* Space after "BBDHOST lhn " allowing for possible "\\\n" */
   lcc = CDSIZE - (ltc + 2);
   while (ltc = min(lhn,lcc)) {     /* Assignment intended */
      memcpy(pcc, phn, ltc);
      pcc += ltc, phn += ltc, lhn -= ltc;
      if (lcc - ltc > lhp) break;   /* lhn must be 0 if breaks */
      *pcc++ = '\\';                /* Signal continuation */
      *pcc++ = '\n';
      if (write(cnssock, ccbuf, pcc-ccbuf) < 0)
         goto InputWriteError;
      memcpy(pcc = ccbuf, " ", 1);  /* Now on a continuation */
      lcc = CDSIZE - 3;             /* Allow for another "\\\n" */
      }
   ssprintf(pcc, ":" qMxClSPAfmt "d\n", DataPort);
   if (write(cnssock, ccbuf, strlen(ccbuf)) < 0)
      goto InputWriteError;

#ifdef DBG_MINP
   strcpy(dbgmsg, "Returned from writing BBDHOST to cns socket.\n");
   write(MINP_GLBL.msgfd, dbgmsg, strlen(dbgmsg));
   fsync(MINP_GLBL.msgfd);
#endif

/* The next argument is either a BBDCEXSI string to be written at the
*  start of the server control file, or the word "NoBBDCEXSI" indicat-
*  ing the absence of these data.  It is assumed this will be a PARAMS
*  or EXECUTE card or both to pass user parameters, e.g.  run no.  */

   if (strcmp(argv[mnpxtra], NoEXSI) && write(cnssock, argv[mnpxtra],
      strlen(argv[mnpxtra])) < 0) goto InputWriteError;

#ifdef DBG_MINP
   strcpy(dbgmsg, "Returned from writing BBDCEXSI to cns socket.\n");
   write(MINP_GLBL.msgfd, dbgmsg, strlen(dbgmsg));
   fsync(MINP_GLBL.msgfd);
#endif

/* Next come the variable definitions, if any.  These are reformatted
*  to form a crk EXECUTE card with continuations as needed.  This code
*  assumes that the total length of the variable name and definition
*  is less than CDSIZE so continuation might only be needed after
*  whole entries.  */

#define MXXENTRY (CDSIZE - 3)    /* Space in col. 1 + end comma, LF */
   if (argc > mnpvard) {
      int iarg;
      strcpy(ccbuf, "EXECUTE ");
      pcc = ccbuf + (ltc = strlen(ccbuf));
      lcc = MXXENTRY - ltc;
      for (iarg=mnpvard; iarg<argc; ++iarg) {
         lhn = strlen(argv[iarg]);
         if (lhn > MXXENTRY) abexitm(BBDcErrVDLong, "A variable "
            "definition exceeds the max input file line length");
#ifdef DBG_MINP    /*** DEBUG ***/
         {  char dbgb[40];
            ssprintf(dbgb, "Got vardef %6d, len %6d\n", iarg, lhn);
            write(MINP_GLBL.msgfd, dbgb, strlen(dbgb));
            write(MINP_GLBL.msgfd, argv[iarg], lhn);
            write(MINP_GLBL.msgfd, "\n", 1);
            }
#endif
         if (lhn > lcc) {
            /* Make a continuation card */        
            pcc[-1] = '\n';
            if (write(cnssock, ccbuf, pcc-ccbuf) < 0)
               goto InputWriteError;
            ccbuf[0] = ' ', pcc = ccbuf + 1;
            lcc = MXXENTRY;
            } 
         memcpy(pcc, argv[iarg], lhn); pcc += lhn;
         if (iarg < argc-1)
            *pcc++ = ',', *pcc++ = ' ', lcc -= (lhn + 2);
         else
            *pcc++ = '\n', lcc -= (lhn+1);
         }
      if (write(cnssock, ccbuf, pcc-ccbuf) < 0)
         goto InputWriteError;
      } /* End vardefs */

#ifdef DBG_MINP
   strcpy(dbgmsg, "Ended writing Vardefs to cns socket.\n");
   write(MINP_GLBL.msgfd, dbgmsg, strlen(dbgmsg));
   fsync(MINP_GLBL.msgfd);
#endif

/* Copy the body of the input file.  If a SIGTERM has been
*  received, send an END card (it won't hurt if CNS gets two)
*  and then terminate.  Don't try to check the write, it's
*  probably too late to do anything special.  */

   CNSinf = fdopen(CNSin, "r");
   if (!CNSinf)
      abexitme(BBDcErrControl, "Error fdopening CNS control file");
   while (1) {
      if (MINP_GLBL.DoStop > 0) {
         write(cnssock, "END\n", 4);
         break; }
      if (!fgets(ccbuf, CDSIZE+1, CNSinf)) {
         if (feof(CNSinf)) break;
         else abexitme(BBDcErrControl,
            "Error reading CNS control file");
            }
      if (write(cnssock, ccbuf, strlen(ccbuf)) < 0)
         goto InputWriteError;
      }

#ifdef DBG_MINP
   strcpy(dbgmsg, "Finished writing control file to cns socket.\n");
   write(MINP_GLBL.msgfd, dbgmsg, strlen(dbgmsg));
   fsync(MINP_GLBL.msgfd);
#endif

/* Close the input file, but leave the control socket open
*  in the other direction (for the log).  */

   if (fclose(CNSinf))
      abexitme(BBDcErrControl, "Unable to close CNS control file");

   tstamp(tmstmp);
   write(MINP_GLBL.msgfd, tmstmp, strlen(tmstmp));
   write(MINP_GLBL.msgfd, " completed normally\n", 20);
   close(MINP_GLBL.msgfd);

   exit(0);

InputWriteError:
   abexitme(BBDcErrControl, "Error writing control file to CNS");

   } /* End bbdcminp() */
