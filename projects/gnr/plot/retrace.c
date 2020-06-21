/* (c) Copyright 1992-2011, The Rockefeller University *21114* */
/* $Id: retrace.c 20 2011-12-24 02:59:03Z  $ */
/***********************************************************************
*                              retrace()                               *
*                                                                      *
*  SYNOPSIS:                                                           *
*     void retrace(int krt)                                            *
*                                                                      *
*  DESCRIPTION:                                                        *
*     Changes the current thickness parameter used to determines the   *
*     thickness of all subsequent line and unfilled objects            *
*     (circle, ellipse...).                                            *
*                                                                      *
*  RETURN VALUES:                                                      *
*     None                                                             *
************************************************************************
*  Version 2, 10/24/18, GMK                                            *
***********************************************************************/

#include "plots.h"
#include "mfint.h"

void retrace(int krt) {

   if (!(_RKG.pcw->MFActive)) return;
   mfbchk(mxlRetrc);   
   mfbitpk(OpThick, Lop);
   mfbitpk(krt, 4);
} /* End retrace() */