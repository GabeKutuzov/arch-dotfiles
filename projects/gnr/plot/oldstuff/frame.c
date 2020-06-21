/* (c) Copyright 1998, Neurosciences Research Foundation, Inc. */
/***********************************************************************
*                               frame()                                *
*                                                                      *
*  SYNOPSIS:                                                           *
*     void frame(unsigned int iframe)                                  *
*                                                                      *
*  DESCRIPTION:                                                        *
*     This function causes subsequent plotting commands to be executed *
*     relative to a previously defined local plotting frame.           *
*                                                                      *
*  ARGUMENT:                                                           *
*     iframe  Number of the plotting frame to be used.  Frames can     *
*             be defined by calling frameb() or framec().  Frame 0     *
*             always refers to the default frame consisting of the     *
*             entire plot.  If 'iframe' refers to a frame that has     *
*             not yet been defined, frame 0 is used.                   *
*                                                                      *
*  CURRENT PLOT POSITION ON RETURN:                                    *
*     (0,0) in the new frame.  The origin shift is also set to (0,0).  *
*                                                                      *
*  RETURN VALUE:                                                       *
*     None.                                                            *
*                                                                      *
*  V2A, 12/29/98, GNR - New routine, make binary metafile              *
***********************************************************************/

#include "mfint.h"
#include "plots.h"

#define FOP    0     /* gks frame operation offset */
#define FNM    1     /* gks frame number offset */

void frame(unsigned int iframe) {

   static char MCframe[] = { OpFrame, Tkn, FOP, 2, Tk, FNM, Tend };

   if (_RKG.s.MFmode) {
      _RKG.gks[FOP] = FOpSet;
      _RKG.gks[FNM] = iframe;
      CMDPACK(MCframe, Lop + 2 + Li11);
      _RKG.x[CUR] = _RKG.xo = 0;
      _RKG.y[CUR] = _RKG.yo = 0;
      } /* End if MFmode */

$$$ CLEAR InReset BIT

   } /* End frame() */
