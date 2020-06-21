/* (c) Copyright 1988-2010 Neurosciences Research Foundation, Inc. */
/* $Id: d3genr.c 46 2011-05-20 20:14:09Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                               d3genr                                 *
*                                                                      *
*  Generate repertoires.  For all celltypes in all repertoires,        *
*     generate or read all numeric variables needed to initialize      *
*     the repertoires.  This includes reading saved repertoire files   *
*     and external connection lists.                                   *
*                                                                      *
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
***********************************************************************/

#define NEWARB          /* Use new EFFARB mechanism */
#define LIJTYPE struct LIJARG
#ifdef D1
#define D1TYPE  struct D1BLK
#endif

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "rkarith.h"
#include "d3global.h"
#include "wdwdef.h"
#include "d3fdef.h"
#ifdef D1
#include "d1def.h"
#endif
#ifndef PARn
#include "rocks.h"
#endif

extern struct LIJARG Lija;

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
   rd_type       *plsd;    /* Ptr to "prd" data for current cell */
   long  celllim;          /* Cell counter limit */
   long  icell;            /* Cell counter */
#ifndef PAR0
   struct IZHICOM *piz;

   si32  lzseed;           /* Local izseed */

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
         doelij = FALSE;

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

            ;

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
                  ix->ul2.p.srcsiz = ix->nux*ix->nuy*ix->srcnel;
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
               float rrad = dS16/(float)ix->radius;

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
               long rowsz = ix->srcnel * ix->srcngx;
               ix->ul1.g.ogoff = ix->loff + ix->xoff*ix->srcnel +
                  ix->yoff*rowsz;
               ix->ul1.g.srcnel1 = ix->nux + ix->nuy*rowsz;
               /* Set source size for partitioned access */
               if (ix->kgen & KGNPT)
                  ix->ul2.p.srcsiz = ix->lsg;
               } /* End setup for KGEN=J */

/* 6) Variables relating to 'G', 'N', 'O', 'U' options
*     (group-oriented source division)  */

            else if (ix->kgen & (KGNGP|KGNND|KGNOG|KGNUD)) {
               /* srcnel1 = cells/group in source rep (all if IA or VG)
               *  srcnel2 = 2*number used for one target group.  */
               if (ix->kgfl & (KGNXR|KGNVG)) { /* VG can be U only */
                  ix->ul1.g.srcnel1 = ix->srcnelt;
                  ix->ul1.g.srcnel2 = ix->srcnelt<<1;
                  }
               else if (ix->kgen & KGNGP) {
                  ix->ul1.g.srcnel1 = ix->srcnel;
                  ix->ul1.g.srcnel2 = ix->srcnel<<1;
                  }
               else if (ix->kgen & KGNUD) {
                  ix->ul1.g.srcnel1 = ix->srcnel;
                  ix->ul1.g.srcnel2 = ix->srcnelt<<1;
                  }
               else if (ix->kgen & KGNOG) {
                  ix->ul1.g.srcnel1 = ix->srcnel;
                  ix->ul1.g.srcnel2 = (ix->srcnelt<<1) - ix->lsg;
                  /* ogoff = offset of first type 'O' connection
                  *  beyond wgrpad=(1/2)(srcnel+sigma) corrected
                  *  for fact that sigma is S1 */
                  ix->ul1.g.ogoff = ((ix->srcnel<<1)+ix->lsg)>>2;
                  }
#ifndef PARn
               else
                  d3exit(LIJGEN_ERR, fmturlnm(il), (long)ix->ict);
#endif
               /* Set source size for partitioned access */
               if (ix->kgen & KGNPT)
                  ix->ul2.p.srcsiz = ix->ul1.g.srcnel2 >> 1;
               } /* End setup for KGEN=G,N,O,U */

