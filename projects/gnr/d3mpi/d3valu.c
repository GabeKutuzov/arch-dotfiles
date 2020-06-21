/* (c) Copyright 1989-2018, The Rockefeller University *21116* */
/* $Id: d3valu.c 77 2018-03-15 21:08:14Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                              d3valu.c                                *
*            Routines for management of CNS value schemes              *
*                                                                      *
*  Call: d3zval(int mzv)                                               *
*     Where: 'mzv' = mask bits for bypassing value reset.              *
*                                                                      *
*     This routine zeros values in memory and sets VBVOPKV bit in      *
*  VBDEF blocks for special handling of value in next cycle.  Call     *
*  at start of a new series and at reset time.  Reset is skipped if    *
*  any bits matching mzv bits are on in vbopt flags (e.g. VBVOPBR).    *
*                                                                      *
*  Call: d3vprt(void)                                                  *
*                                                                      *
*     This routine prints current state of all value schemes to        *
*  implement DP=V option of CYCLE card.                                *
*                                                                      *
*  Call: d3valu(struct VBDEF *pvbt)                                    *
*     Where: 'pvbt' = Ptr to start of rep or env VBDEF thread.         *
*                                                                      *
*     This routine calculates current values of the requested type.    *
*  The environment type thread may be processed on node 0 only.        *
************************************************************************
*  Written by George N. Reeke, Jr.                                     *
*  V4A, 03/03/89, Translated to C from version V3A                     *
*  Rev, 11/02/89, JMS - div0 bug in TENT, NFIRE cases fixed            *
*  Rev, 08/05/91, GNR - Add SNOUT method, reformat file                *
*  V5C, 12/05/91, GNR - Add vbase parameter to several schemes         *
*  Rev, 02/15/92, GNR - Add D1ENV and D1CAT methods                    *
*  Rev, 03/26/92, GNR - Add AVGSI value scheme                         *
*  Rev, 08/08/92, GNR - Use SRA where necessary                        *
*  V5F, 11/10/92, GNR - Remove ENVIRONMENT_TYPE code on comp nodes     *
*  V6D, 02/08/94, GNR - Adjust for axonal delays                       *
*  V8A, 07/20/95, GNR - Get ENVVAL from modality block, remove D1ENV   *
*  Rev, 08/14/96, GNR - Zero vals on new series, add VBVOPAV,VBVOPFS,  *
*                       d3zval, d3vprt, remove pvdat from call args    *
*  Rev, 03/01/97, GNR - Eliminate d3exch(), all s(i) now on host node  *
*  V8B, 12/30/00, GNR - Use separate threads for rep, env values       *
*  V8C, 03/22/03, GNR - Cell responses in millivolts, add conductances *
*  V8D, 05/22/07, GNR - Add BBD value type, correct scaling bug        *
*  Rev, 12/22/07, GNR - Handle resets in d3zval for more consistent    *
*                       treatment as documented in Group III writeup   *
*  ==>, 12/24/07, GNR - Last mod before committing to svn repository   *
*  Rev, 05/11/08, GNR - Add vdelay, change fast start reset value      *
*  Rev, 12/31/08, GNR - Replace jm64sl,jm64sh with msrsle              *
*  V8F, 02/20/10, GNR - Remove PLATFORM (SNOUT value) support          *
*  V8I, 12/04/12, GNR - Recode calcs for better overflow checking      *
*  R67, 04/27/16, GNR - Remove Darwin 1 support                        *
*  R76, 10/20/17, GNR - Add CONSTANT value, longs to si32s             *
*  R77, 02/21/18, GNR - Use mrsrsl for damping that cannot overflow    *
***********************************************************************/

#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "rkarith.h"
#include "d3global.h"
#include "armdef.h"
#include "jntdef.h"
#include "wdwdef.h"
#ifndef PARn
#include "rocks.h"
#endif

/* TEST OPTION:  Define OLDDAMP to restore action of this program
*  prior to V8D, Revision 7, when vdampprv was always set to 0 on
*  a reset, whether environmental or repertoire type.  */

/*   #define OLDDAMP   */

#ifndef PARn
/* Names of VALUE scheme options for d3grp1, d3echo, etc. */
   const char *vbtnames[] = { "TENT", "NFIRE", "AVGSI",
      "HHDIST", "HHRING", "HHDLIN", "HHRLIN", "UJMODE",
      "GRIP", "ENVVAL", "CONSTANT", "==",    /* Code 11 is available */
#ifdef BBDS
      "BBD",
#else
      "==",
#endif
      } ;
