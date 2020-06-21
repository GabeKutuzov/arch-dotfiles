/* (c) Copyright 1992-2011, The Rockefeller University *21114* */
/* $Id: gscale.c 20 2011-12-24 02:59:03Z  $ */
/***********************************************************************
*                              gscale()                                *
*  SYNOPSIS:                                                           *
*     float gscale(void)                                               *
*                                                                      *
*  DESCRIPTION:                                                        *
*     This function return the absolute scale factor used to convert   *
*     coordinates provided to ROCKS plotting routines into whatever    *
*     internal units are used by the plotting interface (e.g. pixels   *
*     on a CRT).                                                       *
*                                                                      *
*  RETURN VALUES:                                                      *
*     None                                                             *
************************************************************************
*  Rev, 04/15/92, ROZ                                                  *
*  ==>, 01/25/07, GNR - Last mod before committing to svn repository   *
*  Rev, 10/11/11, GNR - _NCG to _RKG, gmf.h now in glu.h, etc.         *
***********************************************************************/

#include "plots.h"
#include "glu.h"

float gscale(void) {

   return _RKG.fact*PPI;
   } /* End gscale */
