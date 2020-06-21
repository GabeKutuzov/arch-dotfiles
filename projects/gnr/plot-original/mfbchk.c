/* (c) Copyright 1993-2015, The Rockefeller University *11115* */
/* $Id: mfbchk.c 20 2011-12-24 02:59:03Z  $ */
/***********************************************************************
*                     ROCKS metafile plot library                      *
*                              mfbchk()                                *
*                                                                      *
*  SYNOPSIS:                                                           *
*     int mfbchk(int nbits)                                            *
*     int mfhchk(int nbits)                                            *
*                                                                      *
*  DESCRIPTION:                                                        *
*  mfbchk() should be called on a parallel computer every time a       *
*  graphics routine is about to place data in the metafile command     *
*  buffer.  This routine first checks whether any data have yet been   *
*  entered in the buffer, and if not, calls mfstate to insert the      *
*  necessary state information.  (This avoids sending any data at all  *
*  if a node has no graphics calls in a particular frame.  Then,       *
*  mfbchk checks whether 'nbits' bits will fit in the current buffer.  *
*  If so, it simply returns.  If not, it adds a byte alignment (NOP)   *
*  command, writes the buffer, and tries again.  If the data still     *
*  will not fit, then the buffer is too small and mfckerr is called.   *
*                                                                      *
*  mfhchk() is used by mfhead() and mfstart() to check buffer space    *
*  while preparing header and start records, respectively.  This       *
*  routine is similar to mfbchk, except it does not insert state       *
*  information when starting a new record.                             *
*                                                                      *
*  Both calls return without doing anything if called on a serial      *
*  computer.                                                           *
*                                                                      *
*  Note:  If a record ends with a call to mfnxtbb() and some aligned   *
*  text, there is no need to include the skip bits in 'nbits' because  *
*  they would fit in the last byte if shifted after the text.          *
*                                                                      *
*  RETURN VALUES:                                                      *
*     NEWBUFF(1) if a new buffer just started, otherwise               *
*     OLDBUFF(0) if continuing current buffer.                         *
************************************************************************
*  V1A, 07/15/93, GNR - Rewritten based on roz MFReset routine         *
*  Rev, 08/01/97, GNR - Do not leave space in byte 0 for buffer cmd    *
*  Rev, 10/11/11, GNR - _NCG to _RKG, eliminate gmf.h                  *
*  V2A, 05/24/15, GNR - Updates for binary metafile                    *
*  ==>, 05/24/15, GNR - Last mod before committing to svn repository   *
***********************************************************************/

#include "mfint.h"
#include "mfio.h"

/***********************************************************************
*                               mfbchk                                 *
***********************************************************************/

int mfbchk(int nbits) {

#ifdef PAR

   Win *pw = _RKG.pcw;     /* Locate current window info */

   /* If this is a new buffer, insert state info.  Note:  do not
   *  check WFF_Atomic here, as this may be first segment of an
   *  atomic command sequence.  */
   if (pw->MFCP == pw->MFCmdBuff) mfstate();

   /* If requested data will fit in current buffer, just return */
   if (nbits < pw->MFBrem) return OLDBUFF;

   /* Data do not fit.  Call mfflush() to add an alignment record,
   *  flushe the buffer, and put state info in the new buffer.  */
   mfflush();
   /* If record just written was not part of an atomic sequence,
   *  insert state information in new buffer.  */
   if (!(pw->WinFlags & WFF_Atomic)) mfstate();

   /* Now if data will fit, all is well, otherwise error */
   if (nbits < pw->MFBrem) return NEWBUFF;
   mfckerr(ERR_BUFB);
#ifndef GCC
   return (-1);   /* Can't get here--eliminate warning */
#endif

#else /* Not PAR */
   return OLDBUFF;      /* Not an error if serial call */
#endif

   } /* End mfbchk() */

/***********************************************************************
*                               mfhchk                                 *
***********************************************************************/

int mfhchk(int nbits) {

#ifdef PAR

   Win *pw = _RKG.pcw;     /* Locate current window info */

   if (nbits < pw->MFBrem) return OLDBUFF;
   mfflush();
   if (nbits < pw->MFBrem) return NEWBUFF;
   mfckerr(ERR_BUFB);
#ifndef GCC
   return (-1);   /* Can't get here--eliminate warning */
#endif

#else /* Not PAR */
   return OLDBUFF;
#endif

   } /* End mfhchk() */