/* 7) Variables relating to 'H' option ("hypergroups") */

            else if (ix->kgen & KGNHG) {

               /* srcnel1 = total cells in one row of groups */
               ix->ul1.g.srcnel1 = ix->srcngx*ix->srcnel;
               /* srcnel2 = 2*total cells in a hypergroup */
               ix->ul1.g.srcnel2 =
                  (ir->Rp.ngridx*ir->Rp.ngridy*ix->srcnel)<<1;
               /* gridrow = total cells in one row of a hypergroup */
               ix->ul1.g.gridrow = ir->Rp.ngridx*ix->srcnel;
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
                  long colsz = RP->nsy + RP->nsy;
                  ix->ul1.x.fstoff = colsz*ix->xoff +
                     ix->loff + ix->yoff;
                  ix->ul1.x.incoff = ix->nux*colsz + ix->nuy;
                  }
               else {                     /* VG, TV, or Rep. */
                  long rowsz = ix->srcnel * ix->srcngx;
                  ix->ul1.x.fstoff = ix->loff + ix->xoff*ix->srcnel +
                     ix->yoff*rowsz;
                  ix->ul1.x.incoff = ix->nux + ix->nuy*rowsz;
                  }
               if (ix->kgen & KGNYO) {
                  ix->ul1.x.grpinc = ix->ul1.x.incoff;
                  ix->ul1.x.incoff = ix->lsg >> 1;
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
                  ix->ul2.a.incr = ix->stride << RP->ky1;
                  ix->ul2.a.lim = (ix->srcnelt<<1) - 1;
                  }
               else {         /* Input from Rep or VG */
                  ix->ul2.a.incr = ix->stride;
                  ix->ul2.a.lim = ix->srcnelt - 1;
                  }
               }  /* End setup for KGEN=A */

/* 11) Variables relating to 'B' ("box") option */

            else if (ix->kgen & KGNBL) {
               if (ix->kgfl & KGNXR) {    /* Not repertoire */
                  ix->ul2.b.rrlen = ix->srcngx;
                  ix->ul2.b.rtlen = (-ix->srcngx)*ix->nry;
                  }
               else {                     /* Repertoire */
                  ix->ul2.b.rrlen = ix->srcnel*ix->srcngx;
                  ix->ul2.b.rtlen=(-ix->srcnel)*ix->srcngx*ix->nry;
                  }
               ix->ul2.b.rslen = ix->stride*ix->nrx;
               }  /* End setup for KGEN=B */

/* 12) Variables relating to 'C' ('Crow's foot') option
*      No setup currently required - uses srcngx, etc. */

/* 13) Variables relating to 'D' ('Diagonal') option */

            else if (ix->kgen & KGNDG) {
               if (ix->kgfl & KGNXR) {
                  if (ix->kgfl & (KGNVG|KGNTV)) {  /* VG or TV */
                     ix->ul2.d.rowsz = ix->srcngx;
                     ix->ul2.d.xincr =
                        ix->ul2.d.rowsz*ix->nvx + ix->nhx;
                     ix->ul2.d.yincr =
                        ix->ul2.d.rowsz*ix->nvy + ix->nhy;
                     }
                  else {         /* IA */
                     ix->ul2.d.xincr = (ix->nhx*RP->nsy<<1) + ix->nvx;
                     ix->ul2.d.yincr = (ix->nhy*RP->nsy<<1) + ix->nvy;
                     }}
               else {  /* Repertoire */
                  ix->ul2.d.rowsz = ix->srcnel*ix->srcngx;
                  ix->ul2.d.xincr = ix->ul2.d.rowsz*ix->nvx + ix->nhx;
                  ix->ul2.d.yincr = ix->ul2.d.rowsz*ix->nvy + ix->nhy;
                  }
               ix->ul2.d.yincr -= (ix->nrx-1)*ix->ul2.d.xincr;
               } /* End setup for KGEN=D */

/* 14) Variables relating to 'Q' ('SQuare annulus') option */

            else if (ix->kgen & KGNAN) {
               ix->ul2.q.qatot = ix->nrx*ix->nry - ix->nhx*ix->nhy;
               ix->ul2.q.qhalr = (ix->nhy*(ix->nrx - ix->nhx)) >> 1;
               ix->ul2.q.qhnrx = ix->nrx >> 1;
               ix->ul2.q.qhnry = ix->nry >> 1;
               ix->ul2.q.qhnhy = ix->nhy >> 1;
               if (ix->kgfl & KGNXR) {             /* Not  REP */
                  if (ix->kgfl & (KGNVG|KGNTV))    /* VG or TV */
                     ix->ul2.q.qsrcnel2 = 0;
                  else                             /*    IA    */
                     ix->ul2.q.qsrcnel2 = RP->ky1;
                  }
               else                                /* Repertoire */
                  ix->ul2.q.qsrcnel2 = ix->srcnel << 1;
               } /* End setup for KGEN=Q */

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

