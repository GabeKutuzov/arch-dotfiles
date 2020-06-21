/* (c) Copyright 2016, The Rockefeller University *11115* */
/* $Id: nnputul.c 3 2017-01-16 19:29:37Z  $ */
/***********************************************************************
*                          MPI Tools Library                           *
*                               nnputul                                *
*                                                                      *
*     Write a single long to a message data stream, possibly not on    *
*  a boundary. Assuming the stream is little-endian, swap if necessary *
*  from the current host byte order. Access the output buffer directly *
*  if possible to avoid storing data in an intermediate buffer. Other- *
*  wise, use nnput to write the data and abort on any kind of error.   *
*                                                                      *
*  Data in memory may be 32 or 64 bits long, but output is expanded to *
*  FMLSIZE (8 bytes) for transmission between unlike hosts.  This is   *
*  the version for unsigned longs, which require extension with zeros  *
*  if such expansion is needed.                                        *
*                                                                      *
************************************************************************
*  V1A, 05/24/16, GNR - New routine, based on nnputil                  *
*  ==>, 05/24/16, GNR - Last mod before committing to svn mpi repo     *
***********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "mpitools.h"
#include "swap.h"

void nnputul(struct NNSTR *pnn, ulng ul) {

/* Be sure there is room in current buffer for FMLSIZE bytes
*  or else flush current buffer and start another one.  */

   char *m = nnputp(pnn, FMLSIZE);

#if BYTE_ORDRE > 0
/* System is big-endian, swapping is needed */
#ifdef HAS_I64
   /* Extend with zeros to 8 bytes if long is only 4 */
   register ui64 tu8 = (ui64)ul;
   m[0] = (char)tu8;
   m[1] = (char)(tu8 >>= BITSPERBYTE);
   m[2] = (char)(tu8 >>= BITSPERBYTE);
   m[3] = (char)(tu8 >>= BITSPERBYTE);
   m[4] = (char)(tu8 >>= BITSPERBYTE);
   m[5] = (char)(tu8 >>= BITSPERBYTE);
   m[6] = (char)(tu8 >>= BITSPERBYTE);
   m[7] = (char)(tu8 >>  BITSPERBYTE);
#else
   /* System does not have 64-bit arithmetic, so a long
   *  must be 32 bits.  Upper 32 are just zeros.  */
   register ui32 tu4 = (ui32)ul;
   m[0] = (char)tu4;
   m[1] = (char)(tu4 >>= BITSPERBYTE);
   m[2] = (char)(tu4 >>= BITSPERBYTE);
   m[3] = (char)(tu4 >>  BITSPERBYTE);
   m[4] = m[5] = m[6] = m[7] = 0;
#endif

#else    /* BYTE_ORDRE < 0 */

#if FMLSIZE > LSIZE
   ui32 tu4 = 0;
   memcpy(m, (char *)&ul, LSIZE);
   memcpy(m+4, (char *)&tu4, LSIZE);
#else
   memcpy(m, (char *)&ul, LSIZE);
#endif

#endif

/* Register the transfer */

   nnpsk(pnn, FMLSIZE);

   } /* End nnputul() */

