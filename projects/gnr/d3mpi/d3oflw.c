/* (c) Copyright 2012-2017, The Rockefeller University *11115* */
/* $Id: d3oflw.c 78 2018-08-02 18:36:58Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                               d3oflw                                 *
*                                                                      *
*  Collect and print overflow counts stored in CDAT.pover array.       *
*  (These counts are printed regardless of whether d3stat statistics   *
*  are collected and printed, because they may be important for model  *
*  optimization.)                                                      *
*                                                                      *
************************************************************************
*  V8I, 12/25/12, GNR - New routine                                    *
*  ==>, 01/05/13, GNR - Last mod before committing to svn repository   *
*  Rev, 08/22/14, GNR - Si only EXP decay, remove DECAYDEF from DCYDFLT*
*  R67, 09/30/16, GNR - Use coppack instead of vecollect to sum oflws  *
*  R76, 11/14/17, GNR - Add CNO_SUMS, CNO_VDEP, delete CTO_VDEP        *
*  R80, 08/30/18, GNR - Add CNO_TER                                    *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "sysdef.h"
#include "rkarith.h"
#include "d3global.h"
#include "celldata.h"
#include "collect.h"
#ifndef PARn
#include "rocks.h"
#include "rkxtra.h"
#endif

extern struct CELLDATA CDAT;

#ifdef PAR
/* Define COPDEF structure for collecting pover stats */
static struct COPDEF OflwCop = { ADD+LONG, 0 };
#endif

/***********************************************************************
*                               d3oflw                                 *
***********************************************************************/

