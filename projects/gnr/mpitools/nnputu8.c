/* (c) Copyright 1999-2009, The Rockefeller University *11115* */
/* $Id: nnputu8.c 3 2017-01-16 19:29:37Z  $ */
/***********************************************************************
*                          MPI Tools Library                           *
*                               nnputu8                                *
*                                                                      *
*     Write a single ui64 to a message data stream, possibly not on    *
*  a boundary. Assuming the stream is little-endian, swap if necessary *
*  from the current host byte order. Access the output buffer directly *
*  if possible to avoid storing data in an intermediate buffer. Other- *
*  wise, use nnput to write the data and abort on any kind of error.   *
*                                                                      *
*  Unlike the case with base-type integers, we need separate routines  *
*  for signed and unsigned wide integers because they are usually      *
*  implemented as structures, in which case there are no intrinsic     *
*  typecast routines to interconvert them.                             *
*                                                                      *
************************************************************************
*  V1A, 10/24/99, GNR - New routine                                    *
*  ==>, 04/26/16, GNR - Last mod before committing to svn repository   *
***********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "mpitools.h"
#include "swap.h"

void nnputu8(struct NNSTR *pnn, ui64 u8) {

/* If the item will fit contiguously in the
*  current buffer, just store it directly.  */

   if (pnn->lbsr >= FMWSIZE) {
      lemfmu8((char *)pnn->bptr, u8);
      pnn->bptr += FMWSIZE;
      pnn->lbsr -= FMWSIZE;
      }

/* Item will not fit contiguous in the
*  current buffer, do a standard nnput.  */

   else {
      char cu8[FMWSIZE];
      lemfmu8(cu8, u8);
      nnput(pnn, cu8, FMWSIZE);
      }

   } /* End nnputu8() */

