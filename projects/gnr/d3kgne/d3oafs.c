/* (c) Copyright 1995-2012, Neurosciences Research Foundation, Inc. */
/* $Id: d3oafs.c 52 2012-06-01 19:54:05Z  $ */
/***********************************************************************
*                       CNS Program - D3OAFS.C                         *
*                                                                      *
*                                npsps                                 *
*      Return number of signs of "postsynaptic" potentials that        *
*                    can occur given an ssck code                      *
*                                                                      *
*                         nepsa, nepsg, nepsm                          *
*         Set up ssck codes (allowed signs of "postsynaptic"           *
*         potentials) that can occur for specific, geometric,          *
*                modulatory, cell types, respectively.                 *
*                                                                      *
*                               getoas                                 *
*       Routine to compute the word offset to an afferent sums         *
*   array given il and the allocation bit corresponding to that sum    *
*                                                                      *
*                               d3lafs                                 *
*         Routine to calculate total length of afference sums          *
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
*                                                                      *
*  V8A, 05/06/95, GNR - New routines                                   *
*  Rev, 12/29/97, GNR - Add d3lafs(), eliminate DEFER-ALLOC switch     *
*  V8C, 07/15/03, GNR - Raw sums are now si64, not kept in pAffData    *
*  V8D, 05/20/06, GNR - Add sums for conductances, new pAffData alloc  *
*                       keeps phased raw, persistence for stats        *
*  V8D, 04/28/07, GNR - Add SGM_NoiseMod -- in dprt, stats, not sums   *
*  ==>, 12/29/07, GNR - Last mod before committing to svn repository   *
*  Rev, 04/22/08, GNR - Add ssck (sum sign check) mechanism            *
*  V8E, 01/19/09, GNR - d3lafs sets a few flags for Izhikevich neurons *
*  Rev, 05/26/12, GNR - Set to store raw specific affsums as si64      *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "d3global.h"
#include "celldata.h"
#include "bapkg.h"

extern struct CELLDATA CDAT;

#define NMmask (~((AFF_Excit|AFF_Hyper|AFF_Square) << SGM_NoiseMod))

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
*  array given il and the allocation bit corresponding to that sum.    *
*  This is actually the length of all previous sums array entries, so  *
*  if the selector bit is zero (maybe a missing excitatory sum), the   *
*  value returned still is the offset to the first sum of its type.    *
*                                                                      *
*  nAff32 words for detailed afference and connection data come first. *
*  Noise modulation sums are not counted.                              *
*  Needs to be available on all nodes.                                 *
*---------------------------------------------------------------------*/

int getoas(struct CELLTYPE *il, ui32 bit) {

   ui32 mask = il->kaff & NMmask & (bit-1);
   long indx = bitcnt((unsigned char *)&mask, sizeof(mask));
   return (il->nAff32 + (indx << il->pdshft));

   } /* End getoas() */

/*---------------------------------------------------------------------*
*                               getlas                                 *
*                                                                      *
*  This little function computes the byte length of a set of afferent  *
*  sums arrays given il and the allocation bits corresponding to that  *
*  set.  This is safer than the old method of subtracting pointers     *
*  because it no longer depends on the order of the various blocks.    *
*  Needs to be available on all nodes.                                 *
*---------------------------------------------------------------------*/

#pragma inline (getlas)
long getlas(struct CELLTYPE *il, ui32 bits) {

   ui32 mask = il->kaff & NMmask & bits;
   long indx = bitcnt((unsigned char *)&mask, sizeof(mask));
   return (indx << il->pdshft) * sizeof(long);
   } /* End getlas() */

