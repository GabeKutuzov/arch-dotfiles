/* (c) Copyright 1992-2016, The Rockefeller University *211115* */
/* $Id: mfclose.c 5 2008-03-13 20:32:23Z  $ */
/***********************************************************************
*                     ROCKS metafile plot library                      *
*                              mfclose()                               *
*                                                                      *
* SYNOPSIS:                                                            *
*  void mfclose(void)                                                  *
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
*  Rev, 04/04/08, GNR - Add KSYN_WAIT bit to mfsynch() call            *
*  Rev, 10/11/11, GNR - _NCG to _RKG, eliminate gmf.h                  *
*  V2B, 05/24/15, GNR - Merge old and new versions                     *
*  ==>, 08/05/16, GNR - Last mod before committing to svn repository   *
***********************************************************************/

#include "mfint.h"
#include "mfio.h"
#include "swap.h"
#ifndef PARn
#include "rfdef.h"
#endif

#ifdef PARn
#error File mfclose.c should not be compiled for PARn nodes
#else

void mfclose(void) {

#ifdef PAR0
   AN_Status rstat;
   int src=NC.mfsrid, type=METFACK_MSG;
   char ack[FMJSIZE];               /* Acknowledgement buffer */
#endif

/* Generate end-metafile record, unless mfdraw already terminated */

   if (_RKG.s.MFMode) {

      /* Close any additional windows */
      int iwin;
      for (iwin=_RKG.nwused; iwin>0; --iwin) {
         if (!_RKG.pW0[iwin].WinFlags & (WFF_UClosed|WFF_SClosed)) {
            clswin(iwin);
            freev((char *)_RKG.pW0[iwin].MFFullBuff,
               "Metafile buffer");
            _RKG.pW0[iwin].WinFlags |= WFF_SClosed;
            }
         }
      chgwin(0);
#ifdef PAR
      /* mfsr will supply the ']' record from last node to report,
      *  but it needs EOR to be aligned on a byte boundary.  */
      mfalign();
#else
      /* But serial code writes directly to output--needs OpEnd */
      mfbchk(Lop);
      c2mf(OpEnd);
#endif
      _RKG.pW0[0].MFCountWord |= MFB_CLOSE;
      mfsynch(KSYN_COMPLT|KSYN_FLUSH|KSYN_WAIT);

#ifdef PAR0
/* Parallel.  Wait for mfsr to send us one final ack to transmit any
*  error codes.  This is done regardless of whether SGX mode is
*  active.  Code at this location in earlier versions waited for mfsr
*  child to terminate.  This code has been removed because it is more
*  properly the job of the driver that spawned mfsr to wait.  In a
*  non-hybrid system, PAR0 and mfsr might not be on same processor.
*/

      if (MPI_Recv(ack, FMJSIZE, MPI_UNSIGNED_CHAR, &src, &type,
         NC.commmf, &rstat) != 0) mfckerr(ERR_MACK);
      mfckerr(lemtoi4(ackm));
#endif

#ifndef PAR
/* Serial.  Close real metafile and socket connection to mfdraw.
*  Code at this location in earlier versions waited for mfdraw child
*  to terminate.  This code has been removed because mfdraw may now
*  be executing on a remote server.  */

      if (_RKG.s.MFMode & METF) {
         if (rfclose(_RKG.MFfdes, REWIND, RELEASE_BUFF, NO_ABORT) < 0)
            abexitm(226, "Error closing metafile");
         freev(_RKG.MFfn, "Metafile name");
         }
      if (_RKG.s.MFMode & SGX) {
         if (close(_RKG.MFDsock) < 0) {
            _RKG.MFFlags |= MFF_MFDrawX;
            abexitm(228, "Error closing mfdraw socket");
            }
         }
#endif

#ifndef PARn
      if (_RKG.MFDdisplay) freev(_RKG.MFDdisplay, "X Display name");
#endif
      /* Close last window and free window memory */
      clswin(0);
      freev((char *)_RKG.pW0[0].MFFullBuff, "Metafile buffer");
      freev((char *)_RKG.pW0, "Window array");
      } /* End MFMode not empty */

   if (_RKG.s.dbgmask & MFDBG_CLOSE) {
      dbgprt("mfclose finished.");
      }

   } /* End mfclose() */

#endif /* Not PARn */

