/* (c) Copyright 1995-2017, The Rockefeller University *11115* */
/* $Id: d3oafs.c 77 2018-03-15 21:08:14Z  $ */
/***********************************************************************
*                       CNS Program - D3OAFS.C                         *
*            npsps, getoas, getlas, d3lafs, d3oafs, d3pafs             *
*                                                                      *
*                                npsps                                 *
*      Return number of signs of "postsynaptic" potentials that        *
*                    can occur given an ssck code                      *
*                                                                      *
*                               getoas                                 *
*    Routine to compute the word offset to an afferent sums array      *
*  given il and offset and allocation bit corresponding to that sum    *
*                                                                      *
*                               getlas                                 *
*   Calculate total length of afference sums for an allocation type    *
*                                                                      *
*                               d3lafs                                 *
*   Routine to analyze input types and combine with user overrides,    *
*      check for volt-dep or shunting conns if no regular inputs       *
*                                                                      *
*                               d3oafs                                 *
*   Routine to initialize offsets into afference sums for all kinds    *
*           of input connection types before tree broadcast            *
*                                                                      *
*                               d3pafs                                 *
*     Routine to initialize pointers to afferent sums, lengths of      *
*            accumulation arrays, and consolidation tables             *
*          (called before processing each cell type in d3go)           *
*                                                                      *
*  Note:  In a little speed test on both SPARC and PCLINUX, the        *
*  relative speed of different ways of indexing positive and           *
*  negative sums was as follows (see ~/src/pgms/test/indxtime.c):      *
*  (1) if (x >= 0) sum[0] += x <semicolon> else sum[1] += x            *
*  (2) sum[(ui32)x >> 31] += x                                         *
*  (3) sum[x < 0] += x                                                 *
*  Unoptimized:      (1) faster than (3) faster than (2)               *
*  SPARC with -O2:   (3) faster than (1) faster than (2)               *
*  PCLINUX with -O2: (3) faster than (2) faster than (1)               *
*  So that is why all the sum offsets are stored as arrays of ui16     *
************************************************************************
*  V8A, 05/06/95, GNR - New routines                                   *
*  Rev, 12/29/97, GNR - Add d3lafs(), eliminate DEFER-ALLOC switch     *
*  V8C, 07/15/03, GNR - Raw sums are now si64, not kept in pAffData    *
*  V8D, 05/20/06, GNR - Add sums for conductances, new pAffData alloc  *
*                       keeps phased raw, persistence for stats        *
*  V8D, 04/28/07, GNR - Add SGM_NoiseMod -- in dprt, stats, not sums   *
*  ==>, 12/29/07, GNR - Last mod before committing to svn repository   *
*  Rev, 04/22/08, GNR - Add ssck (sum sign check) mechanism            *
*  V8E, 01/19/09, GNR - d3lafs sets a few flags for Izhikevich neurons *
*  Rev, 06/17/12, GNR - Move kaffck checks here from d3tchk so can     *
*                       change more cnopt options at Group III time    *
*  Rev, 05/21/13, GNR - Add ASHypTable,ASDepTable for autoscale OPT=H  *
*  Rev, 07/20/13, GNR - New kaff scheme encodes self/nonself conntypes *
*  Rev, 08/21/13, GNR - Revise type-dep detail print to all 32-bit     *
*  Rev, 09/03/14, GNR - Add space in AffD32 for ALPHA and DBLE h(t)    *
*  R63, 11/04/15, GNR - Move volt-dep options to vdopt, add 'A' code   *
*  R66, 01/30/16, GNR - Inhib betas are now > 0 excit, < 0 inhib       *
*  Rev, 04/12/17, GNR - Modulatory conns excit,inhib according to data *
*  R76, 11/06/17, GNR - Remove vdopt=C, pctt, NORM_SUMS w/cons tables  *
*  R77, 01/30/18, GNR - vdmm can now affect sign of afference terms    *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "d3global.h"
#include "celldata.h"
#include "rocks.h"
#include "rkarith.h"
#include "bapkg.h"

extern struct CELLDATA CDAT;

/*---------------------------------------------------------------------*
*                                npsps                                 *
*                                                                      *
*  Given an ssck code from an A, G, or M connection type, return the   *
*  number of signs that can occur in postsynaptic potentials from      *
*  inputs to this connection type.  This is currently used solely by   *
*  d3allo() and d3nset() to determine the number of decay variables    *
*  to allocate and by d3news to control NOPCX.                         *
*---------------------------------------------------------------------*/

