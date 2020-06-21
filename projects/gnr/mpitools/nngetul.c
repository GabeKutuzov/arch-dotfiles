/* (c) Copyright 2016, The Rockefeller University *11115* */
/* $Id: nngetul.c 3 2017-01-16 19:29:37Z  $ */
/***********************************************************************
*                          MPI Tools Library                           *
*                               nngetul                                *
*                                                                      *
*     Read a single ulng, possibly unaligned, from a message data      *
*  stream.  Assuming the stream is little-endian, swap if necessary    *
*  to the current host byte order.  Access the input buffer directly   *
*  if possible to avoid copying data to an intermediate buffer.        *
*  Otherwise, use nnget to read the data and abort on errors.          *
*                                                                      *
*  Swapping code from lemtou8() is copied here to avoid call overhead  *
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
#error nngetul assumes message length of ulng is 8
#endif

ulng nngetul(struct NNSTR *pnn) {

   ui64 ril;                  /* Temp for message-sized value */
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
   ril = ((((((
      (ui64)m[7]  << BITSPERBYTE |
      (ui64)m[6]) << BITSPERBYTE |
      (ui64)m[5]) << BITSPERBYTE |
      (ui64)m[4]) << BITSPERBYTE |
      (ui64)m[3]) << BITSPERBYTE |
      (ui64)m[2]) << BITSPERBYTE |
      (ui64)m[1]) << BITSPERBYTE |
      (ui64)m[0];
#else /* ui64 is implemented as a structure */
   ril.hi = ((((((ui32)m[7] << BITSPERBYTE) |
      (ui32)m[6]) << BITSPERBYTE) | (ui32)
      m[5]) << BITSPERBYTE) | (ui32)m[4];
   ril.lo = ((((((ui32)m[3] << BITSPERBYTE) |
      (ui32)m[2]) << BITSPERBYTE) | (ui32)
      m[1]) << BITSPERBYTE) | (ui32)m[0];
#endif

#else
/* System is little-endian, swapping is not needed */
   memcpy((char *)&ril, m, WSIZE);
#endif

#if FMLSIZE > LSIZE
   if (jckulo(ril)) {
      e64act("nngetul", EAabx(86)); return (ulng)UI32_MAX; }
   return (ulng)uwlo(ril);
#else    /* Long is 64 bits */
   return (ulng)ril;
#endif

   } /* End nngetul() */

