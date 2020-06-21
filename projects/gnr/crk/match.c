/* (c) Copyright 1989-2017, The Rockefeller University *11115* */
/* $Id: match.c 64 2017-09-15 17:55:59Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                               MATCH                                  *
*                                                                      *
************************************************************************
*  Rev, 04/19/89, GNR - Incorporate rkxtra.h                           *
*  Rev, 04/28/89, GNR - Recast keys declaration for NCUBE              *
*  Rev, 05/28/94, GNR - Use defined constants for errors               *
*  Rev, 01/13/07, GNR - Add RK_MDLIM iscn code                         *
*  ==>, 05/25/07, GNR - Last date before committing to svn repository  *
*  Rev, 03/19/10, GNR - Add RK_BPMCK error check code                  *
*  Rev, 03/03/17, GNR - Add RK_MPMDL iscn code (PMDELIM w/o ASTDELIM)  *
*  R63, 06/01/17, GNR - Add RK_MPMRQ iscn code (PMDELIM w/ RK_REQFLD)  *
***********************************************************************/

/*
   Function match is used to compare a field with a list of possible
keys.  It calls scan() to parse the field, checks punctuation, then
attempts to match the field to the given list of keys.  It returns
the ordinal position of the successful match in the list, or zero if
there is no unique match.  The keys may be of mixed lengths.  Matches
are tested by calling smatch, which in turn calls ssmatch.

Usage: int match(int keq, int iscn, int mask, int ipnc,
             char **keys, int nkeys)

Arguments:
    'keq' controls error checking.  It is the sum of any of the fol-
         lowing codes that are desired:  Add RK_EQCHK (value 1) or
         RK_BPMCK (value 4) to generate an RK_EQRQERR error if the
         most recently scanned field was not delimited by an equals
         sign (possibly with plus|minus).  Add RK_NMERR (2) to sup-
         press generating an error if the item is not matched.

   'iscn' controls field scanning as follows:
         = 1                 invalid--replace by smatch call
         = RK_MSCAN (2)      match executes scan(field,0)
         = RK_MNEWK (3)      match executes scan(field,RK_NEWKEY)
         = RK_MDLIM (4)      match executes scan(field,RK_NEWKEY|
                             RK_PMDELIM|RK_ASTDELIM|RK_SLDELIM)
         = RK_MREQF (5)      match executes scan(field,RK_REQFLD)
         = RK_MPMDL (6)      scan(field,RK_NEWKEY|RK_PMDELIM)
         = RK_MPMRQ (7)      scan(field,RK_REQFLD|RK_PMDELIM) 
         (where 'field' is an internal buffer containing space for up
         to DFLT_MAXLEN characters).  (The rather curious definition
         of iscn is made for compatibility with the FORTRAN version.)

   'mask' and 'ipnc' are codes used for punctuation checking.  After
         scanning, if (RK.scancode & mask) != ipnc, an error occurs.

   'keys' is an array of strings, each of which is a possible match
         for the item being tested.  Each key is terminated by a
         standard C end-of-string character or by a percent sign.

   'nkeys' is the number of keys in the 'keys' list.

Return value: the ordinal position of the item in the list, or zero
   if the string is not matched.  Negative values are returned for
   other errors, as follows:
      RK_MPERR (-1)  A punctuation test failed.
      RK_MENDC (-2)  Scan was called and no field was found
                     (the end of the input card was reached).

Error procedures:  If no match is found and (keq & RK_NMERR == 0),
   an error message is issued and RK.iexit is set nonzero.  If a
   punctuation test fails, an error message is issued, RK.iexit is
   set nonzero, and RK_MPERR is returned.  If scan is called and no
   field is found, RK_MENDC is always returned, but an error message
   is issued and RK.iexit is set nonzero only if 'iscn' was RK_MREQF.

Note:  The user must call cdscan prior to calling match.  'maxlen'
   must be no larger than DFLT_MAXLEN (normally 16).
*/

#include <stddef.h>
#include <stdlib.h>
#include "sysdef.h"
#include "rocks.h"
#include "rkxtra.h"

int match(int keq, int iscn, int mask, int ipnc,
      char **keys, int nkeys) {

   char lea[DFLT_MAXLEN+1];   /* Scan buffer */

   if (keq & (RK_EQCHK|RK_BPMCK) && !(RK.scancode & RK_EQUALS)) {
      ermark(RK_EQRQERR); return RK_MPERR; }

   switch (iscn) {
      case RK_MSCAN:
         scan(lea,0); break;
      case RK_MNEWK:
         scan(lea,RK_NEWKEY); break;
      case RK_MDLIM:
         scan(lea,RK_NEWKEY|RK_PMDELIM|RK_ASTDELIM|RK_SLDELIM); break;
      case RK_MREQF:
         scan(lea,RK_REQFLD); break;
      case RK_MPMDL:
         scan(lea,RK_NEWKEY|RK_PMDELIM); break;
      case RK_MPMRQ:
         scan(lea,RK_REQFLD|RK_PMDELIM); break;
      default:
         abexit(26);
      } /* End scan switch */

   if (RK.scancode & RK_ENDCARD) return RK_MENDC;
   if ((RK.scancode & mask) != ipnc) {
      ermark(RK_PUNCERR); return RK_MPERR; }

   return smatch((keq&RK_NMERR),lea,keys,nkeys);
   } /* End match */

