/* (c) Copyright 1999-2016, The Rockefeller University *11115* */
/* $Id: nngeti4.c 3 2017-01-16 19:29:37Z  $ */
/***********************************************************************
*                          MPI Tools Library                           *
*                               nngeti4                                *
*                                                                      *
*     Read a single si32, possibly unaligned, from a message data      *
*  stream.  Assuming the stream is little-endian, swap if necessary    *
*  to the current host byte order.  Access the input buffer directly   *
*  if possible to avoid copying data to an intermediate buffer.        *
*  Otherwise, use nnget to read the data and abort on errors.          *
*                                                                      *
************************************************************************
*  V1A, 06/05/99, GNR - New routine                                    *
*  ==>, 12/29/09, GNR - Last mod before committing to svn repository   *
*  Rev, 05/02/16, GNR - Long to si32 for 64-bit MPI version            *
*  ==>, 05/23/16, GNR - Last mod before committing to svn mpi repo     *
***********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "mpitools.h"
#include "swap.h"

si32 nngeti4(struct NNSTR *pnn) {

/* If the item lies contiguously in the current
*  buffer, just pick it up directly.  */

   if (pnn->lbsr >= FMJSIZE) {
      si32 ri4 = lemtoi4(pnn->bptr);
      pnn->bptr += FMJSIZE;
      pnn->lbsr -= FMJSIZE;
      return ri4;
      }

/* Item is not contiguous in memory, do a standard nnget */

   else {
      char ci4[FMJSIZE];
      nnget(pnn, ci4, FMJSIZE);
      return (si32)lemtoi4(ci4);
      }

   } /* End nngeti4() */

