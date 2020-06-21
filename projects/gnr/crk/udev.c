/* (c) Copyright 1989-2009, The Rockefeller University *11115* */
/* $Id: udev.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                                udev                                  *
*                                                                      *
*  This function generates a single uniformly distributed pseudo-      *
*  random number in the range 1 <= ir < (2**31).                       *
*                                                                      *
*  Synopsis:  si32 udev(si32 *seed)                                    *
*                                                                      *
*  Algorithm:  ir = new seed = seed*(7**5) MODULO (2**31-1)            *
*                                                                      *
*  Fortunately, the MODULO operation can be carried out without        *
*  actual division because of the special form of the divisor          *
*  (because (a*(2**31) + b) mod (2**31-1) = a + b).                    *
*                                                                      *
*  This routine is implemented in Assembly language on most machines   *
*  because the lack of provision for 64-bit multiplication and 32-bit  *
*  arithmetic carry in the C language makes implementation clumsy and  *
*  execution slow in C.  The present C code is intended to provide a   *
*  working version to get started on a new port.  Exact compatibility  *
*  with the traditional udev routine is the goal here, not speed.      *
*  Examination of the code emitted by the C compiler may provide a     *
*  useful template for a new Assembler implementation.                 *
*                                                                      *
************************************************************************
*  Initial version, 06/12/89, G. N. Reeke                              *
*  Rev, 03/09/06, GNR - Change long to si32                            *
*  ==>, 03/16/06, GNR - Last date before committing to svn repository  *
*  Rev, 12/22/09, GNR - Clean up for 64 vs 32-bit compilation          *
***********************************************************************/

#include <stddef.h>
#include <math.h>
#include "sysdef.h"

si32 udev(si32 *seed) {

#ifdef HAS_I64                   /* --- 64-bit version --- */

#if LSIZE == 8
#define m 16807L                 /* The magic multiplier */
#define w31 0x7fffffffL          /* Base, 2**31 - 1 */
#else
#define m 16807LL
#define w31 0x7fffffffLL
#endif
   /* Calculate the product and remainder with 64-bit arithmetic */
   ui64 sm = (ui64)(*seed) * m;
   sm = (sm & w31) + (sm >> 31);
   sm = (sm & w31) + (sm >> 31); /* Do it again */

   return (*seed = (si32)sm);

# else                           /* --- 32-bit version --- */

#define m 16807L                 /* The magic multiplier */
   /* Form the product in low 16 and high 31 bit pieces */
   ui32 s0m = ((ui32)(*seed) & 0xffffL) * m;
   ui32 smlo = s0m & 0xffffL;
   ui32 smhi = ((ui32)(*seed)>>16)*m + (s0m>>16);
   /* Form remainder modulo (2**31-1) using Knuth trick */
   smlo += ((smhi & 0x7fffL) << 16) + (smhi >> 15);
   /* Final adjustment--if the result exceeds (2**31-1), subtract
   *  (2**31-1) one more time.  This boils down to: if the sign
   *  bit is set, clear the sign and add one.  */
   smlo = (smlo + (smlo >> 31)) & 0x7fffffffL;

   return (*seed = (si32)smlo);     /* Return result */

#endif

   } /* End udev() */

