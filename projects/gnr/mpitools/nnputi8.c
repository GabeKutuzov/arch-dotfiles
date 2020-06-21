/* (c) Copyright 1999-2016, The Rockefeller University *11115* */
/* $Id: nnputi8.c 3 2017-01-16 19:29:37Z  $ */
/***********************************************************************
*                          MPI Tools Library                           *
*                               nnputi8                                *
*                                                                      *
*     Write a single si64 to a message data stream, possibly not on    *
*  a boundary. Assuming the stream is little-endian, swap if necessary *
*  from the current host byte order. Access the output buffer directly *
*  if possible to avoid storing data in an intermediate buffer. Other- *
*  wise, use nnput to write the data and abort on any kind of error.   *
*                                                                      *
*  Unlike the case with base-type integers, we need separate routines  *
*  for signed and unsigned wide integers because they may be           *
*  implemented as structures, in which case there are no intrinsic     *
*  typecast routines to interconvert them.                             *
*                                                                      *
************************************************************************
*  V1A, 10/24/99, GNR - New routine                                    *
*  ==>, 12/29/09, GNR - Last mod before committing to svn repository   *
*  Rev, 05/23/16, GNR - Copy code of lemfmi8() to remove call overhead *
*  ==>, 05/23/16, GNR - Last mod before committing to svn mpi repo     *
***********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "mpitools.h"
#include "swap.h"

void nnputi8(struct NNSTR *pnn, si64 i8) {

/* Be sure there is room in current buffer for FMWSIZE bytes
*  or else flush current buffer and start another one.  */

   char *m = nnputp(pnn, FMWSIZE);

#if BYTE_ORDRE > 0
/* System is big-endian, swapping is needed */
#ifdef HAS_I64
   register ui64 tu8 = (ui64)i8;
   m[0] = (char)tu8;
   m[1] = (char)(tu8 >>= BITSPERBYTE);
   m[2] = (char)(tu8 >>= BITSPERBYTE);
   m[3] = (char)(tu8 >>= BITSPERBYTE);
   m[4] = (char)(tu8 >>= BITSPERBYTE);
   m[5] = (char)(tu8 >>= BITSPERBYTE);
   m[6] = (char)(tu8 >>= BITSPERBYTE);
   m[7] = (char)(tu8 >>  BITSPERBYTE);
#else /* si64 is implemented as a structure */
   register ui32 i4;
   m[0] = (char)(i4 = i8.lo);
   m[1] = (char)(i4 >>= BITSPERBYTE);
   m[2] = (char)(i4 >>= BITSPERBYTE);
   m[3] = (char)(i4 >> BITSPERBYTE);
   m[4] = (char)(i4 = i8.hi);
   m[5] = (char)(i4 >>= BITSPERBYTE);
   m[6] = (char)(i4 >>= BITSPERBYTE);
   m[7] = (char)(i4 >> BITSPERBYTE);
#endif

#else    /* BYTE_ORDRE < 0 */

   memcpy(m, (char *)&i8, WSIZE);

#endif

/* Register the transfer */

   nnpsk(pnn, FMWSIZE);

   } /* End nnputi8() */

