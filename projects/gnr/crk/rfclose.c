/* (c) Copyright 1992-2017, The Rockefeller University *11115* */
/* $Id: rfclose.c 63 2017-04-13 20:47:00Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                               rfclose                                *
*                                                                      *
*     This routine closes a ROCKS file and may deallocate buffers.     *
*  The file name is left in place for possible reopen later.           *
*                                                                      *
*  INPUT:  File record pointer and updated field values, error flag.   *
*          retbuf has special meaning for listening sockets:  it       *
*          refers there to retention of the listening socket.          *
*  OUTPUT: Zero if OK, else return code from system close function.    *
*                                                                      *
*  Note:  MK_TEMP files are unlinked at the time they are created, so  *
*  when they are closed they are gone and cannot be reopened.  To make *
*  this perfectly clear, the filename is set to an empty string.  To   *
*  read a temporary file after writing it, one should just do an       *
*  rfflush/rfseek to access the contents for reading.                  *
************************************************************************
*  V1A, 02/29/92, GNR - Broken out from filelib.c, use n_fclose (NCUBE)*
*  Rev, 01/30/97, GNR - Change 'char' args to int type                 *
*  Rev, 08/30/97, GNR - Use optimal set of I/O calls on each system    *
*  Rev, 11/05/97, GNR - Eliminate cryout call, use abexitm()           *
*  Rev, 12/08/97, GNR - Change logic to avoid seeking if file not open *
*  Rev, 07/04/99, GNR - RFM_xxxx method control                        *
*  Rev, 09/04/99, GNR - Remove NCUBE support                           *
*  Rev, 03/04/07, GNR - Work with revised rfdef                        *
*  Rev, 09/24/08, GNR - Minor changes for 64-bit compilation           *
*  ==>, 09/24/08, GNR - Last date before committing to svn repository  *
*  Rev, 11/03/10, GNR - Implement IAM_SOCKDUP to avoid double closing  *
*  Rev, 01/11/17, GNR - Add MK_TEMP access method                      *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "sysdef.h"
#ifdef UNIX
#include <errno.h>
#endif
#include "rfdef.h"
#include "rksubs.h"

int rfclose(struct RFdef *fd, int norew1, int retbuf1, int ierr) {

    long rc;                /* Return code */

/* Update norew and retbuf fields */

   if (norew1)  fd->norew  = (char)norew1;
   if (retbuf1) fd->retbuf = (char)retbuf1;

/* Finish writing, do requested rewind, and close file
*  only if the file has been properly opened already.  */

   if (fd->iamopen & IAM_ANYOPEN) {

      /* If writing, write last buffer */
      if (fd->inout >= (char)WRITE) {
         rc = rfflush(fd, ierr);
         if (rc < 0) goto ERROR97;
         }

      /* Rewind file position if specified in rfdef.  Omit if
      *  this is a socket, where it is always an error.  */
      if (fd->norew < (char)NO_REWIND &&
            !(fd->accmeth & (INITIATOR|LISTENER))) {
         rc = rfseek(fd, (size_t)0, SEEKABS, ierr);
         if (rc < 0) goto ERROR97;
         }

      /* Perform system close function */
      if (fd->fmt < SysIn && !(fd->iamopen & IAM_SOCKDUP)) {
#ifdef RFM_OPEN
         rc = close(fd->frwd);
#else
         if (fd->iamopen & IAM_FOPEN)
            rc = fclose(fd->pfil);
         else
            rc = close(fd->frwd);
#endif
         if (rc < 0) goto ERROR97;
         } /* End closing non-system files */
      fd->iamopen &= ~IAM_ANYOPEN;
      } /* End actions for open files */

/* If this is a (nondup) listening socket,
*  also close the listening socket.  */

   if (fd->accmeth & LISTENER &&
         (fd->iamopen & (IAM_LISTENING|IAM_SOCKDUP)) == IAM_LISTENING) {
      rc = close(fd->URF.SOCK.isfd);
      fd->iamopen &= ~IAM_LISTENING;
      if (rc < 0) goto ERROR97;
      }

/* Release buffer if specified in rfdef.  If this is a listening
*  socket, also close the listening socket.  */

   if (fd->retbuf != RETAIN_BUFF) {
      if (fd->buff) {
         freev(fd->buff, "File buffer");
         fd->buff = NULL;
         }
      } /* End release buffers */

   return 0;

/*---------------------------------------------------------------------*
*  Error exit--terminate with abexit if ierr >= ABORT                  *
*---------------------------------------------------------------------*/

ERROR97:
   if (ierr == ABORT) {
      abexitme(97, ssprintf(NULL, "Unable to close file %67s "
         "(ret code %10ld).", fd->fname, rc));
      }
   else if (ierr == NOMSG_ABORT)
      exit(97);
   fd->rferrno = rc;
   return (int)rc;

   } /* End rfclose */

