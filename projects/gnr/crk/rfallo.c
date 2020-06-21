/* (c) Copyright 1992-2017, The Rockefeller University *11115* */
/* $Id: rfallo.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                               rfallo                                 *
*                                                                      *
*     The ROCKS I/O routines provide a standard interface to UNIX and  *
*  UNIX-like I/O systems that permit an additional layer of functions  *
*  to be interposed between the user and the system.  It allows IBM    *
*  MVS files to be accessed using the same paradigm as UNIX files.     *
*                                                                      *
*     This routine, rfallo, creates an RFdef control block for a file, *
*  but does NOT open the file.  rfopen() must be called to do that.    *
*  In the case of a socket device, rfallo obtains the socket.  If it   *
*  is an initiator, rfopen() will issue the connect().  If it is a     *
*  listener, rfallo binds the socket and issues the listen(), but      *
*  leaves rfopen() to do the accept(), which may block until the       *
*  initiator issues its connect().                                     *
*                                                                      *
*  Synopsis:                                                           *
*  struct RFdef *rfallo(char *fname1, int inout1, int fmt1,            *
*     int accmeth1, int append1, int look1, int norew1, int            *
*     retbuf1, size_t blksize1, size_t lrecl1, long nrp1, int ierr)    *
*                                                                      *
*  Arguments:                                                          *
*  (Note: Arguments that are irrelevant with a particular file type    *
*  or operating system are ignored and may be coded as IGNORE (0).     *
*  IGNORE may also be coded to defer specification of an option to     *
*  the rfopen() call.  Generally, if IGNORE is coded in both rfallo()  *
*  and rfopen() calls, the value coded as -1 is taken as the default.) *
*     fname1   Name for this file.  If accmeth MK_TEMP bit is set,     *
*              must be an array that can be written to and must end    *
*              in XXXXXX.  Must be FQDN host name if an initiator      *
*              socket, or NULL if a listener socket or a regular file  *
*              and specification of the name is deferred to rfopen().  *
*     inout1   READ (-1) for an input file, WRITE (+1) for an output   *
*              file, READWRITE (+2) to do both, or IGNORE (0) to       *
*              postpone the designation to rfopen().                   *
*     fmt1     BINARY (-1) for a binary file, TEXT (+1) for a text     *
*              file.  Use the special values SysIn (5), SysOut (6),    *
*              or SysErr (7) to access stdin, stdout, stderr on UNIX   *
*              systems (as text) or some equivalent on other systems.  *
*     accmeth1 ACC_DEFER (-1) to defer setting until rfopen() call,    *
*              SEQUENTIAL (0) for ordinary sequential files, DIRECT    *
*              (1) for direct access (seek will be used or IBM DASD),  *
*              INITIATOR (2) to initiate (connect) a socket, LISTENER  *
*              (4) to listen on a socket, +ON_LINE (8) if online (a    *
*              backspace (^H) in the input then causes the last char-  *
*              acter to be erased), MK_TEMP (16) to create and open a  *
*              temp file using the given fname as template.            *
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
*              on.  In case of a LISTENER file, this argument is       *
*              required, otherwise it can be coded as IGNORE.  ("nrp"  *
*              stands for "number of records or port".)                *
*     ierr     Error flag.  NO_ABORT (-1) if routine should simply     *
*              return NULL on failure, ABORT (0) to terminate with     *
*              an abexit call and message if there is an error, and    *
*              NOMSG_ABORT to terminate with no attempt to print an    *
*              error message (use this if opening the print file!).    *
*              The ABORT message includes an error code which is       *
*              documented in the text returned by the "abexit" utility *
*              program.                                                *
*                                                                      *
*  Returns:                                                            *
*     Pointer to file record if successfully allocated or NULL if      *
*     there was an error and ierr was NO_ABORT.                        *
*                                                                      *
************************************************************************
*  V1A, 02/29/92, GNR - Broken out from filelib.c, renamed             *
*  Rev, 01/30/97, GNR - Change all 'char' args to int type             *
*  Rev, 11/05/97, GNR - Eliminate cryout call, use abexitm()           *
*  Rev, 03/01/07, GNR - Add support for socket buffering               *
*  Rev, 09/24/08, GNR - Type changes for 64-bit compilation            *
*  ==>, 09/24/08, GNR - Last date before committing to svn repository  *
*  Rev, 01/17/11, GNR - Add setsockopt(0) call and error 8.            *
*  Rev, 04/16/13, GNR - Add SysFNms to better identify error files     *
*  Rev, 04/07/16, GNR - Add error 109 for socket port in use           *
*  Rev, 01/11/17, GNR - Add MK_TEMP access method                      *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#ifdef UNIX
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#endif
#include "rfdef.h"
#include "rksubs.h"

/*=====================================================================*
*                              rfallo()                                *
*=====================================================================*/