void d3oflw(void) {

#ifndef PARn
   struct REPBLOCK *ir;       /* Ptr to current repertoire */
   struct CELLTYPE *il;       /* Ptr to current cell block */
   struct CONNTYPE *ix;       /* Ptr to current connection block */
   struct INHIBBLK *ib;       /* Ptr to current gconn block */
   struct MODVAL  *imv;       /* Ptr to current modulation block */
   struct VBDEF   *ivb;       /* Ptr to current value def block */
   long *poc,*poce;           /* Ptrs to overflow counts */
   si64 toto;                 /* Total overflows */
#endif

/* If parallel, collect overflow stats to node 0 */

#ifdef PAR
   {  CPKD *pcpk = coppini(OFLW_CT_MSG, 0, CPK_INC0);
      OflwCop.count = RP->novec;
      coppack(pcpk, CDAT.povec, &OflwCop, 1);
      copcomb(pcpk);
      copfree(pcpk);
      }
#endif

/* Add up the grand total and quit if no overflows */

#ifndef PARn
   toto = jesl(0);
   poce = CDAT.povec + RP->novec;
   for (poc=CDAT.povec; poc<poce; ++poc)
      toto = jaslo(toto, *poc);
   if (!qsw(toto)) return;

   /* If requested, SPOUT everything */
   if (RP->CP.runflags & RP_SSTAT) spout(SPTM_ALL);

   /* Make a global subtitle */
   cryout(RK_P1, "0---ARITHMETIC OVERFLOW COUNTS---",
      RK_NFSUBTTL+RK_LN2, NULL);

/* Loop over all objects that can have iovec's (so can id them) */

   poc = CDAT.povec;
   for (ir=RP->tree; ir; ir=ir->pnrp) {
      for (il=ir->play1; il; il=il->play) {
         char *lnm = fmtlrlnm(il);

         /* Overflows at cell level */
         if (poc[il->iovec+CTO_ALLO])
            convrt("(P1,1X,IL10,4H In A" qLXN ",' mem alloc')",
               poc+il->iovec+CTO_ALLO, lnm, NULL);
         if (poc[il->iovec+CTO_RFN])
            convrt("(P1,1X,IL10,4H In A" qLXN ",' response function')",
               poc+il->iovec+CTO_RFN, lnm, NULL);
         if ((enum RespFunc)il->rspmethu >= RF_IZH3 &&
               poc[((struct IZHICOM *)il->prfi)->iovec])
            convrt("(P1,1X,IL10,4H In A" qLXN
               ",' Izhikevich response function')",
               poc+((struct IZHICOM *)il->prfi)->iovec, lnm, NULL);
         if ((enum RespFunc)il->rspmethu == RF_BREG &&
               poc[((struct BREGDEF *)il->prfi)->iovec])
            convrt("(P1,1X,IL10,4H In A" qLXN
               ",' aEIF response function')",
               poc+((struct BREGDEF *)il->prfi)->iovec, lnm, NULL);
         if (il->phshft && poc[il->pctpd->iovec])
            convrt("(P1,1X,IL10,4H In A" qLXN ",' phasing')",
               poc+il->pctpd->iovec, lnm, NULL);
         if (poc[il->iovec+CTO_RSET])
            convrt("(P1,1X,IL10,4H In A" qLXN ",' s(i) reset')",
               poc+il->iovec+CTO_RSET, lnm, NULL);
         if (poc[il->iovec+CTO_DCY])
            convrt("(P1,1X,IL10,4H In A" qLXN
            ",' s(i), sbar, depr, etc. decay')",
               poc+il->iovec+CTO_DCY, lnm, NULL);
         if (poc[il->iovec+CTO_PSI])
            convrt("(P1,1X,IL10,4H In A" qLXN ",' psi calc')",
               poc+il->iovec+CTO_PSI, lnm, NULL);
         if (poc[il->iovec+CTO_AFF])
            convrt("(P1,1X,IL10,4H In A" qLXN ",' afference terms')",
               poc+il->iovec+CTO_AFF, lnm, NULL);
         if (poc[il->iovec+CTO_RGEN])
            convrt("(P1,1X,IL10,4H In A" qLXN ",' regeneration')",
               poc+il->iovec+CTO_RGEN, lnm, NULL);
         if (il->pauts && poc[il->pauts->iovec])
            convrt("(P1,1X,IL10,4H In A" qLXN ",' autoscaling')",
               poc+il->pauts->iovec, lnm, NULL);

         /* Overflows at specific connection level */
         for (ix=il->pct1; ix; ix=ix->pct) {
            if (poc[ix->iovec+CNO_SUMS])
               convrt("(P1,1X,IL10,4H In A" qLXN ",' conntype ',J1UH6"
                  ",'afferent sums')",
                  poc+ix->iovec+CNO_SUMS, lnm, &ix->ict, NULL);
            if (poc[ix->iovec+CNO_RED])
               convrt("(P1,1X,IL10,4H In A" qLXN ",' conntype ',J1UH6"
                  ",'synaptic scaling')",
                  poc+ix->iovec+CNO_RED, lnm, &ix->ict, NULL);
            if (poc[ix->iovec+CNO_AMP])
               convrt("(P1,1X,IL10,4H In A" qLXN ",' conntype ',J1UH6"
                  ",'amplification')",
                  poc+ix->iovec+CNO_AMP, lnm, &ix->ict, NULL);
            if (poc[ix->iovec+CNO_CDCY])
               convrt("(P1,1X,IL10,4H In A" qLXN ",' conntype ',J1UH6"
                  ",'Cij decay')",
                  poc+ix->iovec+CNO_CDCY, lnm, &ix->ict, NULL);
            if (poc[ix->iovec+CNO_VDEP])
               convrt("(P1,1X,IL10,4H In A" qLXN ",' conntype ',J1UH6"
                  ",'voltage-dependence')",
                  poc+ix->iovec+CNO_VDEP, lnm, &ix->ict, NULL);
            if (poc[ix->iovec+CNO_TER])
               convrt("(P1,1X,IL10,4H In A" qLXN ",' conntype ',J1UH6"
                  ",'ternary modification')",
                  poc+ix->iovec+CNO_TER, lnm, &ix->ict, NULL);
            }

         /* Overflows at geometric connection level */
         for (ib=il->pib1; ib; ib=ib->pib) {
            if (poc[ib->iovec])
               convrt("(P1,1X,IL10,4H In A" qLXN ",' gconn type ',"
                  "J1UH6,'gconn decay')",
                  poc+ib->iovec, lnm, &ib->igt, NULL);
            }

         /* Overflows at modulatory connection level */
         for (imv=il->pmrv1; imv; imv=imv->pmdv) {
            if (poc[imv->iovec]) {
               rlnm vrlnm;
               memcpy(vrlnm.rnm, ir->sname, LSNAME);
               memcpy(vrlnm.lnm, il->lname, LSNAME);
               convrt("(P1,1X,IL10,' for modulation ',A32)",
                  poc+imv->iovec, fmtsrcnm(&vrlnm, REPSRC), NULL);
               }
            }

         } /* End loop over cell types */
      } /* End loop over repertoires */

   /* Overflows from modulation by virtual groups.
   *  Make a fake rlnm for use by fmtsrcnm  */
   for (imv=RP->pmvv1; imv; imv=imv->pmdv) {
      if (poc[imv->iovec]) {
         rlnm vrlnm;
         vrlnm.hbcnm = (si16)imv->mdsrcndx;
         convrt("(P1,1X,IL10,' for modulation ',A32)",
            poc+imv->iovec, fmtsrcnm(&vrlnm, imv->mdsrctyp), NULL);
         }
      }

   /* Overflow in value calculations */
   for (ivb=RP->pvblk; ivb; ivb=ivb->pnxvb) {
      if (poc[ivb->iovec])
         convrt("(P1,1X,IL10,' for value ',J0IH6)",
            poc+ivb->iovec, &ivb->ivdt, NULL);
      }

   /* Print total overflows */
   convrt("(P1,1X,UW10,' Total overflows')", &toto, NULL);
   /* Clear any remnant subtitles */
   cryout(RK_P2, "    ", RK_NTSUBTTL+RK_LN1+4, NULL);
   /* Flush last print buffer so on-line user can see stats */
   cryout(RK_P1, "\0", RK_LN0+RK_FLUSH+1, NULL);
   spout(SPTM_NONE);          /* Turn off "SPOUT ALL" */
#endif
   } /* End d3oflw() */

