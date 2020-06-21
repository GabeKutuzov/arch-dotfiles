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
*  allowing CNS to continue */

   if (!waitpm && (antest(&host,&type) < 0)) return D3_NORMAL;

/* Got one--check length and contents (not significant) */

   if ((anread(cl,FMISIZE,&host,&type,NULL) != FMISIZE) ||
         (bemtoi4(cl) != (int)PSCL_PAUSE))
      d3exit(NULL, PAUSEMSG_ERR, 1);

/* Send PSCL_PSING to driver */

   bemfmi4(cl,PSCL_PSING);
   if (anwrite(cl,FMISIZE,host,type,NULL) < 0)
      d3exit(NULL, PAUSEMSG_ERR, 4);

/* Wait for PSCL_RSM, PSCL_RSMABT, or PSCL_EXIT message from host.
*  (Note: This information cannot be put in the PSCL_PAUSE
*  message because it hasn't been decided yet at that time.) */

   if (anread(cl,FMISIZE,&host,&type,NULL) != FMISIZE)
      d3exit(NULL, PAUSEMSG_ERR, 5);
   EndType = bemtoi4(cl);
   if (EndType == PSCL_EXIT) return OUT_QUICK_EXIT;

/* Resuming:  Send PSCL_RSMING to driver */

   bemfmi4(cl,PSCL_RSMING);
   if (anwrite(cl,FMISIZE,host,type,NULL) < 0)
      d3exit(NULL, PAUSEMSG_ERR, 6);

/* Return appropriate code according to EndType */

   return (EndType == PSCL_RSMABT) ? OUT_USER_PAUSE : D3_NORMAL;
   } /* End d3pxq */
