/* (c) Copyright 1992-2014, The Rockefeller University *21114* */
/* $Id: penset.c 30 2017-01-16 19:30:14Z  $ */
/***********************************************************************
*                              penset()                                *
*                                                                      *
*  SYNOPSIS:                                                           *
*     void penset(const char *pentype, const char *color)              *
*                                                                      *
*  DESCRIPTION:                                                        *
*     Penset sets the pattern and color of obeject to be drawn.        *
*                                                                      *
*  RETURN VALUES:                                                      *
*     None                                                             *
************************************************************************
*  Version 1, 04/15/92, ROZ - pentyp and pencol functions separated    *
*  ==>, 01/25/07, GNR - Last mod before committing to svn repository   *
*  Rev, 08/29/14, GNR - Add 'const' to pointer args                    *
***********************************************************************/

#include "plots.h"

void penset(const char *pentype, const char *color) {
   pentyp(pentype);
   pencol(color);
   } /* End penset */