/* Prepare to generate cell and connection data.  */

         setinsi(il, 0, il->mxsdelay1);   /* Set globals for initsi */

/* If any conntypes have external Lij's & NOPOR, open Lij file.
*  On PAR0, elijopen() also packetizes data to comp nodes. */

         if (doelij) {
#ifdef PARn
            elijopnn();
#else
            if (pelf) elijopen(il, pelf);
            else      d3exit(LIJXF_ERR, NULL, 0);
#endif
            } /* End opening external Lij files */

/*---------------------------------------------------------------------*
*                           Loop over cells                            *
*---------------------------------------------------------------------*/

         celllim = il->locell + il->mcells;  /* Set counter limit */
#ifndef PAR0
         /* N.B. d3allo() guarantees that plsd points
         *  to an appropriate storage boundary */
         plsd = il->prd;
         piz  = (struct IZHICOM *)il->prfi;
         /* Seed for generating individual Izhikevich a,b,c,d */
         if (il->nizvar) {
            lzseed = piz->izseed;
#ifdef PAR
            udevskip(&lzseed, il->nizvar*il->locell);
#endif
            }
#endif

         for (icell=il->locell; icell<celllim; ++icell) {

#ifndef PAR0
            /* Establish group-level, etc. variables for this cell */
            d3kiji(il, icell);

/* If cell-level data not restored, zero them now */

            if ((il->Ct.ctopt & OPTOR) && il->ls)
               memset((char *)plsd, 0, il->ls);

/* Generate Izhikevich variables:  a,b,c,d randomizations.
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
                     tt = mssle(tt, (si32)piz->izra, -31, OVF_IZHI);
                     d3ptl2(tt, plzd); plzd += SHSIZE; }
                  if (piz->izrb) {
                     tt = udev(&lzseed) << 1;
                     tt = mssle(tt, (si32)piz->izrb, -31, OVF_IZHI);
                     d3ptl2(tt, plzd); plzd += SHSIZE; }
                  if (piz->izrc) {
                     tt = udev(&lzseed) << 1;
                     tt = mssle(tt, abs32(tt), -31, OVF_IZHI);
                     tt = mssle(tt, (si32)piz->izrc, -31, OVF_IZHI);
                     d3ptl2(tt, plzd); plzd += SHSIZE; }
                  if (piz->izrd) {
                     tt = udev(&lzseed) << 1;
                     tt = mssle(tt, abs32(tt), -31, OVF_IZHI);
                     tt = mssle(tt, (si32)piz->izrd, -31, OVF_IZHI);
                     d3ptl2(tt, plzd); }
                  }
               } /* End rspmeth >= RF_IZH3 */
#endif   /* Back to all nodes */

/* Generate s(i) data.  (N.B.  initsi is called from d3rstr if
*  restoring and phases or extra delay rows must be initialized.  */

            if (il->Ct.ctopt & (OPTOR|OPTNR))
               initsi(il, icell);

#ifndef PAR0
/* Loop over connection types */

            /* If no connection data to generate, forget it */
            if (!(il->ctf & CTFCT)) continue;

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
               for (Lija.isyn=0; Lija.isyn<ix->nc; ++Lija.isyn) {

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
                     d3ptl1(Lija.dijval, Lija.psyn+offDij);
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
            plsd+=il->lel;

#endif
            } /* End of loop over cells */

         /* If there are any external connections, close current file */
         if (doelij) {
            elijclos();
#ifndef PARn
            pelf = pelf->nf;
#endif
            }

/* Print initial s(i) if requested */

         if (il->Ct.kctp & KRPEP && RP->CP.runflags & RP_OKPRIN)
            d3rpsi(il, -1);

#ifndef PAR0
/* Initialize real modulation sums for this cell type based on
*  starting values of s(i).  These sums are not needed on host.
*  (Note: the actual summation must be done rather than using
*     nmn,nfr so is correct when starting from a saved net,
*     also for compatibility with earlier versions.)  */

         sumrmod(il);
#endif                        /* End host exclusion */

         }  /* End of loop over cell types */

#ifndef PAR0
      /* Initialize ion concentrations */
      inition(ir, 0);
