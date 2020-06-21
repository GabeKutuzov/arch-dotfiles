/* (c) Copyright 1992-2016, The Rockefeller University *21115* */
/* $Id: mfsynch.c 6 2008-03-17 16:19:51Z  $ */
/***********************************************************************
*                     ROCKS metafile plot library                      *
*                              mfsynch()                               *
*                                                                      *
* SYNOPSIS:                                                            *
*  int mfsynch(int ksyn)                                               *
*                                                                      *
* DESCRIPTION:                                                         *
*  mfsynch synchronizes metafile graphics sent by multiple nodes.      *
*  It performs one or more of four actions, depending on the ksyn      *
*  argument and the _RKG.MFPending and _RKG.MFFlags flags:             *
*  (1) If KSYN_COMPLT is set and MFB_COMPLT was not already sent for   *
*      this frame (which is indicated by WFF_COMPLT being FALSE),      *
*      insert the MFB_COMPLT bit in the count word for the current     *
*      buffer in the current window                                    *
*  (2) If KSYN_FLUSH is true, send the count word and any data if the  *
*      count is nonzero to mfsr or mfdraw.                             *
*  (3) If KSYN_SYNC is set, perform an "incoming" synch to be sure     *
*      that all nodes have completed processing of the present frame.  *
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
*  (5) If KSYN_BCMB is set, broadcast the mesgbutt structure just      *
*      received (or a dummy if this is the first frame) to all nodes.  *
*      This constitutes an "outgoing" synch, which is needed at the    *
*      start of a new frame to assure that comp nodes do not write to  *
*      the graphics host until the new plot message (first frame) or   *
*      MFB_COMPLT (later frames) has been sent and processed.  This    *
*      also assures that no node continues processing if the window    *
*      has been closed, etc.  (In earlier versions, this synch was     *
*      accomplished by a blkbcst() in newplt.)                         *
*                                                                      *
*  On a serial computer, KSYN_SYNC and KSYN_BCMB are simply ignored.   *
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
*  Rev, 02/09/94,  LC - Revised ack handling                           *
*  Rev, 05/29/94, GNR - Use mpitools swap support                      *
*  Rev, 11/01/94,  LC - Put conditional around butt read in serial     *
*  Rev, 08/05/97, GNR - Replace "op code" with MFCountWord             *
*  Rev, 08/13/97, GNR - Check for ECONNRESET, treat as a clean finish  *
*  Rev, 04/01/98, GNR - Add code to detect error strings on socket     *
*  Rev, 10/26/05, GNR - Revised handling of socket read errors         *
*  Rev, 02/27/08, GNR - Rewrite, change nn arg to ksyn                 *
*  Rev, 03/16/08, GNR - Correct bug, reading buttmsg when SGX inactive *
*  Rev, 04/04/08, GNR - Correct logic for when select() is performed   *
*  Rev, 12/04/08, GNR - Better end-of-line checks for mfdraw error msgs*
*  Rev, 12/05/08, GNR - Avoid exit loop--error calls abexitm, app calls*
*                       endplt(), socket is already dead.              *
*  Rev, 06/18/09, GNR - Add select() in error msg loop w/timeout       *
*  Rev, 10/11/11, GNR - _NCG to _RKG, gmf.h eliminated, etc.           *
*  V2B, 05/24/15, GNR - Merge old and new versions                     *
*  Rev, 07/25/15, GNR - Add KSYN_BCMB call type                        *
*  ==>, 08/05/16, GNR - Last mod before committing to svn repository   *
***********************************************************************/

#include "sysdef.h"
#include <stddef.h>
#include <stdio.h>
#ifdef UNIX
#include <unistd.h>
#include <sys/time.h>
#endif
#include <errno.h>
#include "mfint.h"
#include "mfio.h"
#ifdef PAR
#include "mpitools.h"
#endif

#define SZBM sizeof(struct butt)

