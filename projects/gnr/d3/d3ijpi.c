/* (c) Copyright 1992-2017, The Rockefeller University *11116* */
/* $Id: d3ijpi.c 77 2018-03-15 21:08:14Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                               d3ijpi                                 *
*        Initialize Cij, Cij0, Mij, PPF, Rj, Sj, or ESJ plots          *
*                                                                      *
************************************************************************
*  V5C, 03/22/92, GNR - Split out from d3ijpz, moved to phase 1        *
*  V6A, 07/24/93, GNR - Handle mapping from platform sensors           *
*  V8A, 09/28/97, GNR - Independent dynamic allocations per conntype   *
*  Rev, 10/31/97, GNR - Add error when range exceeds layer size        *
*  Rev, 05/20/01, GNR - Remove ARBOR options on CIJPLOT/MIJPLOT        *
*  V8B, 11/10/01, GNR - Cell lists, unaveraged plots, OPTZZ, labels    *
*  Rev, 05/04/02, GNR - Add cell list indexing                         *
*  V8C, 07/06/03, GNR - Cell responses in millivolts, add conductances *
*  V8D, 07/01/06, GNR - Move Lij, Cij, Sj generation to new routines   *
*  Rev, 10/26/07, GNR - Add RJPLOT, SJPLOT, ESJPLOT                    *
*  ==>, 12/28/07, GNR - Last mod before committing to svn repository   *
*  Rev, 06/16/08, GNR - Add CSELPLOT (color selectivity)               *
*  Rev, 06/28/10, GNR - Move xlo,yhi calc here, fix shrink scale bug   *
*  Rev, 07/04/10, GNR - Add lcmwsums routine                           *
*  R67, 09/20/16, GNR - Correct some bugs for MPI operation            *
*  R67, 12/08/16, GNR - Improved variable scaling, smshrink->smscale   *
*  R72, 03/08/17, GNR - Add CMBOX, call d3cpli, correct source offsets *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "sysdef.h"
#include "d3global.h"
#include "clblk.h"
#include "ijpldef.h"
#include "rocks.h"
#include "rkxtra.h"

#ifdef PARn
#error Routine d3ijpi.c should not be compiled for comp nodes
#endif

/*---------------------------------------------------------------------*
*                              lcmvsums                                *
*                                                                      *
*  This routine is called from d3nset to determine the amount of space *
*  that needs to be allocated in the psws area for averaging variables *
*  in various plots performed by d3ijpl, etc.  It is separate from     *
*  d3ijpi because it must be called from the early first d3nset call.  *
*  It can just skip cases where target is not found, as those will     *
*  generate a fatal error later when d3ijpi is called.                 *
*---------------------------------------------------------------------*/

long lcmvsums(void) {

   struct IJPLDEF *pc;           /* Ptr to current IJPL block */
   struct CELLTYPE *il;          /* Ptr to current cell type  block */
   struct CONNTYPE *ix;          /* Ptr to current connection block */
   long nua, mxvnum= 0;          /* Max variables in average */

   for (pc=RP0->pfijpl; pc; pc=pc->pnijpl) if (pc->cmopt & CMTAVG) {
      if (!(il = findln(&pc->cmtnm))) continue;
      /*  Fix, 12/10/16, GNR - Make ix loop stop on correct cmtarg */
      for (ix=il->pct1; ix->ict!=pc->cmtarg; ix=ix->pct) ;
      if (!ix) continue;
      /*  Fix, 06/24/08, GNR - Change il->nelt to ix->srcnelt  */
      nua = (long)((pc->cmopt & CMSMAP) ? ix->nc : ix->srcnelt);
      if (nua > mxvnum) mxvnum = nua;
      }
   return mxvnum * sizeof(struct CMVSUMS);
   } /* End lcmvsums() */


/*---------------------------------------------------------------------*
*                               d3ijpi                                 *
*                                                                      *
*  Note:  This code runs each time a new CYCLE card is read.  All      *
*  initializations should allow for option changes that now may        *
*  occur at Group III time.  Code runs even if plot is disabled        *
*  or NOPLOT is in effect--idea is to find any errors in the input     *
*  even if not plotting.                                               *
*---------------------------------------------------------------------*/

