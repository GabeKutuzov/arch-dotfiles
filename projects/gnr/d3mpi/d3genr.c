/* (c) Copyright 1988-2018, The Rockefeller University *21115* */
/* $Id: d3genr.c 77 2018-03-15 21:08:14Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                               d3genr                                 *
*                                                                      *
*  Generate repertoires.  For all celltypes in all repertoires,        *
*     generate or read all numeric variables needed to initialize      *
*     the repertoires.  This includes reading saved repertoire files   *
*     and external connection lists.                                   *
*                                                                      *
************************************************************************
*  Written by G. N. Reeke                                              *
*  V4A, 09/16/88, SCD - Translated to C from version V3F               *
*  Rev, 11/09/89, JWT - Wallace code removed                           *
*  Rev, 01/04/90, JMS - Fixed il->group(x,y) calculation               *
*  Rev, 05/01/90, JMS - Fix in wgrpad calc for J conntypes             *
*  Rev, 02/01/91,  MC - Include phase in alloc calculation             *
*  Rev, 03/29/91,  MC - Add INOISE - initial noise generation          *
*  V5A, 04/29/91, GNR - Fix KGEN=L setup for general CHANGES           *
*  Rev, 06/10/91,  MC - Changes for MATRIX FREEZE option               *
*  V5C, 11/18/91, GNR - Add D1, reorganize, add multifile restore      *
*  V5E, 07/12/92, GNR - Revise to reenable GCONN's in serial version   *
*  Rev, 07/15/92, GNR - Fix UPWITH linking bug, move code to d3tchk    *
*  Rev, 07/23/92, GNR - Fix longstanding bug in rounding |Cij| > 1.0   *
*  Rev, 07/23/92, GNR, KJF - Add code to read CONNLIST files           *
*  Rev, 07/26/92, GNR - Move srcorg setting to d3snsa--must be node 0  *
*  Rev, 07/27/92, GNR - Revise motor map gen, fix PAR init angle bug   *
*  Rev, 08/01/92, GNR - Add support for decay KIND=B                   *
*  Rev, 08/15/92, GNR - More efficient SKIP_TYP, init phase w/phiseed  *
*  Rev, 08/26/92, GNR - Fix the erroneous fix of 7/23/92               *
*  Rev, 09/10/92, GNR - Correct mapping constants for hand vision      *
*  Rev, 10/11/92, GNR - Add KGEN=H setup and wgrpad updating           *
*  V6A, 03/02/93, GNR - Add subarbors, correct lseed update            *
*  V6D, 02/01/94, GNR - Add DELAY, ni1,ni2 args to initsi              *
*  V7B, 06/22/94, GNR - Save skipped connection info in Cij field      *
*  Rev, 08/20/94, GNR - Add support for bilinear Cij maps              *
*  V7C, 01/31/95, GNR - Add support for KGEN=Q square annulus rule     *
*  V8A, 04/13/96, GNR - Remove COMPAT=H,P, call d3inhs()               *
*  Rev, 04/26/97, GNR - Set to always condense connection data         *
*  Rev, 09/28/97, GNR - Independent dynamic allocations per conntype   *
*  Rev, 01/01/98, GNR - Norm tables to d3inhi, beta scaling to d3tchk  *
*  Rev, 02/23/98, GNR - Add resting potential, d3noise, s24tous8       *
*  Rev, 03/13/98, GNR - Add d3rpsi call so user can review initial si  *
*  Rev, 07/30/00, GNR - Use xoff,yoff with X,J rules                   *
*  Rev, 11/24/00, GNR - Separate RADIUS parameter, HV in IA coords     *
*  V8C, 02/25/03, GNR - Cell responses in millivolts, add conductances *
*  Rev, 07/01/06, GNR - Move Lij, Cij, Dij generation to new routines  *
*  ==>, 12/29/07, GNR - Last mod before committing to svn repository   *
*  Rev, 04/24/08, GNR - Remove MOPABS, add mtlo                        *
*  V8E, 01/20/09, GNR - Add Izhikevich variables, PHASEDEF phase info  *
*  Rev, 09/07/09, GNR - Add Brette-Gerstner (aEIF) cells               *
*  V8F, 06/04/10, GNR - Add KGNE2                                      *
*  Rev, 12/07/12, GNR - Convert ns[xy], ku[xy] to ui16, nst to ui32    *
*  Rev, 12/24/12, GNR - New overflow checking                          *
*  R72, 02/07/17, GNR - Add KGNKN compute geometric description tables *
*                       Remove KGNDG setup                             *
*  R73, 04/18/17, GNR - Convert KGNAN to circular or ellipsoidal areas *
*  R74, 06/29/17, GNR - Store Lija.psfcst cos/sin table for filters    *
*  R78, 03/28/18, GNR - Bug fix, alloc (nel/2+1) for cijfrac tables    *
*  R78, 07/19/18, GNR - Add ibias, but noise only if NFL_ADDV          *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "sysdef.h"
#include "rkarith.h"
#include "d3global.h"
#include "lijarg.h"
#include "wdwdef.h"
#include "d3fdef.h"
#include "celldata.h"
#include "rocks.h"

extern struct LIJARG Lija;
extern struct CELLDATA CDAT;

/* Function prototypes not in headers--unique to d3genr() */
void elijclos(void);
#ifdef PARn
void elijopnn(void);
#else
void elijopen(struct CELLTYPE *il, struct RFdef *fptr);
#endif