long npsps(int ssck) {

   static byte nexinb[4] = { 0, 1, 1, 2 };
   return (long)nexinb[ssck & (PSP_POS|PSP_NEG)];

   } /* End npsps() */

/*---------------------------------------------------------------------*
*                               getoas                                 *
*                                                                      *
*  This little function computes the word offset to an afferent sums   *
*  array given il and the kaff offset and mask bit corresponding to    *
*  that sum.  This is actually the length of all previous sums array   *
*  entries, so if the selector bit is zero (maybe a missing excitatory *
*  sum), the value returned still is the offset to the first sum of    *
*  its type.                                                           *
*                                                                      *
*  Note:  Alternatively, d3lafs() could add to each CELLTYPE an array  *
*  of bytes with the number of bits up to and in each kaff byte. Then  *
*  getoas() would only need to add that number plus the count in the   *
*  one byte actually referred to.  This would eliminate khold.         *
*                                                                      *
*  nAff32 words for detailed afference and connection data come first. *
*  Uncounted items (e.g. NMOD) are now in last kaff byte.              *
*  Note:  bitcnt() returns 0 if byte count arg is 0.                   *
*  Needs to be available on all nodes.                                 *
*---------------------------------------------------------------------*/

int getoas(struct CELLTYPE *il, int koff, byte kbit) {

   int woff;
   if (kbit == 1)
      woff = (int)bitcnt(il->kaff, koff);
   else {
      byte khold = il->kaff[koff];
      il->kaff[koff] &= (kbit - 1);
      woff = (int)bitcnt(il->kaff, koff+1);
      il->kaff[koff] = khold;
      }
   return (il->nAff64 + (woff << il->pdshft));

   } /* End getoas() */

/*---------------------------------------------------------------------*
*                               getlas                                 *
*                                                                      *
*  This little function computes the byte length of a set of afferent  *
*  sums arrays given il and the allocation byte corresponding to that  *
*  set.  This is safer than the old method of subtracting pointers     *
*  because it no longer depends on the order of the various blocks.    *
*  Needs to be available on all nodes.                                 *
*---------------------------------------------------------------------*/

#pragma inline (getlas)
ui32 getlas(struct CELLTYPE *il, int koff) {

   ui32 indx = (ui32)bitcnt(il->kaff+koff, 1);
   return (indx << il->pdshft) * sizeof(si64);
   } /* End getlas() */

/*---------------------------------------------------------------------*
*                               d3lafs                                 *
*                                                                      *
*  Results:  il->kaff in all CELLTYPEs is set to indicate the types    *
*  of connections existing in that cell type, il->nAff64 is set to     *
*  the number of si64s to be allocated for raw afference and sums by   *
*  afference type, il->nAff32 to the number of words to be allocated   *
*  for finished afference and persistence terms, and RP->lAffD64 and   *
*  RP->lAffD32 are set bzw to the largest values of il->nAff64 and     *
*  il->nAff32 across the entire model.  This storage must be summed    *
*  over connection types so it can be preserved in d3go to the end of  *
*  each cell evaluation for detail print.                              *
*                                                                      *
*  Please be clear that this storage is not the same as the si64       *
*  storage in praffs, which holds, for each connection type, the       *
*  Aij sums and rbar accumulators, plus space for the d3go rax[pn]     *
*  and sax[pn] arrays.  The latter are reused for each conntype, i.e.  *
*  are high-water, not summed allocations.                             *
*                                                                      *
*  This routine is intended to be executed on the host after d3grp3()  *
*  but before the next tree broadcast.  This now allows treatment of   *
*  afferent connections as excitatory, hyperpolarizing, squared, or    *
*  shunting to be modified at d3grp3() time.                           *
*                                                                      *
*  Thought was given to providing a space-saving mode in which sums    *
*  for the four big connection classes would be consolidated during    *
*  calculation, sacrificing individual detail print, but this is no    *
*  good because the four classes have different lifetimes (SpecSums    *
*  and CondSums: cell, GeomSums: group, ModulSums: celltype) and the   *
*  advantage of reusing ModulSums & GeomSums for multiple cells would  *
*  be lost.                                                            *
*                                                                      *
*  Rev, 04/22/08, GNR - We no longer allocate for bypassed connection  *
*  types or connection types with zero scales--allocations are recom-  *
*  puted at d3news time and thus get made if computation is reinstated *
*  later in the run.  This implies that d3go must refrain from storing *
*  in these areas when the allocation has been skipped.                *
*                                                                      *
*  Rev, 07/26/13, GNR - Now takes into account any input thresholds    *
*  and/or reversal potentials on possible signs of inputs.             *
*---------------------------------------------------------------------*/

