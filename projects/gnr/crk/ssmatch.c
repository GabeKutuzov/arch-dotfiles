/* (c) Copyright 1988-2013, The Rockefeller University *11115* */
/* $Id: ssmatch.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                   SSMATCH ("Single String Match")                    *
*                                                                      *
*  Function ssmatch is used to determine whether a given string is     *
*  a proper abbreviation (an initial substring) of another string.     *
*  Function ssmatch should be used to match all card identifiers and   *
*  keywords in ROCKS programs in order that the rules for abbreviation *
*  may appear uniform to the user for all his input.  Function ssmatch *
*  is used for this purpose by smatch, match, and kwscan.              *
*                                                                      *
*  Usage:   int ssmatch(const char *item, const char *key, int mnc)    *
*                                                                      *
*  Arguments:                                                          *
*     'item' is the item to be tested.  It may be terminated by a      *
*           standard C end-of-string character, by a blank if past     *
*           the minimum columns required to match, or by a colon       *
*           (for use by smatch to identify control cards).  If the     *
*           key contains a period, indicating a qualifying prefix,     *
*           the matching item may or may not contain the period.       *
*                                                                      *
*     'key' is the identifier to which the item is to be compared.     *
*           It may be terminated by a standard C end-of-string         *
*           character or by a percent sign (for use by kwscan).        *
*           Do not include a blank as the last character.  The key     *
*           may contain a period.  If 'mnc' < 0, this indicates a      *
*           qualifying prefix.  A matching item may omit the prefix.   *
*                                                                      *
*     'mnc' is the minimum number of initial characters which must     *
*           be identical in order for a match to be accepted.  The     *
*           normal value for this argument is 1.  A value mnc < 0      *
*           indicates that a period in the key is to be treated as     *
*           a qualifying prefix, otherwise it is an ordinary match     *
*           character.  The absolute value |mnc| is the min match.     *
*                                                                      *
*  Return value:  0 if 'item' is not an initial substring of 'key',    *
*     otherwise the number of initial characters that match (i.e. the  *
*     length of 'item').  The return value may be used to determine    *
*     whether an abbreviation is unique.                               *
*                                                                      *
*  Note:  02/13/89, GNR - In ANSI C, it is implementation-dependent    *
*     whether 'char' is a signed type, but the argument to 'toupper'   *
*     is an 'int'.  Accordingly, the typecast of the argument of       *
*     'toupper' to 'unsigned' has been added.  It is not known whe-    *
*     ther this is necessary or effective on any particular system.    *
*                                                                      *
*  Note:  09/01/13, GNR - In the qualifying prefix match, note that    *
*     the body of the key after the prefix can start with the same     *
*     string as the prefix, e.g. 'A.ALPHA'.  The current code would    *
*     need to be revised if it is ever necessary to handle multiple    *
*     levels of qualification (more than one '.').                     *
*                                                                      *
************************************************************************
*  V1A, 12/25/88, GNR - Newly written for C implementation of ROCKS    *
*  V1B, 02/11/89, GNR - Make case-insensitive on key (item is upper)   *
*  Rev, 04/19/89, GNR - Prototype to rkxtra.h                          *
*  V1C, 06/22/89, GNR - Make totally case-insensitive                  *
*  Rev, 12/07/08, GNR - Reformat comments, check OK for 64-bit use     *
*  ==>, 12/07/08, GNR - Last date before committing to svn repository  *
*  V2A, 09/01/13, GNR - Add qualifying prefix match                    *
***********************************************************************/

#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "sysdef.h"
#include "rkxtra.h"

int ssmatch(const char *item, const char *key, int mnc) {

   char *bkey = mnc < 0 ? strchr(key, '.') : NULL;
   int amnc = abs(mnc);       /* Min match */
   int mc;                    /* Match count */
   int upitem;                /* Uppercase item char */

RestartMatch:
   for (mc=0; item[mc]; key++,mc++) {
      if (item[mc] == ':' || item[mc] == ' ') break;
      if (*key == '\0' || *key == '%') goto Mismatch;
      upitem = toupper((unsigned)item[mc]);
      if (upitem == toupper((unsigned)(*key))) continue;
      if (*key == '.') {
         if (key++ != bkey) abexit(110);
         if (upitem == toupper((unsigned)(*key))) continue;
         }
Mismatch:
      if (bkey) {
         key = bkey+1; bkey = NULL;
         goto RestartMatch;
         }
      return 0;
      } /* End of item -- no mismatch */
   return mc >= amnc ? mc : 0;
   } /* End ssmatch */