struct RFdef *rfallo(char *fname1, int inout1, int fmt1,
   int accmeth1, int append1, int look1, int norew1,
   int retbuf1, size_t blksize1, size_t lrecl1, long nrp1, int ierr) {

   void (*abex)(int, char *) = abexitm;   /* Error exit selector */
   struct RFdef *fd;                      /* Allocated RFdef block */
   int  kexit;                            /* Exit code */

   static char *SysFNms[] =
      { "sysin", "sysout", "stderr", "(unnamed)" };
#define UnNamedNum   3

/* Allocate space for RFdef block and file or host name.
*  Rev, 04/17/13, GNR - Set informative fname1 even before
*                       any possible malloc failures.
*/

   if (!fname1) {
      if (fmt1 >= SysIn && fmt1 <= SysErr)
         fname1 = SysFNms[fmt1 - SysIn];
      else fname1 = SysFNms[UnNamedNum];
      }
   fd = (struct RFdef *)callocv(1, sizeof(struct RFdef), NULL);
   if (!fd) { kexit = 1; goto ERROR98; }
   fd->fname = mallocv(strlen(fname1)+1, NULL);
   if (!fd->fname) { kexit = 2; goto ERROR98; }
   strcpy(fd->fname, fname1);

/* Copy field values */

   fd->inout   = (schr)inout1;
   fd->fmt     = (schr)fmt1;
   fd->accmeth = (schr)accmeth1;
   fd->append  = (schr)append1;
   fd->look    = (schr)look1;
   fd->norew   = (schr)norew1;
   fd->retbuf  = (schr)retbuf1;
   fd->iamopen = (char)NO;
   fd->blksize = blksize1;
   fd->buff    = NULL;

/*---------------------------------------------------------------------*
*              If a socket is requested, acquire socket                *
*   If a listener, bind the port to the socket and start listening     *
*---------------------------------------------------------------------*/

   if (accmeth1 > 0 && accmeth1 & (INITIATOR|LISTENER)) {
#ifdef UNIX

      fd->URF.SOCK.port = (int)nrp1;
#ifdef OSX
      if ((fd->URF.SOCK.isfd = socket(AF_INET, SOCK_STREAM,
         IPPROTO_TCP)) < 0) { kexit = 3; goto ERROR98ME; }
#else
      if ((fd->URF.SOCK.isfd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
         { kexit = 3; goto ERROR98ME; }
#endif

/* If this host is the listener, bind the socket and start
*  listening now.  */

      if (accmeth1 & LISTENER) {
         struct sockaddr_in initaddr;

         int iru = 1;            /* To allow fast reuse */
         if (setsockopt(fd->URF.SOCK.isfd, SOL_SOCKET, SO_REUSEADDR,
            &iru, sizeof(iru)) < 0) { kexit = 8; goto ERROR98; }

         memset((char *)&initaddr, 0, sizeof(struct sockaddr_in));
         initaddr.sin_family = AF_INET;
         if (nrp1 <= 0) { kexit = 4; goto ERROR98; }
         initaddr.sin_port = htons(nrp1);
         /* Expt added 03/30/16, GNR - INADDR_ANY is 0 on Linux */
         initaddr.sin_addr.s_addr = htonl(INADDR_ANY);
         if (bind(fd->URF.SOCK.isfd, (struct sockaddr *)&initaddr,
               sizeof(initaddr)) < 0) {
            /* Since errno 48 (ELNRNG) is not included in the docu-
            *  mentation for bind(), but it occurs in practice, I
            *  am using the explicit number here rather than the
            *  defined name.  */
            if (errno  == 48) goto ERROR109;
            kexit = 5; goto ERROR98ME; }
         if (listen(fd->URF.SOCK.isfd, 1) < 0)
            { kexit = 6; goto ERROR98ME; }
         fd->iamopen |= IAM_LISTENING;
         }
#else
      kexit = 7; goto ERROR98;
#endif
      }

/*---------------------------------------------------------------------*
*                 Not a socket, copy remaining values                  *
*---------------------------------------------------------------------*/

   else {
      fd->URF.DASD.numrec = nrp1;
      fd->URF.DASD.lrecl  = lrecl1;
      }

   return fd;

/*---------------------------------------------------------------------*
*  Error exits--terminate with abexit if ierr >= ABORT                 *
*---------------------------------------------------------------------*/

ERROR98ME:
   abex = abexitme;
ERROR98:
   if (ierr == ABORT)
      abex(98, ssprintf(NULL, "Error %4d allocating RFdef for "
         "%67s.", kexit, fname1));
   else if (ierr == NOMSG_ABORT)
      exit(98);
   if (fd) fd->rferrno = kexit;
   return NULL;
ERROR109:
   if (ierr == ABORT)
      abexitm(109, ssprintf(NULL, "Bind() errno 48, port %d is in use.",
         (int)nrp1));
   else if (ierr == NOMSG_ABORT)
      exit(109);
   if (fd) fd->rferrno = 109;
   return NULL;
   } /* End rfallo() */

