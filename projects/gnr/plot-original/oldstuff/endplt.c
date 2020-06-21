/* (c) Copyright 1990-2016, The Rockefeller University *21114* */
/* $Id: endplt.c 29 2017-01-03 22:07:40Z  $ */
/***********************************************************************
*                         endplt(), finplt()                           *
*                                                                      *
*  SYNOPSIS:                                                           *
*     int finplt(void)                                                 *
*                                                                      *
*  DESCRIPTION:                                                        *
*     An application can call this routine to indicate that a plot     *
*     frame is complete but it does not wish to call newplt at this    *
*     time.  In a stretch of computation when no plotting is being     *
*     done, finplt() can be called at intervals to check whether a     *
*     button message has been returned.  finplt() is provided as a     *
*     wrapper for mfsynch in order to hide the details of the call     *
*     argument used internally.                                        *
*                                                                      *
*  RETURN VALUES:                                                      *
*     0  All is well                                                   *
*     1  (BUT_INTX) Interrupt button pushed.                           *
*     2  (BUT_QUIT) QUIT button pushed.                                *
*                                                                      *
*----------------------------------------------------------------------*
*                                                                      *
*  SYNOPSIS:                                                           *
*     int endplt(void)                                                 *
*                                                                      *
*  DESCRIPTION:                                                        *
*     Close ROCKS plotting package.  Perform whatever steps are        *
*     necessary to complete the last plot, close files, and free       *
*     buffers.  This version contains merged code for parallel,        *
*     serial, host, and metafile graphics.  It is compiled into        *
*     separate node0 and noden versions stored in plot0.lib and        *
*     plotn.lib, respectively.                                         *
*                                                                      *
*  RETURN VALUES:                                                      *
*     0  All is well                                                   *
*     1  (BUT_INTX) Interrupt button pushed.                           *
*     2  (BUT_QUIT) QUIT button pushed.                                *
************************************************************************
*  Rev, 10/04/90, JMS, GNR - Broken into separate function             *
*  Rev, 06/19/92, ROZ - Add NSI graphic metafile support               *
*  Rev, 07/02/93, ABP - Change SUN4 conditionals to PAR                *
*  Rev, 07/16/93, GNR - Remove snapshot support, clean up              *
*  Rev, 05/29/94, GNR - Use nsitools for swap support                  *
*  Rev, 11/25/96, GNR - Delete native NCUBE plotting support           *
*  Rev, 02/27/08, GNR - Add finplt, mfsynch(ksyn)                      *
*  ==>, 02/29/08, GNR - Last mod before committing to svn repository   *
*  Rev, 10/11/11, GNR - _NCG to _RKG, gmf.h now in glu.h, etc.         *
*  Rev, 08/11/16, GNR - No incoming synch, mfsr now counts MFB_CLOSEs  *
***********************************************************************/

#include "plots.h"
#include "glu.h"
#include "mpitools.h"
#ifdef PAR
#include "memshare.h"
#include "swap.h"
#endif

/*=====================================================================*
*                               finplt                                 *
*=====================================================================*/

int finplt(void) {

   int rc = 0;
#if defined(DEBUG) && DEBUG & MFDBG_FINPL
   dbgprt("finplt() called");
#endif
   if (_RKG.s.MFMode) rc = mfsynch(KSYN_COMPLT|KSYN_FLUSH);
   _RKG.msgbutt.intxeq = 0;
   return rc;
   } /* End finplt() */

/*=====================================================================*
*                               endplt                                 *
*=====================================================================*/

int endplt(void) {

   int rc = 0;    /* Return code */

#if defined(DEBUG) && DEBUG & MFDBG_FINPL
   dbgprt("endplt() called");
#endif

/* If a command buffer does not exist, it means MFCreate never
*  returned to newplt the first time.  There has been a startup
*  failure, and there is nothing to do here.  */

   if (!_RKG.MFCmdBuff || !_RKG.s.MFMode) return rc;

/* Wait for all nodes to complete and display metafile drawing */

   mfsynch(KSYN_COMPLT|KSYN_FLUSH|KSYN_SYNC|KSYN_PWAIT);

/* Get button return code applicable to this graph */

   rc = bpause();

/* Send button return code to all nodes.  This also serves as
*  synchronization point allowing comp nodes to continue.  */

#ifdef PAR
   if (IAmMulti) {            /* Must be PAR at run time */
      char crc[FMJSIZE];
      lemfmi4(crc, (ui32)rc);
#if defined(DEBUG) && DEBUG & MFDBG_FINPL
      dbgprt("  Broadcasting return code from bpause");
#endif
#ifdef USE_MPI
      MPI_Bcast(crc, FMJSIZE, MPI_UNSIGNED_CHAR, NC.hostid, NC.commc);
#else
      nxtupush(OPLTTT, OPLTUT);
      blkbcst(crc, NULL, FMJSIZE, MFSYNCH_MSG);
      nxtupop();
#endif
      rc = lemtoi4(crc);
      } /* End if multinode */
#endif

/* Now, only after calling bpause to wait for continue button
*  if we were in still mode, can we close the host graphics.  */

#ifndef PARn
   if (_RKG.s.MFMode) {
#if defined(DEBUG) && DEBUG & MFDBG_FINPL
      dbgprt("  endplt() calling MFClose()");
#endif
      MFClose();
      }
#endif
#if defined(PAR) && defined(USE_MPI)
   MPI_Comm_disconnect(&NC.commmf);
#endif

/* Prevent doing this stuff more than once */

   _RKG.s.MFMode = _RKG.s.MFActive = 0;
   _RKG.MFFlags |= MFF_MEnded;
   _RKG.msgbutt.intxeq = 0;
   return rc;
   } /* End endplt() */
