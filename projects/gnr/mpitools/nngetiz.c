/* (c) Copyright 2016, The Rockefeller University *11115* */
/* $Id: nngetiz.c 3 2017-01-16 19:29:37Z  $ */
/***********************************************************************
*                          MPI Tools Library                           *
*                               nngetiz                                *
*                                                                      *
*     Read a single size_t, possibly unaligned, from a message data    *
*  stream.  Assuming the stream is little-endian, swap if necessary    *
*  to the current host byte order.  Access the input buffer directly   *
*  if possible to avoid copying data to an intermediate buffer.        *
*  Otherwise, use nnget to read the data and abort on errors.          *
*                                                                      *
*  Swapping code from lemtoi8() is copied here to avoid call overhead. *
************************************************************************
*  V1A, 05/02/16, GNR - New routine, based on nngeti4()                *
*  ==>, 12/01/16, GNR - Last mod before committing to svn mpi repo     *
***********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "mpitools.h"
#include "swap.h"

#if FMZSIZE != 8
#error nngetiz assumes message length of size_t is 8
#endif

size_t nngetiz(struct NNSTR *pnn) {

   ui64 rz;                   /* Temp for message-sized value */
   byte *m, mc[FMZSIZE];      /* Value in message */

/* Get FMZSIZE bytes from the message buffer */

   if (pnn->lbsr >= FMZSIZE) {
      /* Item is contiguous in message buffer, just locate it */
      m = pnn->bptr;
      pnn->bptr += FMZSIZE;
      pnn->lbsr -= FMZSIZE;
      }
   else {
      /* Item is not contiguous in memory, do a standard nnget */
      nnget(pnn, mc, FMZSIZE);
      m = mc;
      }

#if BYTE_ORDRE > 0
/* System is big-endian, swapping is needed */
#ifdef HAS_I64
   rz = ((((((
      (ui64)m[7]  << BITSPERBYTE |
      (ui64)m[6]) << BITSPERBYTE |
      (ui64)m[5]) << BITSPERBYTE |
      (ui64)m[4]) << BITSPERBYTE |
      (ui64)m[3]) << BITSPERBYTE |
      (ui64)m[2]) << BITSPERBYTE |
      (ui64)m[1]) << BITSPERBYTE |
      (ui64)m[0];
#else /* ui64 is implemented as a structure */
   rz.hi = ((((((ui32)m[7] << BITSPERBYTE) |
      (ui32)m[6]) << BITSPERBYTE) | (ui32)
      m[5]) << BITSPERBYTE) | (ui32)m[4];
   rz.lo = ((((((ui32)m[3] << BITSPERBYTE) |
      (ui32)m[2]) << BITSPERBYTE) | (ui32)
      m[1]) << BITSPERBYTE) | (ui32)m[0];
#endif

#else
/* System is little-endian, swapping is not needed */
   memcpy((char *)&rz, m, ZSIZE);
#endif

#if FMZSIZE > STSIZE
#ifdef HAS_I64
   if (rz > 0x000000007fffffffL) {
      e64act("nngetiz", EAabx(86)); return (size_t)SI32_MAX; }
#else /* No 64-bit arith */
   if (rz.hi | (rz.lo >> (BITSPERUI32-1)) {
      e64act("nngetiz", EAabx(86)); return (size_t)SI32_MAX; }
#endif
   return (size_t)swlo(rz);
#else
   return (size_t)rz;
#endif

   } /* End nngetiz() */

