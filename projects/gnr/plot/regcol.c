/* (c) Copyright 1992-2011, The Rockefeller University *21114* */
/* $Id: regcol.c 20 2011-12-24 02:59:03Z  $ */
/***********************************************************************
*                               regcol()                               *
*                                                                      *
*  SYNOPSIS:                                                           *
*     unsigned int regcol(const char *pencol)                          *
*                                                                      *
*  DESCRIPTION:                                                        *
*     regcol() changes the current color index,                        *
*     and updates the next color synonym.                              *
*                                                                      *
*  RETURN VALUES:                                                      *
*     None                                                             *
************************************************************************
*  Version 2, 10/24/18, GMK                                            *
***********************************************************************/

#include <string.h>
#include "mfint.h"
#include "plots.h"

unsigned int regcol(const char *pencol){

   ui32 nc = strlen(pencol);
   ui32 rtn = _RKG.nxtcsyn;
   
   if (!(_RKG.s.MFActive)) return;
   mfbchk(mxlRcol);
   mfbitpk(OpCtsyn, Lop);
   k2mf(nc);
   cs2mf(pencol, nc);
   _RKG.nxtcsyn = rtn + 2;
   return rtn;
} /* End regcol() */