/* (c) Copyright 1999-2008, The Rockefeller University *11115* */
/* $Id: rfru8.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                                rfru8                                 *
*                                                                      *
*     Read a single ui64, possibly unaligned, from a binary data       *
*  file.  Assuming the file is big-endian, swap if necessary to the    *
*  current host byte order.  Access the input buffer directly if       *
*  possible to avoid copying data to an intermediate buffer.  Other-   *
*  wise, use rfread to read the data and abort on any kind of error.   *
*  User can detect that end-of-file has been reached by testing that   *
*  fd->lbsr = ATEOF.                                                   *
*                                                                      *
*  Unlike the case with base-type integers, we need separate routines  *
*  for signed and unsigned wide integers because they are usually      *
*  implemented as structures, in which case there are no intrinsic     *
*  typecast routines to interconvert them.                             *
*                                                                      *
************************************************************************
*  V1A, 10/24/99, GNR - New routine                                    *
*  ==>, 07/01/02, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "rfdef.h"
#include "swap.h"

ui64 rfru8(struct RFdef *fd) {

/* If the item lies contiguously in the current
*  buffer, just pick it up directly.  */

   if (fd->lbsr >= FMWSIZE) {
      ui64 ru8 = bemtou8(fd->bptr);
      fd->bptr += FMWSIZE;
      fd->aoff += FMWSIZE;
      fd->lbsr -= FMWSIZE;
      return ru8;
      }

/* Item is not contiguous in memory, do a standard rfread */

   else {
      char cu8[FMWSIZE];
      rfread(fd, cu8, FMWSIZE, ABORT);
      return bemtou8(cu8);
      }

   } /* End rfru8() */

