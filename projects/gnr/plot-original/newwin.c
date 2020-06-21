/* (c) Copyright 2015-2016, The Rockefeller University *11115* */
/* $Id: newwin.c 26 2013-10-31 17:48:00Z  $ */
/***********************************************************************
*                     ROCKS metafile plot library                      *
*                              newwin()                                *
*                                                                      *
*  SYNOPSIS:                                                           *
*     unsigned int newwin(void)                                        *
*                                                                      *
*  DESCRIPTION:                                                        *
*     This function declares the existence of a new plotting window    *
*     and establishes buffering for commands directed to that window.  *
*     Plotting in this window does not begin until a newpltw() call    *
*     specifying the window number returned by newwin().  On parallel  *
*     computers, newwin() must be called in parallel from all nodes.   *
*     Inbound followed by outbound synchronization is performed.       *
*                                                                      *
*  ARGUMENTS:                                                          *
*      None.                                                           *
*                                                                      *
*  RETURN VALUE:                                                       *
*     Number of the newly created window.  If newwin() is called       *
*     before any call to newplt() or newpltw(), window 0 will be       *
*     redundantly created.                                             *
*                                                                      *
*  DESIGN NOTES:                                                       *
*     Although nothing is written to the output file by this routine,  *
*     a broadcast is performed in order to assure that all comp nodes  *
*     get the same window number assignment at the same time.          *
*                                                                      *
*     When a window is closed, its command buffer will be retained,    *
*     as this will make it quicker to reopen the window later.         *
*                                                                      *
************************************************************************
*  V2A, 10/17/15, GNR - New program                                    *
*  ==>, 08/05/16, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "rksubs.h"
#include "mfint.h"
#include "mfio.h"
#include "plots.h"
#include "swap.h"
#ifdef PAR
#include "mpitools.h"
#include "rkgnxdr.h"
extern long RKGTT[];          /* Structure conversion table */
#endif

unsigned int newwin(void) {

   Win *pwin;                 /* Ptr to current window */
   unsigned int iwin;         /* Window number to be assigned */
#ifdef PAR
   int junk;                  /* Unused awhoami arg */
   char rcm[FMJSIZE];         /* Buffer for rc in message format */
#endif

/* The following preparations must be completed whether or not the
*  first frame will actually be written anywhere, so comp nodes can
*  find out whether there might be any graphics in this run.  */

   if (!(_RKG.MFFlags & MFF_DidIfin)) {

#ifndef PARn
      /* If setmf() has not already been called, call it now with
      *  default arguments.  Incidentally, this will ensure that
      *  setmf is loaded and therefore that _RKG is defined.  This
      *  is now done by pound-ifdef I_AM_SETMF in setmf.c so that
      *  setmf() can be called without newplt() in some dynamically
      *  loaded setup code that works with the facility to store
      *  _RKG across dynamic program (e.g. mex-file) invocations.  */

      if (!(_RKG.MFFlags & MFF_DidSetup))
         setmf(NULL, "", NULL, NULL, 0, 0, 'B', 0, 10);
      rkgifin();
#endif /* !PARn */

      /* Determine parallel configuration parameters.  If there is
      *  just one node, flag serial as numnodes == 0.  */
#ifdef PAR

      _RKG.s.numnodes = IAmMulti ? NC.total : 0;

      /* Broadcast control parameters to all nodes */
      if (_RKG.s.numnodes)
         blkbcst(&_RKG.s, RKGTT+IXstr_RKGSdef, 1, MFRKGSI_MSG);
#endif /* PAR */
      } /* End setmf/rkgifin/_RKG.s setup */

/* If there are no free Windef structures available, make some more
*  and make a free list from the new ones.  Then point pwin to the
*  next available free Win.  */

   if (!_RKG.pawn) {
      Win **pplw = &_RKG.pawn;
      int i, newnwallo = _RKG.nwallo + _RKG.nw2allo;
      if (newnwallo > UI16_MAX) mfckerr(ERR_TMWN);
      _RKG.pW0 = (Win *)reallocv(_RKG.pW0, newnwallo*sizeof(Win),
         "Window array");
      pwin = _RKG.pW0 + _RKG.nwallo;
      memset(pwin, 0, _RKG.nw2allo*sizeof(Win));
      for (i=_RKG.nwallo; i<newnwallo; ++pwin,++i) {
         pwin->iwin = (ui16)i;
         *pplw = pwin;
         pplw = &pwin->pnwin;
         }
      *pplw = NULL;
      _RKG.nwallo = newnwallo;
      /* Allocate more next time -- user maybe needs lots */
      _RKG.nw2allo += _RKG.nw2allo/2;
      }
   pwin = _RKG.pawn;
   _RKG.pawn = pwin->pnwin;
   pwin->pnwin = NULL;
   iwin = pwin->iwin;

#ifdef PAR
   /* On a parallel computer, broadcast number of next window
   *  and be sure all nodes are in synch with this number.  */
#ifdef PAR0
   bemfmi4(rcm, iwin);
#endif
   blkbcst(rcm, NULL, FMJSIZE, MFNEWWN_MSG);
#ifdef PARn
   junk = bemtoi4(rcm);
   if (junk != iwin) mfckerr(ERR_PWIN);
#endif
#endif

/* If not already done (i.e. not reusing allocation for a closed
*  window), allocate a buffer.  newplt() or newpltw() is now
*  responsible for allocating a frame.  */

   if (!(pwin->WinFlags & WFF_HasBuff)) {
      pwin->MFFullBuff = (byte *)mallocv(_RKG.s.MFBuffLen +
         sizeof(pwin->MFCountWord), "MF Buffer");
      pwin->MFCmdBuff = pwin->MFFullBuff + sizeof(pwin->MFCountWord);
      pwin->WinFlags |= WFF_HasBuff;
      }

   /* Initializations needed even if this is an old Windef */
   pwin->pfr1 = NULL;
   pwin->MFCP = pwin->MFCmdBuff;
   pwin->XGFrame = pwin->MFFrame = 0;
   pwin->MFBrem0 = pwin->MFBrem = _RKG.s.MFBrem0;
   pwin->MFCountWord = 0;
   pwin->cfrm = 0;
   pwin->MFActive = 0;
   pwin->WinFlags &= WFF_HasBuff;   /* Clear other flags */
#ifndef PARn
   pwin->nexpose = _RKG.nexpose;
   pwin->movie_device = _RKG.movie_device;
   pwin->movie_mode = _RKG.movie_mode;
#endif

   return iwin;
   } /* End newplt() */
