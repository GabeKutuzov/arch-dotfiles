/* (c) Copyright 1994-2018, The Rockefeller University *11115* */
/* $Id: d3decay.c 77 2018-03-15 21:08:14Z  $ */
/***********************************************************************
*                             CNS Program                              *
*            d3decay, fsdecay, rsdecay, updecay, offdecay              *
*                                                                      *
************************************************************************
*  V7C, 08/28/94, GNR - Initial version (hesam to float, 02/02/95)     *
*  V8A, 04/08/96, GNR - Revise calling args for use with cell decay    *
*  Rev, 01/12/98, GNR - New argument arrangement, pers not in DECAYDEF *
*  V8C, 05/04/03, GNR - Scale Aeff, Acal etc. to S20 from S24          *
*  V8D, 07/04/05, GNR - Scale omega to S30 from S16                    *
*  ==>, 08/07/07, GNR - Last mod before committing to svn repository   *
*  Rev, 12/31/08, GNR - Replace jm64sh with msrsle                     *
*  Rev, 10/05/12, GNR - Add ALPHA, DOUBLE-EXP, fsdecay(), offdecay()   *
*  Rev, 08/28/13, GNR - Add rsdecay()                                  *
*  Rev, 10/13/13, GNR - Add updecay(), better ALPH,DBLE oflw action    *
*  Rev, 08/22/14, GNR - Si only EXP decay, remove DECAYDEF from DCYDFLT*
*  Rev, 09/04/14, GNR - Save h terms in AffD32 for ALPHA and DBLE      *
*  Rev, 10/25/15, GNR - Rename Aeff to Aegh, be clear that persistence *
*                       is w,g,h terms passed from previous time step. *
*  R77, 02/21/18, GNR - Use mrsrsl where decay overflow not possible   *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "sysdef.h"
#include "d3global.h"
#include "rkarith.h"

#ifndef PARn
/***********************************************************************
*                              offdecay                                *
*                                                                      *
*  This routine turns off decay in the specified DECAYDEF in a         *
*  standard, safe, manner.  Done at new series time before broadcast   *
*  so not available on PARn nodes.                                     *
***********************************************************************/

void offdecay(struct DECAYDEF *pdcd) {
   pdcd->kdcy = 0;
   pdcd->ndal = 0;
   }
#endif