/*=====================================================================*
*                               d3genr                                 *
*=====================================================================*/

void d3genr(void) {

   struct REPBLOCK *ir;
   struct CELLTYPE *il;
   struct CONNTYPE *ix;
#ifndef PARn
   struct RFdef  *pelf;    /* Ptr to external Lij file */
#endif

#ifndef PAR0
   struct IZHICOM *piz;
   void (*resetu)(struct CELLTYPE *il, ui32 icell);

   rd_type *plsd;          /* Ptr to "ls" data for current cell */

   si32  lzseed;           /* Local izseed */
   int   icell;            /* Cell counter */
   int   kcijstor;         /* TRUE if storing Cij */
   int   kdijstor;         /* TRUE if storing Dij */
   int   klijstor;         /* TRUE if storing Lij */
   int   offCij;           /* Offset to Cij in prd data */
   int   offCij0;          /* Offset to Cij0 in prd data */
   int   offDij;           /* Offset to Dij in prd data */
   int   offLij;           /* Offset to Lij in prd data */
#endif
   int   doelij;           /* TRUE to read CONNLIST file(s) */

/*---------------------------------------------------------------------*
*                       d3genr executable code                         *
*---------------------------------------------------------------------*/

#ifndef PARn
   /* Pick up pointer to first external Lij RFdef */
   pelf = d3file[CONNLIST];
#endif

/*---------------------------------------------------------------------*
*                        Initialize constants                          *
*     Space allocation and items that are different on different       *
*  nodes (and therefore must be recomputed each time high memory is    *
*  broadcast) moved to d3nset, 11/22/91, GNR.  Items that must be      *
*  reinitialized each time generation occurs moved to d3kiji().        *
*     Note for the future:  Any variables currently initialized here   *
*  that are the same on all nodes and cannot be affected by Group III  *
*  control cards or regeneration could be moved to d3tchk.             *
*---------------------------------------------------------------------*/

   /* Loop over all repertoires/regions */
   for (ir=RP->tree; ir; ir=ir->pnrp) {

/* Initialization of working variables in repblock
*     (At present, there aren't any).  */

      /* Loop over all celltypes (layers) */
      for (il=ir->play1; il; il=il->play) {
#ifndef PAR0
         size_t llel = uw2zd(il->lel); /* Data length for one cell */
#endif
         doelij = FALSE;
         CDAT.cdlayr = il;

/* ----- Start of what was GENPASS1 in assembler version ----- */

/*---------------------------------------------------------------------*
*          Loop over all connection types for this cell type           *
*         and initialize option-specific conntype variables.           *
*======================================================================*
*  N.B.  Variables set here remain constant and do not need to be      *
*  recalculated after d3genr() time.  Although these variables are     *
*  not used on host node, code (except code that sets pointers) must   *
*  be executed on host so subsequent tree broadcasts preserve data.    *
*  Much of this code is candidate for moving to d3tchk.                *
*---------------------------------------------------------------------*/

         for (ix=il->pct1; ix; ix=ix->pct) {

/*---------------------------------------------------------------------*
*  V8D, 08/05/06, GNR - Cij generation setup here moved to d3tchk(),   *
*                       d3allo(), d3nset(), and d3kij().               *
*---------------------------------------------------------------------*/

/*---------------------------------------------------------------------*
*  R75, 09/22/17, GNR - Generate cijfrac tables for NOPVP.  This is    *
*                       unrelated to any kgen code, so placed here.    *
*  R76, 10/29/17, GNR - Make local ftainc so KGNTW stuff can go in ucij*
*---------------------------------------------------------------------*/

            if (ix->Cn.cnopt & NOPVP) {
               si32 *pfrac = ix->cijfrac;
               float ftainc = (float)PI*(float)180. /
                  (float)(ix->Cn.tcwid*il->nel);
               int jnel,jnele = (int)(il->nel/2 + 1);
               for (jnel=0; jnel<jnele; ++jnel) {
                  float cosarg = (float)jnel * ftainc;
                  /* cijfrac = 0.5 * (1 + cos)  (S24) */
                  *pfrac++ = (cosarg > (float)(0.999*PI)) ? 0 :
                     (si32)(dS23 * (1.0 + cosf(cosarg)));
                  }
               } /* End cijfrac construction */

/*---------------------------------------------------------------------*
*              Second letter code--first Lij generation                *
*---------------------------------------------------------------------*/

/* 3) Variables relating to 'F' (Floating), 'S' (Scanned),
*     or 'T' (Topographic) options--
*     If mapping is one-on-one, STM11 flag is set, causing same
*     Lij to be reused for all connections in one target group.  */

            if (ix->kgen & (KGNST+KGNFU+KGNTP)) {
               ix->ul1.s.sfx = ix->nux<<1;
               ix->ul1.s.sfy = ix->nuy<<1;
               ix->ul1.s.fstngx = ir->ngx;
               ix->ul1.s.fstngy = ir->ngy;
               /* Set source size for partitioned access */
               if (ix->kgen & KGNPT) {
                  ix->ul2.p.srcsiz = (long)ix->nux*(long)ix->nuy*
                     (long)ix->srcnel;
                  ix->ul2.p.iaxmsk = (1<<RP->kx) - 1; }
               /* Set STM11 if one-on-one and input is from IA */
               if (ix->kgfl & KGNIA && ix->kgen & KGNTP &&
                     (ix->nux==ir->ngx) && (ix->nuy==ir->ngy))
                  ix->cnflgs |= STM11;
               } /* End F,S,T setup */

/* 4) Variables relating to 'W' option--
*     Now reduced to hand vision only.  Test for old Wallace
*     removed 07/31/94, is already tested in g2conn.  Variables
*     in this block are not used in d3genr and are gotten by
*     comp nodes in d3go from bcst, hence the ifndef PARn.  */

#ifndef PARn
            else if (ix->kgen & KGNHV) {
               float width, height; /* Window dimensions */
               float rrad = dS16/(float)ix->hradius;

#ifdef BBDS
               if (ix->kgen & KGNBV) {
                  /* Object is located on a BBD or TV image */
                  width = (float)ix->nux;
                  height = (float)ix->nuy;
                  }
               else
#endif
               if (ix->loff > 0) {
                  /* Window size when it's an actual window */
                  struct WDWDEF *pw = RP0->ppwdw[ix->loff - 1];
                  width = pw->wwd;
                  height = pw->wht;
                  } /* End else */
               else {
                  /* Whole input array - not a window */
                  width = (float)RP->nsx;
                  height = (float)RP->nsy;
                  }

               ix->ul1.w.xwmx  = width;
               ix->ul1.w.ywmx  = height;
               ix->ul1.w.xrat  = width/(float)ir->ngx;
               ix->ul1.w.yrat  = height/(float)ir->ngy;
               ix->ul1.w.radm2 = rrad*rrad;
               } /* End setup for KGEN=W */
#endif

/* 5) Variables relating to 'J' option
*     N.B.  Input from IA is not allowed--checked earlier.
*     Rev, 08/12/00, GNR - Use all terms for VG/TV as well as Rep. */

            else if (ix->kgen & KGNJN) {
               ix->ul1.g.ogoff = (long)ix->loff +
                  (long)ix->xoff*(long)ix->srcnel +
                  (long)ix->yoff*(long)ix->srccpr;
               ix->ul1.g.srcnel1 = (long)ix->nux +
                  (long)ix->nuy*(long)ix->srccpr;
               /* Set source size for partitioned access */
               if (ix->kgen & KGNPT)
                  ix->ul2.p.srcsiz = (long)ix->lsg;
               } /* End setup for KGEN=J */

/* 6) Variables relating to 'G', '1', 'N', 'O', 'U' options
*     (group-oriented source division)  */

            else if (ix->kgen & (KGNGP|KGNG1|KGNND|KGNOG|KGNUD)) {
               /* srcnel1 = cells/group in source rep (all if IA or VG)
               *  srcnel2 = 2*number used for one target group.  */
               if (ix->kgfl & (KGNXR|KGNVG)) { /* VG can be U only */
                  ix->ul1.g.srcnel1 = (long)ix->srcnelt;
                  ix->ul1.g.srcnel2 = (long)ix->srcnelt<<1;
                  }
               else if (ix->kgen & KGNGP) {
                  ix->ul1.g.srcnel1 = (long)ix->srcnel;
                  ix->ul1.g.srcnel2 = (long)ix->srcnel<<1;
                  }
               else if (ix->kgen & KGNG1) {
                  ix->ul1.g.srcnel1 = (long)ix->srcnel;
                  ix->ul1.g.ogoff =
                     (long)ix->xoff*(long)ix->srcnel +
                     (long)ix->yoff*(long)ix->srccpr;
                  }
               else if (ix->kgen & KGNUD) {
                  ix->ul1.g.srcnel1 = (long)ix->srcnel;
                  ix->ul1.g.srcnel2 = (long)ix->srcnelt<<1;
                  }
               else if (ix->kgen & KGNOG) {
                  ix->ul1.g.srcnel1 = (long)ix->srcnel;
                  ix->ul1.g.srcnel2 = ((long)ix->srcnelt<<1) - ix->lsg;
                  /* ogoff = offset of first type 'O' connection
                  *  beyond wgrpad=(1/2)(srcnel+sigma) corrected
                  *  for fact that sigma is S1 */
                  ix->ul1.g.ogoff = (((long)ix->srcnel<<1)+ix->lsg)>>2;
                  }
#ifndef PARn
               else
                  d3exit(fmturlnm(il), LIJGEN_ERR, (int)ix->ict);
#endif
               /* Set source size for partitioned access */
               if (ix->kgen & KGNPT)
                  ix->ul2.p.srcsiz = ix->ul1.g.srcnel2 >> 1;
               } /* End setup for KGEN=G,1,N,O,U */

/* 7) Variables relating to 'H' option ("hypergroups") */

            else if (ix->kgen & KGNHG) {

               /* srcnel1 = total cells in one row of groups */
               ix->ul1.g.srcnel1 = (long)ix->srccpr;
               /* srcnel2 = 2*total cells in a hypergroup */
               ix->ul1.g.srcnel2 =
                  (ir->Rp.ngridx*ir->Rp.ngridy*(long)ix->srcnel)<<1;
               /* gridrow = total cells in one row of a hypergroup */
               ix->ul1.g.gridrow =
                  (long)ir->Rp.ngridx*(long)ix->srcnel;
               /* Set source size for partitioned access */
               if (ix->kgen & KGNPT)
                  ix->ul2.p.srcsiz = ix->ul1.g.srcnel2 >> 1;
               } /* End setup for KGEN=H */

/* 8) Variables relating to 'X', 'Y' (systematic) options.
*     Note:          KGNXO                KGNYO
*     ul1.x.incoff   all increments       within-group increment
*     ul1.x.grpinc   not used             new-group increment  */

            else if (ix->kgen & (KGNXO|KGNYO)) {
               if (ix->kgfl & KGNIA) {    /* IA */
                  /* Wraparound in yoff gives error in d3tchk,
                  *  so we don't need to worry about it here.  */
                  long colsz = (long)RP->nsy + (long)RP->nsy;
                  ix->ul1.x.fstoffx = (long)(ix->xoff + ix->loff)*colsz;
                  ix->ul1.x.fstoffy = (long)ix->yoff;
                  ix->ul1.x.incoffx = (long)ix->nux*colsz;
                  ix->ul1.x.incoffy = (long)ix->nuy;
                  }
               else {                     /* VG, TV, or Rep. */
                  ix->ul1.x.fstoffx =
                     (long)ix->xoff*(long)ix->srcnel + (long)ix->loff;
                  ix->ul1.x.fstoffy = (long)ix->yoff*(long)ix->srccpr;
                  ix->ul1.x.incoffx = (long)ix->nux;
                  ix->ul1.x.incoffy = (long)ix->nuy*(long)ix->srccpr;
                  }
               if (ix->kgen & KGNYO) {
                  ix->ul1.x.grpincx = (long)ix->nux * (long)ix->srcnel;
                  ix->ul1.x.grpincy = ix->ul1.x.incoffy;
                  ix->ul1.x.incoffx = (long)ix->lsg >> 1;
                  ix->ul1.x.incoffy = 0;
                  }
               }  /* End setup for KGEN=X,Y */

/* 9) Variables relating to 'E' (external) option.
*     If not overriding a RESTORE file, set doelij so file
*     will be opened (must be done after d3fchk() has run).
*     Note:  IBM version used srcnel2 as substitute srcnelt so wrap
*     test in ecl1a (now called d3elij) could be nullified when VG
*     input and delayed to d3go.  This is no longer necessary because
*     number of VG inputs is now deterministic.  -GNR  */

            else if (ix->kgen & KGNEX) {
               if (ix->Cn.cnopt & NOPOR) doelij = TRUE;
               } /* End setup for KGEN=E or 2 */

/*---------------------------------------------------------------------*
*            Third letter code--subsequent Lij generation              *
*---------------------------------------------------------------------*/

/* 10) Variables relating to 'A' (adjacent) option.  */

            if (ix->kgen & KGNAJ) {
               if (ix->kgfl & KGNIA) { /* Input from IA */
                  ix->ul2.a.incr = (long)ix->stride << RP->ky1;
                  ix->ul2.a.lim = ((long)ix->srcnelt<<1) - 1;
                  }
               else {         /* Input from Rep or VG */
                  ix->ul2.a.incr = (long)ix->stride;
                  ix->ul2.a.lim = (long)ix->srcnelt - 1;
                  }
               }  /* End setup for KGEN=A */

/* 11) Variables relating to 'B' ("box") option */

            else if (ix->kgen & KGNBL) {
               if (ix->kgfl & KGNXR) {    /* Not repertoire */
                  ix->ul2.b.rrlen = (long)ix->srcngx;
                  ix->ul2.b.rtlen = -((long)ix->srcngx)*(long)ix->nry;
                  }
               else {                     /* Repertoire */
                  ix->ul2.b.rrlen = (long)ix->srccpr;
                  ix->ul2.b.rtlen= -((long)ix->srcnel)*
                     (long)ix->srcngx*(long)ix->nry;
                  }
               ix->ul2.b.rslen = (long)ix->stride*(long)ix->nrx;
               }  /* End setup for KGEN=B */

/* 12) Variables relating to 'C' ('Crow's foot') option
*      No setup currently required - uses srcngx, etc. */

/* 13) Variables relating to 'D' ('Diagonal') option -- R72 removed */

/* 14) Variables relating to 'K' (Kernel) option--
*     kkern tables just hold information on the range of connections
*     that need to be generated for each row above or below the
*     target cell.  Although this is the same on all nodes, it is
*     done here rather than in d3tchk() to save broadcast overhead.  */

            else if (ix->kgen & KGNKN) {
#ifdef PAR0
            if (ix->knopt & KNO_PRINT &&
                  RP->CP.runflags & RP_OKPRIN)
#endif
               d3lkni(ix, NULL);
               } /* End setup for KGEN=K */

/* 15) Variables relating to 'Q' ('Annulus with hole') option.  As
*     with 'K' option, identical tables are generated here on all
*     nodes to save broadcast overhead.  */

#ifndef PAR0
            else if (ix->kgen & KGNAN) {
               d3lani(ix, NULL);
               } /* End setup for KGEN=Q */
#endif

            }  /* End loop over ix */

/* ------- End of what was GENPASS1 in assembler version ------- */

/*---------------------------------------------------------------------*
*                Actually generate the repertoire data                 *
*---------------------------------------------------------------------*/

/* There are 4 major cases:
*     1) Doing a replay--skip generation if KSVNR set.
*     2) Restoring in d3rstr from a previously saved repertoire.
*        Code here must be able to fill in ls or prd data or both.
*     3) Generating ab initio.
*     4) Regenerating at end of trial series--see d3regenr().  */

         if (RP->ksv & KSVNR) continue;

/* Generate s(i) data.  (N.B.  initsi is called from d3rstr if
*  restoring and phases or extra delay rows must be initialized.  */

         if (il->Ct.ctopt & (OPTOR|OPTNR))
            initsi(il, 0, il->mxsdelay1);

/* Print initial s(i) if requested */

         if (il->Ct.kctp & KRPEP && RP->CP.runflags & RP_OKPRIN)
            d3rpsi(il, -1);

#ifndef PAR0

/* Initialize real modulation sums for this cell type based on
*  starting values of s(i).  These sums are not needed on host.
*  (Note: the actual summation must be done rather than using
*     nmn,nfr so is correct when starting from a saved net,
*     also for compatibility with earlier versions.)  */

         sumrmod(il,1);
#endif                        /* End host exclusion */

/* Prepare to generate cell and connection data */

         /* If nothing to generate, forget it */
         if (!il->lel) continue;

/* If any conntypes have external Lij's & NOPOR, open Lij file.
*  On PAR0, elijopen() also packetizes data to comp nodes. */

         if (doelij) {
#ifdef PARn
            elijopnn();
#else
            if (pelf) elijopen(il, pelf);
            else      d3exit(NULL, LIJXF_ERR, 0);
#endif
            } /* End opening external Lij files */

/*---------------------------------------------------------------------*
*                           Loop over cells                            *
*---------------------------------------------------------------------*/

#ifndef PAR0
/*** BEGIN NODE ZERO EXCLUSION FROM LOOP OVER RESIDENT CELLS ***/

         /* N.B. d3allo() guarantees that plsd points
         *  to an appropriate storage boundary */
         plsd = il->prd;
         piz  = (struct IZHICOM *)il->prfi;
         /* Seed for generating individual Izhikevich a,b,c,d */
         if (il->nizvar) {
            lzseed = piz->izseed;
#ifdef PAR
            udevwskp(&lzseed, jmsw(il->nizvar,il->locell));
#endif
            }
         /* Which resetu routine to use */
         resetu = (il->Ct.rspmeth == RF_IZH7) ? resetu7 : resetu3;

         for (icell=il->locell; icell<il->hicell; ++icell,plsd+=llel) {

            /* Establish group-level, etc. variables for this cell */
            d3kiji(il, icell);

/* If cell-level data not restored, zero them now */

            if ((il->Ct.ctopt & OPTOR) && il->ls)
               memset((char *)plsd, 0, il->ls);

/* Generate Brette-Gerstner (aEIF) variables:  32-bit v and w only.  */

            if (il->Ct.rspmeth == RF_BREG) resetw(il, icell);

/* Generate Izhikevich variables:  a,b,c,d randomizations and u.
*  As per Izhikevich (2003), the equilibrium value of u is b*v.
*  N.B.  We apply randomizations as positive or negative offsets
*  to mean values of a,b,c,d--published code always adds them.
*
*  Note:  In the Izhikevich 2003 model, the Vm in the equations is
*  absolute, not relative to Vrest.  And Vrest is a function of
*  cv2,cv1,cv0, and b.  When b is variable, every cell has its own
*  Vrest.  This is computed in d3nset() by a call to d3i3avr()
*  before d3genr() can call resetu3() because new values may be
*  needed after d3chng().    */

            else if (il->Ct.rspmeth >= RF_IZH3) {
               if (ctexists(il, IZRA)) {
                  rd_type *plzd = plsd + il->ctoff[IZRA];
                  si32 tt;
                  if (piz->izra) {
                     tt = udev(&lzseed) << 1;
                     tt = mrssl(tt, (si32)piz->izra, FBrf);
                     d3ptjl2a(tt, plzd); plzd += HSIZE; }
                  if (piz->izrb) {
                     tt = udev(&lzseed) << 1;
                     tt = mrssl(tt, (si32)piz->izrb, FBrf);
                     d3ptjl2a(tt, plzd); plzd += HSIZE; }
                  if (piz->izrc) {
                     tt = udev(&lzseed) << 1;
                     tt = mrssl(tt, abs32(tt), FBrf);
                     tt = mrssl(tt, (si32)piz->izrc, FBrf);
                     d3ptjl2a(tt, plzd); plzd += HSIZE; }
                  if (piz->izrd) {
                     tt = udev(&lzseed) << 1;
                     tt = mrssl(tt, abs32(tt), FBrf);
                     tt = mrssl(tt, (si32)piz->izrd, FBrf);
                     d3ptjl2a(tt, plzd); }
                  }
               resetu(il, icell);
               } /* End rspmeth >= RF_IZH3 */

/* Loop over connection types */

            for (Lija.jctype=0,ix=il->pct1; ix; ix=ix->pct) {
               rd_type *pnuk;    /* Will be ptr to stored nuk */
               long  nuk;        /* Number of connections generated */

               /* Count only external conntypes in Lija.jctype */
               if (ix->kgen & KGNEX) Lija.jctype++;

               /* If conntype was restored from a file,
               *  skip generating now. */
               if (!(ix->Cn.cnopt & NOPOR)) continue;

               /* Set Lij, Cij, and Dij generation variables that
               *  depend on connection type, cell or group number.  */
               d3kijx(ix);
               d3lijx(ix);
               d3cijx(ix);
               d3dijx(ix);
               offCij  = ix->cnoff[CIJ];
               offCij0 = ix->cnoff[CIJ0];
               offDij  = ix->cnoff[DIJ];
               offLij  = ix->cnoff[LIJ];

/*---------------------------------------------------------------------*
*                         Loop over synapses                           *
*---------------------------------------------------------------------*/

               kcijstor = cnexists(ix, CIJ);
               kdijstor = cnexists(ix, DIJ);
               klijstor = cnexists(ix, LIJ);
               for (Lija.isyn=0; Lija.isyn<(long)ix->nc; ++Lija.isyn) {

/* Generate an Lij coefficient and store it.
*  Exit if premature end of list signalled.
*  (Note:  Lij are generated whether saved or not, used for list
*  condensation, motor-map source location, subarbor skip list) */

                  if (!Lija.lijbr(ix)) break;
                  if (klijstor) d3ptln(Lija.lijval,
                     Lija.psyn+offLij, ix->lijlen);

/* Generate a Cij coefficient unless calculating each time used
*  and save a copy of Cij in Cij0 if requested (decay KIND=B or
*  RESET SAVECIJ0 active).
*  Note:  If Lij generation skips one or more connections, the
*  cijbr() routine will do the bookkeeping to keep generation
*  in step with Lija.jsyn without returning a value.  */

                  if (kcijstor) {
                     ix->cijbr(ix);
                     d3pthn(Lija.cijsgn, Lija.psyn+offCij, ix->cijlen);
                     if (cnexists(ix,CIJ0)) d3pthn(Lija.cijsgn,
                        Lija.psyn+offCij0, ix->cijlen);
                     }

/* Generate delay.  Constant delay is not stored in the data
*  structure, but is retrieved when needed from ix->Cn.mxcdelay */

                  if (cnexists(ix, DIJ)) {
                     ix->dijbr(ix);
                     d3ptjl1(Lija.dijval, Lija.psyn+offDij);
                     }

                  Lija.psyn += ix->lc;
                  } /* End of loop over synapses */

/* Here when all synapses of current type have been processed.
*  Save number of connections actually generated.
*  (This value is now unconditionally saved.)  */

               nuk = Lija.isyn;
               pnuk = Lija.pxnd + ix->xnoff[NUK];
               d3ptln(nuk, pnuk, ix->nuklen);

               }  /* End of loop over connection types */

            } /* End of loop over cells */

#endif   /* END NODE 0 EXCLUSION FROM CELL LOOP */

         /* If there are any external connections, close current file */
         if (doelij) {
            elijclos();
#ifndef PARn
            pelf = pelf->nf;
#endif
            }

         }  /* End of loop over cell types */

#ifndef PAR0
      /* Initialize ion concentrations */
      inition(ir, 0);
#endif

      }  /* End of loop over repertoires/regions */

/* If there are any connections with steerable filter input (tuqnel
*  will be nonzero), make a table of cosines and sines for nel
*  fractions of 180 degs. for all unique values of nel.  Scale S16
*  is used so filtered pixel (S14) * trig (S16) => S30 and squared
*  it still fits in a 64-bit long.  */

   if (RP->tuqnel) {
      struct CELLTYPE *jl;
      si32 *psc,*psc0;
      psc0 = (si32 *)mallocv(RP->tuqnel*JSIZE, "PP_STGH sfcst");
#ifdef PAR0
RP0->psfcst = psc0;
#else
Lija.psfcst = psc0;
#endif
      for (jl=RP->pfct; jl && jl->sfnel; jl=jl->pnct) {
         double ang, dang = PI/(double)jl->sfnel;
         psc = psc0 + jl->sfoff;
         for (ang=0; ang<0.999*PI; ang+=dang) {
            *psc++ = (si32)(dS16*cos(ang));
            *psc++ = (si32)(dS16*sin(ang));
            }
         }
      }  /* End making psfcst table */

   }   /* End of d3genr() */


