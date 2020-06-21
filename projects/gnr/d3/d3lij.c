/* (c) Copyright 1988-2017, The Rockefeller University *21115* */
/* $Id: d3lij.c 78 2018-08-02 18:36:58Z  $ */
/***********************************************************************
*                             CNS Program                              *
*               d3lij - Connection Generation Routines                 *
*                                                                      *
*  Note:  These routines are designed to make Lij values accessible    *
*  at any point in CNS without regard to whether or not OPTIMIZE       *
*  STORAGE is in effect.  This makes CNS easier to understand and      *
*  maintain, at a cost of some inner loop calls and redundant bits     *
*  of arithmetic.                                                      *
*                                                                      *
*  None of these routines should be called on a PAR0 node--this file   *
*  should not even be compiled on a PAR0 node, hence no #ifdefs.       *
*                                                                      *
*----------------------------------------------------------------------*
*                                                                      *
*  int Lija.lijbr(struct CONNTYPE *ix)                                 *
*                                                                      *
*  This routine returns YES if a connection was generated or NO if     *
*     there are no more connections of the current type for the        *
*     current cell (should happen only when generating).               *
*  The actual value of Lij is returned in the Lija structure.          *
*  The caller is expected to maintain Lija.isyn as a loop over nuk     *
*     synapses, i.e., the number actually used.  The d3lij routine     *
*     then sets Lija.jsyn, the actual connection number, which may     *
*     increment more than once per value of isyn when connections      *
*     are skipped (due to being out-of-bounds).                        *
*  In V8A, a "quick and dirty" implementation of self-avoidance is     *
*     included that may perform tests when layers are not the same,    *
*     and that just generates an "out-of-bounds" rather than an        *
*     alternate connection when self is hit.  Until this is fixed,     *
*     rule A has been made illegal with V.                             *
*  In V8D, this was fixed to the extent that the test is always        *
*     omitted when the layers are not the same.                        *
*  Prerequisites are that d3kij() must have been called to set up      *
*     for the particular kind of generation wanted (initial, normal    *
*     access, or regenerating), d3kiji() must have been called to set  *
*     up for the particular cell type and cell number to be accessed,  *
*     and d3kijx() and d3lijx() must have been called to set up for    *
*     the particular connection type to be accessed, in that order.    *
*     The caller must maintain the synapse index (isyn) and increment  *
*     the data pointer (psyn) in the Lija structure, as these are      *
*     shared among all d3lij, d3cij, etc. calls.                       *
*  In KMEM_GO mode, Lij values are looked up in memory if OPTIMIZE     *
*     SPEED is in effect or regenerated from input parameters if       *
*     OPTIMIZE STORAGE is in effect.  Otherwise they are generated.    *
*                                                                      *
*----------------------------------------------------------------------*
*                                                                      *
*  void d3lijx(struct CONNTYPE *ix)                                    *
*                                                                      *
*  This routine must be called to initialize the Lija array for a      *
*     particular connection type after the celltype and cell have      *
*     been established by d3kiji().  In KMEM_GO mode, the value of     *
*     NUK stored by d3genr() can be used to control a subsequent       *
*     loop over the connections.                                       *
*                                                                      *
************************************************************************
*  V4A, 10/24/88  Translated to C from version V3F                     *
*  V5C, 02/20/92, GNR - Allow KGEN=E with various third letter types   *
*  V5E, 07/25/92, GNR - Make KGEN=C consistent in serial and parallel  *
*  Rev, 08/06/92, GNR - Force Lij generation if KGEN=L                 *
*  Rev, 08/15/92, GNR - Remove udevskip from lb1j done in d3go on SKIP *
*  Rev, 08/20/92, GNR - Reflect removal of VG bounds checking in d3go  *
*  Rev, 10/11/92, GRN - Add KGEN=H ("Hypergroup") routine              *
*  V6A, 03/03/93, GNR - Add subarbors, eliminate lbsrcla, fix lb1tia   *
*  V7B, 06/23/94, GNR - Assign lijomit if KGNHV, introduce isyn,isub   *
*  V7C, 01/30/95, GNR - Add KGEN=Q (sQuare annulus) routine            *
*  V8A, 04/15/95, GNR - Merge kchng,flags into WDWDEF, add KGNSA       *
*  Rev, 04/26/97, GNR - Set to always condense connection data         *
*  Rev, 07/30/00, GNR - Correct xia bug, use xoff,yoff with X,J rules  *
*  V8D, 07/01/06, GNR - Major rewrite to make transparent to user code *
*  ==>, 10/31/07, GNR - Last mod before committing to svn repository   *
*  Rev, 06/06/08, GNR - Add SUBARBOR options L and 3                   *
*  Rev, 06/23/08, GNR - Revise IA bounds-checking rules, add jsyn      *
*  V8E, 02/25/09, GNR - Correct jsyn for KGEN=Q, isas for getlij       *
*  V8F, 06/17/10, GNR - Correct rowe test in lbqb, add lbsai           *
*  V8H, 10/24/10, GNR - Add xn (once per conntype) variable allocation *
*  Rev, 12/07/12, GNR - Convert ns[xy], ku[xy] to ui16, nst to ui32    *
*  R65, 01/02/16, GNR - n[gq][xyg] to ui16, ngrp to ui32, ncpg to ubig *
*  R72, 02/07/17, GNR - Add lb1k, lb1kia, lb1kvg, remove lb2d, lb2dia  *
*  R73, 04/18/17, GNR - Convert KGNAN to circular or ellipsoidal areas *
*  R74, 08/29/17, GNR - Add KGNG1 rule (first cell in current group)   *
*  R78, 03/15/18, GNR - Allow any KGEN 1st conn code with KGNKN kernel *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "d3global.h"
#include "lijarg.h"
#include "rkarith.h"

extern struct LIJARG Lija;

/*---------------------------------------------------------------------*
*               Prototypes for Lij generating routines                 *
*---------------------------------------------------------------------*/

/* Partitioned connection types */
int lb1fp(struct CONNTYPE *ix);
int lb1fpia(struct CONNTYPE *ix);
int lb1fpvg(struct CONNTYPE *ix);
int lb1gp(struct CONNTYPE *ix);
int lb1hp(struct CONNTYPE *ix);
int lb1jp(struct CONNTYPE *ix);
int lb1op(struct CONNTYPE *ix);
int lb1up(struct CONNTYPE *ix);
int lb1upia(struct CONNTYPE *ix);
int lb1upvg(struct CONNTYPE *ix);

/* Normal connection types--first connection */
int lb1f(struct CONNTYPE *ix);
int lb1fia(struct CONNTYPE *ix);
int lb1fvg(struct CONNTYPE *ix);
int lb1g(struct CONNTYPE *ix);
int lb1g1(struct CONNTYPE *ix);
int lb1h(struct CONNTYPE *ix);
int lb1j(struct CONNTYPE *ix);
int lb1n(struct CONNTYPE *ix);
int lb1o(struct CONNTYPE *ix);
int lb1t(struct CONNTYPE *ix);
int lb1tia(struct CONNTYPE *ix);
int lb1tvg(struct CONNTYPE *ix);
int lb1u(struct CONNTYPE *ix);
int lb1uia(struct CONNTYPE *ix);
int lb1uvg(struct CONNTYPE *ix);
int lb1x(struct CONNTYPE *ix);
int lb1xia(struct CONNTYPE *ix);

/* Normal connection types--subsequent connections */
int lb2a(struct CONNTYPE *ix);
int lb2aia(struct CONNTYPE *ix);
int lb2b(struct CONNTYPE *ix);
int lb2bia(struct CONNTYPE *ix);
int lbqb(struct CONNTYPE *ix);
int lb2c(struct CONNTYPE *ix);
int lb2cia(struct CONNTYPE *ix);
int lb2cvg(struct CONNTYPE *ix);
int lb2k(struct CONNTYPE *ix);
int lb2kia(struct CONNTYPE *ix);
int lb2kvg(struct CONNTYPE *ix);
int lb2q(struct CONNTYPE *ix);
int lb2qia(struct CONNTYPE *ix);
int lb2qvg(struct CONNTYPE *ix);
int lbsa(struct CONNTYPE *ix);
int lbsaia(struct CONNTYPE *ix);
int lbsai(struct CONNTYPE *ix);
int lbsal(struct CONNTYPE *ix);

/* Special cases */
int d3elij(struct CONNTYPE *ix);
int firstLij(struct CONNTYPE *ix);
int getlij(struct CONNTYPE *ix);
int lijomit(struct CONNTYPE *ix);
void lijxyr(struct CONNTYPE *ix);
int otherLij(struct CONNTYPE *ix);


/*=====================================================================*
*                               d3lijx                                 *
*                                                                      *
*  Initialize for Lij generation for a particular connection type      *
*  after the celltype and cell have been established by d3kiji().      *
*=====================================================================*/

