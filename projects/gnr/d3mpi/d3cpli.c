/* (c) Copyright 2018, The Rockefeller University *11116* */
/* $Id: d3cpli.c 77 2018-03-15 21:08:14Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                              d3cpli.c                                *
*                   Cell plot location initializer                     *
*                                                                      *
*  This function should be called at the start of each new set of      *
*  trial series for each type of plot that involves mapping cells      *
*  onto a grid: layer plot, neuroanatomy plot, all types of Cijplots.  *
*                                                                      *
*  It uses information about the object being plotted and the location *
*  of the plot grid to generate a cploc structure that can then be     *
*  used when each object is drawn to find its location quickly.  Plot  *
*  origin, width, height are given to allow making nonsuperposition    *
*  plots.                                                              *
*                                                                      *
*  On a parallel computer, call from host node only and arrange for    *
*  output cploc structure to be broadcast to comp nodes.               *
*                                                                      *
*  This code is a generalization of d3npsl, which originally only      *
*  handled location of sources for neuroanatomy plots.  This avoided   *
*  need for VCELL blocks to be broadcast to comp nodes and also saved  *
*  a little time by computing locator constants only once per CYCLE    *
*  card instead of once per trial.                                     *
*                                                                      *
*  Synopsis:                                                           *
*     int d3cpli(cploc *pcpl, struct CELLTYPE *il, void *psi,          *
*        float xl, float yl, float wd, float ht, int kp)               *
*                                                                      *
*  Arguments:                                                          *
*     pcpl  Ptr to the cploc struct that is to be initialized.         *
*     il    Pointer to a CELLTYPE block for target cell type (cell     *
*           type being plotted or target when a source is plotted).    *
*     psi   Pointer to source information:  differs according to kp.   *
*     xl,yl Origin of plot (lower left corner coordinates).            *
*     wd,ht Width and height of plot.                                  *
*     kp    Type of plot                                               *
*              KP_LAYER (0)   Layer plot (psi may be NULL)             *
*              KP_NANAT (1)   Neuroanatomy plot (psi is ptr to         *
*                             CONNTYPE for source being plotted,       *
*                             which may be real cells, IA, or VG.      *
*                             xl,yl,wd,ht args are not used.           *
*              KP_CIJPT (2)   Target cells for a connection plot       *
*                             (psi is ptr to IJPLDEF describing plot). *
*              KP_CIJPS (3)   Source cells for a connection plot       *
*                             (psi is ptr to IJPLDEF describing plot). *
*                                                                      *
*  Return value:                                                       *
*     Zero if no errors, otherwise an error code from the list in      *
*  d3global.h so the caller can make an informative message.  This     *
*  routine is called before the last RK.iexit check, so a cryout       *
*  RK_E1 message is appropriate.                                       *
************************************************************************
*  R72, 03/10/17, GNR - New program, based on d3npsl                   *
*  ==>, 03/27/17, GNR - Last mod before committing to svn repository   *
*  R73, 04/18/17, GNR - Convert KGNAN to circular or ellipsoidal areas *
*  R77, 03/12/18, GNR - Add kctp=H, hold plot                          *
***********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "sysdef.h"
#include "rkarith.h"
#include "rocks.h"
#include "d3global.h"
#include "tvdef.h"
#include "clblk.h"
#include "ijpldef.h"

#ifdef PARn
#error Program d3cpli is not used on comp nodes
#endif

/*=====================================================================*
*                             LayerCploc                               *
*                                                                      *
*  Generate cploc to locate cells in a cell layer, taking into account *
*  arborization map scaling, inter-group gaps, and layer interleaving. *
*  By design, gaps are expanded by any applied scale.  Gaps divide     *
*  groups, never layers.                                               *
*                                                                      *
*  Arguments:                                                          *
*     il       Ptr to celltype being plotted                           *
*     xl,yl    Lower left corner of plot area                          *
*     wd,ht    Width and height of plot area                           *
*     whm      Width and height multiplier (to implement pc->smscale)  *
*     kxyo     TRUE to offset x,y origins by 0.5 times x,y spacing     *
*     kiint    TRUE to ignore interleaving of layers                   *
*                                                                      *
*  Error cases:  If il points to a layer that is not plotted (CTPHP),  *
*  we give a terminal error, but compute the mapping as if plotting    *
*  will occur, in order to avoid various errors secondary to this one. *
*  (Here we can catch all versions of this error in one place.)        *
*=====================================================================*/

