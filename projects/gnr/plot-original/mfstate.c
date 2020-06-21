/* (c) Copyright 1993-2015, The Rockefeller University *21115* */
/* $Id: mfstate.c 5 2008-03-13 20:32:23Z  $ */
/***********************************************************************
*                     ROCKS metafile plot library                      *
*                              mfstate()                               *
*                                                                      *
*  SYNOPSIS:                                                           *
*     void mfstate()                                                   *
*                                                                      *
*  DESCRIPTION:                                                        *
*     mfstate() causes the current node, window, and frame numbers     *
*     of the graphics virtual machine on the current node to be        *
*     written when a new buffer is entered.  The rendering program     *
*     (mfdraw) can then restore the coordinates, color, pen, etc.      *
*     from information stored there.  This must be done at the start   *
*     of each buffer in a parallel computer because the state may be   *
*     changed by buffers from another node.  (Default state is         *
*     established by newplt() or newwin() when the first buffer is     *
*     written for a new frame.)                                        *
*                                                                      *
*     If the state-restoration commands plus one more command do not   *
*     fit in the buffer, execution must be terminated by mfckerr().    *
*     Since there is no way to know how long the next command will be, *
*     mfstate() sets the kreset InReset bit when it completes.  This   *
*     bit is cleared whenever a normal plot command completes.  An     *
*     error is generated if mfstate() is called and this bit is set.   *
*                                                                      *
*     The relevant state carried with the plotting frame consists of:  *
*     x, y, h, w, r, gmode, line thickness, object identifier, color   *
*     (name or index), font, and pen type.                             *
*                                                                      *
*     The format of the state restoration record is as follows:        *
*     X'1E',k,k,k                                                      *
*     Data: node,iwin,ifrm                                             *
*      node = node writing this metafile buffer.                       *
*      iwin = current drawing window.                                  *
*      ifrm = current drawing frame.                                   *
*                                                                      *
*     It is not necessary to check _RKG.s.MFmode, caller will do it.   *
*                                                                      *
*  RETURN VALUES:                                                      *
*     None.                                                            *
************************************************************************
*  V1A, 07/15/93, GNR - Initial version                                *
*  Rev, 12/20/93,  LC - Added Pentype state                            *
*  V2A, 08/17/02, GNR - Begin rewrite to support binary metafiles      *
***********************************************************************/

#include "mfint.h"
#include "mfio.h"
#include "plots.h"
#include "rocks.h"
#include "rkxtra.h"
#ifdef PAR
#include "mpitools.h"
#endif

void mfstate(void) {

   /* If reentered before at least one command is complete in
   *  the new buffer, we have a buffer that is too small.  */
   Frame *pfrm = _RKG.pcf;
   if (pfrm->kreset & InReset) mfckerr(ERR_BUFS);
   pfrm->kreset |= InReset;

   c2mf(OpState);
#ifdef PAR
   k2mf((ui32)NC.node);
#else
   k2mf((ui32)0):
#endif
   k2mf((ui32)_RKG.cwin);
   k2mf((ui32)_RKG.cfrm);

   } /* End mfstate() */
