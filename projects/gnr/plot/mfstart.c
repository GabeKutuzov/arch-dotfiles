/* (c) Copyright 1992-2016, The Rockefeller University *21115* */
/* $Id: mfstart.c 20 2011-12-24 02:59:03Z  $ */
/***********************************************************************
*                     ROCKS metafile plot library                      *
*                              mfstart()                               *
*                                                                      *
* SYNOPSIS:                                                            *
*  void mfstart(const char *chart)                                     *
*                                                                      *
* DESCRIPTION:                                                         *
*  This function writes the '[' (Start Frame) metafile command to the  *
*  buffer at the begining of each new frame, then, if multinode,       *
*  flushes the buffer to insure that '[' is the first command asso-    *
*  ciated with the new frame.  mfstart() is called only by node 0 and  *
*  only when newpltw is called.  No need to check MFmode, newpltw does *
*  that.  No need to call mfstate, state info is stored at mfdraw with *
*  current window and frame info.                                      *
*                                                                      *
* RETURN VALUES:  None                                                 *
************************************************************************
*  Version 1, 05/12/92, ROZ                                            *
*  Rev, 07/15/93, GNR - Use mfhchk, remove arg for strlen(chart)       *
*  Rev, 02/27/08, GNR - Add MFPending controls, mfsynch(ksyn)          *
*  Rev, 10/11/11, GNR - _NCG to _RKG, gmf.h now in glu.h, etc.         *
*  V2A, 10/11/15, GNR - Update for current library spec                *
*  ==>, 08/27/16, GNR - Last mod before committing to svn repository   *
***********************************************************************/

#include "sysdef.h"
#include "mfint.h"
#include "mfio.h"
#include "mpitools.h"

#ifdef PARn
#error mfstart should not be compiled on comp nodes
#else

void mfstart(const char *chart) {

   Win *pcw = _RKG.pcw;
   int ll = strnlen(chart,MAXLCHART);

   mfhchk(Bytes2Bits(mxlStart+ll));
   c2mf(OpStart);
   k2mf((ui32)_RKG.cwin);
   k2mf((ui32)_RKG.cfrm);
   k2mf(pcw->MFFrame);
   k2mf(pcw->XGFrame);
   g2mf(pcw->xsiz);
   g2mf(pcw->ysiz);
   k2mf((ui32)pcw->nexpose);
   k2mf((ui32)pcw->movie_device);
   k2mf((ui32)pcw->movie_mode);
   mfbitpk(ll,Llc);
   mfalign();
   cs2mf(chart,ll);

#ifdef PAR
   /* If multinode, flush buffer and be sure data have
   *  arrived at host before continuing.  */
   if (IAmMulti) mfsynch(KSYN_FLUSH|KSYN_WAIT);
#endif
   if (_RKG.pcw->MFActive & SGX) _RKG.MFPending += 1;
   } /* End mfstart() */
#endif /* Not PARn */
