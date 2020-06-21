/* (c) Copyright 2011, The Rockefeller University *11115* */
/* $Id: mfdsleep.c 11 2009-04-21 19:07:27Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                              mfdsleep()                              *
*                                                                      *
*  This is a copy of the standard ROCKS library routine rksleep that   *
*  has been modified simply to exit on errors (unlikely) rather than   *
*  call abexit() with all the attendant need to make available a non-  *
*  cryout() version of that.                                           *
*                                                                      *
*  SYNOPSIS:                                                           *
*     void mfdsleep(int sec, int usec)                                 *
*                                                                      *
*  DESCRIPTION:                                                        *
*     Emulates a sleep() or usleep() call in a manner that is safe     *
*     for use under XView, see XView Programming Manual, p. 503        *
*                                                                      *
************************************************************************
*  V1A, 11/08/11, GNR - Modified from rksleep.c as commented above     *
*  ==>, 11/08/11, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "sysdef.h"     /* Must precede any #ifdefs */
#ifdef UNIX
#include <signal.h>
#include <sys/time.h>
#include <errno.h>
#endif

void mfdsleep(int sec, int usec) {

#if defined(LINUX) || defined(SVR4)
   sigset_t oldmask, mask;
   struct timeval tv;

   tv.tv_sec  = sec;
   tv.tv_usec = usec;

   sigemptyset(&mask);
   sigaddset(&mask, SIGIO);
   sigaddset(&mask, SIGALRM);
   sigaddset(&mask, SIGVTALRM);
   sigaddset(&mask, SIGPROF);
   if (sigprocmask(SIG_BLOCK, &mask, &oldmask) == -1)
      exit(66);
   if (select(0,0,0,0,&tv) == -1 && errno != EINTR)
      exit(66);
   if (sigprocmask(SIG_SETMASK, &oldmask, NULL) == -1)
      exit(66);

#elif defined(BSD)
   int oldmask, mask;
   struct timeval tv;

   tv.tv_sec  = sec;
   tv.tv_usec = usec;

   mask = sigmask(SIGIO);
   mask |= sigmask(SIGALRM);
   oldmask = sigblock(mask);
   if (select(0,0,0,0,&tv) == -1)
      exit(66);
   sigsetmask(oldmask);

#endif

   } /* End mfdsleep() */
