/* (c) Copyright 2007-2008, The Rockefeller University *11115* */
/* $Id: rksleep.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                              rksleep()                               *
*                                                                      *
*  SYNOPSIS:                                                           *
*     void rksleep(int sec, int usec)                                  *
*                                                                      *
*  DESCRIPTION:                                                        *
*     Emulates a sleep() or usleep() call in a manner that is safe     *
*     for use under XView, see XView Programming Manual, p. 503        *
*                                                                      *
************************************************************************
*  Rev, 05/11/07, GNR - Change LINUX to use SVR4 protocols             *
*  Rev, 12/07/08, GNR - Check OK for 64-bit use                        *
*  ==>, 12/07/08, GNR - Last date before committing to svn repository  *
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

void rksleep(int sec, int usec) {

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
      abexit(66);
   if (select(0,0,0,0,&tv) == -1 && errno != EINTR)
      abexitme(66, "rksleep: select");
   if (sigprocmask(SIG_SETMASK, &oldmask, NULL) == -1)
      abexit(66);

#elif defined(BSD)
   int oldmask, mask;
   struct timeval tv;

   tv.tv_sec  = sec;
   tv.tv_usec = usec;

   mask = sigmask(SIGIO);
   mask |= sigmask(SIGALRM);
   oldmask = sigblock(mask);
   if (select(0,0,0,0,&tv) == -1)
      abexit(66);
   sigsetmask(oldmask);

#endif

   } /* End rksleep() */
