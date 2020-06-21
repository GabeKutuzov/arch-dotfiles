/* (c) Copyright 1998-2016, The Rockefeller University *11115* */
/* $Id: allocpr.c 3 2017-01-16 19:29:37Z  $ */
/***********************************************************************
*                          MPI Tools Library                           *
*                               allocpr                                *
*                                                                      *
*  This routine is the realloc() analog in a shared memory management  *
*  package for parallel computers.  On serial computers, it is similar *
*  to the traditional realloc() except the arguments are in calloc()   *
*  style and it maintains a running total of the memory allocated in   *
*  each pool.  For a complete description, see comments in membcst.c,  *
*  the master memory broadcast routine.  Do not call on comp nodes.    *
*                                                                      *
*  Synopsis:                                                           *
*     void *allocpr(void *ptr, size_t n, si32 mtype)                   *
*                                                                      *
*  Arguments:                                                          *
*     ptr      pointer to old data object to be reallocated.  This     *
*              cannot be a NULL pointer (because the pool is not       *
*              specified in this call for an initial allocation).      *
*     n        number of objects to allocate.  N.B.  The size          *
*              arguments to allocpr are in the style of calloc()       *
*              rather than malloc() so arrays of blocks of a given     *
*              type can be reallocated (expanded) in a single call.    *
*     mtype    If > 0:  size of each object in bytes.  If <= 0:  code  *
*              identifying type of block to be allocated (0x80000000 + *
*              index of entry in NXDRTT table, the first longword of   *
*              which is the length of a block of the desired type).    *
*              MBT_Unint bit indicates block may contain bad pointers. *
*              The type does not have to match that of the existing    *
*              area, but the memory data will remain unchanged.        *
*                                                                      *
*  Return value:                                                       *
*              Pointer to a contiguous block of memory of the size     *
*              requested, rounded up to next BYTE_ALIGN boundary.      *
*              The old contents are copied into the new memory area.   *
*              As with the traditional realloc(), pointers inside the  *
*              data area are not updated.  (This could be changed.)    *
*              Returns NULL if a block of the requested size is not    *
*              available.                                              *
*                                                                      *
************************************************************************
*  V1A, 06/13/98, GNR - Newly written                                  *
*  V1B, 03/04/00, GNR - Add HostString, use same calls for serial and  *
*                       parallel systems.                              *
*  V1C, 03/23/00, GNR - No pmblk on comp nodes, add mbdptr, datptr,    *
*                       mbsize, negative mbtype for structured data    *
*  V1D, 05/29/00, GNR - MBT_Unint bit in mtype argument                *
*  V1E, 06/10/00, GNR - Use a signal handler to detect bad pointers.   *
*  V1F, 06/20/01, GNR - Handle Host pool and CHP block correctly.      *
*  ==>, 12/29/09, GNR - Last mod before committing to svn repository   *
*  Rev, 10/04/16, GNR - Change alloc codes from longs to si32          *
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

#if 0    /*** DEBUG ***/
#ifdef UNIX
#include <setjmp.h>
#include <signal.h>
static jmp_buf MP_realsegvret;

/*=====================================================================*
*                           MP_realsegvhand                            *
*                                                                      *
*          Detect attempts to reallocate from a bad pointer            *
*=====================================================================*/

static void MP_realsegvhand(int sig) {

   if (sig != SIGSEGV) exit(sig);   /* JIC */
   longjmp(MP_realsegvret, TRUE);
   }
#endif
#endif  /*** DEBUG ***/

/*=====================================================================*
*                               allocpr                                *
*=====================================================================*/

void *allocpr(void *ptr, size_t n, si32 mtype) {

#ifdef PARn
   abexit(MP_COMP_ALLOC);     /* Don't call on comp nodes */
#ifndef GCC
   return NULL;               /* Eliminate warning re return value  */
#endif
#else

   struct mblkdef *pa;        /* Ptr to new allocated area */
   struct mblkdef *po;        /* Ptr to old allocated area */
   struct mbdlist *pl;        /* Ptr to list entry for this area */
   size_t ldata, loldd, ltot; /* Length of new, old data & total */
   memtype       kmem;        /* Memory pool containing block */

/* Verify that ptr points to a valid block and extract kmem */

   po = mbdptr(ptr);
#ifdef UNIX
#if 0   /*** DEBUG ***/
   /* Under UNIX, enable interception of bad pointer errors */
   void (*oldsegvhand)(int) = signal(SIGSEGV, MP_realsegvhand);
   if (setjmp(MP_realsegvret) || (po->info.magic & MPMMask) != MPMAGIC)
      abexitm(MP_REF_UNALLO, ssprintf(NULL, "Attempted to reallocate "
         "unowned memory at %p.", ptr));
   signal(SIGSEGV, oldsegvhand);
#endif   /*** DEBUG ***/
#else
   if ((po->info.magic & MPMMask) != MPMAGIC)
      abexitm(MP_REF_UNALLO, ssprintf(NULL, "Attempted to reallocate "
         "unowned memory at %p.", ptr));
#endif
   kmem = (memtype)(po->info.magic & PoolMask);

/* Check for valid pool */

   if ((int)kmem >= Npools)
      abexit(MP_UNREC_POOL);
   if (kmem == Static && MI.bcsttype > 0)
      abexit(MP_CHG_STATIC);

/* Return with no action if no change in data type and number */

   if (mtype == po->info.mbtype && n == po->info.nmb) return ptr;

/* Allocate mblkdef and storage together to save system calls */

   ldata = n * ((mtype > 0) ? mtype :
      NC.pnxtt[NC.jnxtt][mtype & MBTMask]);
   loldd = mbsize(po);
   ltot = LMBDUP + ALIGN_UP(ldata);
   if ((pa = (struct mblkdef *)realloc(po, ltot)) == NULL)
      return NULL;
   pl = &MI.mchain[kmem];

#ifdef PAR
/* If the area was on the base list, create a locator entry on the
*  mods list so comp nodes can find the old entry.  Follow that with
*  the change entry indicating the new size and location.  This use
*  of two entries avoids expanding the size of the mblkbcst struct
*  just to handle this rather unusual case.  Note:  If the existing
*  block is an MPM_CHP, unlink and relink it, preserving the MPM_CHP
*  bit, to make sure the locator and change blocks end up adjacent.  */

   if (!(pa->info.magic & MPM_CHG)) {
      allounlk(pa);
      /* Make duplicate and link into mods list */
      po = (struct mblkdef *)mallocv(LMBDUP, "Mem locator blk");
      allolink(po, &pl->mods);
      po->info = pa->info;
      po->info.magic |= MPM_LOC;
      /* Link change block into mods list */
      allolink(pa, &pl->mods);
      pa->info.magic = MPMAGIC | MPM_CHG | (intptr)kmem;
      }
   else  /* Pairs with if statement in block below */
#endif

/* If already on the mods list, or serial, and the area was relocated,
*  just update the forward and back links.  Status as an add or change
*  block remains unchanged and old locator block remains valid.  */

   if (pa != po) {
      pa->link.pnmbd->link.ppmbd = pa;
      pa->link.ppmbd->link.pnmbd = pa;
      }

/* Update global pool size */

   pl->lallmblks += (ldata - loldd);

/* Set remaining fields in mblkdef and return pointer to data */

   pa->info.mbtype = (long)mtype;
   pa->info.nmb    = (long)n;

   return (pa->info.pmblk = datptr(pa));

#endif

   } /* End allocpr() */
