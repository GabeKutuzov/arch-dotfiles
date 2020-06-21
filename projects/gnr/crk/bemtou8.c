/* (c) Copyright 1999-2008, The Rockefeller University *11115* */
/* $Id: bemtou8.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                               bemtou8                                *
*                                                                      *
*  ROCKS library routine for retrieving unsigned long long integer     *
*  data from a binary data file in standard big-endian format.  It is  *
*  assumed that the msg pointer may point to an unaligned storage      *
*  area, which means the data must be retrieved byte-by-byte even if   *
*  the storage order already matches.                                  *
*                                                                      *
************************************************************************
*  V2A, 10/17/99, GNR - New scheme for binary data conversions         *
*  ==>, 11/08/08, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include <string.h>
#include "sysdef.h"
#include "swap.h"

#if WSIZE != 8 || FMWSIZE != 8
#error Special bemtou8 routine needed on this machine
#endif

ui64 bemtou8(char *m) {

#if BYTE_ORDRE < 0

#ifdef HAS_I64
   register byte *um = (byte *)m;
   register ui64 u8 = ((((((
      (ui64)um[0]  << BITSPERBYTE |
      (ui64)um[1]) << BITSPERBYTE |
      (ui64)um[2]) << BITSPERBYTE |
      (ui64)um[3]) << BITSPERBYTE |
      (ui64)um[4]) << BITSPERBYTE |
      (ui64)um[5]) << BITSPERBYTE |
      (ui64)um[6]) << BITSPERBYTE |
      (ui64)um[7];
   return (ui64)u8;
#else /* ui64 is implemented as a structure */
   register byte *um = (byte *)m;
   ui64 u8;
   u8.hi = ((((((ui32)um[0] << BITSPERBYTE) |
      (ui32)um[1]) << BITSPERBYTE) | (ui32)
      um[2]) << BITSPERBYTE) | (ui32)um[3];
   u8.lo = ((((((ui32)um[4] << BITSPERBYTE) |
      (ui32)um[5]) << BITSPERBYTE) | (ui32)
      um[6]) << BITSPERBYTE) | (ui32)um[7];
   return u8;
#endif

#else

   ui64 u8;
   memcpy((char *)&u8, m, WSIZE);
   return u8;

#endif

   } /* End bemtou8() */
