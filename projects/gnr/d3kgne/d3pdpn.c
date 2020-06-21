/* (c) Copyright 1992-2012 Neurosciences Research Foundation, Inc. */
/* $Id: d3pdpn.c 52 2012-06-01 19:54:05Z  $ */
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
*  Rev, 05/29/12, GNR - Add AffD64 data                                *
***********************************************************************/

#define LIJTYPE struct LIJARG

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "d3global.h"
#include "celldata.h"
#include "nccolors.h"
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
      long *pl;
      for (pl=&CDAT.cdcell; pl<=&CDAT.new_si; pl++,pd+=FMLSIZE)
         lemfmi4(pd,*pl);
      memcpy(pd, (char *)&CDAT.old_phi,
         (unsigned)&CDAT.flags - (unsigned)&CDAT.old_phi + 1);
      nnput(&dpio, nxdata, lnndat);
      } /* End local scope */

/*---------------------------------------------------------------------*
*  Send afference and decay data for this cell.  In the new scheme,    *
*  all variables are the same type, so there is no need to loop over   *
*  the individual connection type blocks.  Inhibitory band sums are    *
*  sent after all the other data but printed in their natural order.   *
*---------------------------------------------------------------------*/

   if (!(RP->CP.runflags & RP_OPTOTD)) {
      si64 *pa64s,*pe64s = CDAT.pAffD64 + il->nAff64;
      long *paffs,*peffs = CDAT.pAffData + il->nAff32;
      for (pa64s=CDAT.pAffD64;  pa64s<pe64s; pa64s++)
         nnputi8(&dpio, *pa64s);
      for (paffs=CDAT.pAffData; paffs<peffs; paffs++)
         nnputi4(&dpio, *paffs);
      for (ix=il->pct1; ix; ix=ix->pct) if (ix->Cn.kam & KAMCG)
         nnputi2(&dpio, ix->cnwk.go.vmvt);
      for (ib=il->pib1; ib; ib=ib->pib) {
         long jx;
         for (jx=0; jx<ib->nib; jx++)
            nnputi4(&dpio, ib->inhbnd[jx].bandsum);
         }
      }

/*---------------------------------------------------------------------*
*  Send totals by connection class                                     *
*---------------------------------------------------------------------*/

   {  long *ptot = CDAT.pTotSums;   /* Ptr to item in totals array */
      long *ptote =                 /* Ptr to end of totals array */
         (long *)((char *)ptot + CDAT.lSpecSums + CDAT.lGeomSums +
            CDAT.lModulSums + CDAT.lCondSums + CDAT.lTotSums);
      for ( ; ptot<ptote; ptot++)
         nnputi4(&dpio, *ptot);
      } /* End local scope and sending totals */

/*---------------------------------------------------------------------*
*  Send any connection-level data that are to be printed.  The         *
*  protocol is to omit all endmarks if there are no items to be        *
*  printed.  s(j) is sent as -1 if dashes are to be printed.           *
*---------------------------------------------------------------------*/

   taitms = il->dpitem;
   if ((taitms & (DPALL|DPDIDL)) == DPDIDL) taitms &= ~(DPLIJ|DPDIJ);
   if (il->pct1 && taitms &
         (DPLIJ|DPDIJ|DPCIJ|DPCIJ0|DPMIJ|DPRIJ|DPPPF|DPSJ|DPDCIJ)) {

      short *pdelc = CDAT.pdelcij;  /* Ptr to delta(Cij) */
      int  llc;                     /* Local copy of lc */
      int  sendrgb;                 /* TRUE to send raw rgb to host */
      ui16 doLij,doDij,doCij,doCij0,   /* TRUE if printing the */
         doMij,doRij,doPPF,doPPFT,     /* named variable */
         doSj,doDCij;

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
         doDCij = taitms & DPDCIJ && ix->finbnow != NO_FIN_AMP;
         doPPF  = taitms & DPPPF && cnexists(ix,PPF);
         doPPFT = taitms & DPPPF && cnexists(ix,PPFT);
         if (doSj) {
            enum ColorMode emode = (enum ColorMode)ix->cnxc.tvmode;
            d3sjx(ix);
            sendrgb = emode > Col_GS && emode < Col_R4;
            }

/* Loop over synapses */

         nnputi4(&dpio, Lija.nuk);

         for (Lija.isyn=0; Lija.isyn<Lija.nuk;
               ++Lija.isyn,Lija.psyn+=llc) {

            /* Get and send connection number and Lij */
            Lija.lijbr(ix);
            nnputi4(&dpio, Lija.jsyn);
            if (doLij) nnputi4(&dpio, Lija.lijval);

            /* Get and send Dij (delay) */
            if (doDij) {
               ix->dijbr(ix);
               nnputi4(&dpio, Lija.dijval); }

            /* Get and send Cij
            *  (now available even if MATRIX FREEZE is on) */
            if (doCij) {
               ix->cijbr(ix);
               nnputi4(&dpio, Lija.cijval); }

            /* Get and send baseline Cij */
            if (doCij0) {
               long cij0;
               d3gthn(cij0, Lija.psyn+ix->cnoff[CIJ0], ix->cijlen);
               if (cij0 == MINUS0) cij0 = 0;
               cij0 &= ix->cmsk;
               nnputi4(&dpio, cij0); }

            /* Get and send Mij */
            if (doMij) {
               long mij;
               d3gts2(mij, Lija.psyn+ix->cnoff[MIJ]);
               nnputi4(&dpio, mij); }

            /* Get and send Rij */
            if (doRij) {
               long rij;
               d3gts2(rij, Lija.psyn+ix->cnoff[RBAR]);
               nnputi4(&dpio, rij); }

            /* Get and send delta(Cij) */
            if (doDCij) {
               long dcij = *pdelc++;
               nnputi4(&dpio, dcij); }

            /* Get and send PPF */
            if (doPPF) {
               long ppfij;
               d3gts2(ppfij, Lija.psyn+ix->cnoff[PPF]);
               nnputi4(&dpio, ppfij); }

            /* Get and send PPFT */
            if (doPPFT) {
               long ppftij;
               d3gts2(ppftij, Lija.psyn+ix->cnoff[PPFT]);
               nnputi4(&dpio, ppftij); }

            /* Get and send Sj (V8D and after:  Even if Lij must
            *  be generated just for this purpose).  Send phase
            *  data only in PHASC,PHASJ,PHASR cases.  Send raw
            *  color value if input was a colored IA or image.  */
            if (doSj) {
               if (ix->sjbr(ix)) {
                  nnputi4(&dpio, Lija.sjval);
                  if (ix->phopt & (PHASC|PHASR|PHASJ)) {
                     long phi = (long)ix->pjbr(ix);
                     nnputi4(&dpio, phi); }
                  else if (sendrgb)
                     nnputi4(&dpio, Lija.sjraw);
                  }
               else {
                  nnputi4(&dpio, -1L);
                  }
               } /* End Sj data */
            } /* End loop over connections */

         nnputi4(&dpio, END_OF_GROUP);

         } /* End loop over connection types */

      } /* End sending connection-level data */

/* Close stream to host */

   nnpcl(&dpio);
   } /* End d3pdpn() */

