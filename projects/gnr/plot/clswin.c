/* (c) Copyright 1998, Neurosciences Research Foundation, Inc. */
/* $Id: clswin.c 29 2017-01-03 22:07:40Z  $ */
/***********************************************************************
*                               clswin()                               *
*                                                                      *
*  SYNOPSIS:                                                           *
*     void clswin(unsigned int iwin)                                   *
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

void clswin(unsigned int iwin){

   if (!(_RKG.pcw->MFActive)) return;
   mfbchk(mxlWinCtl);
   c2mf(OpWinCtl);
   k2mf(iwin);
} /* end clswin */
