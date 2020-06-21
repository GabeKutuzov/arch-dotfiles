/* (c) Copyright 1989-2017, The Rockefeller University *21115* */
/* $Id: d3pdpn.c 76 2017-12-12 21:17:50Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                               d3pdpn                                 *
*                                                                      *
*     Perform detail print/plot for one cell (comp node version)       *
*   The cell type and cell number are contained in the CDAT global     *
*                                                                      *
************************************************************************
*                                                                      *
*  Usage: void d3pdpn(void)                                            *
*                                                                      *
*  For parallel version, nodes responsible for selected detail print   *
*  cells use d3pdpn() to send relevant data to host, which then uses   *
*  d3pdpz() to format the data as in the serial version. d3pdpn() is   *
*  linked only on PARn nodes, therefore no conditional is required.    *
*                                                                      *
************************************************************************
*  Written by George N. Reeke                                          *
*  V4A, 02/17/89  Translated to 'C' from version V3A                   *
*  V4B, 10/17/89, JWT - Modified for parallel version                  *
*  V4C, 12/14/89, JWT - Split off from zero node code                  *
*  Rev, 03/01/90, GNR - Correct gconn and modby bugs, restructure      *
*                       buffer testing, send conntypes one-by-one      *
*  Rev, 02/14/91,  MC - Print out phase distribution arrays            *
*  Rev, 04/30/91,  MC - Print out phase from connection data           *
*  Rev, 10/23/91, GNR - Include s(i) always, reformat, delete plotting *
*                       and 56 column limit, fix end-of-list detection *
*  Rev, 03/18/92, GNR - Use nnput                                      *
*  Rev, 08/01/92, GNR - Add support for baseline Cij                   *
*  Rev, 08/04/92, GNR - Remove access to Lij when not defined          *
*  Rev, 12/09/92, ABP - Add HYB code                                   *
*  V6D, 02/08/94, GNR - Add Dij and DPALL options, V7B add DPDIDL      *
*  V7C, 12/21/94, GNR - Print raw a(k) if != a(k)                      *
*  V8A, 05/11/96, GNR - New scheme for storing sums and phase dists    *
*  Rev, 11/26/96, GNR - Remove support for non-hybrid version          *
*  Rev, 05/07/97, GNR - Handle connection skips with optimize storage  *
*  Rev, 12/27/97, GNR - Add ADECAY, PPF, PPFT                          *
*  Rev, 05/30/99, GNR - Use new swapping scheme                        *
*  V8B, 01/06/01, GNR - Implement detailed print via ilst package      *
*  V8C, 02/27/03, GNR - Cell responses in millivolts, add conductances *
*  V8D, 05/28/06, GNR - New scheme for accessing individual type data  *
*  Rev, 07/01/06, GNR - Move Lij, Cij, Sj generation to new routines   *
*  Rev, 12/19/06, GNR - Add delta(Cij) output                          *
*  Rev, 09/30/07, GNR - Add Rij                                        *
*  ==>, 11/03/07, GNR - Last mod before committing to svn repository   *
*  V8H, 02/28/11, GNR - Add vmvt print for KAM=C                       *
*  Rev, 08/21/13, GNR - Revise type-dep detail print to all 32-bit     *
*  R67, 10/11/16, GNR - Add CONNDATA transmission to Node 0            *
*  R74, 06/20/17, GNR - Handle all BM_xxx image modes in Sjraw output  *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "d3global.h"
#include "celldata.h"
#include "lijarg.h"
#include "plots.h"
#include "bapkg.h"
#include "rkarith.h"
#include "swap.h"

extern struct CELLDATA CDAT;
extern struct LIJARG   Lija;

/*=====================================================================*
*                              d3pdpn()                                *
*=====================================================================*/

