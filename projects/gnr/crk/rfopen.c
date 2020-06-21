/* (c) Copyright 1992-2017, The Rockefeller University *11115* */
/* $Id: rfopen.c 63 2017-04-13 20:47:00Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                               rfopen                                 *
*                                                                      *
*     This routine opens a ROCKS file and allocates buffers.  The      *
*  method of doing I/O is controlled by the RFM_xxxx compile-time      *
*  variables, one of which must be defined in sysdef.h for each        *
*  operating system (currently fopen on IBM, OS9, SPARC, fdopen on     *
*  none (was on IRIX), and open on OSX and LINUX).                     *
*                                                                      *
*  Synopsis:                                                           *
*  int rfopen(struct RFdef *fd, char *fname1, int inout1, int fmt1,    *
*     int accmeth1, int append1, int look1, int norew1, int            *
*     retbuf1, size_t blksize1, size_t lrecl1, long nrp1, int ierr)    *
*                                                                      *
*  Arguments:                                                          *
*  (Note: Arguments that are irrelevant with a particular file type    *
*  or operating system are ignored and may be coded as IGNORE (0).     *
*  Generally, if IGNORE is coded in both rfallo() and rfopen() calls,  *
*  the value coded as -1 is taken as the default.  SAME (0) may be     *
*  coded to use a value previously specified in an rfallo() or an      *
*  earlier rfopen() call on the same file.)                            *
*     fd       Pointer to an RFdef block previously created with an    *
*              rfallo() call.                                          *
*     fname1   Name for this file.  If accmeth MK_TEMP bit is set,     *
*              must be an array that can be written to and must end    *
*              in XXXXXX.  Must be FQDN host name if an initiator      *
*              socket, or NULL if a listener socket or a regular file  *
*              and the name was given in the rfallo() call.            *
*     inout1   READ (-1) for an input file, WRITE (+1) for an output   *
*              file, READWRITE (+2) to do both, or SAME (0) to use     *
*              the mode given in the rfallo() call.                    *
*     fmt1     BINARY (-1) for a binary file, TEXT (+1) for a text     *
*              file.  Use the special values SysIn (5), SysOut (6),    *
*              or SysErr (7) to access stdin, stdout, stderr on UNIX   *
*              systems (as text) or some equivalent on other systems.  *
*     accmeth1 SEQUENTIAL (0) for ordinary sequential files, DIRECT    *
*              (1) for direct access (seek will be used or IBM DASD),  *
*              INITIATOR (2) to initiate (connect) a socket, LISTENER  *
*              (4) to listen on a socket, +ON_LINE (8) if online (a    *
*              backspace (^H) in the input then causes the last char-  *
*              acter to be erased), MK_TEMP (16) to create, open, and  *
*              unlink a temp file using the given fname as template.   *
*              This argument must be identical to the value given in   *
*              the rfallo() call unless it was coded as ACC_DEFER (-1) *
*         N.B. Note added 02/02/17:  Unfortunately, SAME (0) has the   *
*              same value as SEQUENTIAL.  Therefore SAME gives error   *
*              accmeth changed if rfallo did not specify SEQUENTIAL.   *
*              I am very reluctant to change either definition due to  *
*              extensive use.  Instead, I will treat accmeth == 0 as   *
*              SAME rather than SEQUENTIAL.  Downside:  No error if    *
*              caller tried to change something else to SEQUENTIAL.    *
*     append1  TOP (-1) to start at the beginning of the file, BOTTOM  *
*              (1) to append to the end of the file if writing.        *
*     look1    NO_LOOKAHEAD (-1) if no lookahead reading should be     *
*              attempted (e.g. reading a socket), LOOKAHEAD (+1) if    *
*              lookahead read buffering is useful (e.g. magtape).      *
*     norew1   REWIND (-1) if file should be repositioned at start     *
*              when closed (really only meaningful for magtape),       *
*              NO_REWIND (+1) to leave file position unchanged when    *
*              closing.                                                *
*     retbuf1  RELEASE_BUFF (-1) to release buffers when the file is   *
*              closed, RETAIN_BUFF (+1) to retain them (useful if the  *
*              file will be reopened later).                           *
*     blksize1 Buffer size for file buffering: may be coded as IGNORE  *
*              to use a default system buffer (usually 4096).  If this *
*              is a new IBM MVS file, blksize1 is required to give the *
*              MVS BLKSIZE for the file.                               *
*     lrecl1   Required to give LRECL for for certain IBM MVS files,   *
*              otherwise may be coded as IGNORE.                       *
*     nrp1     If this is a direct access output file, gives the num-  *
*              ber of records expected to be written.  (This may be    *
*              used to preallocate storage space for IBM DASD files    *
*              and possibly other types, otherwise ignored.)  If this  *
*              is a socket file, gives the port to connect or listen   *
*              on.  In case of a LISTENER file, this argument must be  *
*              SAME (0) or identical to the value given in the rfallo  *
*              call.  ("nrp" stands for "number of records or port".)  *
*     ierr     Error flag.  NO_ABORT (-1) if routine should simply     *
*              return zero on failure, ABORT (0) to terminate with     *
*              an abexit call and message if there is an error, and    *
*              NOMSG_ABORT to terminate with no attempt to print an    *
*              error message (use this if opening the print file!).    *
*              The ABORT message includes an error code which is       *
*              documented in the text returned by the "abexit" utility *
*              program.                                                *
*                                                                      *
*  Returns:                                                            *
*     File descriptor for the opened file, or zero if there was an     *
*     error and ierr was NO_ABORT.                                     *
*                                                                      *
************************************************************************
*                                                                      *
*  Following are the times taken to write a file of 10000000 bytes     *
*  in chunks of 1000 or 10 bytes using various I/O calls.  Based on    *
*  these results, the rfxxxx.c routines were rewritten to perform      *
*  their own buffering using faster of read/fread etc.                 *
*  Test (times in seconds)        SunOS/acc     PC/tcc      SGI/cc     *
*  Use read/write buffering         6.1158       24.06      0.4255     *
*     Same, chunks of 10          205.2893      194.93     32.6506     *
*  Use fread/fwrite buffering       4.2183       66.35      0.1867     *
*     Same, chunks of 10            9.8067       72.99      2.1024     *
*  Use faster of above I/O calls                                       *
*     with buffering in rfxxx       4.7657       23.01      0.2309     *
*     Same, chunks of 10           12.0743       33.50      1.9133     *
*  Same, optimized compile -O2      4.7528       FAILS      0.2157     *
*     Same, chunks of 10            8.0417       FAILS      1.6434     *
*                                                                      *
*  Additional tests, Aug. 1999, on 100 MHz Pentium        Sparc/LX     *
*                                PCLINUX/gcc    PC/tcc     Solaris     *
*  write(), chunks of 1000          1.6299      214.65      5.9226     *
*  write(), chunks of 10           16.5359       48.94    346.3460     *
*  fwrite(), chunks of 1000         2.6043       17.79      3.4327     *
*  fwrite(), chunks of 10           5.2043       18.79      7.3774     *
*  rftest, -O2, write               0.0788        3.07                 *
*  rftest, -O2, fwrite              0.1518        1.21      0.4559     *
*  rftest, -O2, read                0.1055        0.28                 *
*  rftest, -O2, fread               0.1040        1.04      0.2406     *
*  The above results for tcc are somewhat mysterious, but read()       *
*  write() was kept because much faster for reading via rfread().      *
*  -O and -v were not much different from -O2 (data not shown).        *
*                                                                      *
************************************************************************
*  V1A, 02/29/92, GNR - Broken out from filelib.c, use n_fopen (NCUBE) *
*  Rev, 06/25/92, ABP - Fake O_BINARY, O_TEXT under SunOS              *
*  Rev, 03/15/95, GNR - Add READWRITE code, set O_TRUNC if write-only  *
*  Rev, 01/30/97, GNR - Change 'char' args to int type                 *
*  Rev, 08/30/97, GNR - Implement self-buffering on all systems        *
*  Rev, 09/13/97, GNR - Implement fopen as open followed by fdopen     *
*  Rev, 11/05/97, GNR - Eliminate cryout call, use abexitm()           *
*  Rev, 08/19/98, GNR - Add SysIn, SysOut, SysErr fmt codes            *
*  Rev, 07/04/99, GNR - RFM_method method control and iamopen types    *
*  Rev, 09/04/99, GNR - Remove NCUBE support                           *
*  Rev, 03/01/07, GNR - Add support for socket buffering               *
*  Rev, 09/25/08, GNR - Minor type changes for 64-bit compilation      *
*  ==>, 09/25/08, GNR - Last date before committing to svn repository  *
*  Rev, 01/17/11, GNR - Retry accept() if EINTR error                  *
*  Rev, 03/24/13, GNR - Add pound-def __USE_GNU for h_addr def         *
*  Rev, 04/16/13, GNR - Add SysFNms to better identify error files     *
*  Rev, 06/17/13, GNR - Correct bug:  Renamed fd->fname to fname2      *
*  Rev, 01/11/17, GNR - Add MK_TEMP access method                      *
*  Rev, 02/02/17, GNR - Kludge for SAME == SEQUENTIAL access           *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "sysdef.h"
#ifdef UNIX
#include <fcntl.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#define __USE_GNU
#include <sys/stat.h>
#include <sys/socket.h>
#endif
#include "rfdef.h"
#include "rksubs.h"

/*=====================================================================*
*                               rfopen                                 *
*=====================================================================*/

