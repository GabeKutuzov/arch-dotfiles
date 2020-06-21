/* (c) Copyright 2008, The Rockefeller University *11115* */
/* $Id: crkvers.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                              crkvers.c                               *
*                                                                      *
*  This routine may be called at any time by an application that is    *
*  using the ROCKS library.  It returns a pointer to a static string   *
*  containing the current subversion revision number of the package    *
*  in the form "ROCKS Library Rev nnn", useful for documentation.      *
*                                                                      *
*  Synopsis:                                                           *
*  char *crkvers(void)                                                 *
*                                                                      *
************************************************************************
*  V1A, 12/22/08, GNR - New program based on plotvers.c                *
*  ==>, 12/22/08, GNR - Last mod before committing to svn repository   *
***********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "sysdef.h"

char *crkvers(void) {

   static char rklv[] = "ROCKS Library Revision " SVNVRS;

   return rklv;

   } /* End crkvers() */
