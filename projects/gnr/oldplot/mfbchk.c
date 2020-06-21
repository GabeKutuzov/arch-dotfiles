/* (c) Copyright 1993-2011, The Rockefeller University *21114* */
/* $Id: mfbchk.c 34 2017-09-15 18:13:10Z  $ */
/***********************************************************************
*                              mfbchk()                                *
*                                                                      *
*  SYNOPSIS:                                                           *
*     int mfbchk(int len)                                              *
*     int mfhchk(int len)                                              *
*                                                                      *
*  DESCRIPTION:                                                        *
*  mfbchk should be called every time a graphics routine is about      *
*  to place data in the metafile command buffer.  This routine first   *
*  checks whether any data have yet been entered in the buffer, and    *
*  if not, calls mfstate to insert the necessary state information.    *
*  (This avoids sending any data at all if a node has no graphics      *
*  calls in a particular frame.  If compiled for serial operation,     *
*  this step is skipped.  Instead, newplt() just writes the state      *
*  information immediately.)  Then, mfbchk checks whether 'len' bytes  *
*  will fit in the current buffer.  If so, it simply returns.  If not, *
*  it writes the buffer, inserts the state information, and tries      *
*  again.  If the data still will not fit, then the buffer is too      *
*  small and an mfckerr is called.                                     *
*                                                                      *
*  mfhchk is used by mfhead and mfst to check buffer space while       *
*  preparing header and start records, respectively.  This routine     *
*  is similar to mfbchk, except it does not insert state information   *
*  when starting a new record.                                         *
*                                                                      *
*  RETURN VALUES:                                                      *
*     NEWBUFF(0) if a new buffer just started, otherwise               *
*     OLDBUFF(1) if continuing current buffer.                         *
*  (If NEWBUFF is returned, callers that are setting new values of     *
*  state variables do not need to store their commands.)               *
************************************************************************
*  V1A, 07/15/93, GNR - Rewritten based on roz MFReset routine         *
*  Rev, 08/01/97, GNR - Do not leave space in byte 0 for buffer cmd    *
*  ==>, 01/25/07, GNR - Last mod before committing to svn repository   *
*  Rev, 10/11/11, GNR - _NCG to _RKG, gmf.h now in glu.h, etc.         *
***********************************************************************/

#include "glu.h"
#include "mpitools.h"
#include "rocks.h"
#include "rkxtra.h"

/***********************************************************************
*                               mfbchk                                 *
***********************************************************************/

int mfbchk(int len) {

   /* Check whether state information is already in buffer */
   if (_RKG.MFCurrPos > _RKG.MFCmdBuff) {

      /* Yes, state information is already in buffer.
      *  If requested data will fit, just return.
      *  Otherwise, start a new buffer and try again.  */
      if (_RKG.MFCurrPos + len < _RKG.MFTopPos) return OLDBUFF;
      mfwrite((char *)_RKG.MFCmdBuff, _RKG.MFCurrPos-_RKG.MFCmdBuff);
      }

   /* New buffer.  Insert initial state information if parallel.
   *  Requested data must fit in remainder or terminate.  */
#ifdef PAR
   if (IAmMulti) mfstate();
#endif
   if (_RKG.MFCurrPos + len < _RKG.MFTopPos) return NEWBUFF;

   /* Data will not fit in a new buffer.  Terminate with mfckerr.
   *  This problem could be avoided by a redesign in which commands
   *  can be split across buffers.  This should at least be done
   *  by bitmap(), bitmaps(), but currently is not.  */
   mfckerr(ERR_BUFB);
   return (-1);   /* Can't get here--eliminate warning */

   } /* End mfbchk */

/***********************************************************************
*                               mfhchk                                 *
***********************************************************************/

int mfhchk(int len) {

   /* If requested data will fit in buffer, just return.
   *  Otherwise, start a new buffer and try again.  */
   if (_RKG.MFCurrPos + len < _RKG.MFTopPos) return OLDBUFF;
   mfwrite((char *)_RKG.MFCmdBuff, _RKG.MFCurrPos-_RKG.MFCmdBuff);

   /* New buffer.  Requested data must fit or terminate.  */
   if (_RKG.MFCurrPos + len < _RKG.MFTopPos) return NEWBUFF;

   /* Data will not fit in a new buffer.  Terminate as in mfbchk. */
   mfckerr(ERR_BUFB);
   return (-1);   /* Can't get here--eliminate warning */

   } /* End mfhchk */
