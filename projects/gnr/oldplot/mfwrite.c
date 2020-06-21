/* (c) Copyright 1992-2017, The Rockefeller University *21114* */
/* $Id: mfwrite.c 34 2017-09-15 18:13:10Z  $ */
/***********************************************************************
*                              mfwrite()                               *
*                                                                      *
* SYNOPSIS:                                                            *
*  void mfwrite(char *buffer, long n)                                  *
*                                                                      *
* DESCRIPTION:                                                         *
*  mfwrite drains the metafile command buffer each time the buffer is  *
*  full or in case there is a need.  Before writing the buffer it      *
*  inserts the MFCountWord with the message length and MFB bits into   *
*  space assumed to have been reserved at the start of the buffer.     *
*  Although this puts an extra burden on any code that creates a       *
*  buffer, it was deemed worth it to save fully half the I/O calls.    *
*                                                                      *
* ARGUMENTS:                                                           *
*  buffer   address of buffer to be written (must have four unused     *
*              bytes available immediately ahead of this location).    *
*  n        number of bytes to be written.                             *
*                                                                      *
* RETURN VALUES:                                                       *
*  Returns void.  Exits via mfckerr (parallel) or abexitm (serial)     *
*     if an error occurs.                                              *
*                                                                      *
* DESIGN NOTE:                                                         *
*  As of R31, we recognize that if mfdraw quits for a reason other     *
*  than BUT_QUIT (serial or parallel), we would like the app to con-   *
*  tinue until the next finplt() or newplt() call so any plot frame    *
*  in progress can be completed. In a serial computer, this means we   *
*  stop writing to mfdraw; in a parallel computer, we still write to   *
*  mfsr and it stops writing to mfdraw.  Then mfsynch learns of the    *
*  problem and the app is notified via the newplt() return code.  The  *
*  app is supposed to do any cleanup it wants, then call endplt().     *
*  In the parallel case, endplt uses mfsynch to send an MFB_CLOSE to   *
*  mfsr which finally lets it terminate.                               *
************************************************************************
*  Version 1, 05/22/92, ROZ                                            *
*  Rev, 07/18/93, GNR - Wait for ack only when ready for next write    *
*  Rev, 02/09/94,  LC - Revised ack handling                           *
*  Rev, 08/05/97, GNR - Replace "op code" with MFCountWord, eliminate  *
*                       'complt' argument and return void, defer wait  *
*                       for ack until next send is ready.              *
*  Rev, 02/29/08, GNR - Use select|antest to check for GUI buttons,    *
*                       deal with separate XG and MF frame numbers     *
*  ==>, 03/01/08, GNR - Last mod before committing to svn repository   *
*  Rev, 10/11/11, GNR - _NCG to _RKG, gmf.h now in glu.h, etc.         *
*  Rev, 07/20/16, GNR - Modify for MPI environment                     *
*  Rev, 04/07/17, GNR - Move to ackbutt combined ack+buttons message   *
*  R34, 08/01/17, GNR - Bug fix: endless wait mfckerr->abexit->endplt  *
***********************************************************************/

#include "sysdef.h"
#include <stdio.h>
#include <ctype.h>
#ifdef UNIX
#include <unistd.h>
#endif
#include "glu.h"
#include "mpitools.h"
#include "swap.h"

/*=====================================================================*
*                             MFFixFrame                               *
*                                                                      *
*  This little routine is used to replace the max frame number placed  *
*  in the buffer by mfst with the correct number when the two kinds of *
*  output have different frame numbers.                                *
*=====================================================================*/

static void MFFixFrame(ui32 ifr) {

   int isz;

   /* Get size of index field already in place */
   for (isz=1; isz<=UI32_SIZE; ++isz) {
#ifdef IBM370
      if (_RKG.MFCmdBuff[isz] < '0') break;     /* EBCDIC letter */
#else
      if (_RKG.MFCmdBuff[isz] > '9') break;     /* ASCII letter */
#endif
      }
   i2a(_RKG.MFCmdBuff+1, (int)ifr, isz, FORCESZ);
   } /* End MFFixFrame() */


/*=====================================================================*
*                               mfwrite                                *
*=====================================================================*/

