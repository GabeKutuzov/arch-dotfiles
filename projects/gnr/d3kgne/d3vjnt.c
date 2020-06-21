/* (c) Copyright 1991-2000, Neurosciences Research Foundation, Inc. */
/* $Id: d3vjnt.c 43 2011-02-24 04:00:37Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                           d3vjnt & d3vwdw                            *
*                                                                      *
*            Evaluate virtual joint and window kinesthesia             *
*                                                                      *
************************************************************************
*  Written by George N. Reeke                                          *
*  V4A, 05/22/89, Translated to C                                      *
*  V8A, 04/15/95, GNR - Split into d3vjnt,d3vwdw, one sense per call   *
*  V8B, 12/27/00, GNR - Move to new memory management routines         *
*  ==>, 02/17/05, GNR - Last mod before committing to svn repository   *
***********************************************************************/

#define NEWARB          /* Use new EFFARB mechanism */

#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "sysdef.h"
#include "d3global.h"
#include "jntdef.h"
#include "armdef.h"
#include "wdwdef.h"
#include "rocks.h"

/*---------------------------------------------------------------------*
*                               d3vjnt                                 *
*  N.B.  'pvc' must point to a VCELL block of type VJ_SRC.             *
*---------------------------------------------------------------------*/

void d3vjnt(struct VCELL *pvc) {

   struct ARMDEF *pa = (struct ARMDEF *)pvc->pvcsrc;
   struct JNTDEF *pj, *pje;
   byte          *pv, *pve;         /* Ptrs to output vcells */
   static float    pi = 3.141593;
   static float twopi = 6.283185;
   float anginc, angsig, angle;
   float anumku;
   float hlfinc;
   float tau;
   long lnku;                       /* Number of kinetic units */
   long value8;                     /* (S8) value of cell */

   if (!RP->pbcst) return;          /* A little safety check */
   pv = RP->pbcst + pvc->ovc;
   lnku = pvc->nvx;
   anumku = (float)(lnku - 1);

/* Loop over all normal joints on this arm */

   pje = pa->pjnt + pa->njnts - 1;
   for (pj=pa->pjnt; pj<=pje; pj++) {
      anginc = (pj->u.jnt.jmx - pj->u.jnt.jmn)/anumku;
      angsig = anginc*pvc->vhw;
      angle  = pj->u.jnt.jmn;

/* Loop over all cells reporting for this joint.
*  Value is computed in fs8 format (8 fraction bits).  */

      for (pve=pv+lnku; pv<pve; pv++) {
         register double work;
         work = (double)((angle-pj->u.jnt.ja)/angsig);
         value8 = (long)(256.0*exp(-(work*work)));
         *pv = min(value8, 0xff);
         angle += anginc;
         }  /* End of loop over cells */
      }  /* End of pj loop */

/* If there is a universal joint, make two sets of lnku cells:
*     1) Absolute angle reporters (center cell = upward)
*     2) Angular change reporters (center cell = no change)
*  To center the distributions around zero, the first cell
*     goes at -pi + incr/2 and the increment is 2*pi/lnku.
*  The largest-ever change is kept in ejl and used for scaling.
*  We won't bother to save all these constants in ARMDEF.
*/

   if (pa->jatt & ARMAU) {
      float ampl;
      int i;
      hlfinc = pi/(float)lnku;
      anginc = 2.0*hlfinc;
      angsig = pvc->vhw*anginc;
      ampl   = (float)sqrt(
         (double)(pj->u.ujnt.ujdxn*pj->u.ujnt.ujdxn) +
         (double)(pj->u.ujnt.ujdyn*pj->u.ujnt.ujdyn));
      pj->jl = max(pj->jl,ampl);
      if (ampl > 0.0) ampl = 256.0*ampl/pj->jl;
      for (i=0; i<=1; i++) {
         angle = hlfinc - pi;
         tau = (i) ? pj->u.ujnt.ujdtau : pj->u.ujnt.ujtau ;
         for (pve=pv+lnku; pv<pve; pv++) {
            register double work;
            register float arg;
            arg = (angle >= tau) ? angle - tau : tau - angle ;
            if (arg > pi) arg = twopi - arg;
            work = (double)(arg/angsig);
            value8 = (long)(ampl*exp(-(work*work)));
            *pv = min(value8, 0xff);
            angle += anginc;
            } /* End loop over cells */
         } /* End loop over two degrees of freedom */
      } /* End if universal joint */
   } /* End d3vjnt */


/*---------------------------------------------------------------------*
*                               d3vwdw                                 *
*  N.B.  'pvc' must point to a VCELL block of type VW_SRC.             *
*---------------------------------------------------------------------*/

void d3vwdw(struct VCELL *pvc) {

   /* Note: pwx = ptr to driven window, used when kchng==WDWLCK
   *  to locate range of motion allowed for the window that is
   *  actually driven.  */
   struct WDWDEF *pwx,*pw = (struct WDWDEF *)pvc->pvcsrc;
   byte          *pv, *pve;         /* Ptrs to output vcells */
   static float twopi = 6.283185;
   float anginc, angsig, angle;
   float anumku;
   float arange;
   float rnsx, rnsy;                /* Floated nsx, nsy */
   float tau;
   long lnku;                       /* Number of kinetic units */
   long value8;                     /* (S8) value of cell */
   int ij;                          /* DOF loop index */

   rnsx = (float)RP->nsx;
   rnsy = (float)RP->nsy;

   if (!RP->pbcst) return;          /* A little safety check */
   pv = RP->pbcst + pvc->ovc;
   lnku = pvc->nvx;
   anumku = (float)(lnku-1);

   pwx = (pw->kchng == WDWLCK) ? pwx = pw->pdrive : pw;

/* Loop over the 5 possible kinds of motion to be reported.
*  Set range (arange) and current value (tau) for each.  */

   for (ij=1; ij<=pvc->nvy; ij++) {

      /* Pick up correct parameters for each type of arbor */
      switch (ij) {

case 1:  /* Arbor 1: X translation */
         arange = rnsx - pwx->wwd;
         tau = pw->wx + 0.5*(pw->wwd - pwx->wwd);
         break;

case 2:  /* Arbor 2: Y translation */
         arange = rnsy - pwx->wht;
         tau = pw->wy - 0.5*(pw->wht + pwx->wht);
         break;

case 3:  /* Arbor 3: X dilation */
         arange = rnsx;
         tau = pw->wwd;
         break;

case 4:  /* Arbor 4: Y dilation */
         arange = rnsy;
         tau = pw->wht;
         break;

case 5:  /* Arbor 5: Rotation--not yet implemented, set tau = 0.0 */
         arange = twopi;
         tau = 0.0;
         break;
         }  /* End of switch */

      anginc = arange/anumku;
      angsig = anginc*pvc->vhw;
      angle = 0.0;

/* Loop over all cells reporting for this parameter.
*  Value is computed in fs8 format (8 fraction bits).  */

      for (pve=pv+lnku; pv<pve; pv++) {
         register double work;
         work = (double)((angle-tau)/angsig);
         value8 = (long)(256.0*exp(-(work*work)));
         *pv = min(value8, 0xff);
         angle += anginc;
         }  /* End of loop over cells */

      } /* End loop over degrees of freedom */

   } /* End d3vwdw() */
