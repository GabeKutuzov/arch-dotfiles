/* (c) Copyright 1992-2010, Neurosciences Research Foundation, Inc. */
/* $Id: d3ijpi.c 30 2010-07-09 21:26:56Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                               d3ijpi                                 *
*                                                                      *
*  Initialize Cij, Cij0, Mij, PPF plots                                *
*                                                                      *
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
***********************************************************************/

#define CMNTYP struct IJPLNODE
#define CMTYPE struct IJPLDEF

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

#define LFTHLF 0xffff0000

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
      for (ix=il->pct1; ix->ict!=pc->cmtarg; ix=ix->pct) {
      /*  Fix, 06/24/08, GNR - Change il->nelt to ix->srcnelt */
         nua = (pc->cmopt & CMSMAP) ? ix->nc : ix->srcnelt;
         if (nua > mxvnum) mxvnum = nua;
         }
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
   static short cmmxo[] = { S14-1,     0, 1-S14, S12-1 } ;
/* Cij/Mij/PPF color value ranges--indexes into cmmno, cmmxo */
#define NEG1_TO_POS1 0
#define NEG1_TO_ZERO 1
#define ZERO_TO_POS1 2
#define PPFRANGE     3        /* Range of PPF */

   /* Order of following inits must match KIJPlot enum: */
   static int cmvars[] =
      {  CIJ,   CIJ0,   MIJ,   PPF,  RBAR,  -1,  RBAR,   CIJ   };
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
   else
      freepv(RP->pijpn, "Var plot node data"), RP->pijpn = NULL;
   pcn = RP->pijpn;
#endif