void d3lijx(struct CONNTYPE *ix) {

   if (Lija.nLij >= 0) return;      /* Already called once */

   Lija.jsyn   = -1;                /* Increments on access */
   Lija.nLij   = 0;
   /* GETLIJ is TRUE if KGNHV or (KMEM_GO && !CLIJ) */
   Lija.lijbr  = ix->cnflgs & GETLIJ ? ix->lijbr1 : firstLij;
   Lija.isas   = 0;
   Lija.nsas   = (long)ix->nsa;
   Lija.jsa    = 0;
   Lija.lb2s   = ix->Cn.saopt & SAOPTL ? (long)ix->nsa : 1;

/* When Cij values are stored in the prd data, a skip-connection list
*  is generated and stored in descending order starting at the top of
*  the space allocated for Cij.  ix->lc may be adjusted up in d3allo()
*  to hold at least ix->nuklen bytes per connection.  */

   Lija.pskpl = cnexists(ix,CIJ) ?
      Lija.pxnd + ix->lct - ix->nuklen : NULL;

/* If call is from a program that consumes rather than generates
*  connections, retrieve Lija.nuk and calculate maximum number
*  skipped from nuk.  Then, if a skip list has been stored, pick
*  up the value of the first index skipped, otherwise set to -1.  */

   Lija.skipndx = -1;
   if (Lija.kcall == KMEM_GO) {
      rd_type *pnuk = Lija.pxnd + ix->xnoff[NUK];
      d3gtln(Lija.nuk, pnuk, ix->nuklen);
      if (Lija.pskpl && (Lija.nskipr = (long)ix->nc - Lija.nuk) > 0)
         d3gtln(Lija.skipndx, Lija.pskpl, ix->nuklen);
      } /* End skip list setup */

/* If doing self-avoidance, set satest to current cell number,
*  else to -1.  This saves one test in each generating routine.
*  (d3tchk() turns off KGNSA bit if the layers are not the same.)  */

   Lija.satest = (ix->kgen & KGNSA) ? Lija.kcell : -1;

/* These variables are only used if generating rather than
*  fetching Lij, but it's probably faster to skip the 'if' and
*  just set them whether needed or not.  */

   Lija.wlseed = ix->clseed;
   Lija.boxe   = 0;                    /* For KGNPT only */
   Lija.xcur   = 0;                    /* For KGNKN only */

   } /* End d3lijx() */


/***********************************************************************
*                                                                      *
*                Actual connection generating routines                 *
*                                                                      *
*     The appropriate lb1 (primary Lij) routine is entered via first-  *
*  Lij() to generate the first connection to each cell, and then the   *
*  lb2 or lbq secondary Lij routine is entered via otherLij() for each *
*  subsequent connection.  This allows for the different handling of   *
*  the skip list on first and subsequent calls to the same generating  *
*  routine in independent and partitioned cases, and incidentally      *
*  allows the skip list to be handled in one place rather than in      *
*  each generating routine.  If subarbors are being generated, the     *
*  subarbor routine replaces the secondary routine in lijbr2, and the  *
*  secondary routine is called from there via lijbr3.                  *
*                                                                      *
*     All these routines return YES if a connection was generated or   *
*  NO if there are no more connections of the current type for the     *
*  current cell.  The actual value of Lij is returned in Lija.lijval.  *
*  If a connection is skipped due to being out-of-bounds, Lija.jsyn    *
*  is incremented, a skip list entry is stored (except in the getlij   *
*  case), and another connection is generated, so the caller never     *
*  knows about the skip unless checking Lija.jsyn.                     *
*                                                                      *
*     All Lij routines must do their own bounds checking.  In the      *
*  case of input from the input array, the check must be done against  *
*  the size of the viewing window (KGNST case) or the whole IA (all    *
*  other cases).  Then it should not be necessary to check again when  *
*  the window offset is added, typically in a d3sj() routine, as long  *
*  as the window remains inside the IA.  Primary Lij routines must     *
*  guarantee that their output is in bounds (or must exit with the     *
*  return value NO but store the starting values in fstx,fsty)         *
*  unless used with one of the secondary routines that allow for       *
*  this--currently B, C, K, and Q.                                     *
*                                                                      *
*     Currently all primary routines (except partitioned) must store   *
*  fstx, fsty, and fstr for possible use by subarbor or secondary Lij  *
*  routines.  Although these are supposed to be 'first' values, it is  *
*  OK to reset them every time the lb1 routine is called, because it   *
*  will be called more than once per subarbor only in the case KGEN=I, *
*  and in that case nobody else will look at the 'first' values.       *
*  Routines that use these variables in their own operations take      *
*  advantage of this and may set them each time called.  Other         *
*  routines call lijxyr() to set them.  This is done only on the       *
*  first call in a subarbor, which saves a little time when KGEN=I.    *
*                                                                      *
*     d3genr() condenses all stored connection data such that skipped  *
*  connections are not stored and the unused space is at the end of    *
*  the data.  If Cij are stored, a list of skipped connection indexes  *
*  is stored in the connection data.  The number of connections actu-  *
*  ally generated is stored in nuklen bytes at pnuk = Lija.pxnd +      *
*  xnoff[NUK].                                                         *
*                                                                      *
*  Lij storage was revised in V3F to allow Lij to have 1-4 bytes as    *
*  needed.  The external Lij connection list (CONNLIST) file format    *
*  has been revised (02/20/92) to allow a full 4-byte Lij, which is    *
*  truncated to lijlen bytes when read.  Note that lijlen clicks up    *
*  one byte when srcnelt just equals a power of 256, even though cell  *
*  numbers start at 0, because the code Lij = all 1's is reserved for  *
*  use as the skip flag and cannot represent a source cell.            *
*                                                                      *
***********************************************************************/

/***********************************************************************
*                  V8D, 08/01/06, Compatibility Note                   *
*                                                                      *
*  Code for partitioned types was modified to eliminate assumption     *
*  that nc < S16 and chsiz < S16.  Results will not match old code.    *
*  With this in mind, no correction was made for change in direction   *
*  of isyn counting, which will also cause results to differ.  A bug   *
*  using srcngx instead of nux to calculate x,y coords for KGNTP was   *
*  also fixed.                                                         *
*                                                                      *
*  Code for type C secondary connections was also modified to          *
*  eliminate S16 assumptions.  Results will not match old code.        *
*  The following translation note dated 11/20/88 is therefore moot:    *
*     "It was not practical in all cases to exactly duplicate the      *
*  arithmetic carried out in the IBM assembler versions of the Lij     *
*  generating routines.  Routines that may perform differently than    *
*  their IBM assembler predecessors are:  lb2cia."                     *
*                                                                      *
*  Under the new IA bounds-checking rules, some results may differ     *
***********************************************************************/

/*---------------------------------------------------------------------*
*  Lij routine for calling first Lij routine and then setting otherLij *
*     to call the secondary Lij routine for subsequent calls.          *
*  V7C, 01/31/95, GNR - Revised to execute lijbr2 routine right away   *
*     if code 'Q' is enabled.  To get rid of the extra 'if' in this    *
*     code, it would be necessary to make another version and put its  *
*     address in lijbr at start of each cell in d3genr/d3go, hence     *
*     utilizing yet another lijbr ptr in the conntype block (not done).*
*  V8D, 08/27/06, GNR - Generate skip list here rather than in d3genr  *
*     or d3regenr.  If connection is out-of-bounds, store isyn in skip *
*     list, increment jsyn, and repeat until ix->nc count fulfilled.   *
*  Rev, 09/29/06, GNR - Add nLij counter so user code, cijbr() code,   *
*     and sjbr() code can all call lijbr() and get just one execution. *
*  V8G, 10/18/10, GNR - Correct bug in earlier KGNAN bug fix.          *
*  R78, 03/16/18, GNR - Treat KGNKN like KGNAN--call lb2 right away.   *
*                       Save in skip list if KGNAN|KGNKN otherLij OOB  *
*---------------------------------------------------------------------*/


int firstLij(struct CONNTYPE *ix) {

   /* Oops, user didn't initialize */
   if (Lija.nLij < 0) d3lijx(ix);

   /* Set jsyn to first conn--getLij,lb2[kq] will increment from -1 */
   Lija.jsyn = 0;
   Lija.lijrc = ix->lijbr1(ix);     /* Call first routine */
   Lija.svlseed = Lija.wlseed;      /* Save restart seed  */
   Lija.lijbr = otherLij;           /* Set for second routine  */
   if (ix->kgen & (KGNAN|KGNKN)) {  /* Types that replace first Lij */
      Lija.jsyn = -1;
      Lija.lijrc = otherLij(ix);    /* Must be OK with first Lij OOB */
      }
   else if (!Lija.lijrc) {          /* TRUE if first Lij is OOB */
      if (Lija.pskpl) {          /* Save indices of skipped item(s) */
         long jsyn;              /* (Loop is really for SAOPTL) */
         for (jsyn=0; jsyn<Lija.lb2s; ++jsyn) {
            d3ptln(jsyn, Lija.pskpl, ix->nuklen);
            Lija.pskpl -= ix->nuklen; }
         }
      Lija.jsyn += Lija.lb2s - 1;
      Lija.lijrc = otherLij(ix);    /* May loop over more skips */
      }
   else                             /* First Lij OK, count & return */
      Lija.nLij = 1;

   return Lija.lijrc;

   } /* End firstLij() */

/* Note:  otherLij() should always be entered with Lija.jsyn equal
*  to the number of the connection previously processed, as it
*  will be incremented here before calling the actual generating
*  routine.  That is the reason for adding Lija.lb2s in firstLij()
*  when the first connection (or subarbor) gets skipped.  */

int otherLij(struct CONNTYPE *ix) {

   /* Return right away if lijbr() already executed for this conn */
   if (Lija.nLij > Lija.isyn) return Lija.lijrc;

   while (++Lija.jsyn < (long)ix->nc) {
      if ((Lija.lijrc = ix->lijbr2(ix)))  /* Assignment intended */
         { ++Lija.nLij; return Lija.lijrc; }
      if (Lija.pskpl) {          /* Save index of skipped item */
         d3ptln(Lija.jsyn, Lija.pskpl, ix->nuklen);
         Lija.pskpl -= ix->nuklen; }
      } /* End loop over out-of-bound connections */

   return (Lija.lijrc = NO);

   } /* End otherLij() */


/*---------------------------------------------------------------------*
*     Lij routine used when Lij's are not stored (hand vision).        *
*---------------------------------------------------------------------*/

int lijomit(struct CONNTYPE *ix) {

   ++Lija.jsyn;
   Lija.lijval = 0;
   return YES;

   } /* End lijomit() */


