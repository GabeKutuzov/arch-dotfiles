/* (c) Copyright 1992-2014, The Rockefeller University *11114* */
/* $Id: symbol.c 30 2017-01-16 19:30:14Z  $ */
/***********************************************************************
*                              symbol()                                *
*                                                                      *
*   Device-independent metafile symbol plotting routine for ROCKS.     *
*                                                                      *
*  N.B.:  This routine assumes the character string to be plotted      *
*     is expressed in the native character set of the host system.     *
*     At present, this is EBCDIC for IBM370 and ASCII for all others.  *
*                                                                      *
************************************************************************
*                                                                      *
*  Usage: void symbol(float xpt, float ypt, float hl,                  *
*           const char *bcd, float theta, int ns)                      *
*                                                                      *
*     xpt,ypt - - X,Y coordinates where the label is to start.         *
*     hl  - - - - Height of the letters, in inches.  If negative or    *
*                 zero, use height and angle from previous call.       *
*                 If negative, concatenate to previous string.         *
*     bcd - - - - The actual label to be plotted, normally encoded as  *
*                 a quoted string in C.  Certain special characters    *
*                 are available and are encoded as values which do not *
*                 correspond to printable characters in the character  *
*                 code being used (e.g. control chars).  These may be  *
*                 system-dependent and are documented in the metafile  *
*                 plotting routines.  These may be encoded in the      *
*                 calling program as an escaped hex character or as an *
*                 integer value in a byte variable.  Note that for     *
*                 historical reasons the length of the string must be  *
*                 encoded in the 'ns' parameter, and the normal C      *
*                 end-of-string indication was ignored.  This was      *
*                 modified, 08/13/11, to use the lesser of the two     *
*                 lengths to avoid a crash when ns is too large.       *
*     theta - - - The angle of the base line of the label above the    *
*                 horizontal, in degrees.                              *
*     ns  - - - - The length of the bcd string.  If ns is zero or      *
*                 negative, a single centered symbol is drawn.  If     *
*                 ns is 0 or -1, the pen is up when moving to the      *
*                 location of the symbol, otherwise the pen is down.   *
*                                                                      *
************************************************************************
*  Rev, 11/25/96, GNR - Delete native NCUBE plotting support           *
*  ==>, 02/27/08, GNR - Last mod before committing to svn repository   *
*  Rev, 08/13/11, GNR - Use lesser of 'ns' or actual string length     *
*  Rev, 10/11/11, GNR - _NCG to _RKG, gmf.h now in glu.h, etc.         *
*  Rev, 08/29/14, GNR - Add 'const' to pointer args                    *
***********************************************************************/

#include "plots.h"
#include "glu.h"
#include "rocks.h"
#include "rkxtra.h"

/*---------------------------------------------------------------------*
*                                                                      *
*                               SYMBOL                                 *
*                                                                      *
*---------------------------------------------------------------------*/

void symbol(float xpt, float ypt, float hl, const char *bcd,
   float theta, int ns) {

   if (_RKG.s.MFActive) {
      int nsu = (ns > 0) ? strnlen(bcd, ns) : 1;
      mfbchk(MFS_SIZE(nsu));
      *_RKG.MFCurrPos++ = 'S';
      i2a(_RKG.MFCurrPos,(int)(xpt*1000.0),5,DECENCD);
      i2a(_RKG.MFCurrPos,(int)(ypt*1000.0),5,DECENCD);
      i2a(_RKG.MFCurrPos,(int)(hl*1000.0),5,DECENCD);
      i2a(_RKG.MFCurrPos,(int)(theta*1000.0),7,DECENCD);
      i2a(_RKG.MFCurrPos,nsu,3,DECENCD);
      memcpy((char *)_RKG.MFCurrPos, bcd, nsu);
      _RKG.MFCurrPos += nsu;
      *_RKG.MFCurrPos++ = '\n';
      }

   } /* End symbol() */

