/* (c) Copyright 1992-2017, The Rockefeller University *21114* */
/* $Id: mfsynch.c 34 2017-09-15 18:13:10Z  $ */
/***********************************************************************
*                              mfsynch()                               *
*                                                                      *
* SYNOPSIS:                                                            *
*  int mfsynch(int ksyn)                                               *
*                                                                      *
* DESCRIPTION:                                                         *
*  mfsynch synchronizes metafile graphics sent by multiple nodes.      *
*  It performs one or more of four actions, depending on the ksyn      *
*  argument and the _NCG.MFPending and _NCG.MFFlags flags:             *
*  (1) Insert the MFB_COMPLT bit in the count word if KSYN_COMPLT is   *
*      set and MFB_COMPLT was not already sent for this frame (which   *
*      is indicated by _NCG.MFFlags & MFF_SentCOMPLT being FALSE).     *
*  (2) If KSYN_FLUSH is true, send the count word and any data if the  *
*      count is nonzero to mfsr or mfdraw.                             *
*  (3) If KSYN_SYNC is set, perform an "incoming" synch to be sure     *
*      that all nodes have completed processing of the present frame.  *
*      (An "outgoing" sync is needed at the start of a new frame to    *
*      assure that comp nodes do not write to the graphics host until  *
*      the mfst message (first frame) or MFB_COMPLT (later frames)     *
*      has been sent and processed.  This is accomplished by an        *
*      MPI_Bcast() (if USE_MPI) or blkbcst() in newplt.)               *
*  (4) Check for feedback from mfsr or mfdraw (serial or PAR0).        *
*      Expect a msgbutt msg.  If KSYN_PWAIT is set and no frame is     *
*      pending, this is basically a polling operation and a select()   *
*      is issued with no wait.  But if KSYN_WAIT is set, or KSYN_PWAIT *
*      is set and a frame is pending, the wait time is indeterminate,  *
*      because the GUI might be in still mode with the user just sit-  *
*      ting there.  With no WAIT bit, the ack may be deferred (call    *
*      from mfst). When a msgbutt is received, a 1 in the 'next' byte  *
*      acknowledges receipt of the last frame and reduces the pending  *
*      count.  This message may also contain an error message or a     *
*      BUT_INTX or BUT_QUIT signal.  The state of these GUI buttons    *
*      will be stored in _NCG.msgbutt until returned to the applica-   *
*      tion by a call to finplt() or endplt().  If there is no GUI     *
*      (metafile graphics only), a dummy msgbutt will be returned      *
*      by mfsr for synchronization.  (If the OS does not support the   *
*      select() system call, mfsynch will always wait for the ack.)    *
*                                                                      *
*  On a serial computer, KSYN_SYNC is simply ignored.                  *
*  Sometimes xgl seems to insert an error message into the socket.     *
*  Code has been added to detect these and generate abend code 221.    *
*  The compile-time variable UNIX is used as a proxy for whether the   *
*      select() system call is available.  When autoconf is set up,    *
*      this should be a more sophisticated test.                       *
*                                                                      *
* RETURN VALUES:                                                       *
*  Returns button status or 0 if buttons were not canvassed.  Exits    *
*  via mfckerr (parallel) or abexitm (serial) if an error occurs.      *
************************************************************************
*  Version 1, 04/03/92, ROZ                                            *
*  Rev, 07/02/93, ABP - Change SUN4 conditional to PAR, for T/NM       *
*  Rev, 07/17/93, GNR - Implement 'nn' argument                        *
*  Rev, 02/09/94,  LC - Revised ack handling                           *
*  Rev, 05/29/94, GNR - Use nsitools swap support                      *
*  Rev, 11/01/94,  LC - Put conditional around butt read in serial     *
*  Rev, 08/05/97, GNR - Replace "op code" with MFCountWord             *
*  Rev, 08/13/97, GNR - Check for ECONNRESET, treat as a clean finish  *
*  Rev, 04/01/98, GNR - Add code to detect error strings on socket     *
*  Rev, 10/26/05, GNR - Revised handling of socket read errors         *
*  Rev, 02/27/08, GNR - Rewrite, change nn arg to ksyn                 *
*  ==>, 02/28/08, GNR - Last mod before committing to svn repository   *
*  Rev, 03/16/08, GNR - Correct bug, reading buttmsg when SGX inactive *
*  Rev, 04/04/08, GNR - Correct logic for when select() is performed   *
*  Rev, 12/04/08, GNR - Better end-of-line checks for mfdraw error msgs*
*  Rev, 12/05/08, GNR - Avoid exit loop--error calls abexitm, app calls*
*                       endplt(), socket is already dead.              *
*  Rev, 06/18/09, GNR - Add select() in error msg loop w/timeout       *
*  Rev, 10/11/11, GNR - _NCG to _RKG, gmf.h now in glu.h, etc.         *
*  Rev, 07/20/16, GNR - Changes for MPI environment                    *
*  Rev, 04/07/17, GNR - Move to ackbutt combined ack+buttons message   *
*  R32, 04/26/17, GNR - Fix bug: newplt sending finplt data 2nd time   *
*  R34, 08/01/17, GNR - Bug fix: endless wait mfckerr->abexit->endplt  *
***********************************************************************/