/*---------------------------------------------------------------------*
*                               d3lafs                                 *
*                                                                      *
*  Results:  il->kaff in all CELLTYPEs is set to indicate the types    *
*  of connections existing in that cell type, il->nAff64 is set to     *
*  the number of si64s to be allocated for raw afference, il->nAff32   *
*  to the number of words to be allocated for finished afference and   *
*  persistence terms and their sums, and RP->lAffD64 and RP->lAffData  *
*  are set bzw to the largest values of il->nAff64 and il->nAff32      *
*  across the entire model.  This storage must be summed over connec-  *
*  tion types so it can be preserved in d3go to the end of each cell   *
*  evaluation for detail print.                                        *
*                                                                      *
*  Please be clear that this storage is not the same as the si64       *
*  storage in rsumaij, which holds, for each connection type, the      *
*  Aij sums and rbar accumulators, plus space for the d3go rax[pn]     *
*  and sax[pn] arrays, which are reused for each conntype, i.e.        *
*  are high-water, not summed allocations.                             *
*                                                                      *
*  This routine is intended to be executed on the host after d3grp3()  *
*  but before the next tree broadcast.  This will allow treatment of   *
*  afferent connections as excitatory, hyperpolarizing, squared, or    *
*  shunting to be modified at d3grp3() time in future versions of CNS. *
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
*---------------------------------------------------------------------*/

