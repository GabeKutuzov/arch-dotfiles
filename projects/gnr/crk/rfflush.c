/* (c) Copyright 1997-2017, The Rockefeller University *11115* */
/* $Id: rfflush.c 63 2017-04-13 20:47:00Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                               rfflush                                *
*                                                                      *
*     This routine flushes any data buffered for writing to a ROCKS    *
*  data file by previous calls to rfwrite().  In this version, all     *
*  buffering is single, not double.                                    *
*                                                                      *
*  INPUT:  Ptr to RFdef struct, error flag.                            *
*  OUTPUT: Length of data written, negative if error.                  *
*                                                                      *
************************************************************************
*  V1A, 09/13/97, GNR - Newly written                                  *
*  Rev, 11/05/97, GNR - Eliminate cryout call, use abexitm()           *
*  Rev, 07/04/99, GNR - RFM_xxxx method control                        *
*  Rev, 09/04/99, GNR - Remove NCUBE support                           *
*  Rev, 03/04/07, GNR - Work with revised rfdef                        *
*  Rev, 09/25/08, GNR - Type changes for 64-bit compilation            *
*  ==>, 09/25/08, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "sysdef.h"
#ifdef UNIX
#include <errno.h>
#endif
#include <string.h>
#include "rfdef.h"

long rfflush(struct RFdef *fd, int ierr) {

   size_t wlen;            /* Write length */
   long rc;                /* Return code */

/* If this is an input file, quietly return */

   if (fd->inout < (char)WRITE) return 0;

/* Flush any data in our buffer out to the system buffer.  Return
*  error code from write if any.  If using a UNIX stream, flush
*  that also.  Save length to return if OK.  */

   if ((wlen = fd->bptr - fd->buff) > 0) {
#ifdef RFM_OPEN
      rc = write(fd->frwd, fd->buff, wlen);
#else    /* Everything else */
      if (fd->iamopen & IAM_FOPEN) {
         rc = (long)fwrite(fd->buff, 1, wlen, fd->pfil);
         if (rc < 0) goto ERROR5;
         rc = (long)fflush(fd->pfil);
         }
      else
         rc = write(fd->frwd, fd->buff, wlen);
#endif
      if (rc < 0) goto ERROR5;
      }

   fd->bptr = fd->buff;
   fd->lbsr = (long)fd->blksize;

   return (long)wlen;         /* Successful return */

/*---------------------------------------------------------------------*
*  Error exit--terminate with abexit if ierr >= ABORT                  *
*---------------------------------------------------------------------*/

ERROR5:
   if (ierr == ABORT) {
      abexitme(5, ssprintf(NULL, "Write error flushing file %67s "
         "(ret code %10ld).", fd->fname, rc));
      }
   else if (ierr == NOMSG_ABORT)
      exit(5);
   fd->rferrno = rc;
   return rc;

   } /* End rfflush */
