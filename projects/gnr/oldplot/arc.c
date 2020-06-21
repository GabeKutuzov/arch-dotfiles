/* (c) Copyright 1992-2011, The Rockefeller University *21114* */
/* $Id: arc.c 30 2017-01-16 19:30:14Z  $ */
/***********************************************************************
*                                arc()                                 *
*                                                                      *
*  SYNOPSIS:                                                           *
*     int arc(float xc, float yc, float xs, float ys, float angle)     *
*                                                                      *
*  DESCRIPTION:                                                        *
*     arc() draws an arc in the current color.  The center of the arc  *
*     is at 'xc' 'yc'.  The starting point of the arc is at 'xs','ys'. *
*     The arc covers 'angle' degrees.  If 'angle' > 0, the arc is      *
*     drawn counterclockwise from xs,ys, otherwise clockwise.          *
*                                                                      *
*  RETURN VALUES:                                                      *
*     None                                                             *
************************************************************************
*  Version 1, 04/15/92, ROZ                                            *
*  Rev, 07/15/93, GNR - Use mfbchk()                                   *
*  Rev, 12/16/93,  LC - Current plot pos. now consistent with doc      *
*  Rev, 11/25/96, GNR - Delete native NCUBE plotting support           *
*  ==>, 12/30/07, GNR - Last mod before committing to svn repository   *
*  Rev, 10/11/11, GNR - _NCG to _RKG, gmf.h now in glu.h, etc.         *
***********************************************************************/

#include "sysdef.h"
#include "plots.h"
#include "glu.h"
#include "rocks.h"
#include "rkxtra.h"

void arc(float xc, float yc, float xs, float ys, float angle) {

   if (_RKG.s.MFActive) {
      mfbchk(MFA_SIZE);
      *_RKG.MFCurrPos++ = 'A';
      *_RKG.MFCurrPos++ = 'C';
      i2a(_RKG.MFCurrPos,(int)(xc*1000.0),5,DECENCD);
      i2a(_RKG.MFCurrPos,(int)(yc*1000.0),5,DECENCD);
      i2a(_RKG.MFCurrPos,(int)(xs*1000.0),5,DECENCD);
      i2a(_RKG.MFCurrPos,(int)(ys*1000.0),5,DECENCD);
      i2a(_RKG.MFCurrPos,(int)(angle*1000.0),7,DECENCD);
      *_RKG.MFCurrPos++ ='\n';
      }

   } /* End arc */
