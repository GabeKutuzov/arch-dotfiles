/* (c) Copyright 1998, Neurosciences Research Foundation, Inc. */
/***********************************************************************
*                               font()                                 *
*                                                                      *
*  SYNOPSIS:                                                           *
*     void font(char *fname)                                           *
*                                                                      *
*  DESCRIPTION:                                                        *
*     This function specifies a font to be used by subsequent calls    *
*     to symbol() and number().  Font names are device-dependent,      *
*     and so should generally be provided from user input.  Also,      *
*     it should be understood that the default stroke font provides    *
*     a uniform character width of 4/7 the character height, whereas   *
*     with proportional fonts no means are currently provided to       *
*     calculate the width of a given character string when plotted.    *
*                                                                      *
*  RETURN VALUE:                                                       *
*     None.                                                            *
*                                                                      *
*  V2A, 12/29/98, GNR - New routine, make binary metafile              *
***********************************************************************/

#include "mfint.h"
#include "plots.h"
#include "rkxtra.h"

#define FLO    0     /* gks font name length offset */

void font(char *fname) {

   static char MCuserfont[] = { OpFont, Tk, FLO, Tchr, FLO, Tend };
   static char MCdfltfont[] = { OpFont, Tk, FLO, Tend };

   if (_RKG.s.MFmode) {
      _RKG.hfontname = savetxt(fname);
      if (fname && strcmp(fname, "DEFAULT")) {
         _RKG.gks[FLO] = strlen(_RKG.psymbol = fname);
         CMDPACK(MCuserfont, Lop + Li11 + _RKG.gks[FLO]);
         }
      else {
         _RKG.gks[FLO] = 0;
         CMDPACK(MCdfltfont, Lop + Li00);
         }
      } /* End if MFmode */

$$$ CLEAR InReset BIT

   } /* End font() */
