/* (c) Copyright 1999-2008, The Rockefeller University *11115* */
/* $Id: bemfmr4.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                               bemfmr4                                *
*                                                                      *
*  ROCKS library routine for storing floating point data in a binary   *
*  data file in standard big-endian format.  It is assumed that the    *
*  msg pointer may point to an unaligned storage area, which means     *
*  the data must be stored byte-by-byte even if byte order matches.    *
*                                                                      *
************************************************************************
*  V2A, 05/08/99, GNR - New scheme for binary data conversions         *
*  V2B, 03/09/06, GNR - Allow for case where long is 64 bits           *
*  ==>, 11/08/08, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include <string.h>
#include "sysdef.h"
#include "swap.h"

#if ESIZE != 4 || FMESIZE != 4 || defined(IBM370)
#error Special bemfmr4 routine needed on this machine
#endif

void bemfmr4(char *m, float r4) {

#if BYTE_ORDRE < 0
   register ui32 i4;
   union {
      float ur4;
      ui32  ui4;
      } u;

   u.ur4 = r4;
   m[3] = (char)(i4 = u.ui4);
   m[2] = (char)(i4 >>= BITSPERBYTE);
   m[1] = (char)(i4 >>= BITSPERBYTE);
   m[0] = (char)(i4 >> BITSPERBYTE);
#else
   memcpy(m, (char *)&r4, FMESIZE);
#endif

   } /* End bemfmr4() */