void d3ijpi(void) {

   register struct IJPLDEF *pc;     /* Ptr to current IJPL block */
   static short cmmno[] = { 1-S14, 1-S14,     0, 1-S12 } ;
   static short cmmxo[] = { S14-1,     0, S14-1, S12-1 } ;
/* Cij/Mij/PPF color value ranges--indexes into cmmno, cmmxo */
#define NEG1_TO_POS1 0
#define NEG1_TO_ZERO 1
#define ZERO_TO_POS1 2
#define PPFRANGE     3        /* Range of PPF */

   /* Order of following inits must match KIJPlot enum: */
   static int cmvars[] =   /* -1 ==> No DVREQ test required */
      {   -1,   CIJ0,   MIJ,   PPF,  RBAR,  -1,  RBAR,    -1   };
   static char *cmtypes[] =
      { "CIJ", "CIJ0", "MIJ", "PPF", "RJ", "SJ", "ESJ", "CSEL" };

#ifdef PAR
   struct IJPLNODE *pcn;
#endif
   char errmsg[34+LXRLNM];    /* Error message work space */

#ifdef PAR
/* In a parallel computer, allocate space to copy just the essential
*  IJPLDEF info to an array of IJPLNODE structs for broadcast.  */
   if (RP->nijpldefs) {
      if (RP->pijpn)
         RP->pijpn = (struct IJPLNODE *)allocprv(RP->pijpn,
            RP->nijpldefs, IXstr_IJPLNODE, "Var plot node data");
      else
         RP->pijpn = (struct IJPLNODE *)allocpcv(Dynamic,
            RP->nijpldefs, IXstr_IJPLNODE, "Var plot node data");
      }
   else if (RP->pijpn) {
      freepv(RP->pijpn, "Var plot node data");
      RP->pijpn = NULL;
      }
   pcn = RP->pijpn;
#endif

/* Loop over linked list of IJPLDEF blocks */

   for (pc=RP0->pfijpl; pc; pc=pc->pnijpl) {

      struct REPBLOCK *ir;    /* Ptr to current repertoire block */
      struct CELLTYPE *il;    /* Ptr to current cell type  block */
      struct CONNTYPE *ix;    /* Ptr to current connection block */
      int  kcr;               /* Kind of Cij range */
      enum KIJPlot kplt = pc->kcmplt;  /* Kind of plot */
      int  lopt = pc->cmopt;           /* Plot options */

      /* Prepare identification string for possible errmsgs */
      sconvrt(errmsg, "(5H FOR J1A5,8HPLOT OF J0A" qLXN
         ",11H, CONNTYPE J0IH4)", cmtypes[kplt],
         fmtrlnm(&pc->cmtnm), &pc->cmtarg, NULL);

      /* Locate cell type whether or not there's a cell list.
      *  (Do this before d3clck call so it can use pc->pcmtl.)  */
      pc->pcmtl = il = findln(&pc->cmtnm);
      if (!il) {
         cryout(RK_E1, "0***CELL TYPE NOT FOUND", RK_LN2,
            errmsg, RK_CCL, NULL);
         continue;
         }
      ir = il->pback;

      /* Locate connection block for the target connections */
      if (pc->cmtarg > il->nct) {
         cryout(RK_E1,"0***JCIJ > NCT",RK_LN2,errmsg,RK_CCL,NULL);
         continue;
         }
      for (ix=il->pct1; ix->ict!=pc->cmtarg; ix=ix->pct) ;
      pc->pcmtx = ix;

      /* Match up cell list with cell type and check range limits.
      *  (If list is absent, as indicated by pcmcl being NULL, all
      *  cells will be plotted.  */
      pc->pcmcl = NULL;
      if (pc->cmclloc.clnum) {
         struct CLHDR *pclh = findcl(&pc->cmclloc, FALSE);
         if (pclh) {
            if (pc->cmclloc.clndx == 0 && pclh->nclb > 1) {
               cryout(RK_E1, "0***CELL LIST MUST NOT REFER TO MORE "
                  "THAN ONE CELL TYPE", RK_LN3, errmsg, RK_CCL,
                  "    (A CLIST INDEX MIGHT RESOLVE THE MATCH.)",
                  RK_LN0, NULL);
               continue;
               }
            switch (d3clck(pclh, pc, CLCK_IJPL)) {
            case CLCK_BADNDX:
               cryout(RK_E1, "0***CLIST INDEX EXCEEDS NUMBER OF LISTS "
                  "ON THAT CLIST", RK_LN2, errmsg, RK_CCL, NULL);
               break;
            case CLCK_BADLYR:
               cryout(RK_E1, "0***SELECTED CLIST REFERS TO A DIFFERENT"
                  " CELL TYPE", RK_LN2, errmsg, RK_CCL, NULL);
               break;
               }
            }
         }

/* Error if variable to be plotted does not exist.
*  (Cij, Sj always exist, obtained by calls to cijbrm sjbr.  */

      if (cmvars[kplt] >= 0 && ix->cnoff[cmvars[kplt]] < DVREQ) {
         cryout(RK_E1, "0***NO ", RK_LN2, cmtypes[kplt], RK_CCL,
            errmsg, RK_CCL, NULL);
         continue;
         }

/* Setups that depend on type of plot:
*     Establish defaults for cmmin,cmmax.
*/
      pc->nc1or3 = ix->nc;
      switch (kplt) {
      case KCIJSEL:
         if (!(ix->cnxc.tvcsel & EXTR3C)) {
            cryout(RK_E1, "0***CSELPLOT REQUIRES COLOR=(", RK_LN2,
               fmtcsel(ix->cnxc.tvcsel), RK_CCL,
               ",3)", 3, errmsg, RK_CCL, NULL);
            continue;
            }
         pc->nc1or3 = (ix->nc + NColorDims - 1)/NColorDims;
         /* Drop into case KCIJC -- no break */
      case KCIJC:
      case KCIJC0:
         if (ix->kgen & KGNKN)
            kcr = ix->knopt & KNO_INHIB ? NEG1_TO_ZERO : ZERO_TO_POS1;
         else
            kcr = !(ix->kgen & KGNRC)  ? NEG1_TO_POS1 :
                  !ix->ucij.r.pp       ? NEG1_TO_ZERO :
                  ix->ucij.r.pp>=65535 ? ZERO_TO_POS1 : NEG1_TO_POS1;
         break;
      case KCIJS:
      case KCIJES:
      case KCIJM:
      case KCIJR:
         kcr = NEG1_TO_POS1;
         break;
      case KCIJPPF:
         kcr = PPFRANGE;
         break;
         }
      if (pc->cmmin == CMLIMDFLT) pc->cmmin = cmmno[kcr];
      if (pc->cmmax == CMLIMDFLT) pc->cmmax = cmmxo[kcr];

/* Set color selection switch.  For the default case of using colors
*  proportional to the quantity plotted, set up the color components
*  here.  */

      if (kplt == KCIJSEL)
         pc->doseq = ix->cnxc.tvcsel & EXMASK;
      else if (lopt & CMCIRC)
         pc->doseq = DCS_CSEQ;
      else {
         if (deccol(&pc->exbgr, pc->cmeico[EXCIT]))
            cryout(RK_E1, "0***INVALID EXCITATORY COLOR", RK_LN2,
               errmsg, RK_CCL, NULL);
         if (deccol(&pc->inbgr, pc->cmeico[INHIB]))
            cryout(RK_E1, "0***INVALID INHIBITORY COLOR", RK_LN2,
               errmsg, RK_CCL, NULL);
         pc->doseq = DCS_SCALE;
         }

/* Set up parameters for mapping target cells onto the plot.
*  d3cpli now handles the three cases:  Average plots (one source
*  map drawn), Target map (ignore target cell locations), Default
*  (like a layer plot, d3cpli error if target not plotted).  */

      d3cpli(&pc->Tloc, il, pc, pc->cmpltx, pc->cmplty,
         pc->cmpltw, pc->cmplth, KP_CIJPT);

/* Set up parameters for mapping source cells onto the plot.  d3cpli
*  now handles the three cases:  Source map (ignores source cell
*  locations), Box map (draws the source figure for kgen = B,C,K,Q),
*  Default (positions source cell as in their own layer plot, d3cpli
*  error if source not plotted).  We give here the origin and size of
*  the area for the first cell.  When plotting happens in d3ijpl/
*  d3ijpz, the origin in Sloc.xlo,Sloc.yhi has to be offset according
*  to where that cell is placed.  */

      if (d3cpli(&pc->Sloc, il, pc, pc->cmpltx, pc->cmtopy +
            pc->Tloc.cdy, pc->Tloc.cdx, -pc->Tloc.cdy, KP_CIJPS))
         cryout(RK_E1, "0***BOX PLOT (OPT=B) REQUIRES KGEN=B,C,K, "
            "or Q", RK_LN2, errmsg, RK_CCL, NULL);

/* If option CMONDL is requested, and source is a repertoire, generate
*  table of needle length times sines and cosines of orientations.  */

      if (lopt & CMONDL) {
         if (ix->cnsrctyp == REPSRC) {
            float dang, vard, szmul, szxndle, szyndle;
            ui32  iang, nang = ix->srcnel;
            /* Realloc will leave it alone if already there */
            pc->pfnsc = (float *)reallocv(pc->pfnsc, 2*nang*ESIZE,
               "Needle orientations");
            vard = pc->cmmax - pc->cmmin;
            if (vard <= 0) vard = 1;      /* So no divide by 0 */
            szmul = pc->smscale *
               (lopt & CMVARY ? 0.5/(float)vard : 0.375);
            szxndle = szmul*pc->Sloc.gdx;
            szyndle = szmul*pc->Sloc.gdy;
            dang = PI/(float)nang;
            for (iang=0; iang<nang; ++iang) {
               float ang = dang*(float)iang;
               pc->pfnsc[  iang   ] = szxndle * cosf(ang);
               pc->pfnsc[iang+nang] = szyndle * sinf(ang);
               }
            /* Adjust parameters for locating symbol--inputs in
            *  same group are superimposed.  */
            pc->Sloc.cdx = pc->Sloc.cdy = 0;
            }
         else
            cryout(RK_E1, "0***NEEDLE PLOT (OPT=N) REQUIRES REAL "
               "SOURCE REGION", RK_LN2, errmsg, RK_CCL, NULL);
         } /* End CMONDL checking */

/* If user did not provide a label, make one and store it in the
*  text cache.  Store the length of the label for plotting.  */

      if (!(lopt & CMNTTL)) {
         if (!pc->cmltxt) {
            char cmlabl[20];
            sconvrt(cmlabl,"(2A" qLSN ",A4,IH4)",
               pc->cmtnm.rnm, cmtypes[kplt], &pc->cmtarg, NULL);
            pc->cmltxt = savetxt(cmlabl);
            }
         pc->cmltln = strlen(getrktxt(pc->cmltxt));
         }

#ifdef PAR
/* In a parallel computer, copy just the items needed
*  on the comp nodes to the pijpn array.  We include
*  disabled blocks, because they may come back later
*  and keeping them minimizes memory fragmentation.  */
      pcn->pcmtl    = pc->pcmtl;
      pcn->pcmtx    = pc->pcmtx;
      pcn->pcmcl    = pc->pcmcl;
      pcn->cmopt    = pc->cmopt;
      pcn->exbgr    = pc->exbgr;
      pcn->inbgr    = pc->inbgr;
      pcn->kcmplt   = pc->kcmplt;
      pcn->kcmsmap  = pc->kcmsmap;
      ++pcn;
#endif

      } /* End loop over IJPLDEF list */

   } /* End d3ijpi() */

