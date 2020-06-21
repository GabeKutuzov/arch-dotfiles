/* (c) Copyright 1989-2017, The Rockefeller University *21115* */
/* $Id: d3ijpn.c 77 2018-03-15 21:08:14Z  $ */
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
*  V8I, 03/02/13, GNR - Begin implementing d3xxj macros                *
*  R67, 12/08/16, GNR - Improved variable scaling, smshrink->smscale   *
*  R72, 04/02/17, GNR - Updates transferred from d3ijpl                *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "d3global.h"
#include "lijarg.h"
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

      size_t coff;               /* Offset to prd data for this cell */
      long  JorL;                /* jsyn or Lij, as needed on host */
      long  nuk;                 /* Number of connections used */
      int   jcell;               /* Current cell number */
      ui32  lijslim;             /* ix->lijlim as a ui32 */
      ui32  llc;                 /* Length of connection data */
      int   lkcmplt;             /* kcmplt jiggered for CSELPLOT */
      int   mylo,myhi;           /* Low cells on this,next nodes */
      si16  Mijtest;             /* Test for Mij a clock */
      si16  var;                 /* Variable to be plotted--typed si16
                                 *  to establish length for nnputi2() */
      byte  lcmsmap;             /* Local copy of source map options */

/* Omit plot if disabled */

      if (pin->kcmsmap & KCM_DISABL) continue;

/* Locate user's cell list or create one to select all cells.
*  Calculate number of cells on this node and drop out if none.  */

      il   = pin->pcmtl;         /* Ptr to target celltype */
      mylo = il->locell;         /* Low cell on this node */
      myhi = il->hicell;         /* First cell on next node */

      if (pin->pcmcl) {          /* Have an explicit cell list */
         pil = pin->pcmcl->pclil;
         if (ilstitct(pil, (long)myhi) <= ilstitct(pil, (long)mylo))
            continue;
         }
      else {                     /* Send all cells on node */
         pil = &cmilst;
         if (il->mcells <= 0)
            continue;
         memset((char *)&cmilst, 0, sizeof(cmilst));
         cmilst.nusm1 = (long)(il->mcells > 1);
         cmilst.pidl = (ilstitem *)&cmilst.frst;
         cmilst.frst = (long)mylo;
         cmilst.last = IL_REND | (long)(myhi - 1);
         }

/*---------------------------------------------------------------------*
*                             Plot setup                               *
*---------------------------------------------------------------------*/

      ix   = pin->pcmtx;         /* Ptr to target conntype */
      llc  = ix->lc;             /* Length of connection data  */
      lcmsmap = pin->kcmsmap;    /* Local copy of option flags */
      lkcmplt = (int)pin->kcmplt;
      lijslim = (ui32)ix->lijlim;

/* Pick up data locator for item being plotted
*  (if there is no standard fetch routine for it).  */

      switch (pin->kcmplt) {

      case KCIJC0:
         pvar0 = ix->psyn0 + ix->lxn + ix->cnoff[CIJ0];
         break;
      case KCIJM:
         pvar0 = ix->psyn0 + ix->lxn + ix->cnoff[MIJ];
         Mijtest = ix->Cn.mticks - S15;
         break;
      case KCIJPPF:
         pvar0 = ix->psyn0 + ix->lxn + ix->cnoff[PPF];
         break;
      case KCIJR:
      case KCIJES:
         pvar0 = ix->psyn0 + ix->lxn + ix->cnoff[RBAR];
         break;
      case KCIJSEL:
         /* If CSEL plot is averaged, input code forces CMSMAP to
         *  prevent averaging over Lij, and that causes d3cpli to
         *  set kcmsmap option to KCM_CFJSYN and Lij is never used,
         *  so d3ijpn does not need to send it to host node.  */
         if (pin->cmopt & CMTAVG) lkcmplt = KCIJC;
         break;

         } /* End kplt switch */

/* Initialize data stream to host */

      nnpcr(&nnio, NC.hostid, IJPL_DATA_MSG);

/* Initiate iteration over cell list and send id of
*  first cell on this node for host safety check.  */

      ilstset(&cmiter, pil, (long)mylo);
      jcell = (int)ilstnow(&cmiter);
      nnputi4(&nnio, jcell);

