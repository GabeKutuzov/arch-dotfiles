/* (c) Copyright 1998-2009, The Rockefeller University *11115* */
/* $Id: memsize.c 3 2017-01-16 19:29:37Z  $ */
/***********************************************************************
*                          MPI Tools Library                           *
*                               memsize                                *
*                                                                      *
*  This routine is part of a shared memory management package for      *
*  parallel computers.  See membcst.c for the package description.     *
*  memsize() returns the total allocated size of one or more memory    *
*  pools as specified in its argument.  It may be called on host or    *
*  comp nodes at any time.                                             *
*                                                                      *
*  Synopsis:                                                           *
*     size_t memsize(mempoolspec pools)                                *
*                                                                      *
*  Argument:                                                           *
*     pools    Sum of one or more values indicating the memory pools   *
*              whose sizes are to be added and returned.  Use the      *
*              constants beginning with "MPS_" defined in memshare.h   *
*              for this purpose.                                       *
*                                                                      *
*  Returns:                                                            *
*     Sum of sizes of the specified memory pools on the calling host.  *
************************************************************************
*  V1A, 07/04/98, GNR - New routine                                    *
*  ==>, 04/30/16, GNR - Last mod before committing to svn repository   *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "memshare.h"
#include "mempools.h"

size_t memsize(mempoolspec pools) {

   size_t totalmem = 0;
   mempoolspec tpools;
   int ipc;

#ifdef PARn
   tpools = pools & ((1<<Nshpools)-1);
#else
   tpools = pools & ((1<<Npools)-1);
#endif
   for (ipc=0; tpools; ++ipc,tpools>>=1) {
      if (tpools & 1)
         totalmem += MI.mchain[ipc].lallmblks;
      }

   return totalmem;

   } /* End memsize() */
