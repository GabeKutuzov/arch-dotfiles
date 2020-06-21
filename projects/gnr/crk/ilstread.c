/* (c) Copyright 2000-2013, The Rockefeller University *11115* */
/* $Id: ilstread.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                             ilstread.c                               *
*                                                                      *
*                    Interpret an interation list                      *
*                                                                      *
*  The syntax of the control cards interpreted by this routine is:     *
*     ([{OFF|NEW|ADD|DEL}],[MX=mxc],i,ib-ie,ib-ie+ii,ib+ii-ie,...,     *
*     [FIRST if],[LAST il],[RANDOM ir],[[OF] EVERY nev],[SEED=s],...   *
*                                                                      *
*  After the card has been read and identified and scanned up to the   *
*  left paren that starts the list, call                               *
*                                                                      *
*  ilst *ilstread(ilst *poldilst, int idop, int ibase, si32 seed)      *
*                                                                      *
*  Where:                                                              *
*     poldlist is a pointer to an existing iteration list that may     *
*              be modified by addition or deletion of elements.        *
*     idop     sets the default action in case none of the keywords    *
*              (NEW, ADD, DEL, OFF) is found initially in the parens.  *
*              It is one of the values:                                *
*              IL_NEW   1  Default is NEW.                             *
*              IL_ADD   2  Default is ADD.                             *
*              IL_DEL   3  Default is DEL.                             *
*              IL_OFF   4  Default is OFF.                             *
*              Add the following to ignore the indicated errors:       *
*              IL_ELOK 16  Empty list is OK (always OK if OFF mode).   *
*              IL_ESOK 32  End-of-scan is OK (always OK if OFF mode).  *
*     ibase    is 0 or 1 according to whether the base for counting    *
*              indexes in an EVERY block is 0 or 1.                    *
*     seed     is a default random number generating seed to be used   *
*              with the RANDOM option if the user does not specify a   *
*              seed.                                                   *
*  N.B.  Before the first call to ilstset() for this list, ilstchk()   *
*     must be called.  See below.                                      *
*                                                                      *
*  The list is interpreted according to the following rules:           *
*  The type keywords (and MX), if entered, must come before any of     *
*  the other options.  If the type keyword or default type is:         *
*     OFF      Any existing list is removed (its memory is freed).     *
*              If any list items precede or follow this keyword, an    *
*              error occurs.  (If OFF is the default, there may be no  *
*              items at all, but if there are, they are scanned to the *
*              end but ignored, then the old list is deleted.)  A NULL *
*              pointer is returned and RK.highrc, if 0, is set to 1.   *
*              For compatibility with certain old input files, NONE    *
*              is accepted as a synonym for OFF.                       *
*     NEW      A new list is created.  If poldlist is a non-NULL       *
*              pointer, the new ilst is placed in the same storage     *
*              as the old ilst.  If any list items precede this        *
*              keyword, an error occurs.                               *
*     ADD      Specified items are added to an existing list.  If      *
*              necessary, the pidl list is expanded by calling         *
*              realloc(), but the new information goes in the old      *
*              ilst.  If poldlist is a NULL pointer, a new list is     *
*              created as if the keyword was NEW.                      *
*     DEL      Specified items are deleted from an existing list.  If  *
*              no items are specified, the entire list is released     *
*              and a NULL pointer is returned.  Otherwise, no memory   *
*              is released--the space occupied by the deleted items    *
*              is available for later additions.  If a specified item  *
*              does not exist, a warning is issued and that item is    *
*              ignored.  A range with nonintegral step size cannot     *
*              be specified in a DEL list.  To delete part or all of   *
*              a list with noninteger step, just indicate the start    *
*              and end of the range to be deleted.                     *
*     MX (or any word beginning with MX) indicates the maximum number  *
*              of items for which memory is to be allocated in the     *
*              list.  This option is no longer required, as memory     *
*              is allocated by expanding the list by 25% each time     *
*              the existing space overflows.  It is accepted for       *
*              compatibility with old input files, and may still be    *
*              used if the realloc() mechanism produces excessive      *
*              memory fragmentation.  The list will still be expanded  *
*              if the requested size overflows.  If any list items     *
*              precede this keyword, an error occurs.                  *
*                                                                      *
*     FIRST if indicates that the first 'if' items are to be selected. *
*              This counts from (ibase) to (ibase + if - 1).           *
*                                                                      *
*     LAST il  indicates that the last 'il' items are to be selected.  *
*                                                                      *
*     RANDOM ir indicates that 'ir' items should be selected at random.*
*              The same selections will be used each time the list is  *
*              processed.                                              *
*                                                                      *
*     EVERY nev  indicates that the iteration list is to be processed  *
*              as if each value or range was repeated at all multiples *
*              of 'nev'.  For example, '(NEW, 2, 5, EVERY 10)' will    *
*              pick up items 2, 5, 12, 15, 22, 25, etc.  When this     *
*              option is used, it is an error for any value in the     *
*              list to be greater than or equal to (nev + ibase).      *
*              When ADD or DEL is used to modify an existing list      *
*              and a new EVERY option is not entered, the existing     *
*              EVERY option remains unchanged.                         *
*                                                                      *
*     SEED=s   provides an optional random number generating seed for  *
*              the RANDOM option.                                      *
*                                                                      *
*     i        Each unsigned integer in the list indicates a single    *
*              value.  The values do not need to be sorted--this is    *
*              taken care of by ilstread().                            *
*                                                                      *
*     ib-ie    Indicates a range of values from 'ib' through 'ie'.     *
*              The range stepping increment is 1.                      *
*                                                                      *
*     ib-ie+ii Indicates a range of values from 'ib' through 'ie'      *
*        OR    with a step size of 'ii'.                               *
*     ib+ii-ie                                                         *
*                                                                      *
*  If the list consists of just a single number or range, the          *
*     parentheses can be omitted.                                      *
*                                                                      *
*  Errors:                                                             *
*     A message is printed via cryout() or ermark() and RK.iexit is    *
*     set non-zero if an error is detected.  Some code in CNS assumes  *
*     that ALL ilstread() errors result in a call to ermark().         *
*                                                                      *
*  Program Notes:                                                      *
*     The routines in ilstiter.c assume that ilstread() guarantees     *
*     the following conditions:  (1) the list is sorted, (2) ranges    *
*     do not overlap, (3) if there is a range increment, it is stored  *
*     before the range end, and (4) if there is an EVERY parameter,    *
*     all values and ranges are contained in the first batch.          *
*                                                                      *
*     An iterlist is never created or checked unless a list item is    *
*     found to be added or deleted.  No input option can be in error   *
*     solely because of what the default processing mode might be.     *
*                                                                      *
*     Because the first two bits of each list item may be used for     *
*     flags, the largest item that can be accommodated in a list is    *
*     2**(BITSPERLONG-2) - 1.  Be careful never to subtract anything   *
*     from an ilstitem (unsigned) if the answer might go negative.     *
*                                                                      *
************************************************************************
*  V1A, 04/08/00, GNR - New routine                                    *
*  V1B, 12/23/00, GNR - Add FIRST, LAST, RANDOM, and SEED options,     *
*                       ilstsalf and ilstchk routines                  *
*  V1C, 02/19/01, GNR - Permit no list at all when default is OFF      *
*  V1D, 04/13/02, GNR - Add setting of RK.highrc if a list is deleted  *
*  Rev, 08/31/03, GNR - Modify makeilst to write over any old list     *
*  ==>, 09/30/04, GNR - Last date before committing to svn repository  *
*  Rev, 07/25/09, GNR - Revise for 64-bit longs (use wbcdin)           *
*  Rev, 10/24/13, GNR - SRA() for signed '>>'                          *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "rocks.h"
#include "rkxtra.h"
#include "rkarith.h"
#include "rkilst.h"

/* Global variables */
static struct {
   ilst *poldl;         /* Ptr to old list */
   long mxsz;           /* Maximum list size--input with 'IL' code,
                        *  so don't declare as a size_t here */
   si32 seed0;          /* Initial random number seed */
   int base0;           /* Stored ibase option */
   int iop;             /* Current operation */
   int kchg;            /* Used to trap ineffective changes */
   int knol;            /* TRUE if no-list msg issued */
   } ILR;
