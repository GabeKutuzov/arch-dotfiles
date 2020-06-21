/* (c) Copyright 2018, The Rockefeller University *11115* */
/* $Id: envin.c 68 2018-08-15 19:33:46Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                                envin                                 *
*                                                                      *
*  This routine checks for an environment variable whose name is given *
*  as the only argument.  If this variable exists, its value is taken  *
*  as a path to a file which is to be used as stdin in this run.  This *
*  routine should be called only once during a run and only before any *
*  cryin() call.  Later input file changes must be made with cdunit(). *
*                                                                      *
*  Return: 0 on success, 1 if the variable was not found.              *
*                                                                      *
************************************************************************
*  R64, 10/07/17, GNR - New program                                    *
*  ==>, 08/07/18, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "rocks.h"
#include "rockv.h"

int envin(const char *envnm) {

   char  *ipath;
   /* Don't even do an abexit--output isn't set up yet */
   if (!envnm) exit(108);
   ipath = getenv(envnm);
   if (!ipath) return 1;
   if (strlen(ipath) <= 1) exit(108);
   cdunit(ipath);
   return 0;

   } /* End envin() */

