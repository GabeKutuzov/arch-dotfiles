/* (c) Copyright 1992-2011, The Rockefeller University *21114* */
/* $Id: mfst.c 31 2017-04-13 21:51:22Z  $ */
/***********************************************************************
*                               mfst()                                 *
*                                                                      *
* SYNOPSIS:                                                            *
*  void mfst(int index, float xsz, float ysz, float xi,                *
*     float yi, char *chart)                                           *
*                                                                      *
* DESCRIPTION:                                                         *
*  This function writes the '[' (Start Frame) metafile command to the  *
*  buffer at the begining of each new frame, then, if multinode,       *
*  flushes the buffer to insure that '[' is the first command asso-    *
*  ciated with the new frame.  If not multinode, also adds the state   *
*  information, because this will be needed only once.  mfst() is      *
*  called only by node 0 and only when newplt is called.               *
*                                                                      *
* N.B.:  Because there are now separate frame indexes for XG and META  *
*  plots, the index argument to mfst should be the larger of the two   *
*  so enough room is allocated in the buffer for the larger.  Then     *
*  mfwrite has to replace this string with the smaller where needed.   *
*                                                                      *
* RETURN VALUES:  None                                                 *
************************************************************************
*  Version 1, 05/12/92, ROZ                                            *
*  Rev, 07/15/93, GNR - Use mfhchk, remove arg for strlen(chart)       *
*  Rev, 12/19/07, GNR - Allow LONG_SIZE digits in frame number         *
*  Rev, 02/27/08, GNR - Add MFPending controls, mfsynch(ksyn)          *
*  ==>, 02/29/08, GNR - Last mod before committing to svn repository   *
*  Rev, 01/04/09, GNR - For compat, allow UI32_SIZE digits in frame #  *
*  Rev, 10/11/11, GNR - _NCG to _RKG, gmf.h now in glu.h, etc.         *
***********************************************************************/

#include "glu.h"
#include "mfio.h"
#include "mpitools.h"
#include "rocks.h"
#include "rkxtra.h"

void mfst(int index, float xsz, float ysz, float xi,
   float yi, const char *chart) {

   int ll = strnlen(chart,8);
   mfhchk(MFSTSIZE(ll));

   *_RKG.MFCurrPos++ = '[';
   i2a(_RKG.MFCurrPos,index,UI32_SIZE,DECENCD);
   i2a(_RKG.MFCurrPos,(int)(xsz*1000.0),5,DECENCD);
   i2a(_RKG.MFCurrPos,(int)(ysz*1000.0),5,DECENCD);
   i2a(_RKG.MFCurrPos,(int)(xi*1000.0),5,DECENCD);
   i2a(_RKG.MFCurrPos,(int)(yi*1000.0),5,DECENCD);
   i2a(_RKG.MFCurrPos,ll,1,DECENCD);
   memcpy((char *)_RKG.MFCurrPos,chart,ll);
   _RKG.MFCurrPos += ll;
   *_RKG.MFCurrPos++ = '\n';

#ifdef PAR
   /* If multinode, flush buffer.  We expect mfsr to send an
   *  immediate ack and that is all that is needed to be sure
   *  this first record is on its way to mfdraw before the
   *  broadcast in newplt allows other nodes to chime in.
   *  Otherwise, write state information.  */
   if (IAmMulti) mfsynch(KSYN_FLUSH);
   else                mfstate();
#else
   mfstate();
#endif
   if (_RKG.s.MFActive & SGX) _RKG.MFPending += 1;
#if defined(DEBUG) && DEBUG & MFDBG_NWPLT
   dbgprt(ssprintf(NULL, "mfst: MFPending incremented to %d",
      (int)_RKG.MFPending));
#endif

   } /* End mfst */
