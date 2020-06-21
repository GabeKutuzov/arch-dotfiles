/* (c) Copyright 2005-2008, The Rockefeller University *11115* */
/* $Id: bitpack.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                               bitpack                                *
*                                                                      *
*  DESCRIPTION:                                                        *
*     bitpack() can be used to insert an arbitrary number of bits      *
*     (up to the length of a long word) into a string beginning at     *
*     an arbitrary bit offset from the start of the string.  The       *
*     information can be retrieved with a corresponding call to        *
*     bitunpk().  The location and size of the string along with       *
*     the current packing location are kept in a BITPKDEF struct       *
*     that must be initialized by a call to setbpack() or setbunpk(),  *
*     bzw.  It is undefined to the user in what order the bit strings  *
*     are packed (in fact, it is high order first) and the user        *
*     should not access information in the BITPKDEF structure.         *
*                                                                      *
*  SYNOPSIS:                                                           *
*     void setbpack(struct BITPKDEF *pbpd,                             *
*        void *pbits, size_t npbpd, long io1)                          *
*     size_t bitpack(struct BITPKDEF *pbpd, long item, int nbits)      *
*     void setbunpk(struct BITPKDEF *pbpd,                             *
*        void *pbits, size_t npbpd, long io1)                          *
*     long bitunpk(struct BITPKDEF *pbpd, int nbits)                   *
*                                                                      *
*  PROTOTYPED IN:                                                      *
*     bapkg.h                                                          *
*                                                                      *
*  ARGUMENTS:                                                          *
*     pbpd     Ptr to a BITPKDEF structure where state information     *
*              can be stored between calls.                            *
*     pbits    Ptr to an array where bits can be/are stored.           *
*     npbpd    Number of bytes available in pbits array.               *
*     io1      Bit offset in pbits array where storage should/does     *
*              start, counting from 0 at start of array.               *
*     item     Data bits to be stored in low-order bits of long word.  *
*     nbits    Number of bits to be stored or retrieved.               *
*                                                                      *
*  RETURN VALUE:                                                       *
*     setbpack:   Nothing is returned.                                 *
*     bitpack:    Returns number of bits of item that could not        *
*                 be stored because pbpd array was full.  Zero         *
*                 indicates successful execution.                      *
*     setbunpk:   Nothing is returned.                                 *
*     bitunpk:    Returns the requested data item in low-order bits    *
*                 of a long word.                                      *
*                                                                      *
*  ABEXIT ERRORS:                                                      *
*     88       nbits argument to bitpack or bitunpk exceeded bits      *
*              available as specified in setbpack or setbunpk call.    *
*     89       io1 offset argument to setbpack or setbunpk exceeded    *
*              stated length of the available space.                   *
*                                                                      *
*  DESIGN NOTES:                                                       *
*     (1) Given that bitpack() is intended for repeated use to pack    *
*        multiple bit strings into a single longer string, it zeros    *
*        all bits to the right of the stored data in the last byte     *
*        accessed and possibly one more byte (but not past the given   *
*        length of the bit array).                                     *
*     (2) This code is a prime candidate to be written in Assembler    *
*        language, since C does not provide the 64-bit shifts between  *
*        two adjacent registers that would greatly simplify matters.   *
*                                                                      *
************************************************************************
*  V1A, 08/24/05, GNR - New routine, based on mfbitpk in plot library  *
*  ==>, 11/09/08, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include <stddef.h>
#include "sysdef.h"
#include "bapkg.h"


/*=====================================================================*
*                              setbpack                                *
*=====================================================================*/

void setbpack(struct BITPKDEF *pbpd,
      void *pbits, size_t npbpd, long io1) {

   int rem = (int)BitRemainder(io1);
   pbpd->pb0 = (byte *)pbits;
   pbpd->pbe = (byte *)pbits + npbpd;
   pbpd->pb  = (byte *)pbits + ByteOffset(io1);
   pbpd->ibr = BITSPERBYTE - rem;

   if (pbpd->pb >= pbpd->pbe)
      abexitm(89, "Bit data too long for array");

   /* Clear data to right of last bit in last byte of existing data */
   *pbpd->pb &= (byte)(0xff00 >> rem);

   } /* End setbpack() */


/*=====================================================================*
*                               bitpack                                *
*=====================================================================*/

int bitpack(struct BITPKDEF *pbpd, long item, int nbits) {

   unsigned long itm = item;     /* Copy of item, may be register  */
   unsigned long ibs;            /* Bits being shifted for storage */
   int  iln = nbits;             /* Copy of nbits, may be register */
   int  imv;                     /* Number of bits of item to move */
   int  irl;                     /* Bits to shift item right, left */
   int  jbr = pbpd->ibr;

   /* Just in case:  abexit if called again after stored on last
   *  even byte or user ignored iln != 0 return.  */
   if (pbpd->pb >= pbpd->pbe)
      abexitm(88, "Bit data too long for array");

   while (iln > 0) {
      imv = min(iln,jbr);
      ibs = itm;
      if (irl = iln - imv) {     /* Assignment intended */
         ibs >>= irl;
         itm -= ibs << irl;
         }
      iln -= imv;
      if (irl = jbr - imv) {     /* Assignment intended */
         *pbpd->pb |= ibs << irl;
         jbr -= imv;
         }
      else {
         *pbpd->pb++ |= ibs;
         if (pbpd->pb >= pbpd->pbe) break;
         *pbpd->pb = 0;          /* Prepare next byte */
         jbr = BITSPERBYTE;
         }
      }

   pbpd->ibr = jbr;
   return iln;

   } /* End bitpack() */


/*=====================================================================*
*                              setbunpk                                *
*=====================================================================*/

void setbunpk(struct BITPKDEF *pbpd,
      void *pbits, size_t npbpd, long io1) {

   pbpd->pb0 = (byte *)pbits;
   pbpd->pbe = (byte *)pbits + npbpd;
   pbpd->pb  = (byte *)pbits + ByteOffset(io1);
   pbpd->ibr = BITSPERBYTE - BitRemainder(io1);

   if (pbpd->pb >= pbpd->pbe)
      abexitm(89, "Bit data too long for array");

   } /* End setbunpk() */


/*=====================================================================*
*                               bitunpk                                *
*=====================================================================*/

long bitunpk(struct BITPKDEF *pbpd, int nbits) {

   unsigned long itm = 0;        /* Collection word for data item  */
   unsigned long ibs;            /* Bits picked up from storage    */
   int  iln = nbits;             /* Copy of nbits, may be register */
   int  imv;                     /* Number of bits of item to move */
   int  irl;                     /* Bits to shift item right, left */
   int  jbr = pbpd->ibr;

   while (iln > 0) {
      imv = min(iln,jbr);
      ibs = *pbpd->pb;
      if (irl = jbr - imv) {     /* Assignment intended */
         ibs >>= irl;
         jbr -= imv;
         }
      else {
         jbr = BITSPERBYTE;
         if (pbpd->pb++ >= pbpd->pbe)
            abexitm(88, "Bit data too long for array");
         }
      itm |= ibs << (iln - imv);
      iln -= imv;
      }

   pbpd->ibr = jbr;
   return itm & (~1UL >> (BITSPERLONG - nbits));

   } /* End bitunpk() */

