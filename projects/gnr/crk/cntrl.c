/* (c) Copyright 1988-2008, The Rockefeller University *11115* */
/* $Id: cntrl.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                       Functions CNTRL, CNTRLN                        *
*                                                                      *
*  Functions cntrl() and cntrln() determine whether a control card     *
*  or data field contains any characters not expected to be found      *
*  in numeric data, i.e. alphabetic characters or punctuation other    *
*  than plus, minus, 'e' or 'E' followed by digits, tab, space, or     *
*  decimal point.                                                      *
*                                                                      *
*  Usage: int cntrl(char *data)                                        *
*         int cntrln(char *data, int length)                           *
*                                                                      *
*  Arguments: 'data' is the location of the character string to be     *
*     tested.  All characters up to the standard C end-of-string       *
*     character or the specified maximum length, whichever comes       *
*     first, are tested.                                               *
*        'length' is the maximum number of characters to be tested.    *
*     In cntrl(), 'length' defaults to the length of a control card.   *
*                                                                      *
*  Value returned: NUMBRET (0) if the string contains only characters  *
*     valid in numeric fields, otherwise CNTLRET (1).                  *
*                                                                      *
*  Changes from original IBM Assembler version:                        *
*     1) provide length spec--original routine always checked 8 chars  *
*     2) does not check for "packed card format"                       *
*     3) can handle E-format numeric data, accepts tabs                *
*                                                                      *
************************************************************************
*  V1A, 12/23/88, GNR - Adapted from my IBM Assembler versions         *
*  Rev, 04/19/89, GNR - Move return value defs to rocks.h              *
*  Rev, 09/04/97, GNR - Accept tabs on data cards                      *
*  Rev, 02/01/03, GNR - Add cntrln(), accept 'e' or 'E' only inside    *
*                       a number                                       *
*  ==>, 09/30/04, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include <stddef.h>
#include <ctype.h>
#include "sysdef.h"
#include "rkxtra.h"

int cntrl(char *data) {
   return cntrln(data, CDSIZE);
   } /* End cntrl() */

int cntrln(char *data, int length) {

   register char *pc = data;  /* Local copy of data arg */
   char *pce = data + length; /* Fence at end of data */
   int etrig = 0;             /* Exponent trigger */

   for ( ; *pc && pc<pce; pc++) {   /* Scan the data */
      if (isdigit(*pc))             /* Digit */
         etrig &= ~1;               /* Cancel 1 exponent trigger */
      else switch (*pc) {
         case ' ':                  /* Punct allowed in numbers? */
         case '\t':
            if (etrig) return CNTLRET;
            continue;
         case '+':
         case '-':
         case '.':
            continue;
         case 'e':
         case 'E':
            etrig += 1;
            continue;
         default:
            return CNTLRET;
         } /* End test for valid numeric characters */
      } /* End loop over all characters in data */

   return (etrig ? CNTLRET : NUMBRET);

   } /* End cntrln() */

