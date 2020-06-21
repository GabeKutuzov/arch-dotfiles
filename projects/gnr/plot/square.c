/* (c) Copyright 1992-2011, The Rockefeller University *21114* */
/* $Id: square.c 20 2011-12-24 02:59:03Z  $ */
/***********************************************************************
*                               square()                               *
*                                                                      *
*  SYNOPSIS:                                                           *
*     void square(float x, float y, float size, int kf)                *
*                                                                      *
*  DESCRIPTION:                                                        *
*                                                                      *
*  RETURN VALUES:                                                      *
*     None                                                             *
************************************************************************
*  Version 2, 10/24/18, GMK                                            *
***********************************************************************/

#include "mfint.h"
#include "plots.h"

void square(float x, float y, float edge, int kf){

   if (!(_RKG.pcw->MFActive)) return;
   mfbchk(_RKG.s.mxlSqre);
   mfbitpk(OpSqre, Lop);
   mfbitpk(kf, 2);
   x2mf(x);
   y2mf(y);
   r2mf(edge);
}/* End square() */