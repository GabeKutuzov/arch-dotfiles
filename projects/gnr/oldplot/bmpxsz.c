/* (c) Copyright 2017, The Rockefeller University *21114* */
/* $Id: bmpxsz.c 34 2017-09-15 18:13:10Z  $ */
/***********************************************************************
*                              bmpxsz()                                *
*                                                                      *
* SYNOPSIS:                                                            *
*     int bmpxsz(int bmcm)                                             *
*                                                                      *
* DESCRIPTION:                                                         *
*  This functions returns the number of bytes needed to store one      *
*  pixel in a color mode described by the argument bmcm.               *
*                                                                      *
* ARGUMENTS:                                                           *
*  bmcm        Bitmap color mode.  Must be one of the modes defined    *
*              by name BM_xxxx in plotdefs.h                           *
*                                                                      *
* PROTOTYPED IN:                                                       *
*  plots.h                                                             *
*                                                                      *
* RETURN VALUES:                                                       *
*  Number of bytes to hold one pixel of mode BM_xxxx, i.e. one color   *
*  level for grayscale modes, three color levels for colored modes.    *
************************************************************************
*  Version 1, 06/08/17, GNR - New program                              *
*  ==>, 06/08/17, GNR - Last mod before committing to svn repository   *
***********************************************************************/

#include <ctype.h>
#include <string.h>
#include "sysdef.h"
#include "plots.h"

int bmpxsz(int bmcm) {

   /* Image pixel size (bytes) as a function of color mode--
   *  Must agree with BM_xxxx definitions in plotdefs.h.
   *  0 indicates modes that require special treatment.  */
   static byte ipxsz[] = {
      0,    /* BM_BW */
      1,    /* BM_GS */
      2,    /* BM_GS16 */
      6,    /* BM_C48 */
      1,    /* BM_COLOR */
      1,    /* BM_C8 */
      2,    /* BM_C16 */
      3,    /* BM_C24 */
                        };
   
   /* Allow caller to deal with bits up to BM_BAD */
   if (bmcm < 0 || bmcm > BM_BAD) abexit(264);
   if (bmcm > BM_C24) return 0;
   return ipxsz[bmcm];

   } /* End bmpxsz() */






