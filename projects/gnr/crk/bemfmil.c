/* (c) Copyright 2016, The Rockefeller University *11115* */
/* $Id: bemfmil.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                               bemfmil                                *
*                                                                      *
*  ROCKS library routine for storing long integer data in a binary     *
*  data file in standard big-endian format.  It is assumed that the    *
*  msg pointer may point to an unaligned storage area, which means the *
*  data must be stored byte-by-byte even if storage order matches.     *
*                                                                      *
*  This routine is the same as bemfmi8 when longs are 64-bits, but     *
*  when longs are only 32 bits, it sign-expands them to 64 bits.       *
************************************************************************
*  V2D, 03/01/16, GNR - New routine                                    *
*  ==>, 03/01/16, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include <string.h>
#include "sysdef.h"
#include "swap.h"

void bemfmil(char *m, long il) {

#if LSIZE == 8
/* A long is 8 bytes */
#if BYTE_ORDRE < 0
   register ulng tu8 = (ulng)il;
   m[7] = (char)tu8;
   m[6] = (char)(tu8 >>= BITSPERBYTE);
   m[5] = (char)(tu8 >>= BITSPERBYTE);
   m[4] = (char)(tu8 >>= BITSPERBYTE);
   m[3] = (char)(tu8 >>= BITSPERBYTE);
   m[2] = (char)(tu8 >>= BITSPERBYTE);
   m[1] = (char)(tu8 >>= BITSPERBYTE);
   m[0] = (char)(tu8 >> BITSPERBYTE);
#else    /* BYTE_ORDRE > 0 */
   memcpy(m, (char *)&il, LSIZE);
#endif

#else
/* A long is 4 bytes */
   register ulng tu4 = (ulng)il;
   m[7] = (char)tu4;
   m[6] = (char)(tu4 >>= BITSPERBYTE);
   m[5] = (char)(tu4 >>= BITSPERBYTE);
   m[4] = (char)(tu4 >>= BITSPERBYTE);
   m[0] = m[1] = m[2] = m[3] = (il >= 0) ? 0 : BYTE_MAX;

#endif /* else LSIZE */

   } /* End bemfmil() */
