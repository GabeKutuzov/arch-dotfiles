/* (c) Copyright 1999-2008, The Rockefeller University *11115* */
/* $Id: rfwu8.c 63 2017-04-13 20:47:00Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                                rfwu8                                 *
*                                                                      *
*     Write a single ui64 into a binary data file, possibly not on a   *
*  boundary.  Assuming the file is big-endian, swap if necessary from  *
*  the current host byte order.  Access the output buffer directly if  *
*  possible to avoid storing data in an intermediate buffer.  Other-   *
*  wise, use rfwrite to write the data and abort on any kind of error. *
*                                                                      *
*  Unlike the case with base-type integers, we need separate routines  *
*  for signed and unsigned wide integers because they are usually      *
*  implemented as structures, in which case there are no intrinsic     *
*  typecast routines to interconvert them.                             *
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

void rfwu8(struct RFdef *fd, ui64 u8) {

/* If the item will fit contiguously in the
*  current buffer, just store it directly.  */

   if (fd->lbsr >= FMWSIZE) {
      bemfmu8(fd->bptr, u8);
      fd->bptr += FMWSIZE;
      fd->aoff += FMWSIZE;
      fd->lbsr -= FMWSIZE;
      }

/* Item will not fit contiguously in the
*  current buffer, do a standard rfwrite.  */

   else {
      char cu8[FMWSIZE];
      bemfmu8(cu8, u8);
      rfwrite(fd, cu8, FMWSIZE, ABORT);
      }

   } /* End rfwu8() */

