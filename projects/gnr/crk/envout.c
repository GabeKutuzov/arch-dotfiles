/* (c) Copyright 2017, The Rockefeller University *11115* */
/* $Id: envout.c 68 2018-08-15 19:33:46Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                               envout                                 *
*                                                                      *
*  This routine diverts stdout from cryout() calls to a file path      *
*  given by an environment variable whose name is the only argument.   *
*  Must be called before the first call to cryout().                   *
*                                                                      *
*  Return: 0 on success, 1 if the variable was not found.              *
*                                                                      *
************************************************************************
*  R63, 10/07/17, GNR - New program                                    *
*  ==>, 10/07/17, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "rocks.h"
#include "rockv.h"

int envout(const char *envnm) {

   char  *opath;
   size_t lpath;
   /* Don't even do an abexit--output isn't set up yet */
   if (!envnm) exit(108);
   opath = getenv(envnm);
   if (!opath) return 1;
   lpath = strlen(opath);
   if (lpath <= 1) exit(108);
   RKC.PU.rfd.fname = malloc(lpath+1);
   if (!RKC.PU.rfd.fname) exit(108);
   strcpy(RKC.PU.rfd.fname, opath);
   RKC.PU.type = 2;
   return 0;

   } /* End envout() */


