/* (c) Copyright 1991-2012, Neurosciences Research Foundation, Inc. */
/* $Id: d3epsi.c 51 2012-05-24 20:53:36Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                               d3epsi                                 *
*                                                                      *
*  Plan repertoire plot positions in superposition plot                *
*  Call every time d3grp3 is called, as plot options can change        *
*                                                                      *
************************************************************************
*                                                                      *
*  Written by George N. Reeke                                          *
*  V4A, 05/10/89, Translated from version V2B                          *
*  V6D, 02/03/94, GNR - Add default vxo,vyo setting                    *
*  V8A, 01/11/98, GNR - Height/width ratio same for strat/interleaved  *
*  V8D, 06/05/07, GNR - Add default sphgt, spwid                       *
*  ==>, 12/28/07, GNR - Last mod before committing to svn repository   *
*  V8H, 05/23/12, GNR - Increase default vyo to include 1.5*stdlht     *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "sysdef.h"
#include "d3global.h"
#include "rocks.h"

void d3epsi(void) {

   struct REPBLOCK *ir;
   float lhta;             /* Left height available */
   float lhtn;             /* Left height needed */
   float lhtr;             /* Left height ratio */
   float lwda;             /* Left width available */
   float lwdr;             /* Left width ratio */
   float ratio;            /* Ratio of height to width */
   int kase;
   byte kxyz,kxyg;         /* Flags for X,Y zero vs X,Y given */

/* Preparations for environmental plot:
*  If user did not enter sphgt, spwid, now default.  (The plan
*     is for these values to be calculated eventually from
*     the positions of all objects in the superposition plot.)
*  If user did not enter eply,eplh, now default from nsx,nsy
*  This code allows user to enter either, neither, or both.  */

   if (RP->sphgt <= 0.0) RP->sphgt = 10.0;
   if (RP->spwid <= 0.0) RP->spwid = 11.0;

   if (RP->eplw < 0.0 && RP->eplh <= 0.0) {
      if (RP->nsx >= RP->nsy) RP->eplw = 5.0;
      else RP->eplh = 5.0;
      }
   if (RP->eplh <= 0.0)
      RP->eplh = RP->eplw*(float)RP->nsy/(float)RP->nsx;
   if (RP->eplw < 0.0)
      RP->eplw = RP->eplh*(float)RP->nsx/(float)RP->nsy;
   if (RP->eply < 0.0)
      RP->eply = 10.5 - RP->eplh;

/* Initial values */

   kxyg = FALSE;
   kxyz = FALSE;
   lwda = 2.5;
   lhta = 10.5;

/* Loop over repertoires, summing requirements.
*  Note: aplx, etc. set to -1 in d3dflt to trigger automatic plots */

   lhtn = 0.0;
   for (ir=RP->tree; ir!=NULL; ir=ir->pnrp) {
      if (!(ir->Rp.krp & (KRPPL|KRPBP))) continue;

      /* Mark presence of X,Y for consistency check */
      if (ir->aplx < 0.0) kxyz = TRUE;
      else                kxyg = TRUE;
      if (ir->aply < 0.0) kxyz = TRUE;
      else                kxyg = TRUE;

      /* Calculate height/width ratio */
      ratio = (float)(ir->ngy*ir->nqy)/(float)(ir->ngx*ir->nqx);

      /* Handle the eight cases described in the writeup */
      kase = 1;
      if (ir->aplw > 0.0) kase += 2;
      if (ir->aplh > 0.0) kase++;
      switch (kase) {

case 4:  /* 4) Both W and H entered */
         if (!kxyz) break;
         ratio = ir->aplh/ir->aplw;
         /*... fall into case 3 ...*/
case 3:  /* 3) Only W entered */
         if (kxyz) ir->aplw = min(ir->aplw,2.5);
         ir->aplh = ir->aplw*ratio;
         break;

case 2:  /* 2) Only H entered */
         if (kxyz) ir->aplh = min(ir->aplh,2.5*ratio);
         ir->aplw = ir->aplh/ratio;
         break;

case 1:  /* 1) Neither W nor H entered */
         ir->aplw = 2.5;
         ir->aplh = ir->aplw*ratio;
         break;
         } /* End of switch */

/* Allow extra 0.20*width for labels.
*  Add into left or right requirement according to 'W' bit.  */

      lhtn += (ratio + 0.20)*lwda;
      } /* End of ir loop */

/* All repertoires scanned.  Now do consistency check on X,Y.  */

   if (kxyg) {
      if (kxyz) {
         cryout(RK_E1,"0***PLOT X,Y MUST BE INPUT OR DEFAULT,"
            " SAME FOR ALL REGIONS/REPERTOIRES.",RK_LN2,NULL);
         return;
         }
      }

/* Rest of program executed only if X,Y coords all defaulted.
*  Have total requirements.  Now decrease the widths if needed
*     to accomodate very high plots.
*  Then, if shrinkage is to less than 80% of original width,
*     actually use the smaller width in the plots also.
*  Code must avoid divide check when no graph on a side.  */

   else {
      lhtr = 1.0;
      if (lhtn > lhta) lhtr = lhta/lhtn;
      lwdr = 1.0;
      if (lhtr < 0.8) lwdr = lhtr;

      /* Make a second pass, storing parameters in repertoire blocks.
      *  Here use lhta for running lower-left Y coord of boxes. */

      for (ir=RP->tree; ir!=NULL; ir=ir->pnrp) {
         if (ir->Rp.krp & (KRPPL|KRPBP)) {
            /* Assign parameters for plotting on the left */
            ir->aplx = 0.25;
            ir->aplh = lhtr*ir->aplh;
            ir->aply = lhta - ir->aplh;
            ir->aplw = lwdr*ir->aplw;
            lhta = ir->aply - 0.20*ir->aplw;
            }
         } /* End of ir loop */
      } /* End KXYG else */

/* Now that aplx,aply,aplh,aplw have been assigned,
*  make another pass to assign default KP=V offsets.
*  This is done whether or not KP=V actually requested,
*  because option may be turned on at d3chng time.  */

   for (ir=RP->tree; ir!=NULL; ir=ir->pnrp) {
      if (ir->vxo==0.0 && ir->vyo==0.0)
         ir->vyo = -(ir->aplh + 1.5*RP->stdlht);
      } /* End KP=V loop */

   } /* End d3epsi() */
