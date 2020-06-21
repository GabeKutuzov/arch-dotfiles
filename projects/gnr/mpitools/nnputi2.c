/* (c) Copyright 1999-2009, The Rockefeller University *11115* */
/* $Id: nnputi2.c 3 2017-01-16 19:29:37Z  $ */
/***********************************************************************
*                          MPI Tools Library                           *
*                               nnputi2                                *
*                                                                      *
*     Write a single short to a message data stream, possibly not on   *
*  a boundary. Assuming the stream is little-endian, swap if necessary *
*  from the current host byte order. Access the output buffer directly *
*  if possible to avoid storing data in an intermediate buffer. Other- *
*  wise, use nnput to write the data and abort on any kind of error.   *
*                                                                      *
************************************************************************
*  V1A, 06/05/99, GNR - New routine                                    *
*  ==>, 04/26/16, GNR - Last mod before committing to svn repository   *
***********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "mpitools.h"
#include "swap.h"

void nnputi2(struct NNSTR *pnn, short i2) {

/* If the item will fit contiguously in the
*  current buffer, just store it directly.  */

   if (pnn->lbsr >= FMSSIZE) {
      lemfmi2(pnn->bptr, i2);
      pnn->bptr += FMSSIZE;
      pnn->lbsr -= FMSSIZE;
      }

/* Item will not fit contiguous in the
*  current buffer, do a standard nnput.  */

   else {
      char ci2[FMSSIZE];
      lemfmi2(ci2, i2);
      nnput(pnn, ci2, FMSSIZE);
      }

   } /* End nnputi2() */

