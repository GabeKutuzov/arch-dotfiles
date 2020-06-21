/* (c) Copyright 1991-2011, Neurosciences Research Foundation, Inc. */
/* $Id: d3resetz.c 46 2011-05-20 20:14:09Z  $ */
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
*  History:                                                            *
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
*  V8H, 07/30/11, GNR - Major rewrite of RESET controls                *
***********************************************************************/

#define NEWARB          /* Use new EFFARB mechanism */
#define TVTYPE  struct TVDEF

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
*  The actions to be performed are specified in RP->kra[RLact].        *
*                                                                      *
*  Argument:                                                           *
*     kns      TRUE if this is a call at the start of a new trial      *
*              series before the first trial (in order to reset        *
*              cameras if necessary before any images are acquired).   *
*              FALSE if this is a normal call before a trial.          *
*---------------------------------------------------------------------*/

void d3resetz(int kns) {

#ifndef PARn
   struct CELLTYPE *il;
   struct ARMDEF   *pa;
   struct WDWDEF   *pw;
   struct TVDEF    *cam;
   int    rl = RP->kra[RLact].rlev;
   int    count;

$$$ TO HERE

if (kns) {

/* Stage 1: Reset/rewind user cameras */

   if (rl >= RL_SERIES) {
      for (cam=RP->pftv; cam; cam=cam->pnxtv) {
         if ((cam->utv.tvsflgs & (TV_ON|TV_UPGM)) == (TV_ON|TV_UPGM) &&
               !(cam->utv.tvuflgs & TV_BYPR)) {
            cam->tvinitfn(&cam->utv, getrktxt(cam->hcamnm),
               NULL, NULL, TV_RESET);
            }
         }
      /* TEST CODE -- RESET RESCALING SEED -- MAY NEED WAY TO BYPASS */
      if (RP0->ptvrsh) RP0->ptvrsh[MAXGRAY] = RS_SEED;
      }

   } /*** End if kns ***/

else {

/* Stage 2: Reset structs for all arms & their joints */

   if (rl >= RP0->rlvl[lvarm])
         for (pa = RP0->parm1; pa; pa = pa->pnxarm) {

      register struct JNTDEF *pj;

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
      for (pj = pa->pjnt, count = 0; count < pa->njnts; pj++, count++)
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

/* Warning (just once/run) if there is immediate amplification and
*  s(i) is being reset by omega1 (level 2) or by CDCY.omega (level
*  3) for any particular celltype.  (The actual decay is done in
*  d3resetn.)  Test must be done here, not in d3news, because rl
*  depends on timers, etc.  */

   if (RP->adefer == 0 && RP0->n0flags & N0F_ANYAMP)
         for (il=RP->pfct; il; il=il->pnct) {
      if (rl == RL_OLDTRIAL) {
         if (il->Dc.omega1 != S30 && !(il->ctf & DIDIMAW2)) {
            cryout(RK_P1, "0-->WARNING:  Immediate amplification can "
               "produce unexpected results (will use decayed s(i))",
               RK_LN3, "    with OMEGA1 decay and OLDTRIAL reset--",
               RK_LN0, fmtlrlnm(il), RK_CCL, NULL);
            il->ctf |= DIDIMAW2;
            }
         }
      else if (rl == RL_TRIAL) {
         if (il->Dc.CDCY.omega != S30 && !(il->ctf & DIDIMAW3) &&
               !(il->Ct.ctopt & OPTBR)) {
            cryout(RK_P1, "0-->WARNING:  Immediate amplification can "
               "produce unexpected results (will use decayed s(i))",
               RK_LN3, "    with OMEGA2 decay and TRIAL reset--",
               RK_LN0, fmtlrlnm(il), RK_CCL, NULL);
            il->ctf |= DIDIMAW3;
            }
         }
      }

   } /*** End else !kns ***/

   RP->CP.trialsr = 0;        /* Reset trial counter */
#endif /* !PARn */
   } /* End d3resetz() */
