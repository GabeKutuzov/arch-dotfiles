/* (c) Copyright 1992-2017, The Rockefeller University *21114* */
/* $Id: mfclose.c 34 2017-09-15 18:13:10Z  $ */
/***********************************************************************
*                              mfclose()                               *
*                                                                      *
* SYNOPSIS:                                                            *
*  void mfclose()                                                      *
*                                                                      *
* DESCRIPTION:                                                         *
*  This function closes communication with mfsr (parallel) or          *
*  mfdraw (serial) and frees allocated memory.  The count word         *
*  for the last buffer is set to contain the MFB_CLOSE bit.            *
*  Call only once and only from node 0 of a parallel computer.         *
*                                                                      *
* RETURN VALUES:                                                       *
*  Returns void.  Exits via mfckerr (parallel) or abexitm (serial)     *
*     if an error occurs.                                              *
************************************************************************
*  Version 1, 05/22/92, ROZ                                            *
*  Rev, 07/02/93, ABP - Change SUN4 conditional to PAR                 *
*  Rev, 05/29/94, GNR - Use nsitools swap support                      *
*  Rev, 07/01/97, GNR - Change kill loop to wait() call for mfdraw!    *
*  Rev, 08/05/97, GNR - Replace "op code" with MFCountWord, remove     *
*                       code that waited for mfsr to terminate, now    *
*                       instead wait for a final button message.       *
*  Rev, 04/02/98, GNR - Use mfsynch to do final wait, does error check *
*  Rev, 03/15/06, GNR - Close socket with 'close', not 'shutdown'      *
*  ==>, 02/28/08, GNR - Last mod before committing to svn repository   *
*  Rev, 04/04/08, GNR - Add KSYN_WAIT bit to mfsynch() call            *
*  Rev, 10/11/11, GNR - _NCG to _RKG, gmf.h now in glu.h, etc.         *
*  Rev, 07/20/16, GNR - Changes for MPI environment                    *
*  Rev, 04/07/17, GNR - Move to ackbutt combined ack+buttons message   *
*  R34, 08/01/17, GNR - Bug fix: endless wait mfckerr->abexit->endplt  *
***********************************************************************/

#include "glu.h"
#include "rksubs.h"
#include "swap.h"
#include "mpitools.h"

#ifdef PARn
#error Do not compile mfclose for a PARn node
#endif

#define LEN_ENDREC 2

void mfclose(void) {

#ifdef PAR0
   AN_Status rstat;
#endif

/* Generate end-metafile record */

   if (_RKG.s.MFMode) {

#if defined(DEBUG) && DEBUG & MFDBG_CLOSE
      dbgprt("mfclose creating ']' record");
#endif
      mfbchk(LEN_ENDREC);
      *_RKG.MFCurrPos++ = ']';
      *_RKG.MFCurrPos++ = '\n';
#if defined(DEBUG) && DEBUG & MFDBG_CLOSE
      dbgprt("mfclose calling mfsynch - COMPLT|CLOSE|FLUSH|WAIT");
#endif
      mfsynch(KSYN_COMPLT|KSYN_CLOSE|KSYN_FLUSH|KSYN_WAIT);

#ifdef PAR0
/* Parallel.  Wait for mfsr to send us one final ack to transmit any
*  error codes.  This is done regardless of whether SGX mode is
*  active.  Code at this location in earlier versions waited for mfsr
*  child to terminate.  This code has been removed because it is more
*  properly the job of the driver that spawned mfsr to wait.  In a
*  non-hybrid system, PAR0 and mfsr might not be on same processor.
*/

#if defined(DEBUG) && DEBUG & MFDBG_CLOSE
      dbgprt("mfclose waiting for mfsr ack");
#endif
      if (MPI_Recv(&_RKG.ackb, sizeof(_RKG.ackb), MPI_UNSIGNED_CHAR,
            NC.mfsrid, METFACK_MSG, NC.commmf, &rstat) != 0)
         mfckerr(ERR_MACK);
#if defined(DEBUG) && DEBUG & MFDBG_CLOSE
      dbgprt(ssprintf(NULL, "mfclose got ackc = %d",
         bemtoi4(_RKG.ackb.ackc)));
#endif
      mfckerr(bemtoi4(_RKG.ackb.ackc));
#endif

#ifndef PAR
/* Serial.  Close real metafile and socket connection to mfdraw.
*  Code at this location in earlier versions waited for mfdraw child
*  to terminate.  This code has been removed because mfdraw may now
*  be executing on a remote server.  */

      if (_RKG.s.MFMode & METF) {
         if (fclose(_RKG.MFfdes) < 0)
            abexitm(226, "Error closing graphics metafile");
         freev(_RKG.MFfn, "Metafile name");
         }
      if (_RKG.s.MFMode & SGX) {
         if (close(_RKG.MFDsock) < 0) {
            _RKG.MFFlags |= (MFF_MFDrawX|MFF_MEnded);
            abexitm(228, "Error closing socket to mfdraw"); }
         }
#endif

      if (_RKG.MFDdisplay) freev(_RKG.MFDdisplay, "X Display");
      freev((char *)_RKG.MFFullBuff, "MF Buffer");

#if defined(DEBUG) && DEBUG & MFDBG_CLOSE
      dbgprt("mfclose finished.\n");
#endif
      }

   } /* End mfclose() */

