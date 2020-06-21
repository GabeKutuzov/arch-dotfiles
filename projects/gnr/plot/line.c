/* (c) Copyright 1992-2011, The Rockefeller University *21114* */
/* $Id: line.c 20 2011-12-24 02:59:03Z  $ */
/***********************************************************************
*                               line()                                 *
*                                                                      *
*  SYNOPSIS:                                                           *
*     void line(float x1, float y1, float x2, float y2)                *
*                                                                      *
*  DESCRIPTION:                                                        *
*     This function draws a line from (x1,y1) to (x2,y2) in the        *
*     current color and thichness.                                     *
*                                                                      *
*  RETURN VALUES:                                                      *
*     None                                                             *
************************************************************************
*  Version 2, 10/24/18, GMK                                            *
***********************************************************************/

#include "mfint.h"
#include "plots.h"

void line(float x1, float y1, float x2, float y2){

	if (!(_RKG.pcw->MFActive)) return;
	mfbchk(_RKG.s.mxlLine);
    mfbitpk(OpLine, Lop);
    x2mf(x1);
    y2mf(y1);
    x2mf(x2);
    y2mf(y2);
}/* End line() */