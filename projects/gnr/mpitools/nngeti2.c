/* (c) Copyright 1999-2009, The Rockefeller University *11115* */
/* $Id: nngeti2.c 3 2017-01-16 19:29:37Z  $ */
/***********************************************************************
*                          MPI Tools Library                           *
*                               nngeti2                                *
*                                                                      *
*     Read a single short, possibly unaligned, from a message data     *
*  stream.  Assuming the stream is little-endian, swap if necessary    *
*  to the current host byte order.  Access the input buffer directly   *
*  if possible to avoid copying data to an intermediate buffer.        *
*  Otherwise, use nnget to read the data and abort on errors.          *
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

short nngeti2(struct NNSTR *pnn) {

/* If the item lies contiguously in the current
*  buffer, just pick it up directly.  */

   if (pnn->lbsr >= FMSSIZE) {
      short ri2 = lemtoi2(pnn->bptr);
      pnn->bptr += FMSSIZE;
      pnn->lbsr -= FMSSIZE;
      return ri2;
      }

/* Item is not contiguous in memory, do a standard nnget */

   else {
      char ci2[FMSSIZE];
      nnget(pnn, ci2, FMSSIZE);
      return lemtoi2(ci2);
      }

   } /* End nngeti2() */

