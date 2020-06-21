/* (c) Copyright 1992-2017, The Rockefeller University *11115* */
/* $Id: rfread.c 63 2017-04-13 20:47:00Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                       rfread, rfgetb, rfgets                         *
*                                                                      *
*     These routines read data from a ROCKS file into a user area.     *
*  We do our own buffering, because testing shows this way is faster.  *
*  It is left to future development to add double buffering.           *
*                                                                      *
************************************************************************
*  V1A, 02/29/92, GNR - Newly written                                  *
*  Rev, 08/04/92, GNR - Correct detection of eof on NCUBE              *
*  Rev, 10/02/92, GNR - Print error code returned by system            *
*  Rev, 01/30/97, GNR - Change 'char' args to int type                 *
*  Rev, 08/30/97, GNR - Add buffering for all cases                    *
*  Rev, 11/05/97, GNR - Eliminate cryout call, use abexitm()           *
*  Rev, 08/16/98, GNR - Add rfgets()                                   *
*  Rev, 03/27/99, GNR - Make rfgetb() public                           *
*  Rev, 07/04/99, GNR - RF_RDWR method control                         *
*  Rev, 09/04/99, GNR - Remove NCUBE support                           *
*  Rev, 03/04/07, GNR - Work with revised rfdef                        *
*  Rev, 09/25/08, GNR - Minor type changes for 64-bit compilation      *
*  ==>, 09/25/08, GNR - Last date before committing to svn repository  *
*  Rev, 01/20/11, GNR - Retry read() on EINTR error                    *
*  Rev, 04/24/14, GNR - Add accmeth ON_LINE switch                     *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "sysdef.h"
#if defined(UNIX)
#include <errno.h>
#endif
#include "rfdef.h"

/*=====================================================================*
*                              rfgetb()                                *
*                                                                      *
*        Routine to read one full buffer and deal with errors.         *
*                                                                      *
*  Synopsis:                                                           *
*     long rfgetb(struct RFdef *fd, int ierr)                          *
*                                                                      *
*  Arguments:                                                          *
*     fd       Ptr to an open RFdef defining the file to be read.      *
*     ierr     Error control flag.  May take on any of the values      *
*              NO_ABORT, ABORT, or NOMSG_ABORT defined in rfdef.h.     *
*                                                                      *
*  Returns:                                                            *
*     Length of data read, zero if eof, negative if error and 'ierr'   *
*     was NO_ABORT.  The data are returned in fd->buff and length of   *
*     data is stored also in fd->lbsr.                                 *
*                                                                      *
*  Note:                                                               *
*     Single buffering is used for now until there is time to look     *
*     into use of non-blocking read functions.                         *
*=====================================================================*/

long rfgetb(struct RFdef *fd, int ierr) {

   long rc;                      /* Return code from system read */

#ifdef UNIX
#ifdef RFM_OPEN
   /* Using read, and don't even want to load fread */
   while (1) {
      rc = (long)read(fd->frwd, fd->buff, fd->blksize);
      if (rc > 0) break;
      if (rc == 0) {
         fd->lbsr = ATEOF; return 0; }
      if (errno != EINTR) goto ERROR3;
      }
#else
   if (fd->iamopen & IAM_FOPEN) {   /* Using fread/fwrite */
      rc = (long)fread(fd->buff, 1, fd->blksize, fd->pfil);
      if (rc == 0 && feof(fd->pfil)) {
         fd->lbsr = ATEOF; return 0; }
      }
   else {                           /* Using read/write */
      while (1) {
         rc = (long)read(fd->frwd, fd->buff, fd->blksize);
         if (rc > 0) break;
         if (rc == 0) {
            fd->lbsr = ATEOF; return 0; }
         if (errno != EINTR) goto ERROR3;
         }
      }
#endif

#else     /* Not UNIX */
#ifdef RFM_OPEN
   /* Using read, and don't even want to load fread */
   rc = (long)read(fd->frwd, fd->buff, fd->blksize);
   if (rc == 0) {
      fd->lbsr = ATEOF; return 0; }
#else
   if (fd->iamopen & IAM_FOPEN) {   /* Using fread/fwrite */
      rc = (long)fread(fd->buff, 1, fd->blksize, fd->pfil);
      if (rc == 0 && feof(fd->pfil)) {
         fd->lbsr = ATEOF; return 0; }
      }
   else {                           /* Using read/write */
      rc = (long)read(fd->frwd, fd->buff, fd->blksize);
      if (rc == 0) {
         fd->lbsr = ATEOF; return 0; }
      }
#endif   /* RFM_OPEN */
   if (rc < 0) goto ERROR3;
#endif   /* UNIX */
   fd->bptr = fd->buff;
   return (fd->lbsr = rc);

/*---------------------------------------------------------------------*
*  Error exit--terminate with abexit if ierr >= ABORT                  *
*---------------------------------------------------------------------*/

ERROR3:
   if (ierr == ABORT) {
      abexitme(3, ssprintf(NULL, "Read error on file %67s "
         "(ret code %10ld).", fd->fname, rc));
      }
   else if (ierr == NOMSG_ABORT)
      exit(3);
   fd->rferrno = rc;
   return rc;

   } /* End rfgetb() */

