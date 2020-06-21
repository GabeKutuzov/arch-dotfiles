/* (c) Copyright 1992-2014, The Rockefeller University *21114* */
/* $Id: pentyp.c 28 2015-12-22 21:24:28Z  $ */
/***********************************************************************
*                              pentyp()                                *
*                                                                      *
*  SYNOPSIS:                                                           *
*     void pentyp(char *pentype)                                       *
*                                                                      *
*  DESCRIPTION:                                                        *
*     Penset sets the pattern of obeject to be drawn.                  *
*                                                                      *
*  RETURN VALUES:                                                      *
*     None                                                             *
************************************************************************
*  Version 1, 04/15/92, ROZ                                            *
*  Rev, 07/15/93, GNR - Use mfbchk(), strnlen                          *
*  Rev, 11/25/96, GNR - Delete native NCUBE plotting support           *
*  ==>, 02/27/08, GNR - Last mod before committing to svn repository   *
*  Rev, 10/11/11, GNR - _NCG to _RKG, gmf.h now in glu.h, etc.         *
*  Rev, 08/29/14, GNR - Add 'const' to pointer args                    *
***********************************************************************/

#include "plots.h"
#include "glu.h"
#include "rocks.h"
#include "rkxtra.h"

void pentyp(const char *pentype) {

   int len = strnlen(pentype,8);

   if (_RKG.s.MFActive) {

#ifdef PAR
      if (mfbchk(MFT_SIZE(len)))
#else
      mfbchk(MFT_SIZE(len));
#endif
         {
         *_RKG.MFCurrPos++ = 'T';
         i2a(_RKG.MFCurrPos,len,1,DECENCD);
         memcpy((char *)_RKG.MFCurrPos,pentype,len);
         _RKG.MFCurrPos += len;
         *_RKG.MFCurrPos++ = '\n';
         }
      }

   } /* End pentyp() */
