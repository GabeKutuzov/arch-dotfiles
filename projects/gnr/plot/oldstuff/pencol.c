/* (c) Copyright 1992-2014, The Rockefeller University *21114* */
/* $Id: pencol.c 28 2015-12-22 21:24:28Z  $ */
/***********************************************************************
*                              pencol()                                *
*                                                                      *
* SYNOPSIS:                                                            *
*  void pencol(const char *color)                                      *
*                                                                      *
* DESCRIPTION:                                                         *
*  This function sets the drawing color to the value given by the      *
*  argument, which may be interpreted as an English color name or      *
*  as a hexadecimal BGR code as documented in the writeup.             *
*  The formula for which entry in look-up table corresponds to a       *
*  desired RGB value is:  Blue + Green + Red  where Blue is a number   *
*  between 0-3, shifted left 6,  Green is a number between 0-7,        *
*  shifted three and Red is a number between 0-7.                      *                                                                      *
*  XWindows uses only 128 of the possible 256 colors.                  *
*  Pencol writes device independent 'K' records to the metafile (it    *
*  passes its argument as the color).  NCG.currcolStr is set.          *
*                                                                      *
* NOTES:                                                               *
*  Some implementations of the drawing server expect hex colors to     *
*  have only uppercase 'A'-'F'.  To minimize execution time, this      *
*  restriction is not enforced here--the application input routine     *
*  should do this.                                                     *
*                                                                      *
*  The BGR values for the named colors are:                            *
*                                                                      *
*    WHITE   0,  0,  0      White ==> background                       *
*    BLACK   C0, C0, C0                                                *
*    BLUE    C0, 0,  0                                                 *
*    CYAN    C0, C0, 0                                                 *
*    MAGENTA C0, 0,  C0                                                *
*    VIOLET  C0, 0,  80                                                *
*    ORANGE  0,  40, E0                                                *
*    GREEN   0,  E0, 0                                                 *
*    YELLOW  0,  E0, E0                                                *
*    RED     0,  0,  E0                                                *
*                                                                      *
* RETURN VALUES:                                                       *
*  None                                                                *
************************************************************************
*  Version 1, 04/16/92, ROZ ,based on old penset() routine             *
*  Rev, 07/15/93, GNR - Use mfbchk(), strnlen                          *
*  Rev, 01/03/94,  LC - Added currcolStr and currcolType               *
*  Rev, 03/15/94,  LC - Reorganized routine                            *
*  Rev, 11/25/96, GNR - Delete native NCUBE plotting support           *
*  Rev, 02/11/05, GNR - Omit change to current color                   *
*  ==>, 02/27/08, GNR - Last mod before committing to svn repository   *
*  Rev, 06/19/08, GNR - Enforce lengths 4 for X and 7 for Z colors     *
*  Rev, 10/11/11, GNR - _NCG to _RKG, gmf.h now in glu.h, etc.         *
*  Rev, 03/16/13, GNR - Fix bug in way color code was forced           *
*  Rev, 08/29/14, GNR - Add 'const' to pointer args                    *
***********************************************************************/

#include "plots.h"
#include "glu.h"
#include "rocks.h"
#include "rkxtra.h"

#define MAX_COLOR_LENGTH    8

void pencol(const char *color) {

   int len = strnlen(color, MAX_COLOR_LENGTH);

   if (color[0] == 'X' && len != 4 || color[0] == 'Z' && len != 7)
      abexit(287);

   if (len == 0 || (_RKG.currcolType == 'K' &&
      !memcmp(_RKG.currcolStr, color, len))) return;

   memcpy(_RKG.currcolStr, color, len+1);
   _RKG.currcolType = 'K';

   if (_RKG.s.MFActive) {

#ifdef PAR
      if (mfbchk(MFK_SIZE(len)))
#else
      mfbchk(MFK_SIZE(len));
#endif
         {
         *_RKG.MFCurrPos++ = 'K';
         i2a(_RKG.MFCurrPos, len, 1, DECENCD);
         memcpy((char *)_RKG.MFCurrPos, color, len);
         _RKG.MFCurrPos += len;
         *_RKG.MFCurrPos++ = '\n';
         }
      }

   } /* End pencol() */
