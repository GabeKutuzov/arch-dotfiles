/* (c) Copyright 2007-2015, The Rockefeller University *11116* */
/* $Id: bbdschk.c 28 2017-01-13 20:35:15Z  $ */
/***********************************************************************
*                      BBD (Server Side) Package                       *
*                              bbdschk.c                               *
*            Check setup of a BBD with network simulation              *
*                                                                      *
*  This routine must be called from a neuronal network simulator       *
*  (presumably CNS) working with a client BBD after all cameras,       *
*  senses, values, and effectors have been registered in order to      *
*  validate the setup with the BBD (on one or more hosts) and          *
*  finalize the data structures that will be used during the actual    *
*  simulation.  The client hosts must all call bbdcchk() to            *
*  communication with this routine.                                    *
*                                                                      *
*  Synopsis:                                                           *
*     int bbdschk(void)                                                *
*                                                                      *
*  Arguments:                                                          *
*     None.  All the data structures will have been set up and will    *
*     be accessible in global memory via the BBDsd structure.          *
*                                                                      *
*  Return value:                                                       *
*     0        Operation successful, BBD may begin running.            *
*                                                                      *
*  Prerequisites:                                                      *
*     First call bbdsinit() to start everything up, then call          *
*     routines in the bbdsreg[stve] family to register the senses,     *
*     cameras, values, and effectors needed in this run.               *
*                                                                      *
*  Error Handling:                                                     *
*     All error are terminal and result in a call to abexit() with a   *
*     suitable message.  There is nothing useful the caller can do.    *
*                                                                      *
*  Implementation Note:                                                *
*     This program calls signal() to ignore SIGPIPE signals.  This is  *
*     to allow read() calls on the pipe to report errors, rather than  *
*     just get the application terminated (although the default action *
*     according to one source at least, is ignore anyway).  Obviously, *
*     if we eventually need a signal handler for some pipe signals and *
*     not others, a more global way of controlling this will be needed.*
************************************************************************
*  V1A, 02/28/07, GNR - New program                                    *
*  ==>, 09/19/07, GNR - Last mod before committing to svn repository   *
*  Rev, 09/26/08, GNR - Minor type changes for 64-bit compilation      *
*  Rev, 11/17/08, GNR - Add call to ignore SIGPIPE to get err msgs     *
*  V1C, 11/09/10, GNR - Add checking for BBDMdlt options               *
*  Rev, 01/15/11, GNR - Add initial handshake with bbdcchk             *
*  Rev, 04/18/13, GNR - Add external access to bbdsquit                *
*  Rev, 12/19/15, GNR - Send effector rows,cols to client              *
***********************************************************************/

#define BBDS

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <strings.h>
#include "sysdef.h"
#ifdef UNIX
#include <signal.h>
#endif
#include "bbd.h"
#ifdef DEBUG
#include "rocks.h"
#include "rkxtra.h"
#endif
#include "rfdef.h"
#include "rksubs.h"

struct BBDComData BBDsd;            /* Common BBD data struct */

/*=====================================================================*
*                              bbdsclst                                *
*                                                                      *
*      Internal routine to check the entries on one device list        *
*=====================================================================*/

