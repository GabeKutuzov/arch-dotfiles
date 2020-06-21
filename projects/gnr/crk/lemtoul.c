/* (c) Copyright 1999-2008, The Rockefeller University *11115* */
/* $Id: lemtoul.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                               lemtou8                                *
*                                                                      *
*  ROCKS library routine for retrieving an unsigned long long integer  *
*  from an interprocessor message in standard little-endian format.    *
*  It is assumed that the msg pointer may point to an unaligned storage*
*  area, which means the data must be retrieved byte-by-byte even if   *
*  the storage order already matches.                                  *
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
#error Special lemtou8 routine needed on this machine
#endif

ui64 lemtou8(char *m) {

#if BYTE_ORDRE > 0

#ifdef HAS_I64
   register byte *um = (byte *)m;
   register ui64 u8 = ((((((
      (ui64)um[7]  << BITSPERBYTE |
      (ui64)um[6]) << BITSPERBYTE |
      (ui64)um[5]) << BITSPERBYTE |
      (ui64)um[4]) << BITSPERBYTE |
      (ui64)um[3]) << BITSPERBYTE |
      (ui64)um[2]) << BITSPERBYTE |
      (ui64)um[1]) << BITSPERBYTE |
      (ui64)um[0];
   return (ui64)u8;
#else /* ui64 is implemented as a structure */
   register byte *um = (byte *)m;
   ui64 u8;
   u8.hi = ((((((ui32)um[7] << BITSPERBYTE) |
      (ui32)um[6]) << BITSPERBYTE) | (ui32)
      um[5]) << BITSPERBYTE) | (ui32)um[4];
   u8.lo = ((((((ui32)um[3] << BITSPERBYTE) |
      (ui32)um[2]) << BITSPERBYTE) | (ui32)
      um[1]) << BITSPERBYTE) | (ui32)um[0];
   return u8;
#endif

#else

   ui64 u8;
   memcpy((char *)&u8, m, WSIZE);
   return u8;

#endif

   } /* End lemtou8() */
