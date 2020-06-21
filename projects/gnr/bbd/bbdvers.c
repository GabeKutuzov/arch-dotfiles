/* (c) Copyright 2008, The Rockefeller University *11115* */
/* $Id: bbdvers.c 28 2017-01-13 20:35:15Z  $ */
/***********************************************************************
*                             BBD Package                              *
*                              bbdvers.c                               *
*                                                                      *
*  This routine may be called at any time by an application that is    *
*  using the BBD package.  It returns a pointer to a static string     *
*  containing the current subversion revision number of the package    *
*  in the form "BBD Library Rev nnn", useful for documentation.        *
*                                                                      *
*  Synopsis:                                                           *
*  char *bbdvers(void)                                                 *
************************************************************************
*  V1A, 03/12/08, GNR - New program                                    *
*  ==>, 03/12/08, GNR - Last mod before committing to svn repository   *
*  Rev, 03/31/08, GNR - Use library svnversion from makefile, not just *
*                       version of this routine returned by Rev prop.  *
***********************************************************************/

#define BBDS

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "sysdef.h"

char *bbdvers(void) {

   static char bbdv[] = "BBD Library Revision " SVNVRS;

   return bbdv;
   
   } /* End bbdvers() */

