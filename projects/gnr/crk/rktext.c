/* (c) Copyright 2001-2008, The Rockefeller University *11115* */
/* $Id: rktext.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                              rktext.c                                *
*                                                                      *
*                     findtxt, getrktxt, savetxt                       *
*                                                                      *
*  This set of routines is used to save text strings, such as names    *
*  and labels, in a cache.  These items can then expand to any size    *
*  (up to CDSIZE if scantxt is used) without having fixed allocations  *
*  in control blocks.  Further benefits are that duplicate strings are *
*  stored only once and, on parallel computers, the text and pointers  *
*  to it are not copied/translated to comp nodes.                      *
*                                                                      *
*  N.B.  Routine names gettxt and gettext exist in many C libraries.   *
*                                                                      *
************************************************************************
*  V8B, 08/12/01, GNR - New family of routines                         *
*  ==>, 08/23/02, GNR - Last date before committing to svn repository  *
*  Rev, 12/07/08, GNR - Expand some ints to longs for 64-bit use       *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "rocks.h"
#include "rkxtra.h"
#include "rkhash.h"

#define NUMTHINIT    40       /* Initial txthdrs to allocate  */
#define LTXTINIT    640       /* Initial text chars to alloc  */

struct txthdr {
   union {
      char  *ptxi;            /* Ptr to instance of this text */
      unsigned long otxi;     /* Offset form for realloc time */
      } utx;
   void  *phlk;               /* Ptr to hash link             */
   };

struct {                      /* Global data for text module--
                              ** All static--initialized to 0 */
   struct txthdr *ptha;       /* Ptr to txthdr array          */
   htbl  *texthash;           /* Ptr to hash table for text   */
   char  *ptxtblk;            /* Ptr to start of text block   */
   size_t ltxtallo;           /* Length of text block alloc'd */
   size_t ltxtused;           /* Length of text block used    */
   int   nthallo;             /* Number of txthdrs allocated  */
   int   nthused;             /* Number of txthdrs used       */
   } TI;

/*=====================================================================*
*                              rktxhash                                *
*                                                                      *
*                    Hash function for text lookup                     *
*=====================================================================*/

static unsigned long rktxhash(void *ptxt) {

   char *ptx = (char *)ptxt;
   unsigned long h = 0;
   int s = 0;

   while (*ptx) {
      h ^= (unsigned long)*ptx++ << s;
      /* Max shift value is chosen to get some text bits into left
      *  half of hash while still using fast '&' rather than '%' */
      s = (s + 3) & (4*LSIZE-1);
      }

   return h;

   } /* End rktxhash() */


/***********************************************************************
*                                                                      *
*                               findtxt                                *
*                                                                      *
*  Locate a text string.                                               *
*                                                                      *
*  Syntax: int findtxt(char *ptxt)                                     *
*                                                                      *
*  Argument:                                                           *
*     ptxt        Ptr to a text string.                                *
*                                                                      *
*  Returns:                                                            *
*     An int that may be used later to retrieve the text with a call   *
*     to getrktxt(), or 0 if the text has not already been stored.     *
*                                                                      *
*  Errors:                                                             *
*     None.                                                            *
*                                                                      *
***********************************************************************/

int findtxt(char *ptxt) {

   struct txthdr *ptxh;       /* Ptr to txthdr for desired text */

   if (TI.nthused <= 0)       /* Exit if no text saved yet */
      return 0;

   ptxh = (struct txthdr *)hashlkup(TI.texthash, ptxt);
   if (!ptxh)
      return 0;
   else
      return (int)(ptxh - TI.ptha + 1);

   } /* End findtxt() */


/***********************************************************************
*                                                                      *
*                              getrktxt                                *
*                                                                      *
*  Locate a text string, given a text locator.                         *
*                                                                      *
*  Syntax: char *getrktxt(int htxt)                                    *
*                                                                      *
*  Argument:                                                           *
*     htxt     A text locator returned by an earlier savetxt() call.   *
*                                                                      *
*  Returns:                                                            *
*     A pointer to the desired text string.                            *
*                                                                      *
*  Errors:                                                             *
*     An abexit if htxt does not correspond to a stored text item.     *
*     This test could be deleted after debugging.                      *
*                                                                      *
***********************************************************************/

