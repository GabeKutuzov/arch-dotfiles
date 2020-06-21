/* (c) Copyright 1991-2014, The Rockefeller University *11115* */
/* $Id: d3resetz.c 78 2018-08-02 18:36:58Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                              d3resetz                                *
*                                                                      *
*  Contains:   Function to reset darwin3 data structures on host node. *
*              Should be called from host or serial version only.      *
*  Notes:      Function d3rset was broken into separate routines to    *
*              handle repertoire data (d3resetn) and everything else   *
*              (d3resetz).  This is necessary so that the following    *
*              sequence of events can happen when the a reset is       *
*              initiated unpredictably by a BBD device:                *
*                 (1) bbdsgetv returns value that initiates reset.     *
*                 (2) Non-repertoire stuff is reset by d3resetz.       *
*                 (3) Sensory cells are updated (may depend on         *
*                       updated arm/window/value data).                *
*                 (4) This stuff, including updated RP array, is       *
*                       broadcast to other nodes, which find out for   *
*                       first time that they are supposed to reset.    *
*                 (5) Repertoire stuff is reset by d3resetn.           *
*              Data in value blocks are updated by both routines.      *
*              In the serial version, both routines are called in turn.*
*                                                                      *
************************************************************************
*  New, 08/06/91, GNR - Separate d3rset into d3resetz and d3resetn     *
*  V6D, 02/08/94, GNR - Revise initsi call for delays                  *
*  V8A, 04/15/95, GNR - Merge kchng,flags into WDWDEF                  *
*  Rev, 08/14/96, GNR - Call d3zval()                                  *
*  V8D, 09/05/05, GNR - New reset options                              *
*  ==>, 12/29/07, GNR - Last mod before committing to svn repository   *
*  V8E, 01/26/09, GNR - Remove node 0 repertoire updates to d3resetn   *
*                       to simplify & correct bug when iti < mxsdelay1 *
*  V8F, 02/20/10, GNR - Remove platform resets                         *
*  Rev, 05/22/10, GNR - Add camera reset                               *
*  Rev, 06/07/12, GNR - Add adefer per conntype                        *
*  Rev, 08/22/14, GNR - Si only EXP decay, remove DECAYDEF from DCYDFLT*
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "sysdef.h"
#include "d3global.h"
#include "jntdef.h"
#include "armdef.h"
#include "wdwdef.h"
#include "tvdef.h"
#include "rocks.h"
#include "rkxtra.h"

/*---------------------------------------------------------------------*
*                                                                      *
*                              d3resetz                                *
*                                                                      *
*  This routine resets arms, windows, values, changes, and cameras.    *
*                                                                      *
*  Argument:  rl is the current reset level                            *
*     kns is temporary to distinguish real series reset from event     *
*---------------------------------------------------------------------*/

