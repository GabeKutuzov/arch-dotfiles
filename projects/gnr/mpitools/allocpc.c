/* (c) Copyright 1998-2016, The Rockefeller University *11115* */
/* $Id: allocpc.c 3 2017-01-16 19:29:37Z  $ */
/***********************************************************************
*                          MPI Tools Library                           *
*                               allocpc                                *
*                                                                      *
*  This routine is the calloc() analog in a shared memory management   *
*  package for parallel computers.  On serial computers, it is similar *
*  to traditional calloc() except for maintaining a running total of   *
*  the memory allocated in each pool.  For a complete description, see *
*  comments in membcst.c, the master memory broadcast routine.  Do not *
*  call on comp nodes.                                                 *
*                                                                      *
*  Synopsis:                                                           *
*     void *allocpc(memtype kmem, size_t n, si32 mtype)                *
*                                                                      *
*  Arguments:                                                          *
*     kmem     memory pool from which blocks are to be allocated       *
*              (an element of the enum mempool defined in memshare.h). *
*     n        number of objects to allocate.                          *
*     mtype    If > 0:  size of each object in bytes.  If <= 0:  code  *
*              identifying type of block to be allocated (0x80000000 + *
*              index of entry in NXDRTT table, the first longword of   *
*              which is the length of a block of the desired type).    *
*              MBT_Unint bit indicates block may contain bad pointers. *
*                                                                      *
*  Return value:                                                       *
*              Pointer to a contiguous block of memory of the size     *
*              requested, rounded up to next BYTE_ALIGN boundary.      *
*              The memory is initialized to zero.  Returns NULL if     *
*              a block of the requested size is not available.         *
*                                                                      *
************************************************************************
*  V1A, 06/13/98, GNR - Newly written                                  *
*  V1B, 03/04/00, GNR - Add HostString, use same calls for serial and  *
*                       parallel systems.                              *
*  V1C, 03/23/00, GNR - No pmblk on comp nodes, add mbdptr, datptr,    *
*                       mbsize, negative mbtype for structured data    *
*  V1D, 05/29/00, GNR - MBT_Unint bit in mtype argument                *
*  ==>, 12/29/09, GNR - Last mod before committing to svn repository   *
*  Rev, 02/18/16, GNR - Change alloc codes from longs to si32          *
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

void *allocpc(memtype kmem, size_t n, si32 mtype) {

#ifdef PARn
   abexit(MP_COMP_ALLOC);     /* Don't call on comp nodes */
#ifndef GCC
   return NULL;               /* Eliminate warning re return value  */
#endif
#else

   struct mblkdef *pa;        /* Ptr to allocated area */
   struct mbdlist *pl;        /* Ptr to list entry for this area */
   size_t ldata, ltot;        /* Length of data, total length */

/* Check for valid pool */

   if ((int)kmem >= Npools)
      abexit(MP_UNREC_POOL);
   if (kmem == Static && MI.bcsttype > 0)
      abexit(MP_CHG_STATIC);

/* Allocate mblkdef and storage together to save system calls */

   ldata = n * ((mtype > 0) ? mtype :
      NC.pnxtt[NC.jnxtt][mtype & MBTMask]);
   ltot = LMBDUP + ALIGN_UP(ldata);
   if ((pa = (struct mblkdef *)calloc(1, ltot)) == NULL)
      return NULL;

/* Link the mblkdef into the mods or base list for this pool */

   pl = &MI.mchain[kmem];
#ifdef PAR
   allolink(pa, &pl->mods);
#else
   allolink(pa, &pl->base);
#endif

/* Increment global pool size */

   pl->lallmblks += ltot;

/* Set remaining fields in mblkdef and return pointer to data */

   pa->info.magic =
#ifdef PAR
      MPMAGIC | MPM_ADD | (intptr)kmem;
#else
      MPMAGIC | (intptr)kmem;
#endif
   pa->info.mbtype = (long)mtype;
   pa->info.nmb    = (long)n;

   return (pa->info.pmblk = datptr(pa));

#endif

   } /* End allocpc() */
