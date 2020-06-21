/* (c) Copyright 2016, The Rockefeller University *11115* */
/* $Id: mpitvers.c 3 2017-01-16 19:29:37Z  $ */
/***********************************************************************
*                          MPI Tools Library                           *
*                             mpitvers.c                               *
*                                                                      *
*  This routine may be called at any time by an application that is    *
*  using the MPITOOLS package.  It returns a pointer to a static       *
*  string containing the current subversion revision number of the     *
*  package in the form "MPITOOLS Library Rev nnn", useful for          *
*  documentation.                                                      *
*                                                                      *
*  Synopsis: char *mpitvers(void)                                      *
*                                                                      *
************************************************************************
*  V1A, 06/25/16, GNR - New program, modified from bbdvers             *
*  ==>, 06/25/16, GNR - Last mod before committing to svn repository   *
***********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "sysdef.h"
#include "mpitools.h"

char *mpitvers(void) {

   static char mpitv[] = "MPITOOLS Library Revision " SVNVRS;

   return mpitv;

   } /* End mpitvers() */