/*---------------------------------------------------------------------*
*     getlij - Pick up Lij from the data structure                     *
*                                                                      *
*     This routine is used when in KMEM_GO mode and Lij values were    *
*     prestored (the usual case with OPTIMIZE SPEED and always with    *
*     external connections regardless of OPTIMIZE setting.             *
*                                                                      *
*     It is no longer necessary to update wlseed when skipping the     *
*     rest of the connections for this cell, because it will be        *
*     initialized from ix->clseed when d3lijx is next called.          *
*---------------------------------------------------------------------*/

int getlij(struct CONNTYPE *ix) {

   /* Return right away if lijbr() already executed for this conn */
   if (Lija.nLij > Lija.isyn) return Lija.lijrc;

   /* If this was a skipped connection, skip it again now.
   *  Note:  If there is no skip list, Lija.skipndx is -1 and this
   *    'while' is skipped--no need to test existence of pskpl.  */
   while (++Lija.jsyn == Lija.skipndx) {
      if (--Lija.nskipr <= 0)
         Lija.skipndx = -1;
      else {
         Lija.pskpl -= ix->nuklen;
         d3gtln(Lija.skipndx, Lija.pskpl, ix->nuklen);
         }
      }

   /* Do subarbor bookkeeping */
   while (Lija.jsyn >= Lija.nsas) {
      Lija.isas = Lija.nsas, Lija.nsas += (long)ix->nsa;
      }

   d3gtln(Lija.lijval, Lija.psyn+ix->cnoff[LIJ], ix->lijlen);
   /* Assignment intended in next line */
   if ((Lija.lijrc = (Lija.lijval != ix->lijlim))) ++Lija.nLij;

   return Lija.lijrc;

   } /* End getlij() */


/*---------------------------------------------------------------------*
*     lb1f - Floating map from a repertoire - new in V2I               *
*                                                                      *
*     Maps uniformly from a specified rectangular region in the        *
*        source.  Sort of a T-U hybrid.                                *
*     Since this routine can generate connections that are out of      *
*        bounds, it currently can not be used with 'A'.                *
*---------------------------------------------------------------------*/

int lb1f(struct CONNTYPE *ix) {

   ui32 low32a,low32b;

   Lija.fstr = jm64sb(udev(&Lija.wlseed),ix->srcnel,1,&low32a);
   Lija.fstx = (long)ix->xoff + jm64nb(low32a>>1,ix->ul1.s.sfx,&low32b);
   Lija.fsty = (long)ix->yoff + jm64nh(low32b>>1,ix->ul1.s.sfy);

   /* Skip the connection if X or Y out of bounds */
   if ((ulng)Lija.fstx >= (ulng)ix->iawxsz) return NO;
   if ((ulng)Lija.fsty >= (ulng)ix->iawysz) return NO;

   Lija.lijval = (Lija.fsty*(long)ix->srcngx + Lija.fstx)*
      (long)ix->srcnel + Lija.fstr;

   return (Lija.lijval != Lija.satest);

   } /* End lb1f() */


/*---------------------------------------------------------------------*
*     lb1fia - Floating map from input array - new in V2I              *
*---------------------------------------------------------------------*/

int lb1fia(struct CONNTYPE *ix) {

   ui32 low32a;

   Lija.fstx = (long)ix->xoff +
      jm64nb(udev(&Lija.wlseed),ix->ul1.s.sfx,&low32a);
   Lija.fsty = (long)ix->yoff +
      jm64nh(low32a>>1, ix->ul1.s.sfy);
   Lija.fstr = 0;

   /* Unsigned test should reject negatives */
   if ((ulng)Lija.fstx >= (ulng)ix->iawxsz) return NO;
   if ((ulng)Lija.fsty >= (ulng)ix->iawysz) return NO;

   Lija.lijval = (Lija.fstx << RP->ky1) + Lija.fsty;

   return YES;

   } /* End lb1fia() */


/*---------------------------------------------------------------------*
*     lb1fvg - Floating map from a virtual repertoire, or TV           *
*                                                                      *
*     Since this routine can generate connections that are out of      *
*        bounds, it currently can not be used with 'A'.                *
*---------------------------------------------------------------------*/

int lb1fvg(struct CONNTYPE *ix) {

   ui32 low32a;

   Lija.fstx = (long)ix->xoff +
      jm64nb(udev(&Lija.wlseed),ix->ul1.s.sfx,&low32a);
   Lija.fsty = (long)ix->yoff +
      jm64nh(low32a>>1, ix->ul1.s.sfy);
   Lija.fstr = 0;

   /* Skip the connection if X or Y out of bounds */
   if ((ulng)Lija.fstx >= (ulng)ix->iawxsz) return NO;
   if ((ulng)Lija.fsty >= (ulng)ix->iawysz) return NO;

   Lija.lijval = Lija.fstx + Lija.fsty*(long)ix->srcngx;

   return YES;

   } /* End lb1fvg() */


/*---------------------------------------------------------------------*
*     lb1fp - Partitioned floating from a repertoire - new in V2I      *
*                                                                      *
*     No need to save fstx,fsty--this routine is its own lb2 routine   *
*---------------------------------------------------------------------*/

int lb1fp(struct CONNTYPE *ix) {

   div_t qrmp,qrmq;
   si32 wx,wy;

   wx = dm64nq(ix->ul2.p.srcsiz, Lija.isyn+1, ix->nc);
   wy = Lija.boxe + jm64nh((wx-Lija.boxe)<<1, udev(&Lija.wlseed));
   Lija.boxe = wx;         /* Start at top of box next time */

   qrmp  = div(wy, ix->srcnel);
   Lija.lijval = qrmp.rem;
   qrmq  = div(qrmp.quot, ix->nux);
   wx = qrmq.rem  + ix->xoff;
   wy = qrmq.quot + ix->yoff;

   /* Skip the connection if X or Y out of bounds */
   if ((ui32)wx >= ix->iawxsz) return NO;
   if ((ui32)wy >= ix->iawysz) return NO;

   Lija.lijval += ((long)wy*(long)ix->srcngx + wx)*(long)ix->srcnel;

   return (Lija.lijval != Lija.satest);

   } /* End lb1fp() */


/*---------------------------------------------------------------------*
*     lb1fpia - Partitioned floating from input array - new in V2I     *
*                                                                      *
*     No need to save fstxy--this routine is its own lb2 routine       *
*---------------------------------------------------------------------*/

int lb1fpia(struct CONNTYPE *ix) {

   ldiv_t qrm;
   long wx,wy;

   wx = dm64nq(ix->ul2.p.srcsiz, Lija.isyn+1, ix->nc);
   wy = Lija.boxe + jm64nh((wx-Lija.boxe)<<1, udev(&Lija.wlseed));
   Lija.boxe = wx;         /* Start at top of box next time */

   qrm = ldiv(wy, (long)ix->nux);
   wx = qrm.rem;
   wy = qrm.quot;

   /* Unsigned test should reject negatives */
   if ((ulng)wx >= (ulng)ix->iawxsz) return NO;
   if ((ulng)wy >= (ulng)ix->iawysz) return NO;

   Lija.lijval = (wx << RP->ky1) + wy;

   return YES;

   } /* End lb1fpia() */


/*---------------------------------------------------------------------*
*     lb1fpvg - Partitioned floating from virtual repertoire or TV     *
*                                                                      *
*     No need to save fstx,fsty--this routine is its own lb2 routine   *
*---------------------------------------------------------------------*/

int lb1fpvg(struct CONNTYPE *ix) {

   ldiv_t qrm;
   long wx,wy;

   wx = dm64nq(ix->ul2.p.srcsiz, Lija.isyn+1, ix->nc);
   wy = Lija.boxe + jm64nh((wx-Lija.boxe)<<1, udev(&Lija.wlseed));
   Lija.boxe = wx;         /* Start at top of box next time */

   qrm = ldiv(wy, (long)ix->nux);
   wx = qrm.rem  + (long)ix->xoff;
   wy = qrm.quot + (long)ix->yoff;

   /* Skip the connection if X or Y out of bounds */
   if ((ulng)wx >= (ulng)ix->iawxsz) return NO;
   if ((ulng)wy >= (ulng)ix->iawysz) return NO;

   Lija.lijval = wy*(long)ix->srcngx + wx;

   return YES;

   } /* End lb1fpvg() */


/*---------------------------------------------------------------------*
*     lb1g - Random from same group as target cell is in               *
*                                                                      *
*     (Note: this is not necessarily in the same layer.                *
*        wgrpad is incremented at d3kiji for new groups.)              *
*---------------------------------------------------------------------*/

int lb1g(struct CONNTYPE *ix) {

   Lija.lijval = jm64nh(udev(&Lija.wlseed),ix->ul1.g.srcnel2)
                 + ix->ul1.g.wgrpad;
   if (Lija.jsyn == Lija.isas) lijxyr(ix);   /* Decode coords */

   return (Lija.lijval != Lija.satest);

   } /* End lb1g() */


/*---------------------------------------------------------------------*
*     lb1g1 - Pick first cell in same group as target cell is in       *
*     (Note: this is not necessarily in the same layer.                *
*        wgrpad is incremented at d3kiji for new groups.)              *
*     R78, 03/17/18, GNR - Now add x,y offsets to group.               *
*---------------------------------------------------------------------*/

int lb1g1(struct CONNTYPE *ix) {

   Lija.lijval = ix->ul1.g.wgrpad;
   if (Lija.jsyn == Lija.isas) lijxyr(ix);   /* Decode coords */

   return (Lija.lijval != Lija.satest);

   } /* End lb1g() */


