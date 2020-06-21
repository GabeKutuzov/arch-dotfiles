/* (c) Copyright 1999-2008, The Rockefeller University *11115* */
/* $Id: lemfmr4.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                               lemfmr4                                *
*                                                                      *
*  ROCKS library routine for storing floating point data in an inter-  *
*  processor message in standard little-endian format.  It is assumed  *
*  that the msg pointer may point to an unaligned storage area, which  *
*  means the data must be stored byte-by-byte even if endian matches.  *
*                                                                      *
************************************************************************
*  V2A, 05/08/99, GNR - New scheme for binary data conversions         *
*  V2B, 03/09/06, GNR - Allow for case where long is 64 bits           *
*  ==>, 03/09/06, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include "sysdef.h"
#include "swap.h"
#include <string.h>

#if ESIZE != 4 || FMESIZE != 4 || defined(IBM370)
#error Special lemfmr4 routine needed on this machine
#endif

void lemfmr4(char *m, float r4) {

#if BYTE_ORDRE > 0
   register ui32 i4;
   union {
      float ur4;
      ui32  ui4;
      } u;

   u.ur4 = r4;
   m[0] = (char)(i4 = u.ui4);
   m[1] = (char)(i4 >>= BITSPERBYTE);
   m[2] = (char)(i4 >>= BITSPERBYTE);
   m[3] = (char)(i4 >> BITSPERBYTE);
#else
   memcpy(m, (char *)&r4, ESIZE);
#endif

   } /* End lemfmr4() */
