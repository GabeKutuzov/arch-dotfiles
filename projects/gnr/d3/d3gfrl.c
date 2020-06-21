/* (c) Copyright 2018, The Rockefeller University *11116* */
/* $Id: d3gfrl.c 77 2018-03-15 21:08:14Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                               d3gfrl                                 *
*         CALCULATE RECORD LENGTH FOR A SIMDATA FILE SEGMENT           *
*                                                                      *
*  Besides computing record lengths, this routine is also responsible  *
*  for preparing working versions (suitms) of cell type and connection *
*  type svitms, with bits for nonexistent variables turned off.  But   *
*  earlier code in d3gfcl establishes the GFSI and GFGLBL bits.        *
*                                                                      *
*  Uses the graph data file format designed by Pearson & Shu (V.3).    *
*  This routine must be called on host node after last time svitms     *
*  can change and before final tree broadcast to computational nodes.  *
*  Calculates length of the data for one cell that must be gathered    *
*  from comp nodes in a parallel computer.                             *
*                                                                      *
*  Arguments:                                                          *
*     il    Ptr to CELLTYPE block for cell type whose record length    *
*           is wanted.                                                 *
*     gdm   Mask specifying svitems bits that are to be included.      *
*                                                                      *
*  Return value:                                                       *
*     lcld  Length of simdata record for one cell of specified type.   *
*     Total number of cells in cell list or nelt if no cell list is    *
*           returned at il->nsdcan.                                    *
************************************************************************
*  R72, 02/22/17, GNR - New routine, broken out from d3gfsh to allow   *
*                       calc of record length indep of saving header.  *
*  ==>, 02/26/17, GNR - Last mod before committing to svn repository   *
*  R76, 12/03/17, GNR - Add VMP                                        *
*  R77, 02/09/18, GNR - Add RAW afference                              *
***********************************************************************/

#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "d3global.h"
#include "swap.h"
#include "clblk.h"
#include "simdata.h"

/*=====================================================================*
*                               d3gfrl                                 *
*=====================================================================*/

