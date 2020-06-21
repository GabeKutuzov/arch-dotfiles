/* (c) Copyright 1990-2017, The Rockefeller University *21114* */
/* $Id: newplt.c 32 2017-04-26 20:15:14Z  $ */
/***********************************************************************
*                              newplt()                                *
*                                                                      *
* SYNOPSIS:                                                            *
*  newplt(float xsiz, float ysiz, float xorg, float yorg,              *
*         char *pentyp, char *expcol, char *chart, int kout)           *
*                                                                      *
*                                                                      *
* DESCRIPTION:                                                         *
*  Initialize ROCKS plotting package for a new plot (use setmf() for   *
*  the once-only initialization).  This is a combined parallel and     *
*  host version for hybrid systems.  It is compiled into separate      *
*  node0 and noden versions stored in libplotP0.a and libplotPn.a,     *
*  respectively.                                                       *
*                                                                      *
* PARAMETERS:                                                          *
*  xsiz     Width of plot.                                             *
*  ysiz     Height of plot.                                            *
*  xorg     X coord of plot origin--lower left corner.                 *
*  yorg     Y coord of plot origin--lower left corner.                 *
*  pentyp   Type of pen--may be implemented as different types         *
*           of lines on CRT displays (dashed, dotted, etc.)            *
*  expcol   Drawing color--defined in ctlkup.                          *
*  chart    Type of paper for hard-copy plots                          *
*  kout     This parameter may have bits SKP_META and/or SKP_XG set    *
*           in order to cause metafile or X graphics to be skipped     *
*           for this frame only (assuming they are active globally).   *
*           The original bit, DO_MVEXP, do movie exposure, is reserved *
*           for implementation in the new plot library.                *
*  The parameters are documented in detail in the writeup for the      *
*     C version of the ROCKS routines.                                 *
*                                                                      *
* RETURN VALUES:                                                       *
*  0        Normal completion                                          *
*  Other    Value of mfdraw intxeq byte, indicating user interruption  *
*           or termination request of premature death of mfdraw.       *
*                                                                      *
* REMARKS:                                                             *
*  Call in parallel from all nodes on a parallel machine.              *
*  State of plot package is saved in struct _RKG, defined in glu.h.    *
*  Originally, this routine was coded to use a single version          *
*  for node0 and noden programs, distinguished by run-time 'if'        *
*  statements.  This approach was abandoned because of the need        *
*  to eliminate formatted I/O calls (e.g. convrt) from noden           *
*  programs, which must be a link-time decision.  Accordingly,         *
*  the run-time if's have been replaced with compile-time if's.        *
*                                                                      *
*  By tradition, the frame numbers written to mfdraw start at 0, not   *
*  1.  This convention must be maintained or mfdraw will fail to       *
*  read old metafiles.                                                 *
************************************************************************
*  Rev, 08/xx/90, MC  - Add new color table for use with TV            *
*  Rev, 10/04/90, JMS, GNR - Broken into separate function             *
*  Rev, 06/13/92, GNR, ROZ - Add metafile                              *
*  Rev, 08/10/92, ROZ - Add support for SUN and NOGDEV                 *
*  Rev, 07/15/93, GNR - Major rewrite.  Merge sergrph.  Use revised    *
*       mfhead, mfst, fix cdtl comps, setup NCG before using it        *
*  Rev, 05/29/94, GNR - Use nsitools swap support                      *
*  Rev, 09/16/94,  LC - NGraph becomes Newplt.  Support variable       *
*                       window size.                                   *
*  Rev, 10/22/96, GNR - Use expcol argument to call pencol()           *
*  Rev, 11/25/96, GNR - Delete native NCUBE plotting support           *
*  Rev, 07/08/97, GNR - Communicate with mfdraw via socket, not pipe   *
*  Rev, 08/07/97, GNR - Add MFFullBuff, fatal error if malloc fails    *
*  Rev, 06/05/07, GNR - Add kout argument                              *
*  Rev, 02/24/08, GNR - Add SKP_META and SKP_XG to kout                *
*  ==>, 02/29/08, GNR - Last mod before committing to svn repository   *
*  Rev, 10/11/11, GNR - _NCG to _RKG, gmf.h now in glu.h, etc.         *
*                       Calls setmf first time in case user didn't.    *
*                       Sets METF,SGX MFMode bits from setmf() fname,  *
*                       station args, no longer affected by BuffLen    *
*  Rev, 03/16/13, GNR - Fix bug in way color code was forced           *
*  Rev, 08/29/14, GNR - Add 'const' to pointer args                    *
*  Rev, 07/20/16, GNR - Modify for MPI environment                     *
*  Rev, 04/07/17, GNR - Move to ackbutt combined ack+buttons message   *
*                       Use MFPending instead of equiv. MFF_FramePend  *
*  R32, 04/26/17, GNR - Fix bug: newplt sending finplt data 2nd time   *
*  R38, 08/22/18, GNR - Fix bug: Not updating frame numbers if NOXG    *
***********************************************************************/

