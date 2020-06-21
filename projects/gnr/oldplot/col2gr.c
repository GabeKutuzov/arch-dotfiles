/* (c) Copyright 2018, The Rockefeller University *21114* */
/* $Id: col2gr.c 37 2018-08-15 19:53:33Z  $ */
/***********************************************************************
*                              col2gr()                                *
*                                                                      *
* SYNOPSIS:                                                            *
*     byte col2gr(byte bmcol);                                         *
*                                                                      *
* DESCRIPTION:                                                         *
*  This function returns the color mode obtained when a color mode     * 
*  from the set accepted by the crk plot library is averaged to        *
*  produce the grayscale equivalent.  Basically, we are getting the    *
*  pixel length (1 or 2) and throwing away the color information.      *
*                                                                      *
* ARGUMENTS:                                                           *
*  bmcol       The colored mode whose pixel length is to be extracted. *
*                                                                      *
* PROTOTYPED IN:                                                       *
*  plots.h                                                             *
*                                                                      *
* RETURN VALUES:                                                       *
*  Byte length of a grayscale pixel, either 1 or 2.                    *
************************************************************************
*  Version 1, 05/27/18, GNR - New program                              *
*  ==>, 05/27/18, GNR - Last mod before committing to svn repository   *
***********************************************************************/

#include <ctype.h>
#include <string.h>
#include "sysdef.h"
#include "plots.h"

byte col2gr(byte bmcol) {
   static byte grcol[] = { BM_BAD, BM_GS, BM_GS16, BM_GS16,
      BM_BAD, BM_BAD, BM_BAD, BM_GS };
   return grcol[bmcol & BM_MASK];
   } /* End col2gr() */
