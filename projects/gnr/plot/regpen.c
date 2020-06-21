/* (c) Copyright 1992-2011, The Rockefeller University *21114* */
/* $Id: regpen.c 20 2011-12-24 02:59:03Z  $ */
/***********************************************************************
*                               regpen()                               *
*                                                                      *
*  SYNOPSIS:                                                           *
*     unsigned int regpen(const char *pentyp)                          *
*                                                                      *
*  DESCRIPTION:                                                        *
*     regpen() changes the current pen type,                           *
*     and updates the next color pen id.                               *
*                                                                      *
*  RETURN VALUES:                                                      *
*     None                                                             *
************************************************************************
*  V2A, 11/23/18, GMK                                                  *
***********************************************************************/

#include <string.h>
#include "mfint.h"
#include "plots.h"

unsigned int regpen(const char *pentyp){
   
   ui32 n = strlen(pentyp);
   ui32 rtn = _RKG.nxtpenid;

   if (!(_RKG.s.MFActive)) return;
   mfbchk(mxlRPen);
   mfbitpk(OpRPTyp, Lop);   
   mfbitpk(n, 4);
   cs2mf(pencol, n);
   _RKG.nxtpenid = rtn + 2;
   return rtn;
} /* End regpen() */