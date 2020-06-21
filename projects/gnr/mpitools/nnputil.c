/* (c) Copyright 2016, The Rockefeller University *11115* */
/* $Id: nnputil.c 3 2017-01-16 19:29:37Z  $ */
/***********************************************************************
*                          MPI Tools Library                           *
*                               nnputil                                *
*                                                                      *
*     Write a single long to a message data stream, possibly not on    *
*  a boundary. Assuming the stream is little-endian, swap if necessary *
*  from the current host byte order. Access the output buffer directly *
*  if possible to avoid storing data in an intermediate buffer. Other- *
*  wise, use nnput to write the data and abort on any kind of error.   *
*                                                                      *
*  Data in memory may be 32 or 64 bits long, but output is expanded to *
*  FMLSIZE (8 bytes) for transmission between unlike hosts.  This is   *
*  the version for signed longs, which require sign extension if       *
*  such expansion is needed.                                           *
*                                                                      *
************************************************************************
*  V1A, 05/24/16, GNR - New routine, based on nnputi8                  *
*  ==>, 05/24/16, GNR - Last mod before committing to svn mpi repo     *
***********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "mpitools.h"
#include "swap.h"

void nnputil(struct NNSTR *pnn, long il) {

/* Be sure there is room in current buffer for FMLSIZE bytes
*  or else flush current buffer and start another one.  */

   char *m = nnputp(pnn, FMLSIZE);

#if BYTE_ORDRE > 0
/* System is big-endian, swapping is needed */
#ifdef HAS_I64
   /* Sign-extend to 8 bytes if long is only 4, but then
   *  treat as unsigned for the right shifts.  */
   register ui64 tu8 = (ui64)(si64)il;
   m[0] = (char)tu8;
   m[1] = (char)(tu8 >>= BITSPERBYTE);
   m[2] = (char)(tu8 >>= BITSPERBYTE);
   m[3] = (char)(tu8 >>= BITSPERBYTE);
   m[4] = (char)(tu8 >>= BITSPERBYTE);
   m[5] = (char)(tu8 >>= BITSPERBYTE);
   m[6] = (char)(tu8 >>= BITSPERBYTE);
   m[7] = (char)(tu8 >>  BITSPERBYTE);
#else
   /* System does not have 64-bit arithmetic, so a long must
   *  be 32 bits.  Upper 32 are just copies of the sign.  */
   register ui32 tu4 = (ui32)il;
   m[0] = (char)tu4;
   m[1] = (char)(tu4 >>= BITSPERBYTE);
   m[2] = (char)(tu4 >>= BITSPERBYTE);
   m[3] = (char)(tu4 >>  BITSPERBYTE);
   tu4 = (ui32)(si32)SRA(il, (BITSPERUI32-1));
   m[4] = m[5] = m[6] = m[7] = (char)tu4;
#endif

#else    /* BYTE_ORDRE < 0 */

#if FMLSIZE > LSIZE
   ui32 tu4 = (ui32)(si32)SRA(il, (BITSPERUI32-1));
   memcpy(m, (char *)&il, LSIZE);
   memcpy(m+4, (char *)&tu4, LSIZE);
#else
   memcpy(m, (char *)&il, LSIZE);
#endif

#endif

/* Register the transfer */

   nnpsk(pnn, FMLSIZE);

   } /* End nnputil() */

