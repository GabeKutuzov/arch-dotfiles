/* (c) Copyright 2007, The Rockefeller University *11116* */
/* $Id: bbdcgete.c 28 2017-01-13 20:35:15Z  $ */
/***********************************************************************
*                      BBD (Client Side) Package                       *
*                             bbdcgete.c                               *
*           Receive effector data from neural system to BBD            *
*                                                                      *
*  This routine may be called on a client BBD to receive simulated     *
*  motor output from the neural system simulator.                      *
*                                                                      *
*  Synopsis:                                                           *
*     int bbdcgete(BBDDev *pdev)                                       *
*                                                                      *
*  Argument:                                                           *
*     pdev is the address of the pfeff list in the BBDcd structure.    *
*                                                                      *
*  Return value:                                                       *
*     FALSE (0) if normal data were received or TRUE (1) if server     *
*     signalled that application should terminate.                     *
*                                                                      *
*  N.B.  Historically, this routine leaves the CNS output on its       *
*     internal S7/14 scale.                                            *
*                                                                      *
*  Prerequisites:                                                      *
*     First call bbdcinit() to start everything up, then call          *
*     routines in the bbdcreg[stve] family to register the senses,     *
*     cameras, values, and effectors needed in this run, then call     *
*     bbdcchk() to confirm that configurations in server and client    *
*     agree.  Then this routine can be called as needed.               *
*                                                                      *
*  Error Handling:                                                     *
*     Error checking is minimal because we are now in the main BBD     *
*     action loop.  Everything should have already been checked.       *
*     All error are terminal and result in a call to abexit() with a   *
*     suitable message.  There is nothing useful the caller can do.    *
************************************************************************
*  V1A, 03/14/07, GNR - New program                                    *
*  ==>, 05/11/07, GNR - Last mod before committing to svn repository   *
***********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

#include "sysdef.h"
#include "bbd.h"
#include "rfdef.h"

struct BBDComData BBDcd;      /* Common BBD data struct */

int bbdcgete(BBDDev *pdev) {

   BBDDev       *pd;          /* Current device */
   struct RFdef *pf;          /* File descriptor */
   float        *pmn,*pmne;   /* Ptrs to client's data array */
   ui16         ckint;        /* Control integer */

   /* Check whether server already terminated */
   if (BBDcd.Mlogrc > 0)
      abexitm(BBDcErrServer, ssprintf(NULL, "Server (CNS) terminated "
         "with abexit code %5d", BBDcd.Mlogrc));

   pf = BBDcd.ndrecv;

   /* Loop over all the devices on the argument list */
   for (pd=pdev; pd; pd=pd->pndev) {

      /* Receive and check the control integer */
      ckint = rfri2(pf);
      /* An error code follows BBDLC_TERM, but the current
      *  definition of this routine has no use for it.  */
      if (ckint == BBDLC_TERM) {
         rfri4(pf); return TRUE; }
      if (ckint != pd->bdid) abexitm(BBDcErrBadData, "Bad control "
         "integer received from neural server");

      /* Receive the motor neuron data */
      pmn = pd->UD.Eff.pmno;
      pmne = pmn + pd->ldat;
      while (pmn < pmne)
         *pmn++ = rfrr4(pf);

      } /* End device loop */

   return FALSE;

   } /* End bbdcgete() */

