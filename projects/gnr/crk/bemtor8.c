/* (c) Copyright 1999-2008, The Rockefeller University *11115* */
/* $Id: bemtor8.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                               bemtor8                                *
*                                                                      *
*  ROCKS library routine for retrieving double-precision floating      *
*  point data from a binary data file in standard big-endian format.   *
*  It is assumed that the msg pointer may point to an unaligned        *
*  storage area, which means the data must be retrieved byte-by-byte   *
*  even if the storage order already matches.                          *
*                                                                      *
************************************************************************
*  V2A, 05/08/99, GNR - New scheme for binary data conversions         *
*  ==>, 03/09/06, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include "sysdef.h"
#include "swap.h"
#include <string.h>

#if DSIZE != 8 || FMDSIZE != DSIZE || defined(IBM370)
#error Special bemtor8 routine needed on this machine
#endif

double bemtor8(char *m) {

#if BYTE_ORDRE < 0
   register byte *um = (byte *)m;
   union {
      double ur8;
      ui32   ui4[2];
      } u;
   u.ui4[1] = ((((((ui32)(byte)um[0] << BITSPERBYTE) |
      (ui32)(byte)um[1]) << BITSPERBYTE) | (ui32)
      (byte)um[2]) << BITSPERBYTE) | (ui32)(byte)um[3];
   u.ui4[0] = ((((((ui32)(byte)um[4] << BITSPERBYTE) |
      (ui32)(byte)um[5]) << BITSPERBYTE) | (ui32)
      (byte)um[6]) << BITSPERBYTE) | (ui32)(byte)um[7];
   return u.ur8;
#else
   double ur8;
   memcpy((char *)&ur8, m, DSIZE);
   return ur8;
#endif

   } /* End bemtor8() */
