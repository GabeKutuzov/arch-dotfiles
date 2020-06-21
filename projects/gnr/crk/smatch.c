/* (c) Copyright 1988-2010, George N. Reeke, Jr. */
/* $Id: smatch.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                               SMATCH                                 *
*                                                                      *
*  Function smatch is used to compare a field with a list of possible  *
*  character string keys.  It returns the ordinal position in this     *
*  list of the longest match, or zero if there is no unique match.     *
*  Matches are tested by calling ssmatch.                              *
*                                                                      *
*  Usage: int smatch(int keq, const char *item, char **keys,           *
*     int nkeys)                                                       *
*                                                                      *
*  Arguments:                                                          *
*   'keq' controls error checking.  It is the sum of any of the fol-   *
*        lowing codes that are desired:  Add RK_EQCHK (value 1) or     *
*        RK_BPMCK (value 4) to generate an RK_EQRQERR error if the     *
*        most recently scanned field was not delimited by an equals    *
*        sign (possibly with plus|minus).  Add RK_NMERR (2) to sup-    *
*        press generating an error if the item is not matched.         *
*                                                                      *
*   'item' is a pointer to the string to be matched by smatch.  It     *
*        may be terminated by a standard C end-of-string character,    *
*        by a blank, or by a colon.                                    *
*                                                                      *
*   'keys' is a pointer to an array of strings, each of which is a     *
*        possible match for the item being tested.  Each key is        *
*        terminated by a standard C end-of-string character or by      *
*        a percent sign.  (Note:  This array is untouched, but use     *
*        of the 'const' keyword with this ptr to a ptr seems to        *
*        confuse the gcc compiler.)                                    *
*                                                                      *
*   'nkeys' is the number of keys in the 'keys' list.                  *
*                                                                      *
*  Return value: the ordinal position of the item in the list, or      *
*     zero if the string is not matched.  Negative values are          *
*     returned for other errors, as follows:                           *
*      RK_MPERR (-1)    The punctuation test failed                    *
*                                                                      *
*  Error procedures:  If no match is found and (keq & RK_NMERR == 0),  *
*     an error message is issued and RK.iexit is set nonzero.  If a    *
*     punctuation test fails, an error message is issued, RK.iexit     *
*     is set nonzero, and RK_MPERR is returned.                        *
*                                                                      *
************************************************************************
*  V1A, 12/25/88, GNR - Newly written for C implementation of ROCKS    *
*  V1B, 02/11/89, GNR - Always accept an exact match                   *
*  Rev, 04/19/89, GNR - Incorporate rkxtra.h and sysdef.h              *
*  Rev, 04/28/89, GNR - Recast key declaration for NCUBE               *
*  Rev, 05/28/94, GNR - Use defined constants for errors               *
*  ==>, 07/01/02, GNR - Last date before committing to svn repository  *
*  Rev, 09/05/09, GNR - Make char args const                           *
*  Rev, 03/19/10, GNR - Add RK_BPMCK error check code                  *
***********************************************************************/

#include <stddef.h>
#include "sysdef.h"
#include "rocks.h"
#include "rkxtra.h"

int smatch(int keq, const char *item, char **keys, int nkeys) {

   register int ik;           /* Key counter */
   int retval = 0;            /* Position of best match */
   int best_match = 0;        /* Longest ssmatch */
   int this_match;            /* Current ssmatch */
   char tie_match;            /* TRUE if tie match */

   /* If punctuation test fails, give error and return */
   if (keq & (RK_EQCHK|RK_BPMCK) && !(RK.scancode & RK_EQUALS)) {
      ermark(RK_EQRQERR); return RK_MPERR; }

   for (ik=0; ik<nkeys; ik++) {
      this_match = ssmatch(item,keys[ik],1);
      /* Return at once if there is an exact match */
      if (keys[ik][this_match] == '\0') return (ik+1);
      /* Otherwise, determine best match */
      if (this_match > best_match) {
         best_match = this_match;
         tie_match = FALSE;
         retval = ik; }
      else if (this_match == best_match) tie_match = TRUE;
      }
   if (best_match && !tie_match) return (retval+1);
   if (!(keq & RK_NMERR)) ermark(RK_IDERR);
   return 0;
   } /* End smatch */
