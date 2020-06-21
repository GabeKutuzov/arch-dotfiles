/* (c) Copyright 2016, The Rockefeller University *11115* */
/* $Id: compdate.c 70 2017-01-16 19:27:55Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                              compdate                                *
*                                                                      *
*  This routine returns a pointer to the date it was compiled.  The    *
*  idea is to force this routine to be compiled every time make runs,  *
*  so the correct date will be available even when darwin3.c is not    *
*  compiled.                                                           *
*                                                                      *
************************************************************************
*  R66, 04/15/16, GNR - New routine                                    *
*  ==>, 04/15/16, GNR - Last mod before committing to svn repository   *
***********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"

char *compdate(void) {

   static char cdate[12];
#ifdef GCC
   strncpy(cdate, __DATE__, sizeof(cdate));
#else
   strcpy(cdate, "??? ?? ????");
#endif
   return cdate;

   } /* End compdate */
