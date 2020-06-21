/* (c) Copyright 1991-2008, Neurosciences Research Foundation, Inc. */
/* $Id: d3ijpn.c 40 2010-11-03 16:02:24Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                               d3ijpn                                 *
*                                                                      *
*  Plot Cij, Cij0, Mij, PPF, Rj, or Sj -- Comp node program.  Sends    *
*  relevant variables to host, which does averaging if requested and   *
*  plotting. This makes for more communication than doing the partial  *
*  sums locally, but avoids need to allocate CMVSUMS arrays and extra  *
*  code on the comp nodes.  This is not too bad, because the averaging *
*  option will probably not be used very often.  (It is really here    *
*  because it used to be the ONLY option.) Another possibility was to  *
*  do all the non-averaging plotting on the comp nodes.  This would    *
*  make the code much more complicated and might even create more      *
*  communication (plot command lists) than just sending the plot       *
*  variables to the host for plotting.                                 *
*                                                                      *
*  Caution:  for speed, high byte of stored variables is accessed      *
*  without using the standard macros provided in d3memacc for this     *
*  purpose.                                                            *
*                                                                      *
*  Note:  Because this routine is built only for parallel CNS,         *
*  explicit preprocessor tests for PAR are not included.               *
************************************************************************
*  V4A, 06/05/89, JWT - Translated into C                              *
*  Rev, 01/17/90, JWT - Parallelized                                   *
*  Rev, 03/22/92, GNR - Use nnput, fix Lij length bug                  *
*  Rev, 08/20/92, GNR - Fix bug accessing Cij with nbc > 8             *
*  Rev, 12/09/92, ABP - Add HYB code                                   *
*  Rev, 11/27/96, GNR - Remove non-hybrid code                         *
*  V8A, 09/28/97, GNR - Independent dynamic allocations per conntype   *
*  V8B, 11/10/01, GNR - Cell lists, unaveraged plots, OPTZZ, labels    *
*  V8C, 07/08/03, GNR - Cell responses in millivolts, add conductances *
*  V8D, 07/29/06, GNR - Use new d3lij to fetch Lij even if not stored  *
*  Rev, 10/26/07, GNR - Add RJPLOT, SJPLOT, ESJPLOT                    *
*  ==>, 11/02/07, GNR - Last mod before committing to svn repository   *
*  Rev, 06/21/08, GNR - Add CSELPLOT (color selectivity)               *
***********************************************************************/

#define CMNTYP struct IJPLNODE
#define LIJTYPE struct LIJARG

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "d3global.h"
#include "clblk.h"
#include "ijpldef.h"

extern struct LIJARG Lija;

/*---------------------------------------------------------------------*
*                               d3ijpn                                 *
*---------------------------------------------------------------------*/

