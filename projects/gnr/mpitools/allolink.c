/* (c) Copyright 1998-2009, The Rockefeller University *11115* */
/* $Id: allolink.c 3 2017-01-16 19:29:37Z  $ */
/***********************************************************************
*                          MPI Tools Library                           *
*             allolink, allounlk, allogetp, and allodel0               *
*                                                                      *
************************************************************************
*  V1A, 07/18/98, GNR - allolink newly written                         *
*  V1F, 06/23/01, GNR - allounlk, allogetp, and allodel0 added         *
*  ==>, 12/29/09, GNR - Last mod before committing to svn repository   *
*  Rev, 04/24/16, GNR - Type of magic changed to intptr                *
*  ==>, 04/29/16, GNR - Last mod before committing to svn mpi repo     *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "memshare.h"
#include "mempools.h"

/***********************************************************************
*                              allolink                                *
*                                                                      *
*  This routine links an mblkdef block to the end of a bidirectional   *
*  linked list and increments the block count for the list.  It is     *
*  used by various routines in the shared memory management package.   *
*  It should not be called directly from applications.                 *
*                                                                      *
*  Synopsis:                                                           *
*     void allolink(struct mblkdef *pa, struct mbbdlink *pl)           *
*                                                                      *
*  Arguments:                                                          *
*     pa       Ptr to mblkdef block to be linked.                      *
*     pl       Ptr to head of list to which pa is to be added.         *
*                                                                      *
*  Return value:                                                       *
*     Nothing is returned.                                             *
*                                                                      *
*  Note:                                                               *
*     There is no good place to do the initialization that             *
*     would be required to get rid of the test on pnmbd.               *
*                                                                      *
***********************************************************************/

void allolink(struct mblkdef *pa, struct mbdlhdr *pl) {

   if (!pl->link.pnmbd) {
      /* First allocation on this list */
      pl->link.pnmbd = pl->link.ppmbd = pa;
      pa->link.pnmbd = pa->link.ppmbd = (struct mblkdef *)pl;
      }
   else {
      /* Add new allocation to end of list */
      (pa->link.ppmbd = pl->link.ppmbd)->link.pnmbd = pa;
      (pa->link.pnmbd = (struct mblkdef *)pl)->link.ppmbd = pa;
      }
   ++pl->nlblks;

   } /* End allolink() */

/***********************************************************************
*                              allounlk                                *
*                                                                      *
*  This routine unlinks an mblkdef block from a bidirectional linked   *
*  list and decrements the block count (but not the total length) for  *
*  the list.  On comp nodes, the "magic" number is passed as an        *
*  argument because it exists only as a local variable in membcst().   *
*  Function allounlk() is used by various routines in the shared       *
*  memory management package.  It should not be called directly from   *
*  applications.                                                       *
*                                                                      *
*  Synopsis:                                                           *
*  (On comp nodes):                                                    *
*     struct mbdlist *allounlk(struct mblkdef *pa, intptr magic)       *
*  (On host or serial nodes):                                          *
*     struct mbdlist *allounlk(struct mblkdef *pa)                     *
*                                                                      *
*  Arguments:                                                          *
*     pa       Ptr to mblkdef block to be unlinked.                    *
*     magic    "magic" field relevant to the pa mblkdef.               *
*                                                                      *
*  Return value:                                                       *
*     Pointer to mbdlist that the block was unlinked from.             *
*                                                                      *
***********************************************************************/

#ifdef PARn
struct mbdlist *allounlk(struct mblkdef *pa, intptr magic)
#else
struct mbdlist *allounlk(struct mblkdef *pa)
#endif
{

   struct mbdlist *pl;

   /* Locate the correct mbdlist.  In pool-change situations, the
   *  block is stored in the old pool on comp nodes, but in the new
   *  pool on host nodes.  Otherwise, it is in the list specified in
   *  the PoolMask field of the block.  */
#ifdef PARn
   pl = &MI.mchain[((magic & MPM_CHP) ?
      magic >> OldPShift : magic) & PoolMask];
#else
   pl = &MI.mchain[pa->info.magic & PoolMask];
#endif
   if (pa->link.pnmbd == pa->link.ppmbd) {
      pa->link.ppmbd->link.pnmbd = NULL;
      pa->link.ppmbd->link.ppmbd = NULL;
      }
   else {
      pa->link.ppmbd->link.pnmbd = pa->link.pnmbd;
      pa->link.pnmbd->link.ppmbd = pa->link.ppmbd;
      }
#ifdef PAR0
   if (pa->info.magic & (CodeMask|MPM_CHP))
      --pl->mods.nlblks;
   else
      --pl->base.nlblks;
#else
   --pl->base.nlblks;
#endif

   return pl;

   } /* End allounlk() */

#ifdef PAR0
/***********************************************************************
*                              allogetp                                *
*                                                                      *
*  This routine locates the MPM_LOC block that precedes a change       *
*  block, checks it, unlinks it, and returns a pointer to it.          *
*                                                                      *
*  Synopsis:                                                           *
*     struct mblkdef *allogetp(struct mblkdef *pa)                     *
*                                                                      *
*  Argument:                                                           *
*     pa       Ptr to mblkdef MPM_CHG block whose locator is           *
*              to be located.                                          *
*                                                                      *
*  Return value:                                                       *
*     A pointer to the MPM_LOC block corresponding to the given        *
*     MPM_CHG block.                                                   *
*                                                                      *
***********************************************************************/

struct mblkdef *allogetp(struct mblkdef *pa) {

   struct mblkdef *pp = pa->link.ppmbd;

   /* Just a little safety check */
   if (!(pp->info.magic & MPM_LOC))
      abexit(MP_REF_UNALLO);
   allounlk(pp);

   return pp;
   } /* End allogetp() */

/***********************************************************************
*                              allodel0                                *
*                                                                      *
*  Link an mblkdef into a mods list as an MPM_DEL block.  If the       *
*  MPM_CHP bit is set, link the deletion into the mods list for        *
*  the original pool, not the one that the block is currently          *
*  assigned to, because it is still in the old pool on the comp        *
*  nodes.                                                              *
*                                                                      *
*  Synopsis:                                                           *
*     void allodel0(struct mblkdef *pa)                                *
*                                                                      *
*  Argument:                                                           *
*     pa       Ptr to mblkdef block to be converted to a deletion.     *
*                                                                      *
*  Return value:                                                       *
*     Nothing is returned.                                             *
*                                                                      *
*=====================================================================*/

void allodel0(struct mblkdef *pa) {

   struct mbdlist *pl;        /* Ptr to list entry for this area */
   memtype       kmem;        /* Memory pool containing block */

   kmem = (memtype)(((pa->info.magic & MPM_CHP) ?
      pa->info.magic >> OldPShift : pa->info.magic) & PoolMask);
   pl = &MI.mchain[kmem];
   pa->info.magic = MPMAGIC | MPM_DEL | (intptr)kmem;
   allolink(pa, &pl->mods);

   } /* End allodel0() */
#endif
