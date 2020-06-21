/* (c) Copyright 1992-2011, The Rockefeller University *21114* */
/* $Id: where.c 30 2017-01-16 19:30:14Z  $ */
/***********************************************************************
*                               where()                                *
*                                                                      *
*  SYNOPSIS:                                                           *
*     void where(float *x, float *y, float *f)                         *
*                                                                      *
*  DESCRIPTION:                                                        *
*     This routine return the current position and the scale factor.   *
*                                                                      *
*  RETURN VALUES:                                                      *
*     None                                                             *
************************************************************************
*  Rev, 01/20/93, ROZ                                                  *
*  ==>, 01/25/07, GNR - Last mod before committing to svn repository   *
*  Rev, 10/11/11, GNR - _NCG to _RKG, gmf.h now in glu.h, etc.         *
***********************************************************************/

#include "plots.h"
#include "glu.h"

void where(float *x, float *y, float *f) {
   *x = _RKG.xcurr;
   *y = _RKG.ycurr;
   *f = _RKG.fact/_RKG.s.base_fact;
   } /* End where */
