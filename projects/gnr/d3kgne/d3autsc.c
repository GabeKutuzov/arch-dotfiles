/* (c) Copyright 2012, Neurosciences Research Foundation, Inc. */
/* $Id: d3autsc.c 50 2012-05-17 19:36:30Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                               d3autsc                                *
*            Calculations relating to response autoscaling             *
************************************************************************
*  V8G, 08/13/10, GNR - New routine                                    *
*  ==>, 08/28/10, GNR - Last mod before committing to svn repository   *
*  V8H, 05/09/12, GNR - Add KAUT_ET option                             *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "sysdef.h"
#include "rocks.h"
#include "rkarith.h"
#include "d3global.h"
#include "celldata.h"

extern struct CELLDATA CDAT;

#ifndef PAR0
/*=====================================================================*
*                               d3assv                                 *
*                                                                      *
*  Passed a response value (before any truncation), d3assv() saves the *
*  largest ngtht responses on its node in the CDAT.prgtht work area.   *
*  Low addresses in the table contain the highest responses.  Code     *
*  assumes it is not called if AUTOSCALE not initialized and in use.   *
*=====================================================================*/

void d3assv(struct AUTSCL *paut, si32 response) {

   si32 *p1,*p2,*pe,*pm;         /* Binary search pointers */
   si32 *prgt = CDAT.prgtht;     /* Ptr to sorted response table */
   ui32 mxigt = paut->ngtht;     /* Max size of table */
   ui32 itop = CDAT.myngtht;     /* Current length of table */

/* If the table is empty, start it by storing the first response */

   if (!itop) goto AddResponseAtTop;

/* If the response is lower than the lowest response in the table,
*  and the table is full, return at once as this common case will
*  save a little time.  Otherwise, add it at the end of the table
*  and increment the count.  */

   if (response < prgt[itop-1]) {
      if (itop == mxigt) return;
      goto AddResponseAtTop;
      }

/* Perform a binary search to find the location in the table where
*  the new response needs to be inserted.  When the "do" terminates,
*  p1 points to the insertion location:  either response was <= *pm,
*  and p1 = pm + 1, or response was > *pm and p1 = pm.  */

   p1 = prgt, p2 = pe = prgt + itop - 1;
   do {
      pm = p1 + ((p2 - p1) >> 1);
      if (response <= *pm) {
         p1 = pm + 1;
         /* (Putting this test inside the other one
         *  saves doing it half the time.)  */
         if (response == *pm) break;
         }
      else
         p2 = pm - 1;
      } while (p1 <= p2);

/* Move all the lower entries down.  This will result in
*  incrementing myngtht unless the table is already full.  */

   if (itop < mxigt) {
      pe += 1;
      CDAT.myngtht += 1; }
   for (pm=pe; pm>p1; --pm) pm[0] = pm[-1];
   *p1 = response;
   return;

/* Table is not full and adding new response at end */

AddResponseAtTop:
   prgt[itop] = response;
   CDAT.myngtht += 1;
   return;

   } /* End d3assv() */
#endif


/*=====================================================================*
*                               d3autsc                                *
*                                                                      *
*  Called from d3go at the end of a trial, this routine merges the     *
*  lists of largest responses to node 0 (if parallel), computes the    *
*  new scale adjustment factor, and stores it into the sensory         *
*  broadcast array for broadcast at the start of the next trial.       *
*=====================================================================*/