#include <stdlib.h>
#include "sysdef.h"
#include "mpitools.h"
#include "rksubs.h"
#include "glu.h"
#ifdef PAR
#include "memshare.h"
#include "swap.h"
#include "plotnxdr.h"
#endif
#include "plots.h"

int newplt(float xsiz, float ysiz, float xorg, float yorg,
         const char *pentyp, const char *expcol, const char *chart,
         int kout) {

#ifndef PARn
   ui32 mxfr;                 /* Max of MFFrame or XGFrame */
#endif
   int  rc = 0;

#if defined(DEBUG) && DEBUG & MFDBG_NWPLT
   dbgprt("newplt() called");
#endif

   /* This abexit is very unlikely, so no msg */
   if (_RKG.MFFlags & (MFF_MFDrawX|MFF_MEnded)) abexit(261);

/***********************************************************************
*     First call to newplt--initialize graphics and load palette       *
***********************************************************************/

/* The following preparations must be completed whether or not the
*  first frame will actually be written anywhere, so comp nodes can
*  find out whether there might be any graphics in this run.  */

   if (!(_RKG.MFFlags & MFF_ModeSet)) {

#ifndef PARn
      /* If setmf() has not already been called, call it now with
      *  default arguments.  Incidentally, this will ensure that
      *  setmf is loaded and therefore that _RKG is defined.  This
      *  is now done by pound-ifdef I_AM_SETMF in setmf.c so that
      *  setmf() can be called without newplt() in some dynamically
      *  loaded setup code that works with the facility to store
      *  _RKG across dynamic program (e.g. mex-file) invocations.  */

      if (!(_RKG.MFFlags & MFF_SetMFRan))
         setmf(NULL, "", NULL, NULL, 0, 0, '8', 0, 10);
#endif /* !PARn */

#ifdef PAR
      /* Broadcast control parameters for metafile graphics to all
      *  nodes.  By convention, message is always little-endian.  */
      if (IAmMulti) {
#if defined(DEBUG) && DEBUG & MFDBG_NWPLT
         dbgprt("newplt() broadcasting _RKG.s");
#endif
         nxtupush(OPLTTT, OPLTUT);
         blkbcst(&_RKG.s, OPLTTT, 1, MFRKGSI_MSG);
         nxtupop();
         } /* End if multinode */
#endif /* PAR */

      /* Everybody can now allocate a metafile buffer and proceed */
      if (_RKG.s.MFMode) {
         _RKG.MFFullBuff = (unsigned char *)mallocv(
            _RKG.s.MFBuffLen + sizeof(_RKG.MFCountWord), "MF Buffer");
         _RKG.MFCmdBuff = _RKG.MFFullBuff + sizeof _RKG.MFCountWord;
         _RKG.MFCurrPos = _RKG.MFCmdBuff;
         _RKG.MFTopPos  = _RKG.MFCmdBuff + _RKG.s.MFBuffLen;
         }
      _RKG.MFFlags |= MFF_ModeSet;
      } /* End mode setting and initial broadcast */

/***********************************************************************
*                  Setup done on all calls to newplt                   *
***********************************************************************/

/* If a frame is pending, now is the time for all nodes to flush
*  output if not already done at finplt() and then do an outgoing
*  synch before writing any stuff from node 0 for a new frame.  Get
*  button return code (bpause() also handles movies).  mfsynch()
*  assures that soft buttons have been returned from host window if
*  one exists.  */

   if (_RKG.MFPending > 0) {
#if defined(DEBUG) && DEBUG & MFDBG_NWPLT
      dbgprt("newplt calling mfsynch because frame pending");
#endif
      /* Do not repeat sending last buffer if done at finplt */
      if (!(_RKG.MFFlags & MFF_DidFinplt))
         mfsynch(KSYN_COMPLT|KSYN_FLUSH);
      _RKG.MFFlags &= ~MFF_DidFinplt; 
      mfsynch(KSYN_SYNC|KSYN_PWAIT);
      rc = bpause();
#if defined(DEBUG) && DEBUG & MFDBG_NWPLT
      dbgprt(ssprintf(NULL, "newplt called bpause, got ackb = %d,%4m",
         bemtoi4(_RKG.ackb.ackc), &_RKG.ackb.mfdb));
#endif
      _RKG.MFPending -= _RKG.ackb.mfdb.next;
      }

/* Determine whether this specific frame will have output.  Until
*  the following broadcast, this can only be known on node 0.  */

#ifndef PARn
   _RKG.s.MFActive = _RKG.s.MFMode & ~kout;
   if (_RKG.s.MFActive) {
      /* If this is the first time there will be output, now is
      *  the time to start up mfsr or mfdraw, open the metafile,
      *  and write the initial metafile header.  */
      if (!(_RKG.MFFlags & MFF_HdrSent)) {
         float ratio = (float) ysiz/xsiz;
#if defined(DEBUG) && DEBUG & MFDBG_NWPLT
         dbgprt("newplt calling mfcreate--seems to be first plot");
#endif
         mfcreate(_RKG.MFfn, _RKG.s.MFBuffLen, ratio);
         mfhead();
         _RKG.MFFlags |= MFF_HdrSent;
         } /* End starting mfsr/mfdraw and writing header */

      /* Write start record to buffer if either type of output
      *  is active--before following broadcast allows comp nodes
      *  to proceed.  Note that by writing larger of two frame
      *  numbers, we guarantee enough space in header for either.
      */
      mxfr = max(_RKG.XGFrame, _RKG.MFFrame);
#if defined(DEBUG) && DEBUG & MFDBG_NWPLT
      dbgprt("newplt calling mfst to write new plot record");
#endif
      /* mfst calles mfsynch, so this record should be out
      *  on return */
      mfst(mxfr, xsiz, ysiz, xorg, yorg, chart);
      /* It is a long-time convention/bug in newplt/mfdraw that the
      *  first frame must be number 0, not 1, so the frame number
      *  incrementing is done here.  It is also not clear that smarx
      *  mfdraw can handle non-consecutive frame numbers.  This should
      *  be looked at when MFD9 is built.
      *  Increment frame numbers for active modes */
      if (_RKG.s.MFActive & METF) _RKG.MFFrame += 1;
      if (_RKG.s.MFActive & SGX)  _RKG.XGFrame += 1;
#if defined(DEBUG) && DEBUG & MFDBG_NWPLT
      dbgprt(ssprintf(NULL, "newplt: new MFFrame = %jd, XGFrame = %jd",
         _RKG.MFFrame, _RKG.XGFrame));
#endif
      }
#endif  /* Not PARn */

/* Send button return code and new active flags to all nodes.
*  The ackc field of the ackb struct is used to hold MFActive.
*  This also serves as a synchronization point allowing comp
*  nodes to start drawing.  */

#ifdef PAR
   if (IAmMulti) {            /* Must be PAR at run time */
#ifdef PAR0
      bemfmi4(_RKG.ackb.ackc, _RKG.s.MFActive);
#if defined(DEBUG) && DEBUG & MFDBG_NWPLT
      dbgprt(ssprintf(NULL, "newplt about to broadcast MFActive,mfdb "
         "= %d,%4m", _RKG.s.MFActive, &_RKG.ackb.mfdb));
#endif
#endif
#ifdef USE_MPI
      MPI_Bcast(&_RKG.ackb, sizeof(_RKG.ackb), MPI_UNSIGNED_CHAR,
         NC.hostid, NC.commc);
#else
      nxtupush(OPLTTT, OPLTUT);
      blkbcst(&_RKG.ackb, NULL, sizeof(_RKG.ackb), MFSYNCH_MSG);
      nxtupop();
#endif
#ifdef PARn
      rc = (int)_RKG.ackb.mfdb.intxeq & (BUT_INTX|BUT_QUIT|BUT_DEAD);
      _RKG.s.MFActive = bemtoi4(_RKG.ackb.ackc);
#if defined(DEBUG) && DEBUG & MFDBG_NWPLT
      dbgprt(ssprintf(NULL, "Received MFActive,mfdb = %d,%4m",
         _RKG.s.MFActive, &_RKG.ackb.mfdb));
#endif
      /* Count on comp nodes already done on host node in mfst() */
      if (_RKG.s.MFActive & SGX) _RKG.MFPending += 1;
#if defined(DEBUG) && DEBUG & MFDBG_NWPLT
      dbgprt(ssprintf(NULL, "newplt: MFPending incremented to %d",
         (int)_RKG.MFPending));
#endif
#endif
      if (_RKG.ackb.mfdb.intxeq & BUT_DEAD) _RKG.MFFlags |= MFF_MFDrawX;
      } /* End if multinode */
#endif

   _RKG.currcolType = 'J';    /* Force pencol to emit code */
   pencol(expcol);
   return rc;
   } /* End newplt */
