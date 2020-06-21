/* (c) Copyright 1992-2016, The Rockefeller University *21115* */
/* $Id: d3pxq.c 70 2017-01-16 19:27:55Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                                d3pxq                                 *
*                                                                      *
*  This function either checks whether user has signalled the CNS      *
*  driver to pause current simulation series, or waits indefinitely    *
*  for such a message, according to the waitpm (wait pause message)    *
*  argument.  Return codes are defined in rpdef.h.                     *
*                                                                      *
*  N.B.  The messages used in this protocol are big-endian, contrary   *
*  to general policy, for historical reasons.  To preserve compati-    *
*  bility, byte-order swapping is conditionally performed if running   *
*  on a little-endian machine.  This convention should be reversed     *
*  at the next feasible opportunity.                                   *
*                                                                      *
*  N.B.  It is not clear whether there will be keyboard control under  *
*  MPI, paritcularly in a batch environment.  For now, this code is    *
*  written to communicate with the 'host' (PAR0) node, but that may    *
*  change.                                                             *
************************************************************************
*  V5C, 02/13/92, GNR and ABP                                          *
*  Rev, 03/17/92, ABP - Add waitpm and waiting option                  *
*  Rev, 04/19/93, GNR - Add PSCL_EXIT for driver termination           *
*  Rev, 05/31/99, GNR - Use new swapping scheme                        *
*  ==>, 11/24/07, GNR - Last mod before committing to svn repository   *
*  V8F, 02/20/10, GNR - Remove PLATFORM support                        *
*  R66, 03/03/16, GNR - Consistent message lengths for 32- or 64- bit  *
*  R80, 09/02/18, GNR - Replace anread/write/test calls with MPI calls *
***********************************************************************/

#include <stdio.h>
#include <stddef.h>
#include "sysdef.h"
#include "d3global.h"
#include "swap.h"

/*---------------------------------------------------------------------*
*                        d3pxq executable code                         *
*---------------------------------------------------------------------*/

int d3pxq(int waitpm) {

   int EndType;               /* Resume or resume-and-ignore */
   int host = NC.hostid;      /* Virtual node number of driver */
   int type = PAUSECTL_MSG;   /* Type of pxq message */
   char *pkt;                 /* Generic packet pointer */
   char cl[FMISIZE];          /* Buffer for integer message */

/* If waitpm flag clear and no PAUSECTL message, return D3_NORMAL,
*  allowing CNS to continue.  This code replaces antest().  */

   if (!waitpm) {
      MPI_Status tstat;
      int flag;
      int rc = MPI_Iprobe(NC.hostid, type, NC.commc, &flag, &tstat);
      if (rc) abexitm(169, ssprintf(NULL,
         "MPI_Iprobe FOR TYPE %d ON HOST RETURNED ERROR CODE %d.",
            type, rc));
      if (flag) {
         host = tstat.MPI_SOURCE;
         type = tstat.MPI_TAG;
         MPI_Get_count(&tstat, MPI_UNSIGNED_CHAR, &rc);
         }
      /* Note, 09/02/18, GNR - This test was "corrected" from
      *  "if (rc < 0)" working w/no access to MPI book--not clear
      *  what a negative Get_Count would have meant.  */
      if (rc <= 0) return D3_NORMAL;
      }
      
/* Got one--check length and contents (not significant) */

   rc = MPI_Recv(cl, FMISIZE, MPI_UNSIGNED_CHAR, host, type,
      NC.commc, MPI_STATUS_IGNORE);
   if (rc || (bemtoi4(cl) != (int)PSCL_PAUSE))
      d3exit(NULL, PAUSEMSG_ERR, 1);

/* Send PSCL_PSING to driver */

   bemfmi4(cl, PSCL_PSING);
   rc = MPI_Send(cl, FMISIZE, MPI_UNSIGNED_CHAR, host, type,
      NC.commc);
   if (rc) d3exit(NULL, PAUSEMSG_ERR, 4);

/* Wait for PSCL_RSM, PSCL_RSMABT, or PSCL_EXIT message from host.
*  (Note: This information cannot be put in the PSCL_PAUSE
*  message because it hasn't been decided yet at that time.) */

   rc = MPI_Recv(cl, FMISIZE, MPI_UNSIGNED_CHAR, host, type,
      NC.commc, MPI_STATUS_IGNORE);
   if (rc) d3exit(NULL, PAUSEMSG_ERR, 5);
   EndType = bemtoi4(cl);
   if (EndType == PSCL_EXIT) return OUT_QUICK_EXIT;

/* Resuming:  Send PSCL_RSMING to driver */

   bemfmi4(cl,PSCL_RSMING);
   rc = MPI_Send(cl, FMISIZE, MPI_UNSIGNED_CHAR, host, type,
      NC.commc);
   if (rc) d3exit(NULL, PAUSEMSG_ERR, 6);

/* Return appropriate code according to EndType */

   return (EndType == PSCL_RSMABT) ? OUT_USER_PAUSE : D3_NORMAL;
   } /* End d3pxq */