#ifndef PARn
void d3lafs(void) {

   struct CELLTYPE *il;
   struct CONNTYPE *ix;
   struct INHIBBLK *ib;
   struct MODBY    *im;
   struct CONDUCT  *ig;

   ui32 mAffData = 0;         /* Max space over cell types */
   ui32 mAffD64  = 0;

/* This little table gives the product (Cij * Sj) sign classes
*  that can occur for each combination of signs of Cij and Sj as
*  reflected in the CNSGN and SJSGN bits of the xssck codes.  */

   static const byte pspsgns[1<<PSP_SSHFT] = {
      0, 0, 0, 0, 0, 1, 2, 3, 0, 2, 1, 3, 0, 3, 3, 3 };

/* For each cell type, look at all A, G, and M connection types and
*  conductances and flag in il->kaff which types are actually present,
*  according to whether they are excitatory, hyperpolarizing, squared,
*  or shunting.  These flags will control which sums are included in
*  total cell response and printed by detailed print.
*
*  Also calculate offsets for storing afferent (raw and cooked) and
*  decay data into CDAT.pAffData for detail print and statistics.
*  Whether allocations for excitatory connections, inhibitory connec-
*  tions, or both, are required is determined by taking into account
*  allowed signs of Cij, beta, or mscl, and of the Sj  input.  (The
*  SJSGN bits cannot change during a run and so are set in d3tchk().)
*  Only specific inputs can have phase.
*
*  When OPTOTD is in effect, data are stored anyway (but not printed)
*  because it is probably faster to do that than to implement all the
*  'if' statements that would be required to skip storing them. Also,
*  the stored data can be used for SIMDATA output and statistics as
*  needed, so the offsets calculated here are the same both ways.  */

   for (il=RP->pfct; il; il=il->pnct) {
      ui32 kkaff = (AFF_Excit<<SGM_CellResp); /* Response bits */
      ui32 kexc,kinh;         /* Extracted PSP bits */
      ui32 laff32,lsum32;     /* Affs,sums space for one celltype */
      ui32 laff64,lsum64;     /* Same for 64-bit afferences */
      byte ssck;

      laff32 = laff64 = 0;    /* nAffs accumulators */

      if (il->Dc.siscl != 0) {
         ui32 sbits;
         if (RP->compat & OLDRANGE)
            sbits = (il->Dc.siscl > 0) ? AFF_Excit : AFF_Hyper;
         else
            sbits = (AFF_Excit|AFF_Hyper);
         kkaff |= (sbits<<SGM_Self) | (sbits<<SGM_Total);
         } /* End self-connection setup */

      for (ix=il->pct1; ix; ix=ix->pct) {
         /* N.B. In the Cij case, we do not clear the CNSGN bits here,
         *  because multiple signs may have been introduced by amplif
         *  in previous trial series  */
         ssck = ix->cssck & ~(PSP_POS|PSP_NEG);
         if (ix->kgen & KGNRC && ((ix->Cn.kam & ANYAMASK) == 0 ||
               ix->Cn.delta == 0 || ix->Cn.kam & KAMNC)) {
            if (ix->ucij.r.pp !=   0) ssck |= CNSGN_POS;
            if (ix->ucij.r.pp != S16) ssck |= CNSGN_NEG;
            }
         else                       ssck |= (CNSGN_POS|CNSGN_NEG);

         /* No output if scale is zero or conntype is bypassed */
         if (ix->Cn.scl != 0 && !(ix->Cn.kam & KAMBY))
            ssck |= pspsgns[ssck >> PSP_SSHFT];
         ix->cssck = ssck;

         lsum32 = ix->lpax + (ix->Cn.ADCY.kdcy != NODECAY);
         lsum64 = ix->lpax64;
         kexc = (ui32)(ssck & PSP_POS);
         kinh = (ui32)(ssck & PSP_NEG);
         ix->oex64  = (ui16)laff64;
         ix->oexafs = (ui16)laff32;
         if (kexc) laff32 += lsum32, laff64 += lsum64;
         ix->oinafs = (ui16)laff32;
         if (kinh) laff32 += lsum32, laff64 += lsum64;
         if (ix->Cn.cnopt & NOPVD)              kkaff |=
            (NORM_SUMS<<SGM_VoltDep);
         if (ix->cnflgs & SCOND)                kkaff |=
            (SCOND_SUMS<<SGM_Conduct);
         else if (ix->Cn.cnopt & NOPVA) {
            ui32 kall = kexc | kinh;            kkaff |=
            (kall << SGM_VoltDep) | (kall << SGM_Total);
            }
         else { /* Faster w/o if (kexc)? */     kkaff |=
            (kexc << SGM_Specific) | (kexc << SGM_Total);
            if (kinh) { /* Three kinds of inhibitory inputs */
               if (ix->Cn.cnopt & NOPSH)        kkaff |=
                  (AFF_Shunt<<SGM_Specific) | (AFF_Shunt<<SGM_Total);
               else if (ix->Cn.cnopt & NOPSQ)   kkaff |=
                  (AFF_Square<<SGM_Specific) | (AFF_Square<<SGM_Total);
               else                             kkaff |=
                  (AFF_Hyper<<SGM_Specific) | (AFF_Hyper<<SGM_Total);
               }
            }
         } /* End specific conntype loop */

      for (ib=il->pib1; ib; ib=ib->pib) {
         int  iband;

         ssck = ib->gssck & ~(PSP_POS|PSP_NEG|CNSGN_POS|CNSGN_NEG);
         for (iband=0; iband<ib->nib; iband++) {
            si32 beta = ib->inhbnd[iband].beta;
            if (beta > 0) ssck |= CNSGN_NEG;
            if (beta < 0) ssck |= CNSGN_POS;
            }
         ssck |= pspsgns[ssck >> PSP_SSHFT];
         ib->gssck = ssck;

         lsum32 = ib->GDCY.kdcy ? 3 : 2;    /* Words per sign */
         ib->lafd = 0;
         ib->ogafs = (ui16)laff32;
         if (ssck & PSP_POS) {                  kkaff |=
            (AFF_Excit<<SGM_Geometric) | (AFF_Excit<<SGM_Total);
            laff32 += lsum32, ib->lafd += (ui16)lsum32; }
         if (ssck & PSP_NEG) {
            if (ib->ibopt & IBOPTS)             kkaff |=
               (AFF_Shunt<<SGM_Geometric) | (AFF_Shunt<<SGM_Total);
            else if (ib->ibopt & IBOPTQ)        kkaff |=
               (AFF_Square<<SGM_Geometric) | (AFF_Square<<SGM_Total);
            else                                kkaff |=
               (AFF_Hyper<<SGM_Geometric) | (AFF_Hyper<<SGM_Total);
            laff32 += lsum32, ib->lafd += (ui16)lsum32; }
         ib->lafd *= sizeof(long);
         } /* End geometric conntype loop */

      for (im=il->pmby1; im; im=im->pmby) {
         ssck = im->mssck & ~(PSP_POS|PSP_NEG|CNSGN_POS|CNSGN_NEG);
         if (im->mscl > 0) ssck |= CNSGN_POS;
         if (im->mscl < 0) ssck |= CNSGN_NEG;
         ssck |= pspsgns[ssck >> PSP_SSHFT];
         im->mssck = ssck;

         im->omafs = laff32;
         if (ssck & (PSP_POS|PSP_NEG) == 0) continue;
         laff32 += im->MDCY.kdcy ? 3 : 2;
         if (im->mct > 0) {
            /* Regular modulation -- goes in cell sums */
            if (im->mscl >= 0)                  kkaff |=
               (AFF_Excit<<SGM_Modulatory) | (AFF_Excit<<SGM_Total);
            else if (im->Mc.mopt & IBOPTS)      kkaff |=
               (AFF_Shunt<<SGM_Modulatory) | (AFF_Shunt<<SGM_Total);
            else if (im->Mc.mopt & IBOPTQ)      kkaff |=
               (AFF_Square<<SGM_Modulatory) | (AFF_Square<<SGM_Total);
            else                                kkaff |=
               (AFF_Hyper<<SGM_Modulatory) | (AFF_Hyper<<SGM_Total);
            }
         else {
            /* Noise modulation -- has affs, no sums */
            if (im->mscl >= 0)               kkaff |=
               (AFF_Excit<<SGM_NoiseMod);
            else if (im->Mc.mopt & IBOPTQ)   kkaff |=
               (AFF_Square<<SGM_NoiseMod);
            else                             kkaff |=
               (AFF_Hyper<<SGM_NoiseMod);
            }
         } /* End modulatory conntype loop */

      for (ig=il->pcnd1; ig; ig=ig->pncd) {
         ig->ocafs = laff32; laff32 += (ig->gDCY.kdcy != NODECAY);
         kkaff |= ((AFF_Excit+AFF_Hyper)<<SGM_Conduct) |
            ((AFF_Excit+AFF_Hyper)<<SGM_Total);
         } /* End conductance check */

      il->kaff = kkaff;       /* Save true allocation bits */
      il->nAff32 = laff32;    /* and number of afferences */
      il->nAff64 = laff64;
      kkaff &= NMmask;        /* Don't count noise mod in sums */
      laff32 += (ui32)
         (bitcnt((unsigned char *)&kkaff,sizeof(kkaff)) << il->pdshft);
      if (laff32 > mAffData) mAffData = laff32;
      if (laff64 > mAffD64)  mAffD64  = laff64;

      } /* End celltype loop */

   RP->lAffData = ALIGN_UP(mAffData*sizeof(long));
   RP->lAffD64  = mAffD64*sizeof(si64);
   } /* End d3lafs() */