static void *(*ILallocv)(size_t n, size_t s, char *m) = callocv;
static void *(*ILallorv)(void *p, size_t s, char *m) = reallocv;
static void  (*ILfreev) (void *p, char *m) = freev;

/*=====================================================================*
*                              ilstsalf                                *
*  To use alternative memory allocation routines (for example, for     *
*     shared memory management on a parallel computer, call            *
*                                                                      *
*  void ilstsalf(                                                      *
*     void * (*ilallocv)(size_t nitm, size_t size, char *msg),         *
*     void * (*ilallorv)(void *block, size_t size, char *msg),         *
*     void   (*ilfreev) (void *block, char *msg));                     *
*                                                                      *
*     where 'ilallocv', 'ilallorv', and 'ilfreev' are the names of     *
*     the alternative verbose memory allocation routines to be used.   *
*     These routines need to have the same arguments as the standard   *
*     routines prototyped in rocks.h.  This call must come before the  *
*     call to ilstread() that uses the new routines.                   *
*=====================================================================*/

void ilstsalf(
      void * (*ilallocv)(size_t nitm, size_t size, char *msg),
      void * (*ilallorv)(void *block, size_t size, char *msg),
      void   (*ilfreev) (void *block, char *msg)) {

   ILallocv = ilallocv;
   ILallorv = ilallorv;
   ILfreev  = ilfreev;

   } /* End ilstsalf() */

/*=====================================================================*
*                              freeilst                                *
*               Function to remove an ilst from memory                 *
*=====================================================================*/

void freeilst(ilst *pil) {

   if (pil) {
      if (pil->pidl) ILfreev(pil->pidl, "Iteration data");
      ILfreev(pil, "Iteration list");
      }

   } /* End freeilst() */

/*=====================================================================*
*                               ilstreq                                *
*            Function to print message about missing list              *
*=====================================================================*/

void ilstreq(void) {

   ermark(RK_MARKDLM);
   cryout(RK_P1, " ***An iteration list is required here.",
      RK_LN1, NULL);

   } /* End ilstreq() */

/*=====================================================================*
*                              movedown                                *
*        Local function to delete items from an iteration list         *
*                                                                      *
*  N.B.  Argument "loc" is offset of destination of first item moved.  *
*=====================================================================*/

static void movedown(ilst *pil, long loc, int ndel) {

   ilstitem *p1, *p2, *pe;

   p1 = pil->pidl + loc;
   p2 = p1 + ndel;
   pe = pil->pidl + pil->nusm1;

   /* Error:  need enough list to delete from */
   if (!pil->pidl || p1 > pe)
      abexit(82);

   while (p2 <= pe) *p1++ = *p2++;
   pil->nusm1 -= ndel;
   ILR.kchg = TRUE;

   } /* End movedown() */

/*=====================================================================*
*                               moveup                                 *
*   Local function to make room for new items on an iteration list     *
*                                                                      *
*  N.B.  Argument "loc" is offset of first item to be moved.           *
*=====================================================================*/

static ilstitem *moveup(ilst *pil, long loc, int nadd) {

   ilstitem *p0, *p1, *p2;

   /* Error: need a list to add to */
   if (!pil->pidl)
      abexit(83);

   /* Expand the list if necessary */
   pil->nusm1 += nadd;
   if (pil->nusm1 >= pil->nallo) {
      pil->nallo = (size_t)(pil->nusm1 + SRA(pil->nusm1,2) + 2);
      pil->pidl = ILallorv(pil->pidl, sizeof(ilstitem)*pil->nallo,
         "Iteration list");
      }

   p0 = pil->pidl + loc;
   p2 = pil->pidl + pil->nusm1;
   p1 = p2 - nadd;
   while (p1 >= p0) *p2-- = *p1--;
   ILR.kchg = TRUE;
   return pil->pidl;

   } /* End moveup() */

/*=====================================================================*
*                               split3                                 *
*  Local function to split a non-unit stride range into two subranges. *
*  Returns index of location above new bottom section, where material  *
*  could be inserted into the hole just made.  Note that pidl might    *
*  be changed when moveup is called--the new value is not returned.    *
*                                                                      *
*  Arguments:                                                          *
*     jj   = index of next location above range to be modified         *
*     jbot = bottom of hole to be opened                               *
*     jtop = top of hole to be opened                                  *
*=====================================================================*/

