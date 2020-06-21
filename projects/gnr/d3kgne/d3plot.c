/* (c) Copyright 1992-2009, Neurosciences Research Foundation, Inc. */
/* $Id: d3plot.c 48 2012-04-23 18:40:15Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                              d3plot.c                                *
*          Perform various plotting options at end of cycle            *
*                                                                      *
*     This routine should be called in parallel on all nodes.  If      *
*  doing superposition plots, it handles titles, Darwin 1 plots,       *
*  color scale plots, Cij plots, environment plot, virtual cell plot,  *
*  user labels and lines, and value.  In this mode, d3ngph and tvplot  *
*  have already been called from main, and d3rplt has been called      *
*  from d3go or d3rpgt.  Whether or not superposition plots are done,  *
*  dynamic configuration plots can be done--d3plot calls d3cnfg for    *
*  this purpose.                                                       *
*     If neither superposition plots nor dynamic configuration plots   *
*  are done, this routine performs all requested individual plots.     *
*     Repertoire printing is done before plotting so printing will     *
*  be complete even if user intervention occurs during plotting.       *
*                                                                      *
*  Arguments:                                                          *
*     None--control switches are now in RP->kpl1 and RP->kpr           *
*                                                                      *
*  Value returned:                                                     *
*     A meaningful value which caller will store in RP->CP.outnow.     *
*     Value indicates whether to terminate trial series due to         *
*     user action detected by newplt.                                  *
*                                                                      *
*  V5E, 06/06/92, GNR - Split out from darwin3.c                       *
*  Rev, 07/27/92, GNR - Omit VALUE title if no value bars plotted      *
*  Rev, 02/27/93, GNR - Remove snapshot support                        *
*  V6B, 07/29/93, GNR - Add VCELL plotting via d3vplt()                *
*  V6D, 02/04/94, GNR - Add d3ngph and d3lplt calls for non-superpos.  *
*  V7C, 01/21/95, GNR - Implement KRPRR                                *
*  V8A, 03/10/97, GNR - Implement KRPEP, 01/11/98 KRPEP per celltype   *
*  Rev, 04/24/98, GNR - Pick up return value from d3vplt properly      *
*  V8B, 01/27/01, GNR - Implement separate timers for all plot options *
*  Rev, 08/27/01, GNR - Separate chains of PLBDEF and PLNDEF blocks    *
*  Rev, 02/16/05, GNR - Add line thickness parameter                   *
*  V8D, 01/04/07, GNR - Move d3rpsi() output to d3go, run every cycle  *
*  Rev, 10/02/07, GNR - Add PSCALE, 12/24/07, add value column 2       *
*  Rev, 02/27/08, GNR - Add finplt call                                *
*  ==>, 02/28/08, GNR - Last mod before committing to svn repository   *
*  Rev, 01/17/09, GNR - Reorganize colors into a single array          *
***********************************************************************/

#define PLBTYPE struct PLBDEF
#define PLNTYPE struct PLNDEF

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "d3global.h"
#include "d3fdef.h"
#ifdef D1
#include "d1def.h"
#endif
#ifndef PARn
#include "rocks.h"
#include "rkxtra.h"
#endif /* PARn */
#include "bapkg.h"
#include "plbdef.h"
#include "plots.h"

void d3ijpz(void);
void d3ijpn(void);

