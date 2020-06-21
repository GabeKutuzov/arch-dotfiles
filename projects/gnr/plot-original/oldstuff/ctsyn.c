/* (c) Copyright 1998, Neurosciences Research Foundation, Inc. */
/***********************************************************************
*                               ctsyn()                                *
*                                                                      *
*  SYNOPSIS:                                                           *
*     unsigned int ctsyn(char *expcol)                                 *
*                                                                      *
*  DESCRIPTION:                                                        *
*        This function assigns an integer value to an explicit color   *
*     specifier.  This value may be used in subsequent calls to        *
*     ctscol() to change the drawing color to the ctsyn() argument.    *
*     This method changes calls to pencol() or penset() into calls     *
*     to ctscol() for more efficient performance in loops.             *
*        The argument may be interpreted as an English color name or   *
*     as a hexadecimal BGR code as documented in the writeup.  The     *
*     argument is passed intact to the drawing program, where it       *
*     is interpreted in accord with the number of colors supported     *
*     by the actual output device.  Note that XWindows permits use     *
*     of only 128 of the possible 256 colors.                          *
*                                                                      *
*  NOTES:                                                              *
*        In a parallel computer, the same color synonyms must be used  *
*     on all processors. Either all ctsyn() calls should be made from  *
*     the host and the results broadcast to the comp nodes, or exactly *
*     the same sequence of ctsyn() calls should be made on all nodes.  *
*     In order to be sure the same index is always used for the same   *
*     color, a hash table of expcol strings is maintained by ctsyn().  *
*     To avoid making a copy of this table on all nodes, the broadcast *
*     method should be used.                                           *
*        Because there is no expectation that color synonyms will need *
*     to be destroyed once they are defined, there is no provision to  *
*     keep track of all the expche structures that are allocated such  *
*     that they can be deallocated as a group.                         *
*                                                                      *
*  RETURN VALUE:                                                       *
*     Color synonym for the explicit color argument, suitable for use  *
*     in subsequent ctscol() calls.  The user should not assume that   *
*     these are consecutive integers or that they are small integers.  *
*                                                                      *
*  V2A, 12/29/98, GNR - New routine, make binary metafile              *
***********************************************************************/

#define HTBLTYPE struct htbl
#include "rkhash.h"           /* Order of includes is significant */
#include "mfint.h"
#include "rksubs.h"
#include "plots.h"

#define CTSI   1              /* gks offset of icts */
#define CTSN   0              /* gks offset of name length */

struct expche {               /* Explicit color hash table entry */
   struct expche *pehl;       /* Pointer to linked entries, if any */
   unsigned int iecs;         /* Synonym assigned to this color */
   char ecnm[MAXLPCOL];       /* Explicit color name stored here */
   };

/*---------------------------------------------------------------------*
*              echash() -- Hash function for color names               *
*---------------------------------------------------------------------*/

unsigned long echash(void *ecnm) {
   unsigned long h = 0;
   int i,s;
   for (i=0,s=0; i<MAXLPCOL && ((byte *)ecnm)[i]; i++,s+=3)
      h ^= ((byte *)ecnm)[i] << s;
   return h;
   } /* End echash() */

/*---------------------------------------------------------------------*
*                               ctsyn()                                *
*---------------------------------------------------------------------*/

unsigned int ctsyn(char *expcol) {

   struct expche *pehe;       /* Ptr to hash entry */
   unsigned int icts = 0;     /* Synonym for this color */

   static char MCctsyn[] = { OpCtsyn, Tk, CTSI, Tk, CTSN,
      Tchr, CTSN, Tend };

   if (_RKG.s.MFmode) {
      /* If the hash table isn't there yet, make it now */
      if (!_RKG.pctshash)
         _RKG.pctshash = hashinit(echash, INITCSYM, MAXLPCOL,
            (char *)&((struct expche *)0)->ecnm - (char *)0,
            (char *)&((struct expche *)0)->pehl - (char *)0);
      /* If the item is already stored, return the existing synonym,
      *  otherwise, create a new synonym and hash the new color.  */
      pehe = (struct expche *)hashlkup(_RKG.pctshash, expcol);
      if (!pehe) {
         pehe = (struct expche *)mallocv(
            sizeof(struct expche), "Color Name");
         pehe->pehl = NULL;
         pehe->iecs = ++_RKG.nxtcsyn;
         strncpy(pehe->ecnm, expcol, MAXLPCOL);
         hashadd(_RKG.pctshash, pehe);
         } /* End making new hash entry */
      _RKG.gks[CTSI] = icts = pehe->iecs;
      _RKG.gks[CTSN] = strnlen(expcol, MAXLPCOL);
      _RKG.psymbol = expcol;
      CMDPACK(MCctsyn, (Lop+Li11+Li11)+_RKG.gks[CTSN]);
      } /* End if MFmode */

   return icts;
$$$ CLEAR InReset BIT

   } /* End ctsyn() */
