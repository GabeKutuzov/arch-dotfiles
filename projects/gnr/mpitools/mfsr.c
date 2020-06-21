/* (c) Copyright 1992-2017, The Rockefeller University *21116* */
/* $Id: mfsr.c 7 2018-05-07 22:20:28Z  $ */
/***********************************************************************
*                     ROCKS metafile plot library                      *
*                                MFSR                                  *
*       Graphics metafile I/O server on UNIX host, MPI version         *
*                                                                      *
*  The mfsr process sits on the host and receives messages containing  *
*  graphics metafile commands from computational nodes.  Depending on  *
*  startup parameters, it does either or both of: writes the metafile  *
*  to an actual file or forwards the metafile to mfdraw for plotting.  *
*  This process is started from aninit which creates an intercommuni-  *
*  cator with which comp nodes can send messages to it. In principle,  *
*  this could be the only part of the graphics library on the host.    *
*                                                                      *
*  N.B.  The startup message sent from the new plot library has been   *
*  made as compatible as possible with the old one, but some changes   *
*  must be made in this program.  The never-used "Aspect Ratio" field  *
*  has been replaced with the length of a possibly added icon name     *
*  followed by an extra version field that may be given some function  *
*  in future versions of the library.  The separate field for the      *
*  length of the metafile buffer has been deleted--it was designed for *
*  IBM mainframes and is now always the same as the command buffer.    *
*  There may be other changes.                                         *
*                                                                      *
*----------------------------------------------------------------------*
* There are two invocation arguments (ASCII strings), viz:             *
*                                                                      *
* argv[1] - Number of MFB_CLOSE buffers to shutdown graphics           *
* argv[2] - Debug mask for mfdraw (merged with mask from setmf)        *
*----------------------------------------------------------------------*
* mfsr expects to receive a startup message from the PAR0 node.  The   *
* format of this message is defined in mfio.h by the MFSRSUMsg struct. *
* All data are ASCII strings so no swapping is needed.  Data from the  *
* startup message are copied into the very similar MFDSUMsg startup    *
* message for mfdraw.  (This is done in mfcreate on serial machines.)  *
* Receipt is acknowledged with a big-endian (legacy code) fullword     *
* error code (zero if startup is successful).                          *
*                                                                      *
* Debug info is written to a log file in /var/tmp sim. to mfdraw, as   *
* there is no MPI communicator path to andmsg.                         *
************************************************************************
*  Version 1, 05/05/92, ROZ                                            *
*  Rev, 07/18/93, GNR - Revise ack mechanism to work with new MFSynch  *
*  Rev, 08/11/93, GNR - Correct allocation for name and path           *
*  Rev, 12/06/93,  LC - Correct Metafile and XG display handling       *
*  Rev, 02/09/94,  LC - Revise acks, all errs sent back to nodes       *
*  Rev, 02/26/94, GNR - Omit writing null lines to metafile, cosmetics *
*  Rev, 03/10/94,  LC - Mod argv, rm CREATE buff msg to mfdraw         *
*  Rev, 03/10/94,  LC - Add MFPLOT_SAV env. var. to mfdraw argv        *
*  Rev, 09/16/94,  LC - Add win. aspect ratio to create buff, MFDraw   *
*  Rev, 11/24/96, GNR - Remove dependency on HYB compile-time def,     *
*                       remove NCUBE setup now in nioinit, add waitdbx *
*  V8A, 07/14/97, GNR - Revise startup parameters and message, switch  *
*                       mfdraw from pipe to socket interface, change   *
*                       "op-codes" to CountWord, make acks big-endian, *
*                       incorporate functionality of mfdiskio().       *
*  Rev, 08/06/97, GNR - Add window title, extra button msg at end.     *
*  Rev, 08/08/97, GNR - Start mfdraw via inetd                         *
*  Rev, 04/02/98, GNR - Intercept xgl errors in button messages        *
*  Rev, 07/26/98, GNR - Use new anread/anwrite intrinsic error exits   *
*  Rev, 05/22/99, GNR - Use new swapping scheme                        *
*  Rev, 03/01/08, GNR - Modify to agree with mfwrite in oldplot lib,   *
*                       allow online and metafile frames to have       *
*                       different indexes -- THIS CODE NOT TESTED      *
*  Rev, 06/28/16ff, GNR - Rewrite for MPI environment, use appexit()   *
*  ==>, 01/01/17, GNR - Last mod before committing to svn repository   *
*  Rev, 04/07/17, GNR - Move to ackbutt combined ack+buttons message   *
***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <errno.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#define  __USE_GNU
#include <netdb.h>
#include <signal.h>
#define MAIN
#include "sysdef.h"
#include "mpitools.h"
#include "swap.h"
#include "rksubs.h"
/* The definitions in these header that are used here should be
*  the same for the new and old plot libraries */
#include "plotdefs.h"
#include "mfio.h"

/* Define MFSRDBG here to get detailed debug info */
#define MFSRDBG