/*---------------------------------------------------------------------*
*     lb1gp - Partitioned from same group as target cell is in         *
*                                                                      *
*     No need to save fstx,fsty--this routine is its own lb2 routine   *
*---------------------------------------------------------------------*/

int lb1gp(struct CONNTYPE *ix) {

   long wx,wy;

   wx = dm64nq(ix->ul2.p.srcsiz, Lija.isyn+1, ix->nc);
   wy = Lija.boxe + jm64nh((wx-Lija.boxe)<<1, udev(&Lija.wlseed));
   Lija.boxe = wx;         /* Start at top of box next time */

   Lija.lijval = ix->ul1.g.wgrpad + wy;

   return (Lija.lijval != Lija.satest);

   } /* End lb1gp() */


/*---------------------------------------------------------------------*
*     lb1h - Random from same "hypergroup" as target cell is in        *
*                                                                      *
*     (A "hypergroup" is the area defined by ngridx,ngridy.)           *
*     Currently lb1hia, lb1hpia have not been implemented (IA needs    *
*        new grid params to do it and hypergroups do not exist with    *
*        virtual group inputs.)                                        *
*     Note: input is not necessarily from the same layer.              *
*        wgrpad is incremented at d3kiji for new hypergroups.          *
*---------------------------------------------------------------------*/

int lb1h(struct CONNTYPE *ix) {

   div_t qrm;
   si32 randcell = jm64nh(udev(&Lija.wlseed),ix->ul1.g.srcnel2);

   qrm = div(randcell, (int)ix->ul1.g.gridrow);
   Lija.lijval = (long)qrm.quot*ix->ul1.g.srcnel1 + qrm.rem +
      ix->ul1.g.wgrpad;
   if (Lija.jsyn == Lija.isas) lijxyr(ix);   /* Decode coords */

   return (Lija.lijval != Lija.satest);

   } /* End lb1h() */


/*---------------------------------------------------------------------*
*     lb1hp - Partitioned from same "hypergroup" as target             *
*                                                                      *
*     This type new in V8D.                                            *
*     No need to save fstx,fsty--this routine is its own lb2 routine   *
*---------------------------------------------------------------------*/

int lb1hp(struct CONNTYPE *ix) {

   ldiv_t qrm;
   long wx,wy;

   wx = dm64nq(ix->ul2.p.srcsiz, Lija.isyn+1, ix->nc);
   wy = Lija.boxe + jm64nh((wx-Lija.boxe)<<1, udev(&Lija.wlseed));
   Lija.boxe = wx;         /* Start at top of box next time */

   qrm  = ldiv(wy, ix->ul1.g.gridrow);
   Lija.lijval = qrm.quot*ix->ul1.g.srcnel1 + qrm.rem +
      ix->ul1.g.wgrpad;

   return (Lija.lijval != Lija.satest);

   } /* End lb1hp() */


/*---------------------------------------------------------------------*
*     lb1j - Joint-style distribution from a repertoire                *
*                                                                      *
*     Choose randomly from lsg cells starting at loff, then            *
*        increment loff by nux+nuy*nel*ngx in wgrpad.                  *
*     This code used for input from REPERTOIRE or VG or TV.            *
*     V8D, 08/27/06, GNR - If out-of-bounds, just skip the one         *
*        connection, it might come in again on next KGNIN call.        *
*---------------------------------------------------------------------*/

int lb1j(struct CONNTYPE *ix) {

   Lija.lijval = jm64nh(udev(&Lija.wlseed),ix->lsg) + ix->ul1.g.wgrpad;

   if (Lija.jsyn == Lija.isas) lijxyr(ix);   /* Decode coords */

   /* Check for out of bounds */
   if ((ulng)(Lija.lijval) >= (ulng)ix->srcnelt) return NO;

   return (Lija.lijval != Lija.satest);

   } /* End lb1j() */


/*---------------------------------------------------------------------*
*     lb1jp - Partitioned from lsg cells starting at loff              *
*                                                                      *
*     This code used for input from REPERTOIRE or VG or TV.            *
*     No need to save fstx,fsty--this routine is its own lb2 routine   *
*---------------------------------------------------------------------*/

int lb1jp(struct CONNTYPE *ix) {

   long wx,wy;

   wx = dm64nq(ix->ul2.p.srcsiz, Lija.isyn+1, ix->nc);
   wy = Lija.boxe + jm64nh((wx-Lija.boxe)<<1, udev(&Lija.wlseed));
   Lija.boxe = wx;         /* Start at top of box next time */

   Lija.lijval = ix->ul1.g.wgrpad + wy;

   /* Check for out of bounds */
   if ((ulng)Lija.lijval >= (ulng)ix->srcnelt) return NO;

   return (Lija.lijval != Lija.satest);

   } /* End lb1jp() */


/*---------------------------------------------------------------------*
*     lb1n - Normal distribution around the target cell                *
*                                                                      *
*     (We want a true 2d distribution--coding postponed)               *
*---------------------------------------------------------------------*/

int lb1n(struct CONNTYPE *ix) {

   /* *** Temporary error exit *** */
   d3exit(fmturlnm(Lija.kl), LIJGEN_ERR, (int)ix->ict);

#ifndef GCC
   /* Logic never gets here, include return to get rid of warning */
   return NO;
#endif

   } /* End lb1n() */


/*---------------------------------------------------------------------*
*     lb1o - Uniformly chosen from groups other than current group     *
*---------------------------------------------------------------------*/

int lb1o(struct CONNTYPE *ix) {

   Lija.lijval = jm64nh(udev(&Lija.wlseed),ix->ul1.g.srcnel2) +
                 ix->ul1.g.wgrpad + ix->ul1.g.ogoff;

   /* If wrapped around, adjust base */
   if (Lija.lijval >= (long)ix->srcnelt)
      Lija.lijval -= (long)ix->srcnelt;

   if (Lija.jsyn == Lija.isas) lijxyr(ix);   /* Decode coords */

   return YES;

   } /* End lb1o() */


/*---------------------------------------------------------------------*
*     lb1op - Partitioned from groups other than target group          *
*                                                                      *
*     No need to save fstx,fsty--this routine is its own lb2 routine   *
*---------------------------------------------------------------------*/

int lb1op(struct CONNTYPE *ix) {

   long wx,wy;

   wx = dm64nq(ix->ul2.p.srcsiz, Lija.isyn+1, ix->nc);
   wy = Lija.boxe + jm64nh((wx-Lija.boxe)<<1, udev(&Lija.wlseed));
   Lija.boxe = wx;         /* Start at top of box next time */

   Lija.lijval = ix->ul1.g.wgrpad + ix->ul1.g.ogoff + wy;

   /* If wrapped around, adjust base */
   if (Lija.lijval >= (long)ix->srcnelt)
      Lija.lijval -= (long)ix->srcnelt;

   return YES;

   } /* End lb1op() */


/*---------------------------------------------------------------------*
*     lb1t - Topographic map from a REPERTOIRE or VG - new in V1K      *
*                                                                      *
*     Maps each target group onto a region of proportional             *
*        fractional size in the source.  A group in the region is      *
*        chosen with probability proportional to the fraction of       *
*        its area that is in the region.  Then a cell is chosen at     *
*        random from within the chosen group.  This algorithm          *
*        maintains an ordering relationship from group to group        *
*        but not from cell to cell within a group.  It guarantees      *
*        that every source cell has equal probability of being         *
*        used overall.                                                 *
*     V3A, 08/07/87, GNR - Modify to add offset, bounds checking.      *
*        Store X,Y coords for lb2 routines.  Correct so all            *
*        cells in a source group are positionally equivalent.          *
*        The sequence of multiplies and divides has been arranged      *
*        to minimize loss of random bits on the right.                 *
*     Since this routine can generate connections that are out of      *
*        bounds, it currently cannot be used with 'A'.                 *
*---------------------------------------------------------------------*/

int lb1t(struct CONNTYPE *ix) {

   ui32 low32a, low32b;

   Lija.lijval = Lija.fstr =
      jm64sb(udev(&Lija.wlseed),ix->srcnel,1,&low32a);
   Lija.fstx = (long)ix->xoff +
      jm64nb(ds64nq(Lija.groupx,low32a,-1,ix->ul1.s.fstngx),
             ix->ul1.s.sfx,&low32b);
   Lija.fsty = (long)ix->yoff +
      jm64nh(ds64nq(Lija.groupy,low32b,-1,ix->ul1.s.fstngy),
             ix->ul1.s.sfy);

   /* Skip the connection if X or Y out of bounds */
   if ((ulng)Lija.fstx >= (ulng)ix->iawxsz) return NO;
   if ((ulng)Lija.fsty >= (ulng)ix->iawysz) return NO;

   Lija.lijval += (long)ix->srcnel * (Lija.fsty*(long)ix->srcngx +
      Lija.fstx);

   return (Lija.lijval != Lija.satest);

   } /* End lb1t() */


/*---------------------------------------------------------------------*
*     lb1tia - Topographic map from the input array                    *
*                                                                      *
*     Rewritten in V2C to permit nonintegral mappings via random       *
*        selection from a box as in lb1t.  Earlier versions were       *
*        fully deterministic, with equal steps (unfortunate side       *
*        effect--this version is slower).                              *
*     This routine can generate connections that are out-of-bounds     *
*        given some combinations of the xoff,yoff,nux,nuy parameters.  *
*        These conns can be stored as origin points for lb2 routines,  *
*        but under the new rules, they must be rejected here.          *
*     Since this routine can generate connections that are out of      *
*        bounds, it currently cannot be used with 'A'.                 *
*     Bug fix in V6A--calculate new x,y coords on first cell on node,  *
*        even if it is not first cell in group.                        *
*---------------------------------------------------------------------*/