/***********************************************************************
*                               d3decay                                *
*                                                                      *
*     This routine handles a standardized decay calculation that may   *
*  be used with any connection type.  For that reason, arguments are   *
*  gathered individually by the caller.  The calculation performed     *
*  depends on the kdcy flag in the DECAYDEF.  Notation:                *
*     Aegh(t)     Value(s) that retain state between cycles.  For EXP, *
*                 LIM, or SAT decay, it is the effective value at time *
*                 t of the afferent term after decay.  For ALPHA or    *
*                 DBLE-EXP decay, it is the pair of values g(t),h(t)   *
*                 defined in the math derivations.  A ptr to Aegh(t-1) *
*                 is passed to d3decay and used to update the stored   *
*                 values.                                              *
*     Acal(t)     Calculated (new) value of afferent term in question. *
*                    (Both Aegh and Acal are allowed to be signed.)    *
*     Pers(t)     Persistence value stored for detail print or stats.  *
*                 For EXP, LIM, or SAT decay, it is omega*Aegh(t-1).   *
*                 For ALPHA or DBLE-EXP decay, it is g(t-1),h(t-1).    *
*     h(t)        Auxiliary value used with ALPHA and DBLE-EXP decay.  *
*     omega       Persistence constant for this kind of term (S30).    *
*     omeg2       Second persistence constant for DBLE-EXP decay (S30).*
*     halfeff     Value of Aegh at which saturation is 0.5.            *
*     hesam       1.3170/(2**20 or 2**27)/halfeff (float).             *
*                                                                      *
*  I.    EXPONENTIAL method (kdcy == DCYEXP)                           *
*        Aegh(t) = Acal(t) + omega*Aegh(t-1)                           *
*  II.   LIMITING method (kdcy == DCYLIM)                              *
*        Aegh(t) = sign(Acal)*max(|Acal(t)|, omega*|Aegh(t-1)|)        *
*  III.  SATURATING method (kdcy == DCYSAT)                            *
*        Aegh(t) = sech(hesam*|Aegh(t-1)|)*Acal(t) + omega*Aegh(t-1)   *
*        (sech(x) is computed by using cosh() library function.  An    *
*        experimental table lookup implementation was too inaccurate.  *
*        sech() is an even function, so we can skip the abs(arg).)     *
*  IV.   ALPHA method (kdcy = DCYALPH)                                 *
*        h(t) = omega*h(t-1) - ln(omega)*Acal(t),                      *
*        g(t) = omega*(g(t-1) + h(t-1))                                *
*  V.    DOUBLE-EXPONENTIAL method (kdcy = DCYDBLE)                    *
*        h(t) = omega1*h(t-1) + Acal(t),                               *
*        g(t) = omega1*h(t-1) + omega2*(g(t-1)-h(t-1))                 *
*                                                                      *
*  Synopsis: si32 d3decay(struct DECAYDEF *pdcd, si32 *pAegh,          *
*        si32 *pPers, si32 Acal)                                       *
*  Arguments:                                                          *
*     pdcd           Ptr to DECAYDEF structure containing parameters   *
*                    governing decay process.                          *
*     pAegh          Ptr to location where Aegh (g,h if ALPHA or DBLE) *
*                    is/are stored (S20). These are updated on return. *
*                    Must be initialized at reset or first cycle.      *
*     pPers          Ptr to location where new persistence value (g    *
*                    or g and h) is left on return for use by detail   *
*                    print or stats (S20).                             *
*     Acal           Calculated new activity value (S20).              *
*  Results returned:                                                   *
*     *pAegh = Aegh(t) or g(t),h(t) = new effective activity (S20).    *
*     *pPers = omega*Aegh(t-1) or g(t-1),h(t-1) = persistence (S20).   *
*      Acal  = Aegh(t) or g(t) = new activity value (S20).             *
*                                                                      *
*  Notes:                                                              *
*    -d3decay reports overflows according to current e64set action.    *
*    -Different callers may impose different max values for Aegh.      *
*    -The only thing that has to be changed if the scale of Aegh,      *
*     Acal is changed is the definition of the constant DCYSDFLT.      *
*    -After revs of 10/25/15, dprnt will display persistence 'W' for   *
*     old decay modes and 'g(t-1)' and 'h(t-1)' for new decay modes.   *
*    -Overflows in Acal in all modes automatically update pAegh[Odg]   *
*     correctly, but in DCYALPH and DCYDBLE pAegh[Odh] must then be    *
*     updated.  Although it adds extra call overhead, updecay() is     *
*     used so the update is always done the same way (+/- NOUPTIME).   *
*    -Thought was given to making all the arguments si64 so swloem     *
*     checking would not be needed in the setups, but this would use   *
*     more space for the Aeghs and the mrsrsl() calls would become     *
*     much slower mrsrswe().                                           *
***********************************************************************/

