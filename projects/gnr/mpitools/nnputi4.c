/* (c) Copyright 1999-2016, The Rockefeller University *11115* */
/* $Id: nnputi4.c 3 2017-01-16 19:29:37Z  $ */
/***********************************************************************
*                          MPI Tools Library                           *
*                               nnputi4                                *
*                                                                      *
*     Write a single si32 to a message data stream, possibly not on    *
*  a boundary. Assuming the stream is little-endian, swap if necessary *
*  from the current host byte order. Access the output buffer directly *
*  if possible to avoid storing data in an intermediate buffer. Other- *
*  wise, use nnput to write the data and abort on any kind of error.   *
*                                                                      *
************************************************************************
*  V1A, 06/05/99, GNR - New routine                                    *
*  ==>, 12/29/09, GNR - Last mod before committing to svn repository   *
*  Rev, 05/23/16, GNR - Long to si32 for 64-bit MPI version            *
*  ==>, 05/23/16, GNR - Last mod before committing to svn mpi repo     *
***********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "mpitools.h"
#include "swap.h"

void nnputi4(struct NNSTR *pnn, si32 i4) {

/* If the item will fit contiguously in the
*  current buffer, just store it directly.  */

   if (pnn->lbsr >= FMJSIZE) {
      lemfmi4(pnn->bptr, i4);
      pnn->bptr += FMJSIZE;
      pnn->lbsr -= FMJSIZE;
      }

/* Item will not fit contiguous in the
*  current buffer, do a standard nnput.  */

   else {
      char ci4[FMJSIZE];
      lemfmi4(ci4, i4);
      nnput(pnn, ci4, FMJSIZE);
      }

   } /* End nnputi4() */