int lb1tia(struct CONNTYPE *ix) {

   /* fstnew is TRUE if a 1:1 mapping and this is first connection of
   *  first cell on this node, or first connection of first cell in a
   *  new group, or not a 1:1 mapping and this is any connection.  */
   if (ix->ul1.s.fstnew) {
      /* Calculate location of source for this cell or group */
      ui32 low32a;
      /* Note shift of udev result: sign bit is filled in */
      Lija.fstx = (long)ix->xoff + jm64nb(ds64nq((si32)Lija.groupx,
         ((ui32)udev(&Lija.wlseed))<<1,-1,ix->ul1.s.fstngx),
         ix->ul1.s.sfx,&low32a);
      Lija.fsty = (long)ix->yoff + jm64nh(ds64nq((si32)Lija.groupy,
            low32a,-1,ix->ul1.s.fstngy),ix->ul1.s.sfy);
      /* If in a 1:1 mapping, save coordinates and set flag to reuse */
      if (ix->cnflgs & STM11) {
         ix->ul1.s.fstsvx = Lija.fstx; ix->ul1.s.fstsvy = Lija.fsty;
         ix->ul1.s.fstnew = FALSE;
         }
      } /* End new source */

   else {
      /* Reuse same source cell if 1:1 mapping and not a new target
      *  group.  Note that this code does not advance wlseed, there-
      *  fore udevskip for first cell on node must compensate.  */
      Lija.fstx = ix->ul1.s.fstsvx; Lija.fsty = ix->ul1.s.fstsvy;
      }
   Lija.fstr = 0;

   /* Unsigned test should reject negatives */
   if ((ulng)Lija.fstx >= (ulng)ix->iawxsz) return NO;
   if ((ulng)Lija.fsty >= (ulng)ix->iawysz) return NO;

   Lija.lijval = (Lija.fstx << RP->ky1) + Lija.fsty;
   return YES;

   } /* End lb1tia() */


/*---------------------------------------------------------------------*
*     lb1tvg - Topographic map from virtual groups or TV               *
*                                                                      *
*     Since this routine can generate connections that are out of      *
*        bounds, it currently can not be used with 'A'.                *
*---------------------------------------------------------------------*/

int lb1tvg(struct CONNTYPE *ix) {

   ui32 low32a;

   Lija.fstx = (long)ix->xoff +
      jm64nb(ds64nq((si32)Lija.groupx,
         ((ui32)udev(&Lija.wlseed))<<1, -1,
         ix->ul1.s.fstngx), ix->ul1.s.sfx, &low32a);
   Lija.fsty = (long)ix->yoff + jm64nh(ds64nq((si32)Lija.groupy,
      low32a, -1, ix->ul1.s.fstngy), ix->ul1.s.sfy);
   Lija.fstr = 0;

   /* Skip the connection if X or Y out of bounds */
   if ((ulng)Lija.fstx >= (ulng)ix->iawxsz) return NO;
   if ((ulng)Lija.fsty >= (ulng)ix->iawysz) return NO;

   Lija.lijval = Lija.fsty*(long)ix->srcngx + Lija.fstx;
   return YES;

   } /* End lb1tvg() */


/*---------------------------------------------------------------------*
*     lb1u - Uniform random from anywhere in the layer                 *
*---------------------------------------------------------------------*/

int lb1u(struct CONNTYPE *ix) {

   Lija.lijval = jm64nh(udev(&Lija.wlseed),ix->ul1.g.srcnel2);

   if (Lija.jsyn == Lija.isas) lijxyr(ix);   /* Decode coords */

   return (Lija.lijval != Lija.satest);

   } /* End lb1u() */


/*---------------------------------------------------------------------*
*     lb1uia - Uniform input from anywhere in Input Array              *
*---------------------------------------------------------------------*/

int lb1uia(struct CONNTYPE *ix) {

   long work = jm64nh(udev(&Lija.wlseed),ix->ul1.g.srcnel2);

   if (Lija.isyn == 0) {
      Lija.fstx = work >> RP->ky;
      Lija.fsty = work & RP->ymask;
      Lija.fstr = 0;
      }
   Lija.lijval = (work & ~RP->ymask) + work;

   return YES;

   } /* End lb1uia() */


/*---------------------------------------------------------------------*
*     lb1uvg - Uniform input from virtual groups or TV                 *
*---------------------------------------------------------------------*/

int lb1uvg(struct CONNTYPE *ix) {

   Lija.lijval = jm64nh(udev(&Lija.wlseed),ix->ul1.g.srcnel2);

   if (Lija.jsyn == Lija.isas) lijxyr(ix);   /* Decode coords */

   return YES;

   } /* End lb1uvg() */


/*---------------------------------------------------------------------*
*     lb1up - Partitioned from entire layer                            *
*                                                                      *
*     No need to save fstx,fsty--this routine is its own lb2 routine   *
*---------------------------------------------------------------------*/

int lb1up(struct CONNTYPE *ix) {

   long wx;

   wx = dm64nq(ix->ul2.p.srcsiz, Lija.isyn+1, ix->nc);
   Lija.lijval = Lija.boxe +
      jm64nh((wx-Lija.boxe)<<1, udev(&Lija.wlseed));
   Lija.boxe = wx;         /* Start at top of box next time */

   return (Lija.lijval != Lija.satest);

   } /* End lb1up() */


/*---------------------------------------------------------------------*
*     lb1upia - Partitioned uniform input from Input Array             *
*                                                                      *
*     No need to save fstx,fsty--this routine is its own lb2 routine   *
*---------------------------------------------------------------------*/

int lb1upia(struct CONNTYPE *ix) {

   long wx,wy;

   wx = dm64nq(ix->ul2.p.srcsiz, Lija.isyn+1, ix->nc);
   wy = Lija.boxe + jm64nh((wx-Lija.boxe)<<1, udev(&Lija.wlseed));
   Lija.boxe = wx;         /* Start at top of box next time */

   Lija.lijval = (wy & ~RP->ymask) + wy;

   return YES;

   } /* End lb1upia() */


/*---------------------------------------------------------------------*
*     lb1upvg - Partitioned input from virtual groups or TV            *
*                                                                      *
*     No need to save fstx,fsty--this routine is its own lb2 routine.  *
*     Routine is same as lb1up except no self-avoidance test.          *
*---------------------------------------------------------------------*/

int lb1upvg(struct CONNTYPE *ix) {

   long wx;

   wx = dm64nq(ix->ul2.p.srcsiz, Lija.isyn+1, ix->nc);
   Lija.lijval = Lija.boxe +
      jm64nh((wx-Lija.boxe)<<1, udev(&Lija.wlseed));
   Lija.boxe = wx;         /* Start at top of box next time */

   return YES;

   } /* End lb1upvg() */


/*---------------------------------------------------------------------*
*     lb1x - Systematic input from a repertoire or virtual groups      *
*                                                                      *
*     Contrary to general policy, this does use wrap-around because    *
*        user controls incrementing, may want it.                      *
*---------------------------------------------------------------------*/

int lb1x(struct CONNTYPE *ix) {

   Lija.lijval = ix->ul1.x.cwrkoff;
   ix->ul1.x.cwrkoff += ix->ul1.x.incoffx + ix->ul1.x.incoffy;

   /* If wrapped around, adjust base */
   if (ix->ul1.x.cwrkoff >= (long)ix->srcnelt)
      ix->ul1.x.cwrkoff -= (long)ix->srcnelt;

   if (Lija.jsyn == Lija.isas) lijxyr(ix);   /* Decode coords */

   return (Lija.lijval != Lija.satest);  /* Always TRUE if VG input */

   } /* End lb1x() */


/*---------------------------------------------------------------------*
*     lb1xia - Systematic from input array                             *
*                                                                      *
*     Similar to lb1tia, but user controls offset directly.            *
*     Wrap-around is permitted, see note for lb1x.                     *
*---------------------------------------------------------------------*/

int lb1xia(struct CONNTYPE *ix) {

   long work;

   Lija.lijval = work = ix->ul1.x.cwrkoff;
#if 0   /*** DEBUG ***/
if (Lija.kl->seqvmo == 1 && ix->ict == 2) {
   long Lx = Lija.lijval >> RP->ky1;
   long Ly = Lija.lijval & RP->ymask;
   dbgprt(ssprintf(NULL, "Cell %ld, jsyn %ld, Lij = %ld, x = %ld, "
      "y = %ld", Lija.kcell, Lija.jsyn, Lija.lijval, Lx, Ly));
   }
#endif
   /* This is a shorter version of the lijwrap function in d3kij.c
   *  that is valid when incoffx and incoffy are known to involve
   *  increments of less than one full row or column, bzw.  */
   work += ix->ul1.x.incoffx + ix->ul1.x.incoffy + RP->xymaskc;
   if (work >= 0) work++;           /* X overflow, bump Y */
   ix->ul1.x.cwrkoff = work & RP->xymask;
   if (Lija.isyn == 0) {
      Lija.fstx = Lija.lijval >> RP->ky1;
      Lija.fsty = Lija.lijval & RP->ymask;
      Lija.fstr = 0;
      }

   return YES;

   } /* End lb1xia() */


/***********************************************************************
*     lijxyr - Decode first Lij into x,y,r coords for subarbor and lb2 *
***********************************************************************/

void lijxyr(struct CONNTYPE *ix) {

   div_t qrm;
   int work = (int)(Lija.lijval/(long)ix->srcnel);
   qrm = div(work, ix->srcngx);
   Lija.fstr = Lija.lijval - (long)work*(long)ix->srcnel;
   Lija.fstx = qrm.rem;
   Lija.fsty = qrm.quot;

   } /* End lijxyr() */


