/* (c) Copyright 2007-2010, The Rockefeller University *11116* */
/* $Id: bbdcputs.c 28 2017-01-13 20:35:15Z  $ */
/***********************************************************************
*                      BBD (Client Side) Package                       *
*                             bbdcputs.c                               *
*              Send sense data from BBD to neural system               *
*                                                                      *
*  This routine may be called on a client BBD to send simulated        *
*  sensory neuron data to the neural system simulator.                 *
*                                                                      *
*  Synopsis:                                                           *
*     void bbdcputs(BBDDev *pdev)                                      *
*                                                                      *
*  Argument:                                                           *
*     pdev is the address of the pfsns list in the BBDcd structure.    *
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
*  V1C, 11/11/10, GNR - Add ability to return stimulus category info   *
***********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

#include "sysdef.h"
#include "bbd.h"
#include "rfdef.h"

struct BBDComData BBDcd;      /* Common BBD data struct */

void bbdcputs(BBDDev *pdev) {

   BBDDev       *pd;          /* Current device */
   struct RFdef *pf;          /* File descriptor */

   /* Check whether server already terminated */
   if (BBDcd.Mlogrc > 0)
      abexitm(BBDcErrServer, ssprintf(NULL, "Server (CNS) terminated "
         "with abexit code %5d", BBDcd.Mlogrc));

   pf = BBDcd.ndsend;

   /* Loop over all the devices on the argument list */
   for (pd=pdev; pd; pd=pd->pndev) {

#ifdef DEBUG
      printf("Writing sense %s to CNS\n", pd->devnm);
#endif

      /* Send the control integer */
      rfwi2(pf, pd->bdid);

      /* Send the sense data if device is used */
      if (pd->bdflgs & BBDUsed) {
         if (pd->bdflgs & BBDSns_R4) {
            /* Handle 4-byte real (float) sense data */
            float *psd,*psde;
            psd = (float *)pd->UD.Sns.psa;
            psde = psd + pd->ldat;
            while (psd < psde) {
#ifdef DEBUG
               printf("  %8.3f", *psd);
#endif
               rfwr4(pf, *psd++);
               }
            }
         else
            /* Handle single-byte sense data */
            rfwrite(pf, pd->UD.Sns.psa, pd->ldat, ABORT);
         if (pd->bdflgs & BBDMdlt) {
            rfwi2(pf, pd->UD.Sns.pisgn[0]);
            rfwi2(pf, pd->UD.Sns.pisgn[1]);
            }
         }
#ifdef DEBUG
      printf("\n");
#endif

      } /* End device loop */

   /* Flush the stream */
   rfflush(pf, ABORT);

   } /* End bbdcputs() */