si32 d3decay(struct DECAYDEF *pdcd, si32 *pAegh, si32 *pPers,
      si32 Acal) {

   e64dec((int)pdcd->iovec);

/* Proceed according to requested mode of calculation */

   switch (pdcd->kdcy) {

   case NODECAY:              /* No decay -- JIC */
      /* In this case, pAegh does not point to valid storage,
      *  so just store zero persistence and return.  */
      *pPers = 0;
      return Acal;

   case DCYEXP:
      *pPers = mrsrsl(*pAegh, pdcd->omega, FBod);
      jasjdm(*pAegh, Acal, *pPers);
      break;

   case DCYLIM:
      *pPers = mrsrsl(*pAegh, pdcd->omega, FBod);
      if (Acal >= 0)
         *pAegh = max(Acal, *pPers);
      else
         *pAegh = min(Acal, *pPers);
      break;

   case DCYSAT:
      *pPers = mrsrsl(*pAegh, pdcd->omega, FBod);
      {  float x = (float)(*pAegh)*pdcd->du1.dsa.hesam;
         Acal = (si32)((float)Acal/coshf(x)); }
      jasjdm(*pAegh, Acal, *pPers);
      break;

   case DCYALPH:
      pPers[Odg] = pAegh[Odg], pPers[Odh] = pAegh[Odh];
      /* Update h(t) but use old h(t) to upate g(t).  If h(t) over-
      *  flows, it is enough to count it and set the result to max,
      *  g(t) is not affected.  Use 64-bit ht here so only one
      *  overflow check is needed.  */
      {  si32 gt = mrsrsl(pAegh[Odg], pdcd->omega, FBod);
         si64 ht = mrssw(jesl(Acal), pdcd->du1.dal.mlnw, FBlw);
         si32 ho = mrsrsl(pAegh[Odh], pdcd->omega, FBod);
         ht = jasl(ht, ho);
         swlodm(pAegh[Odh], ht);
         pAegh[Odg] = gt + ho;
         if (jckss(pAegh[Odg],gt,ho)) {
            e64dac("d3decay");
            updecay(pdcd, pAegh, (ho >= 0) ? SI32_MAX : -SI32_MAX);
            }
         } /* End DCYALPH local scope */
      break;

   case DCYDBLE:
      pPers[Odg] = pAegh[Odg], pPers[Odh] = pAegh[Odh];
      /* Rev, 10/25/15, GNR:  This code now assumes that g(t) and
      *  h(t) normally have the same sign (or are small), therefore
      *  the likelihood of overflow of g(t0)-h(t0) is very small, so
      *  there was not much point in multiplying each separately by
      *  omega2 before subtracting them, as was done in the previous
      *  version, just to reduce the chances of overflow.  But over-
      *  flow in the calc of h(t) must be dealt with.  */
      {  si32 gt,ho;
         jrsjdm(gt, pAegh[Odg], pAegh[Odh]);
         gt = mrsrsl(gt, pdcd->du1.ddb.omeg2, FBod);
         ho = mrsrsl(pAegh[Odh], pdcd->omega, FBod);
         jasjdm(pAegh[Odh], ho, Acal);
         pAegh[Odg] = gt + ho;
         if (jckss(pAegh[Odg],gt,ho)) {
            e64dac("d3decay");
            updecay(pdcd, pAegh, (ho >= 0) ? SI32_MAX : -SI32_MAX);
            }
         } /* End DCYDBLE local scope */
      break;

      }  /* End kdcy switch */

   return *pAegh;

   } /* End d3decay() */


/***********************************************************************
*                               fsdecay                                *
*                                                                      *
*     This routine initializes persistence terms for "fast-start"      *
*  decay.  The idea is to set the stored persistence terms to what     *
*  they would be if the new activity level had remained constant over  *
*  enough time in the past for everything to equilibrate.  fsdecay()   *
*  also then returns the updated Acal so it can be plugged in as a     *
*  direct replacement for d3decay() at fast-start time.                *
*     The ALPHA and DBLE-EXP decay modes are basically designed to     *
*  work with spiking neurons and specific (CONNTYPE) connection types, *
*  where the equilibrium value would depend on the spiking rate of the *
*  input and fast-start will not be an option.  These modes should     *
*  never occur with GCONNs or MODUL conns, but if they do and fast-    *
*  start is specified, I have inserted the math here to create the     *
*  fake history.  Another option would be to treat as a normal cycle   *
*  with zero history.                                                  *
*     With the remaining decay modes, fsdecay() replaces in-situ code  *
*  that performed the fast-start function because the initialization   *
*  is now different for different decay modes.  The user should be     *
*  aware that fast-start may significantly increase Aegh relative to   *
*  Acal and should scale Acal accordingly.  Math details are in the    *
*  design notes, p. 70 and 72.                                         *
*     Previous versions of CNS did not compute the equilibrium Aegh.   *
*  A note from d3go(), 04/19/12, indicates that for EXPONENTIAL decay  *
*  the current Acal was stored as persistence for next time but omega  *
*  times*Acal was not added to itself--to give a more gentle startup.  *
*                                                                      *
*  Synopsis: si32 fsdecay(struct DECAYDEF *pdcd, si32 *pAegh,          *
*        si32 *pPers, si32 Acal)                                       *
*                                                                      *
*  Arguments and return values:  Same as for d3decay().                *
*                                                                      *
*  Note:  I chose to calculate (1-omega) when needed rather than       *
*  store it in the DECAYDEF block, because these calculations are      *
*  expected to be infrequent.                                          *
***********************************************************************/

