/* (c) Copyright 1992-2011, The Rockefeller University *21114* */
/* $Id: retrace.c 30 2017-01-16 19:30:14Z  $ */
/***********************************************************************
*                              retrace()                               *
*                                                                      *
*  SYNOPSIS:                                                           *
*     void retrace(int krt)                                            *
*                                                                      *
*  DESCRIPTION:                                                        *
*     Changes the current thickness parameter used to determines the   *
*     thickness of all subsequent line and unfilled objects            *
*     (circle, ellipse...).                                            *
*                                                                      *
*  RETURN VALUES:                                                      *
*     None                                                             *
************************************************************************
*  Version 1, 06/08/92, ROZ                                            *
*  Rev, 11/25/96, GNR - Delete native NCUBE plotting support           *
*  ==>, 02/27/08, GNR - Last mod before committing to svn repository   *
*  Rev, 10/11/11, GNR - _NCG to _RKG, gmf.h now in glu.h, etc.         *
***********************************************************************/

#include "plots.h"
#include "glu.h"
#include "rocks.h"
#include "rkxtra.h"

void retrace(int krt) {

   /* If retrace state not changed, do nothing */
   if (_RKG.curretrac == krt) return;

   _RKG.curretrac = krt;

   if (_RKG.s.MFActive) {
#ifdef PAR
      if (mfbchk(MFH_SIZE))
#else
      mfbchk(MFH_SIZE);
#endif
         {
         *_RKG.MFCurrPos++ = 'H';
         i2a(_RKG.MFCurrPos,_RKG.curretrac,1,DECENCD);
         *_RKG.MFCurrPos++ = '\n';
         }
      }

   } /* End retrace() */