static void LayerCploc(cploc *pcpl, struct CELLTYPE *il, float xl,
      float yl, float wd, float ht, float whm, int kxyo, int kiint) {

   struct REPBLOCK *ir = il->pback;
   float Gdx,Gdy;          /* Goup increments in x,y */
   float Cdx,Cdy;          /* Cell increments in x,y */
   float fngx = (float)ir->ngx, fngy = (float)ir->ngy;
   float fnqx = (float)ir->nqx, fnqy = (float)ir->nqy;
   /* Input checks that gaps are >= 0, d3tchk that gaps are < 1.0 */
   float Sxg = 1.0 - ir->Rp.xgap, Syg = 1.0 - ir->Rp.ygap;
   float Swd = whm*wd, Sht = whm*ht;
   int kinlv = ir->Rp.krp & KRPIL && !kiint;

   /* Error if cells not being plotted */
   if (il->Ct.kctp & CTPHP) cryout(RK_E1, "0***ATTEMPTING TO PLOT "
      "CONNECTIONS TO/FROM ", RK_LN2, fmturlnm(il), RK_CCL,
      " WITH KCTP=H.", RK_CCL, NULL);

   /* X location is independent of stacking mode */
   pcpl->gdx = Gdx = Swd/fngx;
   pcpl->cdx = Cdx = Sxg*Gdx/fnqx;
   pcpl->xlo = xl + 0.5*(wd - Swd);
   /* Y location depends on stratified vs interleaved */
   pcpl->yhi = yl + 0.5*(ht + Sht);
   Cdy = -Sht/(fngy*fnqy);    /* Before gap correction */
   if (kinlv) {               /* Interleaved layers */
      Gdy = Cdy*fnqy;
      pcpl->yhi += (float)il->lnqyprev*Cdy;
      }
   else {                     /* Stratified layers */
      Gdy = Cdy*(float)il->lnqy;
      pcpl->yhi += fngy*((float)il->lnqyprev*Cdy);
      }
   pcpl->cdy = Syg*Cdy, pcpl->gdy = Gdy;
   if (kxyo) {
      pcpl->xlo += 0.5*(pcpl->cdx + ir->Rp.xgap*Gdx);
      pcpl->yhi += 0.5*(pcpl->cdy + ir->Rp.ygap*Gdy);
      }
   return;
   }

/*=====================================================================*
*                               d3cpli                                 *
*=====================================================================*/