#endif

/*---------------------------------------------------------------------*
*     xhhxy - macro to obtain X,Y components of head-hand distance     *
*---------------------------------------------------------------------*/

#define xhhxy(A,B,ivb) (A = (ivb)->u1.vhh.mvwdw->wx + \
                            (ivb)->u1.vhh.mvwdw->wwd/2.0 - \
                            (ivb)->u1.vhh.mvarm->armx, \
                        B = (ivb)->u1.vhh.mvwdw->wy - \
                            (ivb)->u1.vhh.mvwdw->wht/2.0 - \
                            (ivb)->u1.vhh.mvarm->army)


/*---------------------------------------------------------------------*
*                               d3zval                                 *
*  Perform value reset:  If repertoire value, zero all variables for   *
*  current cycle (new value not yet determined).  If ENV or BBD, etc,  *
*  Vc(t-1) is new current value, damped value is 0 if relative or, if  *
*  absolute, is scaled new value (damped if not fast start).  Value    *
*  returned is same, possibly delayed.  (Caller is expected to test    *
*  for RP->nvblk > 0, but no harm if this is not done.)                *
*---------------------------------------------------------------------*/

void d3zval(int mzv) {

   register struct VBDEF *ivb;
   register struct VDTDEF *val;
   byte bypass_mask = (byte)mzv;

   for (ivb=RP->pvblk,val=RP->pvdat; ivb; ivb=ivb->pnxvb,val++) {
      if (ivb->vbopt & bypass_mask) continue;
      /* Repertoire value -- new value not known, zero everything */
      if (ivb->vbkid == VBK_REP) {
         memset((char *)val, 0, sizeof(struct VDTDEF));
         /* Have d3valu undergo post-reset startup next time */
         ivb->vbflg |= VBFLGCR;
         }
      /* Absolute environmental value -- use new value */
      else if (ivb->vbopt & VBVOPAV) {
         int jec = (int)ivb->iovec;
         val->vdampprv =
            mrsrsle(val->vcalcnow, ivb->vscl, FBsc, jec);
         if (!(ivb->vbopt & VBVOPFS)) val->vdampprv =
            mrsrsl(val->vdampprv, (si32)ivb->vdamp, FBdf);
         /* Value returned may be delayed */
         if (ivb->vdelay == 0) {
            val->fullvalue = val->cnstrval = val->vdampprv;
            if (RP->compat & OLDRANGE) {
               if (val->cnstrval < 0) val->cnstrval = 0;
               else if (val->cnstrval > (1<<FBvl))
                  val->cnstrval = (1<<FBvl);
               }
            }
         else
            val->fullvalue = val->cnstrval = 0;
#ifdef OLDDAMP
         val->vdampprv = 0;
#endif
         }
      /* Relative environmental value -- keep Vc(t-1), rest is 0 */
      else {
         val->vdampprv = val->fullvalue = val->cnstrval = 0;
         } /* End not repertoire value */
      } /* End value block loop */

   } /* End d3zval() */


/*---------------------------------------------------------------------*
*                               d3vprt                                 *
*                     Print current value vector                       *
*---------------------------------------------------------------------*/

#ifndef PARn
void d3vprt(void) {

   if (RP->nvblk) {
      int structsize = sizeof(struct VDTDEF);
      if (RP->kdprt & PRSPT) spout((RP->nvblk + 7)/8);
      convrt("('0Value vector = '|^#(8B" qqv(FBvl) "IJ8.3;/))",
         &structsize,&RP->nvblk,&RP->pvdat[0].fullvalue,NULL);
      }

   } /* End d3vprt() */
#endif


/*---------------------------------------------------------------------*
*                               d3valu                                 *
*             Evaluate current state of all value schemes              *
*---------------------------------------------------------------------*/

