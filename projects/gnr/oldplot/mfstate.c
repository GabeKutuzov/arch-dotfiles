/* (c) Copyright 1993-2013, The Rockefeller University *21114* */
/* $Id: mfstate.c 30 2017-01-16 19:30:14Z  $ */
/***********************************************************************
*                              mfstate()                               *
*                                                                      *
*  SYNOPSIS:                                                           *
*     void mfstate()                                                   *
*                                                                      *
*  DESCRIPTION:                                                        *
*     mfstate writes the current state of the graphics virtual machine *
*     to the metafile buffer in the form of metafile commands.  This   *
*     must be done at the start of each buffer in parallel versions    *
*     because the state may be changed by buffers from another node.   *
*     Otherwise, it is done only when the first buffer is written for  *
*     each frame.                                                      *
*                                                                      *
*     This routine does not have to check whether the commands will    *
*     fit in the buffer, because it is called only at the start of a   *
*     new buffer.  However, as a safety check, this is done anyway.    *
*                                                                      *
*     When the entire graphics package has been implemented as speci-  *
*     fied, the state will consist of:                                 *
*        color, scale, thickness, font, origin, position, gmode        *
*     In the present version, only the following are relevant:         *
*        color, thickness, position                                    *
*                                                                      *
*  RETURN VALUES:                                                      *
*     None.                                                            *
************************************************************************
*  V1A, 07/15/93, GNR - Initial version                                *
*  Rev, 12/20/93,  LC - Added Pentype state                            *
*  Rev, 01/03/94   LC - 'I', 'K' switch for currcol                    *
*  ==>, 01/25/07, GNR - Last mod before committing to svn repository   *
*  Rev, 10/11/11, GNR - _NCG to _RKG, gmf.h now in glu.h, etc.         *
*  Rev, 03/16/13, GNR - Fix bug in way color code was forced           *
***********************************************************************/

#include "glu.h"
#include "rocks.h"
#include "rkxtra.h"

/* Length of state commands is:
*  1 + 2*(2+lcf) + 1             for 'move' command
*  1 + 1 + 1                     for 'retrace' command
*  1 + (lci+3)/4 + 1             for 'color' command, 'I' OR
*  1 + 8 + 1                     for 'color' command, 'K'
*  1 + 8 + 1                     for 'type' command, not implemented
*  Note that due to incorrect implementation in mfdraw,
*  the color index is currently stored as a decimal integer,
*  not a hexadecimal integer as specified in the writeup.
*  Accordingly, the term (lci+3)/4 must be replaced by the
*  number of decimal digits needed to represent lci bits.  */

/* If new state variables are added, the definition of LEN_STATECMDS
   in glu.h must be updated */

void mfstate(void) {

   int   len;

   /* Check space in buffer */
   if (_RKG.MFCurrPos + LEN_STATE_CMDS >= _RKG.MFTopPos)
      mfckerr(ERR_BUFB);

   /* Insert 'move' command */
   *_RKG.MFCurrPos++ = 'M';
   i2a(_RKG.MFCurrPos,(int)(_RKG.xcurr*1000.0),5,DECENCD);
   i2a(_RKG.MFCurrPos,(int)(_RKG.ycurr*1000.0),5,DECENCD);
   *_RKG.MFCurrPos++ = '\n';

   /* Insert 'retrace' command */
   *_RKG.MFCurrPos++ = 'H';
   i2a(_RKG.MFCurrPos,_RKG.curretrac,1,DECENCD);
   *_RKG.MFCurrPos++ = '\n';

   /* Insert 'color' command */
   if (_RKG.currcolType == 'I') {        /* color index */
      *_RKG.MFCurrPos++ = 'I';
      i2a(_RKG.MFCurrPos,_RKG.currcol,3,DECENCD);
      *_RKG.MFCurrPos++ = '\n';
      }
   else {                               /* color string */
      *_RKG.MFCurrPos++ = 'K';
      len = strnlen(_RKG.currcolStr,8);
      i2a(_RKG.MFCurrPos, len, 1, DECENCD);
      memcpy((char *) _RKG.MFCurrPos, _RKG.currcolStr, len);
      _RKG.MFCurrPos+=len;
      *_RKG.MFCurrPos++ = '\n';
      _RKG.currcolType = 'K';    /* (Might have been 'J' here) */
      }

   /* Insert 'type' command. Left commented because not yet implemented
      in Pentyp routine */
#if 0
   *_RKG.MFCurrPos++ = 'T';
   strncpy( _RKG.MFCurrPos, _RKG.currtype, 8);
      _RKG.MFCurrPos+=8;
     *_RKG.MFCurrPos++ = '\n';
#endif

   } /* End mfstate */