int mfsynch(int ksyn) {

   Win *pw = _RKG.pcw;     /* Access current window */
   int mbwin;              /* Window from which msgbutt received */

#ifndef PAR
#ifdef UNIX
   fd_set rdmfd;           /* Set for read select */
   struct timeval polltv;  /* Zero time value for polling */
#endif
   char *pmsg,*qmsg;
   int rrc, nbr;
#endif

/* Just return if mfdraw already terminated */

   if (_RKG.s.MFMode == 0) return 0;

/* Insert MFD_COMPLT bit if requested and not already sent */

   if (ksyn & KSYN_COMPLT && !(pw->WinFlags & WFF_COMPLT)) {
      pw->MFCountWord |= MFB_COMPLT;
      pw->WinFlags |= WFF_COMPLT; }

/* Write the last buffer, unless empty and no flags */

   if (ksyn & (KSYN_FLUSH|KSYN_COMPLT) &&
      (pw->MFCountWord || pw->MFBrem < pw->MFBrem0))
         mfflush();

/* If global synch requested on parallel system, perform it now */

#ifdef PAR
   if (ksyn & KSYN_SYNC)
      isynch();
#endif

/* If SGX inactive, return now */

   if (!(_RKG.s.MFMode & SGX)) return 0;

/* In a parallel system, get ack from mfsr.  If KSYN_WAIT is not set
*  or no frame is pending, and there is no message waiting, can just
*  return existing msgbutt contents.  Otherwise, read and return a
*  message after subtracting 'next' from MFPending count.  The ack
*  may not be for the current window.  */

#ifdef PAR0
   if (ksyn & KSYN_WAIT ||
         (ksyn & KSYN_PWAIT && _RKG.MFPending > 0)) {
      AN_Status rstat;
      int src = NC.mfsrid;
      int type = METBUTT_MSG;
      int flag;
      if (MPI_Iprobe(src, type, NC.commmf, &flag, &rstat) != 0)
         mfckerr(ERR_PROBE);
      if (flag) {
         if (MPI_Recv(&_RKG.msgbutt, SZBM, MPI_UNSIGNED_CHAR,
            src, type, NC.commmf, &rstat) != 0) mfckerr(ERR_BUTT);
         _RKG.MFPending -= _RKG.msgbutt.next;
         if (_RKG.MFPending < 0) mfckerr(ERR_TMMB);
         mbwin = _RKG.msgbutt.gwin;
         if (mbwin > _RKG.nwallo ||
            _RKG.pW0[mbwin].WinFlags & WFF_UClosed) mfckerr(ERR_BWIN);
         _RKG.pW0[mbwin].movie_mode = _RKG.msgbutt.movie_mode;
         }
      }
#endif

/* In a serial system, get ack from mfdraw.  Logic is similar to
*  that given above for the parallel case.  If this is a UNIX
*  system, use select() to determine whether any input is in
*  the read socket.  */

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
   pmsg = (char *)&_RKG.msgbutt;
   nbr = SZBM;
   while (nbr > 0) {       /* Read msgbutt, maybe in pieces */
      rrc = read(_RKG.MFDsock, pmsg, nbr);
      if (rrc < 0 && errno == ECONNRESET) {
         _RKG.MFFlags |= MFF_MFDrawX;
         _RKG.msgbutt.intxeq = BUT_QUIT;
         goto NoAckReturn; }
      else if (rrc <= 0) {
         _RKG.MFFlags |= MFF_MFDrawX;
         abexitme(213, "Error reading mfdraw socket"); }
      pmsg += rrc, nbr -= rrc;
      }
   if (_RKG.msgbutt.movie_mode > MM_BATCH) { /* Check for error msg */
      char etxt0[128];
      memcpy(etxt0, (char *)&_RKG.msgbutt, SZBM);
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
   _RKG.MFPending -= _RKG.msgbutt.next;
   if (_RKG.MFPending < 0)
      abexitm(218, "More acks than frames");
   mbwin = _RKG.msgbutt.gwin;
   if (mbwin > _RKG.nwallo || _RKG.pW0[mbwin].WinFlags & WFF_UClosed)
      abexitm(235, "Butt press msg rcvd for nonexistent window");
   _RKG.pW0[mbwin].movie_mode = _RKG.msgbutt.movie_mode;

NoAckReturn: ;

#else  /* Is PAR */
   if (_RKG.s.dbgmask & MFDBG_SYNCH) {
      dbgprt(ssprintf(NULL, "(mfsynch) node = %d\n", NC.node));
      }

   /* Broadcast button message if requested */
   if (ksyn & KSYN_BCMB && _RKG.s.numnodes > 0) {
#ifdef USE_MPI
      MPI_Bcast(&_RKG.msgbutt, SZBM, MPI_UNSIGNED_CHAR,
         NC.hostid, NC.commc);
#else
      blkbcst(&_RKG.msgbutt, NULL, SZBM, MFBCBUT_MSG);
#endif
      } /* End if KSYN_BCMB and multinode */
#endif

   return _RKG.msgbutt.intxeq & (BUT_INTX|BUT_QUIT);

   } /* End mfsynch() */