int d3plot(void) {

   struct REPBLOCK *ir;       /* Ptr to repertoire block */
   struct CELLTYPE *il;       /* Ptr to celltype block */
#ifndef PARn
   struct PLBDEF *pplb;       /* Ptr to plot label block */
   struct PLNDEF *ppln;       /* Ptr to plot line block */
   struct VBDEF  *pvb;        /* Ptr to a value block */
   struct VCELL  *pvc;        /* Ptr to a virtual cell block */

   float lettht = RP->stdlht; /* Letter height */

   int krt = RP->kplot & PLRTR ? RETRACE : NORETRACE;
#endif

/* Perform all requested region prints.  RP->kpr will be OFF
*  if any of the generic filters (NOPRINT command-line option,
*  etc.) fails.  Only those tests specific to each region
*  must be tested inside the region loop.  */

   if (RP->kpr & KPRREP) {
      for (ir=RP->tree; ir; ir=ir->pnrp) {
         if ((ir->Rp.krp & KRPRR) && ((ir->Rp.krp & KRPAP) ||
            (RP->CP.runflags & RP_FREEZE)) &&
            bittst(RP->CP.trtimer, (long)ir->Rp.jrptim)) d3rprt(ir);
         } /* End repertoire loop */
      } /* End if KPRREP */

/* Perform plotting actions required for superposition plots
*  (d3ngph() already called from main program) */

   stattmr(OP_PUSH_TMR,GRPH_TMR);   /* Count on graphics timer */
   if (RP->kpl1 & (KPLSUP|KPLMFSP)) {

#ifdef D1
      /* Plot Darwin 1 repertoires.  It is currently a restriction
      *  that Darwin 1 is plotted only when in superposition mode. */
      d1rplt();               /* D1 plot on all nodes */
#endif

      /* Perform Cij,Mij,etc. plots */
#ifdef PARn
      if (RP->pijpn)
         d3ijpn();            /* Send IJPLOT data to host */
#endif
#ifdef PAR0
      if (RP0->pfijpl)
         d3ijpz();
#endif
#ifndef PAR
      if (RP0->pfijpl)
         d3ijpl();
#endif

#ifndef PARn
/* Superposition plot items done on host only */

      /* Set retrace switch */
      retrace(krt);

      /* Plot series and trial numbers at upper left corner */
      if (!(RP->kplot & PLNST)) {
         pencol(RP->colors[CI_BOX]);
         symbol(0.5, RP->sphgt-lettht, 0.7*lettht,
            RP0->stitle+1, 0.0, RP0->lst-1);
         }

      /* Plot color scale */
      if (RP->kplot & PLCSP) {
         float bb;               /* Border width */
         float bx,by;            /* Location of a color box */
         float bw,bh;            /* Width, height of one box */
         int ic;                 /* Color index */
         bb = PLCSP_BORDER*RP0->splw;
         bx = RP0->splx + bb; by = RP0->sply + bb; bb *= 2.0;
         pencol(RP->colors[CI_BOX]);
         rect(RP0->splx, RP0->sply, RP0->splw, RP0->splh, THICK);
         if (RP0->splh >= RP0->splw) {
            /* Set for vertical scale */
            bw = RP0->splw - bb;
            bh = (RP0->splh - bb)/SEQNUM;
            for (ic=CI_SEQ; ic<CI_PHS; ++ic,by+=bh) {
               pencol(RP->colors[ic]);
               rect(bx, by, bw, bh, FILLED); }
            }
         else {
            /* Set for horizontal scale */
            bw = (RP0->splw - bb)/SEQNUM;
            bh = RP0->splh - bb;
            for (ic=CI_SEQ; ic<CI_PHS; ++ic,bx+=bw) {
               pencol(RP->colors[ic]);
               rect(bx, by, bw, bh, FILLED); }
            }
         }

      /* Plot labels */
      for (pplb=RP0->pplb1; pplb; pplb=pplb->pnxplb)
            if (pplb->kplbenb) {
         char *plabel = getrktxt(pplb->hplabel);
         pencol(pplb->lbcolor);
         symbol(pplb->lbx, pplb->lby, pplb->ht,
            plabel, pplb->ang, (int)pplb->lplabel);
         } /* End pplb loop */

      /* Plot value bars */
      if ((RP->nvblk > 0) && (RP0->vplw > 0.0) && (RP0->vplh > 0.0)) {
         struct VDTDEF *pval = RP->pvdat;
         float vwids8 = RP0->vplw/dS8;
         float xl,yl,yl0[NVCOLS];
         float bwid;             /* Bar width */
         long  aval;             /* |fullvalue| */
         int   ic;               /* Column selector */
         memcpy((char *)yl0, (char *)RP0->yvstrt, sizeof(yl0));
         for (pvb=RP->pvblk; pvb; pvb=pvb->pnxvb, pval++) {
            if (pvb->vbopt & VBVOPNP) continue;
            ic = (pvb->vbopt & VBVOP2C) != 0;
            aval = labs(pval->fullvalue);
            bwid = vwids8*(float)min(256,aval);
            xl = RP0->vplx[ic];
            yl = yl0[ic] -= RP0->vplh;
            if (bwid < 0.025) {  /* Minimal value */
               pencol("X444");
               line(xl,yl,xl,yl+RP0->yvbxht);
               }
            else if (pval->fullvalue >= 0) {
               pencol(RP->colors[CI_EXC]);
               rect(xl,yl,bwid,RP0->yvbxht,FILLED);
               }
            else {
               pencol(RP->colors[CI_INH]);
               rect(xl-bwid,yl,bwid,RP0->yvbxht,FILLED);
               }
            }  /* End of for loop over pvb */
         for (ic=0; ic<NVCOLS; ++ic) if (yl0[ic] < RP0->yvstrt[ic]) {
            pencol(RP->colors[CI_BOX]);
            symbol(RP0->vplx[ic]-2*lettht,RP0->yvstrt[ic],lettht,
               "VALUE:",0.0,6);
            } /* End plotting value headers */
         }  /* End if nvblk > 0 && vplw > 0.0 && vplh > 0.0 */

      /* Plots below this point have their own retrace options */

      /* Plot lines */
      for (ppln=RP0->ppln1; ppln; ppln=ppln->pnxpln)
            if (ppln->kplnenb) {
         pencol(ppln->lncolor);
         retrace(ppln->kthick >= 0 ? (int)ppln->kthick : krt);
         line(ppln->x1, ppln->y1, ppln->x2, ppln->y2);
         } /* End ppln loop */

      /* Plot simulated environment */
      if ((!(RP->CP.runflags & RP_NOINPT)) && (RP->eplw > 0.0))
         envplot(RP0->Eptr, RP->pstim + RP->nsy, RP->eplx, RP->eply,
            RP->eplw, RP->eplh, NULL);

      /* Plot virtual sense cells */
      for (pvc=RP0->pvc1; pvc; pvc=pvc->pnvc)
         if ((pvc->kvp & VKPPL) && ((RP->CP.outnow = d3vplt(
            pvc, RP->CP.trial, pvc->vcplx, pvc->vcply,
               pvc->vcplw, pvc->vcplh)) > 0)) return RP->CP.outnow;

#endif                        /* End host superposition items */
      } /* End superposition */

   else {               /* Not superposition */

/* Perform individual environment plot */

      if (RP->kpl1 & KPLENV) {
         if ((RP->CP.outnow = /* Start plot on all nodes */
            d3ngph(11.0, 11.0, 0, 0, "DEFAULT", RP->colors[CI_BOX],
               "CLIP", 0)) > 0) return RP->CP.outnow;
#ifndef PARn
         envplot(RP0->Eptr, RP->pstim + RP->nsy, 0.5, 0.5,
            RP0->efplw, RP0->efplh, RP0->stitle+1);
#endif
         } /* End envplot */

/* Non-superposition rplot (all nodes):
*  Plot origin, width and height are fixed here for full frame.
*  The superposition plot calls to d3rplt & d3lplt are in d3go,
*  due to the need to access source activity (Sj's) for neuro-
*  anatomy plots (in parallel version, this information is not
*  available following d3exch), multiple calls for inner cycles
*  with KP=V, and to distribute plotting activity for greater
*  parallelism.  When not doing superpositions, all the plotting
*  is done from here.  There is no need for Sj's in this case,
*  and this arrangement leaves open the possibility of permitting
*  both superposition and nonsuperposition plots in the same run.
*  However, if nonsuperposition plots at all inner cycles are
*  desired, the following code must be moved to d3go.  In V6D,
*  calls to d3ngph were moved out of d3rplt for consistency with
*  superposition plots, and calls to d3lplt were added.  */

      if (RP->kpl1 & KPLIRP) for (ir=RP->tree; ir; ir=ir->pnrp) {
         long lkrp = ir->Rp.krp;
         if (lkrp & (KRPPL|KRPBP) && (lkrp & KRPAP ||
               (RP->CP.runflags & RP_FREEZE)) &&
               bittst(RP->CP.trtimer, (long)ir->Rp.jbptim)) {
            if ((RP->CP.outnow = d3ngph(8.5, 11.0, 0,0, "DEFAULT",
               RP->colors[CI_BOX], "CLIP", 0)) > 0)
               return RP->CP.outnow;
            d3rplt(ir,0,0.5,1.0,7.5,9.5);
            for (il=ir->play1; il; il=il->play) {
               long llo = il->locell;
               d3lplt(il,il->pps[0]+spsize(llo,il->phshft),0,
                  0.5,1.0,7.5,9.5);
               }
            } /* End if plotting this repertoire */
         } /* End repertoire loop */

/* Non-superposition virtual cell plot.  N.B.  Although plotting
*  occurs only on host, comp nodes must participate in d3ngph
*  call and be prepared to exit if user interrupt occurs at this
*  time.  RP->nvcpl gives number of calls to d3ngph required.
*  The actual VCELL blocks are only maintained on host.  This is
*  why we only have a global timer on the CYCLE card, not one on
*  each SENSE card.  */

      if (RP->kpl1 & KPLVCP) {
#ifndef PARn
         for (pvc=RP0->pvc1; pvc; pvc=pvc->pnvc)
            if (pvc->kvp & VKPPL) if ((RP->CP.outnow =
               d3vplt(pvc, RP->CP.trial, 0.5, 1.0, 7.5, 9.5)) > 0)
               return RP->CP.outnow;
#else
         int ivcpl = RP->nvcpl;
         while (ivcpl--) if ((RP->CP.outnow = d3ngph(
               11.0, 11.0, 0, 0,"DEFAULT","DEFAULT","CLIP", 0)) > 0)
            return RP->CP.outnow;
#endif
         } /* End if KPLVCP */

      } /* End else not superposition */

/* Generate dynamic configuration plot if requested.
*  N.B.  This is currently a stub.  When actually implemented,
*  it may need to be called on PARn nodes for plot synch.
*  This will require a changed prototype in d3global.h  */

#ifndef PARn
   if (RP->kpl1 & KPLDYN) d3cnfg(1, RP->pstim);
#endif

   RP->CP.outnow = finplt();
#if OUT_USER_PAUSE != BUT_INTX || OUT_USER_QUIT != BUT_QUIT
   if (RP->CP.outnow == BUT_INTX)      RP->CP.outnow = OUT_USER_PAUSE;
   else if (RP->CP.outnow == BUT_QUIT) RP->CP.outnow = OUT_USER_QUIT;
#endif

   stattmr(OP_POP_TMR,0);     /* Return to previous timer */
   return RP->CP.outnow;
   } /* End d3plot */

