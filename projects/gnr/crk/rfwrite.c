/* (c) Copyright 1992-2017, The Rockefeller University *11115* */
/* $Id: rfwrite.c 63 2017-04-13 20:47:00Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                               rfwrite                                *
*                                                                      *
*     This routine writes data to a ROCKS file from a user area.       *
*  We do our own buffering, because testing shows this way is faster.  *
*  It is left to future development to add double buffering.           *
*                                                                      *
*  INPUT:  Ptr to RFdef struct, ptr to data, data length, error flag.  *
*  OUTPUT: Length of data written, negative if error.                  *
*                                                                      *
************************************************************************
*  V1A, 02/29/92, GNR - Newly written                                  *
*  Rev, 10/02/92, GNR - Print error code returned by system            *
*  Rev, 01/30/97, GNR - Change 'char' args to int type                 *
*  Rev, 08/30/97, GNR - Add buffering for all cases                    *
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
#include <unistd.h>
#include "sysdef.h"
#ifdef UNIX
#include <errno.h>
#endif
#include <string.h>
#include "rfdef.h"

long rfwrite(struct RFdef *fd, void *item, size_t length, int ierr) {

   long ilen = (long)length;  /* Length requested */
   long jlen;                 /* Length of actual move */
   long rc;                   /* Return code */

/*---------------------------------------------------------------------*
*  Check amount of space in buffer.  If not sufficient, copy partial   *
*  data to buffer and write it.  Then copy remainder of data from      *
*  caller and advance bptr.  Return error code from write if any.      *
*  Return length if OK.  Single buffering is used for now.             *
*---------------------------------------------------------------------*/

   while (ilen) {

      /* If buffer is full, write it out and reset ptrs */
      if (!fd->lbsr) {
         if (fd->bptr > fd->buff) {    /* First time, no write */
#ifdef RFM_OPEN
            /* Using write, and don't even want to load fwrite */
            rc = (long)write(fd->frwd, fd->buff, fd->blksize);
#else
            /* Using fwrite except for online stdin or socket */
            if (fd->iamopen & IAM_FOPEN)  /* Using fread/fwrite */
               rc = (long)fwrite(fd->buff, 1, fd->blksize, fd->pfil);
            else                          /* Using read/write */
               rc = (long)write(fd->frwd, fd->buff, fd->blksize);
#endif
            if (rc < 0) goto ERROR5;
            fd->bptr = fd->buff;
            }
         fd->lbsr = fd->blksize;
         } /* End write buffer */
      /* Move lesser of data requested or space in buffer */
      jlen = (ilen < fd->lbsr) ? ilen : fd->lbsr;
      memcpy(fd->bptr, item, (size_t)jlen);
      item      = (char *)item + jlen;
      fd->bptr += jlen;
      fd->aoff += jlen;
      ilen     -= jlen;
      fd->lbsr -= jlen;
      } /* End while ilen */

   return length;          /* Successful return */

/*---------------------------------------------------------------------*
*  Error exit--terminate with abexit if ierr >= ABORT                  *
*---------------------------------------------------------------------*/

ERROR5:
   if (ierr == ABORT) {
      abexitme(5, ssprintf(NULL, "Write error on file %67s "
         "(ret code %10ld).", fd->fname, rc));
      }
   else if (ierr == NOMSG_ABORT)
      exit(5);
   fd->rferrno = rc;
   return rc;

   } /* End rfwrite */
