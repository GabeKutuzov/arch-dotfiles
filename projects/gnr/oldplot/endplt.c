/* (c) Copyright 1990-2017, The Rockefeller University *21114* */
/* $Id: endplt.c 36 2018-05-07 22:18:50Z  $ */
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
*  RETURN VALUES (defined in plotdefs.h):                              *
*     0x00  All is well                                                *
*     0x01  (BUT_INTX) Interrupt button pushed.                        *
*     0x02  (BUT_QUIT) QUIT button pushed.                             *
*      --   [Other values reserved for new plot library]               *
*     0x80  (BUT_DEAD) mfdraw died or mfsr killed it.                  *
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
*     0x00  All is well                                                *
*     0x01  (BUT_INTX) Interrupt button pushed.                        *
*     0x02  (BUT_QUIT) QUIT button pushed.                             *
*      --   [Other values reserved for new plot library]               *
*     0x80  (BUT_DEAD) mfdraw died or mfsr killed it.                  *
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
*  Rev, 04/07/17, GNR - Move to ackbutt combined ack+buttons message   *
*  R31, 04/26/17, GNR - Fix bug: newplt sending finplt data 2nd time   *
*  R34, 08/01/17, GNR - Bug fix: endless wait mfckerr->abexit->endplt  *
*  R36, 03/27/18, GNR - Bug fix: shut down mfsr if newplt never called *
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
   _RKG.MFFlags |= MFF_DidFinplt;
   return rc;
   } /* End finplt() */

/*=====================================================================*
*                               endplt                                 *
*=====================================================================*/

int endplt(void) {

   int rc = 0;    /* Return code */
   /* Prevent infinite abexit loop on error below or at mfckerr */
   if (_RKG.MFFlags & (MFF_EndPend|MFF_MEnded)) return BUT_DEAD;
   _RKG.MFFlags |= MFF_EndPend;

#if defined(DEBUG) && DEBUG & MFDBG_FINPL
   dbgprt("endplt() called");
#endif

/* In a parallel computer, if MFF_HdrSent is not set, newplt was
*  never called (or was unsuccessful) but mfsr may have been started
*  by aninit and we must shut it down by sending a startup message
*  with the special m_doXG code 'X'.  It could be argued that this
*  code should be in appexit(), but that function should be innocent
*  of RKG graphics so it can work with non-graphic applications.  */

#if defined(PAR) && defined(USE_MPI)
   if (!(_RKG.MFFlags & MFF_ModeSet) && NC.commmf) {
#ifdef PAR0
      struct MFSRSUMsg fakestart;
      char ackbuf[FMJSIZE];
      memset(&fakestart, ' ', sizeof(struct MFSRSUMsg));
      fakestart.m_doXG[1] = 'X';
      /* Write MFSRSUMsg to mfsr.  Wait for ack.  */
      rc = MPI_Send(&fakestart, sizeof(struct MFSRSUMsg), 
         MPI_UNSIGNED_CHAR, NC.mfsrid, MFSRSUM_MSG, NC.commmf);
      if (rc) appexit("mfsr fakestart msg", 283, rc);
#if defined(DEBUG) && DEBUG & MFDBG_FINPL
      dbgprt("mfsr fakestart sent, waiting for ack");
#endif
      rc = MPI_Recv(ackbuf, FMJSIZE, MPI_UNSIGNED_CHAR, NC.mfsrid,
         MFSRACK_MSG, NC.commmf, MPI_STATUS_IGNORE);
      if (rc) appexit("mfsr fakestart ack", 283, rc);
      rc = bemtoi4(ackbuf);
#if defined(DEBUG) && DEBUG & MFDBG_FINPL
      dbgprt(ssprintf(NULL, "Got ack = %d", rc));
#endif
      if (rc) appexit("mfsr fakestart msg", 283, rc);
#endif
      MPI_Comm_disconnect(&NC.commmf);
      NC.commmf = NULL;
      goto FinishShutdown;
      }
#endif

/* If a command buffer does not exist (and this is not the mfsr
*  situation commented above), it means mfcreate never returned
*  to newplt the first time.  There has been a startup failure,
*  and there is nothing to do here.  */

   if (!_RKG.MFCmdBuff || !_RKG.s.MFMode) return rc;

/* Wait for all nodes to complete and display metafile drawing */

   /* Do not repeat sending last buffer if done at finplt */
   if (!(_RKG.MFFlags & MFF_DidFinplt))
      mfsynch(KSYN_COMPLT|KSYN_FLUSH);
   _RKG.MFFlags &= ~MFF_DidFinplt;
   mfsynch(KSYN_SYNC|KSYN_PWAIT);

/* Get button return code applicable to this graph */

   rc = bpause();

/* Send ackbutt return code to all nodes.  This also serves
*  as synchronization point allowing comp nodes to continue.  */

#ifdef PAR
   if (IAmMulti) {            /* Must be PAR at run time */
#ifdef PAR0
      bemfmi4(_RKG.ackb.ackc, _RKG.s.MFActive);
#if defined(DEBUG) && DEBUG & MFDBG_FINPL
      dbgprt(ssprintf(NULL, "   endplt about to broadcast MFActive,"
         "mfdb = %d,%4m", _RKG.s.MFActive, &_RKG.ackb.mfdb));
#endif
#endif
#ifdef USE_MPI
      MPI_Bcast(&_RKG.ackb, sizeof(_RKG.ackb), MPI_UNSIGNED_CHAR,
         NC.hostid, NC.commc);
#else
      nxtupush(OPLTTT, OPLTUT);
      blkbcst(&_RKG.ackb, NULL, sizeof(_RKG.ackb), MFSYNCH_MSG);
      nxtupop();
#endif
#ifdef PARn
      rc = (int)_RKG.ackb.mfdb.intxeq;
      _RKG.s.MFActive = bemtoi4(_RKG.ackb.ackc);
#if defined(DEBUG) && DEBUG & MFDBG_NWPLT
      dbgprt(ssprintf(NULL, "   Received MFActive,mfdb = %d,%4m",
         _RKG.s.MFActive, &_RKG.ackb.mfdb));
#endif
#endif
      if (_RKG.ackb.mfdb.intxeq & BUT_DEAD) _RKG.MFFlags |= MFF_MFDrawX;
      } /* End if multinode */
#endif
   _RKG.MFPending -= _RKG.ackb.mfdb.next;

/* Now, only after calling bpause to wait for continue button
*  if we were in still mode, can we close the host graphics.  */

#ifndef PARn
   if (_RKG.s.MFMode) {
#if defined(DEBUG) && DEBUG & MFDBG_FINPL
      dbgprt("  endplt() calling mfclose()");
#endif
      mfclose();
      }
#endif
#if defined(PAR) && defined(USE_MPI)
   MPI_Comm_disconnect(&NC.commmf);
#endif

/* Prevent doing this stuff more than once */

FinishShutdown:
   _RKG.s.MFMode = _RKG.s.MFActive = 0;
   _RKG.MFFlags |= (MFF_MEnded|MFF_MFDrawX);
   _RKG.ackb.mfdb.intxeq = BUT_DEAD;
   return rc;
   } /* End endplt() */
