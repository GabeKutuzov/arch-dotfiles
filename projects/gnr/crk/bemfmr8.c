/* (c) Copyright 1999-2008, The Rockefeller University *11115* */
/* $Id: bemfmr8.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                               bemfmr8                                *
*                                                                      *
*  ROCKS library routine for storing double-precision floating point   *
*  data in a binary data file in standard big-endian format.  It is    *
*  assumed that the msg pointer may point to an unaligned storage      *
*  area, which means the data must be stored byte-by-byte even if      *
*  storage order matches.                                              *
*                                                                      *
************************************************************************
*  V2A, 05/08/99, GNR - New scheme for binary data conversions         *
*  V2B, 03/09/06, GNR - Allow for case where long is 64 bits           *
*  ==>, 11/08/08, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include <string.h>
#include "sysdef.h"
#include "swap.h"

#if DSIZE != 8 || FMDSIZE != 8 || defined(IBM370)
#error Special bemfmr8 routine needed on this machine
#endif

void bemfmr8(char *m, double r8) {

#if BYTE_ORDRE < 0
   register ui32 i4;
   union {
      double ur8;
      ui32   ui4[2];
      } u;

   u.ur8 = r8;
   m[7] = (char)(i4 = u.ui4[0]);
   m[6] = (char)(i4 >>= BITSPERBYTE);
   m[5] = (char)(i4 >>= BITSPERBYTE);
   m[4] = (char)(i4 >> BITSPERBYTE);
   m[3] = (char)(i4 = u.ui4[1]);
   m[2] = (char)(i4 >>= BITSPERBYTE);
   m[1] = (char)(i4 >>= BITSPERBYTE);
   m[0] = (char)(i4 >> BITSPERBYTE);
#else
   memcpy(m, (char *)&r8, DSIZE);
#endif

   } /* End bemfmr8() */
