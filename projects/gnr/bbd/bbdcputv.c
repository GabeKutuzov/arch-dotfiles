/* (c) Copyright 2007, The Rockefeller University *11116* */
/* $Id: bbdcputv.c 28 2017-01-13 20:35:15Z  $ */
/***********************************************************************
*                      BBD (Client Side) Package                       *
*                             bbdcputv.c                               *
*              Send value data from BBD to neural system               *
*                                                                      *
*  This routine may be called on a client BBD to send simulated value  *
*  data to the neural system simulator.                                *
*                                                                      *
*  Synopsis:                                                           *
*     void bbdcputv(BBDDev *pdev)                                      *
*                                                                      *
*  Argument:                                                           *
*     pdev is the address of the pfval list in the BBDcd structure.    *
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
*  ==>, 11/23/07, GNR - Last mod before committing to svn repository   *
***********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

#include "sysdef.h"
#include "bbd.h"
#include "rfdef.h"

struct BBDComData BBDcd;      /* Common BBD data struct */

void bbdcputv(BBDDev *pdev) {

   BBDDev       *pd;          /* Current device */
   struct RFdef *pf;          /* File descriptor */
   float        *pv,*pve;     /* Ptrs to client's data array */

   /* Check whether server already terminated */
   if (BBDcd.Mlogrc > 0)
      abexitm(BBDcErrServer, ssprintf(NULL, "Server (CNS) terminated "
         "with abexit code %5d", BBDcd.Mlogrc));

   pf = BBDcd.ndsend;

   /* Loop over all the devices on the argument list */
   for (pd=pdev; pd; pd=pd->pndev) {

#ifdef DEBUG
      printf("Writing value %s to CNS\n", pd->devnm);
#endif

      /* Send the control integer */
      rfwi2(pf, pd->bdid);

      /* Send the value data */
      pv = pd->UD.Val.pval;
      pve = pv + pd->ldat;
      while (pv < pve) {
#ifdef DEBUG
         printf("  %8.3f", *pv);
#endif
         rfwr4(pf, *pv++);
         }

#ifdef DEBUG
      printf("\n");
#endif

      } /* End device loop */

   /* Flush the stream */
   rfflush(pf, ABORT);

   } /* End bbdcputv() */

