/* (c) Copyright 1998-2013, The Rockefeller University *11115* */
/* $Id: bitpkg.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*               BITPKG - Bit Array Manipulation Package                *
*                                                                      *
*  These routines are designed to provide C support for the routines   *
*  of the same names included in old FORTRAN programs. They may also   *
*  be useful in new programs. They are the same as the IBM assembler   *
*  versions written by GNR except that all arguments are required.     *
*                                                                      *
*  Usage:                                                              *
*                                                                      *
*  void bitclr(unsigned char *array, long bit)                         *
*     Sets the 'bit'th bit (leftmost = 1) of 'array' to 0.             *
*                                                                      *
*  void bitcmp(unsigned char *array, long bit)                         *
*     Inverts the 'bit'th bit (leftmost = 1) of 'array'.               *
*                                                                      *
*  void bitset(unsigned char *array, long bit)                         *
*     Sets the 'bit'th bit (leftmost = 1) of 'array' to 1.             *
*                                                                      *
*  int bittst(unsigned char *array, long bit)                          *
*     Returns 1 if the 'bit'th bit (leftmost = 1) of 'array' is 1,     *
*     otherwise returns 0.                                             *
*                                                                      *
************************************************************************
*  V1A, 03/28/98, GNR - Broken out from original bapkg.c               *
*  ==>, 03/01/06, GNR - Last date before committing to svn repository  *
*  Rev, 01/21/13, GNR - Remove bitcnt to a separate source file        *
***********************************************************************/
#include <stdlib.h>
#include "sysdef.h"
#include "bapkg.h"            /* Make sure prototypes get checked */

/*=====================================================================*
*                               bitclr                                 *
*=====================================================================*/

void bitclr(unsigned char *array, long bit) {
   bit--;
   array[bit>>3] &= ~((unsigned char)0x80 >> (bit&7));
   }

/*=====================================================================*
*                               bitcmp                                 *
*=====================================================================*/

void bitcmp(unsigned char *array, long bit) {
   bit--;
   array[bit>>3] ^= (unsigned char)0x80 >> (bit&7);
   }

/*=====================================================================*
*                               bitset                                 *
*=====================================================================*/

void bitset(unsigned char *array, long bit) {
   bit--;
   array[bit>>3] |= (unsigned char)0x80 >> (bit&7);
   }

/*=====================================================================*
*                               bittst                                 *
*=====================================================================*/

int bittst(unsigned char *array, long bit) {
   bit--;
   return (int)((array[bit>>3] &
      ((unsigned char)0x80 >> (bit&7))) != 0);
   }

