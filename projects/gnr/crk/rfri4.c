/* (c) Copyright 1999-2008, The Rockefeller University *11115* */
/* $Id: rfri4.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                                rfri4                                 *
*                                                                      *
*     Read a single long, possibly unaligned, from a binary data       *
*  file.  Assuming the file is big-endian, swap if necessary to the    *
*  current host byte order.  Access the input buffer directly if       *
*  possible to avoid copying data to an intermediate buffer.  Other-   *
*  wise, use rfread to read the data and abort on any kind of error.   *
*  User can detect that end-of-file has been reached by testing that   *
*  fd->lbsr = ATEOF.                                                   *
*                                                                      *
************************************************************************
*  V1A, 06/02/99, GNR - New routine                                    *
*  Rev, 09/25/08, GNR - Minor type changes for 64-bit compilation      *
*  ==>, 09/26/08, GNR - Last date before committing to svn repository  *
*  Rev, 10/26/13, GNR - FMLSIZE --> FMJSIZE                            *
***********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "rfdef.h"
#include "swap.h"

si32 rfri4(struct RFdef *fd) {

/* If the item lies contiguously in the current
*  buffer, just pick it up directly.  */

   if (fd->lbsr >= FMJSIZE) {
      ui32 ri4 = bemtoi4(fd->bptr);
      fd->bptr += FMJSIZE;
      fd->aoff += FMJSIZE;
      fd->lbsr -= FMJSIZE;
      return ri4;
      }

/* Item is not contiguous in memory, do a standard rfread */

   else {
      char ci4[FMJSIZE];
      rfread(fd, ci4, FMJSIZE, ABORT);
      return bemtoi4(ci4);
      }

   } /* End rfri4() */
