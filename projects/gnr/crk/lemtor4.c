/* (c) Copyright 1999-2008, The Rockefeller University *11115* */
/* $Id: lemtor4.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                               lemtor4                                *
*                                                                      *
*  ROCKS library routine for retrieving floating point data from an    *
*  inter-processor message in standard little-endian format.  It is    *
*  assumed that the msg pointer may point to an unaligned storage      *
*  area, which means the data must be retrieved byte-by-byte even if   *
*  the storage order already matches.                                  *
*                                                                      *
************************************************************************
*  V2A, 05/08/99, GNR - New scheme for binary data conversions         *
*  V2B, 03/09/06, GNR - Allow for case where long is 64 bits           *
*  ==>, 03/09/06, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include "sysdef.h"
#include "swap.h"
#include <string.h>

#if ESIZE != 4 || FMESIZE != ESIZE || defined(IBM370)
#error Special lemtor4 routine needed on this machine
#endif

float lemtor4(char *m) {

#if BYTE_ORDRE > 0
   register byte *um = (byte *)m;
   union {
      float ur4;
      ui32 ui4;
      } u;
   u.ui4 = ((((((ui32)um[3] << BITSPERBYTE) | \
      (ui32)um[2]) << BITSPERBYTE) | \
      (ui32)um[1]) << BITSPERBYTE) | (ui32)um[0];
   return u.ur4;
#else
   float ur4;
   memcpy((char *)&ur4, m, ESIZE);
   return ur4;
#endif

   } /* End lemtor4() */
