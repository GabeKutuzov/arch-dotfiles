/* (c) Copyright 1989-2008, The Rockefeller University *11115* */
/* $Id: jfind.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                               JFIND                                  *
*                                                                      *
*  This routine is used to find a character string on a control card   *
*  without regard to the parsing rules of the 'scan' function.  It is  *
*  similar to the C library function 'strstr' except that it stops     *
*  searching when it encounters a semicolon (comment marker) on the    *
*  input card.                                                         *
*                                                                      *
*  Usage: int jfind(char *card, char *key, int idspl)                  *
*                                                                      *
*  Arguments:                                                          *
*     'card' is the card image to be scanned.                          *
*     'key' is the literal string to be found.                         *
*     'idspl' is the number of columns displacement from the begin-    *
*        ning of the card at which the search is to begin.             *
*                                                                      *
*  Value returned:                                                     *
*     Function jfind returns the displacement of the search key if     *
*     the key is found, and zero if the key is not found.  (There      *
*     is a possible ambiguity if 'idspl' = 0, a case which should      *
*        be avoided.)                                                  *
*                                                                      *
************************************************************************
*  Rev, 04/19/89, GNR - Prototype to rkxtra.h                          *
*  ==>, 07/01/02, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include <string.h>
#include "sysdef.h"
#include "rkxtra.h"

int jfind(char *card, char *key, int idspl) {

   register int i = idspl;    /* Working displacement */
   int lkey = strlen(key);    /* Length of key */

   while ((card[i] != '\0') && (card[i] != ';')) {
      if (strncmp(card+i,key,lkey) == 0) return i;
      i++; }
   return 0;

   } /* End jfind */