/* Static struct which holds acknowledgment error codes and button
*  information sent from mfdraw--defined in mfio.h.  */
static struct ackbutt ackb;      /* Initially 0 per C language spec */

/* Static data shared with error and signal handlers */
static int mfdstat = 0;          /* Status of mfdraw */
#define MFDNotYetUp  0           /* mfdraw not yet up */
#define MFDGotSocket 1           /* Got a socket, but not talking
                                 *  to mfdraw yet  */
#define MFDOperating 2           /* mfdraw is operational */
#define MFDDead      3           /* mfdraw alread gone */

#ifdef MFSRDBG
static FILE *pdbf;               /* Ptr to debug log FILE */
static char *tdbm;               /* Temp for debug messages */
#endif
static int sock;                 /* File descriptor for mfdraw socket */
static int src;                  /* Source node of received msg */
static int szbm;                 /* Size of button message */
static int vers;                 /* Plot lib version (8, 9, or later) */


/*---------------------------------------------------------------------*
*                          mfsrerr, mfsrerr2                           *
*                                                                      *
*  Call mfsrerr() on any kind of error after the startup message       *
*  from PAR0 has been successfully processed.  The argument must be    *
*  an mfckerr-style error code (as defined in mfio.h).  A generic      *
*  message text is provided.                                           *
*                                                                      *
*  To provide a specific message text along with the mfckerr-style     *
*  message, call mfsrerr2().  The arguments are a pointer to the       *
*  desired message text followed by the mfckerr-style error code.      *
*                                                                      *
*  If able to communicate with mfdraw, send it a message to terminate, *
*  otherwise leave it for user to clean up.  (Don't do a 'wait' on it, *
*  it may be running on a different processor.)  Finally, return       *
*  the error code as the next ack to PAR0 and terminate mfsr.          *
*---------------------------------------------------------------------*/

void mfsrerr2(char *mtxt, ui32 ecode) {

   int  i,ieb;
   int  rc;                      /* I/O return code */
   char scw[FMJSIZE];            /* Segment control word for mfdraw */
   static char endcmd[] = "]\n";

   bemfmi4(ackb.ackc, ecode);
   if (mfdstat == MFDDead) ackb.mfdb.intxeq |= BUT_DEAD;
   MPI_Send(&ackb, sizeof(ackb), MPI_UNSIGNED_CHAR, src,
      METFACK_MSG, NC.commc);

   switch (mfdstat) {

   case MFDOperating:
      bemfmi4(scw, MFB_CLOSE + 2);
      rc = write(sock, scw, FMJSIZE);
      if (rc < 0)
         appexit("MFSR: Unable to send term scw to mfdraw", 222, rc);
      /* No else, appexit is marked "noreturn" */
      if (vers == 8)
         rc = write(sock, endcmd, sizeof(endcmd));
      else
         rc = write(sock, endcmd, 1);
      if (rc < 0)
         appexit("MFSR: Unable to send term cmd to mfdraw", 222, rc);
      /* No break ... drop through to case MFDGotSocket */

   case MFDGotSocket:
      rc = close(sock);
      if (rc < 0)
         appexit("MFSR: Unable to close socket to mfdraw", 228, rc);
      break;

   case MFDNotYetUp:
   case MFDDead:
      break;

      } /* End mfdstat switch */

   for (i=0,ieb=1; i<ERRTOT; ++i,ieb<<=1) if (ecode & ieb)
      appexit(mtxt ? mtxt : "MFSR Error", ERRBEG+i, 0);
   appexit("MFSR: Unrecognized error code", 209, ecode);

   } /* End mfsrerr2() */

void mfsrerr(ui32 ecode) {
   mfsrerr2(NULL, ecode);
   } /* End mfsrerr() */


/*---------------------------------------------------------------------*
*                             sockrdck()                               *
*                                                                      *
*  Read an expected button message from the socket and terminate if    *
*  it instead contains an xgl error message.                           *
*---------------------------------------------------------------------*/

void sockrdck(void) {

   char *pbm = (char *)&ackb.mfdb;
   static char etxt0[MAX_EMSG_LENGTH];

   if (mfdstat == MFDDead) return;
   if (read(sock, pbm, szbm) < szbm) {
      if (errno == ECONNRESET) {
         mfdstat = MFDDead;
         ackb.mfdb.intxeq = BUT_DEAD;
         return; }
      else
         mfsrerr(ERR_BUTT);
      }
#ifdef MFSRDBG
   tdbm = ssprintf(NULL, "MFSR: Read buttmsg %4m\n", &ackb.mfdb);
   fwrite(tdbm, strlen(tdbm), 1, pdbf);
   fflush(pdbf);
#endif

   if (strncmp(pbm, "Error", szbm) == 0) {
      char *etxt, *etxte;
      int lmsg = strnlen(pbm, szbm);
      memcpy(etxt0, pbm, lmsg);
      etxt = etxt0 + lmsg, etxte = etxt0 + sizeof(etxt0) - 1;
      for ( ; ; ++etxt) {
         if (read(sock, etxt, 1) != 1) break;
         if (*etxt == '\0') break;
         if (memcmp(etxt-3, "\nErr", 4) == 0) {
            etxt -= 3; break; }
         if (etxt >= etxte) break; /* Exit before incrementing! */
         }
      *etxt = '\0';
      mfsrerr2(etxt0, ERR_RBUT);
      }
   } /* End sockrdck() */