int d3cpli(cploc *pcpl, struct CELLTYPE *il, void *psi,
      float xl, float yl, float wd, float ht, int kp) {

   struct REPBLOCK *ir = il->pback;
   struct CONNTYPE *ix;
   struct IJPLDEF  *pc;

   float Gdx,Gdy;          /* Goup increments in x,y */

   switch (kp) {

/*---------------------------------------------------------------------*
*          KP_LAYER:  Position cells in standard layer plot            *
*                                                                      *
*  This is same as positioning source cells in a neuroanatomy plot     *
*  except using ir and il for the target rather than the source.       *
*---------------------------------------------------------------------*/

   case KP_LAYER:
      LayerCploc(pcpl, il, xl, yl, wd, ht, 1.0, YES, NO);
      break;      /* End KP_LAYER case */

/*---------------------------------------------------------------------*
*        KP_NANAT:  Position source cells in neuroanatomy plot         *
*                                                                      *
*  Geometry of source cells (interleaved or stratified) is same as for *
*  plotting the source layer itself, regardless of whether projection  *
*  or long vectors are drawn.  While this might lead to longer vectors *
*  if the two types are mixed in a projection plot, it is the only way *
*  that avoids confusion if both layers are plotted on the same super- *
*  position plot. It would be permissible to match the target geometry *
*  to source in a non-superposition plot.  If source has no valid      *
*  location, set cnflgs SKPNAP.                                        *
*                                                                      *
*  R73, 04/21/17, GNR - With projection vectors, if the target has     *
*  multiple stratified layers, the source is mapped just onto the area *
*  of the target, but if the target is interleaved, the whole target   *
*  area is used.  This will be documented--it is the only reasonable   *
*  thing to do because source and target may have different numbers    *
*  of layers, different nqy, etc.                                      *
*                                                                      *
*  Note:  This is the only case where the specific source is accessed. *
*  To avoid this, the locations and sizes of the source windows would  *
*  have to be generalized and stored in the CONNTYPE block.            *
*                                                                      *
*  Note:  When input is a scan window, in this case the entire input   *
*  array is used as the canvas, so the plot shows where the window is  *
*  located on the IA rather than just the window, blown up.            *
*---------------------------------------------------------------------*/

   case KP_NANAT: {
      float fnx,fny;
      ix = (struct CONNTYPE *)psi;

      fnx = (float)ix->srcngx;
      fny = (float)ix->srcngy;
      pcpl->gdx = pcpl->gdy = 0;       /* For all but REPSRC */
      if (il->Ct.kctp & KRPPJ) {

/* Setup for short (projection) source vector plotting.  Map source
*  onto region of stratified target per rules given above */

         float yu,hu,yhiu;             /* Target y low, height used */
         yu = ir->aply, hu = ir->aplh;
         if (!(ir->Rp.krp & KRPIL)) {  /* Stratified--use layer area */
            yu += hu*(1.0 - (float)(il->lnqyprev + il->lnqy)/
               (float)ir->nqy);
            hu *= (float)il->lnqy/(float)ir->nqy;
            }
         yhiu = yu + hu;

         switch (ix->cnsrctyp) {
         case REPSRC:               /* Input from REPERTOIRE */
            /* il passed to LayerCploc always needs to be il for the
            *  source cells used to locate them in the area specified
            *  in the x,y,h,w in the call for one target cell.  */
            il = (struct CELLTYPE *)ix->psrc;
            LayerCploc(pcpl, il, ir->aplx, yu, ir->aplw, hu,
               1.0, YES, NO);
            break;
         case IA_SRC:               /* Input array */
         case VS_SRC:               /* Scan window */
            /* Storing cdx = (x increment)/2 saves a step in d3lplt */
            pcpl->xlo = ir->aplx + (pcpl->cdx = 0.5*ir->aplw/fnx);
            pcpl->yhi = yhiu + 0.5*(pcpl->cdy = -hu/fny);
            break;
         case USRSRC:               /* Env & external sensors */
         case VJ_SRC:
         case VW_SRC:
         case VT_SRC:
         case TV_SRC:
            pcpl->xlo = ir->aplx + 0.5*(pcpl->cdx = ir->aplw/fnx);
            pcpl->yhi = yhiu + 0.5*(pcpl->cdy = -hu/fny);
            break;
         case PP_SRC:               /* Image preprocessor */
            /* R73, 04/22/17, GNR - Earlier versions set cdy for one-
            *  column plots here even though d3lplt did calc for dcols
            *  cols in all cases.  We now use dcols in all cases, i.e.
            *  rppc rows/col rather than nytot rows total.  */
            pcpl->xlo = ir->aplx + 0.5*(pcpl->cdx = ir->aplw/fnx);
            pcpl->yhi = yhiu + 0.5*(pcpl->cdy = -hu/(float)ix->rppc);
            break;
         default:
            ix->cnflgs |= SKPNAP;
            break;
            } /* End cnsrctyp switch */

         } /* End projection plot setup */

      else {

/* Setup for long (to source) source vector plotting */

         switch (ix->cnsrctyp) {
         case REPSRC:               /* Input from REPERTOIRE */
            il = (struct CELLTYPE *)ix->psrc;
            ir = il->pback;         /* Map to source rep area */
            LayerCploc(pcpl, il, ir->aplx, ir->aply, ir->aplw,
               ir->aplh, 1.0, YES, NO);
            break;
         case IA_SRC:               /* Input array */
         case VS_SRC:               /* Scan window */
            /* Storing cdx = (x increment)/2 saves a step in d3lplt */
            pcpl->xlo = RP->eplx +
               (pcpl->cdx =  0.5*RP->eplw/fnx);
            pcpl->yhi = RP->eply + RP->eplh +
               0.5*(pcpl->cdy = -RP->eplh/fny);
            break;
         case USRSRC:
         case VJ_SRC:
         case VW_SRC:
         case VT_SRC: {             /* Env & external sensors */
            /* Get pointer to source vcell block */
            struct VCELL *pvc = (struct VCELL *)ix->psrc;
            /* This code formerly always used nsxl,nsya.
            *  R73 will use ntl,nta for VT_SRC as that is what
            *  is in fnx,fny = srcngx,srcngy.  Must check.  */
            pcpl->xlo = pvc->vcplx +
               0.5*(pcpl->cdx = pvc->vcplw/fnx);
            pcpl->cdy = -pvc->vcplh/fny;
            pcpl->yhi = pvc->vcply + pvc->vcplh +
               ((float)ix->olvty + 0.5)*pcpl->cdy;
            } /* End sensors */
            break;
         case TV_SRC: {             /* Camera image source */
            struct TVDEF *ptv = (struct TVDEF *)ix->psrc;
            /* Can't do this plot if image is 1:1 bitmap */
            if (ptv->tvwd <= 0.0 || ptv->tvht <= 0.0)
               ix->cnflgs |= SKPNAP;
            else {
               pcpl->xlo = ptv->tvplx +
                  0.5*(pcpl->cdx = ptv->tvwd/fnx);
               pcpl->yhi = ptv->tvply + ptv->tvht +
                  0.5*(pcpl->cdy = -ptv->tvht/fny);
               }
            } /* End camera input */
            break;
         case PP_SRC: {             /* Image preprocessor */
            struct PREPROC *pip = (struct PREPROC *)ix->psrc;
            /* Can't do this plot if image is 1:1 bitmap */
            if (pip->ipwd <= 0.0)
               ix->cnflgs |= SKPNAP;
            else {
               pcpl->xlo = pip->ipplx +
                  0.5*(pcpl->cdx = pip->ipwd/fnx);
               pcpl->yhi = pip->ipply + pip->ipht +
                  0.5*(pcpl->cdy = -pip->ipht/(float)ix->rppc);
               }
            } /* End image preprocessor input */
            break;
         default:
            ix->cnflgs |= SKPNAP;
            break;
            } /* End cnsrctyp switch */
         } /* End long vector setup */
      break;
      }  /* End KP_NANAT case and local scope */

/*---------------------------------------------------------------------*
*         KP_CIJPT:  Position target cells for CIJ etc plots           *
*                                                                      *
*  There are three cases:                                              *
*  (1) cmopt&CMTAVG.  Average the Cij, Mij, etc. values over all       *
*     selected target cells and make one source map centered on the    *
*     center of the plot.  cd[xy] are used to set symbol sizes only.   *
*  (2) cmopt&CMTMAP (Target Map).  Ignore positioning of target cells  *
*     in region plots and plot cells in neat rows of tgtncx cells per  *
*     row (default to sqrt(number of cells plotted)).  This option     *
*     is useful with sparse cell lists.                                *
*  (3) Default.  Position cells as in region plots.  This case can be  *
*     handled by the same code that handles ordinary region plots      *
*     except that CMITIL option must be honored.                       *
*  Here we do not want to offset the origin (xlo,yhi) by half the size *
*  the box for one cell, because a source map is inserted in this box. *
*---------------------------------------------------------------------*/

   case KP_CIJPT:
      pc = (struct IJPLDEF *)psi;
      pc->cmtopy = yl + ht;
      if (pc->cmopt & CMTAVG) {        /* Target Average */
         pc->tgtncx = 1;
         pcpl->gdx = pcpl->gdy = 0;
         pcpl->cdx = wd;
         pcpl->cdy = -ht;
         pcpl->xlo = xl;
         pcpl->yhi = pc->cmtopy;
         pc->cmopt &= ~CMSGRD;         /* No grid for avg plot */
         }
      else if (pc->cmopt & CMTMAP) {   /* Linear Target Map */
         int tnx,tny;
         int ncells = pc->pcmcl ?
            (int)ilstitct(pc->pcmcl->pclil,il->nelt) : il->nelt;
         if (pc->tgtncx == 0)
            pc->tgtncx = (ui32)sqrt((double)ncells);
         tnx = (int)pc->tgtncx;
         tny = (ncells + tnx - 1)/tnx;
         pcpl->gdx = pcpl->gdy = 0;
         pcpl->cdx = wd/(float)tnx;
         pcpl->xlo = xl;
         pcpl->cdy = -ht/(float)tny;
         pcpl->yhi = pc->cmtopy;
         }
      else {                           /* Default Target Map */
         pc->tgtncx = (ui32)il->lnqx;
         LayerCploc(pcpl, il, xl, yl, wd, ht, 1.0, NO,
            (int)(pc->cmopt & CMITIL));
         }
      break;      /* End KP_CIJPT case */

/*---------------------------------------------------------------------*
*         KP_CIJPS:  Position source cells for CIJ etc plots           *
*                                                                      *
*  There are three cases:                                              *
*  (1) cmopt & CMSMAP (Source Map).  Ignore positioning of cells in    *
*     region plots and plot Cij, etc. in neat rows of srcncx cells     *
*     per row (default to sqrt(nc)), indexed by j rather than Lij.     *
*  (2) cmopt & CMBOX (Box map).  Allocate for each source cell a box   *
*     with nrx x nry slots (only with kgen = B,C,K,Q).  With kgen      *
*     code B, the positions can be calculated from the jsyn alone      *
*     and xlo,yhi can be set here for all source cells.  With C,K,Q,   *
*     the actual Lij must be accessed and that means the xlo,yhi are   *
*     different for every source cell. This code stores the x,y of the *
*     center of the box there and callers must identify the first Lij, *
*     subtract if from and Lij to be plotted, and apply the difference *
*     gx,gy,qx,qy to the cploc to find the plot location.  (The caller *
*     should be aware that the first generated connection is in the    *
*     center of the box for types C,K,Q, but the top left corner for   *
*     type B--changing this is on the wishlist.)                       *
*  (3) Default.  Position Cij, etc. as in region plots of the source   *
*     layer itself.  This allows for source cells to be anywhere in    *
*     the source layer.  With Stratified source geometry, or Inter-    *
*     leaved and CMISIL mode, the plot is stretched to occupy the      *
*     full specified height.                                           *
*  The space available for one map is considered to be the space for   *
*  one cell (or the average over all cells) as computed in the code    *
*  immediately above.  This may be expanded via the smscale parameter  *
*  to fill in more space when there is a sparse target cell list.      *
*  The xlo,yhi values are adjusted to keep the figure centered.        *
*  These parameters or others may be passed to this call via the wd,ht *
*  parameters.                                                         *
*                                                                      *
*  In the default case with input from a scan window, the connections  *
*  are mapped to a box the size of the window, not the full IA.  This  *
*  is different from KP_NANAT, but consistent with CMSMAP and CMBOX.   *
*---------------------------------------------------------------------*/

   case KP_CIJPS: {
      float Swd,Sht;                   /* Scaled wd,ht */
      pc = (struct IJPLDEF *)psi;
      pc->kcmsmap &= KCM_DISABL;       /* Clear mapping options */
      ix = pc->pcmtx;
      Swd = pc->smscale*wd;
      Sht = pc->smscale*ht;

      if (pc->cmopt & CMSMAP) {        /* Linear Source Map */
         float snx,sny;
         if (pc->srcncx == 0)
            pc->srcncx = (ui32)sqrt((double)pc->nc1or3);
         snx = (float)pc->srcncx;
         sny = (float)((pc->nc1or3 + pc->srcncx - 1)/pc->srcncx);
         pcpl->gdx = pcpl->gdy = 0;
         pcpl->cdx = Swd/snx;
         pcpl->xlo = xl + 0.5*(wd - Swd*(snx-1.0)/snx);
         pcpl->cdy = -Sht/sny;
         pcpl->yhi = pc->cmtopy - 0.5*(ht - Sht*(sny-1.0)/sny);
         pc->kcmsmap |= KCM_CFJSYN;
         } /* End CMSMAP */

      else if (pc->cmopt & CMBOX) {    /* Source box map */
         if (ix->kgen & (KGNCF|KGNAN|KGNKN)) {
            /* Crow's foot, annulus, or kernel.  Spacings derive
            *  from Lijs, with first Lij at center.  */
            pcpl->xlo = xl + 0.5*wd;
            pcpl->yhi = yl + 0.5*ht;
            switch (ix->cnsrctyp) {
            case REPSRC:
               /* In this case, nrx,nry refer to groups, not cells.
               *  nrx,nry sets box size, but not number of cells.
               *  We are not inserting group gaps here and stratified/
               *  interleaved does not matter--we are making a map.  */
               il = (struct CELLTYPE *)ix->psrc;
               pcpl->gdx = Gdx = Swd/(float)ix->nrx;
               pcpl->cdx = Gdx/(float)(pc->srcncx = il->lnqx);
               pcpl->gdy = Gdy = -Sht/(float)ix->nry;
               pcpl->cdy = Gdy/(float)il->lnqy;
               pc->kcmsmap |= (KCM_GCFLIJ|KCM_FSTXYA);
               break;
            default:
               pcpl->gdx = pcpl->gdy = 0;
               pcpl->cdx = Swd/(float)(pc->srcncx = ix->srcngx);
               pcpl->cdy = Sht/(float)ix->srcngy;
               pc->kcmsmap |= (KCM_COFLIJ|KCM_FSTXYA);
               break;
               } /* End source switch */
            } /* End (KGNCF|KGNAN|KGNKN) */
         else if (ix->kgen & KGNBL) {
            /* Rectangular box.  Spacings are derived from jsyn,
            *  with first connection at upper left.  */
            pcpl->gdx = pcpl->gdy = 0;
            pcpl->cdx = Swd/(float)(pc->srcncx = (ui32)ix->nrx);
            pcpl->xlo = xl + 0.5*(wd - Swd + pcpl->cdx);
            pcpl->cdy = -Sht/(float)ix->nry;
            pcpl->yhi = pc->cmtopy - 0.5*(ht - Sht + pcpl->cdy);
            pc->kcmsmap |= KCM_CFJSYN;
            } /* End KGNBL */
         else {
            pc->kcmsmap |= KCM_DISABL;   /* Disable the plot */
            return IJPLBOX_ERR;
            }
         } /* End CMBOX */

      else {                           /* Default Source Map */
         switch (ix->cnsrctyp) {
         case REPSRC:
            /* Set source position for input from a repertoire */
            il = (struct CELLTYPE *)ix->psrc;
            LayerCploc(pcpl, il, xl, yl, wd, ht, pc->smscale, YES,
               (int)(pc->cmopt & CMISIL));
            pc->srcncx = il->lnqx;
            pc->kcmsmap |= KCM_GCFLIJ;
            return 0;
         case IA_SRC:
         case VS_SRC:
            /* Set source position for input from input array
            *  or scan window  */
            pcpl->cdx = Swd/(float)(pc->srcncx = ix->iawxsz);
            pcpl->cdy = -Sht/(float)ix->iawysz;
            pc->kcmsmap |= KCM_IAFLIJ;
            break;
         default:
            /* Set source position for virtual groups */
            pcpl->cdx = Swd/(float)(pc->srcncx = ix->srcngx);
            pcpl->cdy = -Sht/(float)ix->srcngy;
            pc->kcmsmap |= KCM_COFLIJ;
            } /* End source switch */
         pcpl->gdx = pcpl->gdy = 0;
         pcpl->xlo = xl + 0.5*(wd - Swd + pcpl->cdx);
         pcpl->yhi = pc->cmtopy - 0.5*(ht - Sht - pcpl->cdy);
         } /* End CMSMAP else */
      break;
      } /* End KP_CIJPS case and local scope */

      } /* End kp switch */
   return 0;
   } /* End d3cpli() */
