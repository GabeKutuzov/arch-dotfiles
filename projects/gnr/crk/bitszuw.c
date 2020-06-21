/* (c) Copyright 2006-2011, The Rockefeller University *11115* */
/* $Id: bitszuw.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                               bitszuw                                *
*                                                                      *
*     ROCKS Mathematics Library -- Routine to calculate the number     *
*     of bits needed to store values from 0 up to (x-1) where x is     *
*     a ui64 argument.  (bitsz, bitszsl, and bitszul are macros        *
*     defined in rkarith.h for ui32, signed long, and unsigned long    *
*     arguments, respectively.)                                        *
*                                                                      *
*     There may be a fast machine instruction to implement this        *
*     function on some architectures.                                  *
*                                                                      *
*     Synopsis:  int bitszuw(ui64 x)                                   *
*                                                                      *
*     Returns:   (int)(log2(x-1)+1) if x > 1, otherwise 0.             *
*                                                                      *
************************************************************************
*  V1A, 04/02/06, GNR - New routine                                    *
*  V1B, 12/31/06, GNR - Add 64-bit versions                            *
*  Rev, 04/28/08, GNR - Correct 64-bit bug:  use x-1, not xhi-1        *
*  Rev, 11/15/08, GNR - Add bitszs32, bitszu32, delete bitsz           *
*                       Correct ui32 bug:  treated x = 0 as max32      *
*  ==>, 11/17/08, GNR - Last date before committing to svn repository  *
*  Rev, 12/10/11, GNR - Put in separate source files for smaller loads *
***********************************************************************/

#include <math.h>
#include <stdlib.h>
#include "sysdef.h"
#include "rkarith.h"

int bitszuw(ui64 x) {

   ui64 xm1;
   register ui32 xv;
   register int n;

   if (!quw(x)) return 0;

   xm1 = jrul(x,1);
   xv = uwhi(xm1);

   if (xv) n = BITSPERUI32;
   else n = 0, xv = uwlo(xm1);

   for ( ; xv>0; xv>>=1) n += 1;

   return n;
   } /* End bitszuw() */
