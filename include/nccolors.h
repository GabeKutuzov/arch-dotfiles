/* (c) Copyright 1992, The Rockefeller University *11116* */
/* $Id: nccolors.h 59 2017-01-13 20:34:16Z  $ */
/***********************************************************************
*                             NCCOLORS.H                               *
*     This header file contains definitions and macros needed for      *
*  converting between colors and corresponding gray scale values.      *
*                                                                      *
*  V1A, 01/15/92, GNR - Newly written                                  *
*  ==>, 07/01/02, GNR - Last date before committing to svn repository  *
***********************************************************************/

/* Macros to convert between gray and RGB */

#define RGBFromGray(bw) ((bw&0xc0)+((bw&0xe0)>>2)+((bw&0xe0)>>5))
#define GrayFromRGB(c)  (((c&0xc0)+((c&0x38)<<2)+((c&0x07)<<5)+0x41)/3)
