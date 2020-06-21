/* (c) Copyright 1993-2009, The Rockefeller University *21115* */
/* $Id: fatal.c 3 2017-01-16 19:29:37Z  $ */
/***********************************************************************
*                          MPI Tools Library                           *
*                               fatal.c                                *
*                                                                      *
*  This file is part of the mpitools library, which may be used by     *
*  all processes running on a parallel system.  fatale() and fatalpe() *
*  maintain backward compatibility with existing code, enabling both   *
*  application and Unix error msgs to be intercepted on comp nodes     *
*  and passed to the host.                                             *
*                                                                      *
*  Usage:                                                              *
*     void fatale(char *s1, char *s2)                                  *
*        Print error message specified by s1,s2 on host stderr.        *
*     void fatalpe(char *s1, char *s2)                                 *
*        Print error message specified by s1,s2 followed by some       *
*        text relating to system errno on host stderr.                 *
*                                                                      *
*  Arguments:                                                          *
*     s1 can be either a single text, in which case s2 is NULL, or it  *
*     can be a format compatible with ssprintf(), in which case s2 can *
*     be any single variable referenced in that format.                *
*                                                                      *
*  N.B. The same library is used by parallel host and serial programs. *
*  The serial case can be distinguished at run time by NC.dim < 0.     *
************************************************************************
*  Written by Ariel Ben-Porath                                         *
*  V1A, 03/24/93 Initial version                                       *
*  Rev, 03/28/98, GNR - Use ssprintf not sprintf, make overflow safe,  *
*                       provide implementation for serial programs     *
*  ==>, 12/29/09, GNR - Last mod before committing to svn repository   *
*  Rev, 04/30/16, GNR - Use new appexit arg spec                       *
*  ==>, 07/07/16, GNR - Last mod before committing to svn mpi repo     *
***********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "sysdef.h"
#include "mpitools.h"
#include "rksubs.h"

extern char *sys_errlist[];

/*---------------------------------------------------------------------*
e*                              fatale()                                *
*---------------------------------------------------------------------*/

void fatale(char *s1, char *s2) {
   char emsg[160];

   /* Make local copy of edited message to avoid
   *  recursive use of ssprintf internal buffer.  */
   if (s2)
      strncpy(emsg, ssprintf(NULL,s1,s2), sizeof(emsg));
   else
      strncpy(emsg, s1, sizeof(emsg));

   appexit(emsg, DRVEXIT_ERR, 0);
#ifndef GCC
   /* JIC: Should never get here from above code */
   exit(1);
#endif
   } /* End fatale() */

/*---------------------------------------------------------------------*
*                              fatalpe()                               *
*---------------------------------------------------------------------*/

void fatalpe(char *s1, char *s2) {
   char emsg[160];
   size_t lemsg = sizeof(emsg);

   if (s2)
      strncpy(emsg, ssprintf(NULL,s1,s2), lemsg);
   else
      strncpy(emsg, s1, lemsg);
   lemsg -= strnlen(emsg, lemsg);
   if (lemsg > 2) {
      strncat(emsg, ": ", lemsg);
      lemsg -= 2;
      strncat(emsg, sys_errlist[errno], lemsg);
      }

   appexit(emsg, DRVEXIT_ERR, errno);
#ifndef GCC
   /* JIC: Should never get here from above code */
   exit(1);
#endif
   } /* End fatalpe() */