static long split3(ilst *pil, long jj, ilstitem jbot, ilstitem jtop) {

   ilstitem inxt, *p = pil->pidl;
   ilstitem ebot = p[jj-3];
   ilstitem einc = p[jj-2] & IL_MASK;
   ilstitem etop = p[jj-1] & IL_MASK;

   /* Make new top section */
   if (etop > jtop) {
      inxt = jtop + einc - (jtop - ebot) % einc;
      if (inxt == etop)
         p = moveup(pil, jj, 1);
      else {
         p = moveup(pil, jj, 3);
         p[jj+1] = p[jj-2];
         p[jj+2] = p[jj-1];
         }
      p[jj] = inxt;
      } /* End generating new upper subrange */

   /* Make new bottom section */
   if (jbot <= ebot) {
      jj -= 3;
      movedown(pil, jj, 3);
      }
   else {
      inxt = jbot - 1;
      inxt -= (inxt - ebot) % einc;
      if (inxt == ebot) {
         jj -= 2;
         movedown(pil, jj, 2);
         }
      else
         p[jj-1] = inxt | IL_REIN;
      } /* End generating new lower subrange */

   return jj;
   } /* End split3() */

/*=====================================================================*
*                              procmini                                *
*    Local function to process a minilist for addition or deletion     *
*=====================================================================*/