/*=====================================================================*
*                               inition                                *
*                                                                      *
*  This routine initializes values of cell-internal and external ion   *
*  concentrations.  It may be called at initial generation (d3genr)    *
*  or reset (d3resetn) time, i.e. at the same times that initsi()      *
*  might be called.  However, due to the special situation of group-   *
*  level Cext variables, it must be called per REPBLOCK, not per       *
*  CELLTYPE.                                                           *
*                                                                      *
*  Arguments:                                                          *
*     ir       Ptr to repblock whose ions are to be initialized.       *
*     bypass   Mask to be checked against region or celltype options   *
*              (0==> always reset, KRPBR==>obey bypass option switch). *
*=====================================================================*/

#ifndef PAR0
void inition(struct REPBLOCK *ir, ui16 bypass) {

   struct CELLTYPE *il;
   struct IONTYPE  *pion;
   rd_type *plid,*plie;       /* Ptrs to layer ion data */
   rd_type *pri0,*prid,*prie; /* Ptrs to region ion data */
   size_t llel;               /* Length of data for one cell */
   size_t lmvi;               /* Length of data to copy */
   int    llg = ir->lg;

/* If not bypassing at region level, reset region-level ions */

   if (ir->andctopt & bypass) return;
   pri0 = ir->pgpd + ir->gpoff[IONG];
   prie = ir->pgpd + ir->lgt;
   for (pion=ir->pionr; pion; pion=pion->pnion) {
      /* Set internal (and individual external if it exists)
      *  ion concentration for each individual cell */
      lmvi = ESIZE;
      if (pion->ionflgs & (CDOP_IONEXT|CDOP_IONREV)) {
         /* Set external conc if region or group */
         if (pion->ionopts & ION_REGION)
            pion->Cext = pion->C0ext;
         else if (pion->ionopts & ION_GROUP) {
            for (prid=pri0; prid<prie; prid+=llg)
               memcpy((char *)prid, (char *)&pion->C0ext, lmvi);
            pri0 += lmvi; }
         else                 /* Just copy with internal conc */
            lmvi += lmvi;
         } /* End checking external update mode */
      /* N.B.  Do not check CELLTYPE bypass for region-level ions */
      for (il=ir->play1; il; il=il->play) {
         llel = uw2zd(il->lel);
         plid = il->prd + il->ctoff[IONC] + pion->oCi;
         plie = il->prd + uw2zd(il->llt);
         for ( ; plid<plie; plid+=llel)
            memcpy((char *)plid, (char *)&pion->C0int, lmvi);
         } /* End loop over CELLTYPEs in this region */
      } /* End region-level IONTYPE loop */

/* If not bypassing at celltype level, reset celltype-level ions */

   for (il=ir->play1; il; il=il->play) {
      llel = uw2zd(il->lel);
      if (il->Ct.ctopt & bypass) continue;
      for (pion=il->pion1; pion; pion=pion->pnion) {
         /* Set internal (and individual external if it exists)
         *  ion concentration for each individual cell */
         lmvi = ESIZE;
         if (pion->ionflgs & (CDOP_IONEXT|CDOP_IONREV)) {
            /* Set external conc if region or group */
            if (pion->ionopts & ION_REGION)
               pion->Cext = pion->C0ext;
            else if (pion->ionopts & ION_GROUP) {
               for (prid=pri0; prid<prie; prid+=llg)
                  memcpy((char *)prid, (char *)&pion->C0ext, lmvi);
               pri0 += lmvi;
               }
            else                 /* Just copy with internal conc */
               lmvi += lmvi;
            } /* End checking external update mode */
         plid = il->prd + il->ctoff[IONC] + pion->oCi;
         plie = il->prd + uw2zd(il->llt);
         for ( ; plid<plie; plid+=llel)
            memcpy((char *)plid, (char *)&pion->C0int, lmvi);
         } /* End CELLTYPE-level IONTYPE loop */
      } /* End CELLTYPE loop */

   } /* End inition() */