/***********************************************************************
*                                                                      *
*     Secondary Lij routines - Generation of connections after the     *
*                                first                                 *
*                                                                      *
*     lb2xx    Generate connections for rule xx.  For rule 'B', lb2b   *
*              assumes the first point (in lijval) is in bounds.       *
*     lbqb     For rule 'B', works from the coords in fstxy instead,   *
*              allowing first point to be out-of-bounds while          *
*              subsequent points can bleed back into bounds.           *
*                                                                      *
*     all lb2 routines must check for connections out-of-bounds        *
*                                                                      *
***********************************************************************/

/*---------------------------------------------------------------------*
*     lb2a - Adjacent cell in a regular or virtual repertoire or TV    *
*                                                                      *
*        lb2a has been defined to give helical boundary conditions.    *
*     Since this code cannot be guaranteed if the first connection     *
*     is out of bounds, code 'A' is illegal with any first-connection  *
*     code that can generate such connections (currently F,S,T).       *
*     This state of affairs must be reviewed when a user-selectable    *
*     boundary condition option is added.                              *
*                                                                      *
*     Rev, 12/10/87, GNR - Use true mod function so stride<0 is OK     *
*     V3F, 09/02/88, GNR - Use this code for VG input--                *
*---------------------------------------------------------------------*/

int lb2a(struct CONNTYPE *ix) {

   Lija.lijval = (Lija.lijval+ix->ul2.a.incr) % (long)ix->srcnelt;
   /* Make sure it's positive */
   if (Lija.lijval < 0) Lija.lijval += (long)ix->srcnelt;

   return YES;

   } /* End lb2a() */


/*---------------------------------------------------------------------*
*     lb2aia - Adjacent cell in input array                            *
*                                                                      *
*     Uses helical boundary conditions--when goes out-of-bounds        *
*        to right, moves to corresponding point in next row--          *
*        when goes out at bottom, moves up and over one column.        *
*     Test X overflow first, adjustment can cause y overflow.          *
*     (Stride is <= IA size, so can only go over by one unit.)         *
*                                                                      *
*     *************************************************************    *
*     ****  This code is not correct if an offset is added in  ****    *
*     ****  d3go.  To avoid this, 'A' is made illegal with 'S' ****    *
*     *************************************************************    *
*                                                                      *
*---------------------------------------------------------------------*/

int lb2aia(struct CONNTYPE *ix) {

   /* If out at the right, increment X */
   if ((Lija.lijval += ix->ul2.a.incr) > ix->ul2.a.lim)
      Lija.lijval -= ix->ul2.a.lim;
   /* Clean up coords */
   Lija.lijval = (Lija.lijval + RP->nsy) & RP->xymask;

   return YES;

   } /* End lb2aia() */


/*---------------------------------------------------------------------*
*     lb2b - 'Box' distribution in a cell layer or VT or TV            *
*          = rectangle with upper left corner at starting point        *
*                                                                      *
*     Case I (old style) version--the first connection must be         *
*        in bounds, otherwise lbqb should be called instead.           *
*     As per bounds checking rules in d3go, bounds are checked         *
*        here for repertoire input.  Any skips must be inserted        *
*        in correct order, as lb2b may be used with 'L' or 'M'         *
*        Cij generating codes.                                         *
*---------------------------------------------------------------------*/

int lb2b(struct CONNTYPE *ix) {

   long work;   /* Scratch space */

   if (Lija.jsyn == Lija.lb2s) {
      /* Here if first call to routine in any subarbor.  First Lij is
      *  supposed to be in bounds or lbqb should have been called, but
      *  this is tested here "just in case".  This code rewritten,
      *  08/09/06, to give development error instead of nonexistent
      *  old LIJ_SKIP_TYP, rename stlim,rowlim-->rowe,boxe.  */
      if ((ulng)Lija.lijval >= (ulng)ix->srcnelt)
         d3exit(fmturlnm(Lija.kl), LIJBND_ERR, (int)ix->ict);
      Lija.rowe = Lija.lijval + ix->ul2.b.rslen;
      work = ix->ul2.b.rrlen - (Lija.lijval%ix->ul2.b.rrlen);
      Lija.boxe = Lija.lijval + min(work, ix->ul2.b.rslen);
      }

   /* Advance horizontally, check for end of row or box */
   if ((Lija.lijval += ix->stride) >= Lija.rowe) {
      /* New row--undo the X motion.  Off the end of the world? */
      if ((Lija.lijval += ix->ul2.b.rrlen - ix->ul2.b.rslen) >=
            (long)ix->srcnelt) {
         /* Force otherLij to skip rest of conntype */
         Lija.jsyn = ix->nc; return NO; }
      Lija.boxe += ix->ul2.b.rrlen;
      Lija.rowe += ix->ul2.b.rrlen;
      }
   else if ((unsigned long)Lija.lijval >= Lija.boxe) return NO;

   return (Lija.lijval != Lija.satest);

   } /* End lb2b() */


/*---------------------------------------------------------------------*
*     lb2bia - 'Box' distribution in input array                       *
*                                                                      *
*     Rev, 06/23/08, GNR - Work with x,y coords, add bounds check per  *
*                          new rule, using lbqb code as a guide        *
*---------------------------------------------------------------------*/

int lb2bia(struct CONNTYPE *ix) {

   if (Lija.jsyn == Lija.lb2s) {
      /* Here if first call to routine in any subarbor */
      if (Lija.fstx >= (signed long)ix->iawxsz ||
            Lija.fsty >= (signed long)ix->iawysz ||
            Lija.fstx + ix->ul2.b.rslen <= 0 ||
            Lija.fsty + (long)ix->nry <= 0) {
         /* Force otherLij to skip rest of conntype */
         Lija.jsyn = ix->nc; return NO; }
      Lija.xcur = Lija.fstx;
      Lija.ycur = Lija.fsty;
      Lija.rowe = Lija.fstx + ix->ul2.b.rslen;
      Lija.boxe = Lija.fsty + (long)ix->nry;
      }

   if ((Lija.xcur += ix->stride) >= Lija.rowe) {
      Lija.xcur = Lija.fstx;
      if (++Lija.ycur >= Lija.boxe) {
         /* Should be impossible to get here if nc count is OK */
         d3exit(fmturlnm(Lija.kl), LIJBND_ERR, (int)ix->ict);
         }
      }

   if ((ulng)Lija.xcur >= (ulng)ix->iawxsz ||
       (ulng)Lija.ycur >= (ulng)ix->iawysz) return NO;

   Lija.lijval = Lija.xcur << RP->ky1 | Lija.ycur;
   return YES;

   } /* End lb2bia() */


/*---------------------------------------------------------------------*
*     lbqb - 'Box' distribution in a cell layer or VT or TV            *
*                                                                      *
*     Case II (new style) version--first connection may be out         *
*        of bounds but coords will be stored in fstx,fsty,fstr.        *
*        The bounds tests are considerably more complicated            *
*        than for lb2b because it is not enough to know that           *
*        lijval has strayed above or below the repertoire limits.      *
*     As per bounds checking rules in d3go, bounds are checked         *
*        here for repertoire or virtual cell input.  Any skips         *
*        must be inserted in correct order, as lbqb may be used        *
*        with 'L' or 'M' Cij generating codes.                         *
*---------------------------------------------------------------------*/

int lbqb(struct CONNTYPE *ix) {

   long work,work2;                 /* Scratch space */

   if (Lija.jsyn == Lija.lb2s) {
      /* Here if first call to routine in any subarbor */
      if ((Lija.fstx >= (long)ix->srcngx) ||
            (Lija.fsty >= (long)ix->srcngy)) {
         /* Force otherLij to skip rest of conntype */
         Lija.jsyn = ix->nc; return NO; }

      work = Lija.fsty*ix->ul2.b.rrlen;
      if (work <= ix->ul2.b.rtlen) {
         Lija.jsyn = ix->nc; return NO; }

      Lija.lijval = Lija.fstr + Lija.fstx*(long)ix->srcnel;
      if ((work2 = Lija.lijval + ix->ul2.b.rslen) <= 0) {
         Lija.jsyn = ix->nc; return NO; }

      Lija.lijval += work;       /* What Lij would be after lijbr1 */
      Lija.rowe = work + work2;     /* Save row loop limit */
      Lija.boxe = work;             /* Save left row boundary */
      } /* End if first call */

   /* Rev, 06/16/10, GNR - '>' here corrected to '>=' */
   if ((Lija.lijval += ix->stride) >= Lija.rowe) {
      /* Undo X motion and advance one along Y.  If runs off the
      *  bottom of the source array, skip rest of conntype.  */
      if ((Lija.lijval += ix->ul2.b.rrlen - ix->ul2.b.rslen) >=
            (long)ix->srcnelt) {
         Lija.jsyn = ix->nc; return NO; }
      Lija.rowe += ix->ul2.b.rrlen;
      Lija.boxe += ix->ul2.b.rrlen;
      }

   /* Skip if still below start of source array */
   if (Lija.lijval < 0 || Lija.lijval < Lija.boxe) return NO;

   /* If past end of current row, omit */
   if ((Lija.boxe + ix->ul2.b.rrlen) <= Lija.lijval) return NO;

   return (Lija.lijval != Lija.satest);

   } /* End lbqb() */


/*---------------------------------------------------------------------*
*     lb2c - 'Crow's foot' distribution within a normal repertoire     *
*                                                                      *
*     Distribution is centered on the cell selected first.             *
*     Any connections generated out-of-bounds are rejected.            *
*     Doesn't make sense to use with subarbors, so not coded.          *
*     (New version in V1K--selects a group in a square of size         *
*        nrx*nry and then selects any cell in that group)              *
*     (Changed 11/25/86 from upper-left to centered for OLS.  This     *
*        will not cause incompat with old decks because C didn't       *
*        exist before V1K--but real problem is to fix B)               *
*     (Changed 01/12/87 add rounding so result is symmetric)           *
*     (Changed 08/13/06 to remove assumption of nel < S16 and          *
*        ngrp < S15--results will differ from old runs.)               *
*     Same routine is now used for Case I and Case II                  *
*---------------------------------------------------------------------*/