/*---------------------------------------------------------------------*
*                                 i2a                                  *
*                                                                      *
*  DESCRIPTION:                                                        *
*     This function converts an integer to a string of digits.         *
*     'vstr' pointer to the destination string.                        *
*     'inp'  the integer to be converted.                              *
*     'size' the maximum size of the output string.                    *
*     'type' type of conversion, FORCESZ to pad left with zeros.       *
*                                                                      *
*  NOTES:                                                              *
*     This version marks the last digit by adding 'a' per metafile     *
*  spec.  This is the same as i2a in the oldplot library except does   *
*  not update buffer position in _NCG structure, which does not exist  *
*  in mfsr.                                                            *
*                                                                      *
*  Version 1, 06/09/92, ROZ                                            *
*  Rev, 07/20/93, GNR - Use max value if number doesn't fit            *
*  Rev, 02/29/08, GNR - Add type == FORCESZ, use div()                 *
*---------------------------------------------------------------------*/

#define FORCESZ 1

void i2a(unsigned char *vstr, int inp, int size, int type) {

   div_t q;
   int   sz;
   char  str[32];

   /* If negative number put '-' sign at beginning of vstr */
   if (inp < 0) {
      *vstr++ = '-';
      inp = (-inp);
      --size;
      }

   /* Make sure there is room for at least one digit */
   if (size <= 0) return;

   /* Calculate and count decimal digits.
   *  Note that str contains digits in reverse order. */
   for (sz=0; sz<size; sz++) {
      q = div(inp, 10);
      str[sz] = q.rem;
      if ((inp = q.quot) == 0) goto stage2;
      }
   /* Number is too long--replace with all nine's */
   --sz;
   while (sz--) *vstr++ = '9';
   *vstr++ = 'j';
   return;

   /* Store digits in output field */
stage2:
   if (type == FORCESZ) {
      --size; while (sz < size) str[++sz] = 0; }
   while (sz) *vstr++ = str[sz--] + '0';
   *vstr = str[0] + 'a';
   return;
   } /* End i2a() */


/*---------------------------------------------------------------------*
*                          mfsr main program                           *
*---------------------------------------------------------------------*/