void d3autsc(struct CELLTYPE *il) {

   struct AUTSCL *paut = il->pauts;
   si32 *psm = RP->psclmul + il->oautsc;  /* Ptr to scale */
#ifdef PARn
   si32 *psmy,*psin,*psou;    /* Ptrs to merge data blocks */
   si32 *psi,*psj,*pso;       /* Ptrs to data being merged */
   si32 *pse;                 /* End of output block */
#else /* !PARn */
   si32 selresp;              /* Selected ngtht'th response */
#endif
   si32 newscl;               /* New scale */
   ui16 lkaut = il->kautu;

   if (!(lkaut & KAUT_NOW))
      goto OmitScaleCalc;

#ifdef PAR
/*---------------------------------------------------------------------*
*           This merge is only needed on parallel computers            *
*                                                                      *
*  During merge, only the ngtht largest responses from each partner    *
*  node are kept.  Out of a lack of hardware to test algorithms for    *
*  different geometries, the code is optimized for hypercubes, with    *
*  the hope that message passing time will not depend very much on     *
*  the source-destination path with modern hardware.  Code carries     *
*  out one merge step for each dimension of the hypercube.  On each    *
*  channel (looping right to left), odd node sends data to even        *
*  partner and exits.  Even node merges received data with its own     *
*  and swaps pointers.  Repeat for log2(num nodes) passes.             *
*                                                                      *
*  For now, assumes all nodes have same byte order--if not, we must    *
*  fix d3exch also.                                                    *
*---------------------------------------------------------------------*/

   int src;
   int chan;                  /* Current hypercube channel */
   int mlen = sizeof(si32)*paut->ngtht;   /* Message length */
   int type = AUTSID_MSG;

   stattmr(OP_PUSH_TMR, COMM_TMR);

#ifdef PAR0

/* Receive the large s(i) data */

   src = 0;
   anread(CDAT.prgtht, mlen, &src, &type, "Big S(i)");

#else    /* I am a comp node */

   /* Locate working storage for merge */
   psmy = CDAT.prgtht;
   psou = (si32 *)CDAT.psws;
   psin = psou + paut->ngtht;

   /* Fill out ngtht entries in psmy with -SI32_MAX.
   *  This greatly simplifies the merge bookkeeping.
   *  (A zero-turn loop if already got ngtht entries.)  */
   psi = psmy + CDAT.myngtht;
   pse = psmy + paut->ngtht;
   while (psi < pse) *psi++ = -SI32_MAX;

/* Assemble merged list of high responses on node 0 */

   for (chan=1; chan<NC.total; chan<<=1) {

      int npard = NC.node ^ chan;      /* Partner node */

      if (NC.node & chan) {
         /* I am an odd node, send data to even partner and exit */
         anwrite(psmy, mlen, npard, type, "Big S(i)");
         break;
         }

      else {
         /* I am an even node, merge and forward */
         anread(psin, mlen, &npard, &type, "Big S(i)");
         psi = psmy, psj = psin, pso = psou;
         pse = pso + paut->ngtht;
         while (pso < pse) {
            if (*psi >= *psj) *pso++ = *psi++;
            else              *pso++ = *psj++;
            }
         /* Swap output and mydata blocks for next round */
         pse = psou, psou = psmy, psmy = pse;
         } /* End merge on even node */
      } /* End node loop */

/* Head node returns updated s(i) data to host */

   if (NC.node==NC.headnode)
      anwrite(psmy, mlen, NC.hostid, type, "Big S(i)");

#endif /* Not PAR0 */

   stattmr(OP_POP_TMR, COMM_TMR);

#endif /* PAR */

/*---------------------------------------------------------------------*
*  Merge to node 0 is complete.  Calculate the new scale multiplier.   *
*                                                                      *
*  Possible enhancements to this simple code: (1) Use the average of   *
*  some number of responses near ngtht, (2) have a fast-start option   *
*  active on maybe the first trial of the run, or the first trial of   *
*  a series, or the first trial after a reset in which the scales      *
*  were reset...                                                       *
*---------------------------------------------------------------------*/

#ifndef PARn
   /* If selected response is < ht/S7, set newscl to high limit, because
   *  we could otherwise generate a negative scale multiplier or get a
   *  divide check (not good).  Still must broadcast to waiting PARn's.
   *  User intervention will be needed to fix this celltype.  */
   if (!CDAT.myngtht || (selresp = CDAT.prgtht[CDAT.myngtht-1]) <=
         il->Ct.ht >> (31-FBsc))
      newscl = paut->asmximm;
   /* Calculate ratio of ht/(selected response) (S24), used both
   *  for immediate and later rescaling */
   else
      newscl = jdswq(jslsw(jesl(il->Ct.ht),FBsc),selresp);

   /* If doing statistics, record largest and smallest newscl */
   if (!(il->Ct.kctp & CTPNS)) {
      if (newscl < il->CTST->loascl) il->CTST->loascl = newscl;
      if (newscl > il->CTST->hiascl) il->CTST->hiascl = newscl;
      }

   /* Calculate running average new scale (unless doing fast start
   *  or this is first cycle of a trial in OPT=T mode).  Combine
   *  additively if immediate scaling, else multiplicatively.  */
   if (lkaut & KAUT_UPD) {
      si32 oldscl = *psm;
      if (lkaut & KAUT_APP) newscl = oldscl +
         mssle(paut->adamp, newscl-oldscl, -FBdf, OVF_AUTS);
      else {
         si32 tt = S24 +
            mssle(paut->adamp, newscl-S24, -FBdf, OVF_AUTS);
         newscl = mssle(oldscl, tt, -FBsc, OVF_AUTS);
         }
      /* Do not change by more than + or - asmxd */
      if (newscl > oldscl) {
         si32 tbig = mssle(oldscl, S24 + paut->asmxd, -FBsc, OVF_AUTS);
         if (newscl > tbig) newscl = tbig;
         }
      else if (newscl < oldscl) {
         si32 tsma = mssle(oldscl, S24 - paut->asmxd, -FBsc, OVF_AUTS);
         if (newscl < tsma) newscl = tsma;
         }
      }
   /* Apply immediate rescaling limit here on Node 0 */
   if (lkaut & KAUT_APP) {
      if      (newscl > paut->asmximm) newscl = paut->asmximm;
      else if (newscl < paut->asmnimm) newscl = paut->asmnimm;
      }
   *psm = newscl;
#endif

/* If parallel, distribute to comp nodes.  This is currently the
*  only material broadcast possibly on every cycle, and we need
*  to apply the scale before input to any downstream layers, so
*  there is nothing to consolidate it with.  */
#ifdef PAR
   blkbcst(psm, ttloc(IXsi32), 1, AUTSCL_MSG);
#endif

OmitScaleCalc: ;

/* Now all nodes have the new scale.  If requested, apply it to
*  the waiting new responses in il->ps2 and update sbar and qbar.  */
#ifndef PAR0
   if (lkaut & KAUT_APP) {
      s_type *psi = il->ps2;
      rd_type *plsd = il->prd;
      long icell, celllim = il->mcells;
      si32 ntrunc = 0;
      si32 rnd1 = paut->asrsd;
      newscl = *psm;
      for (icell=0; icell<celllim; ++icell,psi+=il->lsp) {
         si32 wksi;
         d3gts2(wksi, psi);
         wksi = msrsle(wksi, newscl, -FBsc, OVF_AUTS);
         if (RP->compat & OLDRANGE) {  /* Force old range */
            if (wksi > S14)     wksi = S14, ntrunc += 1;
            else if (wksi < 0)  wksi = 0;
            }
         else if (abs32(wksi) > SHRT_MAX) {
            if (wksi > 0) wksi = SHRT_MAX, ntrunc += 1;
            else wksi = -SHRT_MAX, ntrunc += 1;
            }
         d3ptl2(wksi, psi);

/* Note:  To minimize subroutine call overhead, this sbar,qbar
*  update code was copied from d3go rather than being put in a
*  separate routine callable from both places.  */

         if (il->ksqbup & KSQUP_AS) {
            if (ctexists(il,SBAR)) {
               rd_type *psbr = plsd + il->ctoff[SBAR];
               long sbar1;
               if (il->ctf & CTFDOFS)
                  sbar1 = wksi;
               else {
                  d3gts2(sbar1, psbr);
                  sbar1 = sbar1*(long)il->ctwk.go.sdampc +
                     wksi*(long)il->Dc.sdamp + (rnd1 >> Ss2hi);
                  sbar1 = SRA(sbar1, FBdf);
                  }
               d3ptl2(sbar1, psbr);
               }
            if (ctexists(il,QBAR)) {
               rd_type *pqbr = plsd + il->ctoff[QBAR];
               long qbar1;
               if (il->ctf & CTFDOFS)
                  qbar1 = wksi;
               else {
                  d3gts2(qbar1, pqbr);
                  qbar1 = qbar1*(long)il->ctwk.go.qdampc +
                     wksi*(long)il->Dc.qdamp + (rnd1 & (S15-1));
                  qbar1 = SRA(qbar1, FBdf);
                  }
               d3ptl2(qbar1, pqbr);
               }
            plsd += il->lel;
            udevskip(&rnd1, 4);
            } /* End if KSQUP_AS */
         } /* End cell loop */
      /* Store truncation count if doing statistics */
      if (il->ctf & DIDSTATS)   il->CTST->sover += ntrunc;
      } /* End rescaling cell responses */
#endif
   } /* End d3autsc() */


