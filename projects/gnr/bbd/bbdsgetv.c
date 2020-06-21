/* (c) Copyright 2007-2008, The Rockefeller University *11116* */
/* $Id: bbdsgetv.c 28 2017-01-13 20:35:15Z  $ */
/***********************************************************************
*                      BBD (Server Side) Package                       *
*                             bbdsgetv.c                               *
*              Get value data from BBD to neural system                *
*                                                                      *
*  This routine may be called on a server neural system to receive     *
*  simulated value data from a client BBD.                             *
*                                                                      *
*  Synopsis:                                                           *
*     int bbdsgetv(void)                                               *
*                                                                      *
*  Prerequisites:                                                      *
*     First call bbdsinit() to start everything up, then call          *
*     routines in the bbdsreg[stve] family to register the senses,     *
*     cameras, values, and effectors needed in this run, then call     *
*     bbdschk() to confirm that configurations in server and client    *
*     agree.  Then this routine can be called as needed.               *
*                                                                      *
*  Return value:                                                       *
*     0 if everything is OK, TRUE if client requested normal stop.     *
*                                                                      *
*  Error Handling:                                                     *
*     Error checking is minimal because we are now in the main         *
*     simulation loop.  Everything should have already been checked.   *
*     All error are terminal and result in a call to abexit() with a   *
*     suitable message.  There is nothing useful the caller can do.    *
************************************************************************
*  V1A, 03/14/07, GNR - New program                                    *
*  Rev, 11/20/07, GNR - Add event signalling                           *
*  ==>, 12/21/07, GNR - Last mod before committing to svn repository   *
*  Rev, 09/26/08, GNR - Minor type changes for 64-bit compilation      *
***********************************************************************/

#define BBDS

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include "sysdef.h"
#include "bbd.h"
#include "rfdef.h"

struct BBDComData BBDsd;      /* Common BBD data struct */

int bbdsgetv(void) {

   BBDDev           *pd;      /* Current device */
   struct RFdef     *pf;      /* File descriptor */
   struct BBDsHost *psh;      /* Ptr to client host info */
   float       *pv,*pve;      /* Ptrs to client's data array */
   ui16           ckint;      /* Control integer */

   /* Loop over all the devices on the argument list */
   for (pd=BBDsd.pfval; pd; pd=pd->pndev) {

      /* Locate the correct socket */
      psh = pd->pshost;
      pf = psh->ndrecv;

      /* If a control integer was read and saved by an earlier call to
      *  bbdsevnt(), use it, otherwise read a new one and see whether
      *  it is a terminal or event type.  */
      if (psh->qlabd) {
         ckint = psh->labdid;
         psh->qlabd = FALSE;
         }
      else {
         ckint = rfri2(pf);
         if (ckint == BBDLC_EVNT) {
            /* Pick up event code, then regular data */
            BBDsd.BBDSigCode |= rfri4(pf);
            ckint = rfri2(pf);
            }
         }

      if (ckint == BBDLC_TERM) return TRUE;
      if (ckint != pd->bdid) abexitm(BBDsErrBadData, ssprintf(NULL,
         "Bad control integer %6d received by bbdsgetv", (int)ckint));

      /* Receive the value data */
      pv = pd->UD.Val.pval;
      pve = pv + pd->ldat;
      while (pv < pve)
         *pv++ = rfrr4(pf);

      } /* End device loop */

   return 0;

   } /* End bbdsgetv() */