void mfwrite(char *buffer, long n) {

   char *Fbuffer = buffer - FMJSIZE;
   long  Flength = n      + FMJSIZE;
#ifdef PAR
   AN_Status rstat;
   int   src=NC.mfsrid, type=METFACK_MSG;
#else
   ui32  mxfr;                /* Larger of META,XG frame numbers */
#endif

#if defined(DEBUG) && DEBUG & MFDBG_WRITE
   dbgprt(ssprintf(NULL, "mfwrite called, n = %ld", n));
#endif

#ifdef PAR

/* Parallel version communicates with mfsr */

   /* Generate and insert word count control */
   bemfmi4(Fbuffer, _RKG.MFCountWord | (ui32)n);

   if (!_RKG.s.MFActive) goto ResetBuffer;

   /* Except on the first block of a new frame, now is the time to
   *  receive and check the ack from the previous communication.  */
   if (_RKG.MFFlags & MFF_DeferAck) {
      if (MPI_Recv(&_RKG.ackb, sizeof(_RKG.ackb), MPI_UNSIGNED_CHAR,
         src, type, NC.commmf, &rstat) != 0) mfckerr(ERR_MACK);
#if defined(DEBUG) && DEBUG & MFDBG_WRITE
      dbgprt(ssprintf(NULL, "mfwrite received deferred ackc,mfdb = "
         "%d,%4m", bemtoi4(_RKG.ackb.ackc), &_RKG.ackb.mfdb));
#endif
      mfckerr(bemtoi4(_RKG.ackb.ackc));
      _RKG.MFFlags &= ~MFF_DeferAck;
      }
   else {
      /* Put the MFB_SKPXG and/or MFB_SKPMF bits into the CountWord */
      if (!(_RKG.s.MFActive & SGX))  Fbuffer[0] |= MFB_SKPXG;
      if (!(_RKG.s.MFActive & METF)) Fbuffer[0] |= MFB_SKPMF;
      }

   /* Write full data buffer */
#if defined(DEBUG) && DEBUG & MFDBG_WRITE
   dbgprt("mfwrite sending full buffer to mfsr");
#endif
   if (MPI_Send(Fbuffer, (int)Flength, MPI_UNSIGNED_CHAR,
      NC.mfsrid, METFILE_MSG, NC.commmf) != 0) mfckerr(ERR_RMFS);

   /* If we just wrote an MFB_COMPLT or MFB_INIT record, but not an
   *  MFB_CLOSE record, wait for ack now and reset DeferAck flag to
   *  prepare for next frame.  Otherwise, set DeferAck to defer
   *  reading ack.  */
   if (_RKG.MFCountWord & MFB_CLOSE) ;
   else if (_RKG.MFCountWord & (MFB_COMPLT | MFB_INIT)) {
      ui32 lackc;
      if (MPI_Recv(&_RKG.ackb, sizeof(_RKG.ackb), MPI_UNSIGNED_CHAR,
         src, type, NC.commmf, &rstat) != 0) mfckerr(ERR_MACK);
      lackc = bemtoi4(_RKG.ackb.ackc);
#if defined(DEBUG) && DEBUG & MFDBG_WRITE
      dbgprt(ssprintf(NULL, "mfwrite received non-close ack = %d,%4m",
         lackc, &_RKG.ackb.mfdb));
#endif
      mfckerr(lackc);
/* DEBUG NOTE AND LESSON LEARNED (04/11/17):
*  This code seemed to reveal a bug in gcc, which, luckily, I did not
*  report.  There was a local static variable 'DeferAck' that was set
*  here to 0 when an _RKG.ackb was received and checked, and to 1 in
*  the following 'else' if not.  However, after the mfckerr call, the
*  compiled code skipped the DeferAck = 0 line and went right to the
*  DeferAck = 1, causing the app to fail.  This was verified by com-
*  pilation with -S and inspection of the Assembler code.  To fix this
*  problem, I removed the local DeferAck and now use _RKG.MFFlags bit
*  MFF_DeferAck, which the compiler should see as external.  However,
*  the problem remained.  Turned out, I had marked mfckerr in the
*  mfio.h header file as __attribute ((noreturn)) and that of course
*  was what caused the following code to be skipped.  */
      _RKG.MFFlags &= ~MFF_DeferAck;
      }
   else
      _RKG.MFFlags |= MFF_DeferAck;

#else

/* Serial version writes metafile directly and communicates with
*  mfdraw via socket.  Detect when a start of frame record is being
*  written and replace the frame index if it is not the one passed
*  from mfst.  This may require padding on the left.
*
*  N.B.  On return from MFFixFrame, _RKG.MFCurrPos is invalid,
*  but the current code doesn't care, so it is not corrected.
*
*  N.B.  We abort here in a 32-bit environment if metafile will
*  exceed 2GB because files that big otherwise cause crashes with
*  no message.  */

   /* Ignore attempts to write if app has shut down
   *  or both output modes are disabled for this frame  */
   if (!_RKG.s.MFActive) goto ResetBuffer;

   /* Generate and insert word count control */
   bemfmi4(Fbuffer, _RKG.MFCountWord | (ui32)n);
   mxfr = max(_RKG.MFFrame, _RKG.XGFrame);
   if (_RKG.s.MFActive & METF) { /* Write to metafile */
      if (n) {                   /* Skip empty buffer flush */
#ifndef BIT64
         if ((_RKG.totcount += n) > SI32_MAX) {
            _RKG.MFFlags |= MFF_MEnded;
            abexitm(232, "Metafile size exceeds 2GB limit");
            }
#endif
         if (*buffer == '[' && _RKG.MFFrame < mxfr) {
            MFFixFrame(_RKG.MFFrame);
            /* Poison mxfr to force SGX code to restore XGFrame */
            mxfr += 1; }
         if (fwrite(buffer, n, 1, _RKG.MFfdes) != 1) {
            _RKG.MFFlags |= MFF_MEnded;
            abexitme(227, "Error writing graphics metafile");
            }
         }
      } /* End if metafile */

   /* Write to mfdraw socket unless mfdraw is already dead.
   *  See DESIGN NOTE at top regarding why this is not terminal.  */
   if (_RKG.s.MFActive & SGX && !(_RKG.MFFlags & MFF_MFDrawX)) {
      if (*buffer == '[' && _RKG.XGFrame < mxfr)
         MFFixFrame(_RKG.XGFrame);
      if (write(_RKG.MFDsock, Fbuffer, Flength) < 0) {
         _RKG.MFFlags |= (MFF_MFDrawX|MFF_MEnded);
         abexitme(222, "Error writing to mfdraw socket");
         }
      } /* End if host graphics */

#endif

   /* Reset glu for next buffer */
ResetBuffer:
   _RKG.MFCountWord = 0;
   _RKG.MFCurrPos = _RKG.MFCmdBuff;

   } /* End of mfwrite */

