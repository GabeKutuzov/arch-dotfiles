/* (c) Copyright 1992-2011, The Rockefeller University *21114* */
/* $Id: regfont.c 20 2011-12-24 02:59:03Z  $ */
/***********************************************************************
*                               regfont()                              *
*                                                                      *
*  SYNOPSIS:                                                           *
*     unsigned int regfont(const char *font)                           *
*                                                                      *
*  DESCRIPTION:                                                        *
*     regfont() changes the current font index,                        *
*     and updates the next font id.                                    *
*                                                                      *
*  RETURN VALUES:                                                      *
*     _RKG.nxtfontid + 2;                                              *
************************************************************************
*  Version 2, 11/23/18, GMK                                            *
***********************************************************************/

#include <string.h>
#include "plots.h"
#include "mfint.h"

unsigned int regfont(const char *font){

   ui32 n = strlen(font);
   ui32 rtn = _RKG.nxtfontid;
   
   if (!(_RKG.s.MFActive)) return;
   mfbchk(_RKG.s.mxlRFont);
   mfbitpk(OpRFont, Lop);
   mfbitpk(n, 6);
   cs2mf(pencol, n);
   _RKG.nxtfontid = rtn + 2;
   return rtn;
} /* End regfont() */