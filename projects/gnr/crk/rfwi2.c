/* (c) Copyright 1999-2008, The Rockefeller University *11115* */
/* $Id: rfwi2.c 63 2017-04-13 20:47:00Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                                rfwi2                                 *
*                                                                      *
*     Write a single short into a binary data file, possibly not on a  *
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

void rfwi2(struct RFdef *fd, short i2) {

/* If the item will fit contiguously in the
*  current buffer, just store it directly.  */

   if (fd->lbsr >= FMSSIZE) {
      bemfmi2(fd->bptr, i2);
      fd->bptr += FMSSIZE;
      fd->aoff += FMSSIZE;
      fd->lbsr -= FMSSIZE;
      }

/* Item will not fit contiguously in the
*  current buffer, do a standard rfwrite.  */

   else {
      char ci2[FMSSIZE];
      bemfmi2(ci2, i2);
      rfwrite(fd, ci2, FMSSIZE, ABORT);
      }

   } /* End rfwi2() */

