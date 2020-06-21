/* (c) Copyright 2015, The Rockefeller University *11115* */
/* $Id: s2mf.c 5 2008-03-13 20:32:23Z  $ */
/***********************************************************************
*                     ROCKS metafile plot library                      *
*                               s2mf()                                 *
*                                                                      *
*  This routine converts a signed 32-bit integer, 's', into the        *
*  compressed form described in the documentation and emits the result *
*  into the metafile buffer.                                           *
*  There are four output cases:                                        *
*     00 + sign + 5 data bits                                          *
*     01 + sign + 13 data bits                                         *
*     10 + sign + 21 data bits                                         *
*     11 + sign + 29 data bits                                         *
*                                                                      *
************************************************************************
*  V2A, 03/29/15, GNR - New program                                    *
***********************************************************************/

#include "mfint.h"

void s2mf(si32 s) {

   ui32 as = abs32(s);
   if (as <= (ITST00>>1))      mfbitpk((s & ITST00), 8);
   else if (as <= (ITST01>>1)) mfbitpk((s & ITST01) | ITYP01, 16);
   else if (as <= (ITST10>>1)) mfbitpk((s & ITST10) | ITYP10, 24);
   else                        mfbitpk(s | ITYP11, 32);

   } /* End s2mf() */