si32 fsdecay(struct DECAYDEF *pdcd, si32 *pAegh, si32 *pPers,
      si32 Acal) {

   e64dec((int)pdcd->iovec);

/* Proceed according to requested mode of calculation */

   switch (pdcd->kdcy) {

   case NODECAY:              /* No decay -- JIC */
      /* In this case, pAegh does not point to valid storage,
      *  so just store zero persistence and return.  */
      *pPers = 0;
      return Acal;

   case DCYEXP:
      /* Calc pers first, as Aegh may overflow even if pers does not */
      *pPers = dmsjqd(Acal, pdcd->omega, Sod - pdcd->omega);
      jasjdm(*pAegh, Acal, *pPers);
      break;

   case DCYLIM:
      *pPers = mrsrsl(Acal, pdcd->omega, FBod);
      *pAegh = Acal;
      break;

   case DCYSAT:
      /* omega == Sod is an error at input time, so no divide by 0.
      *  Unadjusted tol will be a reasonable value if COMPAT=C.  */
      {  float A0 = (float)Acal*dSFBod/(float)(Sod - pdcd->omega);
         float Aip1, Ai = A0;
         float tol = DCYSTOL * dS20;
         int ji;
         for (ji=0; ji<DCYSMXIT; ++ji) {
            Aip1 = 0.5*(Ai + A0/coshf(pdcd->du1.dsa.hesam * Ai));
            if (fabsf(Aip1 - Ai) < tol) break;
            Ai = Aip1; }
         *pAegh = (si32)Aip1;
         *pPers = mrsrsl(*pAegh, pdcd->omega, FBod);
         }
      break;

   case DCYALPH:
      {  si64 tt;
         si32 omw = Sod - pdcd->omega;
         tt = jmsw(Acal, pdcd->du1.dal.mlnw);
         pAegh[Odh] = pPers[Odh] =
            dsrswqd(tt, FBod-FBlw, omw);
         tt = jmsw(pAegh[Odh], pdcd->omega);
         pAegh[Odg] = pPers[Odg] =
            dsrswqd(tt, 0, omw);
         }
      break;

   case DCYDBLE:
      pAegh[Odh] = pPers[Odh] =
         dsrsjqd(Acal, FBod, Sod - pdcd->omega);
      pAegh[Odg] = pPers[Odg] = dmsjqd(pAegh[Odh],
         pdcd->omega - pdcd->du1.ddb.omeg2,
         Sod - pdcd->du1.ddb.omeg2);
      break;

      }  /* End kdcy switch */

   return *pAegh;

   } /* End fsdecay() */


/***********************************************************************
*                               rsdecay                                *
*                                                                      *
*     This routine performs reset decay for any variable subject to    *
*  one of the standard decay modes described above.  A reset decay     *
*  involves the passage of nit time steps of length RP->timestep but   *
*  with no new input.  The math is described in the Design Notes,      *
*  pp. 66-70.                                                          *
*                                                                      *
*  Synopsis: void rsdecay(struct DECAYDEF *pdcd, ubig ndcy, int nit)   *
*                                                                      *
*  Arguments:                                                          *
*     pdcd           Ptr to DECAYDEF structure containing parameters   *
*                    governing decay process.                          *
*     ndcy           Number of entries in peipv array in DECAYDEF.     *
*     nit            Number of time steps of size RP->timestep.        *
*                                                                      *
*  Results returned:                                                   *
*     Decaying values in peipv array are updated.                      *
*                                                                      *
*  Notes:                                                              *
*    -rsdecay reports overflows according to current e64set action.    *
*    -To handle multiples of timestep, double-precision arithmetic is  *
*     used (SodPow macro) rather than ui32pow() for better speed and   *
*     accuracy.  pdcd->omega is exp(-b*timestep) therefore decay       *
*     for nit steps is exp(nit * log(omega)).                          *
*    -omega is restricted on input to be < 1.0, therefore there is no  *
*     need to check for overflow when decaying peipv values.           *
*    -Thought was given to precalculating the omega**nit terms in      *
*     d3nset and storing in DECAYDEF, but the code here does not run   *
*     often and this way it is safe to call with different nits.       *
***********************************************************************/

