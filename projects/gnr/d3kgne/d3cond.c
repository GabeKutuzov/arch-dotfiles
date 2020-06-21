/* (c) Copyright 2005-2007, Neurosciences Research Foundation, Inc. */
/* $Id: d3cond.c 3 2008-03-11 19:50:00Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                               d3cond                                 *
*                                                                      *
*      Calculate contribution of composite conductances to cell        *
*                 state and update ion concentrations                  *
*                                                                      *
************************************************************************
*  V8D, 08/28/05, GNR - New routines                                   *
*  ==>, 12/28/07, GNR - Last mod before committing to svn repository   *
***********************************************************************/

#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <math.h>
#include "sysdef.h"
#include "rksubs.h"
#include "rkarith.h"
#include "d3global.h"
#include "celldata.h"

extern struct CELLDATA CDAT;


/*=====================================================================*
*                               cndshft                                *
*                                                                      *
*  Shifts all the stored g values for a given conductance right by     *
*  s places and increments the igi scale accordingly.                  *
*                                                                      *
*  Synopsis:   void cndshft(struct CELLTYPE *il, struct CONDUCT *pcnd, *
*     int s)                                                           *
*                                                                      *
*  Arguments:                                                          *
*     il       Ptr to cell type of interest                            *
*     pcnd     Ptr to conductance block of interest                    *
*     s        Number of bits to shift                                 *
*                                                                      *
*  Note:  This looks like a bad plan, but it should happen very        *
*  infrequently during the course of a given run.                      *
*=====================================================================*/

void cndshft(struct CELLTYPE *il, struct CONDUCT *pcnd, int s) {

   rd_type *pr,*pre;
   ui32 g;
   long llel = il->lel;

   /* Scale the conductance data stored with the cell data */
   pr  = il->prd + il->ctoff[CNDG] + pcnd->ogrd;
   pre = il->prd + il->llt;
   for ( ; pr<pre; pr+=llel) {
      d3gtl3(g, pr);
      g >>= s;
      d3ptl3(g, pr);
      }

   /* Scale the full event data (if present) by the same amount */
   /* $$$ THIS CODE NOT WRITTEN YET $$$ */


   pcnd->ugi.igi += s;

   } /* End cndshft() */


/*=====================================================================*
*                               d3cond                                 *
*                                                                      *
*  Evaluates all ionic conductances and updates ion concentrations     *
*  for the current cell.                                               *
*                                                                      *
*  Synopsis: long d3cond(struct CELLTYPE *il)                          *
*                                                                      *
*  Returns:                                                            *
*     Cell response (mV S20)                                           *
*                                                                      *
*  NOTE: g values are FBgt = S30                                       *
*=====================================================================*/

long d3cond(struct CELLTYPE *il) {

   struct CONDUCT *pcnd;         /* Ptr to current conductance */
   rd_type  *pr;                 /* Ptr to conductance rep data */

   si32  gt;                     /* Temporal conductance factor */
   si32  si = CDAT.old_si;       /* si(t-1) (S20/27) */
   ui32  nhits;                  /* Number of hits at last event */
   ui32  tlhit,etime;            /* Time of last hit, expiry time */

   int   KeepEvent;              /* TRUE to retain nhits longer */

   static ui32 zero = 0;

   for (pcnd=il->pcnd1; pcnd; pcnd=pcnd->pncd) {
      pr  = il->prd + il->ctoff[CNDG] + pcnd->ogrd;
      KeepEvent = FALSE;

/* Get last hit information if present */

      if (pcnd->cdflgs & CDFL_HITNT) {
         d3gtl2(nhits, pr+ognh);
         d3gtl2(tlhit, pr+ogth);
         }

/*---------------------------------------------------------------------*
*               Switch according to type of conductance                *
*---------------------------------------------------------------------*/

      switch (pcnd->kcond) {

/*---------------------------------------------------------------------*
*                         PASSIVE CONDUCTANCE                          *
*---------------------------------------------------------------------*/

      case CDTP_PASSIVE:

         gt = S30;
         KeepEvent = TRUE;    /* Because nhits doesn't exist */
         break;

/*---------------------------------------------------------------------*
*               LINEAR (VOLTAGE-DEPENDENT) CONDUCTANCE                 *
*---------------------------------------------------------------------*/

      case CDTP_LINEAR: {

         /* Note: d3news assures that vmax > vth,
         *  so the division can never fail. */
         si32  dsi = si - pcnd->ugt.lin.vth;
         if      (dsi <= 0)                gt = 0;
         else if (si > pcnd->ugt.lin.vmax) gt = S30;
         else gt = jduwq(jsluw(jcuw(0, (ui32)dsi), FBgt),
            pcnd->ugt.lin.vmmth);
         KeepEvent = TRUE;    /* Because nhits doesn't exist */
         break;
         } /* End CDTP_LINEAR local scope */

/*---------------------------------------------------------------------*
*           GATED (ALPHA or DOUBLE-EXPONENTIAL) CONDUCTANCE            *
*---------------------------------------------------------------------*/

      case CDTP_ALPHA:
      case CDTP_DOUBLE: {

         int   GotHit;

/* Check for being in a refractory period */

         if (pcnd->refrac && nhits) {
            etime = (tlhit | (RP->CP.effcyc & ~UI16_MAX)) +
               pcnd->refrac;
            if (RP->CP.effcyc <= etime) {
               /* Refractory */
               KeepEvent = TRUE;
               goto NoHitsNow;
               }
            } /* End checking for refractory period */

/* Determine whether an activation event has occurred */

         switch (pcnd->kactv) {

         case CDAC_CELLFIRE:

            GotHit = si >= pcnd->uga.at;
            break;

         case CDAC_SYNAPTIC:

            GotHit = CDAT.psumaij[pcnd->iact-1] >= pcnd->uga.at;
            break;

         case CDAC_IONIC:
            
            
            break;
            
         case CDAC_POISSON:


            break;

            } /* End switch over activation types */








         break;
         } /* End GATED local scope */


         } /* End switch over conductance types */

/*---------------------------------------------------------------------*
*                Common code for all conductance types                 *
*---------------------------------------------------------------------*/


NoHitsNow:  ;


      if (!KeepEvent) d3ptl2(zero, pr+ognh);
      } /* End loop over conductances */


   } /* End d3cond() */