static void procmini(ilst *pil,
      ilstitem ibot, ilstitem iinc, ilstitem itop) {

/* Local declarations: */

   ilstitem *p;               /* Ptr to bottom of list */
   ilstitem ebot,einc,etop;   /* Existing list range def */
   ilstitem inxt;             /* Start of next entry */
   ilstitem jbot,jtop;        /* Piece of input item being added */
   ilstitem tag;              /* Tag bits from range end */
   long ji,jj;                /* Location of item in list, temp */
   int inphase;               /* TRUE if two ranges are in phase */

   static long ki[3] = { 1,3,2 };   /* Range size by terminal code */

   /* Set trap for ineffective changes */
   ILR.kchg = FALSE;

   if (ILR.iop == IL_DEL) {

/*---------------------------------------------------------------------*
*                                                                      *
*     Handle deletions                                                 *
*                                                                      *
*     This code does not check for creation of degenerate ranges--that *
*     is done more compactly at consolidate time.  It both assumes and *
*     assures that the top of all ranges is a multiple of the stride   *
*     above the bottom.                                                *
*                                                                      *
*---------------------------------------------------------------------*/

/* If there is no list to delete from, print a warning
*  (first time only) and continue with next list item.  */

      if (!pil) {
         if (!ILR.knol) {
            cryout(RK_P3, " -->WARNING: Deletion(s) found, but there "
               "is no old list to delete from.", RK_LN1, NULL);
            ILR.knol = TRUE;
            }
         return;
         }

      p = pil->pidl;
      ji = ilstsrch(pil, ibot);

/* Work through the existing list until the end is reached
*  or an item is found that is above the range to be deleted.  */

      if (ji < 0) ji = 0;
      while (ji <= pil->nusm1 && (ebot = p[ji]) <= itop) {

/* Case D1:  A single list item is in the range.  Delete it.  */

         if (ji >= pil->nusm1 || (p[ji+1] & IL_REIN) == 0) {

            if (ebot >= ibot)
               movedown(pil, ji, 1);
            else
               ji += 1;

            } /* End match to single list entry */

/* Case D2:  Closest list item is a range with unit stride */

         else if ((p[ji+1] & IL_INCR) == 0) {
            etop = p[ji+1] & IL_MASK;
            if (ebot >= ibot) {

/* Case D2a:  The current range falls entirely within the deletion
*  range.  Delete the entire range.  */

               if (etop <= itop)
                  movedown(pil, ji, 2);

/* Case D2b:  The current range starts within the deletion range
*  but ends above it.  Modify the current range to begin above
*  the deletion range.  Exit from range scan loop.  */

               else {
                  ILR.kchg = TRUE;
                  p[ji] = itop + 1;
                  break;
                  }

               } /* End case in which current range
                  *  starts above deletion range.  */

            else {

/* Case D2c:  The current range starts below the deletion range
*  but ends within it.  Modify the current range to end below
*  the deletion range.  Continue scanning.  */

               if (etop <= itop) {
                  ILR.kchg = TRUE;
                  p[ji+1] = (ibot - 1) | IL_REND;
                  ji += 2;
                  }

/* Case D2d:  The current range starts below the deletion range
*  and ends above it.  Divide the range into two pieces, above
*  and below the omitted range.  Exit from the range scan loop.  */

               else {
                  jj = ji + 2;
                  p = moveup(pil, jj, 2);
                  p[ji+1] = (ibot - 1) | IL_REND;
                  p[ jj ] =  itop + 1;
                  p[jj+1] =  etop | IL_REND;
                  break;
                  }

               } /* End case in which current range
                  *  starts below deletion range.  */

            } /* End deletion involving range with unit stride */

/* Case D3:  Closest list item is a range with nonunit stride */

         else {
            einc = p[ji+1] & IL_MASK;
            etop = p[ji+2] & IL_MASK;

            if (ebot >= ibot) {

/* Case D3a:  The current range falls entirely within the deletion
*  range.  Delete the entire range.  */

               if (etop <= itop)
                  movedown(pil, ji, 3);

/* Case D3b:  The current range starts within the deletion range
*  but ends above it.  Modify the current range to begin above
*  the deletion range.  Exit from range scan loop.  */

               else {
                  ILR.kchg = TRUE;
                  p[ji] = itop + einc - (itop - ebot) % einc;
                  break;
                  }

               } /* End case in which current range
                  *  starts above deletion range.  */

            else {

/* Case D3c:  The current range starts below the deletion range
*  but ends within it.  Modify the current range to end below
*  the deletion range.  Continue scanning.  */

               if (etop <= itop) {
                  ILR.kchg = TRUE;
                  inxt = ibot - 1;
                  p[ji+2] = (inxt - (inxt - ebot) % einc) | IL_REIN;
                  ji += 3;
                  }

/* Case D3d:  The current range starts below the deletion range
*  and ends above it.  Divide the range into two pieces, above
*  and below the omitted range.  Exit from the range scan loop.  */

               else {
                  split3(pil, ji+3, ibot, itop);
                  p = pil->pidl;
                  break;
                  }

               } /* End case in which current range
                  *  starts below deletion range.  */

            } /* End deletion involving range with nonunit stride */

         } /* End loop checking stuff in a range to be deleted */

/* Warning if the deletion item was not found on the list */

      if (!ILR.kchg) {
         ermark(RK_WARNING+RK_MARKFLD);
         convrt("(P3,22H -->WARNING: Deletion J1IL8,"
            "29Hdoes not match any list item.)", &ibot, NULL);
         }

      } /* End processing deletions */

   else {

/*---------------------------------------------------------------------*
*                                                                      *
*     Handle additions                                                 *
*                                                                      *
*     This code both assumes and assures that the top of all ranges    *
*     is a multiple of the stride above the bottom.                    *
*                                                                      *
*---------------------------------------------------------------------*/

      p = pil->pidl;

/* Add the new range, possibly in fragments */

      while (itop >= ibot) {
         ji = ilstsrch(pil, ibot);

/* Determine start of next higher range in existing list,
*  which bounds largest insertion we can make in this pass.  */

         if (ji < pil->nusm1)
            /* OK if ji == -1, sets jj to 0 */
            jj = ji + ki[p[ji+1] >> IL_BITS];
         else
            jj = ji + 1;
         /* If at end of list, inxt is largest possible value */
         inxt = (jj > pil->nusm1) ? IL_MASK : p[jj]-1;
         jbot = ibot;
         jtop = min(inxt, itop);
         jtop -= (jtop - jbot) % iinc;
         ibot = jtop + iinc;

/* Case A1:  Nothing below or entry below is too far away--
*  just add the new entry at the present location and
*  continue to check any remaining piece from above.
*
*  Note:  The way etop is set involves a redundant test,
*  but more compact code did not optimize correctly.   */

         etop = (jj > 0) ? p[jj-1] & IL_MASK : 0;
         if (jj <= 0 || (etop < jbot && etop + iinc != jbot)) {

            if (jtop == jbot)
               p = moveup(pil, jj, 1);
            else if (iinc == 1) {
               p = moveup(pil, jj, 2);
               p[jj+1] = jtop | IL_REND;
               }
            else {
               p = moveup(pil, jj, 3);
               p[jj+1] = iinc | IL_INCR;
               p[jj+2] = jtop | IL_REIN;
               }
            p[jj] = jbot;

            } /* End addition with nothing below */

/* Case A2:  Entry below is a single item.  Tests already
*  performed assure it is coincident with or just below the
*  new item.  If new item is single and coincident with old,
*  do nothing.  Otherwise, construct a range ending at the
*  new range end.  */

         else if ((tag = p[jj-1] & IL_REIN) == 0) {

            if (etop == jbot && jtop == jbot)
               ;
            else if (iinc == 1) {
               p = moveup(pil, jj, 1);
               p[ jj ] = jtop | IL_REND;
               }
            else {
               p = moveup(pil, jj, 2);
               p[ jj ] = iinc | IL_INCR;
               p[jj+1] = jtop | IL_REIN;
               }

            } /* End addition making list with entry below */

/* Case A3:  New range has unit stride */

         else if (iinc == 1) {

/* Case A3a:  Entry below is also a range with unit stride.
*  Merge the two ranges.  */

            if (tag == IL_REND) {
               if (jtop > etop) {
                  p[jj-1] = jtop | IL_REND;
                  ILR.kchg = TRUE;
                  }
               }

/* Case A3b:  Entry below is a range with nonunit stride.
*  It may need to be broken into two pieces.  We need to
*  eliminate degenerate ranges now, rather than waiting
*  for consolidation, to avoid infinite loop in A5.  */

            else {
               jj = split3(pil, jj, jbot, jtop);
               if (jbot == jtop)
                  p = moveup(pil, jj, 1);
               else {
                  p = moveup(pil, jj, 2);
                  p[jj+1] = jtop | IL_REND;
                  }
               p[jj] = jbot;
               }

            } /* End new range with unit stride */

/* Case A4:  New range has nonunit stride and old has unit stride */

         else if ((p[jj-1] & IL_REIN) == IL_REND) {

/* Case A4a:  New range extends above top of old.
*  Insert the new range with new starting value.  */

            if (jtop > etop) {
               inxt = etop + iinc - (etop - jbot) % iinc;
               if (inxt == jtop)
                  p = moveup(pil, jj, 1);
               else {
                  p = moveup(pil, jj, 3);
                  p[jj+1] = iinc | IL_INCR;
                  p[jj+2] = jtop | IL_REIN;
                  }
               p[jj] = inxt;
               }

/* Case A4b:  New range is fully contained within
*  old range.  Swallow it--no code needed here.  */

            } /* End new range w/nonunit, old range w/unit stride */

/* Case A5:  New range and old range both have nonunit stride.  */

         else {
            einc = p[jj-2] & IL_MASK;
            inphase = (jbot - p[jj-3]) % einc == 0;

/* Case A5a:  Strides are the same and ranges are in phase:
*  just modify the top to be the larger of the two.  */

            if (inphase && einc == iinc) {
               if (jtop > etop) {
                  p[jj-1] = jtop | IL_REIN;
                  ILR.kchg = TRUE;
                  }
               }

/* Case A5b:  Old stride is a multiple of new stride
*  and ranges are in phase:  absorb overlapping portion
*  of old range into new range.  */

            else if (inphase && einc % iinc == 0) {

               jj = split3(pil, jj, jbot, jtop);
               if (jbot == jtop)
                  p = moveup(pil, jj, 1);
               else {
                  p = moveup(pil, jj, 3);
                  p[jj+1] = iinc | IL_INCR;
                  p[jj+2] = jtop | IL_REIN;
                  }
               p[jj] = jbot;
               }

/* Case A5c:  Old and new ranges are incompatible.  Split the
*  existing range so it ends just below the start of the new
*  range, then go back and process the new range again.  This
*  time, at least one piece is guaranteed to be inserted, so
*  the process will eventually terminate.  */

            else {
               split3(pil, jj, jbot, jbot);
               p = pil->pidl;
               ibot = jbot;
               } /* End handling of incompatible ranges */

            } /* End new and old range w/nonunit stride */

         } /* End loop over range segments */

/* If addition has been suppressed, print warning message */

      if (!ILR.kchg) {
         ermark(RK_WARNING+RK_MARKFLD);
         cryout(RK_P3," -->WARNING: Redundant addition ignored.",
            RK_LN1, NULL);
         }

      } /* End processing additions */

/*---------------------------------------------------------------------*
*                                                                      *
*     Consolidate.                                                     *
*                                                                      *
*     List changes are complete.  Scan through the new ilst from the   *
*     top down and consolidate any entries that have become redundant. *
*     Consolidate adjacent single values into minilists (this allows   *
*     longer lists to be detected when a third item is added and also  *
*     makes ilstiter processing slightly faster).  Convert degenerate  *
*     ranges (top == bottom) into single items.  Since one change may  *
*     enable another, iterate until no further changes occur.          *
*                                                                      *
*---------------------------------------------------------------------*/

   do {

      ILR.kchg = FALSE;
      for (ji = pil->nusm1; ji >= 0; --ji) {

/* Item at 'ji' is start of a range with nonunit stride */

         if (p[ji] & IL_INCR) {

            ji -= 2;
            ebot = p[ji];
            einc = p[ji+1] & IL_MASK;
            etop = p[ji+2] & IL_MASK;

            /* If range has no contents, delete it */
            if (etop < ebot)
               movedown(pil, ji, 3);

            /* If range is degenerate, convert it to a single */
            else if (etop == ebot)
               movedown(pil, ji+1, 2);

            /* If there are entries below this range,
            *  check for possible consolidation.  */
            else if (ji > 0) {

               if ((tag = p[ji-1] & IL_REIN) == 0) {
                  /* Item below is single:  bring it into
                  *  the range if it is one stride away.  */
                  if (p[ji-1] + einc == ebot) {
                     movedown(pil, ji, 1);
                     ji -= 1;
                     }
                  }
               else if (tag == IL_REIN) {
                  /* Item below is also a range:  merge the two
                  *  if adjacent and both have same stride.  */
                  if (p[ji-2] == p[ji+1] &&
                        (p[ji-1] & IL_MASK) + einc == ebot) {
                     movedown(pil, ji-1, 3);
                     ji -= 3;
                     }
                  }

               } /* End checking below the range */

            } /* End consolidating range with nonunit stride */

/* Item at 'ji' is start of a range with unit stride */

         else if (p[ji] & IL_REND) {

            ji -= 1;
            ebot = p[ji];
            etop = p[ji+1] & IL_MASK;

            /* If range has no contents, delete it */
            if (etop < ebot)
               movedown(pil, ji, 2);

            /* If range is degenerate, convert it to a single */
            else if (etop == ebot)
               movedown(pil, ji+1, 1);

            /* If there are entries below this range,
            *  check for possible consolidation.  */
            else if (ji > 0) {

               if ((tag = p[ji-1] & IL_REIN) == 0) {
                  /* Item below is single:  bring it into
                  *  the range if it is adjacent.  */
                  if (p[ji-1] + 1 == ebot) {
                     movedown(pil, ji, 1);
                     ji -= 1;
                     }
                  }
               else if (tag == IL_REND) {
                  /* Item below is also a range:  merge the two
                  *  if adjacent and both have unit stride.  */
                  if ((p[ji-1] & IL_MASK) + 1 == ebot) {
                     movedown(pil, ji-1, 2);
                     ji -= 2;
                     }
                  }

               } /* End checking below the range */

            } /* End consolidating range with unit stride */

/* Item at 'ji' is a single value */

         else if (ji > 0) {

            if ((tag = p[ji-1] & IL_REIN) == 0) {
               /* Item below is also single:  make into
               *  a range if they are adjacent integers.  */
               if (p[ji-1] +1 == p[ji]) {
                  p[ji] |= IL_REND;
                  ji -= 1;
                  }
               }

            else {
               /* Item below is a range:  extend it to include
               *  the current single item if it fits.  */
               if (p[ji] == ((p[ji-1] +
                     (tag == IL_REIN ? p[ji-2] : 1)) & IL_MASK)) {
                  p[ji-1] = p[ji] | tag;
                  movedown(pil, ji, 1);
                  }
               }

            } /* End consolidating a single value */

         } /* End one consolidation scan */

      } while (ILR.kchg);  /* Repeat until no more changes */

   } /* End procmini() */

