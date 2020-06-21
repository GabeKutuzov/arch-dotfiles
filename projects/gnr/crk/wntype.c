/* (c) Copyright 2009-2013, The Rockefeller University *11115* */
/* $Id: wntype.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                     wntype, wnclen, and wntlen                       *
*                                                                      *
*  These routines are used by convrt, inform, eqscan, and kwscan.      *
*  wntype() translates a "wide-new" type letter (C,H,M,I,J,L,B, or W)  *
*     following an 'I' or 'U' conversion code to the corresponding     *
*     wbcdin/wbdwt numeric type code.                                  *
*  wnclen() returns the length of the variable associated with a       *
*     given code returned by wntype().                                 *
*  wntlen() combines the operations of wntype() and wnclen() to        *
*     return directly the length given the type letter.                *
*                                                                      *
************************************************************************
*  V1A, 08/21/09, GNR - New routines                                   *
*  Rev, 12/18/09, GNR - Correct sparc wntype bug if arg points to '\0' *
*  ==>, 12/18/09, GNR - Last date before committing to svn repository  *
*  Rev, 08/31/13, GNR - Add type codes M and B                         *
***********************************************************************/

#include <ctype.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "sysdef.h"
#include "rocks.h"
#include "rockv.h"
#include "rkxtra.h"

#define KaszMask       7         /* Size code mask */

/*=====================================================================*
*                               wntype                                 *
*                                                                      *
*  Synopsis:  ui32 wntype(char **ppfmt)                                *
*                                                                      *
*  Checks whether char at **ppfmt is one of the numeric type codes     *
*  recognized by inform, kwscan, eqscan, or convrt (C,H,M,I,J,L,B,W).  *
*  If so, returns the relevant RK_Nxxx type code and advances the      *
*  caller's format pointer.  Otherwise, returns code RK_NDFLT and      *
*  does not advance the format pointer.  It is the job of the caller   *
*  to determine whether the following character is valid--earlier      *
*  versions of this code that did that failed when the list of         *
*  valid following codes was different in different callers.           *
*                                                                      *
*  A faster version is provided that uses the low-order 3 bits of the  *
*  character (if ASCII) to return the type code.  But with 'B' added,  *
*  which has the same low-order bits (2) as 'J', the trick doesn't     *
*  quite work so neatly.                                               *
*=====================================================================*/

ui32 wntype(char **ppfmt) {

#ifndef IBM370                /* This is the normal, fast version */
#ifdef BIT64
   static const ui32 tcodes[8] = { RK_NHALF, RK_NINT, RK_NI32,
      RK_NBYTE, RK_NLONG, RK_NI32, RK_NI64, RK_NI64 };
#else  /* 32-Bit compilation */
   static const ui32 tcodes[8] = { RK_NHALF, RK_NINT, RK_NI32,
      RK_NBYTE, RK_NLONG, RK_NHALF, RK_NLONG, RK_NI64 };
#endif
   int tcode = toupper((int)*(byte *)*ppfmt);
   if (tcode != 0 && strchr("BCHIJLMW", tcode)) {
      ++(*ppfmt);
      if (tcode == 'B') tcode = 6;
      return tcodes[tcode & 7];
      }
#else                         /* Safer or non-ASCII version */
   int tcode = toupper((int)*(byte *)*ppfmt);
   if (strchr("BCHIJLMW", tcode)) {
      ++(*ppfmt);
      switch (tcode) {
#ifdef BIT64
      case 'B': return RK_NI64
#else
      case 'B': return RK_LONG;
#endif
      case 'C': return RK_NBYTE;
      case 'H': return RK_NHALF;
      case 'I': return RK_NINT;
      case 'J': return RK_NI32;
      case 'L': return RK_NLONG;
      case 'M': return RK_NHALF;
      case 'W': return RK_NI64;
         }
      }
#endif
   return RK_NDFLT;

   } /* End wntype() */


/*=====================================================================*
*                              wnclen                                  *
*                                                                      *
*  Synopsis:  int wnclen(ui32 cc)                                      *
*                                                                      *
*  Returns the length of a variable encoded by one of the wbcdin/      *
*  wbcdwt fixed-point data type codes.                                 *
*=====================================================================*/

int wnclen(ui32 cc) {

   /* Size of each input variable type as a function of size code */
   static const int nisz0[] = { sizeof(int), sizeof(byte),
      sizeof(short), sizeof(int), sizeof(ui32), sizeof(long),
      sizeof(ui64), 0 };

   return nisz0[cc >> RK_TS & KaszMask];

   } /* End wnclen() */


/*=====================================================================*
*                               wntlen                                 *
*                                                                      *
*  Synopsis:  int wntlen(char **ppfmt)                                 *
*                                                                      *
*  This convenience routine combines the functions of wntype() and     *
*  wnclen().                                                           *
*=====================================================================*/

int wntlen(char **ppfmt) {

   return wnclen(wntype(ppfmt));

   } /* End wntlen() */