void rsdecay(struct DECAYDEF *pdcd, ubig ndcy, int nit) {

   si32 *pdv,*pdve;     /* Ptrs to decay value */
   si32 efac;           /* Exponential decay factor (Sod) */

   e64dec((int)pdcd->iovec);
   pdv = pdcd->peipv;
   pdve = pdv + ndcy*(ubig)pdcd->ndal;
   efac = SodPow(pdcd->omega, nit, FBod);

/* Proceed according to requested mode of calculation */

   switch (pdcd->kdcy) {

   case NODECAY:              /* No decay -- JIC */
      break;

   case DCYEXP:
   case DCYLIM:
   case DCYSAT:
      for ( ; pdv<pdve; ++pdv)
         *pdv = mrsrsl(*pdv, efac, FBod);
      break;


   case DCYALPH:
      for ( ; pdv<pdve; pdv+=2) {
         si32 gnw = mrsrsl(pdv[Odg], efac, FBod);
         pdv[Odh] = mrsrsl(pdv[Odh], efac, FBod);
         jasjdm(pdv[Odg], gnw, pdv[Odh]);
         }
      break;

   case DCYDBLE:
      {  si32 gh, gnw, efb2;
         efb2 = SodPow(pdcd->du1.ddb.omeg2, nit, FBod);
         for ( ; pdv<pdve; pdv+=2) {
            jrsjdm(gh, pdv[Odg], pdv[Odh]);
            pdv[Odh] = mrsrsl(pdv[Odh], efac, FBod);
            gnw = mrsrsl(gh, efb2, FBod);
            jasjdm(pdv[Odg], gnw, pdv[Odh]);
            }
         } /* End DCYBDLE local scope */
      break;

      }  /* End kdcy switch */

   return;
   } /* End rsdecay() */

/***********************************************************************
*                               updecay                                *
*                                                                      *
*     This routine is used to update stored decay-in-progress (peipv)  *
*  information when the state of a cell or connection is forced to     *
*  have a particular value, e.g. by cell spiking or response capping.  *
*  For the DCYALPH and DCYDBLE cases, the code computes the value of   *
*  input that would be needed to generate the specified output, then   *
*  updates the persistence as if that were the input received.  For    *
*  the other modes, the new response is stored as the persistence.     *
*                                                                      *
*  Synopsis:  void updecay(struct DECAYDEF *pdcd, si32 *pAegh,         *
*     si32 Acal)                                                       *
*                                                                      *
*  Arguments:  Same as for d3decay (except no pPers)                   *
*                                                                      *
*  Note:  Normally, d3decay|fsdecay will have been called before this  *
*  routine, for example, to determine whether a spike has occurred.    *
*  If updating Aegh at the same time point, the 'h' history variable   *
*  computes to 0.  Here, we pretend one more time step has occurred    *
*  so some history can be preserved in 'h'.  Set NOUPTIME to turn off  *
*  this behavior.                                                      *
*                                                                      *
*  Note added, 08/23/14:  Originally, this routine dealt with updates  *
*  to Si after decay was added.  Now we decided that Si will have only *
*  EXP decay, so the peipv variable is not used, the updated Si is the *
*  persistence, and this routine is no longer used there.  It is still *
*  called from d3decay in certain overflow situations.                 *
***********************************************************************/

void updecay(struct DECAYDEF *pdcd, si32 *pAegh, si32 Acal) {

   if (!pAegh) return;           /* JIC */

/* Proceed according to requested mode of calculation */

   switch (pdcd->kdcy) {

   case NODECAY:                 /* No decay -- JIC */
      break;

   case DCYEXP:
   case DCYSAT:
      *pAegh = Acal;
      break;

   case DCYLIM:
      if (Acal >= 0)
         *pAegh = max(Acal, *pAegh);
      else
         *pAegh = min(Acal, *pAegh);
      break;

   case DCYALPH:
#ifdef NOUPTIME
      pAegh[Odh] = 0;
#else
      {  si32 tt;
         e64dec((int)pdcd->iovec);
         tt = mrsrsl(pAegh[Odg], pdcd->omega, FBod);
         jrsjdm(pAegh[Odh], Acal, tt);
         }
#endif
      pAegh[Odg] = Acal;
      break;

   case DCYDBLE:
#ifdef NOUPTIME
      pAegh[Odh] = 0;
#else
      {  si32 t1,t2;
         e64dec((int)pdcd->iovec);
         t1 = mrsrsl(pAegh[Odg], pdcd->du1.ddb.omeg2, FBod);
         jrsjdm(t2, Acal, t1);
         pAegh[Odh] = mrsrsld(t2, pdcd->du1.ddb.w1ovdw, FBsc);
         } /* End local scope */
#endif
      pAegh[Odg] = Acal;
      break;
      } /* End kdcy switch */

   return;
   } /* End updecay() */
