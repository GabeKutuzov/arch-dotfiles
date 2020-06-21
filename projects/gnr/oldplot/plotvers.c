/* (c) Copyright 2008, The Rockefeller University *11114* */
/* $Id: plotvers.c 30 2017-01-16 19:30:14Z  $ */
/***********************************************************************
*                             plotvers()                               *
*                                                                      *
*  This routine may be called at any time by an application that is    *
*  using the plot library.  It returns a pointer to a static string    *
*  containing the current subversion revision number of the package    *
*  in the form "Plot Library Rev nnn", useful for documentation.       *
*                                                                      *
*  Synopsis:                                                           *
*  char *plotvers(void)                                                *
************************************************************************
*  V1A, 03/12/08, GNR - New program                                    *
*  ==>, 03/12/08, GNR - Last mod before committing to svn repository   *
*  Rev, 03/31/08, GNR - Use library svnversion from makefile, not just *
*                       version of this routine returned by Rev prop.  *
***********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "sysdef.h"

char *plotvers(void) {

   static char pllv[] = "Plot Library Revision " SVNVRS;

   return pllv;

   } /* End plotvers() */
