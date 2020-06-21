/* (c) Copyright 2016, The Rockefeller University *11115* */
/* $Id: nngetil.c 3 2017-01-16 19:29:37Z  $ */
/***********************************************************************
*                          MPI Tools Library                           *
*                               nngetil                                *
*                                                                      *
*     Read a single long, possibly unaligned, from a message data      *
*  stream.  Assuming the stream is little-endian, swap if necessary    *
*  to the current host byte order.  Access the input buffer directly   *
*  if possible to avoid copying data to an intermediate buffer.        *
*  Otherwise, use nnget to read the data and abort on errors.          *
*                                                                      *
*  Swapping code from lemtoi8() is copied here to avoid call overhead  *
************************************************************************
*  V1A, 05/02/16, GNR - New routine, based on nngeti4()                *
*  ==>, 05/02/16, GNR - Last mod before committing to svn mpi repo     *
***********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "mpitools.h"
#include "swap.h"

#if FMLSIZE != 8
#error nngetil assumes message length of long is 8
#endif

long nngetil(struct NNSTR *pnn) {

   si64 ril;                  /* Temp for message-sized value */
   byte *m, mc[FMLSIZE];      /* Value in message */

/* Get FMLSIZE bytes from the message buffer */

   if (pnn->lbsr >= FMLSIZE) {
      /* Item is contiguous in message buffer, just locate it */
      m = pnn->bptr;
      pnn->bptr += FMLSIZE;
      pnn->lbsr -= FMLSIZE;
      }
   else {
      /* Item is not contiguous in memory, do a standard nnget */
      nnget(pnn, mc, FMLSIZE);
      m = mc;
      }

#if BYTE_ORDRE > 0
/* System is big-endian, swapping is needed */
#ifdef HAS_I64
   ril = (si64)(((((((
      (ui64)m[7]  << BITSPERBYTE |
      (ui64)m[6]) << BITSPERBYTE |
      (ui64)m[5]) << BITSPERBYTE |
      (ui64)m[4]) << BITSPERBYTE |
      (ui64)m[3]) << BITSPERBYTE |
      (ui64)m[2]) << BITSPERBYTE |
      (ui64)m[1]) << BITSPERBYTE |
      (ui64)m[0]);
#else /* ui64 is implemented as a structure */
   ril.hi = (si32)(((((((ui32)m[7] << BITSPERBYTE) |
      (ui32)m[6]) << BITSPERBYTE) | (ui32)
      m[5]) << BITSPERBYTE) | (ui32)m[4]);
   ril.lo = ((((((ui32)m[3] << BITSPERBYTE) |
      (ui32)m[2]) << BITSPERBYTE) | (ui32)
      m[1]) << BITSPERBYTE) | (ui32)m[0];
#endif

#else
/* System is little-endian, swapping is not needed */
   memcpy((char *)&ril, m, WSIZE);
#endif

#if FMLSIZE > LSIZE
   if (jckslo(ril)) {
      e64act("nngetil", EAabx(86));
      return ril >= 0 ? (long)SI32_MAX : -(long)SI32_MAX;
      }
   return (long)swlo(ril);
#else    /* Long is 64 bits */
   return (long)ril;
#endif

   } /* End nngetil() */