/* Loop over cells and send nuk to host */

      while ((jcell = (int)ilstiter(&cmiter)) >= mylo && jcell < myhi) {

         /* Locate center of C,K, or Q distribution and send
         *  to host node for adjusting position.  */
         if (lcmsmap & KCM_FSTXYA) {
            d3kijf(il, ix, jcell);
            ix->lijbr1(ix);
            nnputi4(&nnio, (si32)Lija.fstx);
            nnputi4(&nnio, (si32)Lija.fsty);
            }

         d3kiji(il, jcell);      /* Set Lij retrieval for this */
         d3kijx(ix);             /*   cell and connection type */
         d3lijx(ix);
         coff = Lija.lcoff;
         nuk = Lija.nuk;
         nnputi4(&nnio, (int)nuk);

/* We use a separate connection loop for each type of variable in
*  order to avoid a switch inside the loop.  Even with KCM_CFJSYN,
*  we call Lija.lijbr in order to detect skipped connections.  For
*  KCIJSEL, the three color components are sent as separate records
*  so d3ijpz can handle the same as the other plot types.  With aver-
*  aging, only jsyn is sent because CMSMAP is forced; w/o averaging,
*  we send both lijval and jsyn for indexing color plus mapping.  */

         switch (lkcmplt) {

         case KCIJC:          /* Cij plot or CIJSEL with averaging */
            d3cijx(ix);
            for (Lija.isyn=0; Lija.isyn<nuk; ++Lija.isyn) {
               if (!Lija.lijbr(ix)) {
                  nnputi4(&nnio, lijslim);
                  break; }
               JorL = (lcmsmap & KCM_CFJSYN) ? Lija.jsyn : Lija.lijval;
               nnputi4(&nnio, (int)JorL);
               ix->cijbr(ix);
               var = (si16)SRA(Lija.cijval, CM_Cij2S14); /* To S14 */
               nnputi2(&nnio, var);
               Lija.psyn += llc;
               } /* End connection loop */
            break;

         case KCIJC0:         /* Cij0 plot */
            pvar = pvar + coff;
            for (Lija.isyn=0; Lija.isyn<nuk; ++Lija.isyn) {
               if (!Lija.lijbr(ix)) {
                  nnputi4(&nnio, lijslim);
                  break; }
               JorL = (lcmsmap & KCM_CFJSYN) ? Lija.jsyn : Lija.lijval;
               nnputi4(&nnio, (int)JorL);
               d3gtms2(var, pvar);
               /* Scale to S14, allowing for Cij0 = -0 */
               var = (var == 0x8000 ? 0 : SRA(var, 1));
               nnputi2(&nnio, var);
               pvar += llc;
               Lija.psyn += llc;
               } /* End connection loop */
            break;

         case KCIJM:          /* Mij plot */
            pvar = pvar0 + coff;
            for (Lija.isyn=0; Lija.isyn<nuk; ++Lija.isyn) {
               if (!Lija.lijbr(ix)) {
                  nnputi4(&nnio, lijslim);
                  break; }
               JorL = (lcmsmap & KCM_CFJSYN) ? Lija.jsyn : Lija.lijval;
               nnputi4(&nnio, (int)JorL);
               /* If Mij is acting as a timer, use mxmij */
               d3gtms2(var, pvar);
               if (var <= Mijtest) var = ix->Cn.mxmij;
               nnputi2(&nnio, var);
               pvar += llc;
               Lija.psyn += llc;
               } /* End loop over connections */
            break;

         case KCIJS:          /* Sj plot */
            d3sjx(ix);
            for (Lija.isyn=0; Lija.isyn<nuk; ++Lija.isyn) {
               if (!Lija.lijbr(ix)) {
                  nnputi4(&nnio, lijslim);
                  break; }
               JorL = (lcmsmap & KCM_CFJSYN) ? Lija.jsyn : Lija.lijval;
               nnputi4(&nnio, (int)JorL);
               ix->sjbr(ix);
               var = (si16)Lija.sjval;
               nnputi2(&nnio, var);
               Lija.psyn += llc;
               } /* End loop over connections */
            break;

         case KCIJES:         /* Effective Sj plot */
            d3sjx(ix);
            pvar = pvar0 + coff;
            for (Lija.isyn=0; Lija.isyn<nuk; ++Lija.isyn) {
               if (!Lija.lijbr(ix)) {
                  nnputi4(&nnio, lijslim);
                  break; }
               JorL = (lcmsmap & KCM_CFJSYN) ? Lija.jsyn : Lija.lijval;
               nnputi4(&nnio, (int)JorL);
               if (ix->sjbr(ix)) {
                  d3gtms2(var, pvar);  /* Pick up RBAR */
                  var = (si16)Lija.sjval - var;
                  }
               else
                  var = 0;
               nnputi2(&nnio, var);
               pvar += llc;
               Lija.psyn += llc;
               } /* End loop over connections */
            break;

         case KCIJSEL:        /* Color sensitivity plot (w/o CMTAVG) */
            d3cijx(ix);
            for (Lija.isyn=0; Lija.isyn<nuk; ++Lija.isyn) {
               if (!Lija.lijbr(ix)) {
                  nnputi4(&nnio, lijslim);
                  break; }
               nnputi4(&nnio, (int)Lija.lijval);
               nnputi4(&nnio, (int)Lija.jsyn);
               ix->cijbr(ix);
               var = (si16)SRA(Lija.cijval, CM_Cij2S14);
               nnputi2(&nnio, var);
               Lija.psyn += llc;
               } /* End connection loop */
            break;

         default:             /* PPF or Rj plot */
            pvar = pvar0 + coff;
            for (Lija.isyn=0; Lija.isyn<nuk; ++Lija.isyn) {
               if (!Lija.lijbr(ix)) {
                  nnputi4(&nnio, lijslim);
                  break; }
               JorL = (lcmsmap & KCM_CFJSYN) ? Lija.jsyn : Lija.lijval;
               nnputi4(&nnio, (int)JorL);
               d3gtms2(var, pvar);
               nnputi2(&nnio, var);
               pvar += llc;
               Lija.psyn += llc;
               } /* End loop over connections */
            } /* End kcmplt switch */

         } /* End loop over cells */

/* Send end signal and close stream to host */

      nnputil(&nnio, EndSignal);
      nnpcl(&nnio);
      } /* End pin loop */

   } /* End d3ijpn() */

