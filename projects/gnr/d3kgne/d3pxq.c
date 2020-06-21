/* (c) Copyright 1992-1910, Neurosciences Research Foundation, Inc. */
/* $Id: d3pxq.c 28 2010-04-12 20:18:17Z  $ */
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
*  V5C, 02/13/92, GNR and ABP                                          *
*  Rev, 03/17/92, ABP - Add waitpm and waiting option                  *
*  Rev, 04/19/93, GNR - Add PSCL_EXIT for driver termination           *
*  Rev, 05/31/99, GNR - Use new swapping scheme                        *
*  ==>, 11/24/07, GNR - Last mod before committing to svn repository   *
*  V8F, 02/20/10, GNR - Remove PLATFORM support                        *
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
   char cl[FMLSIZE];          /* Buffer for message-format long */

/* If waitpm flag clear and no PAUSECTL message, return D3_NORMAL,
*  allowing CNS to continue */

   if (!waitpm && (antest(&host,&type) < 0)) return D3_NORMAL;

/* Got one--check length and contents (not significant) */

   if ((anread(cl,FMLSIZE,&host,&type,NULL) != FMLSIZE) ||
         (bemtoi4(cl) != (long)PSCL_PAUSE))
      d3exit(PAUSEMSG_ERR,NULL,1);

/* Send PSCL_PSING to driver */

   bemfmi4(cl,PSCL_PSING);
   if (anwrite(cl,FMLSIZE,host,type,NULL) < 0)
      d3exit(PAUSEMSG_ERR,NULL,4);

/* Wait for PSCL_RSM, PSCL_RSMABT, or PSCL_EXIT message from host.
*  (Note: This information cannot be put in the PSCL_PAUSE
*  message because it hasn't been decided yet at that time.) */

   if (anread(cl,FMLSIZE,&host,&type,NULL) != FMLSIZE)
      d3exit(PAUSEMSG_ERR,NULL,5);
   EndType = bemtoi4(cl);
   if (EndType == PSCL_EXIT) return OUT_QUICK_EXIT;

/* Resuming:  Send PSCL_RSMING to driver */

   bemfmi4(cl,PSCL_RSMING);
   if (anwrite(cl,FMLSIZE,host,type,NULL) < 0)
      d3exit(PAUSEMSG_ERR,NULL,6);

/* Return appropriate code according to EndType */

   return (EndType == PSCL_RSMABT) ? OUT_USER_PAUSE : D3_NORMAL;
   } /* End d3pxq */