void d3resetz(int rl, int kns) {

#ifndef PARn
   struct CELLTYPE *il;
   struct CONNTYPE *ix;
   struct ARMDEF   *pa;
   struct WDWDEF   *pw;
   struct TVDEF    *cam;

if (kns) {

/* Stage 1: Reset/rewind user cameras */

   if (rl >= RL_SERIES) {
      for (cam=RP->pftv; cam; cam=cam->pnxtv) {
         if ((cam->utv.tvsflgs & (TV_ON|TV_UPGM)) == (TV_ON|TV_UPGM) &&
               !(cam->utv.tvuflgs & TV_BYPR)) {
            cam->tvinitfn(&cam->utv, getrktxt(cam->hcamnm),
               cam->hcamuf ? getrktxt(cam->hcamuf) : NULL,
               NULL, TV_RESET);
            }
         }
      }

   } /*** End if kns ***/

else {

/* Stage 2: Reset structs for all arms & their joints */

   if (rl >= RP0->rlvl[lvarm])
         for (pa = RP0->parm1; pa; pa = pa->pnxarm) {

      struct JNTDEF *pj,*pje;

      /* Test for arm reset bypass */
      if (pa->jatt & ARMBY)
      /* If bypass, then if bypass override, */
         if (pa->jatt & ARMOR)
            /* Clear override flag and do reset */
            pa->jatt &= ~ARMOR;
         else
            /* Else do the bypass (go to next arm) */
            continue;

      /* Clear trace and gription-on/off flags in arm attributes */
      pa->jatt &= ~(ARMTR | ARMGP);

      /* Reset joint angles to initial values for all joints in arm */
      pje = pa->pjnt + pa->njnts;
      for (pj = pa->pjnt; pj<pje; pj++)
         pj->u.jnt.ja = pj->u.jnt.ja0;

      /* Is there a universal joint ? */
      if (pa->jatt & ARMAU) {
         pj->u.ujnt.ujct =
         pj->u.jnt.jx =
         pj->u.jnt.jy =
         pj->u.ujnt.ujdxn =
         pj->u.ujnt.ujdyn =
         pj->u.ujnt.ujtau =
         pj->u.ujnt.ujdtau = 0;
         }

      /* If a gripper is present, zero the grip count */
      if (pa->jatt & ARMGE)
         (pj + 1)->u.grip.grct = 0;

      } /* End loop over arms */

/* Stage 3: For all windows, reset x, y, w and h to input values */

   if (rl >= RP0->rlvl[lvwin])
         for (pw = RP0->pwdw1; pw; pw = pw->pnxwdw) {

      /* Check for reset bypass */
      if (pw->flags & WBWOPBR)
         /* If bypass set, then if bypass override set, */
         if (pw->flags & WBWOPOR)
            /* Clear override flag and reset */
            pw->flags &= ~WBWOPOR;
         else
            /* Else do the bypass */
            continue;

      pw->wx = pw->wx0;
      pw->wy = pw->wy0;
      pw->wwd = pw->wwd0;
      pw->wht = pw->wht0;
      } /* End loop over windows */

/* Stage 4: Reset value schemes */

   if (rl >= RP0->rlvl[lvval] && RP->nvblk > 0) d3zval(VBVOPBR);

/* Stage 5: Zero the changes array */

   if (RP0->ljnwd > 0)
      memset((char *)RP0->pjnwd, 0, sizeof(float)*RP0->ljnwd);

/* Issue warning (just once/run) if there is immediate amplification
*  and s(i) is being reset by omega1 (level 2) or decayed by Dc.omega
*  (level 3) for any particular celltype.  (The actual decay is done
*  in d3resetn.)  Test must be done here, not in d3news, because rl
*  depends on timers, etc.  and we loop over all conntypes because
*  adefer can change independently for any of them.  */

   if (RP0->n0flags & N0F_ANYAMP) {
      if (rl == RL_OLDTRIAL) {
         for (il=RP->pfct; il; il=il->pnct) {
            if (il->Dc.omega1 != S15 && !(il->ctf & DIDIMAW2)) {
               for (ix=il->pct1; ix; ix=ix->pct) {
                  if (ix->Cn.adefer == 0) {
            cryout(RK_P1, "0-->WARNING:  Immediate amplification can "
               "produce unexpected results (will use decayed s(i))",
               RK_LN3, "    with OMEGA1 decay and OLDTRIAL reset--",
               RK_LN0, fmtlrlnm(il), RK_CCL, NULL);
                     il->ctf |= DIDIMAW2;
                     break;
                     } /* End if adefer == 0 */
                  } /* End conntype loop */
               } /* End nonzero decay and no message yet */
            } /* End celltype loop */
         } /* End old trial reset */
      else if (rl == RL_TRIAL) {
         for (il=RP->pfct; il; il=il->pnct) {
            if (il->Dc.omega && !(il->ctf & DIDIMAW3) &&
                  !(il->Ct.ctopt & OPTBR)) {
               for (ix=il->pct1; ix; ix=ix->pct) {
                  if (ix->Cn.adefer == 0) {
            cryout(RK_P1, "0-->WARNING:  Immediate amplification can "
               "produce unexpected results (will use decayed s(i))",
               RK_LN3, "    with OMEGA decay and TRIAL reset--",
               RK_LN0, fmtlrlnm(il), RK_CCL, NULL);
                     il->ctf |= DIDIMAW3;
                     break;
                     } /* End if adefer == 0 */
                  } /* End conntype loop */
               } /* End nonzero decay and no message yet */
            } /* End celltype loop */
         } /* End trial reset */
      } /* End ANYAMP */

   } /*** End else !kns ***/

   RP->CP.trialsr = 0;        /* Reset trial counter */
#endif /* !PARn */
   } /* End d3resetz() */
