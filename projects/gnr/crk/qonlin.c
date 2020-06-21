/* (c) Copyright 1998-2008, The Rockefeller University *11115* */
/* $Id: qonlin.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                               QONLIN                                 *
*                                                                      *
************************************************************************
*  V1A, 09/21/98, GNR - Attempts to determine whether run is online    *
*  ==>, 08/18/07, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include "sysdef.h"
#include <stddef.h>
#include <ctype.h>
#include "rkxtra.h"
#if defined(UNIX)
#define __USE_BSD
#include <sys/stat.h>
#endif

int qonlin(void) {

#if defined(UNIX)
   struct stat statbuf;
   fstat(0, &statbuf);
   if (statbuf.st_mode & S_IFCHR)
      return TRUE;
#endif
   return FALSE;

   } /* End qonlin() */