#endif


/*=====================================================================*
*                               initsi                                 *
*                                                                      *
*  This routine initializes s(i) and phase values to zero or noise as  *
*  specified in CELLTYPE.  Note that if s(i) is at resting potential,  *
*  phase has no meaning, and, for compatibility with old versions, it  *
*  is set to zero and the phase seed is not advanced.                  *
*                                                                      *
*  initsi() must be called in parallel from all nodes: On host the     *
*  values are updated for use in rpsi print.                           *
*  The logic is quite subtle--See Design Notes, p. 43.                 *
*  Arguments:                                                          *
*     il       Ptr to CELLTYPE for which initialization to be done.    *
*     ni1      Number of delay rows initialized with phase only.       *
*              This condition occurs in d3rstr when s(i) is read in    *
*              from a SAVENET file but phase is newly generated.       *
*              Caller should assure ni1 == zero if no phases.          *
*     ni2      Number of delay rows initialized with s(i) and phase.   *
*              This condition occurs in d3genr, d3reset, and d3rstr    *
*              when a full set of s(i),phase values must be generated. *
*  Affected rows will end with the last row in the pps array.          *
*                                                                      *
*  Rev, 08/16/92, GNR - Use phiseed to set phi, unless const|COMPAT=P  *
*  Rev, 02/07/94, GNR - ni1,ni2 allow both kinds of init in one call   *
*  Rev, 04/13/96, GNR - Compatibility with original use of nsd removed *
*  Rev, 03/05/98, GNR - Add resting potential (V8C: only w/COMPAT=C)   *
*  Rev, 10/15/05, GNR - Randomize phase even if s(i) == Vrest.         *
*  Rev, 01/03/07, GNR - Negative s(i) not allowed with old range.      *
*  Rev, 02/16/09, GNR - Remove addition of Vrest w/COMPAT=C            *
*  Rev, 08/25/14, GNR - Add CLEAREDSI flag                             *
*  R78, 07/11/18, GNR - Add ibias, revise check for range overflows    *                                     *
*=====================================================================*/