/*=====================================================================*
*                               eat1num                                *
*        Local function to eat one argument if it is a number          *
*=====================================================================*/

static void eat1num(void) {

   char lea[DFLT_MAXLEN+1];   /* Scan field */
   scan(lea, RK_PMDELIM|RK_ASTDELIM|RK_SLDELIM);
   if (!(RK.scancode & RK_ENDCARD) && cntrl(lea) == CNTLRET)
      scanagn();

   } /* End eat1num */

/*=====================================================================*
*                              makeilst                                *
*            If there is no list to add to, create one now             *
*=====================================================================*/

static ilst *makeilst(void) {

   ilst *pil = ILR.poldl;

   /* If new list requested, get rid of old list items now */
   if (ILR.iop == IL_NEW && pil) {
      if (pil->pidl) ILfreev(pil->pidl, "Iteration data");
      memset((char *)pil, 0, sizeof(ilst));
      pil->evry = (long)ILR.base0 << IL_BITS;
      pil->seed = ILR.seed0;
      }

   if (ILR.iop != IL_DEL) {
      if (!pil) {
         pil = ILallocv(1, sizeof(ilst), "Iterator list head");
         pil->evry = (long)ILR.base0 << IL_BITS;
         pil->seed = ILR.seed0; }
      if (!pil->pidl) {
         pil->nallo = (size_t)ILR.mxsz;
         pil->nusm1 = -1;
         pil->pidl = ILallocv(pil->nallo, sizeof(long),
            "Iteration data list");
         }
      }

   return pil;

   } /* End makeilst() */

/*=====================================================================*
*                              ilstread                                *
*=====================================================================*/