#endif

/*---------------------------------------------------------------------*
*                               d3oafs                                 *
*                                                                      *
*  Initialize the il->oas fields in all CONNTYPEs, INHIBBLKs, MODBYs,  *
*  and CONDUCTs with offsets into the specific sums in the pAffData    *
*  array, taking into account whether excitatory sums, inhibitory      *
*  sums, or both are required based on d3lafs calls to neps[cgm].      *
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

/* For each cell type, scan all A, G, M connection types and
*  conductances and assign offset to sums array in oas variables.  */

   for (il=RP->pfct; il; il=il->pnct) {
      long affbit;         /* Bit specifying afferent type */

      il->ossei[EXCIT] = getoas(il, (AFF_Excit << SGM_Self));
      il->ossei[INHIB] = getoas(il, (AFF_Hyper << SGM_Self));

      for (ix=il->pct1; ix; ix=ix->pct) {
         if (ix->cnflgs & SCOND)       /* Activates a conductance */
            ix->oasei[EXCIT] = ix->oasei[INHIB] =
                        getoas(il, (SCOND_SUMS<<SGM_Conduct));
         else if (ix->Cn.cnopt & NOPVA) { /* Voltage-dependent */
            ix->oasei[EXCIT] = getoas(il, (AFF_Excit<<SGM_VoltDep));
            ix->oasei[INHIB] = getoas(il, (AFF_Hyper<<SGM_VoltDep));
            }
         else {                        /* Not voltage-dependent */
            ix->oasei[EXCIT] = getoas(il, (AFF_Excit<<SGM_Specific));
            if (ix->Cn.cnopt & NOPSH)
               affbit = (AFF_Shunt<<SGM_Specific);
            else if (ix->Cn.cnopt & NOPSQ)
               affbit = (AFF_Square<<SGM_Specific);
            else
               affbit = (AFF_Hyper<<SGM_Specific);
            ix->oasei[INHIB] = getoas(il, affbit);
            }
         } /* End specific conntype loop */

      for (ib=il->pib1; ib; ib=ib->pib) {
         ib->ogsei[EXCIT] = getoas(il, (AFF_Excit<<SGM_Geometric));
         if (ib->gssck & PSP_NEG) {
            if (ib->ibopt & IBOPTS)
               affbit = (AFF_Shunt<<SGM_Geometric);
            else if (ib->ibopt & IBOPTQ)
               affbit = (AFF_Square<<SGM_Geometric);
            else
               affbit = (AFF_Hyper<<SGM_Geometric);
            ib->ogsei[INHIB] = getoas(il, affbit);
            } /* End inhibitory */
         } /* End geometric conntype loop */

      for (im=il->pmby1; im; im=im->pmby) {
         /* Skip noise modulation blocks */
         if (im->mct > 0) {
            im->omsei[EXCIT] = getoas(il, (AFF_Excit<<SGM_Modulatory));
            if (im->Mc.mopt & IBOPTS)
               affbit = (AFF_Shunt<<SGM_Modulatory);
            else if (im->Mc.mopt & IBOPTQ)
               affbit = (AFF_Square<<SGM_Modulatory);
            else
               affbit = (AFF_Hyper<<SGM_Modulatory);
            im->omsei[INHIB] = getoas(il, affbit);
            }
         } /* End modulatory conntype loop */

      for (ig=il->pcnd1; ig; ig=ig->pncd) {
         ig->ocsei[EXCIT] = getoas(il, (AFF_Excit<<SGM_Conduct));
         ig->ocsei[INHIB] = getoas(il, (AFF_Hyper<<SGM_Conduct));
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
*---------------------------------------------------------------------*/

void d3pafs(void) {

   struct CELLTYPE *il = CDAT.cdlayr;  /* Ptr to current CELLTYPE */
   short *pcontbl;         /* Ptr to current consol. table entry */
   short *pfsttbl;         /* Ptr to first consol. table in set */
   short *pnxttbl;         /* Ptr to next consol. table in set */
   long summask,itype;

/* Calculate locations and lengths of sums to be initialized */

   CDAT.pTotSums = CDAT.pAffData +
      getoas(il, (AFF_Excit<<SGM_Total));
   CDAT.pSqSums = CDAT.pAffData +
      getoas(il, (AFF_Square<<SGM_Total));
   CDAT.pShSums = CDAT.pAffData +
      getoas(il, (AFF_Shunt<<SGM_Total));
   CDAT.pCellResp = CDAT.pAffData +
      getoas(il, (AFF_Excit<<SGM_CellResp));

   CDAT.pSpecSums = CDAT.pAffData +
      getoas(il, (AFF_Excit<<SGM_Specific));
   CDAT.pNormSums = CDAT.pAffData +
      getoas(il, (NORM_SUMS<<SGM_VoltDep));
   CDAT.pGeomSums = CDAT.pAffData +
      getoas(il, (AFF_Excit<<SGM_Geometric));
   CDAT.pModulSums = CDAT.pAffData +
      getoas(il, (AFF_Excit<<SGM_Modulatory));
   CDAT.pCondSums = CDAT.pAffData +
      getoas(il, (AFF_Excit<<SGM_Conduct));

   /* Note:  For simplicity, the SCOND_SUMS are cleared along with
   *  the other SGM_Conduct sums, even though they are stored into
   *  earlier, with the SpecSums.  They are never used, so the only
   *  reason to clear them at all is to avoid any overflows.  */
   CDAT.lTotSums   = getlas(il,
      (AFF_ALL4<<SGM_Total)|(AFF_Excit<<SGM_CellResp));
   CDAT.lSpecSums  = getlas(il, (AFF_ALL4<<SGM_Specific)|
      ((AFF_Excit|AFF_Hyper)<<SGM_Self)|(AFF_ALL4<<SGM_VoltDep));
   CDAT.lGeomSums  = getlas(il, (AFF_ALL4<<SGM_Geometric));
   CDAT.lModulSums = getlas(il, (AFF_ALL4<<SGM_Modulatory));
   CDAT.lCondSums  = getlas(il,
      (AFF_Excit|AFF_Hyper|SCOND_SUMS)<<SGM_Conduct);

/* Set up tables of offsets used for calculating total input in the
*  four AFF categories.  Excitatory and hyperpolarizing terms are
*  divided into two sets according to whether they are or are not
*  included in voltage seen by voltage-dependent connections.
*  Geometric and modulatory types are included in first set unless
*  OLDVDC compatibility option is specified.  NORMSUMS are filled
*  from S(t-1) and not calculated by Affsums if volt-dep type P.  */

#define CdtlStoreOas(bit) \
   if (summask & (bit)) *pcontbl++ = getoas(il,(bit))

   /* Consolidation tables for voltage-dependent connections */
   pfsttbl = pcontbl = CDAT.ConsTables;
   if (il->ctf & (CTFVD|CTFVP)) {
      summask = il->kaff & ((RP->compat & OLDVDC) ?
         /* Terms present under compatibility = V option */
         (((AFF_Excit | AFF_Hyper)<<SGM_Specific) |
            ((AFF_Excit | AFF_Hyper)<<SGM_Self)) :
         /* Terms present under new default conditions */
         (((AFF_Excit | AFF_Hyper)<<SGM_Specific) |
            ((AFF_Excit | AFF_Hyper)<<SGM_Self) |
            ((AFF_Excit | AFF_Hyper)<<SGM_Geometric) |
            ((AFF_Excit | AFF_Hyper)<<SGM_Modulatory)));
      for (itype=AFF_Excit; itype<=AFF_Hyper; itype+=itype) {
         pnxttbl = pcontbl;
         CdtlStoreOas(itype<<SGM_Specific);
         CdtlStoreOas(itype<<SGM_Self);
         CdtlStoreOas(itype<<SGM_Geometric);
         CdtlStoreOas(itype<<SGM_Modulatory);
         if (pcontbl != pnxttbl) *pcontbl++ = EndConsTbl;
         } /* End loop over excit,hyper connections */
      if (pcontbl != pfsttbl) --pcontbl;
      *pcontbl++ = EndAllConsTbl;
      /* Set up to sum Excit+Hyper to get net voltage */
      if (il->ctf & CTFVD) {
         if (summask & (AFF_Excit<<SGM_Specific |
                        AFF_Excit<<SGM_Self |
                        AFF_Excit<<SGM_Geometric |
                        AFF_Excit<<SGM_Modulatory))
            *pcontbl++ = getoas(il,(AFF_Excit<<SGM_Total));
         if (summask & (AFF_Hyper<<SGM_Specific |
                        AFF_Hyper<<SGM_Self |
                        AFF_Hyper<<SGM_Geometric |
                        AFF_Hyper<<SGM_Modulatory))
            *pcontbl++ = getoas(il,(AFF_Hyper<<SGM_Total));
         *pcontbl++ = EndAllConsTbl;
         }
      /* Set to pick up remaining terms in main sums below */
      summask ^= il->kaff;
      } /* End setup for volt-dep conns */
   else
      summask = il->kaff;
   summask &= ~(NORM_SUMS<<SGM_VoltDep | SCOND_SUMS<<SGM_Conduct);

   /* General consolidation tables */
   pfsttbl = pcontbl;
   for (itype=AFF_Excit; itype<=AFF_Shunt; itype+=itype) {
      pnxttbl = pcontbl;
      CdtlStoreOas(itype<<SGM_Specific);
      CdtlStoreOas(itype<<SGM_VoltDep);
      CdtlStoreOas(itype<<SGM_Self);
      CdtlStoreOas(itype<<SGM_Geometric);
      CdtlStoreOas(itype<<SGM_Modulatory);
      CdtlStoreOas(itype<<SGM_Conduct);
      if (pcontbl != pnxttbl) *pcontbl++ = EndConsTbl;
      } /* End loop over six types of connections */
   if (pcontbl != pfsttbl) --pcontbl;
   *pcontbl++ = EndAllConsTbl;
   /* Set up to sum excit,hyper to get initial cell response */
   CdtlStoreOas(AFF_Excit<<SGM_Total);
   CdtlStoreOas(AFF_Hyper<<SGM_Total);
   *pcontbl = EndAllConsTbl;

#undef CdtlStoreOas

   } /* End d3pafs() */
