/* (c) Copyright 1999-2008, The Rockefeller University *11115* */
/* $Id: rfrr8.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                                rfrr8                                 *
*                                                                      *
*     Read a single double, possibly unaligned, from a binary data     *
*  file.  Assuming the file is big-endian, swap if necessary to the    *
*  current host byte order.  Access the input buffer directly if       *
*  possible to avoid copying data to an intermediate buffer.  Other-   *
*  wise, use rfread to read the data and abort on any kind of error.   *
*  User can detect that end-of-file has been reached by testing that   *
*  fd->lbsr = ATEOF.                                                   *
*                                                                      *
************************************************************************
*  V1A, 06/02/99, GNR - New routine                                    *
*  ==>, 07/01/02, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "rfdef.h"
#include "swap.h"

double rfrr8(struct RFdef *fd) {

/* If the item lies contiguously in the current
*  buffer, just pick it up directly.  */

   if (fd->lbsr >= FMDSIZE) {
      double rr8 = bemtor8(fd->bptr);
      fd->bptr += FMDSIZE;
      fd->aoff += FMDSIZE;
      fd->lbsr -= FMDSIZE;
      return rr8;
      }

/* Item is not contiguous in memory, do a standard rfread */

   else {
      char cr8[FMDSIZE];
      rfread(fd, cr8, FMDSIZE, ABORT);
      return bemtor8(cr8);
      }

   } /* End rfrr8() */

