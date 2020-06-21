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
*  Version 2, 11/09/18, GMK                                            *
***********************************************************************/

#include "mfint.h"
#include "plots.h"

// cx, cy

void where(float *x, float *y, float *f) {
   *x = _RKG.pcf->cx;
   *y = _RKG.pcf->cy;
   *f = _RKG.s.fcsc/_RKG.s.base_fact;
   } /* End where() */