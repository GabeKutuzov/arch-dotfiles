/* (c) Copyright 1999-2008, The Rockefeller University *11115* */
/* $Id: rfwr8.c 63 2017-04-13 20:47:00Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                                rfwr8                                 *
*                                                                      *
*     Write a single double into a binary data file, possibly not on a *
*  boundary.  Assuming the file is big-endian, swap if necessary from  *
*  the current host byte order.  Access the output buffer directly if  *
*  possible to avoid storing data in an intermediate buffer.  Other-   *
*  wise, use rfwrite to write the data and abort on any kind of error. *
*                                                                      *
************************************************************************
*  V1A, 06/02/99, GNR - New routine                                    *
*  ==>, 12/22/08, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "rfdef.h"
#include "swap.h"

void rfwr8(struct RFdef *fd, double r8) {

/* If the item will fit contiguously in the
*  current buffer, just store it directly.  */

   if (fd->lbsr >= FMDSIZE) {
      bemfmr8(fd->bptr, r8);
      fd->bptr += FMDSIZE;
      fd->aoff += FMDSIZE;
      fd->lbsr -= FMDSIZE;
      }

/* Item will not fit contiguously in the
*  current buffer, do a standard rfwrite.  */

   else {
      char cr8[FMDSIZE];
      bemfmr8(cr8, r8);
      rfwrite(fd, cr8, FMDSIZE, ABORT);
      }

   } /* End rfwr8() */

