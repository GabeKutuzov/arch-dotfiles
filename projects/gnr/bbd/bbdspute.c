/* (c) Copyright 2007-2013, The Rockefeller University *11116* */
/* $Id: bbdspute.c 28 2017-01-13 20:35:15Z  $ */
/***********************************************************************
*                      BBD (Server Side) Package                       *
*                             bbdspute.c                               *
*            Send effector data from neural system to BBD              *
*                                                                      *
*  This routine may be called on a server neural system to send        *
*  simulated motor output to the client BBD.                           *
*                                                                      *
*  Synopsis:                                                           *
*     void bbdspute(int quit)                                          *
*                                                                      *
*  Argument:                                                           *
*     'quit' is FALSE for normal output, TRUE to send a message to     *
*     all attached BBD devices that they should terminate.             *
*                                                                      *
*  Prerequisites:                                                      *
*     First call bbdsinit() to start everything up, then call          *
*     routines in the bbdsreg[stve] family to register the senses,     *
*     cameras, values, and effectors needed in this run, then call     *
*     bbdschk() to confirm that configurations in server and client    *
*     agree.  Then this routine can be called as needed.               *
*                                                                      *
*  Error Handling:                                                     *
*     Error checking is minimal because we are now in the main         *
*     simulation loop.  Everything should have already been checked.   *
*     All error are terminal and result in a call to abexit() with a   *
*     suitable message.  There is nothing useful the caller can do.    *
************************************************************************
*  V1A, 03/14/07, GNR - New program                                    *
*  ==>, 05/11/07, GNR - Last mod before committing to svn repository   *
*  Rev, 04/18/13, GNR - Add BBDsGotEff bit to kpute                    *
***********************************************************************/

#define BBDS

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

#include "sysdef.h"
#include "bbd.h"
#include "rfdef.h"

#ifdef DEBUG
#include "rocks.h"
#include "rkxtra.h"
#endif

struct BBDComData BBDsd;      /* Common BBD data struct */

void bbdspute(int quit) {

   struct BBDsHost *pbsh;
   BBDDev       *pd;          /* Current device */
   struct RFdef *pf;          /* File descriptor */
   float        *pmn,*pmne;   /* Ptrs to client's data array */

/* Loop over all the devices on the argument list */

   for (pd=BBDsd.pfeff; pd; pd=pd->pndev) {

#ifdef DEBUG
      cryout(RK_P1, " Sending effector ", RK_LN1, pd->devnm, 15,
         " to BBD", RK_CCL, NULL);
#endif

      /* Locate the correct socket */
      pf = pd->pshost->ndsend;

      /* Send the control integer */
      if (quit) {
#ifdef DEBUG
         cryout(RK_P1, " Sending BBDLC_TERM code 101", RK_CCL, NULL);
#endif
         rfwi2(pf, BBDLC_TERM);
         rfwi4(pf, (si32)101);
         }
      else {
#ifdef DEBUG
         convrt("(P1,' Sending bdid ',J0IH6)", &pd->bdid, NULL);
#endif
         rfwi2(pf, pd->bdid);
         }

      /* Send the motor neuron data */
#ifdef DEBUG
      convrt("(P1,' ldat = ',J0IJ10)", &pd->ldat, NULL);
      convrt("(P1,#,(10F10.4))", &pd->ldat, pd->UD.Eff.pmno, NULL);
#endif
      pmn = pd->UD.Eff.pmno;
      pmne = pmn + pd->ldat;
      while (pmn < pmne)
         rfwr4(pf, *pmn++);

      } /* End device loop */

/* Now loop over clients and flush connections to just those
*  that have effectors attached.  */

   for (pbsh=BBDsd.phlist; pbsh; pbsh=pbsh->pnhost)
      if (pbsh->kpute & BBDsGotEff) rfflush(pbsh->ndsend, ABORT);

   } /* End bbdspute() */

