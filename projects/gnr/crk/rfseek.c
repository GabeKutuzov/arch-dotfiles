/* (c) Copyright 1992-2017, The Rockefeller University *11115* */
/* $Id: rfseek.c 63 2017-04-13 20:47:00Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                               rfseek                                 *
*                                                                      *
*     This routine repositions the read pointer in an open ROCKS file. *
*  If within current buffer, a skip is performed, otherwise a seek.    *
*  A read is not performed at the new position, as chances are good    *
*  it will never be needed, or, if a read is next, it will come soon   *
*  enough that there was no real advantage to trying to look ahead.    *
*  N.B.  kseek = SEEKEND is not currently implemented.                 *
*                                                                      *
*  INPUT:  Ptr to RFdef struct, offset, type of seek, error flag.      *
*  OUTPUT: Absolute offset if successful, else a negative error code.  *
*                                                                      *
************************************************************************
*  V1A, 02/29/92, GNR - Newly written                                  *
*  Rev, 08/14/92, GNR - Correct bug trying to seek after eof reached   *
*  Rev, 08/20/92, GNR - Correct bug failing to read if seek is initial *
*  Rev, 01/30/97, GNR - Change 'char' args to int type                 *
*  Rev, 08/30/97, GNR - Change to do our own buffering in all cases    *
*  Rev, 09/15/97, GNR - Major rewrite--just position, let rfread do    *
*                       the next read, forget concept of blocks        *
*  Rev, 11/05/97, GNR - Eliminate cryout call, use abexitm()           *
*  Rev, 07/04/99, GNR - RF_RDWR method control                         *
*  Rev, 09/04/99, GNR - Remove NCUBE support                           *
*  Rev, 03/04/07, GNR - Work with revised rfdef                        *
*  Rev, 09/25/08, GNR - Minor type changes for 64-bit compilation      *
*  ==>, 09/26/08, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include "sysdef.h"
#ifdef UNIX
#include <errno.h>
#endif
#include "rfdef.h"

long rfseek(struct RFdef *fd, size_t offset, int kseek, int ierr) {

   void (*abex)(int, char *) = abexitm;   /* Error exit selector */
   size_t aoff1 = offset;                 /* New absolute offset */
   long rc;                               /* Return code */

/* Determine relative and absolute offset of target */

   if (kseek == SEEKEND) { rc = -1; goto ERROR4; }
   if (kseek == SEEKABS) offset -= fd->aoff;
   else                  aoff1  += fd->aoff;

   if (fd->bptr + offset < fd->buff || offset >= fd->lbsr) {

/*---------------------------------------------------------------------*
*  If target is outside current buffer, lseek, or fseek to desired     *
*  location and adjust buffer controls to force a new read.            *
*  If already at the end-of-file, lbsr will be 0 or -1 and a forward   *
*  offset will result in a seek.  If target is negative or past end    *
*  of file, library will complain.                                     *
*---------------------------------------------------------------------*/

#ifdef RFM_OPEN
      rc = (long)lseek(fd->frwd, aoff1, SEEK_SET);
      if (rc < 0) goto ERROR4ME;
#else    /* Everything else */
      if (fd->iamopen & IAM_FOPEN) {
         rc = (long)fseek(fd->pfil, aoff1, SEEK_SET);
         if (rc) goto ERROR4ME;
         }
      else {
         rc = (long)lseek(fd->frwd, aoff1, SEEK_SET);
         if (rc < 0) goto ERROR4ME;
         }
#endif   /* Not RFM_OPEN */

      fd->aoff = aoff1;
      fd->bptr = fd->buff;
      fd->lbsr = 0;
      }

/*---------------------------------------------------------------------*
*  If target is within current buffer, just modify pointer and count.  *
*---------------------------------------------------------------------*/

   else {
      fd->aoff += offset;
      fd->bptr += offset;
      fd->lbsr -= offset;
      }

   return aoff1;

/*---------------------------------------------------------------------*
*  Error exit--terminate with abexit if ierr >= ABORT                  *
*---------------------------------------------------------------------*/

ERROR4ME:
   abex = abexitme;
ERROR4:
   if (ierr == ABORT) {
      abex(4, ssprintf(NULL, "Seek error on file %67s "
         "(abs. offset %10ld).", fd->fname, (long)aoff1));
      }
   else if (ierr == NOMSG_ABORT)
      exit(4);
   fd->rferrno = rc;
   return rc;

   } /* End rfseek() */
