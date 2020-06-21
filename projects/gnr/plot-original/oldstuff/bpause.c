/* (c) Copyright 1990-2011, The Rockefeller University *21114* */
/* $Id: bpause.c 29 2017-01-03 22:07:40Z  $ */
/***********************************************************************
*                              bpause()                                *
*                                                                      *
*  SYNOPSIS:                                                           *
*    int bpause(void)                                                  *
*                                                                      *
*  DESCRIPTION:                                                        *
*  Purpose: Control flow of graphical output to X display.  This       *
*     is part of the "glue" routines that adapt the ROCKS plot calls   *
*     to operate on X and parallel systems.  This routine should be    *
*     called ONLY FROM NODE 0.  Normally, it is called by newplt()     *
*     and by endplt(), and not directly by the user.                   *
*                                                                      *
*  Parameters: None.  All necessary setup parameters are contained     *
*     in struct _RKG and are stored by calls to setmf.                 *
*                                                                      *
*  Action:  If terminate button is pressed, return 1, which signals    *
*     application to terminate.  Otherwise, if in pause mode, wait     *
*     for continue button.  Then, perform requested number of movie    *
*     exposures (2 seconds each) if nexp > 0.                          *
*                                                                      *
*  RETURN VALUES:                                                      *
*    0 if all is well, 1 if user pressed terminate button.             *
************************************************************************
*  Rev, 08/07/92, ROZ - Add conditional compilation NOGDEV             *
*  Rev, 10/01/92, ROZ - Add button control in case mfdraw is running   *
*  Rev, 07/16/93, GNR - Remove snapshot support                        *
*  Rev, 12/14/96, GNR - Remove NCUBE tablet support                    *
*  Rev, 02/20/98, GNR - Examplary code to replace busy wait by rksleep *
*  ==>, 02/27/08, GNR - Last mod before committing to svn repository   *
*  Rev, 08/26/11, GNR - Change iexp long --> int                       *
*  Rev, 10/11/11, GNR - _NCG to _RKG, gmf.h now in glu.h, etc.         *
***********************************************************************/

#include "sysdef.h"
#include "glu.h"
#include "mpitools.h"
#include "rocks.h"

int bpause(void) {

   int rc=0;

#if defined(DEBUG) && DEBUG & MFDBG_SYNCH
   dbgprt("bpause called");
#endif

#ifndef PARn                  /* Make stub if compiled for noden */

   /* If host graphic server is used return only _RKG.intxeq.
   *  Mfdraw takes care of other buttons pressed by user */
   if (_RKG.s.MFMode & SGX) {
      /* Copy mode status returned from mfdraw to local mode switch.
      *  This is currently for consistency only--becomes useful if
      *  plan to work with both sets of buttons is effectuated. */
      if (_RKG.msgbutt.movie) _RKG.movie_mode = _RKG.msgbutt.movie;
      rc = (_RKG.msgbutt.intxeq & (BUT_INTX|BUT_QUIT));
      }

/* If one or more camera exposures requested, execute them now, each
*  with built-in EXPTIME wait.  A little extra wait is added at the
*  beginning to let the picture pop up.
*
*  N.B.  On the NCUBE, exposures were initiated by ringing the bell
*  on the host board.  This code is retained here as a start toward
*  supporting this in some new way, but all it will do as written
*  is ring the bell at the user's terminal.  The time delays were
*  implemented as spin waits, since there was no parallel processing
*  within node 0.  Because this would be very unfriendly on a UNIX
*  system, I have inserted alternative code here using rksleep().
*  This code has not been tested.  -GNR      */

   if (_RKG.nexp > 0) {
#ifdef UNIX
      double fsec = floor(EXPTIME);
      int isec = (int)fsec;
      int usec = (int)(1.0E6*(EXPTIME-fsec));
#endif
      int iexp;
      if (_RKG.movie_device) {      /* Code for beeper */
#ifdef UNIX
         rksleep(0, 240000);        /* Wait 0.24 sec */
         for (iexp=0; iexp<_RKG.nexp; iexp++) {
            fputc('\a',stderr); fflush(stderr); /* Ring bell */
            rksleep(isec, usec);
            } /* End exposure loop */
#else
         double strt = second();    /* Get starting time */
         while ((second() - strt) < 0.24); /* Wait 0.24 sec */
         for (iexp=0; iexp<_RKG.nexp; iexp++) {
            fputc('\a',stderr); fflush(stderr); /* Ring bell */
            strt = second();        /* Get starting time */
            while ((second() - strt) < EXPTIME);
            } /* End exposure loop */
#endif
         } /* End beeper exposures */
      } /* End movie exposures */
#endif

   return rc;
   } /* End of bpause() */
