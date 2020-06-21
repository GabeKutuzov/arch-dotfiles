/* (c) Copyright 1999-2008, The Rockefeller University *11115* */
/* $Id: lemfmr8.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                               lemfmr8                                *
*                                                                      *
*  ROCKS library routine for storing double-precision floating point   *
*  data in an interprocessor message in standard little-endian format. *
*  It is assumed that the msg pointer may point to an unaligned stor-  *
*  age area, which means the data must be stored byte-by-byte even if  *
*  endian matches.                                                     *
*                                                                      *
************************************************************************
*  V2A, 05/08/99, GNR - New scheme for binary data conversions         *
*  V2B, 03/09/06, GNR - Allow for case where long is 64 bits           *
*  ==>, 03/09/06, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include "sysdef.h"
#include "swap.h"
#include <string.h>

#if DSIZE != 8 || FMDSIZE != 8 || defined(IBM370)
#error Special lemfmr8 routine needed on this machine
#endif

void lemfmr8(char *m, double r8) {

#if BYTE_ORDRE > 0
   register ui32 i4;
   union {
      double ur8;
      ui32   ui4[2];
      } u;

   u.ur8 = r8;
   m[0] = (char)(i4 = u.ui4[1]);
   m[1] = (char)(i4 >>= BITSPERBYTE);
   m[2] = (char)(i4 >>= BITSPERBYTE);
   m[3] = (char)(i4 >> BITSPERBYTE);
   m[4] = (char)(i4 = u.ui4[0]);
   m[5] = (char)(i4 >>= BITSPERBYTE);
   m[6] = (char)(i4 >>= BITSPERBYTE);
   m[7] = (char)(i4 >> BITSPERBYTE);
#else
   memcpy(m, (char *)&r8, DSIZE);
#endif

   } /* End lemfmr8() */
