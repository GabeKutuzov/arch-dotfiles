/* (c) Copyright 2001-2016, The Rockefeller University *11115* */
/* $Id: chngpool.c 3 2017-01-16 19:29:37Z  $ */
/***********************************************************************
*                          MPI Tools Library                           *
*                              chngpool                                *
*                                                                      *
*  This routine is part of the shared memory management package for    *
*  parallel computers.  It may be called at any time on the host node  *
*  to change the memory pool to which an existing block is allocated.  *
*  It may not be used to move a block into the STATIC pool after the   *
*  first broadcast, however.  (On systems where pools are maintained   *
*  by the operating system, the data may actually have to be copied    *
*  to the new pool.  This is not the case with the standard UNIX       *
*  implementation.)  Do not call on comp nodes.                        *
*                                                                      *
*  Synopsis:                                                           *
*     void chngpool(void *ptr, memtype newkmem)                        *
*                                                                      *
*  Arguments:                                                          *
*     ptr      pointer to the old data object that is to be moved to   *
*              a different pool.                                       *
*     newkmem  memtype enumeration name of the new pool.  It is not    *
*              an error to request a move to the pool that the block   *
*              currently belongs to.                                   *
*                                                                      *
************************************************************************
*                              whatpool                                *
*                                                                      *
*  This routine may be used to determine what memory pool a block of   *
*  memory currently is assigned to.  Do not call on comp nodes.        *
*                                                                      *
*  Synopsis:                                                           *
*     memtype whatpool(void *ptr)                                      *
*                                                                      *
*  Argument:                                                           *
*     ptr      pointer to a data object allocated with this package.   *
*                                                                      *
************************************************************************
*  V1A, 05/27/01, GNR - Newly written                                  *
*  ==>, 12/29/09, GNR - Last mod before committing to svn repository   *
*  Rev, 04/24/16, GNR - Change some longs to ui32                      *
*  ==>, 08/13/16, GNR - Last mod before committing to svn mpi repo     *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "mpitools.h"
#include "memshare.h"
#include "mempools.h"
#include "rksubs.h"

/*=====================================================================*
*                              whatpool                                *
*=====================================================================*/

memtype whatpool(void * ptr) {

#ifdef PARn
   abexit(MP_COMP_ALLOC);     /* Don't call on comp nodes */
#ifndef GCC
   return 0;                  /* Eliminate warning re return value */
#endif
#else
   struct mblkdef *pa;
   if (!ptr) abexit(MP_REF_UNALLO);
   pa = mbdptr(ptr);
   return (memtype)(pa->info.magic & PoolMask);
#endif

   } /* End whatpool() */

/*=====================================================================*
*                              chngpool                                *
*=====================================================================*/

