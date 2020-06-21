/* (c) Copyright 1999-2008, The Rockefeller University *11115* */
/* $Id: rfri8.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                                rfri8                                 *
*                                                                      *
*     Read a single si64, possibly unaligned, from a binary data       *
*  file.  Assuming the file is big-endian, swap if necessary to the    *
*  current host byte order.  Access the input buffer directly if       *
*  possible to avoid copying data to an intermediate buffer.  Other-   *
*  wise, use rfread to read the data and abort on any kind of error.   *
*  User can detect that end-of-file has been reached by testing that   *
*  fd->lbsr = ATEOF.                                                   *
*                                                                      *
************************************************************************
*  V1A, 10/24/99, GNR - New routine                                    *
*  ==>, 09/26/08, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "rfdef.h"
#include "swap.h"

si64 rfri8(struct RFdef *fd) {

/* If the item lies contiguously in the current
*  buffer, just pick it up directly.  */

   if (fd->lbsr >= FMWSIZE) {
      si64 ri8 = bemtoi8(fd->bptr);
      fd->bptr += FMWSIZE;
      fd->aoff += FMWSIZE;
      fd->lbsr -= FMWSIZE;
      return ri8;
      }

/* Item is not contiguous in memory, do a standard rfread */

   else {
      char ci8[FMWSIZE];
      rfread(fd, ci8, FMWSIZE, ABORT);
      return bemtoi8(ci8);
      }

   } /* End rfri8() */
