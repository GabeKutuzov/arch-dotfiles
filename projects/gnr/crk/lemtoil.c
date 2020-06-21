/* (c) Copyright 1999-2008, The Rockefeller University *11115* */
/* $Id: lemtoil.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                               lemtoi8                                *
*                                                                      *
*  ROCKS library routine for retrieving long long integer data from    *
*  an interprocessor message in standard little-endian format.  It is  *
*  assumed that the msg pointer may point to an unaligned storage      *
*  area, which means the data must be retrieved byte-by-byte even if   *
*  the storage order already matches.                                  *
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
#error Special lemtoi8 routine needed on this machine
#endif

si64 lemtoi8(char *m) {

#if BYTE_ORDRE > 0

#ifdef HAS_I64
   register byte *um = (byte *)m;
   register ui64 ui8 = ((((((
      (ui64)um[7]  << BITSPERBYTE |
      (ui64)um[6]) << BITSPERBYTE |
      (ui64)um[5]) << BITSPERBYTE |
      (ui64)um[4]) << BITSPERBYTE |
      (ui64)um[3]) << BITSPERBYTE |
      (ui64)um[2]) << BITSPERBYTE |
      (ui64)um[1]) << BITSPERBYTE |
      (ui64)um[0];
   return (si64)ui8;
#else /* si64 is implemented as a structure */
   register byte *um = (byte *)m;
   si64 i8;
   i8.hi = ((((((ui32)um[7] << BITSPERBYTE) |
      (ui32)um[6]) << BITSPERBYTE) | (ui32)
      um[5]) << BITSPERBYTE) | (ui32)um[4];
   i8.lo = ((((((ui32)um[3] << BITSPERBYTE) |
      (ui32)um[2]) << BITSPERBYTE) | (ui32)
      um[1]) << BITSPERBYTE) | (ui32)um[0];
   return i8;
#endif

#else

   si64 i8;
   memcpy((char *)&i8, m, WSIZE);
   return i8;

#endif

   } /* End lemtoi8() */