int rfopen(struct RFdef *fd, char *fname1, int inout1, int fmt1,
   int accmeth1, int append1, int look1, int norew1,
   int retbuf1, size_t blksize1, size_t lrecl1, long nrp1, int ierr) {

   void (*abex)(int, char *) = abexitm;   /* Error exit selector */
   char *fname2;           /* File name for error message */
   size_t blksize0;        /* Existing block and buffer size */
   int kexit;              /* Subcode for ERROR99 */
#if defined(RFM_OPEN) || defined(RFM_FDOPEN) || defined (UNIX)
   int kopen;              /* Arg for open call */
#endif
#if defined(RFM_FOPEN) | defined(RFM_FDOPEN)
   char kopened;           /* Bits for fd->iamopen */
#endif

   static char *SysFNms[] =
      { "sysin", "sysout", "stderr", "(unnamed)" };
#define UnNamedNum   3

/* Set fname2 before any possible error exits */

   if (!(fname2 = fname1)) {           /* Assignment intended */
      if (fmt1 >= SysIn && fmt1 <= SysErr)
         fname2 = SysFNms[fmt1 - SysIn];
      else if (fd->fname) fname2 = fd->fname;
      else fname2 = SysFNms[UnNamedNum];
      }

/* If already open, close it first--may be changing options */

   if (!fd)
      { kexit = 1; goto ERROR99; }     /* A little safety check */
   if (fd->iamopen & IAM_ANYOPEN && rfclose(fd, norew1, retbuf1, ierr))
      { kexit = 2; goto ERROR99ME; }   /* Unable to close file */
   fd->iamopen &= ~IAM_ANYOPEN;

/*---------------------------------------------------------------------*
*  Update field values.  Do not allow access method to change          *
*  (except ON_LINE bit or ACC_DEFER).  Allocate a buffer.              *
*---------------------------------------------------------------------*/

   if (accmeth1 < 0)
      { kexit = 16; goto ERROR99; }
   /* Skip all these tests if accmeth1 == SAME, see note above */
   if (accmeth1 > 0) {
      if (fd->accmeth == ACC_DEFER)
         fd->accmeth = accmeth1;
      else if ((accmeth1 ^ fd->accmeth) & ~ON_LINE)
         { kexit = 3; goto ERROR99; }  /* Tried to change accmeth */
      else
         fd->accmeth = accmeth1;       /* May change ON_LINE bit */
      }

   if (inout1)   fd->inout   = (char)inout1;
   if (fmt1)     fd->fmt     = (char)fmt1;
   if (retbuf1)  fd->retbuf  = (char)retbuf1;

   blksize0 =    fd->blksize;
   if (blksize1) fd->blksize = blksize1;
   if (fd->blksize == 0) fd->blksize = DFLT_BLKSIZE;

   /* Get rid of existing buffer if wrong size */
   if (fd->buff && fd->blksize != blksize0) {
      freev(fd->buff, "File buffer");
      fd->buff = NULL;
      }
   if (!fd->buff &&
          ((fd->buff = mallocv(fd->blksize, NULL)) == NULL))
      { kexit = 4; goto ERROR99; }

/*---------------------------------------------------------------------*
*                           Handle sockets                             *
*---------------------------------------------------------------------*/

/* If access method is initiator, file name is really host name.
*  File name and port can be specified here for first time, but
*  never changed.  If no host name is given, use current running
*  host.  If access method is listener, file name is never used.
*  Port must be specified in rfallo() and cannot be changed here.
*  Some other items are forced to appropriate values, quietly
*  ignoring the input parameters.  */

   if (fd->accmeth & (INITIATOR|LISTENER)) {
#ifdef UNIX

      fd->append = BOTTOM;
      fd->look   = NO_LOOKAHEAD;
      fd->norew  = NO_REWIND;

      if (fd->accmeth & INITIATOR) {

         struct sockaddr_in initaddr;
         struct hostent *hp, *gethostbyname();

         if (fname1) {              /* Check host name */
            if (fd->fname) {
               if (strcmp(fname1, fd->fname))
                  { kexit = 5; goto ERROR99; }
               }
            else {
               if (!(fd->fname = mallocv(strlen(fname1)+1, NULL)))
                  { kexit = 6; goto ERROR99; }
               strcpy(fd->fname, fname1);
               }
            }
         else if (!fd->fname) {
            char hnm[MAX_FQDN_LENGTH+1];
            if (gethostname(hnm, MAX_FQDN_LENGTH) < 0)
               { kexit = 7; goto ERROR99; }
            fname2 = hnm;
            if (!(fd->fname = mallocv(strlen(hnm)+1, NULL)))
               { kexit = 6; goto ERROR99; }
            strcpy(fd->fname, hnm);
            }

         if (nrp1) {                /* Check port */
            if (fd->URF.SOCK.port) {
               if (nrp1 != fd->URF.SOCK.port)
                  { kexit = 8; goto ERROR99; }
               }
            else
               fd->URF.SOCK.port = nrp1;
            }
         else if (!fd->URF.SOCK.port)
               { kexit = 8; goto ERROR99; }

         initaddr.sin_family = AF_INET;
         initaddr.sin_port = htons(fd->URF.SOCK.port);

         if (!(hp = gethostbyname(fd->fname)))
            { kexit = 9; goto ERROR99; }
         memcpy((char *)&initaddr.sin_addr, (char *)hp->h_addr,
            hp->h_length);

         if (connect(fd->frwd = fd->URF.SOCK.isfd,
               (struct sockaddr *)&initaddr, sizeof(initaddr)) < 0)
            { kexit = 10; goto ERROR99ME; }
         }

      else {                  /* LISTENER */

         socklen_t alen = 0;        /* For the accept call */

         /* rfallo() gives error if port not specified there.  */
         if (nrp1 && nrp1 != fd->URF.SOCK.port)
            { kexit = 8; goto ERROR99; }

         /* Accept socket connection back from server.  Block
         *  until server has something to send us.  */
         while (1) {
            int arc = accept(fd->URF.SOCK.isfd, NULL, &alen);
            if (arc >= 0) { fd->frwd = arc; break; }
            if (errno != EINTR) { kexit = 11; goto ERROR99ME; }
            }  /* Try again if accept() was interrupted */
         }

      fd->iamopen |= IAM_OPEN;

#else /* Not UNIX */
      kexit = 12; goto ERROR99;
#endif
      }

/*---------------------------------------------------------------------*
*                            Not a socket                              *
*---------------------------------------------------------------------*/

/* If not a socket, user can now change file name and most other
*  parameters.  If file name is still missing after this, and file
*  is not an "always-open" system file, error is caught by open.  */

   else {
      if (fname1 && (!fd->fname || strcmp(fname1, fd->fname))) {
         fd->fname =
            reallocv(fd->fname, strlen(fname1)+1, "File name");
         strcpy(fd->fname, fname1);
         }
      if (append1)  fd->append  = (char)append1;
      if (look1)    fd->look    = (char)look1;
      if (norew1)   fd->norew   = (char)norew1;
      if (lrecl1)   fd->URF.DASD.lrecl = lrecl1;
      if (nrp1)     fd->URF.DASD.numrec = nrp1;


#if defined(RFM_OPEN) || defined(RFM_FDOPEN)

/*---------------------------------------------------------------------*
*  If on UNIX, use open with appropriate access flags.                 *
*---------------------------------------------------------------------*/

/* Add needed flags into access argument */

      if (fd->inout == (char)READWRITE)
         kopen = O_RDWR | O_CREAT;
      else if (fd->inout == (char)WRITE)
         kopen = O_CREAT | O_WRONLY | O_TRUNC;
      else
         kopen = O_RDONLY;
      if (fd->append >= (char)BOTTOM)
         kopen |= O_APPEND;

#if defined(RFM_FDOPEN)

/*---------------------------------------------------------------------*
*  In RFM_FDOPEN mode, now use fdopen() to change the ordinary open()  *
*  into an fopen() (except for online sysin or MK_TEMP files).  This   *
*  maneuver allows the O_TRUNC function to be invoked on WRITE files,  *
*  which is otherwise not available with fopen().  This gets rid of    *
*  annoying junk left over when a longer file is overwritten with a    *
*  shorter one.                                                        *
*---------------------------------------------------------------------*/

      kopened = IAM_OPEN | IAM_FOPEN;
      if (fd->accmeth & MK_TEMP) {
         if ((fd->frwd = mkstemp(fd->fname)) < 0)
            { kexit = 17; goto ERROR99ME; }
         if (unlink(fd->fname) < 0)
            { kexit = 18; goto ERROR99ME; }
         fd->pfil = NULL;
         kopened = IAM_OPEN;
         }
      else if (fd->fmt >= SysIn) {
         switch (fd->fmt - SysIn) {
         case 1: fd->pfil = stdin;
               kopened = IAM_OPEN;
               break;
         case 2: fd->pfil = stdout;
               break;
         case 3: fd->pfil = stderr;
               break;
            } /* End switch */
         fd->frwd = fileno(fd->pfil);
         }
      else {                     /* Not SysIn type file */
         char *omode;            /* Output mode */

         fd->frwd = open(fd->fname, kopen, S_IREAD | S_IWRITE);
         if (fd->frwd < 0)
            {kexit = 13; goto ERROR99ME; }

         if (fd->inout == (char)READ)
            omode = "r";
         else if (fd->inout == (char)WRITE)
            omode = (fd->append >= (char)BOTTOM) ? "a" : "w";
         else
            omode = (fd->append >= (char)BOTTOM) ? "a+" : "r+";
         fd->pfil = fdopen(fd->frwd, omode);
         if (!fd->pfil)
            { kexit = 14; goto ERROR99ME; }
         }
      fd->iamopen |= kopened;

#else /* System using read/write for all files */

/*---------------------------------------------------------------------*
*     Some systems do better just using read/write for all files.      *
*---------------------------------------------------------------------*/

      if (fd->accmeth & MK_TEMP) {
         if ((fd->frwd = mkstemp(fd->fname)) < 0)
            { kexit = 17; goto ERROR99ME; }
         if (unlink(fd->fname) < 0)
            { kexit = 18; goto ERROR99ME; }
         }
      else if (fd->fmt >= SysIn)
         fd->frwd = fd->fmt - SysIn;
      else {
#if defined(_ISOC99_SOURCE)
         fd->frwd = open(fd->fname, kopen, S_IRUSR | S_IWUSR);
#else
         fd->frwd = open(fd->fname, kopen, S_IREAD | S_IWRITE);
#endif
         if (fd->frwd < 0)
            { kexit = 15; goto ERROR99ME; }
         }
      fd->iamopen |= IAM_OPEN;
#endif

#else    /* Not RFM_OPEN or RFM_FDOPEN */

/*---------------------------------------------------------------------*
*       All other cases, use fopen (should be POSIX compatible)        *
*  Here we don't know whether the TEXT vs BINARY mode distinction is   *
*  recognized, so omit it.  Some versions of fopen allow a 't' or 'b'  *
*  to be appended to the mode string--this could be added in case      *
*  needed for future implementations.                                  *
*---------------------------------------------------------------------*/

      kopened = IAM_OPEN | IAM_FOPEN;
      if (fd->accmeth & MK_TEMP) {
         if ((fd->frwd = mkstemp(fd->fname)) < 0)
            { kexit = 17; goto ERROR99ME; }
         if (unlink(fd->fname) < 0)
            { kexit = 18; goto ERROR99ME; }
         fd->pfil = NULL;
         kopened = IAM_OPEN;
         }
      else if (fd->fmt >= SysIn) {
         switch (fd->fmt - SysIn) {
         case 0: fd->pfil = stdin;
               kopened = IAM_OPEN;
               break;
         case 1: fd->pfil = stdout;
               break;
         case 2: fd->pfil = stderr;
               break;
            } /* End switch */
         fd->frwd = fileno(fd->pfil);
         } /* End standard sys files */
      else {
#ifdef IBM370
         char omode[32];      /* Output mode */
#else
         char omode[4];       /* Output mode */
#endif

         if (fd->inout < (char)WRITE)
            strcpy(omode, "r");
         else {
            strcpy(omode, (fd->append < (char)BOTTOM) ? "w" : "a");
            if (fd->accmeth >= (char)DIRECT) strcat(omode, "+");
            }
#if 0
         if (fd->fmt < (char)TEXT) strcat(omode, "b");
#endif

#ifdef IBM370
         strcat(omode, (fd->fmt<(char)TEXT) ? ",recfm=f" : ",recfm=v");
         if (fd->URF.DASD.lrecl > 0) strcat(omode,
            ssprintf(NULL, ",lrecl=%6ld", (long)fd->URF.DASD.lrecl));
#endif

         fd->pfil = fopen(fd->fname, omode);
         if (!fd->pfil)
            { kexit = 15; goto ERROR99ME; }
         fd->frwd = fileno(fd->pfil);
         } /* End scope of omode */

      fd->iamopen |= kopened;

#endif   /* Not RFM_OPEN or RFM_FDOPEN (UNIX) */

      } /* End not a socket */

/*---------------------------------------------------------------------*
*                              Finish up                               *
*---------------------------------------------------------------------*/

/* Initialize buffer controls to indicate no contents in a way that
*  will work correctly whether a read or write is attempted first.  */

   fd->aoff = 0;
   fd->lbsr = 0;
   fd->bptr = fd->buff;

   return fd->frwd;


/*---------------------------------------------------------------------*
*  Error exits--terminate with abexit if ierr >= ABORT                 *
*---------------------------------------------------------------------*/

ERROR99ME:
   abex = abexitme;
ERROR99:
   if (ierr == ABORT)
      abex(99, ssprintf(NULL, "Error %10d opening file %67s.",
         kexit, fname2));
   else if (ierr == NOMSG_ABORT)
      exit(99);
   fd->rferrno = kexit;
   return 0;

   } /* End rfopen() */