static void bbdsclst(BBDDev *pfdev) {

   BBDDev          *pd;
   struct BBDsHost *pbsh;
   long            rack;            /* Received ack */
   ui16            ckint;

/* First make sure the rfdefs for all the client hosts have
*  been set up and zero the userdata field in each so a count
*  can be kept below of the number of devices attached at that
*  client host.  */

   for (pbsh=BBDsd.phlist; pbsh; pbsh=pbsh->pnhost) {
      if (!pbsh->ndsend) {
         pbsh->ndsend = rfallo(pbsh->pbbdnm, WRITE, BINARY,
            INITIATOR, TOP, NO_LOOKAHEAD, REWIND, RELEASE_BUFF,
            IGNORE, IGNORE, pbsh->bbdport, ABORT);
         rfopen(pbsh->ndsend, NULL, WRITE, BINARY,
            INITIATOR, TOP, NO_LOOKAHEAD, REWIND, RELEASE_BUFF,
            IGNORE, IGNORE, pbsh->bbdport, ABORT);
         pbsh->ndrecv = rfdups(pbsh->ndsend, 8192, ABORT);
         rfwi2(pbsh->ndsend, 0);    /* Tell bbdcchk all is well */
         }
      pbsh->ndsend->userdata = 0;
      }

/* Make a sort-of random number for checking data exchange.  Force
*  this number to be no more than 8 bits.  Larger values can then be
*  used to signal termination or other future functions via the data
*  interface.  */

   ckint = (ui16)((ui32)pfdev >> 3 & BBDLC_MASK);

/* Loop over devices of one type */

   for (pd=pfdev; pd; pd=pd->pndev) {

      ui16 kdev = pd->bdflgs & (BBDTMask|BBDMdlt);
#ifdef DEBUG
   cryout(RK_P1, "  processing device <", RK_LN1, pd->devnm,
      MaxDevNameLen, ">", 1+RK_FLUSH, NULL);
#endif

      /* Get and check pointer to client connection info */
      if (!(pbsh = pd->pshost))     /* Assignment intended */
         abexitm(BBDsErrNoHost, ssprintf(NULL, "BBD Device <%15s> "
            "has no associated host", pd->devnm));

      /* Error if modality info requested and pointer not set */
      if (kdev == (BBDTSns|BBDMdlt) && !pd->UD.Sns.pisgn ||
            kdev == (BBDTCam|BBDMdlt) && !pd->UD.Cam.pisgn)
         abexitm(BBDsErrParams, ssprintf(NULL, "BBD Device <%15s> "
            "rqsts stim id but no address given", pd->devnm));

      /* Send the length first -- a nonzero value tells the BBD
      *  to keep listening...  */
#ifdef DEBUG
   cryout(RK_P1, ssprintf(NULL, "  Sending ldat = %d", pd->ldat),
      RK_LN1+RK_FLUSH, NULL);
#endif
      rfwi4(pbsh->ndsend, pd->ldat);
      /* Send the name next so bbdcchk can match to BBDDev */
      rfwrite(pbsh->ndsend, pd->devnm, sizeof(pd->devnm), ABORT);
      /* Send flags */
      rfwi2(pbsh->ndsend, pd->bdflgs);
#ifdef DEBUG
   cryout(RK_P1, ssprintf(NULL, "  Sending bdid = %d", (int)ckint),
      RK_LN1+RK_FLUSH, NULL);
#endif
      /* For effectors, send row, col dimensions */
      if (kdev & BBDTEff) {
         rfwi4(pbsh->ndsend, pd->UD.Eff.effx);
         rfwi4(pbsh->ndsend, pd->UD.Eff.effy);
         }
      /* Send check integer and increment for next device */
      rfwi2(pbsh->ndsend, pd->bdid = ckint);
      ckint = (ckint + CkIntIncr) & BBDLC_MASK;

      /* Increment count for acknowledgement validation below */
      pbsh->ndsend->userdata += 1;
      } /* End loop over device list */

/* All done with this list, send end signal to each client and flush.
*  The number received back should match the number of devices of the
*  current type attached by each client.  A negative value indicates a
*  mismatch at the client.  Set the kpute BBDsGotEff flag if this host
*  receives effector data.  */

   for (pbsh=BBDsd.phlist; pbsh; pbsh=pbsh->pnhost)
         if (pbsh->ndsend->userdata) {
      rfwi4(pbsh->ndsend, 0);
      rfflush(pbsh->ndsend, ABORT);

      rack = (long)rfri4(pbsh->ndrecv);
      if (rack != pbsh->ndsend->userdata) abexitm(BBDsErrBadData,
         "Configuration does not agree with that defined on BBD");
      if (pfdev == BBDsd.pfeff) pbsh->kpute |= BBDsGotEff;
      } /* End checkcount checking */

   } /* End bbdsclst */

/*=====================================================================*
*                               bbdschk                                *
*=====================================================================*/

int bbdschk(void) {

#ifdef UNIX
   signal(SIGPIPE, SIG_IGN);
#endif
#ifdef DEBUG
   cryout(RK_P1, " -->bbdschk processing sensors",
      RK_LN1+RK_FLUSH, NULL);
#endif
   if (BBDsd.pfsns) bbdsclst(BBDsd.pfsns);
#ifdef DEBUG
   cryout(RK_P1, " -->bbdschk processing ttvs",
      RK_LN1+RK_FLUSH, NULL);
#endif
   if (BBDsd.pfttv) bbdsclst(BBDsd.pfttv);
#ifdef DEBUG
   cryout(RK_P1, " -->bbdschk processing etvs",
      RK_LN1+RK_FLUSH, NULL);
#endif
   if (BBDsd.pfetv) bbdsclst(BBDsd.pfetv);
#ifdef DEBUG
   cryout(RK_P1, " -->bbdschk processing stvs",
      RK_LN1+RK_FLUSH, NULL);
#endif
   if (BBDsd.pfstv) bbdsclst(BBDsd.pfstv);
#ifdef DEBUG
   cryout(RK_P1, " -->bbdschk processing values",
      RK_LN1+RK_FLUSH, NULL);
#endif
   if (BBDsd.pfval) bbdsclst(BBDsd.pfval);
#ifdef DEBUG
   cryout(RK_P1, " -->bbdschk processing effectors",
      RK_LN1+RK_FLUSH, NULL);
#endif
   if (BBDsd.pfeff) bbdsclst(BBDsd.pfeff);
#ifdef DEBUG
   cryout(RK_P1, " -->bbdschk returning",
      RK_LN1+RK_FLUSH, NULL);
#endif

   return 0;

   } /* End bbdschk() */

