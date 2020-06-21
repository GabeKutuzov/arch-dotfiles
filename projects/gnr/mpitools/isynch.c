/* (c) Copyright 1991-2018, The Rockefeller University *21115* */
/* $Id: isynch.c 3 2017-01-16 19:29:37Z  $ */
/***********************************************************************
*                          MPI Tools Library                           *
*                              isynch()                                *
*                                                                      *
*  Routine to perform incoming synchronization across processor        *
*     array.  The NC struct must be initialized before any calls.      *
************************************************************************
*  V1A, 07/08/91, GNR                                                  *
*  V2A, 09/04/93, ABP - Add T/NM ring version.                         *
*  V3A, 11/16/96, GNR - Remove support for non-hybrid versions.        *
*  ==>, 12/29/09, GNR - Last mod before committing to svn repository   *
*  Rev, 06/20/16, GNR - Modify for MPI interface                       *
*  ==>, 06/25/16, GNR - Last mod before committing to svn mpi repo     *
*  R10, 09/01/18, GNR - Eliminate anread/anwrite                       *
***********************************************************************/

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include "sysdef.h"
#include "mpitools.h"

void isynch(void) {

   int mynode = rnode(NC.node);
   int nnodes = NC.cnodes + 1;      /* One more for host */
   int type = ISYNCH_MSG;
   int ismsg = 0x4953594E;
   int rc;
   register int chan;

/* Each node in the remaining upper half cube sends a message
*  to the corresponding node in the lower half, then drops out. */

   for (chan=1; chan<nnodes; chan<<=1) {
      int pard = mynode ^ chan;
      int next = anode(pard);

      if (mynode & chan) {
         rc = MPI_Send(&ismsg, ISIZE, MPI_UNSIGNED_CHAR, next, type,
            NC.commc);
         if (rc) mpiwex(type, next, rc, "isynch");
         break; }
      else if (pard < nnodes) {
         rc = MPI_Recv(&ismsg, ISIZE, MPI_UNSIGNED_CHAR, next, type,
            NC.commc, MPI_STATUS_IGNORE);
         if (rc) mpirex(type, next, rc, "isynch");
         }

      } /* End of loop over channels */

   } /* End isynch() */

