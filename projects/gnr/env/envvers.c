/* (c) Copyright 2008, The Rockefeller University *11115* */
/* $Id: envvers.c 3 2008-03-13 19:34:28Z  $ */
/***********************************************************************
*              Darwin III Environment Simulation Package               *
*                               envvers                                *
*                                                                      *
*  This routine may be called at any time by an application that is    *
*  using the env library.  It returns a pointer to a static string     *
*  containing the current subversion revision number of the package    *
*  in the form "Env Library Revision nnn", useful for documentation.   *
*                                                                      *
*  Synopsis:                                                           *
*  char *envvers(void)                                                *
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

char *envvers(void) {

   static char envv[] = "Env Library Revision " SVNVRS;

   return envv;

   } /* End envvers() */

