/* (c) Copyright 2010, The Rockefeller University *11114* */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                               nwdevs2                                *
*                                                                      *
*  This is a test function for "rapid" testing of congruential         *
*  random number generators for possible inclusion in the wdevxx       *
*  family of rocks routines.  It implements only the 64-bit            *
*  fixed-point calculations--when a suitable generator is found,       *
*  the 32-bit version will be developed and included in wdevcom.h      *
*                                                                      *
*  Note that for test purposes, "seed27" may actually be a 32-bit      *
*  value.                                                              *
*                                                                      *
*  This function is one of the 'wdev' family of pseudorandom number    *
*  generators for the ROCKS system.  It generates an array of n uni-   *
*  formly distributed pseudorandom signed integers in the range        *
*  1 <= |ir| < (2**31).  The seed is passed as a pointer to a 'wseed'  *
*  struct, defined in sysdef.h.  For use in large-scale simulations,   *
*  the period will be about 2**57 if both components of the wseed are  *
*  nonzero.                                                            *
*                                                                      *
*  Synopsis:  void nwdevs(si32 *ir, wseed *seed, long n)               *
*                                                                      *
************************************************************************
*  V1A, 04/10/10, GNR - New program                                    *
***********************************************************************/

#include <stddef.h>
#include <math.h>
#include "sysdef.h"
#include "rkwdev.h"

#ifndef HAS_I64
#error This test program requires HAS_I64 arithmetic
#endif

void nwdevs2(si32 *ir, wseed *seed, long n) {

#if LSIZE == 4
#define m31 16807ULL             /* Multiplier for seed31 */
#define w31 0x7fffffffULL        /* Base mask for seed31  */
#define m27 1223106847ULL        /* Multiplier for seed27 */
#define w27 4294967291ULL        /* Base for seed27       */
#else
#define m31 16807UL
#define w31 0x7fffffffUL
#define m27 1223106847UL
#define w27 4294967291UL
#endif

   while (--n >= 0) {

      ui64 um;
      ui32 r,rand;                  /* Remainder, result */

      /* Update the 31-bit seed */
      um = (ui64)(seed->seed31) * m31;
      um = (um & w31) + (um >> 31);    /* r + q */
      um = (um & w31) + (um >> 31);    /* Correct if (r + q > w) */
      rand = (ui32)um;
      seed->seed31 = (si32)um;
      /* Update the 32-bit seed (if not zero).  (The fast
      *  modulo trick is not available for w27 = 2**32 - 5) */
      if (seed->seed27 > 0) {
         um = ((ui64)(seed->seed27) * m27) % w27;
         r = (ui32)um;
         seed->seed27 = (si32)um;
         r = (r >> 15) | (r << 17); /* Rotate */
         rand ^= r;                 /* Make signed rand */
         } /* End if seed27 block */
      else
         rand <<= 1;

      *ir++ = (si32)rand;
      }

   } /* End nwdevs2() */