void chngpool(void *ptr, memtype newkmem) {

#ifdef PARn
   abexit(MP_COMP_ALLOC);     /* Don't call on comp nodes */
#else

   struct mbdlist *pnl,*pol;  /* Ptrs to new, old pool lists */
   struct mblkdef       *pa;  /* Ptr to current mblkdef */
#ifdef PAR
   struct mblkdef       *pp;  /* Ptr to previous mblkdef */
#endif
   size_t              ltot;  /* Length of data + mblkdef */
#ifdef PAR
   int                btype;  /* Action code from old block */
#endif
   intptr          lnewkmem;  /* newkmem as intptr for magic */
   memtype          oldkmem;  /* Memory pool containing block */

/* Verify that ptr points to a valid block and extract oldkmem */

   pa = mbdptr(ptr);
   if (!ptr || (pa->info.magic & MPMMask) != MPMAGIC)
      abexitm(MP_REF_UNALLO, ssprintf(NULL, "Attempted to change "
         "pool of unowned memory at %p.", ptr));
   oldkmem = (memtype)(pa->info.magic & PoolMask);

/* If new pool is same as old pool, there is nothing to do */

   if (newkmem == oldkmem) return;

/* Check for valid new pool */

   if ((int)newkmem >= Npools)
      abexit(MP_UNREC_POOL);
   if (newkmem == Static && MI.bcsttype > 0)
      abexit(MP_CHG_STATIC);
   lnewkmem = (intptr)newkmem;

/* Unlink the memory block from its present location
*  and locate heads of new and old pools.  */

   pol = allounlk(pa);
   pnl = &MI.mchain[newkmem];

/* Update the memory sizes of the new and old pools */

   ltot = mbsize(pa);
   ltot = LMBDUP + ALIGN_UP(ltot);
   pol->lallmblks -= ltot;
   pnl->lallmblks += ltot;

#ifdef PAR

/* Extract the operation code bits from the existing mblkdef */

   btype = (int)(pa->info.magic & (CodeMask|MPM_CHP));

/* Case I.  Old memory is Host, new is not.  If the block is an
*  MPM_CHG, find the preceding MPM_LOC block and delete it.  Move
*  the block to the mods list for the new pool as an MPM_ADD.  */

   if (oldkmem == Host) {
      switch (btype) {
      case 0:                       /* Block is a base entry */
      case MPM_ADD:                 /* Block is a new addition */
         break;
      case MPM_CHG:                 /* Block is a realloc */
         free(allogetp(pa));
         break;
      default:
         abexit(MP_BAD_POOLDR);
         } /* End btype switch */
      pa->info.magic = MPMAGIC | MPM_ADD | lnewkmem;
      allolink(pa, &pnl->mods);
      }

/* Case II.  New memory is Host, old is not.  If the old block is
*  an MPM_ADD, just move it to the host list.  Otherwise, make an
*  MPM_DEL on the old list and an MPM_ADD on the new list.  */

   else if (newkmem == Host) {
      switch (btype) {
      case 0:                       /* Block is a base entry */
      case MPM_CHP:                 /* Block is a pool change */
         /* Make duplicate and link into mods list as a deletion */
         pp = (struct mblkdef *)mallocv(LMBDUP, "Mem locator blk");
         pp->info = pa->info;
         allodel0(pp);
         break;
      case MPM_ADD:                 /* Block is a new addition */
         break;
      case MPM_CHG:                 /* Block is a realloc */
         /* Make preceding LOC block into a deletion */
         allodel0(allogetp(pa));
         break;
      default:
         abexit(MP_BAD_POOLDR);
         } /* End btype switch */
      /* We can just make a base list entry for the Host pool */
      pa->info.magic = MPMAGIC | (intptr)Host;
      allolink(pa, &pnl->base);
      }

/* Case III.  Both new and old memory are on comp nodes.  If the old
*  block is a base block or MPM_CHP block, move it to the mods list
*  for the new pool as an MPM_CHP.  If it is an MPM_ADD, move it
*  unchanged to the mods list for the new pool.  If it is an MPM_CHG,
*  move it to the mods list for the new pool and move the preceding
*  MPM_LOC block there as an MPM_LOC+MPM_CHP block (unless the pool
*  change would actually restore it to its earlier base pool).  */

   else {
      switch (btype) {
      case 0:                       /* Block is a base entry */
         pa->info.magic = MPMAGIC | MPM_CHP |
            ((intptr)oldkmem << OldPShift) | lnewkmem;
         break;
      case MPM_ADD:                 /* Block is a new addition */
         pa->info.magic = MPMAGIC | MPM_ADD | lnewkmem;
         break;
      case MPM_CHG:                 /* Block is a realloc */
         pp = allogetp(pa);
         pp->info.magic = ((pp->info.magic & MPM_CHP) ?
            ( ((pp->info.magic >> OldPShift & PoolMask) == lnewkmem) ?
               MPMAGIC | MPM_LOC : pp->info.magic & ~PoolMask ) :
            MPMAGIC | MPM_CHP | MPM_LOC | (intptr)oldkmem<<OldPShift) |
            lnewkmem;
         allolink(pp, &pnl->mods);
         pa->info.magic = MPMAGIC | MPM_CHG | lnewkmem;
         break;
      case MPM_CHP:                 /* Block is a pool change */
         /* If new pool == original pool, we can just move the
         *  block back to the base list on the host and forget
         *  about broadcasting a mods item.  */
         if ((pa->info.magic>>OldPShift & PoolMask) == lnewkmem) {
            pa->info.magic = MPMAGIC | lnewkmem;
            allolink(pa, &pnl->base);
            return;
            }
         pa->info.magic = (pa->info.magic & ~PoolMask) | lnewkmem;
         break;
      default:
         abexit(MP_BAD_POOLDR);
         } /* End btype switch */
      allolink(pa, &pnl->mods);
      } /* End else (new and old pools on comp nodes) */

#else    /* Serial */

/* On a serial system, there are no mods lists.  Just move the
*  memory block definition to the new base list.  */

   pa->info.magic = MPMAGIC | lnewkmem;
   allolink(pa, &pnl->base);

#endif   /* Serial */

#endif   /* not PARn */

   } /* End chngpool() */
