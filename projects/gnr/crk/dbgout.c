/* (c) Copyright 2016, The Rockefeller University *11115* */
/* $Id: dbgout.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                               dbgout                                 *
*                                                                      *
*  This routine diverts stdout from cryout() calls to a file path      *
*  given as the only argument.  It is intended for use when the        *
*  calling application is run as a server, where command-line output   *
*  redirection is not available.                                       *
*                                                                      *
************************************************************************
*  R60, 03/29/16, GNR - New program                                    *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "rocks.h"
#include "rockv.h"

void dbgout(char *opath) {

   size_t lpath;
   /* Don't even do an abexit--output isn't set up yet */
   if (!opath) exit(108);
   lpath = strlen(opath);
   if (lpath <= 1)  exit(108);
   RKC.PU.rfd.fname = malloc(lpath+1);
   if (!RKC.PU.rfd.fname) exit(108);
   strcpy(RKC.PU.rfd.fname, opath);
   RKC.PU.type = 2;

   } /* End dbgout() */


