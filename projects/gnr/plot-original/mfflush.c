/* (c) Copyright 1992-2016, The Rockefeller University *211115* */
/* $Id: mfflush.c 5 2008-03-13 20:32:23Z  $ */
/***********************************************************************
*                     ROCKS metafile plot library                      *
*                              mfflush()                               *
*                                                                      *
* DESCRIPTION:                                                         *
*  Drains the current window's metafile command buffer each time the   *
*  buffer is full or in case there is a need.  Resets Win.MFBrem and   *
*  Win.MFCP so the caller can immediately begin putting data in the    *
*  next buffer.  Before writing the buffer it inserts the MFCountWord  *
*  with message length and MFB bits into space assumed to have been    *
*  reserved at the start of the buffer. Although this puts an extra    *
*  burden on any code that creates a buffer, it was deemed worth it    *
*  to save fully half the I/O calls.                                   *
*                                                                      *
* SYNOPSIS:                                                            *
*  void mfflush(void)                                                  *
*                                                                      *
* ARGUMENTS:                                                           *
*  None.  Information on current position in metafile buffer and the   *
*     number of bits remaining is contained in the _RKG global struct. *
*                                                                      *
* RETURN VALUES:                                                       *
*  Returns void.  Exits via mfckerr (parallel) or abexitm (serial)     *
*     if an error occurs.                                              *
*                                                                      *
* DESIGN NOTES:                                                        *
*  This is a renamed version of the old MFWrite.  New plot calls now   *
*  insert both metafile and graphics plot numbers in start-of-plot     *
*  records, so this code does not need to manipulate those data.       *
*  Note that messages to mfsr, like metafiles, are big-endian.         *
************************************************************************
*  MFWrite version 1, 05/22/92, ROZ                                    *
*  Rev, 07/18/93, GNR - Wait for ack only when ready for next write    *
*  Rev, 08/05/97, GNR - Replace "op code" with MFCountWord, eliminate  *
*                       'complt' argument and return void, again defer *
*                       wait for ack until next send is ready.         *
*  Rev, 05/23/99, GNR - Use new swapping scheme for handling ack.      *
*  Rev, 08/13/02, GNR - Deal with atomic sequences via MFB_ATOM bit.   *
*  Rev, 02/29/08, GNR - Use select|antest to check for GUI buttons,    *
*                       deal with separate XG and MF frame numbers     *
*  Rev, 10/11/11, GNR - _NCG to _RKG, gmf.h now in glu.h, etc.         *
*  ==>, 08/28/16, GNR - Last mod before committing to svn repository   *
***********************************************************************/

#include "sysdef.h"
#include <stdio.h>
#include <ctype.h>
#ifdef UNIX
#include <unistd.h>
#endif
#include "mfint.h"
#include "mfio.h"
#include "swap.h"
#ifdef PAR
#include "mpitools.h"
#else
#include "rfdef.h"
#endif


/*=====================================================================*
*                               mfflush                                *
*=====================================================================*/

