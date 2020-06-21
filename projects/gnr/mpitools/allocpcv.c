/* (c) Copyright 1998-2016, The Rockefeller University *11115* */
/* $Id: allocpcv.c 3 2017-01-16 19:29:37Z  $ */
/***********************************************************************
*                          MPI Tools Library                           *
*                              allocpcv                                *
*                                                                      *
*  This routine is the callocv() analog in a shared memory management  *
*  package for parallel computers.  On serial computers, it is similar *
*  to traditional callocv() except for maintaining a running total of  *
*  the memory allocated in each pool.  For a complete description, see *
*  comments in membcst.c, the master memory broadcast routine.  Do not *
*  call on comp nodes.                                                 *
*                                                                      *
*  Synopsis:                                                           *
*     void *allocpcv(memtype kmem, size_t n, si32 mtype, char *emsg)   *
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
*     emsg     String to incorporate in error message that is written  *
*              if allocation fails (max 48 chars).                     *
*                                                                      *
*  Return value:                                                       *
*              Pointer to a contiguous block of memory of the size     *
*              requested, rounded up to next BYTE_ALIGN boundary.      *
*              The memory is initialized to zero.  If allocation       *
*              fails, a message incorporating the string 'emsg' is     *
*              printed and the calling program is terminated by a      *
*              call to abexitm with code MP_ALLOC_FAIL (=32).  The     *
*              'emsg' string should indicate what was being allocated. *
*                                                                      *
************************************************************************
*  V1A, 06/27/98, GNR - Newly written                                  *
*  V1B, 03/04/00, GNR - Add HostString, use same calls for serial and  *
*                       parallel systems.                              *
*  V1C, 03/23/00, GNR - No pmblk on comp nodes, add mbdptr, datptr,    *
*                       mbsize, negative mbtype for structured data    *
*  V1D, 05/29/00, GNR - MBT_Unint bit in mtype argument                *
*  ==>, 12/29/09, GNR - Last mod before committing to svn repository   *
*  Rev, 08/13/16, GNR - Change alloc codes from longs to si32          *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "mpitools.h"
#include "memshare.h"
#include "mempools.h"

void *allocpcv(memtype kmem, size_t n, si32 mtype, char *emsg) {

#ifdef PARn
   abexit(MP_COMP_ALLOC);     /* Don't call on comp nodes */
#ifndef GCC
   return NULL;               /* Eliminate warning re return value  */
#endif
#else

   void *pmem;                /* Ptr to allocated memory */

   if ((pmem=(void *)allocpc(kmem, n, mtype)) == NULL && emsg)
      abexitm(MP_ALLOC_FAIL, ssprintf(NULL, "Memory alloc failed "
         "for %48s (%10zd bytes requested).", emsg, n *
         (size_t)(mtype > 0 ? mtype :
         NC.pnxtt[NC.jnxtt][mtype & MBTMask])));
   return pmem;

#endif

   } /* End allocpcv() */