ui32 d3gfrl(struct CELLTYPE *il, ui32 gdm)  {

   struct CONNTYPE *ix;       /* Ptr to current connection type */
   ulng lcld = 0;             /* Length of cell-level data returned */
   ui32 ilsu;                 /* Flags for cell & connection items */
   ui32 lsvgl;                /* Global svitms codes */
   ui32 lsuct,lsucn;          /* CELLTYPE and CONNTYPE suitms */
   ui32 svand;                /* Temp for GFAFF,GFRAW,GFNUK tests */

/* It is necessary to determine before starting whether any selectors
*  will be written for this segment, because, if not, it will be best
*  to skip writing the SEGMENT and LENGTH records, too.  This routine
*  should never be called from main if pgds was never set.  */

   lsvgl = RP0->pgds->svitms;    /* Global svitms codes */

/* In order to simplify run-time testing, turn off suitms request bits
*  for items that do not exist.  However, items Izha-d,Lij,Cij,Dij,Sj
*  can be calculated on-the-fly even if not saved (but should not be
*  output if never used at all).  Items AK and RAW, and PHD when there
*  is phasing, are always calculated but stored in prd data only when
*  needed for SIMDATA output or bar plots (svand will indicate if not
*  saved for any conntype).  Note also that it would be possible to
*  calculate an item solely because it is requested in svitms (SBAR
*  might be a candidate).  Results are saved in suitms at all levels
*  to avoid need to repeat these tests on every output cycle.  */

   lsuct = il->svitms;
   il->suitms = 0;
   /* We have to skip the celltype if Si not recorded, because
   *  there needs to be a cell list for the other variables.  */
   if (!(lsuct & GFSI)) return 0;
   if (!il->nizvar)              lsuct &= ~GFABCD;
   if (!ctexists(il,SBAR))       lsuct &= ~GFSBAR;
   if (!ctexists(il,QBAR))       lsuct &= ~GFQBAR;
   if (!il->phshft)              lsuct &= ~(GFPHI|GFDIS);
   if (!ctexists(il,IZU))        lsuct &= ~GFIFUW;
   if (!il->kautu)               lsuct &= ~GFASM;
   il->suitms = lsuct &
      (GFABCD|GFSI|GFGLBL|GFASM|GFIFUW|GFDIS|GFPHI|GFQBAR|GFSBAR|GFVMP);
   for (ix=il->pct1; ix; ix=ix->pct) {
      lsucn = ix->svitms;
      if (ix->kgen & KGNHV)      lsucn &= ~GFLIJ;
      if (!cnexists(ix,CIJ0))    lsucn &= ~GFCIJ0;
      if (ix->Cn.kdelay == 0)    lsucn &= ~GFDIJ;
      if (!cnexists(ix,MIJ))     lsucn &= ~GFMIJ;
      if (!cnexists(ix,PPF))     lsucn &= ~GFPPF;
      lsucn &=
         (GFRAW|GFAFF|GFNUK|GFLIJ|GFCIJ|GFCIJ0|GFSJ|GFDIJ|GFMIJ|GFPPF);
      ix->suitms = lsucn;
      il->suitms |= lsucn;       /* If empty, kills loop below */
      } /* End CONNTYPE loop */

   /* Exit if nothing in this segment */
   if (!(ilsu = il->suitms & gdm)) return 0;    /* Asgn intended */

   /* Locate cell list.  If GFGLBL bit is set, use all cells, even if
   *  there is an explicit list.
   *  R72, 02/22/17, GNR - Removed error 498 here if no list found--
   *     instead, just pretend GFGLBL was set.  */
   if (ilsu & GFGLBL)
      il->nsdcan = il->nelt;
   else {
      struct CLBLK *pclb = il->psdclb;
      if (pclb)
         il->nsdcan = ilstitct(pclb->pclil, (long)il->nelt);
      else {
         il->suitms |= GFGLBL;   /* JIC */
         il->nsdcan = il->nelt;
         }
      }

/*---------------------------------------------------------------------*
*  Selectors for STATE, PHASE, IFUW, VM, SBAR, QBAR, PHIDIST, and      *
*     Izhikevich a,b,c,d (only if variable) in that order.             *
*                                                                      *
*     It is assumed here that all the cells requested do in fact       *
*     exist.  This is assured by d3clck().                             *
*---------------------------------------------------------------------*/

   /* State, Izhu, phase, Vm, sbar, qbar, phase distribution will be
   *  written only if requested and they exist.  No error if they do
   *  not exist.  Note that lcld will get multiplied by ncells later. */
   if (ilsu & GFSI)     lcld += FMSSIZE;
   if (ilsu & GFPHI)    lcld += FMBSIZE;
   if (ilsu & GFIFUW)   lcld += FMJSIZE;
   if (ilsu & GFVMP)    lcld += FMJSIZE;
   if (ilsu & GFSBAR)   lcld += FMSSIZE;
   if (ilsu & GFQBAR)   lcld += FMSSIZE;
   if (ilsu & GFDIS)    lcld += PHASE_RES*FMJSIZE;
   if (ilsu & GFABCD) {
      /* il->prfi must exist or GFABCD bit will be off */
      struct IZHICOM *piz = (struct IZHICOM *)il->prfi;
      if (piz->izra)    lcld += FMJSIZE;
      if (piz->izrb)    lcld += FMJSIZE;
      if (piz->izrc)    lcld += FMJSIZE;
      if (piz->izrd)    lcld += FMJSIZE;
      }

/*---------------------------------------------------------------------*
* Selectors for AK, RAW (afference) and NUK (number connections used)  *
*---------------------------------------------------------------------*/

   /* Quietly omit this whole section if nothing was requested.
   *  Logic above leaves ilsu clear if there are no conntypes.  */
   ilsu &=
      (GFRAW|GFAFF|GFNUK|GFLIJ|GFCIJ|GFCIJ0|GFSJ|GFDIJ|GFMIJ|GFPPF);
   if (!ilsu) goto NO_CONNTYPE_VARIABLES;

   /* If the only variables to be written out for all connection types
   *  are AK and/or RAW and/or NUK, do shortcut accounting here.  */
   svand = (GFRAW|GFAFF|GFNUK);
   for (ix=il->pct1; ix; ix=ix->pct) svand &= ix->svitms;
   if ((ilsu ^ svand) == 0) {
      if (ilsu & GFAFF) lcld += FMJSIZE*il->nct;
      if (ilsu & GFRAW) lcld += FMJSIZE*il->nct;
      if (ilsu & GFNUK) lcld += FMJSIZE*il->nct;
      }

   /* Normal counting--check all connection-level variables  */
   else for (ix=il->pct1; ix; ix=ix->pct) {
      ui32 ixsu = ix->suitms & gdm;
      ui16 lsd1 = 0;
      if (!ixsu) continue;
      if (ixsu & GFAFF) lcld += FMJSIZE;
      if (ixsu & GFRAW) lcld += FMJSIZE;
      if (ixsu & GFNUK) lcld += FMJSIZE;

/*---------------------------------------------------------------------*
*  Selectors for LIJ, CIJ, CIJ0, SJ, MIJ, DIJ, PPFIJ                   *
*---------------------------------------------------------------------*/

      if ((ixsu & (GFLIJ|GFCIJ|GFCIJ0|GFSJ|GFMIJ|GFDIJ|GFPPF)) == 0)
         continue;
      /* Note order is Lij first, matching d3gfsv order */
      if (ixsu & GFLIJ)  lsd1 += FMJSIZE, lcld += FMJSIZE*ix->nc;
      if (ixsu & GFCIJ)  lsd1 += FMSSIZE, lcld += FMSSIZE*ix->nc;
      if (ixsu & GFCIJ0) lsd1 += FMSSIZE, lcld += FMSSIZE*ix->nc;
      if (ixsu & GFSJ)   lsd1 += FMSSIZE, lcld += FMSSIZE*ix->nc;
      if (ixsu & GFMIJ)  lsd1 += FMSSIZE, lcld += FMSSIZE*ix->nc;
      if (ixsu & GFDIJ)  lsd1 += FMBSIZE, lcld += FMBSIZE*ix->nc;
      if (ixsu & GFPPF)  lsd1 += FMSSIZE, lcld += FMSSIZE*ix->nc;
      /* Save length of simdata for one connection to d3gvsvct */
      if (gdm & (GFLIJ|GFDIJ)) ix->lsd1n0 = lsd1;
      else                     ix->lsd1n  = lsd1;
      } /* End loop over connection types */

NO_CONNTYPE_VARIABLES:
   if (lcld > (ulng)UI32_MAX) d3exit(fmturlnm(il), GFSVRL_ERR, 0);
   return (ui32)lcld;
   } /* End d3gfrl() */
