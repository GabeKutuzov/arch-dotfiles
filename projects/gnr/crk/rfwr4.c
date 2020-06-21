/* (c) Copyright 1999-2008, The Rockefeller University *11115* */
/* $Id: rfwr4.c 63 2017-04-13 20:47:00Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                                rfwr4                                 *
*                                                                      *
*     Write a single float into a binary data file, possibly not on a  *
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

void rfwr4(struct RFdef *fd, float r4) {

/* If the item will fit contiguously in the
*  current buffer, just store it directly.  */

   if (fd->lbsr >= FMESIZE) {
      bemfmr4(fd->bptr, r4);
      fd->bptr += FMESIZE;
      fd->aoff += FMESIZE;
      fd->lbsr -= FMESIZE;
      }

/* Item will not fit contiguously in the
*  current buffer, do a standard rfwrite.  */

   else {
      char cr4[FMESIZE];
      bemfmr4(cr4, r4);
      rfwrite(fd, cr4, FMESIZE, ABORT);
      }

   } /* End rfwr4() */

