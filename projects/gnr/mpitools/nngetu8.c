/* (c) Copyright 1999-2016, The Rockefeller University *11115* */
/* $Id: nngetu8.c 3 2017-01-16 19:29:37Z  $ */
/***********************************************************************
*                          MPI Tools Library                           *
*                               nngetu8                                *
*                                                                      *
*     Read a single ui64, possibly unaligned, from a message data      *
*  stream.  Assuming the stream is little-endian, swap if necessary    *
*  to the current host byte order.  Access the input buffer directly   *
*  if possible to avoid copying data to an intermediate buffer.        *
*  Otherwise, use nnget to read the data and abort on errors.          *
*                                                                      *
*  Unlike the case with base-type integers, we need separate routines  *
*  for signed and unsigned wide integers because they may be           *
*  implemented as structures, in which case there are no intrinsic     *
*  typecast routines to interconvert them.                             *
*                                                                      *
************************************************************************
*  V1A, 10/24/99, GNR - New routine                                    *
*  ==>, 12/29/09, GNR - Last mod before committing to svn repository   *
*  Rev, 05/02/16, GNR - Copy code of lemtou8() to remove call overhead *
*  ==>, 05/02/16, GNR - Last mod before committing to svn mpi repo     *
***********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "mpitools.h"
#include "swap.h"

ui64 nngetu8(struct NNSTR *pnn) {

   ui64 ui8;                  /* Result */
   byte *m, mc[FMWSIZE];      /* Value in message */

/* Get FMWSIZE bytes from the message buffer */

   if (pnn->lbsr >= FMWSIZE) {
      /* Item is contiguous in message buffer, just locate it */
      m = pnn->bptr;
      pnn->bptr += FMWSIZE;
      pnn->lbsr -= FMWSIZE;
      }
   else {
      /* Item is not contiguous in memory, do a standard nnget */
      nnget(pnn, mc, FMWSIZE);
      m = mc;
      }

#if BYTE_ORDRE > 0
/* System is big-endian, swapping is needed */
#ifdef HAS_I64
   ui8 = ((((((
      (ui64)m[7]  << BITSPERBYTE |
      (ui64)m[6]) << BITSPERBYTE |
      (ui64)m[5]) << BITSPERBYTE |
      (ui64)m[4]) << BITSPERBYTE |
      (ui64)m[3]) << BITSPERBYTE |
      (ui64)m[2]) << BITSPERBYTE |
      (ui64)m[1]) << BITSPERBYTE |
      (ui64)m[0];
   return ui8;
#else /* ui64 is implemented as a structure */
   ui8.hi = ((((((ui32)m[7] << BITSPERBYTE) |
      (ui32)m[6]) << BITSPERBYTE) | (ui32)
      m[5]) << BITSPERBYTE) | (ui32)m[4];
   ui8.lo = ((((((ui32)m[3] << BITSPERBYTE) |
      (ui32)m[2]) << BITSPERBYTE) | (ui32)
      m[1]) << BITSPERBYTE) | (ui32)m[0];
   return ui8;
#endif

#else
/* System is little-endian, swapping is not needed */
   memcpy((char *)&ui8, m, WSIZE);
   return ui8;
#endif

   } /* End nngetu8() */

