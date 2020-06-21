/* (c) Copyright 1992-2011, The Rockefeller University *21114* */
/* $Id: color.c 30 2017-01-16 19:30:14Z  $ */
/***********************************************************************
*                               color()                                *
*                                                                      *
*  SYNOPSIS:                                                           *
*     void color(COLOR index)                                          *
*                                                                      *
*  DESCRIPTION:                                                        *
*     This function changes the drawing color to a particular index    *
*     value.  The argument to the function is interpreted as an index  *
*     into a color lookup table.                                       *
*                                                                      *
*  RETURN VALUES:                                                      *
*     None.                                                            *
************************************************************************
*  Version 1, 04/15/92, ROZ                                            *
*  Rev, 07/15/93, GNR - Use mfbchk()                                   *
*  Rev, 01/03/94,  LC - added currcolType                              *
*  Rev, 11/25/96, GNR - Delete native NCUBE plotting support           *
*  ==>, 02/27/08, GNR - Last mod before committing to svn repository   *
*  Rev, 10/11/11, GNR - _NCG to _RKG, gmf.h now in glu.h, etc.         *
***********************************************************************/

#include "plots.h"
#include "glu.h"
#include "rocks.h"
#include "rkxtra.h"

void color(COLOR index) {

   _RKG.currcol = index;
   _RKG.currcolType = 'I';

   if (_RKG.s.MFActive) {

#ifdef PAR
      if (mfbchk(MFI_SIZE))
#else
      mfbchk(MFI_SIZE);
#endif
         {
         *_RKG.MFCurrPos++ = 'I';
         i2a(_RKG.MFCurrPos,index,3,DECENCD);
         *_RKG.MFCurrPos++ = '\n';
         }
      }

   } /* End color() */
