/* (c) Copyright 1999-2008, The Rockefeller University *11115* */
/* $Id: lemfmi8.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                               lemfmi8                                *
*                                                                      *
*  ROCKS library routine for storing long long integer data in an      *
*  interprocessor message in standard little-endian format.  It is     *
*  assumed that the msg pointer may point to an unaligned storage      *
*  area, which means the data must be stored byte-by-byte even if      *
*  storage order matches.                                              *
*                                                                      *
************************************************************************
*  V2A, 10/17/99, GNR - New scheme for binary data conversions         *
*  V2B, 03/09/06, GNR - Allow for case where long is 64 bits           *
*  ==>, 03/09/06, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include <string.h>
#include "sysdef.h"
#include "swap.h"

#if WSIZE != 8 || FMWSIZE != 8
#error Special lemfmi8 routine needed on this machine
#endif

void lemfmi8(char *m, si64 i8) {

#if BYTE_ORDRE > 0

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

   } /* End lemfmi8() */