/*=====================================================================*
*                              rfread()                                *
*                                                                      *
*     Read a block of data of specified length into a user data area.  *
*                                                                      *
*  Synopsis:                                                           *
*     long rfread(struct RFdef *fd, void *item, size_t length,         *
*        int ierr)                                                     *
*                                                                      *
*  Arguments:                                                          *
*     fd       Ptr to an open RFdef defining the file to be read.      *
*     item     Ptr to memory area where data are to be returned.       *
*     length   Number of bytes to be read into 'item'.                 *
*     ierr     Error control flag.  May take on any of the values      *
*              NO_ABORT, ABORT, or NOMSG_ABORT defined in rfdef.h.     *
*                                                                      *
*  Returns:                                                            *
*     Length of data read (should equal length requested), zero if     *
*     eof, negative if error and 'ierr' was NO_ABORT.  The data are    *
*     returned in the buffer pointed to by the 'item' argument.  No    *
*     action is taken to reduce CR-LF (where used as newline) to LF.   *
*=====================================================================*/

long rfread(struct RFdef *fd, void *item, size_t length, int ierr) {

   long ilen = (long)length;  /* Length requested */
   long jlen;                 /* Length of actual move */

/*---------------------------------------------------------------------*
*  Check amount of data in current buffer.  If not sufficient, copy    *
*  what there is to caller and read another.  Then copy remainder of   *
*  data requested by caller and advance pointer.  Return error code    *
*  from read if any.  Return len if OK.                                *
*---------------------------------------------------------------------*/

   /* If already at end-of-file, return without reading */
   if (fd->lbsr == ATEOF) return 0;

   while (ilen) {

      /* If buffer is empty, read into it */
      if (!fd->lbsr) {
         long rc = rfgetb(fd, ierr);
         if (rc <= 0) {
            fd->rferrno = rc;
            return rc;
            }
         } /* End read new buffer */

      /* Move lesser of data requested or data in buffer */
      jlen = (ilen < fd->lbsr) ? ilen : fd->lbsr;
      memcpy(item, fd->bptr, (size_t)jlen);
      item      = (char *)item + jlen;
      fd->bptr += jlen;
      fd->aoff += jlen;
      fd->lbsr -= jlen;
      ilen     -= jlen;
      } /* End while ilen */

   return length;          /* Successful return */

   } /* End rfread() */

