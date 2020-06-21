/* (c) Copyright 2016-2018, The Rockefeller University *11115* */
/* $Id: jmax.c 3 2017-01-16 19:29:37Z  $ */
/***********************************************************************
*                          MPI Tools Library                           *
*                               jmax()                                 *
*                                                                      *
*  Routine to find maximum value of an si32 variable across a proces-  *
*  sor array.  Incidentally performs an incoming synchronization.      *
************************************************************************
*  V1A, 06/20/16, GNR - New routine, based on imax()                   *
*  ==>, 06/25/16, GNR - Last mod before committing to svn mpi repo     *
*  R10, 09/01/18, GNR - Eliminate anread/anwrite                       *
***********************************************************************/

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include "sysdef.h"
#include "mpitools.h"
#include "swap.h"

si32 jmax(si32 mypart) {

   int mynode = rnode(NC.node);
   int nnodes = NC.cnodes + 1;      /* One more for host */
   int type = JMAX_MSG;
   int rc;
   char msgsi32[FMJSIZE];
   register int chan;

/* Each node in the remaining upper half cube sends a message
*  to the corresponding node in the lower half, then drops out. */

   for (chan=1; chan<nnodes; chan<<=1) {
      int pard = mynode ^ chan;
      int next = anode(pard);

      if (mynode & chan) {
         lemfmi4(msgsi32, mypart);
         rc = MPI_Send(msgsi32, FMJSIZE, MPI_UNSIGNED_CHAR,
            next, type, NC.commc);
         if (rc) mpiwex(type, next, rc, "jmax");
         break; }
      else if (pard < nnodes) {
         si32 mymax;
         rc = MPI_Recv(msgsi32, FMJSIZE, MPI_UNSIGNED_CHAR,
            next, type, NC.commc, MPI_STATUS_IGNORE);
         if (rc) mpirex(type, next, rc, "jmax");
         mymax = lemtoi4(msgsi32);
         if (mymax > mypart) mypart = mymax;
         }

      } /* End of loop over channels */

   return mypart;
   } /* End of jmax() */

