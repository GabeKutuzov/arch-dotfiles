/* (c) Copyright 1992-2011, The Rockefeller University *21114* */
/* $Id: factor.c 20 2011-12-24 02:59:03Z  $ */
/***********************************************************************
*                              factor()                                *
*                                                                      *
*  SYNOPSIS:                                                           *
*     void factor(float f)                                             *
*                                                                      *
*  DESCRIPTION:                                                        *
*     The function enable the user to enlarge or reduce the entire     *
*     plot or part of the plot.                                        *
*                                                                      *
*  RETURN VALUES:                                                      *
*     None                                                             *
************************************************************************
*  Rev, 04/15/92, ROZ                                                  *
*  Rev, 02/29/08, GNR - Now have separate XG and META frame numbers    *
*  ==>, 02/29/08, GNR - Last mod before committing to svn repository   *
*  Rev, 10/11/11, GNR - _NCG to _RKG, gmf.h now in glu.h, etc.         *
***********************************************************************/

#include "plots.h"
#include "glu.h"

void factor(float f) {

/* Calls to FACTOR prior to first NGRAPH call are WHERE-transparent */

   if (_RKG.XGFrame | _RKG.MFFrame)
      _RKG.fact = f;
   else
      _RKG.s.base_fact = _RKG.fact = f;

   } /* End factor */
