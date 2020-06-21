/* (c) Copyright 1998-2018, The Rockefeller University *11115* */
/* $Id: imax.c 3 2017-01-16 19:29:37Z  $ */
/***********************************************************************
*                          MPI Tools Library                           *
*                               imax()                                 *
*                                                                      *
*  Routine to find maximum value of an int variable across a processor *
*  array.  Incidentally performs an incoming synchronization.          *
************************************************************************
*  V1A, 10/10/98, GNR - New routine, based on isum()                   *
*  Rev, 05/22/99, GNR - Use new swapping scheme                        *
*  ==>, 12/29/09, GNR - Last mod before committing to svn repository   *
*  Rev, 04/30/16, GNR - Replicate into imax(). jmax(), wmax()          *
*  ==>, 06/25/16, GNR - Last mod before committing to svn mpi repo     *
*  R10, 09/01/18, GNR - Eliminate anread/anwrite                       *
***********************************************************************/

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include "sysdef.h"
#include "mpitools.h"
#include "swap.h"

int imax(int mypart) {

   int mynode = rnode(NC.node);
   int nnodes = NC.cnodes + 1;      /* One more for host */
   int type = IMAX_MSG;
   int rc;
   char msgint[FMISIZE];
   register int chan;

/* Each node in the remaining upper half cube sends a message
*  to the corresponding node in the lower half, then drops out. */

   for (chan=1; chan<nnodes; chan<<=1) {
      int pard = mynode ^ chan;
      int next = anode(pard);

      if (mynode & chan) {
         lemfmi4(msgint, mypart);
         rc = MPI_Send(msgint, FMISIZE, MPI_UNSIGNED_CHAR, next, type,
            NC.commc);
         if (rc) mpiwex(type, next, rc, "imax");
         break; }
      else if (pard < nnodes) {
         int mymax;
         rc = MPI_Recv(msgint, FMISIZE, MPI_UNSIGNED_CHAR, next, type,
            NC.commc, MPI_STATUS_IGNORE);
         if (rc) mpirex(type, next, rc, "imax");
         mymax = lemtoi4(msgint);
         if (mymax > mypart) mypart = mymax;
         }

      } /* End of loop over channels */

   return mypart;
   } /* End of imax() */

