/* (c) Copyright 1999-2008, The Rockefeller University *11115* */
/* $Id: lemfmu8.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                               lemfmu8                                *
*                                                                      *
*  ROCKS library routine for storing unsigned long long integer data   *
*  in an interprocessor message in standard little-endian format.  It  *
*  is assumed that the msg pointer may point to an unaligned storage   *
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
#error Special lemfmu8 routine needed on this machine
#endif

void lemfmu8(char *m, ui64 u8) {

#if BYTE_ORDRE > 0

#ifdef HAS_I64
   register ui64 tu8 = (ui64)u8;
   m[0] = (char)tu8;
   m[1] = (char)(tu8 >>= BITSPERBYTE);
   m[2] = (char)(tu8 >>= BITSPERBYTE);
   m[3] = (char)(tu8 >>= BITSPERBYTE);
   m[4] = (char)(tu8 >>= BITSPERBYTE);
   m[5] = (char)(tu8 >>= BITSPERBYTE);
   m[6] = (char)(tu8 >>= BITSPERBYTE);
   m[7] = (char)(tu8 >>  BITSPERBYTE);
#else /* ui64 is implemented as a structure */
   register ui32 u4;
   m[0] = (char)(u4 = u8.lo);
   m[1] = (char)(u4 >>= BITSPERBYTE);
   m[2] = (char)(u4 >>= BITSPERBYTE);
   m[3] = (char)(u4 >> BITSPERBYTE);
   m[4] = (char)(u4 = u8.hi);
   m[5] = (char)(u4 >>= BITSPERBYTE);
   m[6] = (char)(u4 >>= BITSPERBYTE);
   m[7] = (char)(u4 >> BITSPERBYTE);
#endif

#else    /* BYTE_ORDRE < 0 */

   memcpy(m, (char *)&u8, WSIZE);

#endif

   } /* End lemfmu8() */
