/* (c) Copyright 1999-2008, The Rockefeller University *11115* */
/* $Id: rfwi8.c 63 2017-04-13 20:47:00Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                                rfwi8                                 *
*                                                                      *
*     Write a single si64 into a binary data file, possibly not on a   *
*  boundary.  Assuming the file is big-endian, swap if necessary from  *
*  the current host byte order.  Access the output buffer directly if  *
*  possible to avoid storing data in an intermediate buffer.  Other-   *
*  wise, use rfwrite to write the data and abort on any kind of error. *
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

void rfwi8(struct RFdef *fd, si64 i8) {

/* If the item will fit contiguously in the
*  current buffer, just store it directly.  */

   if (fd->lbsr >= FMWSIZE) {
      bemfmi8(fd->bptr, i8);
      fd->bptr += FMWSIZE;
      fd->aoff += FMWSIZE;
      fd->lbsr -= FMWSIZE;
      }

/* Item will not fit contiguously in the
*  current buffer, do a standard rfwrite.  */

   else {
      char ci8[FMWSIZE];
      bemfmi8(ci8, i8);
      rfwrite(fd, ci8, FMWSIZE, ABORT);
      }

   } /* End rfwi8() */