char *getrktxt(int htxt) {

   if (htxt <= 0 || htxt > TI.nthused)
      abexitm(93, ssprintf(NULL, "Unknown text locator %8d.", htxt));
   return TI.ptha[htxt-1].utx.ptxi;

   } /* End getrktxt() */


/***********************************************************************
*                                                                      *
*                               savetxt                                *
*                                                                      *
*  This routine saves a text string in the text cache and returns an   *
*  int text locator value that can be used to retrieve the text later. *
*                                                                      *
*  Syntax: int savetxt(char *ptxt)                                     *
*                                                                      *
*  Argument:                                                           *
*     ptxt        Ptr to text string to be stored.  Length is          *
*                 restricted only by the caller.                       *
*                                                                      *
*  Returns:                                                            *
*     Text locator.                                                    *
*                                                                      *
*  Errors:                                                             *
*     Just the unlikely abexit if all node 0 storage is exhausted.     *
*                                                                      *
*  Note:  One might think the utx union could be avoided by working    *
*  always with offsets, but the pointer form is needed by hashadd()    *
*  and hashlkup, while the offset form is needed to handle reallocs    *
*  in accord with the C standard for pointer arithmetic.               *
*                                                                      *
***********************************************************************/

int savetxt(char *ptxt) {

   int htxt = findtxt(ptxt);

   if (!htxt) {
      char *pstl;                /* Ptr to saved text location */
      size_t newltxu;            /* New value of TI.ltxtused   */
      struct txthdr *pth,*pthe;  /* Ptrs for updating txthdrs  */

/* Create the table if this is the first call */

      if (TI.nthallo <= 0) {
         TI.ptha = callocv(TI.nthallo = NUMTHINIT,
            sizeof(struct txthdr), "Text cache headers");
         TI.texthash = hashinit(rktxhash, NUMTHINIT, -1,
            (char *)&((struct txthdr *)0)->utx.ptxi - (char *)0,
            (char *)&((struct txthdr *)0)->phlk     - (char *)0);
         }

/* Make room for the text and move it to the cache.  If the
*  cache has to be expanded, update all the txthdr entries
*  by converting them to offsets during the realloc() call.  */

      newltxu = TI.ltxtused + strlen(ptxt) + 1;
      pthe = TI.ptha + TI.nthused;
      if (newltxu > TI.ltxtallo) {
         for (pth=TI.ptha; pth<pthe; ++pth)
            pth->utx.otxi = pth->utx.ptxi - TI.ptxtblk;
         TI.ptxtblk = reallocv(TI.ptxtblk,
            TI.ltxtallo = newltxu + LTXTINIT, "Text cache");
         for (pth=TI.ptha; pth<pthe; ++pth)
            pth->utx.ptxi = TI.ptxtblk + pth->utx.otxi;
         }
      pstl = TI.ptxtblk + TI.ltxtused;
      strcpy(pstl, ptxt);
      TI.ltxtused = newltxu;

/* If the txthdr table has to be expanded, update all the hash
*  table entries and links.  The only reasonable way to do this
*  is to delete all of them and then re-add them.  */

      if (TI.nthallo <= TI.nthused) {
         for (pth=TI.ptha; pth<pthe; ++pth)
            hashdel(TI.texthash, pth);
         TI.ptha = reallocv(TI.ptha, sizeof(struct txthdr) *
            (TI.nthallo += NUMTHINIT), "Text cache headers");
         pthe = TI.ptha + TI.nthused;
         for (pth=TI.ptha; pth<pthe; ++pth)
            hashadd(TI.texthash, pth);
         }

/* Add an entry to the txthdr table for the new text */

      pthe->utx.ptxi = pstl;
      hashadd(TI.texthash, pthe);
      htxt = ++TI.nthused;

      } /* End if (!htxt) */

   return htxt;

   } /* End savetxt() */
