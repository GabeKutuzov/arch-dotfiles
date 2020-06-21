/* (c) Copyright 1998, Neurosciences Research Foundation, Inc. */
/***********************************************************************
*                               gmode()                                *
*                                                                      *
*  SYNOPSIS:                                                           *
*     void gmode(int mode)                                             *
*                                                                      *
*  DESCRIPTION:                                                        *
*     This function changes the drawing mode for all following         *
*     graphics commands except bitmaps, which have their own mode      *
*     parameter.  The drawing mode is ignored on implementations       *
*     that do not support it.                                          *
*                                                                      *
*  ARGUMENT:                                                           *
*     mode    =0 (GM_SET) to set the color index at each point         *
*                 unconditionally to the current drawing color.        *
*             =1 (GM_XOR) to XOR the current drawing color with        *
*                 any existing color index at each point.              *
*             =2 (GM_AND) to AND the current drawing color with        *
*                 any existing color index at each point.              *
*             =3 (GM_CLR) to clear any existing color index at         *
*                 each point to 0 (background color, black).           *
*                                                                      *
*  RETURN VALUE:                                                       *
*     None.                                                            *
*                                                                      *
*  V2A, 12/29/98, GNR - New routine, make binary metafile              *
***********************************************************************/

#include "mfint.h"
#include "plots.h"

#define GMO    0     /* gks gmode offset */

void gmode(int mode) {

   static char MCgmode[] = { OpGMode, Tkn, GMO, 4, Tend };

   if (_RKG.s.MFmode) {
      _RKG.gmode = _RKG.gks[GMO] = mode;
      CMDPACK(MCgmode, Lop + 4);
      } /* End if MFmode */

$$$ CLEAR InReset BIT

   } /* End gmode() */
