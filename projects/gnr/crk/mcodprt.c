/* (c) Copyright 2003-2009, The Rockefeller University *11116* */
/* $Id: mcodprt.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                               MCODPRT                                *
*                         Print option codes                           *
*                                                                      *
*     Routine mcodprt, which is part of the ROCKS library, is used to  *
*  prepare an output string that can be used to list single-letter or  *
*  digit codes corresponding to binary options that have been encoded  *
*  as bits in a ui32 word, possibly by a call to mcodes().             *
*                                                                      *
*  Usage: char *mcodprt(ui32 item, char *keys, int olen)               *
*                                                                      *
*  Arguments:                                                          *
*     'item' is a word containing the binary flags to be reported.     *
*        (Items shorter than a full word can be typecast to ui32.)     *
*     'keys' is a string that specifies the codes to be reported.      *
*        Only upper-case letters and digits are meaningful flag codes. *
*        Each character in 'keys' that corresponds, scanning from      *
*        right-to-left, with a 1 bit in the 'item' is copied to        *
*        the output.  Minuses may be used to indicate bit positions    *
*        that should not be reported.  The number of bit positions     *
*        reported is the lesser of olen, the length of 'keys', or      *
*        the number of bits in an unsigned long.                       *
*     'olen' is the maximum length of the output.  If coded as a       *
*        positive integer, the output is padded with blanks if         *
*        necessary to the exact length specified.  If coded as a       *
*        negative integer, the output will be whatever length it       *
*        takes to report the item, up to a maximum of olen.  If        *
*        abs(olen) is greater than the number of bits in a ui32        *
*        word, it is quietly replaced with that number.  If olen       *
*        is too short to contain all the output, the excess is         *
*        simply dropped.                                               *
*                                                                      *
*  Return value:                                                       *
*     Character string containing the codes from 'keys' that corres-   *
*        pond to 1 bits in 'item'.  This string is contained in a      *
*        static array and remains valid until the next call to         *
*        mcodprt().  The string is terminated with a NULL character    *
*        that is not counted in the maximum length 'olen'.             *
*     RK.length is set to one less than the number of data characters  *
*        (i.e. before the padding) in the output string.               *
*                                                                      *
************************************************************************
*  V1A, 09/20/03, GNR - New routine                                    *
*  ==>, 03/09/06, GNR - Last date before committing to svn repository  *
*  Rev, 08/11/09, GNR - Change item type from long to ui32 for 64-bit  *
***********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "sysdef.h"
#include "rocks.h"
#include "rkxtra.h"

static char mcodbuf[BITSPERUI32+1];

/*=====================================================================*
*                               mcodprt                                *
*=====================================================================*/

char *mcodprt(ui32 item, char *keys, int olen) {

   char *pky,*pkyr;              /* Ptrs to codes in keys */
   char *pmcb,*pmce;             /* Ptrs to output buffer */
   char *poc,*poce;              /* Ptrs to ocodes */
   int klen = strlen(keys);      /* Length of keys */
   int mlen = abs(olen);         /* Max message length */

   /* Establish left-right order of possible output codes */
   static char ocodes[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

   if (klen > BITSPERUI32) abexit(25);
   if (mlen > BITSPERUI32) mlen = BITSPERUI32;

   pkyr = keys + klen - 1;       /* Ptr to rightmost code in keys */
   pmcb = mcodbuf;               /* Ptr to current output */
   pmce = mcodbuf + mlen;        /* Ptr to end of mcodbuf */
   poce = ocodes+sizeof(ocodes); /* Ptr to end of ocodes */

   for (poc=ocodes; poc<poce && pmcb<pmce; ++poc) {
      if (pky = strchr(keys, *poc)) {     /* Assignment intended */
         if (item & (ui32)1 << (pkyr - pky))
            *pmcb++ = *poc;
         }
      }

   RK.length = pmcb - mcodbuf - 1;

   if (olen > 0) while (pmcb < pmce) *pmcb++ = ' ';
   *pmcb = '\0';                 /* Terminate the output */

   return mcodbuf;

   } /* End mcodprt() */
