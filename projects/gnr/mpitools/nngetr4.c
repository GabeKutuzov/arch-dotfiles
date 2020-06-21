/* (c) Copyright 1999-2009, The Rockefeller University *11115* */
/* $Id: nngetr4.c 3 2017-01-16 19:29:37Z  $ */
/***********************************************************************
*                          MPI Tools Library                           *
*                               nngetr4                                *
*                                                                      *
*     Read a single float, possibly unaligned, from a message data     *
*  stream.  Assuming the stream is little-endian, swap if necessary    *
*  to the current host byte order.  Access the input buffer directly   *
*  if possible to avoid copying data to an intermediate buffer.        *
*  Otherwise, use nnget to read the data and abort on errors.          *
*                                                                      *
*  Note:  The code for lemtor4() is not copied here, as conversion     *
*  between different floating-point formats may be implied.            *
************************************************************************
*  V1A, 06/05/99, GNR - New routine                                    *
*  ==>, 05/02/16, GNR - Last mod before committing to svn repository   *
***********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "mpitools.h"
#include "swap.h"

float nngetr4(struct NNSTR *pnn) {

/* If the item lies contiguously in the current
*  buffer, just pick it up directly.  */

   if (pnn->lbsr >= FMESIZE) {
      float rr4 = lemtor4((char *)pnn->bptr);
      pnn->bptr += FMESIZE;
      pnn->lbsr -= FMESIZE;
      return rr4;
      }

/* Item is not contiguous in memory, do a standard nnget */

   else {
      char cr4[FMESIZE];
      nnget(pnn, cr4, FMESIZE);
      return lemtor4(cr4);
      }

   } /* End nngetr4() */

