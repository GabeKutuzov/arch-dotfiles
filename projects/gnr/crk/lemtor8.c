/* (c) Copyright 1999-2008, The Rockefeller University *11115* */
/* $Id: lemtor8.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                               lemtor8                                *
*                                                                      *
*  ROCKS library routine for retrieving double-precision floating      *
*  point data from an inter-processor message in standard little-      *
*  endian format.  It is assumed that the msg pointer may point to     *
*  an unaligned storage area, which means the data must be retrieved   *
*  byte-by-byte even if the storage order already matches.             *
*                                                                      *
************************************************************************
*  V2A, 05/08/99, GNR - New scheme for binary data conversions         *
*  ==>, 03/09/06, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include "sysdef.h"
#include "swap.h"
#include <string.h>

#if DSIZE != 8 || FMDSIZE != 8 || defined(IBM370)
#error Special lemtor8 routine needed on this machine
#endif

double lemtor8(char *m) {

#if BYTE_ORDRE > 0
   register byte *um = (byte *)m;
   union {
      double ur8;
      ui32   ui4[2];
      } u;
   u.ui4[0] = ((((((ui32)um[7] << BITSPERBYTE) | \
      (ui32)um[6]) << BITSPERBYTE) | \
      (ui32)um[5]) << BITSPERBYTE) | (ui32)um[4];
   u.ui4[1] = ((((((ui32)um[3] << BITSPERBYTE) | \
      (ui32)um[2]) << BITSPERBYTE) | \
      (ui32)um[1]) << BITSPERBYTE) | (ui32)um[0];
   return u.ur8;
#else
   double ur8;
   memcpy((char *)&ur8, m, DSIZE);
   return ur8;
#endif

   } /* End lemtor8() */
