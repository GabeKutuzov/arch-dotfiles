/* (c) Copyright 2010-2016, The Rockefeller University *11116* */
/* $Id: bbdcchk.c 17 2017-10-13 19:00:07Z  $ */
/***********************************************************************
*               BBD (Client Side) mex-Function Package                 *
*                              bbdcchk.c                               *
*           Check setup of a BBD with a network simulation             *
*                                                                      *
*  This is the matlab mex-function version of bbdcchk().               *
*                                                                      *
*  This routine must be called from a MATLAB script using the BBD      *
*  package after all cameras, senses, values, and effectors have       *
*  been registered in order to validate the setup with the network     *
*  simulator (presumably CNS) and finalize the data structures that    *
*  will be used during the actual simulation.  The server side must    *
*  call bbdschk() to communicate with this routine.                    *
*                                                                      *
*  MATLAB Synopsis:                                                    *
*     bbdcchk(BBDComm)                                                 *
*                                                                      *
*  Arguments:                                                          *
*     BBDComm  MATLAB array containing private data returned by a      *
*              previous call to bbdcinit.                              *
*                                                                      *
*  Prerequisites:                                                      *
*     First call bbdcinit() to start everything up, then call          *
*     routines in the bbdcreg[stve] family to register the senses,     *
*     cameras, values, and effectors needed in this run.               *
*                                                                      *
*  Error Handling:                                                     *
*     All error are terminal and result in a call to abexit() with a   *
*     suitable message.  There is nothing useful the caller can do.    *
************************************************************************
*  V1A, 10/22/10, GNR - New mex version                                *
*  ==>, 11/04/10, GNR - Last mod before committing to svn repository   *
*  Rev, 11/05/10, GNR - Better error checking for bad child procs.     *
*  Rev, 01/15/11, GNR - Now can expect BBDLC_TERM if host fails.       *
*  Rev, 12/19/15, GNR - Receive effector dimensions from server,       *
*                       allow mnad == 0 to use unchecked CNS value.    *
*  Rev, 02/19/16, GNR - Mac: use gettimeofday, simulate sem_timedwait  *
***********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <semaphore.h>
#include <string.h>

#define I_AM_MEX
#define BAD_CHILD_WAIT_TIME  5   /* Seconds */

#include "mex.h"
#include "sysdef.h"
#include "bbd.h"
#include "rfdef.h"

/*=====================================================================*
*                              bbdcclst                                *
*                                                                      *
*      Internal routine to check the entries on one device list        *
*                                                                      *
*  (Arguments are a pointer to the BBDcd common data and a pointer to  *
*   a pointer to the start of the device list to be checked.)          *
*=====================================================================*/

static void bbdcclst(struct BBDComData *BBDcd, BBDDev **ppsdev) {

   BBDDev  **ppd,*pd;
   BBDDev  *pd0 = *ppsdev;    /* Ptr to list of unmatched devs */
   si32    len;               /* Native-ordered length */
   int     nack = 0;          /* Negative error count */
   int     pack = 0;          /* Number of devices -- check integer */
   ui16    rdflgs;            /* Flags as read */
   char    dname[MaxDevNameLen+1];  /* Device name */

   *ppsdev = NULL;            /* Initialize sorted list */

   /* A length of zero indicates end of this list
   *  (from server's point of view).  */
   while (len = rfri4(BBDcd->ndrecv)) { /* Assignment intended */
      if (rfread(BBDcd->ndrecv, dname, sizeof(dname), ABORT)
            != sizeof(dname))
         abexitm(BBDcErrChkConf, "Could not read confirmation data");
      /* Match the name to devices on the unmatched list */
      for (ppd=&pd0; pd=*ppd; ppd=&pd->pndev) {
         if (!strncmp(dname, pd->devnm, MaxDevNameLen))
            goto GotNameMatch;
         }
      mexWarnMsgTxt(ssprintf(NULL, "Name from server <%15s> not "
         "found on BBD.\n", dname));
      nack -= 1;
      continue;
      /* Matched the name, check the length */
GotNameMatch:
      /* Read the flags.  Everything should match except 0x70 bits.
      *  Set BBDUsed bit from server so client can skip sending data
      *  for unused devices.  */
      rdflgs = rfri2(BBDcd->ndrecv);
      if ((rdflgs ^ pd->bdflgs) & ~BBDUMask) {
         mexWarnMsgTxt(ssprintf(NULL, "Flags mismatch: %6x != %6x "
            "for %15s\n", (int)rdflgs, (int)pd->bdflgs, pd->devnm));
         nack -= 1; }
      pd->bdflgs = pd->bdflgs & ~BBDUsed | rdflgs & BBDUsed;
      /* Check or accept the length */
      if (rdflgs & BBDTEff && pd->ldat == 0)
         pd->ldat = len;
      else if (len != pd->ldat) {
         mexWarnMsgTxt(ssprintf(NULL,
            "Length mismatch: %6ld != %6ld for %15s\n",
            (long)len, (long)pd->ldat, pd->devnm));
         nack -= 1; }
      /* If this is an effector, read the cols and rows.  C programs
      *  can use this info or not as they wish--MATLAB programs will
      *  use it to dimension the data returned to the client.  */
      if (rdflgs & BBDTEff) {
         pd->UD.Eff.effx = rfri4(BBDcd->ndrecv);
         pd->UD.Eff.effy = rfri4(BBDcd->ndrecv);
         }
      /* Store the id code and move the device to the sorted list */
      pd->bdid = rfri2(BBDcd->ndrecv);
#ifdef DBG_BDID
      mexPrintf("bbdclist read bdid = %d for %s\n",
         (int)pd->bdid, dname);
#endif
      pack += 1;
      *ppd = pd->pndev; pd->pndev = NULL;
      *ppsdev = pd; ppsdev = &pd->pndev;
      } /* End loop over server's device list */
   /* Check that client device list is now empty */
   for (pd=pd0; pd; pd=pd->pndev) {
      mexWarnMsgTxt(ssprintf(NULL, "Name from BBD <%15s> not found "
         "on server.\n", pd->devnm));
      nack -= 1;
      }
   if (nack) pack = nack;
   rfwi4(BBDcd->ndsend, (si32)pack);
   rfflush(BBDcd->ndsend, ABORT);
   if (nack) abexitm(BBDcErrChkConf, "Configuration does not agree "
      "with that defined on server");
   } /* End bbdcclst */

