/* (c) Copyright 1992-2011, The Rockefeller University *21114* */
/* $Id: plot.c 20 2011-12-24 02:59:03Z  $ */
/***********************************************************************
*                               color()                                *
*                                                                      *
*  SYNOPSIS:                                                           *
*     void color(COLOR blue, COLOR green, COLOR red)                   *
*                                                                      *
*  DESCRIPTION:                                                        *
*     color() changes the drawing color to a specified                 *
*     blue-green-red value.                                            *
*                                                                      *
*  RETURN VALUES:                                                      *
*     None                                                             *
************************************************************************
*  Version 2, 10/24/18, GMK                                            *
***********************************************************************/

#include "mfint.h"
#include "plots.h"
#include "plotdefs.h"

void color(COLOR blue, COLOR green, COLOR red){

    if (!(_RKG.pcw->MFActive)) return;
    mfbchk(mxlColor);
    c2mf(OpColor);
    mfbitpk(OpColor, Lop * 8);
    mfbitpk(blue, 8);
    mfbitpk(green, 8);
    mfbitpk(red, 8);
}/* End color() */
