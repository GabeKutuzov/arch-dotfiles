/* (c) Copyright 1999-2013, The Rockefeller University *11115* */
/* $Id: rfwi4.c 63 2017-04-13 20:47:00Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                                rfwi4                                 *
*                                                                      *
*     Write a single long into a binary data file, possibly not on a   *
*  boundary.  Assuming the file is big-endian, swap if necessary from  *
*  the current host byte order.  Access the output buffer directly if  *
*  possible to avoid storing data in an intermediate buffer.  Other-   *
*  wise, use rfwrite to write the data and abort on any kind of error. *
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

void rfwi4(struct RFdef *fd, si32 i4) {

/* If the item will fit contiguously in the
*  current buffer, just store it directly.  */

   if (fd->lbsr >= FMJSIZE) {
      bemfmi4(fd->bptr, i4);
      fd->bptr += FMJSIZE;
      fd->aoff += FMJSIZE;
      fd->lbsr -= FMJSIZE;
      }

/* Item will not fit contiguously in the
*  current buffer, do a standard rfwrite.  */

   else {
      char ci4[FMJSIZE];
      bemfmi4(ci4, i4);
      rfwrite(fd, ci4, FMJSIZE, ABORT);
      }

   } /* End rfwi4() */
