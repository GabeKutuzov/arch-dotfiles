/* (c) Copyright 2016-2018, The Rockefeller University *11115* */
/* $Id: wsum.c 3 2017-01-16 19:29:37Z  $ */
/***********************************************************************
*                          MPI Tools Library                           *
*                               wsum()                                 *
*                                                                      *
*  Routine to find sum of an si64 variable across a processor array.   *
*  Incidentally performs an incoming synchronization.                  *
************************************************************************
*  V1A, 06/20/16, GNR - New routine, based on isum()                   *
*  ==>, 06/25/16, GNR - Last mod before committing to svn mpi repo     *
*  R10, 09/02/18, GNR - Eliminate anread/anwrite                       *
***********************************************************************/

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include "sysdef.h"
#include "mpitools.h"
#include "swap.h"

si64 wsum(si64 mypart) {

   int mynode = rnode(NC.node);
   int nnodes = NC.cnodes + 1;      /* One more for host */
   int type = WSUM_MSG;
   int rc;
   char msgsi64[FMWSIZE];
   register int chan;

/* Each node in the remaining upper half cube sends a message
*  to the corresponding node in the lower half, then drops out. */

   for (chan=1; chan<nnodes; chan<<=1) {
      int pard = mynode ^ chan;
      int next = anode(pard);

      if (mynode & chan) {
         lemfmi8(msgsi64, mypart);
         rc = MPI_Send(msgsi64, FMWSIZE, MPI_UNSIGNED_CHAR, next, type,
            NC.commc);
         if (rc) mpiwex(type, next, rc, "wsum");
         break; }
      else if (pard < nnodes) {
         si64 mysum;
         rc = MPI_Recv(msgsi64, FMWSIZE, MPI_UNSIGNED_CHAR, next, type,
            NC.commc, MPI_STATUS_IGNORE);
         if (rc) mpirex(type, next, rc, "wsum");
         mysum = lemtoi8(msgsi64);
         mypart = jasw(mysum, mypart);
         }

      } /* End of loop over channels */

   return mypart;
   } /* End of wsum() */