void d3ijpn(void) {

   struct NNSTR nnio;            /* Node-node I/O control */
   struct IJPLNODE *pin,*pine;   /* IJPLNODE loop control */
   long EndSignal = CM_END_DATA;

/* Loop over array of IJPLNODE blocks */

   pine = RP->pijpn + RP->nijpldefs;
   for (pin=RP->pijpn; pin<pine; ++pin) {

      struct CELLTYPE *il;       /* Ptr to target cell type */
      struct CONNTYPE *ix;       /* Ptr to target conn type */
      ilst            *pil;      /* Ptr to iteration list */
      rd_type         *pvar0;    /* Ptr to 1st plot var for locell */
      rd_type         *pvar;     /* Ptr to current plot variable */

      ilst  cmilst;              /* Fake ilst to plot all cells */
      iter  cmiter;              /* Cell list iterator */

      long  coff;                /* Offset to prd data for this cell */
      long  Lij;                 /* Lij */
      long  Mijtest;             /* Test for Mij a clock */
      long  jcell;               /* Current cell number */
      long  llc,llel;            /* Length of conn, cell data */
      long  mylo,myhi;           /* Low cells on this,next nodes */
      long  nuk;                 /* Number of connections used */
      long  tvar;                /* Temp of var */
      long  var;                 /* Variable to be plotted--typed long
                                 *  to establish length for nnputi4() */
      int   lkcmplt;             /* kcmplt jiggered for CSELPLOT */
      ui16  lopt;                /* Local copy of plot options */

/* Omit plot if disabled */

      if (pin->kcmpldis) continue;

/* Locate user's cell list or create one to select all cells.
*  Calculate number of cells on this node and drop out if none.  */

      il   = pin->pcmtl;         /* Ptr to target celltype */
      mylo = il->locell;         /* Low cell on this node */
      myhi =                     /* First cell on next node */
         il->locell + il->mcells;

      if (pin->pcmcl) {          /* Have an explicit cell list */
         pil = pin->pcmcl->pclil;
         if (ilstitct(pil, myhi) <= ilstitct(pil, mylo))
            continue;
         }
      else {                     /* Send all cells on node */
         pil = &cmilst;
         if (il->mcells <= 0)
            continue;
         memset((char *)&cmilst, 0, sizeof(cmilst));
         cmilst.nusm1 = (il->mcells > 1);
         cmilst.pidl = (ilstitem *)&cmilst.frst;
         cmilst.frst = mylo;
         cmilst.last = IL_REND | (myhi - 1);
         }

/*---------------------------------------------------------------------*
*                             Plot setup                               *
*---------------------------------------------------------------------*/

      ix   = pin->pcmtx;         /* Ptr to target conntype */
      lopt = pin->cmopt;         /* Local copy of option flags */
      llc  = ix->lc;             /* Length of connection data  */
      llel = il->lel;            /* Length of one cell element */
      lkcmplt = (int)pin->kcmplt;

/* Pick up data locator (high-order byte or first byte if Mij,
*  relative to first cell on node) for item being plotted.  */

      switch (pin->kcmplt) {

      case KCIJC:
         break;
      case KCIJC0:
         pvar0 = ix->psyn0 + ix->lxn + ix->cnoff[CIJ0]
#if D3MEMACC_BYTE_ORDER < 0      /* Access high byte only */
                                 + ix->cijlen - 1
#endif
            ;
         break;
      case KCIJM:
         pvar0 = ix->psyn0 + ix->lxn + ix->cnoff[MIJ];
         Mijtest = ix->Cn.mticks - S15;
         break;
      case KCIJPPF:
         pvar0 = ix->psyn0 + ix->lxn + ix->cnoff[PPF]
#if D3MEMACC_BYTE_ORDER < 0      /* Access high byte only */
                                 + 1
#endif
            ;
         break;
      case KCIJR:
         pvar0 = ix->psyn0 + ix->lxn + ix->cnoff[RBAR]
#if D3MEMACC_BYTE_ORDER < 0      /* Access high byte only */
                                 + 1
#endif
         ;
         break;
      case KCIJES:
         pvar0 = ix->psyn0 + ix->lxn + ix->cnoff[RBAR];
      case KCIJS:
         break;
      case KCIJSEL;
         if (lopt & CMSMAP) lkcmplt = KCIJC;
         break;

         } /* End kplt switch */

/* Initialize data stream to host */

      nnpcr(&nnio, NC.hostid, IJPL_DATA_MSG);

/* Initiate iteration over cell list and send id of
*  first cell on this node for host safety check.  */

      ilstset(&cmiter, pil, mylo);
      jcell = ilstnow(&cmiter);
      nnputi4(&nnio, jcell);

/* Loop over cells and send nuk to host */

      while ((jcell = ilstiter(&cmiter)) >= mylo && jcell < myhi) {

         d3kiji(il, jcell);      /* Set Lij retrieval for this */
         d3kijx(ix);             /*   cell and connection type */
         d3lijx(ix);
         coff = Lija.lcoff;
         nuk = Lija.nuk;
         nnputi4(&nnio, nuk);

/* We use a separate connection loop for each type of variable in
*  order to avoid a switch inside the loop.  Even with CMSMAP, we
*  call Lija.lijbr in order to detect skipped connections.  For
*  CSELPLOT w/o CMSMAP, send both jsyn and lijval.  */

         switch (lkcmplt) {

         case KCIJC:          /* Cij plot */
            d3cijx(ix);
            for (Lija.isyn=0; Lija.isyn<nuk; ++Lija.isyn) {
               if (!Lija.lijbr(ix)) {
                  nnputi4(&nnio, ix->lijlim);
                  break; }
               Lij = (CM.lopt & CMSMAP) ? Lija.jsyn : Lija.lijval;
               nnputi4(&nnio, Lij);
               ix->cijbr(ix);
               nnputi4(&nnio, SRA(Lija.cijval, CM_Cij2S8));
               Lija.psyn += llc;
               } /* End connection loop */
            break;

         case KCIJM:          /* Mij plot */
            pvar = pvar0 + coff;
            for (Lija.isyn=0; Lija.isyn<nuk; ++Lija.isyn) {
               if (!Lija.lijbr(ix)) {
                  nnputi4(&nnio, ix->lijlim);
                  break; }
               Lij = (CM.lopt & CMSMAP) ? Lija.jsyn : Lija.lijval;
               nnputi4(&nnio, Lij);
               /* If Mij is acting as a timer, use mxmij */
               d3gts2(var, pvar)
               if (var <= Mijtest) var = (long)ix->Cn.mxmij;
               nnputi4(&nnio, SRA(var, CM_Mij2S8));
               pvar += llc;
               Lija.psyn += llc;
               } /* End loop over connections */
            break;

         case KCIJS:          /* Sj plot */
            d3sjx(ix);
            for (Lija.isyn=0; Lija.isyn<nuk; ++Lija.isyn) {
               if (!Lija.lijbr(ix)) {
                  nnputi4(&nnio, ix->lijlim);
                  break; }
               Lij = (CM.lopt & CMSMAP) ? Lija.jsyn : Lija.lijval;
               nnputi4(&nnio, Lij);
               ix->sjbr(ix);
               nnputi4(&nnio, SRA(Lija.sjval, CM_Sj2S8));
               Lija.psyn += llc;
               } /* End loop over connections */
            break;

         case KCIJES:         /* Effective Sj plot */
            d3sjx(ix);
            pvar = pvar0 + coff;
            for (Lija.isyn=0; Lija.isyn<nuk; ++Lija.isyn) {
               if (!Lija.lijbr(ix)) {
                  nnputi4(&nnio, ix->lijlim);
                  break; }
               Lij = (CM.lopt & CMSMAP) ? Lija.jsyn : Lija.lijval;
               nnputi4(&nnio, Lij);
               if (ix->sjbr(ix)) {
                  d3gts2(var, pvar);
                  var = Lija.sjval - var;
                  var = SRA(var, CM_Sj2S8);
                  }
               else
                  var = 0;
               nnputi4(&nnio, var);
               pvar += llc;
               Lija.psyn += llc;
               } /* End loop over connections */
            break;

         case KCIJSEL:        /* Color sensitivity plot */
            d3cijx(ix);
            for (Lija.isyn=0; Lija.isyn<nuk; ++Lija.isyn) {
               if (!Lija.lijbr(ix)) {
                  nnputi4(&nnio, ix->lijlim);
                  break; }
               nnputi4(&nnio, Lija.jsyn);
               nnputi4(&nnio, Lija.lijval);
               ix->cijbr(ix);
               nnputi4(&nnio, SRA(Lija.cijval, CM_Cij2S8));
               Lija.psyn += llc;
               } /* End connection loop */
            break;

         default:             /* Cij0, PPF, or Rj plot */
            pvar = pvar0 + coff;
            for (Lija.isyn=0; Lija.isyn<nuk; ++Lija.isyn) {
               if (!Lija.lijbr(ix)) {
                  nnputi4(&nnio, ix->lijlim);
                  break; }
               Lij = (CM.lopt & CMSMAP) ? Lija.jsyn : Lija.lijval;
               nnputi4(&nnio, Lij);
               /* Construct proper negs, allowing for Cij0 = -0 */
               tvar = (long)*pvar;
               var = tvar & 0x7fL;
               if (tvar > 0x7fL) var -= 0x80L;
               nnputi4(&nnio, var);
               pvar += llc;
               Lija.psyn += llc;
               } /* End loop over connections */
            } /* End kcmplt switch */

         } /* End loop over cells */

/* Send end signal and close stream to host */

      nnputi4(&nnio, EndSignal);
      nnpcl(&nnio);
      } /* End pin loop */

   } /* End d3ijpn() */