/*=====================================================================*
*                               bbdcchk                                *
*=====================================================================*/

void mexFunction(int nlhs, mxArray *plhs[],
      int nrhs, const mxArray *prhs[]) {

   struct BBDComData *BBDcd;
#ifndef OSX
   struct timespec semtime;
#endif
   ui16   initcode;           /* Initial handshake code */

/* Standard MATLAB check for proper number of arguments,
*  then locate BBDComData */

   if (nlhs != 0 || nrhs != 1)
      abexitm(BBDcErrMexCall, "Bad arguments to bbdcchk");
   BBDcd = (struct BBDComData *)mxGetData(prhs[0]);
   if (!BBDcd)
      abexitm(BBDcErrMexCall, "Bad BBDcd arg to bbdcchk");

/* Check whether a subprocess terminated, typically because of a bad
*  CNS input file, but also because the fork/execve mechanism failed
*  (executable not found, etc.).  Both sub-processes must have posted,
*  meaning either they are running OK or their execve failed, before
*  we can go ahead.  This should happen fairly quickly, so to avoid
*  an infinite wait loop if something goes wrong, we use a timed wait
*  for the semaphore and report an abexit if more than 5 sec.  But we
*  still have to repeat the wait on an EINTR error.  */

   /* Check the bbdcminp (CNS input) process */
#ifndef OSX
   if (clock_gettime(CLOCK_REALTIME, &semtime) < 0)
      abexitme(BBDcErrSema, "clock_gettime fails");
   semtime.tv_sec += BAD_CHILD_WAIT_TIME;
#endif
   while (1) {
#ifdef OSX
      alarm(BAD_CHILD_WAIT_TIME);
      if (sem_wait(BBDcd->pMinpSem) == 0) break;
#else
      if (sem_timedwait(BBDcd->pMinpSem, &semtime) == 0) break;
#endif
      if (errno != EINTR)
         abexitme(BBDcErrSema, "Error waiting on " MinpSemNm);
      }

   /* Check the bbdcmlog (CNS output) process */
#ifndef OSX
   if (clock_gettime(CLOCK_REALTIME, &semtime) < 0)
      abexitme(BBDcErrSema, "clock_gettime fails");
   semtime.tv_sec += BAD_CHILD_WAIT_TIME;
#endif
   while (1) {
#ifdef OSX
      alarm(BAD_CHILD_WAIT_TIME);
      if (sem_wait(BBDcd->pMinpSem) == 0) break;
#else
      if (sem_timedwait(BBDcd->pMlogSem, &semtime) == 0) break;
#endif
      if (errno != EINTR)
         abexitme(BBDcErrSema, "Error waiting on " MlogSemNm);
      }

   /* In case CNS dies before we get here */
   if (BBDcd->Mlogrc > 0)
      abexitm(BBDcErrServer, ssprintf(NULL, "Server (CNS) terminated "
         "with abexit code %5d", BBDcd->Mlogrc));

/* Accept socket connection back from server.  Block until server has
*  something to send us.  (Server may connect with multiple BBD hosts,
*  but one BBD host can only connect with one server.)  If CNS fails,
*  for example, due to a bad input file, bbdsquit is supposed to send
*  us a BBDLC_TERM.  So now if the open fails, it is an actual problem
*  with the socket.  */

   rfopen(BBDcd->ndrecv, NULL, SAME, SAME, LISTENER, SAME, SAME,
      SAME, SAME, SAME, IGNORE, SAME, ABORT);
   initcode = rfri2(BBDcd->ndrecv);
   if (initcode == BBDLC_TERM) {
      int svrc = rfri4(BBDcd->ndrecv);
      abexitm(BBDcErrServer, ssprintf(NULL, "Server (CNS) terminated "
         "with abexit code %5d", svrc));
      }

/* Make a duplicate RFdef for writing */

   BBDcd->ndsend = rfdups(BBDcd->ndrecv, 8192, ABORT);

/* Check each list that is nonempty */

   if (BBDcd->pfsns) bbdcclst(BBDcd, &BBDcd->pfsns);
   if (BBDcd->pfttv) bbdcclst(BBDcd, &BBDcd->pfttv);
   if (BBDcd->pfetv) bbdcclst(BBDcd, &BBDcd->pfetv);
   if (BBDcd->pfstv) bbdcclst(BBDcd, &BBDcd->pfstv);
   if (BBDcd->pfval) bbdcclst(BBDcd, &BBDcd->pfval);
   if (BBDcd->pfeff) bbdcclst(BBDcd, &BBDcd->pfeff);

   return;

   } /* End bbdcchk() */