/*=====================================================================*
*                              rfgets()                                *
*                                                                      *
*     This routine reads from a ROCKS RFdef input file until either    *
*  a new line is found or the buffer is full.  All input characters    *
*  are retained in the data item except the OS-specific end-of-line    *
*  marker (NL, CR-LF, etc.) is eaten.  (IBM systems with no newline    *
*  are currently not supported.)  If the accmeth ON_LINE bit is set,   *
*  backspace (^H) causes the last character to be erased.  A NULL is   *
*  appended to mark the end of the data returned.                      *
*                                                                      *
*     This routine is similar in function to the standard C library    *
*  routine fgets(), but the arguments, return value, and handling of   *
*  the newline marker are different.                                   *
*                                                                      *
*  Note:  The need to handle BS arises when a crk program is run at    *
*  runlevel 1--otherwise, the tty handler seems to take care of this.  *
*                                                                      *
*  Synopsis:                                                           *
*     long rfgets(struct RFdef *fd, char *item, size_t length,         *
*        int ierr)                                                     *
*                                                                      *
*  Arguments:                                                          *
*     fd       Ptr to an open RFdef defining the file to be read.      *
*     item     Ptr to memory area where data are to be returned.       *
*     length   Size of 'item'.  The maximum number of characters       *
*              that can be read is (length-1) because of the NULL      *
*              that is appended to the end of the data.                *
*     ierr     Error control flag.  May take on any of the values      *
*              NO_ABORT, ABORT, or NOMSG_ABORT defined in rfdef.h.     *
*                                                                      *
*  Returns:                                                            *
*     Length of data read (including the NULL terminator so empty      *
*     lines can be distinguished from eof), zero if eof, negative      *
*     if error and 'ierr' was NO_ABORT.  The data are returned in      *
*     the buffer pointed to by the 'item' argument.                    *
*=====================================================================*/

long rfgets(struct RFdef *fd, char *item, size_t length, int ierr) {

   char *pit = item;             /* Ptr to user data field */
   long ilen = (long)length - 1; /* Space remaining in item buffer */
#if !defined(UNIX)
   int gotcrlf = NO;             /* Records pending newlines */
   int nxtchar;                  /* Look-ahead character */
#endif
   int iamonline = fd->accmeth & ON_LINE;

/*---------------------------------------------------------------------*
*  Move data from rfread() buffer into user's item until newline is    *
*  found or buffer is full.  If current read buffer is exhausted,      *
*  read another.  Return data length if all goes well, otherwise       *
*  handle error as requested.                                          *
*---------------------------------------------------------------------*/

   /* If already at end-of-file, return without reading */
   if (fd->lbsr == ATEOF) return 0;

   while (ilen > 0) {

      /* If buffer is empty, read into it */
      if (!fd->lbsr) {
         long rc = rfgetb(fd, ierr);
         if (rc <= 0) {
#if !defined(UNIX)
            if (rc == 0 && gotcrlf) break;
#endif
            fd->rferrno = rc;
            return rc;
            }
         } /* End read new buffer */

      /* Scan through data looking for newline */
#if defined(UNIX)
      if (*fd->bptr == '\n') {
         fd->bptr += 1;    /* Eat the newline */
         fd->aoff += 1;
         fd->lbsr -= 1;
         break;
         }
#else
      /* Hopefully, this code will work on a variety of non-UNIX
      *  systems:  A lone CR or LF or one of each in either order
      *  is treated as a newline.  Two alike in a row is treated
      *  as two lines.  Beware that the newline sequence might be
      *  divided across two input buffers.  Problem:  if online,
      *  might have to wait for second line to recognize first.  */
      nxtchar = *fd->bptr;
      if (nxtchar == '\n' || nxtchar == '\r') {
         if (gotcrlf == nxtchar) /* Two CR or LF in a row */
            break;               /*    We'll see this one again */
         fd->bptr += 1;          /* Eat the CR or LF */
         fd->aoff += 1;
         fd->lbsr -= 1;
         if (gotcrlf) break;     /* CR-LF or LF-CR sequence */
         gotcrlf = nxtchar;      /* Record presence of LF or CR but */
         continue;               /*    do not move it to the output */
         }
      if (gotcrlf)               /* CR or LF alone */
         break;
      /* Insert any different tests for other systems here.  */
#endif

      /* If online and character is backspace, and not at
      *  start of user's field, delete one character,
      *  otherwise, move current character to output */
      if (iamonline && *fd->bptr == '\b' && pit > item) {
         --pit, ++fd->bptr;
         ilen += 1;
         }
      else {
         *pit++ = *fd->bptr++;
         ilen     -= 1;
         }
      fd->aoff += 1;
      fd->lbsr -= 1;
      } /* End while ilen */

   *pit = '\0';               /* Terminate input string */
   return length - ilen;      /* Successful return */

   } /* End rfgets() */
