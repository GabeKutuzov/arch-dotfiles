/* (c) Copyright 2015, The Rockefeller University *11115* */
/* $Id: rkgifin.c 5 2008-03-13 20:32:23Z  $ */
/***********************************************************************
*                     ROCKS metafile plot library                      *
*                              rkgifin()                               *
*                                                                      *
* SYNOPSIS:                                                            *
*     void rkgifin(void)                                               *
*                                                                      *
* DESCRIPTION:                                                         *
*     This function is executed during the first call to newpltw().    *
*     It completes initialization of the common data structure, _RKG,  *
*     after setmf() has had a chance to change defaults such as lci,   *
*     lcf, or buffer length, and before the _RKG.s inner structure     *
*     is broadcast to the other nodes in a parallel system.            *
*                                                                      *
* RETURN VALUES:                                                       *
*     None.  There are no error exits.                                 *
*                                                                      *
************************************************************************
*  V2A, 05/16/15, GNR - New routine, code updated from old newplt()    *
*  ==>, 10/03/15, GNR - Last mod before committing to svn repository   *
***********************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include "sysdef.h"
#include "mfint.h"
#include "mfio.h"

void rkgifin(void) {

   Frame *pfr,**pplf;
   int   i;
#ifdef PAR                    /* Items needed for mfsr */
   int   llxy, llhw, llxy2, llhw2;
#endif

   /* Calculate various constants relating to lcf,lci that will
   *  be shared with all nodes, windows, and frames  */
   _RKG.s.xymask   = (1 << _RKG.s.lct) - 1;
   _RKG.s.sxymask  = (2 << _RKG.s.lct) - 1;
   _RKG.s.sfrmask  = (2 << _RKG.s.lcf) - 1;
   _RKG.s.unitxy   = 1 << _RKG.s.lcf;
   _RKG.s.fcsc     = (float)_RKG.s.unitxy;
   _RKG.s.mxlrhw   = Lcx + _RKG.s.lct;
   _RKG.s.mxlxy    = _RKG.s.mxlrhw + 1;
#ifdef PAR
   /* Calculate maximum lengths of records for various plotting
   *  commands that will be used to determine when an alignment
   *  record needs to be inserted near the end of a plot buffer.
   *  For record types that include character strings (chart, font,
   *  etc.), the caller will add the exact length to the value
   *  given here.  There is no need for a mxlAlign because space
   *  is always reserved in the metafile buffer for that record.  */
   llhw = _RKG.s.mxlrhw, llhw2 = llhw + llhw;
   llxy = _RKG.s.mxlxy,  llxy2 = llxy + llxy;
   _RKG.s.mxlArc   = (Lop + 2 + Lla) + llxy2 + llxy2;
   _RKG.s.mxlBmap  = (Lop + 11 + BITSPERBYTE) + llxy2;
   _RKG.s.mxlSBmap = (Lop + 11 + BITSPERBYTE) + llxy2 + llxy2;
   _RKG.s.mxlCirc  = (Lop + 2) + llxy2 + llhw;
   _RKG.s.mxlDraw  = Lop + llxy2;
   _RKG.s.mxlEllps = (Lop + 2 + Lla) + llxy2 + llhw2;
   _RKG.s.mxlStar  = (Lop + 3 + 2*Lg111 + Li10 + Lla) + llxy2 + llhw;
   _RKG.s.mxlLine  = Lop + llxy2 + llxy2;
   _RKG.s.mxlMove  = Lop + llxy2;
   _RKG.s.mxlSqre  = (Lop + 2) + llxy2 + llhw;
   _RKG.s.mxlRect  = (Lop + 2) + llxy2 + llhw2;
   _RKG.s.mxlSymb  = (Lop + Lla + Li01) + llxy2 + llhw;
   _RKG.s.mxlCSym  = (Lop + Lla + Li01) + llhw;
   _RKG.s.mxlFrmNR = (Lop + 10 + Li01) + llhw2;
   _RKG.s.mxlFrmWVxx = (Lop + 10 + Li01 + 6*Lg111) + llhw2;
   _RKG.s.MFBrem0 = BITSPERBYTE *
      (_RKG.s.MFBuffLen - BytesInXBits(LNop + (BITSPERBYTE-1))) - 1;
#else    /* Not PAR */
   _RKG.s.MFBrem0 = BITSPERBYTE * _RKG.s.MFBuffLen - 1;
#endif   /* PAR */

   /* Allocate initial collection of empty frame arrays and
   *  stack them on the empty list.  */
   _RKG.pcf = _RKG.pF0 = (Frame *)callocv(NFRMALLO, sizeof(Frame),
      "Frame array");
   _RKG.nfallo = NFRMALLO;
   pplf = &_RKG.pafr;
   for (pfr=_RKG.pF0,i=0; i<NFRMALLO; ++pfr,++i) {
      pfr->vv[vxx] = pfr->vv[vyy] = 1.0;
      pfr->ifrm = (ui16)i;
      pfr->kreset = (ResetX|ResetY|ResetR|ResetH|ResetW);
      *pplf = pfr;
      pplf = &pfr->pnfrm;
      }

   /* Set initial number of windows to allocate */
   _RKG.nw2allo = NWINALLO;

   /* Indicate that this initialization was completed */
   _RKG.MFFlags |= MFF_DidIfin;

   } /* End of rkgifin() */