#ifndef PARn
void d3lafs(void) {

   struct CELLTYPE *il;
   struct CONNTYPE *ix;
   struct INHIBBLK *ib;
   struct MODBY    *im;
   struct CONDUCT  *ig;

   ui32 mAffD32 = 0;          /* Max 32-bit space over cell types */
   ui32 mAffD64 = 0;          /* Max 64-bit space over cell types */

/* This little table gives the product (Cij * Sj) sign classes
*  that can occur for each combination of signs of Cij and Sj as
*  reflected in the CNSGN and SJSGN bits of the xssck codes.  */

   static const byte pspsgns[1<<PSP_SSHFT] = {
      0, 0, 0, 0, 0, 1, 2, 3, 0, 2, 1, 3, 0, 3, 3, 3 };

/* For each cell type, look at all A,G,M,S connection types and
*  conductances and flag in il->kaff which types are actually present,
*  according to whether they are excitatory, hyperpolarizing, squared,
*  or shunting, self or nonself input.  These flags control which sums
*  are included in total cell response and printed by detailed print.
*  Which allocations are required is determined by taking into account
*  allowed signs of Sj inputs, Cij, scl, beta, or mscl, whether thresh-
*  olds are above or below any reversal potential, and whether usage
*  overrides were provided.  (See Design Notes, p. 66.  The SJSGN bits
*  cannot change during a run and so are set in d3tchk().)
*
*  N.B.  These calculations assume current usage of input thresholds
*  (siet, et, etc.) and also assume thresholds on sums (mxax, mxib,
*  etc.) operate on absolute values of sums and therefore do not
*  affect allowed signs.  Code must be modified if these assumptions
*  change.
*
*  Also calculate offsets for storing afferent (raw and cooked) and
*  decay data into CDAT.pAffD32 for detail print and statistics.
*  Only specific inputs can have phase.
*
*  When OPTOTD is in effect, data are stored anyway (but not printed)
*  because it is probably faster to do that than to implement all the
*  'if' statements that would be required to skip storing them. Also,
*  the stored data can be used for SIMDATA output and statistics as
*  needed, so the offsets calculated here are the same both ways.  */

   for (il=RP->pfct; il; il=il->pnct) {
      byte *pkaff = il->kaff; /* Ptr to master list */
      ui32 laff32,lsum32;     /* Affs,sums space for one celltype */
      ui32 laff64;            /* Space for 64-bit sums */
      int  isgm;              /* Index into kaff array */
      int  kaffck = 0;        /* Afference type check bits */
#define HAS_EHQ  1            /* Cell type has type E,H,Q conns */
#define HAS_S    2            /* Cell type has type S conns */
      byte ssck;

      memset(pkaff, 0, NKAFB);
      laff32 = 0;             /* nAffs accumulators */

      /*-------- Autaptic connections --------*/
      il->apspov &= ~USG_QIR;
      if (il->Dc.siscl != 0) {   /* Code for autaptic input */
         ssck = RP0->rssck;
         if (il->Dc.siet >= 0) ssck &= ~SJSGN_NEG;
         ssck |= (il->Dc.siscl < 0) ? CNSGN_NEG : CNSGN_POS;
         pkaff[SGM_Self] =
            (pspsgns[ssck >> PSP_SSHFT] & ~il->apspov) << AFS_Shft;
         /* Impose overrides on autaptic input autoscale */
         if ((il->apspov & USG_IIR || il->kautu & KAUT_A) &&
            !(il->apspov & USG_XIR)) il->apspov |= USG_QIR;
         }

      /*-------- Specific connections --------*/
      for (ix=il->pct1; ix; ix=ix->pct) {

         /* Setups done even if conntype is skipped */
         ix->spspov &= ~USG_QIR;
         ix->sssck &= ~(PSP_POS|PSP_NEG);

         /* Conntype is not processed if bypass bit is set */
         if (ix->Cn.cnopt & NOPBY) continue;

         /* N.B. In the Cij case, we do not clear the CNSGN bits here,
         *  because multiple signs may have been introduced by amplif
         *  in previous trial series  */
         ssck = ix->sssck & (CNSGN_POS|CNSGN_NEG) | ix->sssck0;
         if (ix->Cn.sjrev > 0)        ssck |= SJSGN_NEG;
         else if (ix->Cn.sjrev < 0)   ssck |= SJSGN_POS;
         if (ix->Cn.et == SHRT_MAX && ix->Cn.etlo <= 0)
                                      ssck &= ~SJSGN_POS;
         if (ix->Cn.etlo == ~SHRT_MAX && ix->Cn.et >= 0)
                                      ssck &= ~SJSGN_NEG;
         if (ix->Cn.emxsj < 0)        ssck &= ~SJSGN_POS;
         if (ix->kgen & KGNRC && ((ix->Cn.kam & ANYAMASK) == 0 ||
               ix->Cn.mdelta == 0 || ix->Cn.kam & KAMNC)) {
            if (ix->ucij.r.pp !=   0) ssck |= CNSGN_POS;
            if (ix->ucij.r.pp != S16) ssck |= CNSGN_NEG;
            }
         /* R76, 11/24/17, bug fix:  KNO_INHIB makes Cij >= 0 */
         else if ((ix->kgen & (KGNKN|KGNE2)) == (KGNKN|KGNE2))
                                      ssck |= CNSGN_POS;
         else                         ssck |= (CNSGN_POS|CNSGN_NEG);
         if (ix->Cn.scl < 0) {
            byte bpos = ssck & CNSGN_POS, bneg = ssck & CNSGN_NEG;
            ssck = ssck & ~(CNSGN_POS|CNSGN_NEG) | bpos<<1 | bneg>>1;
            }
         if (ix->Cn.vdopt & VDANY && ix->Cn.vdmm < 0)
            ssck |= (CNSGN_POS|CNSGN_NEG);
         /* No output if scale is zero but may process anyway */
         if (ix->Cn.scl != 0) {
            ssck |= pspsgns[ssck >> PSP_SSHFT] & ~ix->spspov;
            /* Impose overrides on specific input autoscale */
            if ((ix->spspov & USG_IIR || il->kautu & KAUT_S) &&
               !(ix->spspov & USG_XIR)) ix->spspov |= USG_QIR;
            }
         ix->sssck = ssck;

         lsum32 = ix->npaftot =
            ix->npafraw + ix->npaffin + ix->Cn.ADCY.ndal;
         ix->odpafs = (ui16)laff32;
         if (ssck & PSP_POS) laff32 += lsum32;
         if (ssck & PSP_NEG) laff32 += lsum32;

         if (!(ix->cnflgs & SCOND)) {
            byte xtyp = (ssck & PSP_POS) ? AFF_Excit : 0;
            isgm = (ix->Cn.vdopt & VDANY) ? SGM_VoltDep : SGM_Specific;
            if (ssck & PSP_NEG) {
               if (ix->Cn.cnopt & NOPSH)      xtyp |= AFF_Shunt;
               else if (ix->Cn.cnopt & NOPSQ) xtyp |= AFF_Square;
               else                           xtyp |= AFF_Hyper;
               }
            pkaff[isgm] |= ix->jself ? xtyp << AFS_Shft : xtyp;
            }
         kaffck |= (ix->Cn.cnopt & NOPSH) ? HAS_S : HAS_EHQ;
         } /* End specific conntype loop */

      /*-------- Geometric connections --------*/
      for (ib=il->pib1; ib; ib=ib->pib) {
         ui32 iband;

         ib->gpspov &= ~USG_QIR;
         ssck = RP0->rssck;
         if (ib->gjrev > 0)        ssck |= SJSGN_NEG;
         else if (ib->gjrev < 0)   ssck |= SJSGN_POS;
         if (ib->itt == SHRT_MAX && ib->ittlo <= 0)
                                   ssck &= ~SJSGN_POS;
         if (ib->ittlo == ~SHRT_MAX && ib->itt >= 0)
                                   ssck &= ~SJSGN_NEG;
         for (iband=0; iband<ib->nib; iband++) {
            si32 beta = ib->inhbnd[iband].beta;
            if (beta > 0) ssck |= CNSGN_POS;
            if (beta < 0) ssck |= CNSGN_NEG;
            }
         ssck |= pspsgns[ssck >> PSP_SSHFT] & ~ib->gpspov;
         ib->gssck = ssck;
         /* Impose overrides on geometric input autoscale */
         if ((ib->gpspov & USG_IIR || il->kautu & KAUT_G) &&
            !(ib->gpspov & USG_XIR)) ib->gpspov |= USG_QIR;

         lsum32 = 2 + ib->GDCY.ndal;   /* Words per sign */
         ib->lafd = 0;
         ib->ogafs = (ui16)laff32;
         if (ssck & PSP_POS) {
            pkaff[SGM_Geometric] |= ib->jself ? AFS_Excit : AFF_Excit;
            laff32 += lsum32, ib->lafd += (ui16)lsum32; }
         if (ssck & PSP_NEG) {
            byte gtyp;
            if (ib->ibopt & IBOPTS)       gtyp = AFF_Shunt;
            else if (ib->ibopt & IBOPTQ)  gtyp = AFF_Square;
            else                          gtyp = AFF_Hyper;
            if (ib->jself) gtyp <<= AFS_Shft;
            pkaff[SGM_Geometric] |= gtyp;
            laff32 += lsum32, ib->lafd += (ui16)lsum32; }

         ib->lafd *= sizeof(si32);
         kaffck |= (ib->ibopt & IBOPTS) ? HAS_S : HAS_EHQ;
         } /* End geometric conntype loop */

      /*-------- Modulatory connections --------*/
      for (im=il->pmby1; im; im=im->pmby) {
         im->Mc.mpspov &= ~USG_QIR;
         ssck = im->mssck0;
         if (im->Mc.mjrev > 0)      ssck |= SJSGN_NEG;
         else if (im->Mc.mjrev < 0) ssck |= SJSGN_POS;
         if (im->Mc.mt == SHRT_MAX && im->Mc.mtlo <= 0)
                                    ssck &= ~SJSGN_POS;
         if (im->Mc.mtlo == ~SHRT_MAX && im->Mc.mt >= 0)
                                    ssck &= ~SJSGN_NEG;
         if (im->mscl > 0) ssck |= CNSGN_POS;
         if (im->mscl < 0) ssck |= CNSGN_NEG;
         ssck |= pspsgns[ssck >> PSP_SSHFT] & ~im->Mc.mpspov;
         im->mssck = ssck;

         im->omafs = (ui16)laff32;
         lsum32 = 2 + im->MDCY.ndal;
         if (im->mct > 0) {
            /* Regular modulation -- goes in cell sums */
            if (ssck & PSP_POS) {
               pkaff[SGM_Modulatory] |= im->Mc.jself ?
                  AFS_Excit : AFF_Excit;
               laff32 += lsum32; }
            if (ssck & PSP_NEG) {
               byte mtyp;
               if (im->Mc.mopt & IBOPTS)        mtyp = AFF_Shunt;
               else if (im->Mc.mopt & IBOPTQ)   mtyp = AFF_Square;
               else                             mtyp = AFF_Hyper;
               if (im->Mc.jself) mtyp <<= AFS_Shft;
               pkaff[SGM_Modulatory] |= mtyp;
               laff32 += lsum32;
               }
            /* Impose overrides on modulatory input autoscale */
            if ((im->Mc.mpspov & USG_IIR || il->kautu & KAUT_M) &&
               !(im->Mc.mpspov & USG_XIR)) im->Mc.mpspov |= USG_QIR;
            kaffck |= (im->Mc.mopt & IBOPTS) ? HAS_S : HAS_EHQ;
            }
         /* Note, 07/31/13:  Code here to allocate kaff bits for noise
         *  modulatory connections was removed as they are never used
         *  in AffSums, individual contributions are already available
         *  in detail print, as is the sum in CDAT.nmodval.  */
         } /* End modulatory conntype loop */

      for (ig=il->pcnd1; ig; ig=ig->pncd) {
         /* At present, conductances are assumed possibly to provide
         *  AFF_Excit or AFF_Hyper input.  As this code is developed,
         *  other options (and self-nonself choice) may be added.  */
         ig->ocafs = laff32; laff32 += ig->gDCY.ndal;
         pkaff[SGM_Conduct] |= (AFF_Excit | AFF_Hyper);
         } /* End conductance check */

      /* Set the SGM_Total bits for all the connection types that
      *  were found in this layer plus always AFF_CellResp.  */
      ssck = 0;
      for (isgm=SGM_Specific; isgm<=SGM_Conduct; ++isgm)
         ssck |= pkaff[isgm] | pkaff[isgm] >> AFS_Shft;
      pkaff[SGM_Total] = AFF_CellResp |
         (ssck & (AFF_Excit|AFF_Hyper|AFF_Square|AFF_Shunt));

      /* N.B. il-nAff32 is for sending afference data to node 0
      *  for detail print -- does not include 64-bit sums.
      *  Now check for very unlikely overflow of 16-bit offsets.  */
      if (laff32 > UI16_MAX) e64act("d3oafs", EAfl(OVF_ALLO));
      il->nAff32 = laff32;    /* Save number of afferences */
      laff64 = (ui32)(bitcnt(pkaff, NKAFB-1) << il->pdshft);
      if (laff32 > mAffD32) mAffD32 = laff32;
      if (laff64 > mAffD64) mAffD64 = laff64;

      /* Give error if ALL conntypes to this cell type are voltage-
      *  dependent, shunting, or squared inhibition.  */
         if (il->pcnd1) kaffck |= HAS_EHQ;
         if (kaffck == HAS_S)
            cryout(RK_E1, "0***", RK_LN2, fmturlnm(il), LXRLNM,
               " HAS NO EXCIT/HYPERPOL/SQ CONNECTIONS NEEDED"
               " WITH SHUNTING INHIB.", RK_CCL, NULL);

      } /* End celltype loop */

   RP->lAffD32 = ALIGN_UP(mAffD32*sizeof(si32));
   RP->lAffD64 = mAffD64*sizeof(si64);
   } /* End d3lafs() */
