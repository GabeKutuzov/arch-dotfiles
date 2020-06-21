/* (c) Copyright 2007-2013, The Rockefeller University *11116* */
/* $Id: bbdsgets.c 28 2017-01-13 20:35:15Z  $ */
/***********************************************************************
*                      BBD (Server Side) Package                       *
*                             bbdsgets.c                               *
*              Get sense data from BBD to neural system                *
*                                                                      *
*  This routine may be called on a server neural system to receive     *
*  simulated sensory neuron data from a client BBD.                    *
*                                                                      *
*  Synopsis:                                                           *
*     int bbdsgets(void)                                               *
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
*  V1C, 11/09/10, GNR - Add code for isn,ign return                    *
*  Rev, 07/08/13, GNR - Correct DEBUG convrt/printf calls              *
***********************************************************************/

#define BBDS

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include "sysdef.h"
#include "bbd.h"
#include "rfdef.h"


#ifdef DEBUG
#include  "rocks.h"
#include  "rkxtra.h"
#endif

struct BBDComData BBDsd;      /* Common BBD data struct */

int bbdsgets(void) {

   BBDDev           *pd;      /* Current device */
   struct RFdef     *pf;      /* File descriptor */
   struct BBDsHost *psh;      /* Ptr to client host info */
   float     *psd,*psde;      /* Ptrs to float data, data end */
   ui16           ckint;      /* Control integer */

   /* Loop over all the devices on the argument list */
   for (pd=BBDsd.pfsns; pd; pd=pd->pndev) {

#ifdef DEBUG
      cryout(RK_P1," Reading sense ", RK_LN1, pd->devnm, 15,
         " from BBD", RK_CCL, NULL);
#endif

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
         "Bad control integer %6d received by bbdsgets", (int)ckint));

      /* Receive the sense data if device is used */
      if (pd->bdflgs & BBDUsed) {
         if (pd->bdflgs & BBDSns_R4) {
            /* Handle 4-byte real (float) sense data */
            psd = (float *)pd->UD.Sns.psa;
            psde = psd + pd->ldat;
            while (psd < psde)
               *psd++ = rfrr4(pf);
#ifdef DEBUG
            convrt("(P1,#,(10F10.4))",&pd->ldat,pd->UD.Sns.psa,NULL);
#endif
            }
         else
            /* Handle single-byte sense data */
            rfread(pf, pd->UD.Sns.psa, pd->ldat, ABORT);
         if (pd->bdflgs & BBDMdlt) {
            pd->UD.Sns.pisgn[0] = rfri2(pf);
            pd->UD.Sns.pisgn[1] = rfri2(pf);
            }
         }
#ifdef DEBUG
         cryout(RK_P1," ", RK_LN1+RK_FLUSH+1, NULL);
#endif


      } /* End device loop */

   return 0;

   } /* End bbdsgets() */

