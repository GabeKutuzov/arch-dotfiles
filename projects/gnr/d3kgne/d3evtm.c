/* (c) Copyright 2001-2008, Neurosciences Research Foundation, Inc. */
/* $Id: d3evtm.c 7 2008-05-02 22:16:44Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                              d3evtm.c                                *
*                                                                      *
*                        Evaluate trial timers                         *
*                                                                      *
*  This file contains two routines used for evaluating trial timers:   *
*                                                                      *
*  (1) void d3evt1(void)                                               *
*     This routine evaluates all timers according to their trial       *
*     number and event trial criteria.  Keeping this separate from     *
*     evaluation of stimulus number criteria allows trial number       *
*     criteria to be used for timing placement of stimuli in the       *
*     environment.                                                     *
*                                                                      *
*  (2) void d3evt2(void)                                               *
*     This routine applies stimulus number criteria to trial timers.   *
*     Timers are set OFF if stimuli were specified and none of those   *
*     stimuli are present in any specified modality.  Timers stay ON   *
*     if no stimuli or modalities were specified.                      *
*                                                                      *
*  These routines are called only on host node or in serial version.   *
*                                                                      *
*  V8B, 01/21/01, GNR - New routines                                   *
*  ==>, 02/17/05, GNR - Last mod before committing to svn repository   *
*  Rev, 03/28/08, GNR - Add EVRELTN, EVENT=ON|OFF|LIST                 *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "d3global.h"
#include "bapkg.h"

/*---------------------------------------------------------------------*
*                               d3evt1                                 *
*---------------------------------------------------------------------*/

void d3evt1(void) {

   struct TRTIMER *ptt;          /* Ptr to current timer */

   /* The first timer is timer '0', which is always OFF.  The
   *  others, including those not specified, are always ON.  */
   RP->CP.trtimer[0] = 0x7f;
   memset((char *)RP->CP.trtimer+1, 0xff, BytesInXBits(MXNTRT)-1);
   for (ptt=RP0->ptrt1; ptt; ptt=ptt->pntrt) {
      /* If this is a special event trial and there is a timer
      *  override, set or clear the timer now.  */
      if (RP->CP.runflags & RP_EVTRL) {
         if (ptt->tmrop & TRTEVON)        continue;
         if (ptt->tmrop & TRTEVOFF)       goto SetTimerOff;
         }
      /* Otherwise, if there is a list, apply it.
      *  N.B.  ilsttest() is used rather than ilstiter()
      *  to allow for the possibility that trials may be
      *  accomplished out-of-order during a replay.  */
      if (ptt->ptil) {
         long trial =
            (ptt->tmrop & TRTREL) ? RP0->trialse : RP->CP.trial;
         if (ilsttest(ptt->ptil, trial))  continue;
         else                             goto SetTimerOff;
         }
      /* Finally, if not a special event trial, and there is
      *  no trial list, timer stays on unless TRTEVON option
      *  was specified, in which case turn it off, because
      *  otherwise it would just always be on and useless.  */
      if (!(ptt->tmrop & TRTEVON))        continue;
SetTimerOff:
      bitclr(RP->CP.trtimer, ptt->jtno);
      } /* End timer loop */

   } /* End d3evt1() */


/*---------------------------------------------------------------------*
*                               d3evt2                                 *
*---------------------------------------------------------------------*/

void d3evt2(void) {

   struct MODALITY *pmdlt;       /* Ptr to current modality */
   struct TRTIMER  *ptt;         /* Ptr to current timer */
   int i;

   for (ptt=RP0->ptrt1; ptt; ptt=ptt->pntrt)
         if (ptt->psil && ptt->smdlts) {
      for (pmdlt=RP->pmdlt1; pmdlt; pmdlt=pmdlt->pnmdlt)
            if (bittst((byte *)&ptt->smdlts, (long)pmdlt->mdltno)) {
         for (i=0; i<pmdlt->ndsbyt; i++)
            if (pmdlt->pxcyc[i] & ptt->timsm[i])
               goto GotStimMatch;   /* Break out of two loops */
         } /* End modality loop */
      bitclr(RP->CP.trtimer, ptt->jtno);
GotStimMatch: ;
      } /* End timer loop */

   } /* End d3evt2() */

