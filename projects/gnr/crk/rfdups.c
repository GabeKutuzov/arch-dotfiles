/* (c) Copyright 2007-2008, The Rockefeller University *11115* */
/* $Id: rfdups.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                               rfdups                                 *
*                                                                      *
*     The ROCKS I/O routines provide a standard interface to UNIX and  *
*  UNIX-like I/O systems that permit an additional layer of functions  *
*  to be interposed between the user and the system.  It allows IBM    *
*  MVS files to be accessed using the same paradigm as UNIX files.     *
*                                                                      *
*     This routine, rfdups(), duplicates an RFdef control block that   *
*  was opened for a socket, allowing read and write operations on the  *
*  socket to be carried out separately, each with buffering.           *
*                                                                      *
*  Synopsis:                                                           *
*  struct RFdef *rfdups(struct RFdef *fd, size_t blksize1, int ierr)   *
*                                                                      *
*  Arguments:                                                          *
*     fd       Pointer to an RFdef structure that is already open      *
*              for socket I/O.  Sockets are by nature bidirectional,   *
*              therefore, two RFdef blocks may be bound to the same    *
*              socket in order to buffer read and write operations     *
*              separately.  If the original RFdef is marked READ,      *
*              the new one will be marked WRITE and vice-versa.        *
*     blksize1 Buffer size for file buffering: may be coded as SAME    *
*              to use same size as parent RFdef.  There may be reason  *
*              to use a different buffer size for writing than for     *
*              reading.                                                *
*     ierr     Error flag.  NO_ABORT (-1) if routine should simply     *
*              return NULL on failure, ABORT (0) to terminate with     *
*              an abexit call and message if there is an error, and    *
*              NOMSG_ABORT to terminate with no attempt to print an    *
*              error message.  The only error reported by rfdups()     *
*              is a failure to allocate memory for the new RFdef of    *
*              the new buffer.                                         *
*                                                                      *
*  Returns:                                                            *
*     Pointer to file record if successfully allocated or NULL if      *
*     there was an error and ierr was NO_ABORT.                        *
*                                                                      *
************************************************************************
*  V1A, 03/04/07, GNR - New routine                                    *
*  ==>, 09/24/08, GNR - Last date before committing to svn repository  *
*  Rev, 11/03/10, GNR - Implement IAM_SOCKDUP to avoid double closing  *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "rfdef.h"
#include "rksubs.h"

/*=====================================================================*
*                              rfdups()                                *
*=====================================================================*/

struct RFdef *rfdups(struct RFdef *fd, size_t blksize1, int ierr) {

   struct RFdef *nfd;         /* New allocated RFdef block */

/* Allocate space for RFdef block and copy everything */

   nfd = (struct RFdef *)mallocv(sizeof(struct RFdef), NULL);
   if (!nfd) goto ERROR53;
   *nfd = *fd;

/* Modify fields that are handled differently */

   nfd->nf = NULL;
   nfd->fname = NULL;
   /* If READWRITE, leave it alone, but user must do only
   *  one of the two operations on each of the RFdefs.  */
   if (nfd->inout <= READ) nfd->inout = WRITE;
   else if (nfd->inout == WRITE) nfd->inout = READ;
   if (blksize1 > 0) nfd->blksize = blksize1;
   /* The IAM_SOCKDUP flag should prevent trying to close
   *  the underlying socket twice */
   nfd->iamopen |= IAM_SOCKDUP;

/* Allocate buffer and initialize buffer controls to indicate no
*  contents in a way that will work correctly whether a read or
*  write is attempted first.  */

   nfd->buff = mallocv(nfd->blksize, NULL);
   if (!nfd->buff) goto ERROR53;
   nfd->aoff = 0;
   nfd->lbsr = 0;
   nfd->bptr = nfd->buff;

   return nfd;

/*---------------------------------------------------------------------*
*  Error exit--terminate with abexit if ierr >= ABORT                  *
*---------------------------------------------------------------------*/

ERROR53:
   if (ierr == ABORT)
      abexitm(53, "Not enough memory to read & write same socket");
   else if (ierr == NOMSG_ABORT)
      exit(53);
   fd->rferrno = 53;
   return NULL;

   } /* End rfdups() */
