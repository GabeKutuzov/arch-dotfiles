/* (c) Copyright 2002-2015, The Rockefeller University *11115* */
/* $Id: mfbitpk.c 7 2008-08-23 15:30:00  $ */
/***********************************************************************
*                     ROCKS metafile plot library                      *
*                              mfbitpk()                               *
*                                                                      *
*  DESCRIPTION:                                                        *
*     mfbitpk() is the low-level routine used in the ROCKS plotting    *
*     package to insert bit strings into binary metafile records.      *
*     The specification requires that the output be packed in big-     *
*     endian order regardless of the native byte order on the machine  *
*     where it is executed.  Up to the number of bits in a ui32 word   *
*     can be handled in one call.  Buffering info is in window global. *
*                                                                      *
*  SYNOPSIS:                                                           *
*     void mfbitpk(ui32 item, int ilen)                                *
*                                                                      *
*  ARGUMENTS:                                                          *
*     item    The data item from which the low-order 'ilen' bits are   *
*             to be stored.                                            *
*     ilen    Number of low-order bits from 'item' to be stored.       *
*                                                                      *
*  RETURN VALUE:                                                       *
*     None                                                             *
*                                                                      *
*  DESIGN NOTES:                                                       *
*    (1) In a serial computer, Win.MFBrem always contains one less     *
*     than the number of bits left in the buffer, so an empty byte     *
*     doesn't require a special test when BitRemainder() returns 0.    *
*     In a parallel computer, Win.MFBrem is reduced by LNop (17) so    *
*     there is always room to insert an alignment code at the end      *
*     of the buffer before writing it.                                 *
*    (2) When storing into part of a byte, the rest of the byte will   *
*     be left untouched.  Data to left of ilen bits in item are not    *
*     stored--for example, leading sign bits in a negative number.     *
*    (3) On a parallel computer, checks will already have been done    *
*     to assure that the entire command fits in the current buffer     *
*     and therefore the current item must also fit.  However, on a     *
*     serial computer, it is OK to go to a new buffer in the middle    *
*     of a command, so the check is done here.  It is also done in     *
*     the parallel case "just in case" (but the caller has to do       *
*     the mfstate() call).                                             *
*    (4) This code is a prime candidate to be written in Assembler     *
*     language, since C does not provide the 64-bit shifts between     *
*     two adjacent registers that would greatly simplify matters.      *
*    (5) This routine will never store into more than one byte within  *
*     one operation, therefore it should work equally on little-endian *
*     or big-endian machines.                                          *
*    (6) When a buffer is filled, it will be written, so there is      *
*     always at least one byte available to write into.                *
*    (7) The test routine for this routine is in test/mfpktest.c       *
************************************************************************
*  V1A, 08/03/02, GNR - New routine for major revision of plot lib     *
*  Rev, 08/23/08, GNR - Change longs to ui32                           *
*  Rev, 02/07/15, GNR - Revise never-tested 2008 version               *
***********************************************************************/

#include "mfint.h"
#include "mfio.h"

void mfbitpk(ui32 item, int ilen) {

   Win *pw = _RKG.pcw;        /* Locate info for current window */
   register ui32 itm = item;  /* Copy of item, may be register  */

   while (ilen) {
      int irb = BitRemainder(pw->MFBrem) + 1;   /* Bits in MFCP */
      int irl = ilen - irb;                     /* Bits to hold */
      ui32 mask;
      if (irl > 0) {
         /* Fill as much as fits in current byte */
         mask = ((ui32)1 << irb) - 1;
         *pw->MFCP++ = *pw->MFCP & (byte)(~mask) |
            (byte)((itm >> irl) & mask);
         ilen = irl;
         //printf("1 ~ %d : %d\n", pw->MFBrem, irb);
         if ((pw->MFBrem -= irb) < 0) mfflush();
         }
      else if (irl < 0) {
         /* Current buffer byte holds ilen with space left over */
         irl = -irl;
         mask = ((ui32)1<<irb) - ((ui32)1<<irl);
         *pw->MFCP = *pw->MFCP & (byte)(~mask) |
            (byte)((itm<<irl) & mask);
         pw->MFBrem -= ilen;
         return;
         }
      else {   /* irl == 0 */
         /* Amount to store just fits in current byte */
         mask = ((ui32)1 << irb) - 1;
         *pw->MFCP++ = *pw->MFCP & (byte)(~mask) |
            (byte)(itm & mask);
         //printf("2 ~ %d : %d\n", pw->MFBrem, ilen);
         if ((pw->MFBrem -= ilen) < 0) mfflush();
         return;
         }
      }
   return;
   } /* End mfbitpk() */