#ifndef PARn
/*=====================================================================*
*                               d3aprt                                 *
*                                                                      *
*  Print autoscale multipliers (currently DP=A on CYCLE card)          *
*=====================================================================*/

void d3aprt(void) {

   if (RP0->nautsc) {
      struct CELLTYPE *il;
      char line[20+2*LSNAME];
      int ias;
      /* One line for every four celltypes plus one for title */
      if (RP->kdprt & PRSPT) spout(((int)RP0->nautsc+7)>>2);
      cryout(RK_P2, "0Autoscale multipliers:", RK_LN2, NULL);
      for (il=RP->pfct,ias=0; il; il=il->pnct) {
         if (!il->kautu) continue;
         if ((ias++ & 3) == 0) cryout(RK_P2, " ", RK_LN1+1, NULL);
         sconvrt(line, "(3XA" qLSN ",XA" qLSN ",2X,JB24IJ10.<5)",
            il->pback->sname, il->lname, RP->psclmul+il->oautsc,
            NULL);
         cryout(RK_P2, line, RK_CCL, NULL);
         } /* End CELLTYPE loop */
      if (ias & 3) cryout(RK_P2, "\0", RK_LN0+RK_FLUSH+1, NULL);
      } /* End if nautsc */
   } /* End d3aprt() */
#endif