#endif

/*---------------------------------------------------------------------*
*                               d3oafs                                 *
*                                                                      *
*  Initialize the il->oas fields in all CONNTYPEs, INHIBBLKs, MODBYs,  *
*  and CONDUCTs with offsets into the specific sums in the pAffDnn     *
*  arrays, taking into account whether excitatory sums, inhibitory     *
*  sums, or both are required based on kaff bits set by d3lafs().      *
*                                                                      *
*  This routine is intended to be executed on the host after d3grp3()  *
*  and d3lafs() but before the next tree broadcast.                    *
*---------------------------------------------------------------------*/

#ifndef PARn
void d3oafs(void) {

   struct CELLTYPE *il;
   struct CONNTYPE *ix;
   struct INHIBBLK *ib;
   struct MODBY    *im;
   struct CONDUCT  *ig;

/* For each cell type, scan all A,G,M,S connection types and
*  conductances and assign offset to sums array in oas variables.  */

   for (il=RP->pfct; il; il=il->pnct) {
      ui32 affbit;            /* Bit specifying afferent type */
      int isgm;               /* Index int kaff array */

      il->ossei[EXCIT] = getoas(il, SGM_Self, AFS_Excit);
      il->ossei[INHIB] = getoas(il, SGM_Self, AFS_Hyper);

      for (ix=il->pct1; ix; ix=ix->pct) {
         if (!(ix->cnflgs & SCOND)) {
            isgm = (ix->Cn.vdopt & VDANY) ? SGM_VoltDep : SGM_Specific;
            affbit = ix->jself ? AFS_Excit : AFF_Excit;
            ix->oasei[EXCIT] = getoas(il, isgm, affbit);
            if (ix->Cn.cnopt & NOPSH)      affbit = AFF_Shunt;
            else if (ix->Cn.cnopt & NOPSQ) affbit = AFF_Square;
            else                           affbit = AFF_Hyper;
            if (ix->jself) affbit <<= AFS_Shft;
            ix->oasei[INHIB] = getoas(il, isgm, affbit);
            }
         } /* End specific conntype loop */

      for (ib=il->pib1; ib; ib=ib->pib) {
         affbit = ib->jself ? AFS_Excit : AFF_Excit;
         ib->ogsei[EXCIT] = getoas(il, SGM_Geometric, affbit);
         if (ib->gssck & PSP_NEG) {
            if (ib->ibopt & IBOPTS)      affbit = AFF_Shunt;
            else if (ib->ibopt & IBOPTQ) affbit = AFF_Square;
            else                         affbit = AFF_Hyper;
            if (ib->jself) affbit <<= AFS_Shft;
            ib->ogsei[INHIB] = getoas(il, SGM_Geometric, affbit);
            } /* End inhibitory */
         } /* End geometric conntype loop */

      for (im=il->pmby1; im; im=im->pmby) {
         /* Skip noise modulation blocks */
         if (im->mct > 0) {
            affbit = im->Mc.jself ? AFS_Excit : AFF_Excit;
            im->omsei[EXCIT] = getoas(il, SGM_Modulatory, affbit);
            if (im->Mc.mopt & IBOPTS)      affbit = AFF_Shunt;
            else if (im->Mc.mopt & IBOPTQ) affbit = AFF_Square;
            else                           affbit = AFF_Hyper;
            if (im->Mc.jself) affbit <<= AFS_Shft;
            im->omsei[INHIB] = getoas(il, SGM_Modulatory, affbit);
            }
         } /* End modulatory conntype loop */

      for (ig=il->pcnd1; ig; ig=ig->pncd) {
         ig->ocsei[EXCIT] = getoas(il, SGM_Conduct, AFF_Excit);
         ig->ocsei[INHIB] = getoas(il, SGM_Conduct, AFF_Hyper);
         } /* End conductance loop */

      } /* End celltype loop */

   } /* End d3oafs() */