int lb2c(struct CONNTYPE *ix) {

   long wx,wy;                /* Scratch space */
   ui32 low32a;

   /* Make a random number that's negative half the time */
   wx = jm64nb(udev(&Lija.wlseed)<<1,(si32)ix->nrx,&low32a);
   if (low32a & UI32_SGN) wx++;
   wx += Lija.fstx;
   /* Return NO if X out of range -- high or negative */
   if ((ulng)wx >= (ulng)ix->iawxsz) {
      udev(&Lija.wlseed);     /* Make deterministic */
      return NO; }

   /* Make another random number that's negative half the time */
   wy = jm64nb(udev(&Lija.wlseed)<<1,(si32)ix->nry,&low32a);
   if (low32a & UI32_SGN) wy++;
   wy += Lija.fsty;
   /* Return NO if Y out of range -- high or negative */
   if ((ulng)wy >= (ulng)ix->iawysz) return NO;

   Lija.lijval = (long)ix->srcnel*(wy*(long)ix->srcngx + wx) +
      uwhi(jmuw(low32a, ix->srcnel));

   return (Lija.lijval != Lija.satest);

   } /* End lb2c() */


/*---------------------------------------------------------------------*
*     lb2cia - 'Crow's foot' distribution in input array               *
*                                                                      *
*     Selects points uniformly distributed in an nrx*nry box centered  *
*        on the pixel chosen by the lb1 routine.  Revised in V2C to    *
*        have center of distribution, not corner, on first point       *
*        chosen.  This in turn requires rounding.                      *
*     The following comments are obsolete in light of the new bounds   *
*        checking rules:                                               *
*        (See design notes, p. 33, for an analysis showing that all    *
*        out-of-bounds points can be detected by d3go provided we      *
*        require nry <= nsy; ht <= nsy.)                               *
*        (There was a choice to be made, whether connections outside   *
*        the scan window but inside the input array should be rejected.*
*        In this code such connections are considered valid on grounds *
*        (1) probably more realistic biologically, (2) anyway, user    *
*        can get rid of them by using smaller nux, nuy, (3) allows     *
*        bia,bwa and cia,cwa to be the same routines.  Leading 1's in  *
*        negative Y's are kept--automatically corrects X in d3go.)     *
*     Currently using same routine for case I and case II.             *
*     Rev, 06/23/08, GNR - Restore bounds checking via iawxsz,iawysz   *
*     R73, 04/19/17, GNR - Use two udevs for uplijss consistent w/lb2c *
*---------------------------------------------------------------------*/

int lb2cia(struct CONNTYPE *ix) {

   long wx,wy;                /* Scratch space */
   ui32 low32x,low32y;

   wx = Lija.fstx + jm64nb(udev(&Lija.wlseed)<<1,(si32)ix->nrx,&low32x);
   if (low32x & UI32_SGN) wx++;
   if ((ulng)wx >= (ulng)ix->iawxsz) return NO;

   /* Make another random number that's negative half the time */
   wy = Lija.fsty + jm64nb(udev(&Lija.wlseed)<<1,(si32)ix->nry,&low32y);
   if (low32y & UI32_SGN) wy++;
   if ((ulng)wy >= (ulng)ix->iawysz) return NO;

   Lija.lijval = wx << RP->ky1 | wy;
   return YES;

   } /* End lb2cia() */


/*---------------------------------------------------------------------*
*     lb2cvg - 'Crow's foot' distribution within virtual touch or TV   *
*                                                                      *
*     Distribution is centered on the cell selected first.             *
*        Any connections generated out-of-bounds are rejected.         *
*        VT is treated as having special case--one cell/group.         *
*     Same routine is now used for Case I and Case II.                 *
*---------------------------------------------------------------------*/

int lb2cvg(struct CONNTYPE *ix) {

   long wx,wy;                /* Scratch space */
   ui32 low32a;

   /* Make a random number that is negative half the time */
   wx = jm64nb(udev(&Lija.wlseed)<<1,(si32)ix->nrx,&low32a);
   if (low32a & UI32_SGN) wx++;
   wx += Lija.fstx;
   /* Return NO if X out of range -- high or negative */
   if ((ulng)wx >= (ulng)ix->iawxsz) {
      udev(&Lija.wlseed);     /* Make deterministic */
      return NO; }

   /* Make another random number that's negative half the time */
   wy = jm64nb(udev(&Lija.wlseed)<<1,(si32)ix->nry,&low32a);
   if (low32a & UI32_SGN) wy++;
   wy += Lija.fsty;
   /* Return NO if Y out of range -- high or negative */
   if ((ulng)wy >= (ulng)ix->iawysz) return NO;

   Lija.lijval = wx + wy*(long)ix->srcngx;

   return YES;

   } /* End lb2cvg() */


/*---------------------------------------------------------------------*
*     lb2k - Generate a geometric kernel around the target cell.       *
*                                                                      *
*     This routine uses geometry information stored at ul2.k.pkern to  *
*     generate the same pattern of connections around each target cell.*
*     It performs bounds checking assuming all out-of-bounds conns are *
*     to be skipped (only BC=ZERO implemented).  It also leaves Cij in *
*     Lija.cijext.                                                     *                                                        *
*                                                                      *
*     It is necessary to test for the end of the pkern table because   *
*     nc is max over all target orientations. rkern cannot be advanced *
*     because it is only set once per cell in d3kiji, not in d3lijx--  *
*     therefore Lija.xcur is used to advance into the kernel table.    *
*                                                                      *
*     R78, 03/17/18, GNR - Revised to work with any lb1 routine, now   *
*        leaves applying offsets to the lb1 routine.  fstr is not      *
*        used, lb1 routine just picks a group.                         *
*---------------------------------------------------------------------*/

int lb2k(struct CONNTYPE *ix) {

   kern *pk = ix->ul2.k.rkern + Lija.xcur;
   si32 tx,ty;          /* This x,y */
   /* Already at the end of the kernel? */
   if (pk->ori < 0) return NO;

   /* Unpack the info for this row, column, and angle and skip
   *  the connection if row or column is out-of-bounds */
   tx = Lija.fstx + pk->kx;
   ty = Lija.fsty + pk->ky;
   Lija.xcur += 1;      /* Use next one next time */
   /* Unsigned test rejects negatives */
   if ((ui32)tx >= ix->iawxsz) return NO;
   if ((ui32)ty >= ix->iawysz) return NO;

   Lija.lijval = pk->ori + (long)ix->srcnel *
      (tx + ty*(long)ix->srcngx);
   Lija.cijext = pk->Cij;
   return YES;

   } /* End lb2k() */


/*---------------------------------------------------------------------*
*     lb2kia - Topographic map from the input array                    *
*                                                                      *
*     Simplified version of lb2k for input from input array, in case   *
*     we want to apply a kernel directly to an input array.  In this   *
*     case, the source has no orientation, and presumably srcnel == 1  *
*     and the d3lkni routine will have prepared pkern accordingly.     *
*---------------------------------------------------------------------*/

int lb2kia(struct CONNTYPE *ix) {

   kern *pk = ix->ul2.k.rkern + Lija.xcur;
   si32 tx,ty;          /* This x,y */
   /* Already at the end of the kernel? */
   if (pk->ori < 0) return NO;

   /* Unpack the info for this row, column, and angle and skip
   *  the connection if row or column is out-of-bounds */
   tx = Lija.fstx + pk->kx;
   ty = Lija.fsty + pk->ky;
   Lija.xcur += 1;      /* Use next one next time */
   /* Unsigned test rejects negatives */
   if ((ui32)tx >= (ulng)ix->iawxsz) return NO;
   if ((ui32)ty >= (ulng)ix->iawysz) return NO;

   Lija.lijval = (tx << RP->ky1) + ty;
   Lija.cijext = pk->Cij;
   return YES;

   } /* End lb2kia() */


/*---------------------------------------------------------------------*
*     lb2kvg - Topographic map from virtual groups or TV               *
*                                                                      *
*     This routine will be used to apply a connection kernel directly  *
*     to an image.  Although image pixels do not intrinsically have    *
*     orientation, one might apply filters to obtain pixels that       *
*     reflect energy in certain orientation directions, as in Piech,   *
*     Reeke, and Gilbert, PNAS E4108 (2013).  This code will need to   *
*     be revised to reflect the organization in memory of such images, *
*     but for now, we assume srcnel gives the number of orientations   *
*     and this selects a whole plane of the image.  The 'orientation'  *
*     index could also be used to select a color.                      *
*---------------------------------------------------------------------*/

int lb2kvg(struct CONNTYPE *ix) {

   kern *pk = ix->ul2.k.rkern + Lija.xcur;
   si32 tx,ty;          /* This x,y */
   /* Already at the end of the kernel? */
   if (pk->ori < 0) return NO;

   /* Unpack the info for this row, column, and angle and skip
   *  the connection if row or column is out-of-bounds */
   tx = Lija.fstx + pk->kx;
   ty = Lija.fsty + pk->ky;
   Lija.xcur += 1;      /* Use next one next time */
   /* Unsigned test rejects negatives */
   if ((ui32)tx >= (ulng)ix->iawxsz) return NO;
   if ((ui32)ty >= (ulng)ix->iawysz) return NO;

   Lija.lijval = pk->ori * (long)ix->srcngx * (long)ix->srcngy +
      (long)ix->srcnel * (tx + ty*(long)ix->srcngx);
   Lija.cijext = pk->Cij;
   return YES;

   } /* End lb2kvg() */


