/* (c) Copyright 1992-2011, The Rockefeller University *21114* */
/* $Id: arc.c 20 2011-12-24 02:59:03Z  $ */
/***********************************************************************
*                                arc()                                 *
*                                                                      *
*  SYNOPSIS:                                                           *
*     void arc(float xc, float yc, float xs, float ys, float angle)    *
*                                                                      *
*  DESCRIPTION:                                                        *
*     arc() draws an arc in the current color.  The center of the arc  *
*     is at 'xc' 'yc'.  The starting point of the arc is at 'xs','ys'. *
*     The arc covers 'angle' degrees.  If 'angle' > 0, the arc is      *
*     drawn counterclockwise from xs,ys, otherwise clockwise.          *	
*                                                                      *
*  RETURN VALUES:                                                      *
*     None                                                             *
************************************************************************
*  Version 2, 10/24/18, GMK                                            *
***********************************************************************/

#include "mfint.h"
#include "plots.h"

void arc(float xc, float yc, float xs, float ys, float angle) {

   if (!(_RKG.pcw->MFActive)) return;
   
   mfbchk(_RKG.s.mxlArc);
   c2mf(OpArc);
   mfbitpk(0, 2); 
   x2mf(xc);
   y2mf(yc);
   x2mf(xc + xs);
   y2mf(yc + ys);
   a2mf(angle);
} /* End arc() */