void d3pdpn(void) {

   struct CELLTYPE *il;    /* Ptr to current cell layer */
   struct CONNTYPE *ix;    /* Ptr to current conn block */
   struct INHIBBLK *ib;    /* Ptr to current geom block */
   struct NNSTR   dpio;    /* Struct to manage data stream */
   ui16 taitms;            /* Codes for connection items */

/* Pick up needed info from CELLDATA block */

   il = CDAT.cdlayr;

/* Open stream to host */

   nnpcr(&dpio, NC.hostid, DP_DATA_MSG);

/*---------------------------------------------------------------------*
*  Send basic data in CELLDATA block for this cell                     *
*---------------------------------------------------------------------*/

/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!  The following is not an example of good programming.  Since only    !
!  part of struct CELLDATA is transmitted across machines, the nxdr    !
!  conversion routine is bypassed and ad-hoc code is used here to      !
!  control byte swapping.  This might be handled better by using an    !
!  inner struct within CELLDATA for nxdr control. Meanwhile, whenever  !
!  struct CELLDATA is modified, this code must be changed accordingly. !
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/

   {  char nxdata[lnndat];
      char *pd = nxdata;
      ui32 *pl;
      for (pl=&CDAT.cdcell; pl<=(ui32 *)&CDAT.new_vm;
         pl++,pd+=FMJSIZE) lemfmi4(pd,*pl);
      lemfmi2(pd,CDAT.groupx); pd += HSIZE;
      lemfmi2(pd,CDAT.groupy); pd += HSIZE;
      memcpy(pd, (char *)&CDAT.old_phi,
         (ulng)&CDAT.flags - (ulng)&CDAT.old_phi + 1);
      nnput(&dpio, nxdata, lnndat);
      } /* End local scope */

/*---------------------------------------------------------------------*
*  Send afference and decay data for this cell.  32-bit data are sent  *
*  from sequential locations in CDAT.  But conntype and bandsum data   *
*  require loops over their containers.                                *
*---------------------------------------------------------------------*/

   if (!(RP->CP.runflags & RP_OPTOTD)) {
      si32 *pa32s,*pe32s = CDAT.pAffD32 + il->nAff32;
      for (pa32s=CDAT.pAffD32; pa32s<pe32s; pa32s++)
         nnputi4(&dpio, *pa32s);
      for (ix=il->pct1; ix; ix=ix->pct) if (ix->Cn.kam & KAMCG)
         nnputi4(&dpio, CDAT.pconnd[ix->ocdat].vmvt);
      for (ib=il->pib1; ib; ib=ib->pib) {
         ui32 jx;
         for (jx=0; jx<ib->nib; jx++)
            nnputi8(&dpio,
               ib->inhbnd[jx].bandsum[CDAT.groupn-il->logrp]);
         }
      }

/*---------------------------------------------------------------------*
*  Send totals by connection class                                     *
*---------------------------------------------------------------------*/

   {  si64 *ptot = CDAT.pTotSums;   /* Ptr to item in totals array */
      si64 *ptote =                 /* Ptr to end of totals array */
         (si64 *)((char *)ptot + CDAT.lSpecSums + CDAT.lGeomSums +
            CDAT.lModulSums + CDAT.lCondSums + CDAT.lTotSums);
      for ( ; ptot<ptote; ptot++)
         nnputi8(&dpio, *ptot);
      } /* End local scope and sending totals */

/*---------------------------------------------------------------------*
*  Send any connection-level data that are to be printed.  The         *
*  protocol is to omit all endmarks if there are no items to be        *
*  printed.  s(j) is sent as -1 if dashes are to be printed.           *
*---------------------------------------------------------------------*/

   taitms = il->dpitem;
   if ((taitms & (DPALL|DPDIDL)) == DPDIDL) taitms &= ~(DPLIJ|DPDIJ);
   if (il->pct1 && taitms &
      (DPLIJ|DPDIJ|DPCIJ|DPCIJ0|DPMIJ|DPRIJ|DPPPF|DPSJ|DPDCIJ|DPTERM)) {

      struct CONNDATA *pND;         /* Ptr to connection data */
      int  llc;                     /* Local copy of lc */
      int  sendrgb = 0;                /* pix sz to send rgb to host */
      ui16 doLij,doDij,doCij,doCij0,   /* TRUE if printing the */
         doMij,doRij,doPPF,doPPFT,     /* named variable */
         doSj,doDCij,doTerm;

      /* Lij (except KGNHV, when lijbr returns 0), Cij, and Sj always
      *  exist and can always be accessed and printed whether or not
      *  stored, therefore, their doXij flags can be set before the
      *  conntype loop.  */
      doLij = taitms & DPLIJ;
      doCij = taitms & DPCIJ;
      doSj  = taitms & DPSJ;

      /* Prepare for access to connection variables for this cell */
      d3kiji(il, CDAT.cdcell);

/* Loop over connection types */

      for (ix=il->pct1; ix; ix=ix->pct) {
         if (ix->kgfl & KGNBP) continue;

         llc = (int)ix->lc;

         /* Set up action switches according to the items present.
         *  (Always need Lij even if just to send value of J.)  */
         d3kijx(ix);
         d3lijx(ix);    /* Assignment intended in next line */
         if (doDij = taitms & DPDIJ && ix->Cn.kdelay) d3dijx(ix);
         if (doCij) d3cijx(ix);
         doCij0 = taitms & DPCIJ0 && cnexists(ix,CIJ0);
         doMij  = taitms & DPMIJ && cnexists(ix,MIJ);
         doRij  = taitms & DPRIJ && cnexists(ix,RBAR);
         doDCij = taitms & (DPDCIJ|DPTERM) &&
            ix->finbnow != NO_FIN_AMP;
         doPPF  = taitms & DPPPF && cnexists(ix,PPF);
         doPPFT = taitms & DPPPF && cnexists(ix,PPFT);
         if (doSj) {
            int rmode = ix->cnxc.tvmode & BM_MASK;
            d3sjx(ix);
            sendrgb = rmode >= BM_C48 ? bmpxsz(rmode) : 0;
            }
         doTerm = taitms & DPTERM && ix->finbnow != NO_FIN_AMP;

/* Loop over synapses */

         nnputil(&dpio, Lija.nuk);
         pND = CDAT.pconnd + ix->ocdat;
         for (Lija.isyn=0; Lija.isyn<Lija.nuk;
               ++Lija.isyn,Lija.psyn+=llc) {

            /* Keep everything in phase in case skipping Sj OOB */
            Lija.lijbr(ix);
            if (doDij) ix->dijbr(ix);
            if (doCij) ix->cijbr(ix);
            /* New protocol here:  Before skipping this connection if
            *  Sj was out-of-bounds and DPOOB option was selected,
            *  send either true Sj or MISSING_SJ if OOB to host node.
            *  (Note that sending CONNDATA is skipped in this case.
            *  In V8D and after there are cases where Lij must be
            *  generated solely to check Sj bounds).  Send phase data
            *  only in PHASC,PHASJ,PHASR cases.  Send raw color value
            *  if input was a colored IA or image.  But if Sj print is
            *  not being done, this is all omitted.
            *  R67, 12/01/16, GNR - Bug fix, introduce MISSING_SJ flag
            *     old flag, -1, can now be a real Sj.  */
            if (doSj)  {
               if (!(ix->sjbr(ix))) {
                  nnputi4(&dpio, MISSING_SJ);
                  if (taitms & DPOOB) continue;
                  }
               else {
                  nnputi4(&dpio, Lija.sjval);
                  if (ix->phopt & (PHASC|PHASR|PHASJ)) {
                     int phi = ix->pjbr(ix);
                     nnputi4(&dpio, phi); }
                  else if (sendrgb) {
                     byte *psj;     /* Ptr to Sj */
                     long Lij = Lija.lijval;
                     if (ix->kgfl & (KGNIA|KGNPB))
                        Lij += ix->osrc;
                     else
                        Lij *= sendrgb;
                     psj = ix->srcorg + Lij;
                     nnput(&dpio, psj, sendrgb);
                     }
                  }
               }

            /* Send CONNDATA now (if needed for any variables
            *  below).  (This call increments pND for us.)  */
            if (doDCij|doTerm) nncom(&dpio, (void **)&pND,
               ttloc(IXstr_CONNDATA), NNC_Send);

            /* Send connection number and Lij */
            nnputil(&dpio, Lija.jsyn);
            if (doLij) nnputil(&dpio, Lija.lijval);

            /* Send Dij (delay) */
            if (doDij) nnputi4(&dpio, Lija.dijval);

            /* Get and send Cij
            *  (now available even if MATRIX FREEZE is on) */
            if (doCij) nnputi4(&dpio, Lija.cijsgn);

            /* Get and send baseline Cij */
            if (doCij0) {
               si32 cij0;
               d3gthn(cij0, Lija.psyn+ix->cnoff[CIJ0], ix->cijlen);
               cij0 &= ix->cmsk;
               nnputi4(&dpio, cij0); }

            /* Get and send Mij */
            if (doMij) {
               si32 mij;
               d3gtjs2(mij, Lija.psyn+ix->cnoff[MIJ]);
               nnputi4(&dpio, mij); }

            /* Get and send Rij */
            if (doRij) {
               si32 rij;
               d3gtjs2(rij, Lija.psyn+ix->cnoff[RBAR]);
               nnputi4(&dpio, rij); }

            /* Get and send PPF */
            if (doPPF) {
               si32 ppfij;
               d3gtjs2(ppfij, Lija.psyn+ix->cnoff[PPF]);
               nnputi4(&dpio, ppfij); }

            /* Get and send PPFT */
            if (doPPFT) {
               si32 ppftij;
               d3gtjs2(ppftij, Lija.psyn+ix->cnoff[PPFT]);
               nnputi4(&dpio, ppftij); }

            } /* End loop over connections */

         nnputi4(&dpio, END_OF_GROUP);

         } /* End loop over connection types */

      } /* End sending connection-level data */

/* Close stream to host */

   nnpcl(&dpio);
   } /* End d3pdpn() */