void initsi(struct CELLTYPE *il, int ni1, int ni2) {

   s_type *pc0,*pc,*pce;            /* Cell pointers and limit */
   struct PHASEDEF *ppd;            /* Ptr to PHASEDEF if exists */
   si32 bias;                       /* Constant addition (S7/14) */
   si32 hisi,losi;                  /* Test values for si */
   si32 nmnS7;                      /* Mean noise (S7) */
   si32 nsgS11;                     /* Noise sigma (S11) */
   int Noisy;                       /* TRUE to generate noise */
   int Phsupd;                      /* Method of phase generation */
   int idly;                        /* Delay counter */
   int irow1;                       /* First row for ni1 */
   int irow2;                       /* First row for ni2 */

   /* Sanity check to avoid overflowing s(i) array */
#ifndef PARn
   if ((ni1 + ni2) > (int)il->mxsdelay1)
      d3exit(fmturlnm(il), INITSI_ERR, 0);
#endif

   /* Calculate row limits for s(i) generation.  These limits
   *  assure idly loop omits rows that have no noise or phase.  */
   irow1 = (irow2 = il->mxsdelay1 - ni2) - ni1;

   /* Sanity check to avoid underflowing s(i) array */
#ifndef PARn
   if (irow1 < 0) d3exit(fmturlnm(il), INITSI_ERR, 0);
#endif

   bias = il->ibias;
   nmnS7 = SRA(il->No.nmn, FBwk-FBsi);
   nsgS11 = il->No.nsg >> (FBwk-FBsi);
   Noisy = (il->No.noikadd & (NFL_ADDR|NFL_INIT)) &&
      !(il->Ct.ctopt & OPTNIN);
   /* Overflow test values */
   if (RP->compat & OLDRANGE)
      hisi = S14, losi = 0;
   else
      hisi = S15-1, losi = -S15;

   /* Method of phase update: Phsupd >=0 constant, -1 random */
   if (il->phshft) {
      ppd = il->pctpd;
      Phsupd = Noisy ?
         ((ppd->phimeth == PHI_CONST) ? (int)ppd->fixpha : -1) : 0;
      }

   for (idly=irow1; idly<(int)il->mxsdelay1; idly++) {

/* N.B.  Must loop over all cells, not just those on my node.  */

      pc0 = il->pps[idly];                /* Set up cell loop */
      pce = pc0 + il->lspt;

      if (idly >= irow2) {                /* Generating s(i) here */
         if (Noisy) for (pc=pc0; pc<pce; pc+=il->lsp) {
            si32 wksi = bias +
               d3noise(nmnS7, nsgS11, il->No.nfr, il->No.nsd);
            if (wksi > hisi) wksi = hisi;
            else if (wksi < losi) wksi = losi;
            d3ptjl2(wksi, pc);
            }
         else if (bias) for (pc=pc0; pc<pce; pc+=il->lsp)
            d3ptjl2(bias, pc);            /* Just bias, no overflow */
         else {                           /* All cells zero at once */
            memset((char *)pc0, 0, il->lspt);
            CDAT.flags |= CLEAREDSI;
            }
         }  /* End generating s(i) */

      if (il->phshft) {                   /* Generating phase here */
         if (Phsupd >= 0) for (pc=pc0; pc<pce; pc+=il->lsp)
            pc[LSmem] = (s_type)Phsupd;      /* Fixed phase */
         else for (pc=pc0; pc<pce; pc+=il->lsp)
            pc[LSmem] = (s_type)(udev(&ppd->phiseed) & PHASE_MASK);
         } /* End generating phase */

      } /* End delay row loop */

   } /* End initsi */
