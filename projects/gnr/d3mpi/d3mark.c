/* (c) Copyright 1995-2012, The Rockefeller University *11116* */
/* $Id: d3mark.c 70 2017-01-16 19:27:55Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                              d3mark.c                                *
*           Mark modality response bits in sense broadcast             *
*                                                                      *
*  This file contains the following three routines which constitute    *
*  a sort of "module" for dealing with responses in modalities.        *
*  (1) void d3mark1(struct MODALITY *pmdlt, ui16 isn,                  *
*        ui16 ign, short val)                                          *
*     This routine marks the sensing by modality 'pmdlt' of stimulus   *
*     'isn' in group 'ign' with environmental value 'val'.  It uses    *
*     group number from the environment if no GRPNO cards entered. If  *
*     'isn' exceeds 'nds' for its modality, error counter RP0->exisn   *
*     is incremented.  This routine should be called from sensory      *
*     input routines related to TV, BBD senses, etc.                   *
*  (2) void d3mark(void)                                               *
*     This routine performs marking for simulated visual senses,       *
*     that is, for all the objects placed on the input array by        *
*     the 'env' package.                                               *
*  (3) void d3mclr(void)                                               *
*     This routine prepares for a new trial by clearing the pxcyc      *
*     and mbits stimulus marking bits and setting mdltvalu to dflt.    *
*                                                                      *
*  These routines are called only on host node or in serial version.   *
************************************************************************
*  V8A, 05/27/95, GNR - Initial version                                *
*  V8B, 12/27/00, GNR - New memory management routines, trial timers   *
*  ==>, 02/13/07, GNR - Last mod before committing to svn repository   *
*  V8H, 02/25/11, GNR - Add KAM=C for categorization models            *
*  Rev, 04/28/11, GNR - Add CTOPT=C for new-stim-group fast start      *
*  Rev, 07/28/12, GNR - Eliminate bit offsets and bitiors              *
*  Rev, 08/16/12, GNR - Use mdltbid to eliminate mdltno bit tests      *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "d3global.h"
#include "celldata.h"
#include "wdwdef.h"
#include "objdef.h"
#include "bapkg.h"

extern struct CELLDATA CDAT;

/*---------------------------------------------------------------------*
*                               d3mark1                                *
*---------------------------------------------------------------------*/

void d3mark1(struct MODALITY *pmdlt, ui16 isn, ui16 ign, short val) {

/* Check and record stimulus number */

   if (isn > pmdlt->nds || isn == 0)
      ++RP0->exisn;
   else {
      long lisn = (long)isn;
      /* Record stimulus number (isn) and largest isn seen */
      bitset(pmdlt->pxcyc, lisn);
      if (pmdlt->mdltflgs & MDLTFLGS_USED)
         bitset(CDAT.pmbits + pmdlt->mdltboff, lisn);
      pmdlt->um1.MS.mxisn = max(pmdlt->um1.MS.mxisn, lisn);

      /* Record value of this stimulus or pick up default.
      *  N.B.  Test will fail if val is changed to an int.  */
      if (pmdlt->mdltflgs & MDLTFLGS_VALU) {
         if ((unsigned short)val == (unsigned short)DFLT_ENVVAL)
            val = pmdlt->penvl ? pmdlt->penvl[isn-1] : DFLT_VALUE;
         if (val > pmdlt->mdltvalu) pmdlt->mdltvalu = val;
         }

      /* Record group number of this stimulus, but only if pgpno
      *  array exists but has not been preloaded by GRPNO card.  */
      if (pmdlt->ngpnt == 0) pmdlt->pgpno->grp[isn-1] = ign;
      }

/* Record group number for broadcast for KAMCG and changes for OPTCFS.
*  Unlike the usage above for stats, here we may not exceed ncs.  */
   if (pmdlt->mdltflgs & (MDLTFLGS_KAMC|MDLTFLGS_CTFS)) {
      if (ign > pmdlt->ncs || ign == 0)
         ++RP0->exisn;
      else {
         long lign = (long)ign;
         if (pmdlt->mdltflgs & MDLTFLGS_KAMC)
            bitset(CDAT.pmbits + pmdlt->mdltgoff, lign);
         if (pmdlt->mdltflgs & MDLTFLGS_CTFS) {
            long nb = (long)pmdlt->ncsbyt;
            if (!bittst(pmdlt->pignc+nb, lign))
               bitset(CDAT.pmbits, (long)pmdlt->mdltnoff);
            bitset(pmdlt->pignc, lign);
            }
         } /* End else ign is OK */
      } /* End checking KAMC and CTFS data */

   } /* End d3mark1 */


/*---------------------------------------------------------------------*
*                               d3mark                                 *
*---------------------------------------------------------------------*/