int main(int argc, char *argv[]) {

   struct MFSRSUMsg *psum;    /* Ptr to start up message */
   byte  *tmp;                /* Ptr to variable-length items */
   byte  *pmb;                /* Ptr to message buffer */
   char  *disp;               /* Ptr to display name */
   char  *icon;               /* Ptr to icon name */
   char  *wtitl;              /* Ptr to window title */
   char  *mfname;             /* Ptr to metafile name */
   FILE  *mfdes;              /* FILE descriptor for metafile */
   AN_Status rstat;           /* Message received status */
   long  mfdlen;              /* Length of mfdraw command buffer */
   long  mfblen;              /* Length of metafile block */
   size_t lmsgbuff;           /* Length of mfdraw startyp message */
   size_t lnames;             /* Total length of names in startup */
   ui32  ack;                 /* Acknowledgment sent to comp array */
   ui32  CountWord;           /* Count word at start of each msg */
   ui32  mdbg;                /* Debug codes via startup msg */
   ui32  totcount;            /* Total metafile size (vers 8 */
   int   doXG;                /* TRUE for X graphics display */
   int   doMF;                /* TRUE to write metafile to disk */
   int   isfirst;             /* TRUE if reading first mf record */
   int   ldn;                 /* Length of display name */
   int   lfn;                 /* Length of metafile name */
   int   lin;                 /* Length of icon name */
   int   lwt;                 /* Length of window title */
   int   MovieMode;           /* Movie mode */
   int   MFFrame,XGFrame;     /* Metafile, XG frame numbers */
   int   CompltCount;         /* Number of nodes sending MFB_COMPLT */
   int   dostop;              /* TRUE when MFB_CLOSE received */
   int   rc,rc2;              /* Return code from MPI,sscanf call */
   int   src;                 /* Source node of input buffer */
   char  hdrbits;             /* Bits saved from frame header */

/* Initialization of mfsr */

#if 0
   {  volatile int wct=1;
      while (wct) {
         sleep(1);
         }
      }
#endif

   /* Prior to any other activity, initialize communication with MPI.
   *  N.B.  Do not turn on DBG_START here because no connection to
   *  andmsg from mfsr.  */
   aninit(0, 0, 0);
   MPI_Comm_get_parent(&NC.commmf);

#ifdef MFSRDBG
   /* Open a log file for debug messages */
   pdbf = fopen("/var/tmp/mfsr.debug.log", "w");
#endif

   /* Interpret command-line arguments */
   if (argc > 1) NC.debug = strtol(argv[2], NULL, 16);
   if (argc > 0) NC.total = atoi(argv[1]);
   else appexit("MFSR: Did not get node count argv[1]", 216, 0);

   /* Allocate a buffer to receive messages */
   pmb = malloc(MAX_MSG_LENGTH);
   if (!pmb) appexit("MFSR: Unable to alloc msg buffer", 210, 0);
   psum = (struct MFSRSUMsg *)pmb;

/* Receive and interpret startup messages from PAR0 node */

   rc = MPI_Recv(pmb, MAX_MSG_LENGTH, MPI_UNSIGNED_CHAR,
      MPI_ANY_SOURCE, MFSRSUM_MSG, NC.commmf, &rstat);
   if (rc) appexit("MFSR: Error reading startup msg", 260,
      rstat.MPI_ERROR);
   NC.hostid = rstat.MPI_SOURCE;

/* Check for special fake start message with doxg == 'X'.  This is
*  sent from endplt() when it is called before any newplt() calls,
*  i.e.  application aborted before doing any graphics.  mfsr just
*  needs to ack, Comm_disconnect, and finalize.  */

   if (pmb[1] == 'X') {
#ifdef MFSRDBG
      tdbm = "MFSR: Got abort startup msg\n";
      fwrite(tdbm, strlen(tdbm), 1, pdbf);
#endif
      /* Send ack to endplt */
      ack = 0;
      rc = MPI_Send(&ack, FMJSIZE, MPI_UNSIGNED_CHAR, NC.hostid,
         MFSRACK_MSG, NC.commmf);
      if (rc) mfsrerr2(ssprintf(NULL,
            "MFSR: MPI Send ack Error %d", rc), ERR_MACK);
#ifdef MFSRDBG
      tdbm = "Startup ack sent to endplt\n";
      fwrite(tdbm, strlen(tdbm), 1, pdbf);
      fflush(pdbf);
#endif
      MPI_Comm_disconnect(&NC.commmf);
      MPI_Finalize();
      free(pmb);
      return 0;
      } /* End aborted startup */

#ifdef MFSRDBG
   tdbm = ssprintf(NULL, "MFSR: Got startup msg: %s\n", pmb);
   fwrite(tdbm, strlen(tdbm), 1, pdbf);
   fwrite("\n", 1, 1, pdbf);
#endif

   rc = sscanf(pmb, "%2d%2d%4d%4d%4d%6ld%6ld%2d%4d",
      &doXG, &doMF, &ldn, &lfn, &lwt, &mfdlen, &mfblen,
      &MovieMode, &lin);
   rc2 = sscanf(psum->m_debug, "%9x", &mdbg);
   if (rc != 9 || rc2 != 1)
      appexit("MFSR: Unable to parse startup msg", 216, rc);

   /* Allocate space for the variable-length items */
   lnames = (size_t)(ldn + lfn + lwt + lin + 4);
   disp = malloc(lnames);
   if (!disp) appexit("MFSR: Unable to alloc vl items", 210, 0);
   mfname = disp + ldn + 1;
   wtitl = mfname + lfn + 1;
   icon = wtitl + lwt + 1;
   tmp = pmb + sizeof(struct MFSRSUMsg);

   /* If user specified a display in startup msg, read it */
   if (ldn > 0) {
      memcpy(disp, tmp, ldn); tmp += ldn;
      disp[ldn] = '\0';
#ifdef MFSRDBG
      tdbm = ssprintf(NULL, "  Disp = %*s\n", ldn, disp);
      fwrite(tdbm, strlen(tdbm), 1, pdbf);
#endif
      }

   /* If user specified a metafile name in startup msg, read it */
   if (lfn > 0) {
      memcpy(mfname, tmp, lfn); tmp += lfn;
      mfname[lfn] = '\0';
#ifdef MFSRDBG
      tdbm = ssprintf(NULL, "  Filename = %*s\n", lfn, mfname);
      fwrite(tdbm, strlen(tdbm), 1, pdbf);
#endif
      }

   /* If user specified a window title in startup msg, read it */
   if (lwt > 0) {
      memcpy(wtitl, tmp, lwt); tmp += lwt;
      wtitl[lwt] = '\0';
#ifdef MFSRDBG
      tdbm = ssprintf(NULL, "  WinTitl = %*s\n", lwt, wtitl);
      fwrite(tdbm, strlen(tdbm), 1, pdbf);
#endif
      }

   /* If user specified an icon name in startup msg, read it */
   if (lin > 0) {
      memcpy(icon, tmp, lin);
      icon[lin] = '\0';
#ifdef MFSRDBG
      tdbm = ssprintf(NULL, "  Icon = %*s\n", lin, icon);
      fwrite(tdbm, strlen(tdbm), 1, pdbf);
#endif
      }

   /* Merge any debug options from startup msg and driver */
   NC.debug |= mdbg;

   /* Store version number and Save size of button message */
   vers = memchr(psum->m_version, '8', sizeof(psum->m_version)) ? 8 : 9;
   szbm = (vers == 8) ? 4 : sizeof(struct mfdbm);

/* Startup message successfully received--send ack to PAR0.
*  (Here we keep the old 4-byte message, no button status yet
*  and no mfsrerr calls that could send a longer message.)  */

   ack = 0;                      /* Errors will accumulate here */
   bemfmi4(ackb.ackc, ack);
   rc = MPI_Send(ackb.ackc, FMJSIZE, MPI_UNSIGNED_CHAR, NC.hostid,
      MFSRACK_MSG, NC.commmf);
   if (rc) mfsrerr2(ssprintf(NULL,
         "MFSR: MPI Send ack Error %d", rc), ERR_MACK);
#ifdef MFSRDBG
   fwrite("End startup\n", 12, 1, pdbf);
   fflush(pdbf);
#endif

/* Currently, newplt assures that mfdlen == mfblen,
*  but this logic is a little safer if that ever changes. */

   if (mfblen > mfdlen) mfdlen = mfblen;
   if (!mfdlen) mfsrerr(ERR_BUFB);

/* If a physical metafile is to be written, open it now.
*  (This code quietly assumes that mfsr is running on a processor
*  that has access to the file system where the file is to go.)  */

   if (doMF) {
      if (!lfn) mfsrerr2("No file name", ERR_OSMF);
      if ((mfdes = fopen(mfname, "w")) == NULL)
         mfsrerr(ERR_OSMF);
#ifdef MFSRDBG
      tdbm = "Metafile opened\n";
      fwrite(tdbm, strlen(tdbm), 1, pdbf);
#endif
      } /* End opening metafile */

/* Start up mfdraw--code should have same effect as that in mfcreate.
*  But now if there is an error, can send nonzero ack code to PAR0.  */

   if (doXG) {

      struct sockaddr_in client;
      struct hostent *hp, *gethostbyname();
      struct MFDSUMsg *pdsm;        /* Ptr to mfdraw startup msg */
      char *home;                   /* Ptr to home directory name */
      int  lhn;                     /* Length of home dir name */

/* Create a socket and try to contact mfdraw.  If display name was not
*  specified, look for MFDRAW_HOST in env--if not found, try host we
*  are currently running on.  N.B.  mfdraw is only interested in the
*  screen spec part of the disp name--no need to update the host name
*  part if that is loaded from env or LocalHostIP here.  */

      sock = socket(AF_INET, SOCK_STREAM, 0);
      if (sock == -1) mfsrerr(ERR_CRSK);

      client.sin_family = AF_INET;
      client.sin_port = htons(MFDRAWPORT);

      if (ldn > 0) {
         /* Temporarily zap out the ':' screen identifier, get
         *  host ip, then restore ':'  */
         char *pcolon = strchr(disp, ':');
         if (pcolon) *pcolon = '\0';
         hp = gethostbyname(disp);
         if (pcolon) *pcolon = ':';
         }
      else {
         if (!(tmp = getenv("MFDRAW_HOST"))) tmp = LocalHostIP;
         hp = gethostbyname(tmp);
         }
      if (!hp)
         mfsrerr2("Bad display host", ERR_CNSK);
      memcpy((char *)&client.sin_addr, (char *)hp->h_addr,
         hp->h_length);
      if (connect(sock, (struct sockaddr *)&client,
            sizeof(client)) == -1)
         mfsrerr(ERR_CNSK);
      mfdstat = MFDGotSocket;
#ifdef MFSRDBG
      tdbm = "Got socket connection to mfdraw\n";
      fwrite(tdbm, strlen(tdbm), 1, pdbf);
#endif

/* Generate the mfdraw startup message and send it */

      /* If MFDRAW_HOME specified, use it as home directory */
      home = getenv("MFDRAW_HOME");
      if (!home) {
         /* Otherwise, support old style test of MFPLOT_SAV,
         *  which is valid if mfdraw host == mfcreate host */
         if ((tmp = getenv("MFPLOT_SAV")) != NULL) {
            if (!strcasecmp(tmp, "ON")) {
               if (!(home = getenv("HOME")))
                  mfsrerr(ERR_HOME);
               }
            }
         }
      lhn = home ? strlen(home) : 0;

      /* Generate an error if any of the parameters would overflow
      *  into padding between mfdraw startup message fields.  */
      if (ldn > MXSUML || lhn > MXSUML || lwt > MXSUML ||
         lin > MXSUML || mfdlen >= MXSUBL) mfsrerr(ERR_MFDO);

      /* Calc length and allocate space for mfdraw startup msg */
      lmsgbuff = sizeof(struct MFDSUMsg) + ldn + lhn + lwt + lin;
      pdsm = (struct MFDSUMsg *)malloc(lmsgbuff+1);
      if (!pdsm) mfsrerr(ERR_MEMA);

      /* Note:  mfcreate() uses ROCKS routines here because it is
      *  running in that environment.  mfsr() runs in a scanf()/
      *  printf() environment.  sprintf() has no 'c' format.  */
      sprintf(pdsm->vers, "%4d%4d%4d%4d%6ld%2d%4d",
         vers, ldn, lhn, lwt, mfdlen, MovieMode, lin);
      memcpy(pdsm->m_version, psum->m_version, sizeof(pdsm->m_version));
      sprintf(pdsm->z_debug, "%9x ", NC.debug);

      /* Append the variable-length data fields */
      tmp = (byte *)pdsm + sizeof(struct MFDSUMsg);
      if (ldn > 0) { memcpy(tmp, disp, ldn);  tmp += ldn; }
      if (lhn > 0) { memcpy(tmp, home, lhn);  tmp += lhn; }
      if (lwt > 0) { memcpy(tmp, wtitl, lwt); tmp += lwt; }
      if (lin > 0) { memcpy(tmp, icon, lin);  tmp += lin; }

      if (write(sock, pdsm, lmsgbuff) != lmsgbuff)
         mfsrerr(ERR_WRSK);
#ifdef MFSRDBG
      tdbm = "Wrote startup message to mfdraw\n";
      fwrite(tdbm, strlen(tdbm), 1, pdbf);
      fflush(pdbf);
#endif

/* Wait for mfdraw to ack successful startup */

      if (read(sock, pdsm->m_pad, 1) != 1) mfsrerr(ERR_RCHD);
      if ((schr)pdsm->m_pad[0] != -1)      mfsrerr(ERR_RCHD);
      mfdstat = MFDOperating;
#ifdef MFSRDBG
      tdbm = "Read one-byte mfdraw ack\n";
      fwrite(tdbm, strlen(tdbm), 1, pdbf);
      fflush(pdbf);
#endif

/* Send extra ack to PAR0 indicating successful startup of mfdraw.
*  Note:  If startup failed, this message would be sent instead
*  by one of the above calls to mfsrerr.  */

      bemfmi4(ackb.ackc, ack);
      ackb.mfdb.movie_mode = MovieMode;   /* Initial movie mode */
      rc = MPI_Send(&ackb, sizeof(ackb), MPI_UNSIGNED_CHAR, NC.hostid,
         MSDRACK_MSG, NC.commmf);
      if (rc) mfsrerr2(ssprintf(NULL,
         "MFSR: MPI Send ack Error %d", rc), ERR_MACK);
#ifdef MFSRDBG
      tdbm = ssprintf(NULL, "Extra ack = %d to host\n", ack);
      fwrite(tdbm, strlen(tdbm), 1, pdbf);
      fflush(pdbf);
#endif

      free(pdsm);
      } /* End mfdraw startup */

/* Allocate message buffer, including space for CountWord and one
*  extra byte, which will enable us to detect when a message is
*  in excess of the length declared at startup time.  */

   lmsgbuff = mfdlen + sizeof(CountWord) + 1;
   pmb = realloc(pmb, lmsgbuff);
   /* Technically, this mfsrerr call is wrong because we have not
   *  yet received a message that it could be an ack to.  However,
   *  the channel is supposed to be full duplex, so it should work.
   *  Also, this realloc will probably never fail, so it's enough
   *  that we die somehow, even if messy, when that happens.  */
   if (!pmb) mfsrerr(ERR_MEMA);

/*---------------------------------------------------------------------*
*  Infinite loop reading messages.  In order to improve parallelism,   *
*  we spoof the protocol by returning an ack as soon as a message is   *
*  received, before we have acted upon it.  The first ack is always    *
*  zero (meaning OK).  Subsequently, we return the ack generated by    *
*  the processing of the previous message.  An error ack may be        *
*  returned to a different node than the one whose message generated   *
*  the error, but it is equally effective at shutting down the app.    *
*                                                                      *
*  Program revised, 04/07/17, to sent button information and acks      *
*  together.  This reduces confusion on host node as to what is        *
*  expected, and reduces call overhead into MPI system.                *
*---------------------------------------------------------------------*/

   ack = 0;
   src = ANYNODE;             /* Changes on atomic buffers */
   totcount = 0;
   MFFrame = XGFrame = CompltCount = 0;
   isfirst = TRUE;            /* Do long search for '[' record */
   hdrbits = 0;               /* Allow both kinds of output */
   dostop = FALSE;

   while (!dostop) {
      char *pfrmno = NULL;    /* Ptr to old frame number */
      ui32 lmd;               /* Length of current message data */
      int  lfndx;             /* Length of coded frame index */
      int  nbytes;            /* Length of received data message */

      rc = MPI_Recv(pmb, lmsgbuff, MPI_UNSIGNED_CHAR,
         src, METFILE_MSG, NC.commmf, &rstat);
      if (rc) mfsrerr2(ssprintf(NULL,
         "MFSR: MPI Read Error %d", rstat.MPI_ERROR), ERR_MPIR);
      src = rstat.MPI_SOURCE;
      MPI_Get_count(&rstat, MPI_UNSIGNED_CHAR, &nbytes);
      /* Extract the Count Word */
      CountWord = bemtoi4(pmb);
      lmd = CountWord & MFB_COUNT;

#ifdef MFSRDBG
      tdbm = ssprintf(NULL, "Got msg from node %d, len = %d, "
         "CntWd = %jd,%jd, start = %8s\n", src, nbytes,
         CountWord>>28, lmd, pmb+4);
      fwrite(tdbm, strlen(tdbm), 1, pdbf);
#endif

      /* Create an error if the message is longer than mfdlen */
      if (nbytes >= lmsgbuff) ack |= ERR_BUFB;

      /* Acknowledge the incoming message at once.
      *  If ack is nonzero, initiate death.  */
      bemfmi4(ackb.ackc,ack);
      rc = MPI_Send(&ackb, sizeof(ackb), MPI_UNSIGNED_CHAR, src,
         METFACK_MSG, NC.commmf);
      if (ack) mfsrerr(ack);
      if (rc) mfsrerr2(ssprintf(NULL,
         "MFSR: MPI Send ack Error %d", rc), ERR_MACK);
#ifdef MFSRDBG
      tdbm = ssprintf(NULL, "Msg ack = %d to src = %d\n", ack, src);
      fwrite(tdbm, strlen(tdbm), 1, pdbf);
      fflush(pdbf);
#endif

      /* Set to exit after MFB_CLOSE msg received */
      if (CountWord & MFB_CLOSE) {
         dostop = TRUE;
#ifdef MFSRDBG
         tdbm = "MFB_CLOSE record received\n";
         fwrite(tdbm, strlen(tdbm), 1, pdbf);
         fflush(pdbf);
#endif
         }

      /* If this is the header of a new frame, extract and save
      *  the skip bits and the length of the coded frame index
      *  (only if this is a version 8 app). */
      else if (vers >= 9) {            /* New start record */
         /* 0x6C here is OpStart command left-shifted to first
         *  6 bits of a command buffer--see mfint.h  */
         if (isfirst || (pmb[I32SIZE] & MFB_OPMSK) == 0x6C) {
            hdrbits = 0;
            if (!doMF || !lmd) hdrbits |= MFB_SKPMF;
            if (!doXG)         hdrbits |= MFB_SKPXG;
            }
         }
      else {                           /* Version 8 start record */
         if (isfirst) {
            pfrmno = strchr(pmb+I32SIZE, '[');
            isfirst = FALSE;
            }
         else {
            pfrmno = pmb + I32SIZE;
            if (*pfrmno != '[') pfrmno = NULL;
            }
         if (pfrmno) {
            hdrbits = 0;
            if (!doMF || !lmd) hdrbits |= MFB_SKPMF;
            if (!doXG)         hdrbits |= MFB_SKPXG;
            for (lfndx=1; lfndx<UI32_SIZE; ++lfndx) {
               if (!isdigit(pfrmno[lfndx])) break;
               }
#ifdef MFSRDBG
            tdbm = ssprintf(NULL,"Found '[' msg, lfndx = %d\n",lfndx);
            fwrite(tdbm, strlen(tdbm), 1, pdbf);
            fflush(pdbf);
#endif
            }
         }
      /* Clean skip bits in version sent to mfdraw */
      pmb[0] &= ~(MFB_SKPXG|MFB_SKPMF);

/* If message is an MFB_COMPLT message, count nodes.  The normal
*  ack must go back to each sending node, then, when all nodes
*  have checked in, the button info must be retrieved from mfdraw
*  and returned to PAR0 node.  Until the count is fulfilled, the
*  MFB_COMPLT bit is turned off in the data sent to mfdraw so it
*  will not look at the buttons too soon.  */

      if (CountWord & MFB_COMPLT) {
         if (++CompltCount < NC.total) {
            pmb[0] &= ~(MFB_COMPLT>>24);
            }
#ifdef MFSRDBG
         else {
            tdbm = "MFB_COMPLT count reached NC.total\n";
            fwrite(tdbm, strlen(tdbm), 1, pdbf);
            fflush(pdbf);
            }
#endif
         }

/* Omit writing both outputs if no data */

      if (!lmd) goto SkipWriting;

/* Write message to the metafile if present and not skipped.  If this
*  is a new frame record in an old-format file, update frame index.
*  Easier to do whether necessary or not than to check.  See note
*  in newplt() regarding compatibility issure that frame numbers
*  must start with 0, not 1.  */

      if (!(hdrbits & MFB_SKPMF)) {
#ifdef BIT64
         if (vers == 8 && (totcount += lmd) > SI32_MAX)
            ack |= ERR_2GMF;
#else
         if ((totcount += lmd) > SI32_MAX)
            ack |= ERR_2GMF;
#endif
         if (pfrmno) {
            i2a(pfrmno+1,MFFrame++,lfndx,FORCESZ);
#ifdef MFSRDBG
            tdbm = ssprintf(NULL, "MFFrame = %d, start = %8s\n",
               MFFrame, pmb + I32SIZE);
            fwrite(tdbm, strlen(tdbm), 1, pdbf);
#endif
            }
         if (fwrite(pmb + I32SIZE, lmd, 1, mfdes) != 1)
            ack |= ERR_WSMF;
#ifdef MFSRDBG
         tdbm = ssprintf(NULL,
            "Data (size = %d) copied to metafile\n", lmd);
         fwrite(tdbm, strlen(tdbm), 1, pdbf);
#endif
         }

/* Write message to mfdraw() unless count and flags are all 0
*  (or it is already dead for some reason).  */

      if (!(hdrbits & MFB_SKPXG) && mfdstat == MFDOperating) {
         if (pfrmno) {
            i2a(pfrmno+1,XGFrame++,lfndx,FORCESZ);
#ifdef MFSRDBG
            tdbm = ssprintf(NULL, "XGFrame = %d, start = %8s\n",
               XGFrame, pmb + I32SIZE);
            fwrite(tdbm, strlen(tdbm), 1, pdbf);
#endif
            }
         if (write(sock, pmb, nbytes) != nbytes)
            ack |= ERR_WRSK;
#ifdef MFSRDBG
         tdbm = ssprintf(NULL,
            "Data (size = %d) copied to socket\n", nbytes);
         fwrite(tdbm, strlen(tdbm), 1, pdbf);
#endif
         }

/* If a frame has just been completed, pick up the button
*  status from mfdraw and return it to the parallel host node.  */

SkipWriting:
      if (CompltCount == NC.total) {

         if (doXG) sockrdck();

         /* Send ack and status of buttons to node 0 */
         rc = MPI_Send(&ackb, sizeof(ackb), MPI_UNSIGNED_CHAR, NC.hostid,
            METBUTT_MSG, NC.commmf);
         if (rc) mfsrerr2(ssprintf(NULL,
            "MFSR: MPI Send buttons Error %d", rc), ERR_MACK);
         CompltCount = 0;
#ifdef MFSRDBG
         tdbm = ssprintf(NULL,
            "ackbutt = %d,%4m sent to host\n", &ackb.ackc, &ackb.mfdb);
         fwrite(tdbm, strlen(tdbm), 1, pdbf);
         fflush(pdbf);
#endif
         }

/* If not atomic, set src back to ANYNODE.  In the version 8 library,
*  a 'BEGATM/ENDATM' machanism was documented but never implemented.
*  It would be easier if needed to use the MFB_ATOM mechanism of
*  version 9, so just check that here without checking version... */

      if (!(CountWord & MFB_ATOM)) src = ANYNODE;

      }  /* End grand loop */

/* Main loop terminates after an MFB_CLOSE msg has been processed.
*  At this point, the end-metafile record has already been written
*  to the file and sent on to mfdraw.  All that is left to do is
*  to close the metafile, wait for mfdraw to send final buttons,
*  close the mfdraw socket, send the final button ack to the host
*  node along with a regular error ack, free memory, and terminate.
*  As in the main loop, we send dummy button status if doXG is off.  */

   if (doMF && fclose(mfdes) != 0) {
      ack |= ERR_CSMF;
#ifdef MFSRDBG
      tdbm = "Metafile closed\n";
      fwrite(tdbm, strlen(tdbm), 1, pdbf);
#endif
      }

   if (doXG && mfdstat < MFDDead) {
      sockrdck();
      if (close(sock) < 0)
         ack |= ERR_CLSK;
#ifdef MFSRDBG
      tdbm = "Socket closed\n";
      fwrite(tdbm, strlen(tdbm), 1, pdbf);
#endif
      }

   bemfmi4(ackb.ackc, ack);
   rc = MPI_Send(&ackb, sizeof(ackb), MPI_UNSIGNED_CHAR, NC.hostid,
      METBUTT_MSG, NC.commmf);
   if (rc) mfsrerr2(ssprintf(NULL,
      "MFSR: Send final buttons Error %d", rc), ERR_MACK);
#ifdef MFSRDBG
   tdbm = ssprintf(NULL,
      "Final ackbutt = %d,%4m sent to host\n", &ackb.ackc, &ackb.mfdb);
   fwrite(tdbm, strlen(tdbm), 1, pdbf);
   fclose(pdbf);
#endif

   MPI_Comm_disconnect(&NC.commmf);
   MPI_Finalize();

   free(pmb);
   free(disp);

   return 0;

   } /* End main */

