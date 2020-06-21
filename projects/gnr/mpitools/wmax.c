/* (c) Copyright 2016-2018, The Rockefeller University *11115* */
/* $Id: wmax.c 3 2017-01-16 19:29:37Z  $ */
/***********************************************************************
*                          MPI Tools Library                           *
*                               wmax()                                 *
*                                                                      *
*  Routine to find maximum value of an si64 variable across a proces-  *
*  sor array.  Incidentally performs an incoming synchronization.      *
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

si64 wmax(si64 mypart) {

   int mynode = rnode(NC.node);
   int nnodes = NC.cnodes + 1;      /* One more for host */
   int type = WMAX_MSG;
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
         if (rc) mpiwex(type, next, rc, "wmax");
         break; }
      else if (pard < nnodes) {
         si64 mymax;
         rc = MPI_Recv(msgsi64, FMWSIZE, MPI_UNSIGNED_CHAR, next, type,
            NC.commc, MPI_STATUS_IGNORE);
         if (rc) mpirex(type, next, rc, "wmax");
         mymax = lemtoi8(msgsi64);
         if (qsw(jrsw(mymax,mypart)) > 0) mypart = mymax;
         }

      } /* End of loop over channels */

   return mypart;
   } /* End of wmax() */