ilst *ilstread(ilst *poldlist, int idop, int ibase, si32 seed) {

/* Local declarations: */

   ilst *pil = NULL;          /* Ptr to ilst being built */
   ilstitem ibot,iinc,itop;   /* Input bottom, incr, top */
   long jj;                   /* Temp */
   long ievry = 0;            /* EVERY block size */
   int ic;                    /* Scan code */
   int ifop;                  /* First operation */
   int ilop = 0;              /* Last operation */
   int ninc,ntop;             /* Counts of range incrs, tops */
   int oldfldlen;             /* Old scan field length */
   char lea[DFLT_MAXLEN+1];   /* Scan field */

/* Option keys:
*  N.B.  Order of first 4 must match option defs in rkilst.h.
*  Further defs below are used to switch on these options.  */

   static char *opkeys[] = { "NEW", "ADD", "DEL", "OFF", "NONE",
      "OF", "EVERY", "ALL", "FIRST", "LAST", "RANDOM", "SEED" };
   static char needpchk[] = { 1, 1, 1, 0, 0, 1, 1, 0, 1, 1, 1, 1 };

#define IL_NEEDPIL   8  /* Options above this need pil defined */
#define IL_NONE      5
#define IL_OF        6
#define IL_EVERY     7
#define IL_ALL       8
#define IL_FIRST     9
#define IL_LAST     10
#define IL_RANDOM   11
#define IL_SEED     12

   /* Be sure maximum scan length is default */
   oldfldlen = scanlen(DFLT_MAXLEN);

   /* Extract default operation code from idop and check */
   ifop = idop & (IL_NEW|IL_ADD|IL_DEL|IL_OFF);
   if (!ifop) abexit(81);

   /* Initialize global variables for this call */
   ILR.poldl = poldlist;
   ILR.mxsz  = IL_ILSZ;
   ILR.seed0 = seed ? seed : IL_DFLTSEED;
   ILR.base0 = ibase;
   ILR.iop   = 0;
   ILR.knol  = FALSE;

/*---------------------------------------------------------------------*
*                                                                      *
*  Scan a field and interpret keyword options                          *
*  (In general, report punctuation errors but process the field as if  *
*  punctuation was OK, unless this would clearly cause further errors. *
*  Keeping all options in a single scan loop allows more informative   *
*  error messages when options are in the wrong sequence.)             *
*                                                                      *
*---------------------------------------------------------------------*/

   do {
      scan(lea, RK_PMDELIM|RK_ASTDELIM|RK_SLDELIM);

/* End-of-card:  If not the first scan, this is the normal end of the
*  scan loop (cdscan should already have given an error if closing
*  parens are missing).  If it is the first scan and IL_ESOK bit is
*  set or mode is OFF, it is the same as (OFF) and causes any old
*  list that is present to be deleted.  Otherwise, give an error.  */

      if (RK.scancode & RK_ENDCARD) {
         if (ILR.iop)
            break;
         if (idop & IL_OFF)
            goto finito2;
         if (idop & IL_ESOK)
            goto finito3;
         ilstreq();
         break;
         } /* End handling end-of-card */

/* Empty field:  If this is not the first scan, or it is the first
*  scan and other stuff in parens follows the empty field, we have a
*  required-field-missing error.  Otherwise (i.e. it is only thing in
*  parens or it is not in parens), if IL_ELOK bit is set or mode is
*  OFF, it is the same as (OFF), but accepted whether or not in parens,
*  i.e.  CELLS=, NEXT-OPTION is OK.  Otherwise, give an error.   */

      if (RK.length < 0) {
         if (RK.scancode & ~(RK_COMMA|RK_RTPAREN|RK_INPARENS))
            ermark(RK_PUNCERR);
         if (ILR.iop || (RK.scancode & (RK_INPARENS|RK_RTPAREN)) ==
               RK_INPARENS) {
            ermark(RK_REQFERR);
            goto nextfield;  }
         if (idop & (IL_ELOK|IL_OFF))
            goto finito2;
         ilstreq();
         break;
         } /* End handling empty field */

/* If not a number, look for an option keyword */

      else if (cntrl(lea) == CNTLRET) {

/* Handle obsolete "MX" option */

         /* (Note inversion of ssmatch args to accept
         *  any string with initial chars "MX".)  */
         if (ssmatch("MX", lea, 2)) {
            if (RK.scancode != (RK_INPARENS|RK_EQUALS)) {
               ermark(RK_PUNCERR);
               eat1num();     /* Skip the data */
               }
            else if (pil) {
               /* Error if entered after list is set up */
               ermark(RK_MARKFLD);
               cryout(RK_P1, " ***MX must be first item in list.",
                  RK_LN1, NULL);
               eat1num();     /* Skip the data */
               }
            else
               /* Sets mxsz to 1 if <= 0 is entered */
               eqscan(&ILR.mxsz, "V>IL", RK_EQCHK);
            }

/* Look for a mode option keyword */

         else if (ic = smatch(RK_NMERR, lea, /* Assignment intended */
               opkeys, sizeof(opkeys)/sizeof(void *))) {

            /* Make sure multi-field options are in parens */
            if (needpchk[ic-1] && !(RK.scancode & RK_INPARENS)) {
               ermark(RK_PNRQERR);
               goto finito;
               }

            /* Make sure options that store in ilst have one */
            if (ic >= IL_NEEDPIL) {
               if (!ILR.iop)
                  ILR.iop = ifop;
               if (!pil)
                  pil = makeilst();
               }

            switch (ic) {

            case IL_NEW:
               if (ILR.iop > 0 || pil) {
                  ermark(RK_MARKFLD);
                  cryout(RK_P1, " ***NEW must be first item in list.",
                     RK_LN1, NULL);
                  if (pil) freeilst(pil), ILR.poldl = pil = NULL;
                  }
               /* Drop through to IL_DEL case ... */

            case IL_DEL:
               ILR.knol = FALSE; /* Might delete after some adds... */
               /* Drop through to IL_ADD case ... */

            case IL_ADD:
               if (RK.scancode & RK_RTPAREN)
                  ermark(RK_REQFERR);
               /* Omit paren check here--paren errors marked above */
               if (RK.scancode & ~(RK_COMMA|RK_INPARENS|RK_RTPAREN))
                  ermark(RK_PUNCERR);
               if (ilop == ic) cryout(RK_P3, " -->WARNING:  Mode "
                  "keyword is repeated.", RK_LN1, NULL);
               ILR.iop = ilop = ic;
               break;

            case IL_OFF:
            case IL_NONE:
               if (RK.scancode & ~(RK_INPARENS|RK_RTPAREN|RK_COMMA))
                  ermark(RK_PUNCERR);
               if (ILR.iop > 0 || pil || ((RK.scancode &
                     (RK_INPARENS|RK_RTPAREN)) == RK_INPARENS))
                  goto finito1;
               goto finito2;

            case IL_OF:
               if (RK.scancode & ~(RK_COMMA|RK_INPARENS))
                  ermark(RK_PUNCERR);
               scan(lea, RK_PMDELIM|RK_ASTDELIM|RK_SLDELIM|RK_REQFLD);
               if (!ssmatch(lea, "EVERY", 1)) {
                  ermark(RK_IDERR);
                  scanagn();
                  break;
                  }
               /* Drop through to EVERY case... */

            case IL_EVERY:
               if (pil->frst == IL_ALLI) ermark(RK_EXCLERR);
               eqscan(&ievry, "VIL", RK_BEQCK);
               if (ievry > IL_MASK) ermark(RK_NUMBERR);
               break;

            case IL_ALL:
               if (pil->frst & IL_MASK || ievry > 0)
                  ermark(RK_EXCLERR);
               pil->frst = IL_ALLI;
               break;

            case IL_FIRST:
               if (pil->frst == IL_ALLI) ermark(RK_EXCLERR);
               eqscan(&pil->frst, "VIL", RK_BEQCK);
               if (pil->frst > IL_MASK)  ermark(RK_NUMBERR);
               break;

            case IL_LAST:
               eqscan(&pil->last, "VIL", RK_BEQCK);
               if (pil->last > IL_MASK)  ermark(RK_NUMBERR);
               break;

            case IL_RANDOM:
               eqscan(&pil->rand, "VIL", RK_BEQCK);
               if (pil->rand > IL_MASK)  ermark(RK_NUMBERR);
               break;

            case IL_SEED:
               eqscan(&pil->seed, "VIJ", RK_BEQCK);
               break;

               }  /* End option switch */
            } /* End smatch found */

/* Special Case:  If an acceptable option was not found on the first
*  scan, but the default mode is OFF, push back the field, delete any
*  existing list, and return.  */

         else if (ifop == IL_OFF && ILR.iop == 0) {
            scanagn();
            goto finito2;
            }

/* Otherwise, an unmatched control field is an error */

         else {
            ermark(RK_IDERR);
            }

         } /* End handling nonnumeric field */

/*---------------------------------------------------------------------*
*                                                                      *
*     Process a number                                                 *
*                                                                      *
*     N.B.  To shorten the code, single numbers are treated as         *
*     mini-lists both for deletions and for additions.                 *
*---------------------------------------------------------------------*/

/* First check the punctuation--ignores field if error */

      else if (RK.scancode & (RK_ASTERISK|RK_SLASH|RK_EQUALS))
         ermark(RK_PUNCERR);

      else {

         if (!ILR.iop)           /* Use default mode if not input */
            ILR.iop = ifop;
         if (ILR.iop == IL_OFF)
            goto finito1;

/* Read the numeric value or range and check for errors.
*  Except where going to nextfield to look for a new range
*  start, multiple errors of the same kind are detected and,
*  marked but only one copy of the error message is printed.  */

         wbcdin(lea, &jj, RK_IORF|RK_CTST|RK_NLONG|(long)RK.length);
         if (lea[0] == '+') {
            ermark(RK_MARKFLD);
            convrt("(P1,' ***Signed plus value ',J1IL8,"
               "'(stride) must follow range start.')",
               &jj, NULL);
            goto nextfield;
            }
         if (lea[0] == '-') {
            ermark(RK_MARKFLD);
            convrt("(P1,' ***Negative value ',J1IL8,"
               "'(end of range) must follow range start.')",
               &jj, NULL);
            goto nextfield;
            }
         ibot = (ilstitem)jj;

         ninc = ntop = ic = 0;
         while (RK.scancode & RK_PMINUS) {
            scan(lea, RK_PMDELIM|RK_ASTDELIM|RK_SLDELIM|RK_REQFLD);
            if (RK.scancode & RK_ENDCARD)
               goto finito;
            if (RK.length < 1 || RK.scancode &
                  (RK_ASTERISK|RK_SLASH|RK_EQUALS)) {
               ermark(RK_PUNCERR);
               goto nextfield;
               }
            wbcdin(lea, &jj,
               RK_IORF|RK_CTST|RK_ZTST|RK_NLONG|(long)RK.length);
            if (jj >= 0) {
               /* Got a stride */
               iinc = (ilstitem)jj;
               if (ninc++ > 0 || (ILR.iop == IL_DEL && iinc != 1))
                  ermark(RK_TOOMANY);
               }
            else {
               /* Got a range top */
               itop = (ilstitem)(-jj);
               if (ntop++ > 0)
                  ermark(RK_TOOMANY);
               if (itop <= ibot)
                  ermark(RK_NUMBERR);
               }
            }

         if (ntop > 1) {
            cryout(RK_P1, " ***A range may have only one top.",
               RK_LN1, NULL);
            ic = TRUE; }
         if (ninc > 1) {
            cryout(RK_P1, " ***A range may have only one stride.",
               RK_LN1, NULL);
            ninc = 1;   /* (So following ninc>ntop test works OK */
            ic = TRUE; }
         if (ninc > ntop) {
            ermark(RK_REQFERR);
            cryout(RK_P1, " ***A stride requires a range end.",
               RK_LN1, NULL);
            ic = TRUE;  /* (Because cryout zeros RK.erscan) */
            }
         if (!ntop)
            itop = ibot;
         else if (itop <= ibot) {
            cryout(RK_P1, " ***Range end must be above "
               "range start.", RK_LN1, NULL);
            ic = TRUE; }
         if (ILR.iop == IL_DEL && ninc && iinc != 1) {
            cryout(RK_P1, " ***A non-unit stride is not allowed "
               "in a deletion range.", RK_LN1, NULL);
            ic = TRUE; }

         if (ic || (RK.erscan & 0x00FFFFFF))
            goto nextfield;

         /* Create minilist from single item */
         if (!ninc)
            iinc = 1;
         else if (iinc == 1)
            ninc = 0;

         if (!pil) pil = makeilst();

         procmini(pil, ibot, iinc, itop);

         } /* End processing numeric field */

/* End scanning unless in parens and not at right paren yet */

nextfield: ;
      } while ((RK.scancode & (RK_INPARENS|RK_RTPAREN)) == RK_INPARENS);

/*---------------------------------------------------------------------*
*          End scan loop, perform final checking and return            *
*---------------------------------------------------------------------*/

finito:

/* Store and check the EVERY option.  There is a better test in
*  ilstchk(), but that routine might not get called, also, this
*  error report comes out just below the offending control card.  */

   if (pil) {
      long evin;
      pil->evry |= ievry;
      evin = pil->evry & IL_MASK;
      if (evin && ilsthigh(pil) >= evin + SRA(pil->evry,IL_BITS)) {
         ermark(RK_MARKDLM);
         cryout(RK_P1, "0***One or more list entries exceeds "
            "the \"EVERY\" block length.", RK_LN2, NULL);
         }
      } /* End checking EVERY option */
   else
      /* This handles the case that a misspelled keyword is
      *  found before any numbers are processed--we want to
      *  preserve any existing old list.  */
      pil = ILR.poldl;

   scanlen(oldfldlen);
   return pil;

/* Got data items while in OFF mode--give error and skip over them */

finito1:
   ermark(RK_MARKFLD);
   cryout(RK_P1, " ***Data not allowed in OFF mode.", RK_LN1, NULL);
   while ((RK.scancode & (RK_INPARENS|RK_RTPAREN)) == RK_INPARENS)
      scan(NULL, 0);
   if (pil != ILR.poldl)      /* JIC */
      freeilst(pil);

/* Delete existing list, restore scan length, and return NULL */

finito2:
   if (RK.highrc < IL_OFFFLG) RK.highrc = IL_OFFFLG;
finito3:
   freeilst(ILR.poldl);
   scanlen(oldfldlen);
   return NULL;

   } /* End ilstread() */