#include "sysdef.h"
#include <stddef.h>
#include <stdio.h>
#ifdef UNIX
#include <unistd.h>
#include <sys/time.h>
#endif
#include <errno.h>
#include "glu.h"
#include "mpitools.h"

#define SZBM 4             /* Size of MFD8 old-style button message */

int mfsynch(int ksyn) {

#ifndef PAR
#ifdef UNIX
   fd_set rdmfd;           /* Set for read select */
   struct timeval polltv;  /* Zero time value for polling */
#endif
   char *pmsg,*qmsg;
   int rrc, nbr;
#endif

/* Just return if mfdraw already terminated */

#if defined(DEBUG) && DEBUG & MFDBG_SYNCH
   dbgprt(ssprintf(NULL, "mfsynch called, ksyn = %d", ksyn));
#endif
   if (_RKG.s.MFMode == 0) return 0;

/* Insert MFB_CLOSE bit if requested */

   if (ksyn & KSYN_CLOSE) _RKG.MFCountWord |= MFB_CLOSE;

/* Insert MFD_COMPLT bit if requested.  Duplication of this is
*  now prevented by MFF_DidFinplt bit tested at newplt().  */

   if (ksyn & KSYN_COMPLT) {
      _RKG.MFCountWord |= MFB_COMPLT;
      }

#if defined(DEBUG) && DEBUG & MFDBG_SYNCH
   if (ksyn & (KSYN_COMPLT|KSYN_CLOSE)) dbgprt(ssprintf(NULL,
      "mfsynch adds CLOSE or COMPLT, CountWord = %jd,%jd",
      _RKG.MFCountWord>>28, _RKG.MFCountWord & MFB_COUNT));
#endif

/* Write the last buffer, unless empty and no flags */

   if (ksyn & (KSYN_FLUSH|KSYN_COMPLT|KSYN_CLOSE)) {
      ui32 nn = _RKG.MFCurrPos - _RKG.MFCmdBuff;
      if (_RKG.MFCountWord | nn) {
#if defined(DEBUG) && DEBUG & MFDBG_SYNCH
         dbgprt(ssprintf(NULL, "mfsynch-->mfwrite, CountWord = %jd,%jd",
            _RKG.MFCountWord>>28, _RKG.MFCountWord & MFB_COUNT));
#endif
         mfwrite((char *)_RKG.MFCmdBuff, nn);
         }
      }

/* If global synch requested on parallel system, perform it now */

#ifdef PAR
   if (ksyn & KSYN_SYNC) {
#if defined(DEBUG) && DEBUG & MFDBG_SYNCH
      dbgprt("mfsynch calling isynch()");
#endif
      isynch();
      }
#endif

/* If SGX inactive, return now */

   if (!(_RKG.s.MFActive & SGX)) return 0;
   if (_RKG.MFFlags & MFF_MFDrawX) goto NoAckReturn;

/* In a parallel system, get ack from mfsr.  If KSYN_WAIT is not set
*  or no frame is pending, and there is no message waiting, can just
*  return existing msgbutt contents.  Otherwise, read and return a
*  message after subtracting 'next' from MFPending count.
*  N.B.  If we are just doing metafile, not X graphics, MFPending
*  will always be zero.  */

#ifdef PAR0
   if (ksyn & (KSYN_WAIT|KSYN_PWAIT)) {
      AN_Status rstat;
      int src = NC.mfsrid;
      int flag;
      if (ksyn & KSYN_PWAIT && !_RKG.MFPending) {
#if defined(DEBUG) && DEBUG & MFDBG_SYNCH
         dbgprt("mfsynch calling Iprobe, MFPending == 0");
#endif
         if (MPI_Iprobe(src, METBUTT_MSG, NC.commmf, &flag,
            &rstat) != 0) mfckerr(ERR_PROBE);
#if defined(DEBUG) && DEBUG & MFDBG_SYNCH
         dbgprt(ssprintf(NULL, "Iprobe returned flag = %d", flag));
#endif
         if (!flag) goto NoAckReturn;
         }
      /* When mfdraw returns an error message with text, or a BUT_QUIT,
      *  in the parallel case, mfsr intercepts, prints the error, and
      *  terminates.  Therefore, this code can expect to receive
      *  nothing other than a regular ackb.  */
      if (MPI_Recv(&_RKG.ackb, sizeof(_RKG.ackb), MPI_UNSIGNED_CHAR,
            src, METBUTT_MSG, NC.commmf, &rstat) != 0) {
         _RKG.MFFlags |= MFF_MFDrawX;
         mfckerr(ERR_BUTT);
         }
      if (_RKG.ackb.mfdb.intxeq & BUT_DEAD)
         _RKG.MFFlags |= MFF_MFDrawX;
#if defined(DEBUG) && DEBUG & MFDBG_SYNCH
      dbgprt(ssprintf(NULL, "mfsynch got ackc,mfdb = %d,%4m, "
         "MFPending = %d", bemtoi4(_RKG.ackb.ackc), &_RKG.ackb.mfdb,
         (int)_RKG.MFPending));
#endif
      if (_RKG.MFPending < 0) mfckerr(ERR_TMMB);
      }  /* End doing WAIT or PWAIT */
#endif

/* In a serial system, get ack from mfdraw.  Logic is similar to
*  that given above for the parallel case.  If this is a UNIX
*  system, use select() to determine whether any input is in
*  the read socket.
*  N.B.  If we are just doing metafile, not X graphics, MFPending
*  will always be zero.  */

#ifndef PAR
#ifdef UNIX
   /* If mfdraw socket already dead, just return */
   if (_RKG.MFFlags & MFF_MFDrawX) goto NoAckReturn;
   /* Use select() to poll for msgbutt if wait not required */
   if (ksyn & KSYN_WAIT ||
      (ksyn & KSYN_PWAIT && _RKG.MFPending > 0)) goto WaitForAck;
   FD_ZERO(&rdmfd);
   FD_SET(_RKG.MFDsock, &rdmfd);
   polltv.tv_sec = 0;
   polltv.tv_usec = 0;
   rrc = select(_RKG.MFDsock+1, &rdmfd, NULL, NULL, &polltv);
   if (rrc < 0) {
      _RKG.MFFlags |= MFF_MFDrawX;
      abexitme(221, "Select error reading mfdraw socket"); }
   if (rrc == 0) goto NoAckReturn;
WaitForAck:
#else    /* No select() available -- UNIX just a proxy */
   if (_RKG.MFPending == 0) goto NoAckReturn;
#endif
   pmsg = (char *)&_RKG.ackb.mfdb;
   nbr = SZBM;
   while (nbr > 0) {          /* Read msgbutt, maybe in pieces */
      rrc = read(_RKG.MFDsock, pmsg, nbr);
      if (rrc < 0 && errno == ECONNRESET) {
         _RKG.MFFlags |= MFF_MFDrawX;
         _RKG.ackb.mfdb.intxeq = (BUT_QUIT|BUT_DEAD);
         goto NoAckReturn; }
      else if (rrc <= 0) {
         _RKG.MFFlags |= MFF_MFDrawX;
         abexitme(213, "Error reading mfdraw socket"); }
      pmsg += rrc, nbr -= rrc;
      }
   /* Check for error msg in place of button codes */
   if (_RKG.ackb.mfdb.movie_mode > MM_BATCH) {
      char etxt0[128];
      memcpy(etxt0, (char *)&_RKG.ackb.mfdb, SZBM);
      pmsg = etxt0 + SZBM, nbr = sizeof(etxt0) - SZBM - 1;
      while (nbr > 0) {       /* Read rest of emsg text */
#ifdef UNIX
         /* Use select to time out if read freezes */
         FD_ZERO(&rdmfd);
         FD_SET(_RKG.MFDsock, &rdmfd);
         polltv.tv_sec = 1;
         polltv.tv_usec = 0;
         rrc = select(_RKG.MFDsock+1, &rdmfd, NULL, NULL, &polltv);
         if (rrc < 0) {
            _RKG.MFFlags |= MFF_MFDrawX;
            abexitme(221, "Select error reading mfdraw errmsg"); }
         if (rrc == 0) break;
#endif
         rrc = read(_RKG.MFDsock, pmsg, nbr);
         if (rrc < 0 && errno == ECONNRESET) {
            _RKG.MFFlags |= MFF_MFDrawX;
            abexitme(213, "Error reading error msg from mfdraw"); }
         if (rrc <= 0) break;
         pmsg += rrc, nbr -= rrc;
         }
      /* Experience shows that mfdraw-smx can send an error
      *  message with added junk... so filter it a bit */
      for (qmsg=etxt0; qmsg<pmsg; ++qmsg) {
         if (*qmsg < ' ' || *qmsg > '~') break;
         }
      *qmsg = '\0';
      _RKG.MFFlags |= MFF_MFDrawX;
      abexitm(220, etxt0);
      } /* End dealing with error message */
#if defined(DEBUG) && DEBUG & MFDBG_SYNCH
      dbgprt(ssprintf(NULL, "mfsynch: MFPending decremented to %d",
         (int)_RKG.MFPending));
#endif
   if (_RKG.MFPending < 0) {
      _RKG.MFFlags |= MFF_MEnded;
      abexitm(218, "More acks than frames");
      }

#endif   /* Not PAR */

   /* Note for future work:  When mfdraw V9 becomes available, it
   *  will return new intxeq codes specifying actions such as closing
   *  a window.  Those actions should be done in bpause or maybe from
   *  here and the corresponding bits deleted when the actions are
   *  complete.  */
NoAckReturn: ;
   return (int)_RKG.ackb.mfdb.intxeq & (BUT_INTX|BUT_QUIT|BUT_DEAD);

   } /* End mfsynch */

