/* (c) Copyright 1989-2014, The Rockefeller University *11115* */
/* $Id: second.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                         double second(void)                          *
*                                                                      *
*     This ROCKS routine uses whatever means are available on a given  *
*  system to return the elapsed execution time to best accuracy.       *
*                                                                      *
*  N.B.  The clock() version returns process time, the gettimeofday()  *
*  version returns wall-clock time.  Presumably process time is what   *
*  is wanted here.                                                     *
************************************************************************
*  V1B, 04/21/89, GNR - Add VERTEX, return 0 on undefined systems      *
*  Rev, 02/28/92, GNR - Revised to include SUNOS (acc compiler)        *
*  Rev, 08/20/92, GNR - Put time1 in RKC for overlay save              *
*  Rev, 12/31/96, GNR - Correct ICC code to divide by CLOCKS_PER_SEC,  *
*                       VERTEX code to use 64-bit result, no more RKC  *
*  Rev, 09/04/99, GNR - Remove NCUBE support                           *
*  ==>, 08/18/07, GNR - Last date before committing to svn repository  *
*  Rev, 04/08/14, GNR - Add LINUX case -- use clock() to get just time *
*                       for this process.  Delete ICC support.         *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include "sysdef.h"

#ifdef IBM370
#error Use SECOND ASSEMBLE to create the IBM370 version of second.
#endif

#ifdef LINUX
#include <time.h>
#elif defined UNIX
#include <sys/time.h>
#endif

double SECtime1 = -1.0;          /* Global to retain starting time */

double second(void) {

   double SECtime2;              /* Current time */

#ifdef LINUX
   SECtime2 = (double)clock()/(double)CLOCKS_PER_SEC;
#elif defined UNIX
   struct timeval now;
   gettimeofday(&now,0);
   SECtime2 = (double)now.tv_sec + 0.000001*(double)now.tv_usec;
#endif

   if (SECtime1 < 0.0) SECtime1 = SECtime2;
   return SECtime2 - SECtime1;

   } /* End second */