/*=====================================================================*
*                              ilsthigh                                *
*                                                                      *
*  Return the largest value that will be touched by a given iterator.  *
*  Returns -1 if the list is empty.                                    *
*=====================================================================*/


long ilsthigh(ilst *pil) {

   ilstitem etop, einc, *p;

   if (pil->pidl == NULL || pil->nusm1 < 0)  /* Double safe test */
      return -1L;
   p = pil->pidl + pil->nusm1;
   if ((*p & IL_REIN) == 0)
      return (long)*p;
   etop = *p-- & IL_MASK;
   if ((*p & IL_REIN) == 0)
      return (long)etop;
   einc = *p-- & IL_MASK;
   return (long)(etop - (etop - *p)%einc);

   } /* End ilsthigh() */

/*=====================================================================*
*                               ilstchk                                *
*  To check that no list entry exceeds the maximum available items,    *
*     and to complete processing of the FIRST, LAST, and RANDOM        *
*     options, call                                                    *
*                                                                      *
*  int ilstchk(ilst *pil, long nmax, char *msg)                        *
*                                                                      *
*  Where:                                                              *
*     pil      is a pointer to the list to be checked.                 *
*     nmax     is the size of the array that is being iterated over.   *
*              If the size is not known, for example, for a cycle      *
*              iterator, then a value of IL_MASK can be used.          *
*     msg      is text to include in any error messages to identify    *
*              the particular iterator that has the error.             *
*                                                                      *
*  Returns:                                                            *
*     0 if the list is valid, 1 if there was an error.                 *
*     Gives abexit(84) if nmax exceed 2**30.                           *
*                                                                      *
*  This call is separate from ilstread() because there will often be   *
*  situations where 'imax' is not yet known at the time the list is    *
*  read in.  No error will occur if ilstchk() is called more than      *
*  once for the same list.                                             *
*                                                                      *
*  Notes on how the RANDOM parameter is handled:  We want to avoid     *
*  generating items that duplicate each other or anything already on   *
*  the list.  Assuming that the space of imax items is not populated   *
*  too densely, the best way to do this is probably just to generate   *
*  and test and keep going until enough items have been generated or   *
*  the list is full.  The preliminary test for (rand > imax) doesn't   *
*  take into account any items already on the list, but it will trap   *
*  the more egregious user errors.                                     *
*=====================================================================*/