/* Loop over linked list of IJPLDEF blocks */

   for (pc=RP0->pfijpl; pc; pc=pc->pnijpl) {

      struct REPBLOCK *ir;    /* Ptr to current repertoire block */
      struct CELLTYPE *il;    /* Ptr to current cell type  block */
      struct CONNTYPE *ix;    /* Ptr to current connection block */
      float  shcdx,shcdy;     /* Shrunk tcdx,tcdy                */
      float  t1msh;           /* (1.0 - shrink factor)           */
      long nc1or3;            /* Source connections/plot object  */
      long ncells;            /* Number of cells in map          */
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
      if (!il) {              /* Assignment intended */
         cryout(RK_E1, "0***CELL TYPE NOT FOUND", RK_LN2,
            errmsg, RK_CCL, NULL);
         goto ContinueWithError;
         }
      ir = il->pback;

      /* Locate connection block for the target connections */
      if (pc->cmtarg > il->nct) {
         cryout(RK_E1,"0***JCIJ > NCT",RK_LN2,errmsg,RK_CCL,NULL);
         goto ContinueWithError;
         }
      for (ix=il->pct1; ix->ict!=pc->cmtarg; ix=ix->pct) ;
      pc->pcmtx = ix;

      /* Match up cell list with cell type and check range limits.
      *  (If list is absent, as indicated by pcmcl being NULL, all
      *  cells will be plotted.  */
      pc->pcmcl = NULL;
      if (pc->cmclid) {
         struct CLHDR *pclh = findcl((int)pc->cmclid, FALSE);
         if (pclh) {
            if (pc->cmclndx == 0 && pclh->nclb > 1) {
               cryout(RK_E1, "0***CELL LIST MUST NOT REFER TO MORE "
                  "THAN ONE CELL TYPE", RK_LN3, errmsg, RK_CCL,
                  "    (A CLIST INDEX MIGHT RESOLVE THE MATCH.)",
                  RK_LN0, NULL);
               goto ContinueWithError;
               }
            if (pc->cmclndx <= pclh->nclb)
               d3clck(pclh, pc, CLCK_IJPL);
            }
         }

/* Error if variable to be plotted does not exist */

      if (kplt != KCIJS && ix->cnoff[cmvars[kplt]] < DVREQ)
         cryout(RK_E1, "0***NO ", RK_LN2, cmtypes[kplt], RK_CCL,
            errmsg, RK_CCL, NULL);

/* Setups that depend on type of plot:
*     Initialize flags for fetching or generating Lij.
*     Establish defaults for cmmin,cmmax.
*     Set constants needed to calculate colors.
*/
      nc1or3 = ix->nc;
      switch (kplt) {
      case KCIJSEL:
         if (!(ix->cnxc.tvcsel & EXTR3C))
            cryout(RK_E1, "0***CSELPLOT REQUIRES COLOR=(", RK_LN2,
               fmtcsel(ix->cnxc.tvcsel), RK_CCL,
               ",3)", 3, errmsg, RK_CCL, NULL);
         if ((lopt & (CMSMAP|CMTAVG)) == CMTAVG)
            cryout(RK_E1, "0***CSELPLOT WITH TARGET AVERAGING REQU"
               "IRES SOURCE MAPPING", RK_LN2, errmsg, RK_CCL, NULL);
         nc1or3 = (ix->nc + NColorDims - 1)/NColorDims;
         /* Drop into case KCIJC -- no break */
      case KCIJC:
      case KCIJC0:
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

/* Set up parameters for mapping target cells onto the plot.
*
*  There are three cases:
*  (1) cmopt&CMTAVG.  Average the Cij, Mij, or PPF values for all
*     selected cells and plot as if there is but one cell present.
*     Set cdx,cdy for full plot for use in setting source mapping.
*  (2) cmopt&CMTMAP (Target Map).  Ignore positioning of cells in
*     region plots and plot cells in neat rows of tgtncx cells per
*     row (default to sqrt(number of cells plotted)).  This option
*     is useful with sparse cell lists.
*  (3) Default.  Position cells as in region plots.  There are two
*     subcases--Interleaved and Stratified.  With Stratified, or
*     Interleaved and CMITIL mode, the single cell type is stretched
*     to occupy the full height.
*  Incompatible options are flagged in getijpl().
*/

      pc->cmtopy = pc->cmplty + pc->cmplth;
      ncells = pc->pcmcl ?
         ilstitct(pc->pcmcl->pclil, il->nelt) : il->nelt;
      if (lopt & CMTAVG) {          /* Target Average */
         ncells = 1;
         pc->tcdx = pc->cmpltw;
         pc->tcdy =-pc->cmplth;
         }
      else if (lopt & CMTMAP) {     /* Linear Target Map */
         long tnx,tny;
         if (pc->tgtncx == 0)
            pc->tgtncx = (long)sqrt((double)ncells);
         tnx = pc->tgtncx;
         tny = (ncells + tnx - 1)/tnx;
         pc->tcdx = pc->cmpltw/(float)tnx;
         pc->tcdy =-pc->cmplth/(float)tny;
         }
      else {                        /* Default Target Map */
         pc->tgtncx = (ir->ncpg == 1) ? 1 : ir->nqx;
         pc->tgdx = pc->cmpltw/(float)ir->ngx;
         pc->tgdy =-pc->cmplth/(float)ir->ngy;
         pc->tcdx = pc->tgdx / (float)pc->tgtncx;
         if (ir->Rp.krp & KRPIL && !(lopt & CMITIL)) {
            pc->tcdy = pc->tgdy/(float)ir->nqg;
            pc->cmtopy += (float)il->lnqyprev * pc->tgdy; }
         else
            pc->tcdy = pc->tgdy/(float)il->lnqy;
         }

/* Set up parameters for mapping source cells onto the plot.
*
*  There are two cases:
*  (1) cmopt & CMSMAP (Source Map).  Ignore positioning of cells in
*     region plots and plot Cij, etc. in neat rows of srcncx cells
*     per row (default to sqrt(nc)), indexed by j rather than Lij.
*  (2) Default.  Position Cij, etc. as in region plots of the
*     source layer itself.  With Stratied source geometry, or
*     Interleaved and CMISIL mode, the plot is stretched to
*     occupy the full specified height.
*/

      /* If smshrink is SHRNKDFLT, the user did not enter a value
      *  and the value in RP0->smshrnkdflt is the default, except
      *  the default is zero when there is only one map.  */
      if (pc->smshrink == SHRNKDFLT)
         pc->smshrink = (ncells <= 1) ? 0.0 : RP0->smshrnkdflt;
      t1msh = 1.0 - pc->smshrink;
      shcdx = t1msh * pc->tcdx;
      shcdy = t1msh * pc->tcdy;
      if (lopt & CMSMAP) {             /* Linear Source Map */
         long snx,sny;
         if (pc->srcncx == 0)
            pc->srcncx = (long)sqrt((double)nc1or3);
         snx = pc->srcncx;
         sny = (nc1or3 + snx - 1)/snx;
         pc->scdx = shcdx/(float)snx;
         pc->scdy = shcdy/(float)sny;
         }
      else {
         switch (ix->cnsrctyp) {       /* Default Source Map */

         case REPSRC: {
            /* Set source position for input from a repertoire */
            struct CELLTYPE *ilsrc = (struct CELLTYPE *)ix->psrc;
            struct REPBLOCK *irsrc = ilsrc->pback;
            pc->srcncx = (irsrc->ncpg == 1) ? 1 : irsrc->nqx;
            pc->sgdx = shcdx/(float)ix->srcngx;
            pc->sgdy = shcdy/(float)ix->srcngy;
            pc->scdx = pc->sgdx/(float)pc->srcncx;
            if (irsrc->Rp.krp & KRPIL && !(lopt & CMISIL)) {
               pc->scdy = pc->sgdy/(float)irsrc->nqg;
               pc->cmtopy += (float)ilsrc->lnqyprev * pc->sgdy; }
            else
               pc->scdy = pc->sgdy/(float)ilsrc->lnqy;
            } /* End case REPSRC */
            break;

         case IA_SRC:
            /* Set source position for input from input array */
            pc->scdx = shcdx/(float)RP->nsx;
            pc->scdy = shcdy/(float)RP->nsy;
            break;

         default:
            /* Set source position for virtual groups */
            pc->scdx = shcdx/(float)ix->srcngx;
            pc->scdy = shcdy/(float)ix->srcngy;

            } /* End source switch */
         } /* End CMSMAP else */

      /* Locate center of first plot symbol.  Centers are offset
      *  to allow for shrinking.  If shrink is negative, plot is
      *  expanded, and centers may go off bounds.  This is not
      *  considered an error.  (smshrink is forced on input to
      *  be < 1.0, so there cannot be divide by 0 here.)  */
      pc->xlo = pc->cmpltx + 0.5*(pc->scdx + pc->smshrink*pc->tcdx);
      pc->yhi = pc->cmtopy + 0.5*(pc->scdy + pc->smshrink*pc->tcdy);

/* If user did not provide a label, make one and store it in the
*  text cache.  Store the length of the label for plotting.  */

      if (!(lopt & CMNTTL)) {
         if (!pc->cmltxt) {
            char cmlabl[20];
            sconvrt(cmlabl,"(2A" qLSN ",A4,IH4)",
               &pc->cmtnm, cmtypes[kplt], &pc->cmtarg, NULL);
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
      pcn->kcmplt   = pc->kcmplt;
      pcn->kcmpldis = pc->kcmpldis;
      ++pcn;
#endif

      continue;

ContinueWithError:
      RK.iexit |= 1;

      } /* End loop over IJPLDEF list */

   } /* End d3ijpi() */