#endif

/*---------------------------------------------------------------------*
*                               d3pafs                                 *
*                                                                      *
*  On all nodes, initialize pointers, length fields, and consolidation *
*  tables in CDAT struct used to accumulate and combine contributions  *
*  from all classes of afferent connections for particular cell type.  *
*                                                                      *
*  This is repeated for each layer in each cycle, only because to      *
*  save space in CELLTYPE structs, the consolidation tables are in     *
*  CDAT where they get overwritten for each new celltype.              *
*---------------------------------------------------------------------*/

static struct CSO_t {      /* Common data for CdtlStoreOas() */
   short *pcontbl;         /* Ptr to current consol. table entry */
   short *pasdep;          /* Ptr to autoscale depolarzn table */
   short *pashyp;          /* Ptr to autoscale hyperplzn table */
   ui16 lkaut;             /* Working copy of il->paut->kaut */
   byte pspov;             /* Working copy of xpspov for conntype */
   byte wkaff[NKAFB];      /* Working copy of kaff array */
   } CSO;

static void CdtlStoreOas(int ityp, byte kbit) {
   if (CSO.wkaff[ityp] & kbit) {
      int ioas = getoas(CDAT.cdlayr, ityp, kbit);
      *CSO.pcontbl++ = ioas;
      if (CSO.pspov & USG_HYP)   goto MakeHyper;
      if (CSO.pspov & USG_DEP)   goto MakeDepol;
      if (kbit == AFS_Hyper)     goto MakeHyper;
      if (!(CSO.lkaut & KAUT_WTA) && kbit == AFF_Hyper)
                                 goto MakeHyper;
MakeDepol:  *CSO.pasdep++ = ioas; return;
MakeHyper:  *CSO.pashyp-- = ioas; return;
      }
   } /* End CdtlStoreOas() */