void mfflush(void) {

   Win *pw = _RKG.pcw;        /* Access current window */
   ui32  n;                   /* Write count */
#ifdef PAR
   AN_Status rstat;
   static int DeferAck = 0;   /* TRUE after first block */
   int   src=NC.mfsrid, type=METFACK_MSG;
   char  ack[FMJSIZE];        /* Acknowledgement buffer */
#endif

#if defined(DEBUG) && DEBUG & MFDBG_WRITE
   dbgprt(ssprintf(NULL, "mfflush called", n));
#endif

   /* Ignore attempts to write if nothing in buffer or app has shut
   *  down or both output modes are disabled for this frame.  */
   if (!pw->MFActive || pw->MFCmdBuff == pw->MFCP) return;

   /* Insert alignment command if necessary.
   *  Parallel:  Any call.  This even includes atomic sequences and
   *     closing sequences so next segment can start on a new byte.
   *  Serial:  Only at end of frame (so mfdraw can start next
   *     frame memory on a byte boundary).  */
#ifdef PAR
   mfalign();
#else
   if (pw->MFCountWord & MFB_COMPLT) mfalign();
#endif

   /* Generate and insert word count control.  Count does not include
   *  length of count word.  Flag for atomic sequences must be kept
   *  in WinFlags, not just in count word, so mfbchk() can detect it
   *  after count word is cleared here.  Note that if alignment NOP
   *  extends into reserved space, MFBrem will be negative at this
   *  point.  */
   if (pw->WinFlags & WFF_Atomic) pw->MFCountWord |= MFB_ATOM;
   n = _RKG.s.MFBuffLen - ByteOffset(pw->MFBrem);
   bemfmi4(pw->MFFullBuff, pw->MFCountWord | n);

#ifdef PAR

/*---------------------------------------------------------------------*
*               Parallel mfflush communicates with mfsr                *
*---------------------------------------------------------------------*/

   /* Except on the first block of a new frame, now is the time to
   *  receive and check the ack from the previous communication.  */
   if (DeferAck) {
      if (MPI_Recv(ack, FMJSIZE, MPI_UNSIGNED_CHAR, src, type,
         NC.commmf, &rstat) != 0) mfckerr(ERR_MACK);
#if defined(DEBUG) && DEBUG & MFDBG_WRITE
      dbgprt(ssprintf(NULL, "mfflush received DeferAck = %d",
         bemtoi4(ack)));
#endif
      mfckerr(lemtoi4(ack));
      }
   else {
      /* Put the MFB_SKPXG and/or MFB_SKPMF bits into the CountWord */
      if (!(pw->MFActive & SGX))  pw->MFFullBuff[0] |= MFB_SKPXG;
      if (!(pw->MFActive & METF)) pw->MFFullBuff[0] |= MFB_SKPMF;
      }

   /* Write full data buffer, including count word */
#if defined(DEBUG) && DEBUG & MFDBG_WRITE
   dbgprt("mfflush sending full buffer to mfsr");
#endif
   if (MPI_Send(pw->MFFullBuff, n + FMJSIZE, MPI_UNSIGNED_CHAR,
      NC.mfsrid, METFILE_MSG, NC.commmf) != 0) mfckerr(ERR_RMFS);

   /* If we just wrote an MFB_COMPLT or MFB_INIT record, but not an
   *  MFB_CLOSE record, wait for ack now and reset DeferAck flag to
   *  prepare for next frame.  Otherwise, set DeferAck to defer
   *  reading ack.  */
   if (_RKG.MFCountWord & MFB_CLOSE) ;
   else if (pw->MFCountWord & (MFB_COMPLT | MFB_INIT)) {
      if (MPI_Recv(ack, FMJSIZE, MPI_UNSIGNED_CHAR, src, type,
         NC.commmf, &rstat) != 0) mfckerr(ERR_MACK);
#if defined(DEBUG) && DEBUG & MFDBG_WRITE
      dbgprt(ssprintf(NULL, "mfflush received non-close ack = %d",
         bemtoi4(ack)));
#endif
      mfckerr(lemtoi4(ack));
      DeferAck = 0;
      }
   else
      DeferAck = 1;

#else /* Serial */

/*---------------------------------------------------------------------*
*    Serial mfflush writes metafile directly and communicates with     *
*                         mfdraw via socket.                           *
*---------------------------------------------------------------------*/

   if (pw->MFActive & METF) { /* Write to metafile */
#if PSIZE < 8
      /* In the 32-bit implementation, we abort here if metafile
      *  will exceed 2GB because files that big otherwise cause
      *  crashes with no message.  */
      if ((_RKG.totcount += n) > SI32_MAX)
         abexitm(232, "Metafile size exceeds 2GB limit");
#endif
      if (rfwrite(_RKG.MFfdes, pw->MFCmdBuff, n, NO_ABORT) < 0)
         abexitme(227, "Error writing graphics metafile");
      } /* End if metafile */

   if (pw->MFActive & SGX) {  /* Write to mfdraw socket */
      if (write(_RKG.MFDsock, pw->MFFullBuff, n + FMJSIZE) < 0)
         abexitme(222, "Error writing to mfdraw socket");
      } /* End if X graphics */

#endif

   /* Reset _RKG for next buffer */
   pw->MFCP = pw->MFCmdBuff;
   pw->MFBrem = pw->MFBrem0;
   pw->MFCountWord = 0;
   _RKG.MFFlags &= ~MFF_FirstBuff;

   } /* End of mfflush() */