#endif

      }  /* End of loop over repertoires/regions */

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
         plid = il->prd + il->ctoff[IONC] + pion->oCi;
         plie = il->prd + il->llt;
         for ( ; plid<plie; plid+=il->lel)
            memcpy((char *)plid, (char *)&pion->C0int, lmvi);
         } /* End loop over CELLTYPEs in this region */
      } /* End region-level IONTYPE loop */

/* If not bypassing at celltype level, reset celltype-level ions */

   for (il=ir->play1; il; il=il->play) {
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
         plie = il->prd + il->llt;
         for ( ; plid<plie; plid+=il->lel)
            memcpy((char *)plid, (char *)&pion->C0int, lmvi);
         } /* End CELLTYPE-level IONTYPE loop */
      } /* End CELLTYPE loop */

   } /* End inition() */
#endif


/*=====================================================================*
*                           initsi, setinsi                            *
*                                                                      *
*  initsi() initializes s(i) and phase values to zero or noise as      *
*  specified in CELLTYPE.  Izhikevich u and Brette-Gerstner w are      *
*  initialized to be in equilibrium with s(i).  Note that if s(i)      *
*  is at resting potential, phase has no meaning, and, for compa-      *
*  tibility with old versions, it is set to zero and the phase         *
*  seed is not advanced.                                               *
*                                                                      *
*  setinsi() sets the globals in Lija used by initsi.  (Formerly,      *
*  initsi was called once per layer, not once per cell, and these      *
*  variables were set inside the initsi call.                          *
*                                                                      *
*  Both routines must be called in parallel from all nodes.  (On       *
*  the host, the values may be needed for rpsi print or other uses.)   *
*  N.B.  s(i), phase data are in pps arrays for all cells on all       *
*  nodes, but u,w data in prd arrays are only for this node's cells.   *
*  Handling this distinction here makes life easier for callers.       *
*  The logic is quite subtle--See Design Notes, p. 43.                 *
*                                                                      *
*  setinsi() argumemts:                                                *
*     il       Ptr to CELLTYPE for which initialization to be done.    *
*     ni1      Number of delay rows initialized with phase only.       *
*              This condition occurs in d3rstr when s(i) is read in    *
*              from a SAVENET file but phase is newly generated.       *
*              Caller should assure ni1 == zero if no phases.          *
*     ni2      Number of delay rows initialized with s(i) and phase.   *
*              This condition occurs in d3genr, d3reset, and d3rstr    *
*              when a full set of s(i),phase values must be generated. *
*                                                                      *
*  initsi() arguments:                                                 *
*     il       Ptr to CELLTYPE for which initialization to be done.    *
*     icell    Number of the cell to be initialized.  -1 indicates     *
*              initsi should initialize all cells on node.             *
*                                                                      *
*  The following globals are now kept in LIJA:                         *
*     resetu   Pointer to routine used to initialize Izhikevich u or   *
*              Brette-Gerstner w.                                      *
*     irow1    First delay row for ni1 initialization.                 *
*     irow2    First delay row for ni2 initializaiton.                 *
*     Noisy    TRUE to generate noise.                                 *
*     Phsupd   Method of phase generation: 0, fixpha, or -1 for random *
*                                                                      *
*  Affected rows will end with the last row in the pps array.          *
*                                                                      *
*  Rev, 08/16/92, GNR - Use phiseed to set phi, unless const|COMPAT=P  *
*  Rev, 02/07/94, GNR - ni1,ni2 allow both kinds of init in one call   *
*  Rev, 04/13/96, GNR - Compatibility with original use of nsd removed *
*  Rev, 03/05/98, GNR - Add resting potential (V8C: only w/COMPAT=C)   *
*  Rev, 10/15/05, GNR - Randomize phase even if s(i) == Vrest.         *
*  Rev, 01/03/07, GNR - Negative s(i) not allowed with old range.      *
*  Rev, 02/16/09, GNR - Remove addition of Vrest w/COMPAT=C            *
*  V8F, 07/24/10, GNR - Make it a call for a single cell, not a whole  *
*                       celltype, to eliminate parallel regeneration   *
*                       code.  Initialize Izhikevich u and Gerstner w. *
*=====================================================================*/

/*---------------------------------------------------------------------*
*                              setinsi()                               *
*---------------------------------------------------------------------*/