void d3pafs(void) {

   struct CELLTYPE *il = CDAT.cdlayr;  /* Ptr to current CELLTYPE */
   short *pfsttbl;         /* Ptr to first consol. table in set */
   short *pnxttbl;         /* Ptr to next consol. table in set */
   byte imode,jmode;       /* Index over application modes */

/* Calculate locations and lengths of sums to be initialized */

   CDAT.pTotSums = CDAT.pAffD64 +
      getoas(il, SGM_Total, AFF_Excit);
   CDAT.pSqSums = CDAT.pAffD64 +
      getoas(il, SGM_Total, AFF_Square);
   CDAT.pShSums = CDAT.pAffD64 +
      getoas(il, SGM_Total, AFF_Shunt);
   CDAT.pCellResp = CDAT.pAffD64 +
      getoas(il, SGM_Total, AFF_CellResp);

   CDAT.pSpecSums = CDAT.pAffD64 +
      getoas(il, SGM_Specific, AFF_Excit);
   CDAT.pGeomSums = CDAT.pAffD64 +
      getoas(il, SGM_Geometric, AFF_Excit);
   CDAT.pModulSums = CDAT.pAffD64 +
      getoas(il, SGM_Modulatory, AFF_Excit);
   CDAT.pCondSums = CDAT.pAffD64 +
      getoas(il, SGM_Conduct, AFF_Excit);

   /* Calc length of sums for each connection type.  These are used
   *  to zero the sums at the appropriate times.  */
   CDAT.lTotSums   = getlas(il, SGM_Total);
   CDAT.lSpecSums  = getlas(il, SGM_Specific) +
                     getlas(il, SGM_VoltDep) + getlas(il, SGM_Self);
   CDAT.lGeomSums  = getlas(il, SGM_Geometric);
   CDAT.lModulSums = getlas(il, SGM_Modulatory);
   CDAT.lCondSums  = getlas(il, SGM_Conduct);

/* Set up tables of offsets used for calculating total input in the
*  four AFF categories for all the SGM_xxx categories.  */

   pfsttbl = CSO.pcontbl = CDAT.ConsTables;
   CSO.pasdep = CDAT.ASDepTable;
   CSO.pashyp = CDAT.ASHypTable;
   CSO.lkaut = il->pauts ? il->pauts->kaut : 0;
   memcpy(CSO.wkaff, il->kaff, NKAFB);
   pfsttbl = CSO.pcontbl;

   for (imode=AFF_Excit; imode<=AFF_Shunt; imode<<=1) {
      pnxttbl = CSO.pcontbl;
      for (jmode=imode; jmode!=0; jmode<<=AFS_Shft) {
         CdtlStoreOas(SGM_Specific,   jmode);
         CdtlStoreOas(SGM_VoltDep,    jmode);
         CdtlStoreOas(SGM_Self,       jmode);
         CdtlStoreOas(SGM_Geometric,  jmode);
         CdtlStoreOas(SGM_Modulatory, jmode);
         CdtlStoreOas(SGM_Conduct,    jmode);
         }
      if (CSO.pcontbl != pnxttbl) *CSO.pcontbl++ = EndConsTbl;
      } /* End loop over six types of connections */
   if (CSO.pcontbl != pfsttbl) --CSO.pcontbl;
   *CSO.pcontbl++ = EndAllConsTbl;
   /* Set up to sum excit,hyper to get initial cell response */
   if (CSO.wkaff[SGM_Total] & AFF_Excit)
      *CSO.pcontbl++ = getoas(il, SGM_Total, AFF_Excit);
   if (CSO.wkaff[SGM_Total] & AFF_Hyper)
      *CSO.pcontbl++ = getoas(il, SGM_Total, AFF_Hyper);
   *CSO.pcontbl = *CSO.pasdep = *CSO.pashyp = EndAllConsTbl;

   } /* End d3pafs() */