void d3valu(struct VBDEF *pvbt) {

   struct VBDEF  *ivb;
   struct VDTDEF *val;
#ifndef PARn
   double dFBvl;
   float deltax;              /* Distances for HHDIST, etc. schemes */
   float deltay;
   float rdistsq;
#endif
   si32 curvalue;             /* Current value (FBvl=8) */
   si32 lprvvalue;            /* Working copy of old 'vcalcnow' */
   si32 OneFBvl;              /* 1.0 (FBvl=8) */
#ifdef OLDDAMP
   int zvdp;
#endif

/* Protect against null value array */

   if (!RP->pvdat) d3exit(NULL, VALDAT_ERR, 0);

/* Loop over all value blocks in the thread specified in the call.
*  Repertoire types are calculated on comp nodes in every cycle,
*  environment types are calculated on host only once per trial.
*  (In V8A, value data were moved to general input area, formerly
*  had their own separate broadcast in d3go.  In V8B, VBDEFs were
*  double threaded to eliminate traversing the irrelevant ones.  */

   OneFBvl = 1 << FBvl;
#ifndef PARn
   dFBvl = (double)OneFBvl;
#endif
   for (ivb=pvbt; ivb; ivb=ivb->pnxvt) {

      int jec = (int)ivb->iovec;
      val = RP->pvdat + ivb->ivdt;
      switch (ivb->kval) {

/*---------------------------------------------------------------------*
*        TENT method--                                                 *
*           Calculate value based on number of cells firing            *
*           Value has falling branch if nbest exceeded                 *
*---------------------------------------------------------------------*/

case VB_TENT: {
         struct CELLTYPE *il = ivb->u1.vrp.pvsrc;
         register si32 hcnt = 0;       /* Hit counter, then (FBvl=8) */
         si32 hts7 = SRA(il->Ct.ht,(FBwk-FBsi));
                                       /* Hit test value (S7) */
         si32 wksi;                    /* Working value of s(i) */
         s_type *pcell = il->pps[0];   /* Cell pointer and loop limit */
         s_type *pecell= pcell + il->lspt;

         /* Count cells that are firing */
         for (; pcell<pecell; pcell+=il->lsp) {
            d3gtjs2(wksi, pcell);
            if (wksi >= hts7) hcnt++;
            }

         /* If the halfwidth is zero, make a delta function,
         *  otherwise make a tent as per the writeup.  */
         if (ivb->vhalf == 0)
            curvalue = (hcnt == ivb->vbest) ? OneFBvl : ivb->vbase;
         else {
            /* N.B. Can lose all precision if absorb vhalf in omvbd2 */
            hcnt = OneFBvl - labs(hcnt - ivb->vbest)*
               ivb->u1.vrp.omvbd2/ivb->vhalf;
            curvalue = max(hcnt,ivb->vbase);
            }
         break;
         }

/*---------------------------------------------------------------------*
*        NFIRE method--                                                *
*           Calculate value based on number of cells firing            *
*---------------------------------------------------------------------*/

case VB_FIRE: {
         struct CELLTYPE *il = ivb->u1.vrp.pvsrc;
         register si32 hcnt = 0;       /* Hit counter, then (FBvl=8) */
         si32 hts7 = SRA(il->Ct.ht,(FBwk-FBsi));
                                       /* Hit test value (S7) */
         si32 wksi;                    /* Working value of s(i) */
         s_type *pcell = il->pps[0];   /* Cell pointer and loop limit */
         s_type *pecell= pcell + il->lspt;

         /* Count cells that are firing */
         for (; pcell<pecell; pcell+=il->lsp) {
            d3gtjs2(wksi, pcell);
            if (wksi >= hts7) hcnt++;
            }

         /* If nfire exceeds vbest, value is maximal */
         if (hcnt >= ivb->vbest) curvalue = OneFBvl;
         /* If the halfwidth is zero, make a step function */
         else if (ivb->vhalf == 0) curvalue = ivb->vbase;
         /* Otherwise, make a ramp as per the writeup */
         else {
            /* N.B. Can lose all precision if absorb vhalf in omvbd2 */
            hcnt = OneFBvl - (ivb->vbest - hcnt)*
               ivb->u1.vrp.omvbd2/ivb->vhalf;
            curvalue = max(hcnt,ivb->vbase);
            }
         break;
         }

/*---------------------------------------------------------------------*
*        AVGSI method--                                                *
*           Calculate value based on average s(i) in a layer           *
*                                                                      *
*  Note:  This is the only value scheme that depends on s(i) values    *
*  and with COMPAT=C they ranged from 0-1 (S14).  Here we treat 128mV  *
*  on new scale (equiv. to old Si == 1.0) as value 1.0.                *
*                                                                      *
*  R76, 10/20/17, GNR - Scaling corrected, AVGSI is not in v8tin tests *
*---------------------------------------------------------------------*/

case VB_AVGSI: {
         struct CELLTYPE *il = ivb->u1.vrp.pvsrc;
         si64 sum = jesl(0);           /* Total s(i) (S7) */
         si32 wksi;                    /* Working value of s(i) */
         s_type *pcell = il->pps[0];   /* Cell pointer and loop limit */
         s_type *pecell= pcell + il->lspt;

         /* Calculate average of s(i) and keep just FBvl bits */
         for (; pcell<pecell; pcell+=il->lsp) {
            d3gtjs2(wksi, pcell);      /* S7/14 */
            sum = jasl(sum, wksi); }
         curvalue = dsrswq(sum, FBvl - (FBsi+Sr2mV), il->nelt);
         break;
         }

/*---------------------------------------------------------------------*
*        HHDIST method--                                               *
*           Calculate value based on distance from arm to window       *
*---------------------------------------------------------------------*/

case VB_DIST:
#ifdef PARn
         continue;
#else
         /* Pick up X,Y components in deltax, deltay */
         xhhxy(deltax,deltay,ivb);
         /* Compute Pythagoras */
         rdistsq = deltax*deltax + deltay*deltay;
         curvalue = (si32)(dFBvl*exp((double)
            (rdistsq*ivb->u1.vhh.vradm2)));
         break;
#endif

/*---------------------------------------------------------------------*
*        HHRING method--                                               *
*           Value based on ring distance from arm to window            *
*---------------------------------------------------------------------*/

case VB_RING:
#ifdef PARn
         continue;
#else
         /* Pick up X,Y components in deltax, deltay */
         xhhxy(deltax,deltay,ivb);
         /* Compute ring distance = max(|dx|,|dy|) */
         if (deltax < 0.0) deltax = -deltax;
         if (deltay < 0.0) deltay = -deltay;
         rdistsq = (deltax > deltay) ? deltax : deltay;
         rdistsq *= rdistsq;
         curvalue = (si32)(dFBvl*exp((double)
            (rdistsq*ivb->u1.vhh.vradm2)));
         break;
#endif

/*---------------------------------------------------------------------*
*        HHDLIN method--                                               *
*           Calculate value based on distance from arm to window       *
*---------------------------------------------------------------------*/

case VB_DLIN:
#ifdef PARn
         continue;
#else
         /* Pick up X,Y components in deltax, deltay */
         xhhxy(deltax,deltay,ivb);
         rdistsq = sqrt((double)(deltax*deltax + deltay*deltay));
         curvalue = (si32)(dFBvl*(rdistsq*ivb->u1.vhh.vradm2 + 1.0));
         curvalue = max(curvalue,ivb->vbase);
         break;
#endif

/*---------------------------------------------------------------------*
*        HHRLIN method--                                               *
*           Value based on ring distance from arm to window            *
*---------------------------------------------------------------------*/

case VB_RLIN:
#ifdef PARn
         continue;
#else
         /* Pick up X,Y components in deltax, deltay */
         xhhxy(deltax,deltay,ivb);
         if (deltax < 0.0) deltax = -deltax;
         if (deltay < 0.0) deltay = -deltay;
         rdistsq = (deltax > deltay) ? deltax : deltay;
         curvalue = (si32)(dFBvl*(rdistsq*ivb->u1.vhh.vradm2 + 1.0));
         curvalue = max(curvalue,ivb->vbase);
         break;
#endif

/*---------------------------------------------------------------------*
*        UJMODE method--                                               *
*           Value is 1 if arm in ujmode, otherwise is vbase            *
*---------------------------------------------------------------------*/

case VB_UJMD:
#ifdef PARn
         continue;
#else
         curvalue = (ivb->u1.vhh.mvarm->jatt & ARMTR) ?
            OneFBvl : ivb->vbase;
         break;
#endif

/*---------------------------------------------------------------------*
*        GRIP method--                                                 *
*           Value is 1 if arm is gripping, otherwise is vbase          *
*---------------------------------------------------------------------*/

case VB_GRIP:
#ifdef PARn
         continue;
#else
         curvalue = (ivb->u1.vhh.mvarm->jatt & ARMGP) ?
            OneFBvl : ivb->vbase;
         break;
#endif

/*---------------------------------------------------------------------*
*        ENVVAL method--                                               *
*           Use value stored in referenced MODALITY block              *
*---------------------------------------------------------------------*/

case VB_ENVL:
#ifdef PARn
         continue;
#else
         {  short mval = ivb->u1.vev.pvmdl->mdltvalu;
            if ((unsigned short)mval == (unsigned short)DFLT_ENVVAL)
               mval = DFLT_VALUE;
            curvalue = (si32)mval;
            } /* End local scope */
         break;
#endif

/*---------------------------------------------------------------------*
*        CONSTANT method--                                             *
*           Use constant value 'vbase'                                 *
*---------------------------------------------------------------------*/

case VB_CONS:
#ifdef PARn
         continue;
#else
       curvalue = ivb->vbase;
       break;
#endif

#ifdef BBDS
/*---------------------------------------------------------------------*
*        BBD method--                                                  *
*           (float)value stored in ivb->u1.vbbd.fbbdval by bbdsgetv(). *
*           Check to be sure conversion to curvalue will not overflow. *
*---------------------------------------------------------------------*/

case VB_BBD:
#ifdef PARn
         continue;
#else
         if (fabsf(ivb->u1.vbbd.fbbdval) >= dS23)
            d3exit(NULL, VALMAG_ERR, ivb->ivdt+1);
         curvalue = (si32)((float)OneFBvl*ivb->u1.vbbd.fbbdval);
         break;
#endif
#endif

         } /* End of switch over value schemes */

/*---------------------------------------------------------------------*
*     Have new value (not scaled yet) in curvalue (FBvl=8)--           *
*        Fetch previous value and save new                             *
*        Apply vmin and form difference, unless VBVOPAV (abs val)      *
*        Scale and form running average                                *
*        Store full (FBvl=8) value in 'fullvalue'--                    *
*           used for bar graphs, V amp, universal joint, print         *
*        Store a copy of 'fullvalue' in 'cnstrval' if new mV scale,    *
*           otherwise value reduced to range 0 <= V < 1 if COMPAT=C:   *
*           used for direct connection, modulation, nmod input         *
*---------------------------------------------------------------------*/

#ifdef OLDDAMP
      zvdp = ivb->vbflg & VBFLGC1;
#endif
      lprvvalue = val->vcalcnow;
      val->vcalcnow = curvalue; /* Save vc(t) for vc(t-1) next time */
      /* Special handling for first cycle of run (VBFLGC1, ENV/BBD
      *  value), first cycle after reset (VBFLGCR, REPVAL value),
      *  or constant value.  */
      if (ivb->vbflg & (VBFLGC1|VBFLGCR|VBFLCON)) {
         ivb->vbflg &= ~(VBFLGC1|VBFLGCR);
         if (ivb->vbopt & VBVOPFS) {   /* Fast start */
            val->vdampprv = (ivb->vbopt & VBVOPAV) ?
               mrsrsle(curvalue, ivb->vscl, FBsc, jec) : 0;
            }
         else {                        /* Not fast start */
            curvalue = mrsrsle(curvalue, ivb->vscl, FBsc, jec);
            val->vdampprv = mrsrsl(curvalue, (si32)ivb->vdamp, FBdf);
            }
         }
      else {                           /* Not first cycle */
         si32 cvscl;                   /* curvalue scaled */
         si32 tsum;                    /* Temp sum for jasjem */
         if (!(ivb->vbopt & VBVOPAV)) {
            if (lprvvalue < ivb->vmin) lprvvalue = ivb->vmin;
            curvalue -= lprvvalue;  /* Use change as new value */
            }
         cvscl = mrsrsle(curvalue, ivb->vscl, FBsc, jec);
         jrsjem(curvalue, cvscl, val->vdampprv, jec);
         cvscl = mrsrsl(curvalue, (si32)ivb->vdamp, FBdf);
         jasjem(tsum, val->vdampprv, cvscl, jec);
         val->vdampprv = tsum;
         }

      /* Value returned may be delayed */
      if (RP->CP.trialsr >= (ui32)ivb->vdelay) {
         val->fullvalue = val->cnstrval = val->vdampprv;
         if (RP->compat & OLDRANGE) {
            if (val->cnstrval < 0) val->cnstrval = 0;
            else if (val->cnstrval > OneFBvl)
               val->cnstrval = OneFBvl;
            }
         }
      else
         val->fullvalue = val->cnstrval = 0;
#ifdef OLDDAMP
      if (zvdp) val->vdampprv = 0;
#endif

      } /* End of loop over value blocks */

   } /* End d3valu() */
