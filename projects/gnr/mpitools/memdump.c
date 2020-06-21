/* (c) Copyright 1999-2016, The Rockefeller University *11115* */
/* $Id: memdump.c 3 2017-01-16 19:29:37Z  $ */
/***********************************************************************
*                          MPI Tools Library                           *
*                              memdump.c                               *
*                                                                      *
*  This routine may be called on a serial, host, or compxp node to     *
*  dump the list of memory blocks allocated with the new allocp[mcr]   *
*  family of routines.  Output is to stdout on serial and par host     *
*  nodes, to dbgprt on parallel comp nodes.                            *
*                                                                      *
*  Argument:                                                           *
*     pools    Specification (see memshare.h) of pools that should     *
*              be dumped.                                              *
************************************************************************
*  V1A, 09/11/99, GNR - New program                                    *
*  V1B, 03/04/00, GNR - StringData -> SharedString, add HostString     *
*  V1C, 03/23/00, GNR - No pmblk on comp nodes, add mbdptr, datptr,    *
*                       mbsize, negative mbtype for structured data    *
*  ==>, 12/29/09, GNR - Last mod before committing to svn repository   *
*  ==>, 08/13/16, GNR - Last mod before committing to svn mpi repo     *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "rksubs.h"
#include "memshare.h"
#include "mempools.h"
#include "mpitools.h"

void memdump(mempoolspec pools) {

   struct mblkdef *pa;        /* Ptr to mblkdef being processed */
   struct mbdlist *pl;        /* Ptr to info for a given pool */
   mempoolspec tpools;        /* Temp for scanning pools */
   size_t nblks,iblk;         /* Block loop control */
   int ipc;                   /* Pool selector, pool counter */
   static char *poolnames[] = {
      "Static", "Dynamic", "Shared", "Private", "Host" } ;

   /* Avoid accessing nonexistent mchain entries */
#ifdef PARn
   tpools = pools & ((1<<Nshpools)-1);
#else
   tpools = pools & ((1<<Npools)-1);
#endif
   for (ipc=0; tpools; ++ipc,tpools>>=1) {
      if (tpools & 1) {       /* Only dump requested pools */
         dbgprt(ssprintf(NULL, "Dumping memory blocks in %s pool",
            poolnames[ipc]));
         pl = &MI.mchain[ipc];
         pa = pl->base.link.pnmbd;
         nblks = (size_t)pl->base.nlblks;
         for (iblk=0; iblk<nblks; iblk++,pa=pa->link.pnmbd) {
            ui32 mtype = (ui32)pa->info.mbtype;
#ifdef PARn
            if (pa->info.mbtype > 0)
               dbgprt(ssprintf(NULL, "Mem at %p (host %p) has %ld "
                  "blocks of size %jd", datptr(pa), pa->info.phblk,
                  pa->info.nmb, mtype));
            else
               dbgprt(ssprintf(NULL, "Mem at %p (host %p) has %ld "
                  "blocks of type 0x%8x and size %ld", datptr(pa),
                  pa->info.phblk, pa->info.nmb, mtype,
                  NC.pnxtt[NC.jnxtt][mtype & MBTMask]));
#else /* Must be serial or PAR0--same code for both */
            if (pa->info.mbtype > 0)
               fputs(ssprintf(NULL, "Host mem at %p has %ld blocks "
                  "of size %jd\n", pa->info.pmblk, pa->info.nmb,
                  mtype), stderr);
            else
               fputs(ssprintf(NULL, "Host mem at %p has %ld blocks "
                  "of type 0x%8x and size %ld\n", pa->info.pmblk,
                  pa->info.nmb, mtype,
                  NC.pnxtt[NC.jnxtt][mtype & MBTMask]), stderr);
#endif
            } /* End dumping one mblkinfo block */
         dbgprt(ssprintf(NULL, "The total memory recorded for this"
            " pool is %zd bytes", pl->lallmblks));
         } /* End dumping one pool */
      } /* End loop over requested memory pools */

   } /* End memdump() */