void setinsi(struct CELLTYPE *il, int ni1, int ni2) {

   /* Sanity check to avoid overflowing s(i) array */
#ifndef PARn
   if ((ni1 + ni2) > (int)il->mxsdelay1)
      d3exit(INITSI_ERR, fmturlnm(il), 0);
#endif

   /* Calculate row limits for s(i) generation.  These limits
   *  assure idly loop omits rows that have no noise or phase.  */
   Lija.irow1 = (Lija.irow2 = il->mxsdelay1 - ni2) - ni1;

   /* Sanity check to avoid underflowing s(i) array */
#ifndef PARn
   if (Lija.irow1 < 0) d3exit(INITSI_ERR, fmturlnm(il), 0);
#endif

#ifndef PAR0
   /* Which resetu routine to use */
   switch (il->Ct.rspmeth) {
   case RF_BREG:
      Lija.resetx = resetw; break;
   case RF_IZH3:
      Lija.resetx = resetu3; break;
   case RF_IZH7:
      Lija.resetx = resetu7; break;
   default:
      Lija.resetx = NULL;
      }  /* End rspmeth switch */
#endif

   Lija.Noisy = (il->ctf & CTFHN) && !(il->Ct.ctopt & OPTNIN);
   Lija.nmnS7 = SRA(il->No.nmn, FBwk-FBsi);
   Lija.nsgS11 = il->No.nsg >> (FBwk-FBsi);
   /* Method of phase update: Phsupd >=0 constant, -1 random */
   if (il->phshft) {
      struct PHASEDEF *ppd = il->pctpd;
      Lija.Phsupd = Lija.Noisy ?
         ((ppd->phimeth == PHI_CONST) ? (int)ppd->fixpha : -1) : 0;
      }

   } /* End setinsi() */


/*---------------------------------------------------------------------*
*                               initsi                                 *
*---------------------------------------------------------------------*/

void initsi(struct CELLTYPE *il, long icell) {

   s_type *psi,*psi0,*psie;      /* Ptr to pps cell data */
   size_t lpsi = (icell < 0) ? (size_t)il->lspt : (size_t)il->lsp;
   si32 wksi;                    /* Working value of si */
   int idly;                     /* Delay counter */

   for (idly=Lija.irow1; idly<(int)il->mxsdelay1; idly++) {

      /* Locate data for specified cell or cells */
      if (icell < 0)
         psi0 = il->pps[idly], psie = psi0 + il->lspt;
      else
         psi0 = psie = il->pps[idly] + spsize(icell, il->phshft);

      if (idly >= Lija.irow2) {     /* Generating s(i) here */
         if (Lija.Noisy) for (psi=psi0; psi<psie; psi+=il->lsp) {
            wksi = d3noise(Lija.nmnS7,Lija.nsgS11,il->No.nfr,il->nsd);
            if (RP->compat & OLDRANGE) {
               if (wksi > S14) wksi = S14;
               else if (wksi < 0) wksi = 0;
               }
            d3ptl2(wksi, psi);
            }
         else                          /* No noise, set s(i) to 0 */
            memset((char *)psi0, 0, lpsi);
         }  /* End generating s(i) */

      if (il->phshft) {             /* Generating phase here */
         if (Lija.Phsupd >= 0) for (psi=psi0; psi<psie; psi+=il->lsp)
            psi[LSmem] = (s_type)Lija.Phsupd;      /* Fixed phase */
         else {
            si32 *ppsd = &il->pctpd->phiseed;
            for (psi=psi0; psi<psie; psi+=il->lsp)
               psi[LSmem] = (s_type)(udev(ppsd) & PHASE_MASK);
            }
         } /* End generating phase */

      } /* End delay row loop */

#ifndef PAR0
   /* If cell is Izhikevich or Brette-Gerstner type, must also
   *  initialize the adaptation variable. This exists only for
   *  cells on the current node, and only for the current time,
   *  i.e. this is outside the idly loop.  */
   if (Lija.resetx) {
      if (icell < 0) {
         long celllim = il->locell + il->mcells;
         for (icell=il->locell; icell<celllim; ++icell)
            Lija.resetx(il, icell);
         }
      else
         Lija.resetx(il, icell);
      }
#endif

   } /* End initsi */
