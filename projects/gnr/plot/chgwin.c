/* (c) Copyright 1998, Neurosciences Research Foundation, Inc. */
/* $Id: chgwin.c 29 2017-01-03 22:07:40Z  $ */
/***********************************************************************
*                               chgwin()                               *
*                                                                      *
*  SYNOPSIS:                                                           *
*     void chgwin(unsigned int iwin)                                   *
*                                                                      *
*  DESCRIPTION:                                                        *
*                                                                      *
*  RETURN VALUE:                                                       *
*     None.                                                            *
*                                                                      *
************************************************************************
*  Version 2, 11/16/18, GMK                                            *
***********************************************************************/

#include "mfint.h"
#include "plots.h"

void chgwin(unsigned int iwin){
   
   if (!(_RKG.pcw->MFActive)) return;
   mfbchk(mxlWinChg);
   mfbitpk(OpChgWin, Lop);
   k2mf(iwin);
} /* end chgwin() */