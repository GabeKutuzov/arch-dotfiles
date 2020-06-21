/* (c) Copyright 2015, The Rockefeller University *11115* */
/* $Id: k2mf.c 5 2008-03-13 20:32:23Z  $ */
/***********************************************************************
*                     ROCKS metafile plot library                      *
*                               k2mf()                                 *
*                                                                      *
*  This routine converts an unsigned 32-bit integer, 'k', into the     *
*  compressed form described in the documentation and emits the result *
*  into the metafile buffer.                                           *
*  There are four output cases:                                        *
*     00 +  6 data bits                                                *
*     01 + 14 data bits                                                *
*     10 + 22 data bits                                                *
*     11 + 30 data bits                                                *
************************************************************************
*  V2A, 03/29/15, GNR - New program                                    *
***********************************************************************/

#include "mfint.h"

void k2mf(ui32 k) {

   if (k <= ITST00)      mfbitpk(k, 8);
   else if (k <= ITST01) mfbitpk(k | ITYP01, 16);
   else if (k <= ITST10) mfbitpk(k | ITYP10, 24);
   else                  mfbitpk(k | ITYP11, 32);

   } /* End k2mf() */
