/* (c) Copyright 1997-2008, The Rockefeller University *11115* */
/* $Id: qwhite.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                           Function QWHITE                            *
*                                                                      *
*  Function QWHITE determines whether a control card contains any-     *
*  thing other than whitespace characters, defined as tabs or blanks.  *
*  It may be used by a program that does not process conventional      *
*  data cards to ignore whitespace cards (cryin must pass these as     *
*  possible data).  Note that this definition is not the same as in    *
*  the standard C library function, isspace().                         *
*                                                                      *
*  Usage: int qwhite(char *card)                                       *
*                                                                      *
*  Argument: 'card' is the location of the character string to be      *
*  tested.  All characters up to the standard C end-of-string          *
*  character are tested.                                               *
*                                                                      *
*  Value returned: qwhite returns TRUE (1) if the string is empty      *
*  or contains only whitespace characters, otherwise FALSE (0).        *
*                                                                      *
************************************************************************
*  V1A, 08/15/97, GNR                                                  *
*  Rev, 12/07/08, GNR - Reformat comments, check OK for 64-bit use     *
*  ==>, 12/07/08, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include <stddef.h>
#include <ctype.h>
#include "sysdef.h"
#include "rkxtra.h"

int qwhite(char *card) {

   register char *pc = card;  /* Local copy of card arg */

   for ( ; *pc; pc++) {       /* Scan the card */
      if (*pc != ' ' && *pc != '\t') return 0;
      } /* End scan of card or string */
   return 1;

   } /* End qwhite() */
