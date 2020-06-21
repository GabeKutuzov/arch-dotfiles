/* (c) Copyright 2009, Neurosciences Research Foundation, Inc. */
/* $Id: d3bgis.c 26 2009-12-31 21:09:33Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                               d3bgis                                 *
*                                                                      *
*  "Brette-Gerstner and Izhikevich setup".  This routine performs any  *
*  set up calculations, particularly, but not exclusively, those for   *
*  Brette-Gerster (aEIF) or Izhikevich neurons, that are required on   *
*  Node 0 both before d3genr runs and again each time a new CYCLE card *
*  is read.                                                            *
************************************************************************
*  V8E, 10/25/09, GNR - New routine                                    *
*  ==>, 11/23/09, GNR - Last mod before committing to svn repository   *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "sysdef.h"
#include "d3global.h"
#include "rocks.h"
#include "rkarith.h"

void d3bgis(void) {

   struct CELLTYPE *il;

   for (il=RP->pfct; il; il=il->pnct) {

      /* Compute 1/Cm (S34 = 57 - 23, requires Cm < 8) */
      il->RCm = dsruwq(jeul(1), 57, il->Ct.Cm);

      /* If neuron has AEIF (Brette-Gerstner) or Izhikevich response
      *  function, any other decay mode entered is an error.
      *  Compute derived parameters that will be used in d3genr()
      *  (resetu3(), resetu7, or resetw()) and d3go().  If it is
      *  Izhikevich, set rspmethu to signal d3go() when slower
      *  extended code is needed.  */

      il->rspmethu = il->Ct.rspmeth;
      if (il->Ct.rspmeth >= RF_BREG) {
         if (il->Dc.CDCY.kdcy != DCYEXP)
            cryout(RK_E1, "0***DECAY MODE INCOMPATIBLE WITH RESPON"
            "SE FUNCTION FOR ", RK_LN2, fmturlnm(il), RK_CCL, NULL);
         if (il->Ct.rspmeth == RF_BREG) {
            struct BREGDEF *pbg = (struct BREGDEF *)il->prfi;
            ui32 tgLoCm = msrule(pbg->gL, il->RCm, -26, OVF_BREG);
            pbg->ugLoCm = S28 - tgLoCm;
            pbg->ugLdToCm =
               msrsle((si32)tgLoCm, pbg->delT, -28, OVF_BREG);
            pbg->u1oln2dT = /* (2^48/ln2)/delT (S20) ==> S28 */
#ifdef HAS_I64
#if LSIZE == 8
#define TWO48oLn2 406082553035091L
#else
#define TWO48oLn2 406082553035091LL
#endif
               dsrswq(TWO48oLn2, 0, pbg->delT);
#else
               dsrswq(jcsw(94548, 1985132884), 0, pbg->delT);
#endif
            pbg->u1otW  = dsruwq(jeul(1), 48, pbg->tauW);
            pbg->ubga   = msrsle(pbg->bga, il->RCm, -30, OVF_BREG);
            pbg->ubgb   = msrsle(pbg->bgb, il->RCm, -34, OVF_BREG);
            /* pbg->uspthr is value of V-vT in aEIF exponential term
            *  that will cause an overflow at 2^30 level.  20.794415
            *  is 30*ln2 = log2(2^30).  (Derivation takes into
            *  account S20 scale of ugLdToCm term.  Works with logf
            *  in Linux, but not all systems have logf().) */
            pbg->uspthr = (si32)((double)pbg->delT*
               (20.794415 - log((double)pbg->ugLdToCm)));
            }
         else if (il->Ct.rspmeth == RF_IZH3) {
            /* This code stores Vrest (here called i3vr) for IZH3
            *  cells when b is constant.  (See comments in d3i3vr.c) */
            struct IZ03DEF *pi3 = (struct IZ03DEF *)il->prfi;
            il->Ct.vrest = pi3->i3vr = d3i3vr(pi3, pi3->Z.izb);
            pi3->izcv1p1 = pi3->izcv1 + (1 << FBsc);
            if (il->nizvar) il->rspmethu = RF_IZH3X;
            }
         else if (il->Ct.rspmeth >= RF_IZH7) {
            struct IZ07DEF *pi7 = (struct IZ07DEF *)il->prfi;
#define OVFI OVF_IZHI   /* Just to save wraparound lines */
            pi7->ukoCm  = msrsle(pi7->izk,  il->RCm, -28, OVFI);
            pi7->Z.izb  = msrsle(pi7->iizb, il->RCm, -29, OVFI);
            pi7->Z.izd  = msrsle(pi7->iizd, il->RCm, -31, OVFI);
            pi7->Z.izrb = (ui16)msrule((ui32)pi7->iizrb, il->RCm,
               -30, OVFI);
            pi7->Z.izrd = (ui16)msrule((ui32)pi7->iizrd, il->RCm,
               -34, OVFI);
            if (pi7->bvlo == MINUS0)   {
               pi7->ubvlooCm = pi7->Z.izb;
               pi7->usmvtest = MINUS0; }
            else {
               pi7->ubvlooCm = msrsle(pi7->bvlo, il->RCm, -29, OVFI);
               pi7->usmvtest = 0;
               il->rspmethu = RF_IZH7X; }
            if (pi7->Z.izrb == 0) pi7->usmvtest = SI32_MAX;
            if (pi7->umax == SI32_MAX) pi7->uumaxoCm = SI32_MAX;
            else {                     pi7->uumaxoCm =
               msrsle(pi7->umax, il->RCm, -31, OVFI);
               il->rspmethu = RF_IZH7X; }
            pi7->uumctCm = msrsle(pi7->umc, il->Ct.Cm, -27, OVFI);
            pi7->uumvptCm = msrsle(pi7->umvp, il->Ct.Cm, -27, OVFI);
            pi7->uuv3oCm = msrsle(pi7->uv3, il->RCm, -28, OVFI);
            if (pi7->uv3lo == MINUS0)  pi7->uuv3looCm = pi7->uuv3oCm;
            else {                     pi7->uuv3looCm =
               msrsle(pi7->uv3lo, il->RCm, -28, OVFI);
               il->rspmethu = RF_IZH7X; }
            if (pi7->umc | pi7->umvp | pi7->uv3 | (si32)il->nizvar)
               il->rspmethu = RF_IZH7X;
            } /* End if RF_ZIH7 */
         } /* End if >= RF_BREG */
#undef OVFI
      } /* End il loop */

   } /* End d3bgis() */
