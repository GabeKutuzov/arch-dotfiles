/* (c) Copyright 1999-2008, The Rockefeller University *11115* */
/* $Id: bemfmu8.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                               bemfmu8                                *
*                                                                      *
*  ROCKS library routine for storing unsigned long long integer        *
*  data in an binary data file in standard big-endian format.  It is   *
*  assumed that the msg pointer may point to an unaligned storage      *
*  area, which means the data must be stored byte-by-byte even if      *
*  storage order matches.                                              *
*                                                                      *
************************************************************************
*  V2A, 10/17/99, GNR - New scheme for binary data conversions         *
*  V2B, 03/09/06, GNR - Allow for case where long is 64 bits           *
*  ==>, 03/09/06, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include "sysdef.h"
#include "swap.h"
#include <string.h>

#if WSIZE != 8 || FMWSIZE != 8
#error Special bemfmu8 routine needed on this machine
#endif

void bemfmu8(char *m, ui64 u8) {

#if BYTE_ORDRE < 0

#ifdef HAS_I64
   register ui64 tu8 = (ui64)u8;
   m[7] = (char)tu8;
   m[6] = (char)(tu8 >>= BITSPERBYTE);
   m[5] = (char)(tu8 >>= BITSPERBYTE);
   m[4] = (char)(tu8 >>= BITSPERBYTE);
   m[3] = (char)(tu8 >>= BITSPERBYTE);
   m[2] = (char)(tu8 >>= BITSPERBYTE);
   m[1] = (char)(tu8 >>= BITSPERBYTE);
   m[0] = (char)(tu8 >> BITSPERBYTE);
#else /* ui64 is implemented as a structure */
   register ui32 i4;
   m[7] = (char)(i4 = u8.lo);
   m[6] = (char)(i4 >>= BITSPERBYTE);
   m[5] = (char)(i4 >>= BITSPERBYTE);
   m[4] = (char)(i4 >> BITSPERBYTE);
   m[3] = (char)(i4 = u8.hi);
   m[2] = (char)(i4 >>= BITSPERBYTE);
   m[1] = (char)(i4 >>= BITSPERBYTE);
   m[0] = (char)(i4 >> BITSPERBYTE);
#endif

#else    /* BYTE_ORDRE > 0 */

   memcpy(m, (char *)&u8, WSIZE);

#endif

   } /* End bemfmu8() */
