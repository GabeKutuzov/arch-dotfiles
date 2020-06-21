/* (c) Copyright 2015, The Rockefeller University *11115* */
/* $Id: cs2mf.c 5 2008-03-13 20:32:23Z  $ */
/***********************************************************************
*                     ROCKS metafile plot library                      *
*                               cs2mf()                                *
*                                                                      *
*  This routine copies a character string to the metafile buffer and   *
*  updates the buffer pointer and bits remaining.  It is assumed that  *
*  mfalign() or mfnxtbb() has already been called to align on a byte   *
*  boundary.  In the usual case that there are 8 bits per byte, this   *
*  routine only needs to use memcpy to copy the string.  Otherwise,    *
*  the string must be copied one character at a time using mfbitpk().  *
*  [This may be incorrect if characters themselves have some length    *
*  other than 8.  This is so unlikely that this case is left as an     *
*  exercise for the future programmer who has to deal with it.]        *
************************************************************************
*  V2A, 05/25/15, GNR - New program                                    *
***********************************************************************/

#include "mfint.h"

void cs2mf(const char *cs, int len) {

#if BITSPERBYTE == 8
   Win *pcw = _RKG.pcw;
   int blen = Bytes2Bits(len);
   mfbchk(blen);
   memcpy(pcw->MFCP, cs, len);
   pcw->MFCP += len;
   pcw->MFBrem -= blen;
#else
   char *cse = cs + len;
   while (cs < cse) mfbitpk((ui32)(*cs++), 8);
#endif

   } /* End cs2mf() */