/*---------------------------------------------------------------------*
*     lb2q -  Annular connection distribution with possible hole       *
*                                                                      *
*     Selects with uniform probability from within an ellipsoidal      *
*        annulus with principle axes nrx * nry groups and possibly     *
*        including an ellipsoidal hole with inner dimensions nhx *     *
*        nhy groups centered on the cell selected first.               *
*        A cell is chosen at random from the selected group.           *
*        Any connections generated out-of-bounds are rejected.         *
*     Similarly to the lb2c routines, no provision is made to clone    *
*        Lij selections across subarbors.                              *
*     N.B.  To save detailed geometric calculations on every call,     *
*        a list of acceptable x,y offsets from the center point is     *
*        generated by d3lani at d3genr time and stored at ul2.q.paxy.  *
*        All this routine needs to do is select a random x,y from      *
*        this table and then a random cell from within the chosen      *
*        group (skipped if lb2qvg is called).                          *
*---------------------------------------------------------------------*/

int lb2q(struct CONNTYPE *ix) {

   xysh *pat;
   si32 rcs;                     /* Random cell selector */
   si32 ox,oy;                   /* Offset x coord, y coord */
   ui32 wx,wy;                   /* x,y offset by first point */

   /* Get x,y from a randomly selected paxy table entry */
   pat = ix->ul2.q.paxy + (udev(&Lija.wlseed) % ix->ul2.q.lannt);
   ox = pat->sx, oy = pat->sy;

   /* Deterministically get second random number whether used or not */
   rcs = udev(&Lija.wlseed);

   /* Check bounds.  As usual, unsigned test eliminates negatives */
   wx = (ui32)(Lija.fstx + ox);
   if (wx >= ix->iawxsz) return NO;
   wy = (ui32)(Lija.fsty + oy);
   if (wy >= ix->iawysz) return NO;

   Lija.lijval = (long)ix->srcnel*(wx + (long)ix->srcngx*wy) +
      (rcs % ix->srcnel);

   return YES;

   } /* End lb2q() */


/*---------------------------------------------------------------------*
*     lb2qia - Annular connection selection from input array           *
*                                                                      *
*     Selects with uniform probability from within an annulus of       *
*        inner axes nhx * nhy pixels and outer axes nrx * nry pixels   *
*        centered on the pixel selected first.                         *
*     Similarly to the lb2c routines, no provision is made to clone    *
*        Lij selections across subarbors.                              *
*---------------------------------------------------------------------*/

int lb2qia(struct CONNTYPE *ix) {

   xysh *pat;
   si32 ox,oy;                   /* Offset x coord, y coord */
   ui32 wx,wy;                   /* x,y offset by first point */

   /* Get x,y from a randomly selected paxy table entry */
   pat = ix->ul2.q.paxy + (udev(&Lija.wlseed) % ix->ul2.q.lannt);
   ox = pat->sx, oy = pat->sy;

   /* Check bounds.  As usual, unsigned test eliminates negatives */
   wx = (ui32)(Lija.fstx + ox);
   if (wx >= ix->iawxsz) return NO;
   wy = (ui32)(Lija.fsty + oy);
   if (wy >= ix->iawysz) return NO;

   Lija.lijval = wx << RP->ky1 | wy;

   return YES;

   } /* End lb2qia() */


/*---------------------------------------------------------------------*
*     lb2qvg - Annular connection selection from virtual groups        *
*                                                                      *
*     Selects with uniform probability from within an annulus of       *
*        inner axes nhx * nhy pixels and outer axes nrx * nry pixels   *
*        centered on the pixel selected first.                         *
*     Similarly to the lb2c routines, no provision is made to clone    *
*        Lij selections across subarbors.                              *
*---------------------------------------------------------------------*/

int lb2qvg(struct CONNTYPE *ix) {

   xysh *pat;
   si32 ox,oy;                   /* Offset x coord, y coord */
   ui32 wx,wy;                   /* x,y offset by first point */

   /* Get x,y from a randomly selected paxy table entry */
   pat = ix->ul2.q.paxy + (udev(&Lija.wlseed) % ix->ul2.q.lannt);
   ox = pat->sx, oy = pat->sy;

   /* Check bounds.  As usual, unsigned test eliminates negatives */
   wx = (ui32)(Lija.fstx + ox);
   if (wx >= ix->iawxsz) return NO;
   wy = (ui32)(Lija.fsty + oy);
   if (wy >= ix->iawysz) return NO;

   Lija.lijval = (long)wx + (long)ix->srcngx*(long)wy;

   return YES;

   } /* End lb2qvg() */


/*---------------------------------------------------------------------*
*     lbsa - Subarbor generation routine--repertoire or VG source      *
*                                                                      *
*     When connection number is not a multiple of ix->nsa, executes    *
*        normal lb2 routine via pointer in lijbr3.  Otherwise,         *
*        advances to next subarbor, modifying lijval, fstx, fsty, and  *
*        fstr so that subsequent lb2 generation will proceed from the  *
*        new origin.                                                   *
*---------------------------------------------------------------------*/

int lbsa(struct CONNTYPE *ix) {

   /* If not at end of subarbor, execute lijbr3 routine */
   if (Lija.jsyn < Lija.nsas) return ix->lijbr3(ix);

   Lija.lb2s += (long)ix->nsa;
   Lija.isas = Lija.nsas;
   Lija.nsas += (long)ix->nsa;

   /* Time to advance.  Works like lbqb, except box width is
   *  in nsax and design optimizes memory rather than speed.  */

   Lija.wlseed = Lija.svlseed;            /* Reuse seed sequence */
   if (++Lija.jsa < (long)ix->nsax)
      ++Lija.fstx;                        /* Increment along row */
   else {
      ++Lija.fsty;                        /* Increment to next row */
      Lija.fstx -= ((long)ix->nsax - 1);  /* Restart row position */
      Lija.jsa = 0;                       /* Restart row counter */
      }

   /* Check for out of bounds, then convert coords to Lij */
   if ((ulng)Lija.fstx >= (ulng)ix->iawxsz) return NO;
   if ((ulng)Lija.fsty >= (ulng)ix->iawysz) return NO;
   Lija.lijval = Lija.fstr + (long)ix->srcnel *
      (Lija.fstx + (long)ix->srcngx * Lija.fsty);

   return YES;

   } /* End lbsa() */


/*---------------------------------------------------------------------*
*     lbsaia - Subarbor generation routine--input array source         *
*                                                                      *
*     When connection number is not a multiple of ix->nsa, executes    *
*        normal lb2 routine via pointer in lijbr3.  Otherwise,         *
*        advances to next subarbor, modifying lijval, fstx, fsty, and  *
*        fstr so that subsequent lb2 generation will proceed from the  *
*        new origin.                                                   *
*     Per usual IA input conventions, no bounds checking.              *
*---------------------------------------------------------------------*/

int lbsaia(struct CONNTYPE *ix) {

   /* If not at end of subarbor, execute lijbr3 routine */
   if (Lija.jsyn < Lija.nsas) return ix->lijbr3(ix);

   Lija.lb2s += (long)ix->nsa;
   Lija.isas = Lija.nsas;
   Lija.nsas += (long)ix->nsa;

   /* Time to advance.  Works like lbqb, except box width is in
   *  nsax and design optimizes memory rather than speed.  */

   Lija.wlseed = Lija.svlseed;            /* Reuse seed sequence */
   if (++Lija.jsa < (long)ix->nsax)
      ++Lija.fstx;                        /* Increment along row */
   else {
      ++Lija.fsty;                        /* Increment to next row */
      Lija.fstx -= ((long)ix->nsax - 1);  /* Restart row position */
      Lija.jsa = 0;                       /* Restart row counter */
      }

   Lija.lijval = ((Lija.fstx << RP->ky1) + Lija.fsty) & RP->xymask;

   return YES;

   } /* End lbsaia() */


/*---------------------------------------------------------------------*
*            lbsai - Subarbor generation routine--Option I             *
*                                                                      *
*     Treats each subarbor as an independent arbor, running the lb1    *
*        routine on the first connection and the normal lb2 routine    *
*        via the pointer in lijbr3 for the rest.                       *
*---------------------------------------------------------------------*/

int lbsai(struct CONNTYPE *ix) {

   /* If not at end of subarbor, execute lijbr3 routine */
   if (Lija.jsyn < Lija.nsas) return ix->lijbr3(ix);

   /* New subarbor.  Update synapse count, run lijbr1 routine */
   Lija.lb2s += (long)ix->nsa;
   Lija.isas = Lija.nsas;
   Lija.nsas += (long)ix->nsa;

   return ix->lijbr1(ix);

   } /* End lbsai() */


/*---------------------------------------------------------------------*
*            lbsal - Subarbor generation routine--Option L             *
*                                                                      *
*     When connection number is not a multiple of ix->nsa, repeats     *
*     same Lij obtained last time.  Otherwise, performs normal lb2     *
*     routine.  This is basically intended to allow 3 consecutive      *
*     connections to sample three colors from the same input, but      *
*     there may be other uses, so it is allowed with REPSRC input.     *
*---------------------------------------------------------------------*/

int lbsal(struct CONNTYPE *ix) {

   /* If not at end of subarbor, return previous result */
   if (Lija.jsyn < Lija.nsas) return Lija.lijrc;

   /* Advance to next subarbor, but do not advance Lija.lb2s,
   *  because lb2 routines are cycling just once in this case.  */
   Lija.isas = Lija.nsas;
   Lija.nsas += (long)ix->nsa;

   /* Now execute normal lb2 routine */
   return ix->lijbr3(ix);

   } /* End lbsal() */