void d3mark(void) {

   struct OBJDEF *pfobj = envgob1(RP0->Eptr); /* Ptr to first object */
   struct OBJDEF *pobj;       /* Ptr to current object */
   struct MODALITY *pmdlt;    /* Ptr to current modality */
   mdlt_type omdlts;          /* Modality object associated with */
   byte refno;                /* Ref number of current object */

/* Loop over all current objects, skipping over those that are
*  not visible.  Visibility test is abstracted in qobjvis() in
*  the env package--see envcyc.c for discussion of conditions
*  under which an object can become invisible.  */

   for (pobj=pfobj; pobj; pobj=pobj->pnxobj) {
      if (!qobjvis(pobj)) continue;
      refno = (byte)pobj->irf;
      omdlts = (mdlt_type)pobj->userdata;

/* Loop over modalities and identify those by which the current
*  object can be sensed.  The bit array test is inefficient, but
*  there are no obvious alternatives.  In general, there are
*  probably <= 4 modalities, so this code is not too bad.
*  Note:  At one time the plan was to sort the modality blocks
*  with all the visual ones first--then this loop could stop on
*  reaching the first nonvisual one.  But, we need them in the
*  original order for the indexing calculations in d3go().  */

      for (pmdlt=RP->pmdlt1; pmdlt; pmdlt=pmdlt->pnmdlt) {
         if (!(omdlts & pmdlt->mdltbid)) continue;

         if (pmdlt->mdltwdw) {

/* The current modality is linked to a particular window.  We must
*  determine whether the object is visible through that window in
*  its present position.  A detailed pixel scan is necessary only
*  if the object and window positions overlap.  There is no overlap
*  if xho < xlw, xlo > xhw, yho < ylw, or ylo > yhw, with allowance
*  for toroidal object boundary conditions.  Recall that xw,yw in
*  wdwdef refer to the lower-left-hand corner of the window.  The
*  object coords are (S16)--convert to integers for these tests.
*
*  It is theoretically possible that an object could be viewed by
*  two different modalities through one and the same window.  The
*  results of the visibility test could be recorded and checked to
*  avoid repetition of the following code, but the savings that
*  could be obtained do not appear to be worth the added complexity.
*/
            struct WDWDEF *pwdw = pmdlt->pmdwdw;
            byte *pia,*px;          /* Ptr to input array, col. */
            byte *xls[2],*xhs[2];   /* X lo search, x hi search */
            long lnsy,lnsy2;        /* Local copy of nsy,2*nsy  */
            long y;                 /* Y value during search    */
            long yls[2],yhs[2];     /* Y lo search, y hi search */
            long xlo,xho;           /* X lo object, x hi object */
            long ylo,yho;           /* Y lo object, y hi object */
            long xlp,xhp;           /* X lo border, x hi border */
            long ylp,yhp;           /* Y lo border, y hi border */
            long xlw,xhw;           /* X lo window, x hi window */
            long ylw,yhw;           /* Y lo window, y hi window */
            int lky1;               /* Local copy of RP->ky1    */
            int ixs, iys;           /* X,Y range selectors      */
            int nxs, nys;           /* Number of x,y searches   */

/* It may be necessary to determine the object's real bounds */

            if (pobj->objflag & OBJBR) envlim(RP0->Eptr, pobj);

/* First check x coords */

            xlw = (long)pwdw->wx;
            xhw = (long)(pwdw->wx + pwdw->wwd);
            xlo = pobj->icurx + pobj->elox, xlo = SRA(xlo,16);
            xho = pobj->icurx + pobj->ehix, xho = SRA(xho,16);
            xlp = (long)pobj->pxmin;
            xhp = (long)pobj->pxmax;
            if (qobjtor(pobj)) {          /* BC=TOROIDAL */
               long xsz = xhp - xlp + 1;  /* Boundary box size */
               long xsz12 = xsz<<12;
               xlo = (xlo - xlp +         /* Calc xlo modulo edge */
                  xsz12) % xsz + xlp;     /* Force pos remainder */
               xho = (xho - xlp +
                  xsz12) % xsz + xlp;
               if (xlo < xho) {           /* No wrap */
                  if (xlo > xhw || xho < xlw) continue; }
               else {                     /* Object wraps around */
                  if (xlo > xhw && xho < xlw) continue; }
               } /* End TOROIDAL */
            else {                        /* BC=ZERO */
               xlo = max(xlo,xlp);
               xho = min(xho,xhp);
               if (xlo > xhw || xho < xlw) continue;
               } /* End ZERO */

/* Then check y coords */

            ylw = (long)(pwdw->wy - pwdw->wht);
            yhw = (long)pwdw->wy;
            ylo = pobj->icury + pobj->eloy, ylo = SRA(ylo,16);
            yho = pobj->icury + pobj->ehiy, yho = SRA(yho,16);
            ylp = (long)pobj->pymin;
            yhp = (long)pobj->pymax;
            if (qobjtor(pobj)) {          /* BC=TOROIDAL */
               long ysz = yhp - ylp + 1;  /* Boundary box size */
               long ysz12 = ysz<<12;
               ylo = (ylo - ylp +         /* Calc ylo modulo edge */
                  ysz12) % ysz + ylp;     /* Force pos remainder */
               yho = (yho - ylp +
                  ysz12) % ysz + ylp;
               if (ylo < yho) {           /* No wrap */
                  if (ylo > yhw || yho < ylw) continue; }
               else {                     /* Object wraps around */
                  if (ylo > yhw && yho < ylw) continue; }
               } /* End TOROIDAL */
            else {                        /* BC=ZERO */
               ylo = max(ylo,ylp);
               yho = min(yho,yhp);
               if (ylo > yhw || yho < ylw) continue;
               } /* End ZERO */

/* Object not eliminated by simple bounds tests.  Must scan those
*  pixels that are common to the object and window rectangles to
*  determine whether any object pixels are actually visible at the
*  present window location.  There can be up to four such regions
*  under toroidal boundary conditions.  The x and y looping ranges
*  are manipulated to minimize index arithmentic in the searches.  */

            lnsy = RP->nsy, lnsy2 = lnsy + lnsy;
            lky1 = RP->ky1;
            pia = RP->pstim + lnsy;
            nxs = 0;
            if (xho >= xlo) {             /* No wrap */
               xls[0] = pia + (max(xlo,xlw)<<lky1);
               xhs[0] = pia + (min(xho,xhw)<<lky1);
               }
            else {                        /* Object wraps around */
               xls[0] = pia + (max(xlp,xlw)<<lky1);
               xhs[0] = pia + (min(xho,xhw)<<lky1);
               if (xhs[0] >= xls[0]) ++nxs;
               xls[nxs] = pia + (max(xlo,xlw)<<lky1);
               xhs[nxs] = pia + (min(xhp,xhw)<<lky1);
               }
            if (xhs[nxs] >= xls[nxs]) ++nxs;

            nys = 0;
            if (yho >= ylo) {             /* No wrap */
               yls[0] = max(ylo,ylw);
               yhs[0] = min(yho,yhw);
               }
            else {                        /* Object wraps around */
               yls[0] = max(ylp,ylw);
               yhs[0] = min(yho,yhw);
               if (yhs[0] >= yls[0]) ++nys;
               yls[nys] = max(ylo,ylw);
               yhs[nys] = min(yhp,yhw);
               }
            if (yhs[nys] >= yls[nys]) ++nys;

/* Perform the pixel scans */

            for (ixs=0; ixs<nxs; ixs++) {
               for (px=xls[ixs]; px<=xhs[ixs]; px+=lnsy2) {
                  for (iys=0; iys<nys; iys++) {
                     for (y=yls[iys]; y<=yhs[iys]; y++) {
                        if (px[y] == refno) goto SawObject;
                        } /* End y loop */
                     } /* End y range loop */
                  } /* End x loop */
               } /* End x range loop */
            /* Checked all pixels and no portion of object
            *  overlaps the window.  Go on to next modality.  */
            continue;
            } /* End window-linked modality */

/* The object is visible through the window at its current position,
*  or else the current modality is not linked to a particular window.
*  In that case, the object must be visible from somewhere on the
*  input array, and it is not necessary actually to inspect that
*  array.  Use the 'isn' number left in the object block by the env
*  package to mark the appropriate modality object-sense bit.  */

SawObject:
         d3mark1(pmdlt, pobj->isn, pobj->ign, pobj->iadapt);

         } /* End modality loop */
      } /* End object loop */
   } /* End d3mark */


/*---------------------------------------------------------------------*
*                               d3mclr                                 *
*---------------------------------------------------------------------*/

void d3mclr(void) {

   struct MODALITY *pmdlt;    /* Ptr to current modality */

   memset((char *)CDAT.pmbits, 0, RP->cumnds);

   for (pmdlt=RP->pmdlt1; pmdlt; pmdlt=pmdlt->pnmdlt) {
      memset((char *)pmdlt->pxcyc, 0, (size_t)pmdlt->ndsbyt);
      if (pmdlt->pignc) {     /* Track stim class changes */
         size_t nb = (size_t)pmdlt->ncsbyt;
         memcpy((char *)pmdlt->pignc+nb, (char *)pmdlt->pignc, nb);
         memset((char *)pmdlt->pignc, 0, nb);
         }
      pmdlt->mdltvalu = DFLT_ENVVAL;
      } /* End modality scan */

   } /* End d3mclr */