int ilstchk(ilst *pil, long nmax, char *msg) {

#define ILCKBadRet 1             /* Value returned on errors */

   int rc = 0;

   if (pil) {
      long tops = ilsthigh(pil);
      long base = SRA(pil->evry,IL_BITS);
      long evin = pil->evry & IL_MASK;
      long imax;

      ILR.iop = IL_ADD;          /* Prepare for additions */

      if (evin) {
         if (evin > nmax) {
            convrt("(P1,'0***Iteration EVERY step ',J1IL8,"
               "'exceeds array size ',J1IL8,'for ',J0A32H.)",
               &evin, &nmax, msg, NULL);
            rc = ILCKBadRet;
            }
         else
            nmax = evin;
         }

      imax = nmax + base - 1;
      if (imax > IL_MASK)
         abexit(84);             /* Program needs to be rewritten */

      if (tops > imax) {
         convrt("(P1,'0***Iteration list value ',J1IL8,'exceeds "
            "array or EVERY block top ',J1IL8,'for ',J0A32H.)",
            &tops, &imax, msg, NULL);
         rc = ILCKBadRet;
         }

      if (pil->frst) {
         if (pil->frst == IL_ALLI)
            procmini(pil, base, 1, imax);
         else if (pil->frst > nmax) {
            convrt("(P1,'0***Iteration FIRST value ',J1IL8,'exceeds "
               "array or EVERY block size ',J1IL8,'for ',J0A32H.)",
               &pil->frst, &nmax, msg, NULL);
            rc = ILCKBadRet;
            }
         else
            procmini(pil, base, 1, pil->frst + base - 1);
         pil->frst = 0;          /* Prevent processing again */
         }

      if (pil->last) {
         long bott = imax - pil->last + 1;
         if (bott < base) {
            convrt("(P1,'0***Iteration LAST value ',J1IL8,'exceeds "
               "array or EVERY block size ',J1IL8,'for ',J0A32H.)",
               &pil->last, &imax, msg, NULL);
            rc = ILCKBadRet;
            }
         else
            procmini(pil, bott, 1, imax);
         pil->last = 0;          /* Prevent processing again */
         }

      if (pil->rand) {
         ilstitem *p = pil->pidl;
         long nran = pil->rand;
         if (nran > nmax) {
            convrt("(P1,'0***Iteration RANDOM value ',J1IL8,"
               "'exceeds array or EVERY block size ',J1IL8,"
               "'for ',J0A32H.)", &nran, &nmax, msg, NULL);
            rc = ILCKBadRet;
            }
         else if (nran == nmax) {
            /* Shortcut to speed things up in this unusual case */
            procmini(pil, base, 1, imax);
            }
         /* Quit if all items in the range already selected */
         else while (nran && (pil->nusm1 != 1 ||
               p[0] != base || p[1] != (imax|IL_REND))) {
            long oldn = pil->nusm1;
            long rtry = udev(&pil->seed) % nmax + base;
            procmini(pil, rtry, 1, rtry);
            /* This is how we see if the item was accepted */
            if (pil->nusm1 > oldn) --nran;
            }
         pil->rand = 0;          /* Prevent processing again */
         }

      } /* End if pil */

   RK.iexit |= rc;
   return rc;

   } /* End ilstchk() */
